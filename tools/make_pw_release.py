#!/usr/bin/env python3
"""Create an explicitly-scoped GGClassic PW Edition update ZIP and SHA256 sidecar.

Example:
  python tools/make_pw_release.py --build-dir "D:\Builds\GGClassic" --version 0.1.0 --out-dir dist
  python tools/make_pw_release.py --build-dir "D:\Builds\GGClassic" --version 0.1.1 --out-dir dist --include MyPWPlugin.dll

This NEVER packages user levels, entitybank, terrainbank, purchased GameGuru media,
configuration files, or source. Only opt-in binary/shader files are permitted.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path, PurePosixPath

ASSET = "GGClassic-PW-Edition-win64.zip"
RELEASE_VERSION = re.compile(r"^\d+\.\d+\.\d+$")
ALLOWED = re.compile(
    r"(?i)^(Guru-MapEditor\.exe|GameGuru\.exe|pw-updater\.ps1|"
    r"pw-edition-version\.txt|[a-z0-9_.-]+\.dll|"
    r"Files/effectbank/reloaded/[a-z0-9_.-]+\.fx)$"
)
HERE = Path(__file__).resolve().parent


def check_name(value: str) -> str:
    name = value.replace("\\", "/")
    parts = name.split("/")
    if (
        not name
        or name.startswith("/")
        or ":" in name
        or any(part in ("", ".", "..") or part.endswith((".", " ")) for part in parts)
        or not ALLOWED.fullmatch(name)
    ):
        raise ValueError(f"Unsafe or unapproved release file: {value!r}")
    return name


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def make_release(build: Path, version: str, output: Path, extras: list[str]) -> tuple[Path, Path]:
    if not RELEASE_VERSION.fullmatch(version):
        raise ValueError("Version must be exactly major.minor.patch (e.g. 0.1.0)")
    if not build.is_dir():
        raise FileNotFoundError(f"Build directory not found: {build}")
    payload: dict[str, bytes] = {}
    engine = build / "Guru-MapEditor.exe"
    if not engine.is_file():
        raise FileNotFoundError(f"No built Guru-MapEditor.exe at {engine}")
    payload["Guru-MapEditor.exe"] = engine.read_bytes()
    payload["pw-updater.ps1"] = (HERE / "pw-updater.ps1").read_bytes()
    payload["pw-edition-version.txt"] = (version + "\n").encode("ascii")

    for item in extras:
        name = check_name(item)
        if name in payload:
            raise ValueError(f"File is already included: {name}")
        source = (build / Path(name)).resolve()
        # A simple path check complements the strict release filename whitelist.
        if not source.is_relative_to(build.resolve()):
            raise ValueError(f"Refusing file outside build directory: {name}")
        if not source.is_file():
            raise FileNotFoundError(f"Extra release file missing: {source}")
        payload[name] = source.read_bytes()

    manifest = {
        "version": version,
        "files": [
            {"path": name, "sha256": digest(data)}
            for name, data in sorted(payload.items())
        ],
    }
    output.mkdir(parents=True, exist_ok=True)
    archive = output / ASSET
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6, allowZip64=True) as zipped:
        zipped.writestr("pw-release.json", json.dumps(manifest, indent=2) + "\n")
        for name, data in sorted(payload.items()):
            zipped.writestr(PurePosixPath(name).as_posix(), data)

    checksum = output / (ASSET + ".sha256")
    checksum.write_text(digest(archive.read_bytes()) + "\n", encoding="ascii")
    return archive, checksum


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path, help="Folder with compiled Guru-MapEditor.exe")
    parser.add_argument("--version", required=True, help="Release semver, e.g. 0.1.0")
    parser.add_argument("--out-dir", required=True, type=Path, help="Output folder for GitHub release assets")
    parser.add_argument("--include", action="append", default=[],
                        help="Optional relative DLL / effectbank FX path; repeat for each file")
    args = parser.parse_args()
    try:
        archive, checksum = make_release(args.build_dir, args.version, args.out_dir, args.include)
    except (OSError, ValueError) as exc:
        print(f"Release error: {exc}", file=sys.stderr)
        return 1
    print("Created:", archive)
    print("Checksum:", checksum)
    print("Publish a GitHub release tagged v" + args.version)
    print("Attach BOTH files to that release. Do not upload original GameGuru media.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
