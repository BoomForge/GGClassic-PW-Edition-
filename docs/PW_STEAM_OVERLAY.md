# GGClassic PW Edition — build and safe Steam overlay

**Status 21 Nov 2026:** Only source, offline test fixtures and packaging are prepared. A complete functioning editor binary has NOT been built or gameplay tested. This workflow is for a personally owned GameGuru Classic installation, not public distribution; TGC's source is not open source.

## Windows build

Install the original GameGuru Classic from Steam. Install Visual Studio 2022 Desktop C++ tools, Windows SDK and MFC for x64, plus the original repo's additional dependencies. Open GameGuru Core/GameGuru.sln; select Release|x64 and build the solution. The modified engine output location is now PW-Build/Release/Guru-MapEditor.exe (relative to the repository root). Its build copies the PW updater, version marker, transition Lua scripts and optional shader into that directory.

The GameGuru.exe frontend is a separate project: GameGuru IDE/GameGuru IDE.sln. The supplied overlay leaves Steam's existing launcher untouched by default. The Guru-Lightmapper.exe and original purchased media still come from your installation.

A new .github/workflows/pw-full-build.yml source-only Windows CI job attempts a full build, but cannot prove runtime stability, availability of licensed dependencies, or publisher permission to distribute EXEs. No public binary artifact is uploaded.

## Local overlay ZIP from a successful build

Run from repo root with Python 3.11+:

    python tools/make_pw_steam_overlay.py --build-dir "PW-Build\Release" --version 0.1.0 --out-dir dist

This refuses to package without the new engine, shader and Lua companion files. Optional --with-ide includes a newly built GameGuru.exe ONLY when you put it in the build folder; do not copy the original Steam launcher there.

Output: dist/GGClassic-PW-Edition-win64.zip, matching .zip.sha256, and dist/install_pw_steam_overlay.py. The ZIP contains only approved binaries, updater/version marker, shader and scripts. It contains no entitybank, maps, models, terrain, user settings or game media.

## First manual installation

Close GameGuru. In Steam use GameGuru Classic > Manage > Browse local files to confirm your exact installation folder. First run a non-mutating preview:

    python dist/install_pw_steam_overlay.py --zip "dist\GGClassic-PW-Edition-win64.zip" --steam-dir "YOUR GAMEGURU STEAM FOLDER"

If the paths are correct, explicitly apply:

    python dist/install_pw_steam_overlay.py --zip "dist\GGClassic-PW-Edition-win64.zip" --steam-dir "YOUR GAMEGURU STEAM FOLDER" --apply

The installer validates ZIP SHA-256 and individual manifest checksums, rejects non-approved paths, creates timestamped backups OUTSIDE the Steam directory under your home folder, and attempts automatic rollback on an installation failure. Still save copies of your custom worlds and assets separately. Steam "Verify integrity" restores official files, not necessarily user creations.

## Deferred verification

First confirm startup, save/reopen, Test Game, a tiny standalone build, PNG/DDS import, Mixamo animation, updater interface and 25+ level transitions. Leave GGCPW_FPM_CACHE, GGCPW_PREFETCH_LEVELS and GGCPW_KEEP_LEVEL_STATS unset until full regression tests pass. No release or engine success is claimed yet.
