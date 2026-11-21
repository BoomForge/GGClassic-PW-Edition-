#!/usr/bin/env python3
"""Build a local-only PW Edition patch ZIP for an owned GameGuru Classic installation."""
import argparse
import shutil
import sys
from pathlib import Path
from make_pw_release import make_release

COMPANIONS = (
    "Files/effectbank/reloaded/apbr_animwithtran_emissive.fx",
    "Files/scriptbank/global.lua",
    "Files/scriptbank/pw_transition_state.lua",
)

def make_overlay(build: Path, version: str, out: Path, with_ide=False):
    extras = list(COMPANIONS) + (["GameGuru.exe"] if with_ide else [])
    missing = [item for item in extras if not (build / item).is_file()]
    if missing:
        raise FileNotFoundError("Missing built files: " + ", ".join(missing))
    archive, checksum = make_release(build, version, out, extras)
    shutil.copy2(Path(__file__).parent / "install_pw_steam_overlay.py", out)
    return archive, checksum

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--build-dir", required=True, type=Path)
    p.add_argument("--version", required=True)
    p.add_argument("--out-dir", type=Path, default=Path("dist"))
    p.add_argument("--with-ide", action="store_true")
    a = p.parse_args()
    try:
        archive, checksum = make_overlay(a.build_dir, a.version, a.out_dir, a.with_ide)
    except (OSError, ValueError) as exc:
        print("Packaging failed:", exc, file=sys.stderr)
        return 1
    print("Local package:", archive, "Checksum:", checksum)
    print("Preview with install_pw_steam_overlay.py, then explicitly select --apply.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
