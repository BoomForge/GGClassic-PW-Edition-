#!/usr/bin/env python3
"""Compare explicit baseline and experimental gameplay benchmark CSVs.
No third-party dependencies. No fabricated metrics or automatic pass on no data.
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path

EDGES = [5, 8, 10, 12, 16, 20, 25, 33, 50, 100, 250]
BINS = ["le" + str(n) for n in EDGES] + ["gt250"]
COUNTERS = ["admitted", "demotions", "promotions", "pins", "failures", "shrink_bytes"]
FIELDS = ["generation", "tick_ms", "stage_on", "tiled_on", "broad_on", "dxgi_valid",
          "dxgi_usage", "dxgi_budget", "ram_valid", "working_set", "private_bytes",
          "pool_bytes", "cpu_backing_bytes", "frame_count", "frame_sum_ms", "frame_max_ms"] + COUNTERS + BINS

def percentile_interval(bins, quantile, maximum):
    total = sum(bins)
    if total <= 0:
        raise ValueError("No frame observations")
    rank = math.ceil(total * quantile)
    cumulative = 0
    for i, count in enumerate(bins):
        cumulative += count
        if cumulative >= rank:
            return [EDGES[i - 1] if i else 0,
                    min(maximum, EDGES[i] if i < len(EDGES) else maximum)]
    raise ValueError("Invalid histogram")

def load(path, generation, expected_mode):
    with Path(path).open(newline="", encoding="utf-8-sig") as source:
        reader = csv.DictReader(source)
        if not set(FIELDS).issubset(reader.fieldnames or []):
            raise ValueError(f"{path}: missing required benchmark columns")
        raw = list(reader)
    rows = []
    for row in raw:
        if int(row["generation"]) != generation:
            continue
        values = {k: (float(row[k]) if k in ("frame_sum_ms", "frame_max_ms") else int(row[k])) for k in FIELDS}
        if any(not math.isfinite(v) or v < 0 for v in values.values()):
            raise ValueError(f"{path}: invalid numeric values")
        if any(values[k] not in (0, 1) for k in ("stage_on", "tiled_on", "broad_on", "dxgi_valid", "ram_valid")):
            raise ValueError(f"{path}: invalid flag")
        if values["frame_count"] <= 0 or sum(values[k] for k in BINS) != values["frame_count"]:
            raise ValueError(f"{path}: inconsistent frame histogram")
        if expected_mode == "baseline":
            if any(values[k] for k in ("stage_on", "tiled_on", "broad_on", "pool_bytes", "cpu_backing_bytes")):
                raise ValueError(f"{path}: baseline must have all streaming features disabled")
        elif not values["stage_on"] or not values["tiled_on"]:
            raise ValueError(f"{path}: experimental mode not active (check GPU capability and flags)")
        if rows:
            if values["tick_ms"] <= rows[-1]["tick_ms"]:
                raise ValueError(f"{path}: duplicate or unordered timestamps")
            if any(values[k] < rows[-1][k] for k in COUNTERS):
                raise ValueError(f"{path}: counters reset within a generation")
        rows.append(values)
    if not rows:
        raise ValueError(f"{path}: no samples for generation {generation}")
    return rows

def summarize(rows):
    bins = [sum(r[k] for r in rows) for k in BINS]
    count = sum(bins)
    total = sum(r["frame_sum_ms"] for r in rows)
    maximum = max(r["frame_max_ms"] for r in rows)
    gpu = [r["dxgi_usage"] for r in rows if r["dxgi_valid"] and r["dxgi_budget"]]
    ram = [r["private_bytes"] for r in rows if r["ram_valid"]]
    def mem(values):
        return {"median_mib": statistics.median(values) / 1048576,
                "peak_mib": max(values) / 1048576, "samples": len(values)} if values else None
    return {"windows": len(rows), "frames": count, "sampled_seconds": total / 1000,
            "mean_interval_ms": total / count, "p95_interval_ms_range": percentile_interval(bins, .95, maximum),
            "max_interval_ms": maximum, "dxgi_local_usage": mem(gpu), "private_ram": mem(ram),
            "peak_pool_mib": max(r["pool_bytes"] for r in rows) / 1048576,
            "peak_cpu_backing_mib": max(r["cpu_backing_bytes"] for r in rows) / 1048576,
            "counters": {k: rows[-1][k] for k in COUNTERS}}

def compare(baseline, experimental):
    a, b = summarize(baseline), summarize(experimental)
    warnings = []
    for label, run in (("baseline", a), ("experimental", b)):
        if run["sampled_seconds"] < 30:
            warnings.append(label + " shorter than 30 seconds")
        if run["dxgi_local_usage"] is None or run["private_ram"] is None:
            warnings.append(label + " lacks valid memory telemetry")
    if abs(a["sampled_seconds"] - b["sampled_seconds"]) > .1 * max(a["sampled_seconds"], b["sampled_seconds"]):
        warnings.append("Run durations differ by more than 10%")
    counts = b["counters"]
    if not all(counts[k] > 0 for k in ("admitted", "demotions", "promotions", "shrink_bytes")):
        warnings.append("Experimental run did not demonstrate admission, shrink AND restoration")
    if counts["failures"]:
        warnings.append("Experimental run recorded residency failures")
    gpu_a, gpu_b = a["dxgi_local_usage"], b["dxgi_local_usage"]
    delta = gpu_b["median_mib"] - gpu_a["median_mib"] if gpu_a and gpu_b else None
    return {"baseline": a, "experimental": b, "experimental_minus_baseline_dxgi_median_mib": delta,
            "experimental_minus_baseline_mean_interval_ms": b["mean_interval_ms"] - a["mean_interval_ms"],
            "warnings": warnings, "evidence_ready_for_review": not warnings,
            "acceptance": "Not automatic. Match map, camera route, hardware, settings and visual results; repeat A/B at least three times."}

def self_test():
    from tempfile import TemporaryDirectory
    def example(mode):
        r = {k: 0 for k in FIELDS}
        r.update(generation=1, tick_ms=1000, dxgi_valid=1, dxgi_usage=100*1048576,
                 dxgi_budget=1000*1048576, ram_valid=1, private_bytes=200*1048576,
                 frame_count=2000, frame_sum_ms=32000, frame_max_ms=16, le16=2000)
        if mode:
            r.update(stage_on=1, tiled_on=1, admitted=1, demotions=1, promotions=1,
                     shrink_bytes=1048576, pool_bytes=1048576, dxgi_usage=99*1048576)
        return r
    with TemporaryDirectory() as folder:
        paths = [Path(folder) / (str(i)+".csv") for i in range(2)]
        for i, path in enumerate(paths):
            with path.open("w", newline="") as f:
                writer = csv.DictWriter(f, FIELDS)
                writer.writeheader()
                writer.writerow(example(i))
        a, b = load(paths[0], 1, "baseline"), load(paths[1], 1, "experimental")
        report = compare(a, b)
        assert report["evidence_ready_for_review"]
        assert report["experimental_minus_baseline_dxgi_median_mib"] == -1
        assert report["baseline"]["p95_interval_ms_range"] == [12, 16]
        b[0]["demotions"] = 0
        assert not compare(a, b)["evidence_ready_for_review"]
        try:
            load(paths[1], 1, "baseline")
        except ValueError:
            pass
        else:
            raise AssertionError("Experimental CSV incorrectly accepted as baseline")
    print("Synthetic parser/summary checks passed; no hardware evidence generated.")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline")
    parser.add_argument("--experimental")
    parser.add_argument("--baseline-generation", type=int, default=1)
    parser.add_argument("--experimental-generation", type=int, default=1)
    parser.add_argument("--output")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.baseline or not args.experimental:
        parser.error("Provide both CSV paths or --self-test")
    try:
        report = compare(load(args.baseline, args.baseline_generation, "baseline"),
                         load(args.experimental, args.experimental_generation, "experimental"))
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.exit(2, f"Invalid benchmark evidence: {exc}\n")
    result = json.dumps(report, indent=2, allow_nan=False)
    if args.output:
        Path(args.output).write_text(result + "\n", encoding="utf-8")
    print(result)
    return 0 if report["evidence_ready_for_review"] else 3

if __name__ == "__main__":
    raise SystemExit(main())
