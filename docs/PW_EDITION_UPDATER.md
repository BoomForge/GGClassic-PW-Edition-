# GGClassic PW Edition — in-engine updater

Status: **source integrated, Windows build and end-to-end install unverified** (21 November 2026).

The 64-bit ImGui editor adds **Help > Check for PW Edition Updates...**. It launches a locally installed Windows PowerShell helper, pw-updater.ps1, rather than the old disabled FPSC/X10-era updater.

## User flow

1. Choose Help > Check for PW Edition Updates in the running editor.
2. The helper checks only the latest published stable GitHub release at https://github.com/BoomForge/GGClassic-PW-Edition-/releases/latest.
3. If a newer SemVer release exists, the user is asked to download/install. The helper downloads exactly GGClassic-PW-Edition-win64.zip and GGClassic-PW-Edition-win64.zip.sha256 over HTTPS.
4. It validates the ZIP SHA-256, checks package manifest/version consistency, and verifies SHA-256 for every extracted file.
5. The user is asked to **save current work and close the editor**. The updater waits up to 20 minutes for that editor process to exit.
6. A separate helper backs up each target file, installs only allowlisted files, validates hashes, and attempts rollback on failure. Windows UAC is requested if necessary.
7. The helper asks whether to **restart the engine**. Declining leaves the new version installed without restarting.

The update must never force-close the editor, silently discard unsaved levels, delete user content, or pull releases from third-party repositories.

## Build integration

The current engine project is GameGuru Core/Guru-MapEditor/DarkEXE.vcxproj (Visual Studio 2022 x64). Its post-build step now copies tools/pw-updater.ps1 and tools/pw-edition-version.txt into the built editor's executable folder.

The bootstrap version marker is 0.0.0. Published update packages supply their SemVer number. Both files must be installed beside Guru-MapEditor.exe. The updater refuses to modify an installation without the PW Edition version marker. Install the first compiled PW Edition package manually once, before relying on future in-engine updates.

## Publishing release assets

Requires Python 3.9+ and a successful x64 Windows build:

    python tools/make_pw_release.py --build-dir "D:\GGClassic-PW-Build" --version 0.1.0 --out-dir dist

To include approved supporting files:

    python tools/make_pw_release.py --build-dir "D:\GGClassic-PW-Build" --version 0.1.1 --out-dir dist --include "Files/effectbank/reloaded/pw_shader.fx" --include "MyPatch.dll"

The tool packages only the built engine executable, PW updater script and release version marker, plus explicitly opted-in DLLs or Files/effectbank/reloaded/*.fx files. It does NOT distribute proprietary GameGuru content, user levels or full asset/media banks.

Create a **stable GitHub release** tagged v0.1.0 in BoomForge/GGClassic-PW-Edition- and attach both output assets:
- dist/GGClassic-PW-Edition-win64.zip
- dist/GGClassic-PW-Edition-win64.zip.sha256

The release tag, ZIP manifest, version marker and checksums must agree. Drafts and prereleases are not offered by the updater. No release or installer has been published yet.

## Verification checklist

Packaging tests:

    python -m unittest discover -s tools -p "test_make_pw_release.py"

- [ ] VS2022 x64 compilation with licensed GameGuru original media.
- [ ] Help menu action and installed script location verified on Windows.
- [ ] Missing GitHub release and no-newer-version behaviour.
- [ ] Real disposable PW Edition install updated from a stable release ZIP and checksum.
- [ ] Corrupted SHA, manifest tampering and path traversal rejected.
- [ ] Locked-file failure tested with rollback and retained backups.
- [ ] UAC and standard user install locations tested.
- [ ] User levels and asset folders unchanged after successful update.
- [ ] Restart prompt optional and launches correct updated executable.
- [ ] Distribution rights and release permissions checked before publishing any custom binaries.

## Security and limitations

Allowlisted updates may replace only the engine EXE, root DLLs, the PW updater/version marker, and explicitly named Files/effectbank/reloaded FX shaders. Levels, settings, FPM/FPE, entitybank and terrainbank are excluded. All extracted paths are checked and all staged files are SHA-256 checked again before installation. Existing files are backed up before any writes.

Release SHA-256 files provide download integrity but not an independent publisher signature if the release account itself is compromised. Authenticode/code signing and stronger provenance checks are future hardening. Current user prompts are Windows dialog boxes launched from the editor, not yet embedded progress bars in ImGui.

**Nothing in this branch proves the updater has been built or run successfully on Windows.** Do not merge as production-ready until the checks above pass.
