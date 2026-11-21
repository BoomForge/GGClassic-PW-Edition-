#!/usr/bin/env python3
"""Summarise opt-in GGClassic PW Edition load-time traces.

Run after setting GGCPW_PROFILE_LOAD=1 and launching a PW Edition engine build:
    python tools/analyse_pw_loadtrace.py "%TEMP%\\GGCPW-loadtrace-1234.csv"

The report uses inclusive scope durations. Nested stages overlap: do not sum
all row times and claim that number is total load time.
"""
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path


def read_trace(path: Path) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    with path.open(newline="", encoding="utf-8-sig") as file:
        reader = csv.DictReader(file)
        required = {"tick_ms", "pid", "stage", "duration_ms"}
        if not required.issubset(set(reader.fieldnames or [])):
            raise ValueError(f"{path}: missing expected CSV columns")
        for row in reader:
            try:
                duration = float(row["duration_ms"])
                pid = int(row["pid"])
                tick = int(row["tick_ms"])
            except (ValueError, TypeError) as exc:
                raise ValueError(f"{path}: malformed trace row") from exc
            if not math.isfinite(duration) or duration < 0:
                raise ValueError(f"{path}: negative/non-finite duration")
            stage = str(row["stage"]).strip()
            if not stage:
                raise ValueError(f"{path}: empty stage")
            rows.append({"stage": stage, "duration_ms": duration, "pid": pid, "tick_ms": tick})
    return rows


def aggregate(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    groups: dict[str, list[float]] = defaultdict(list)
    for row in rows:
        stage = str(row["stage"])
        # begin.* survives an incomplete/stalled phase; slow.* flags >10s.
        # They are diagnostics, not additional completed duration samples.
        if stage.startswith(("begin.", "slow.")):
            continue
        groups[stage].append(float(row["duration_ms"]))
    result: list[dict[str, object]] = []
    for stage, durations in groups.items():
        samples = sorted(durations)
        p95_index = math.ceil(0.95 * len(samples)) - 1
        result.append({
            "stage": stage,
            "count": len(samples),
            "mean_ms": statistics.fmean(samples),
            "median_ms": statistics.median(samples),
            "p95_ms": samples[p95_index],
            "max_ms": samples[-1],
        })
    return sorted(result, key=lambda x: float(x["median_ms"]), reverse=True)


def report(paths: list[Path], top: int = 30) -> str:
    data: list[dict[str, object]] = []
    for path in paths:
        data.extend(read_trace(path))
    if not data:
        return "No trace samples found."
    lines = [
        f"PW Edition load profiler: {len(data)} events from {len(paths)} file(s)",
        "Stage                                        Count  Mean ms  Median ms   P95 ms   Max ms",
        "-" * 98,
    ]
    for entry in aggregate(data)[:top]:
        lines.append(
            f"{str(entry['stage'])[:44]:44} {int(entry['count']):5d}"
            f" {float(entry['mean_ms']):8.1f} {float(entry['median_ms']):10.1f}"
            f" {float(entry['p95_ms']):8.1f} {float(entry['max_ms']):8.1f}"
        )
    lines.append("")
    lines.append("Times are inclusive. Parent scopes contain child scopes; do not add them together.")
    lines.append("Compare an empty level and your small town level, cold launch vs repeat Test Level.")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="+", type=Path, help="One or more PW loadtrace CSV files")
    parser.add_argument("--top", type=int, default=30, help="Number of ranked phases to display")
    options = parser.parse_args()
    try:
        print(report(options.logs, options.top))
    except (OSError, ValueError) as exc:
        parser.exit(1, f"Trace analysis failed: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
