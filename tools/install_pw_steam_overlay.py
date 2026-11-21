#!/usr/bin/env python3
"""Verify and safely apply a PW patch to an existing GameGuru Steam directory."""
import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

ALLOW = re.compile(
    r"(?i)^(Guru-MapEditor\.exe|GameGuru\.exe|pw-updater\.ps1|pw-edition-version\.txt|"
    r"[a-z0-9_.-]+\.dll|Files/effectbank/reloaded/[a-z0-9_.-]+\.fx|"
    r"Files/scriptbank/(?:global|pw_transition_state)\.lua)$")
REQUIRED = {"Guru-MapEditor.exe", "pw-updater.ps1", "pw-edition-version.txt"}

def allowed(name):
    return (isinstance(name, str) and bool(ALLOW.fullmatch(name)) and
            "\\" not in name and all(
                x not in ("", ".", "..") and not x.endswith((".", " "))
                for x in name.split("/")))

def sha(data):
    return hashlib.sha256(data).hexdigest()

def verify_package(archive: Path):
    digest = archive.with_name(archive.name + ".sha256").read_text(encoding="ascii").strip()
    if not re.fullmatch(r"[0-9a-fA-F]{64}", digest) or sha(archive.read_bytes()) != digest.lower():
        raise ValueError("Archive SHA-256 mismatch")
    with zipfile.ZipFile(archive) as zipped:
        infos = zipped.infolist()
        names = [x.filename for x in infos]
        if len(names) != len(set(names)) or any(
            x.is_dir() or ((x.external_attr >> 16) & 0o170000) == 0o120000 for x in infos):
            raise ValueError("Duplicate entry, directory or symlink")
        if "pw-release.json" not in names:
            raise ValueError("Missing release manifest")
        manifest = json.loads(zipped.read("pw-release.json"))
        if not isinstance(manifest, dict) or not re.fullmatch(
            r"\d+\.\d+\.\d+", str(manifest.get("version", ""))):
            raise ValueError("Invalid package version")
        if not isinstance(manifest.get("files"), list):
            raise ValueError("Invalid manifest")
        payload = {}
        for item in manifest["files"]:
            if not isinstance(item, dict):
                raise ValueError("Malformed manifest item")
            name, digest = item.get("path"), item.get("sha256")
            if not allowed(name) or name not in names or name in payload:
                raise ValueError("Unsafe, duplicate or missing file: " + repr(name))
            if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
                raise ValueError("Invalid file hash")
            data = zipped.read(name)
            if sha(data) != digest.lower():
                raise ValueError("File hash mismatch: " + name)
            payload[name] = data
        if set(names) != set(payload) | {"pw-release.json"} or not REQUIRED.issubset(payload):
            raise ValueError("Unexpected ZIP file or missing required engine file")
        if payload["pw-edition-version.txt"].decode("ascii").strip() != manifest["version"]:
            raise ValueError("Version marker mismatch")
        return payload

def is_within(path, root):
    return path == root or root in path.parents

def apply_overlay(payload, steam: Path, backup: Path, apply=False):
    steam = steam.resolve(strict=True)
    if not steam.is_dir() or not (
        (steam / "GameGuru.exe").is_file() or (steam / "Guru-MapEditor.exe").is_file()):
        raise ValueError("Not a GameGuru installation folder")
    backup = backup.resolve()
    if is_within(backup, steam):
        raise ValueError("Backups must be outside the Steam folder")
    targets = []
    for name in sorted(payload):
        if not allowed(name):
            raise ValueError("Disallowed path: " + repr(name))
        dest = steam.joinpath(*name.split("/"))
        if any(x.is_symlink() for x in (dest, *dest.parents) if is_within(x, steam)):
            raise ValueError("Symlink in destination: " + name)
        if dest.exists() and not dest.is_file():
            raise ValueError("Target not a file: " + name)
        targets.append((name, dest))
    if not apply:
        return [name for name, _ in targets]
    if backup.exists():
        raise FileExistsError("Refusing to overwrite earlier backup: " + str(backup))
    backup.mkdir(parents=True)
    existed = {}
    for name, dest in targets:
        existed[name] = dest.exists()
        if dest.exists():
            target = backup.joinpath(*name.split("/"))
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(dest, target)
    (backup / "backed-up-paths.json").write_text(json.dumps(existed, indent=2), encoding="utf-8")
    touched = []
    try:
        for name, dest in targets:
            dest.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(dir=dest.parent, delete=False) as temp:
                temp.write(payload[name])
                tmp_name = temp.name
            try:
                touched.append((name, dest))
                os.replace(tmp_name, dest)
            finally:
                if os.path.exists(tmp_name):
                    os.unlink(tmp_name)
            if sha(dest.read_bytes()) != sha(payload[name]):
                raise OSError("Post-install hash mismatch: " + name)
    except Exception:
        for name, dest in reversed(touched):
            if existed[name]:
                shutil.copy2(backup.joinpath(*name.split("/")), dest)
            elif dest.exists():
                dest.unlink()
        raise
    return [name for name, _ in targets]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--zip", type=Path, required=True)
    p.add_argument("--steam-dir", type=Path, required=True)
    p.add_argument("--backup-dir", type=Path)
    p.add_argument("--apply", action="store_true", help="Install; default is preview only")
    a = p.parse_args()
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    backup = a.backup_dir or Path.home() / "PW-Edition-Backups" / stamp
    try:
        files = apply_overlay(verify_package(a.zip), a.steam_dir, backup, a.apply)
    except (OSError, ValueError, KeyError, zipfile.BadZipFile, UnicodeError) as exc:
        print("PW install failed:", exc, file=sys.stderr)
        return 1
    print("Installed:" if a.apply else "Preview only; no files changed:")
    for item in files:
        print(" ", item)
    if a.apply:
        print("Backup:", backup)
    else:
        print("Exit GameGuru, check these paths, then rerun with --apply.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
