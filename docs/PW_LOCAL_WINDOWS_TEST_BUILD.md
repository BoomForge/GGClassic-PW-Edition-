# Pixelwolf GameGuru Classic — local Windows test build

GameGuru Classic's original engine is proprietary. This public GitHub repository does not distribute any compiled GameGuru executables or Steam media. Build and test only with your own licensed GameGuru Classic installation.

## Requirements

- Windows 10/11 x64, GameGuru Classic installed through Steam
- Git checkout of BoomForge/GGClassic-PW-Edition- branch work/pw-community-integration-20261121
- Visual Studio 2022 with Desktop Development with C++ / MSBuild and Windows SDK installed
- Internet connection once to retrieve the official Microsoft D3DX 9.29.952.8 NuGet SDK

## Build and stage in one command

**Simplest route:** download/clone the active development branch, then double-click **START_PW_TEST_BUILD.cmd** at the repository root. Choose your **owned Steam GameGuru Classic folder** (the one containing Files and GameGuru.exe) when prompted. This uses the same local-build script described below, without making you type a command.

Alternatively, open PowerShell in the checkout and run:

    powershell -ExecutionPolicy Bypass -File .\tools\pw-local-test.ps1 -SteamGameDir "D:\SteamLibrary\steamapps\common\Game Guru"

Substitute your actual Steam path. The source build is always performed locally. It is **not** a downloaded compiled GameGuru binary or a public installer.

No Steam file or installer is modified. The script:

1. Downloads official Microsoft.DXSDK.D3DX import libraries into a local ignored SDK folder.
2. Compiles DirectXTex, Effects11, Ogg/Vorbis, and the GameGuru x64 solution. The old GameGuru IDE launcher is Win32-only, so the script reuses the launcher from your owned Steam installation in the separate test copy.
3. Verifies the complete PW staging set, checks the compiled editor contains the PW Edition identity marker and records its SHA-256 and source commit. `-SkipBuild` refuses a stale/mismatched build.
4. Copies the owned Steam installation into a **separate** PW-TestInstall directory (this can require substantial storage and copy time).
5. Overlays locally compiled PW-Build/Release files.
6. Compares the staged EXE SHA-256 with the compiled EXE and writes a `PW-TESTING-README.txt` with an Asset Inspector-specific acceptance checklist.

Run **`PW-TestInstall\Guru-MapEditor.exe` directly** for the clearest PW Edition verification. Do not launch the original Steam installation or run the in-engine updater during these tests. The script stops on compilation, branding or file-hash failures before claiming success; it does not automatically start the editor. To avoid overwriting other folders, a custom test location cannot be the Steam directory, checkout, build output or a different repository subfolder. It also rejects an explicitly linked/junction test folder. The default `PW-TestInstall` is reused on later runs and *not wiped*; use a fresh external `-TestDir` for a pristine test.

## Smoke testing — priority: Asset Inspector

1. Launch the isolated `Guru-MapEditor.exe`; confirm **PW Edition** identity, Classic toolbar icons, centre viewport and permanently docked bottom Asset Browser.
2. Open the **separate Asset Inspector** from the docked browser. Expand several nested folders on the left. Preview PNG/DDS images and FPE/DBO/X/FBX models on the right; test mouse orbit, zoom and reset.
3. Insert a stock FPE in front of the camera; verify terrain height, scene hierarchy, Undo, FPM save and reopen. Then try a simple textured FBX and verify its thumbnail and placement.
4. Select a missing/broken model; confirm it fails clearly without producing a phantom thumbnail/entity.
5. Import **one image** for a skybox and grass template, inspect the native clone, select more than one grass style and adjust spread weights. Save/reopen.
6. Run Test Level, return to the editor, and verify docking, selections and scene contents remain intact.
7. Only afterwards test animated FBX, Character Creator, level transitions and loading times on disposable projects. Keep GLB disabled/deferred.
8. Optional: set `GGCPW_VERIFY_SKELETON_SAVE=1` before launching for the slower skeletal DBO audit. This does not prove rig retargeting.

**Report failures** with the displayed error, build commit from `PW-TESTING-README.txt`, selected asset type and what you clicked. Do not test the in-engine updater against your Steam folder yet.

This is experimental engine software; full runtime testing, skeletal retargeting, and updated Character Creator UI are still outstanding. No public binaries are provided. Do not use the in-engine updater to install this development branch into Steam.

## Steamworks runtime dependency

The GameGuru engine links to 27 Steam API and encrypted-ticket symbols from its original SteamMultiplayer library. The public source fork cannot ship Valve's import libraries. `tools/pw-prepare-steam-imports.ps1` generates **local link metadata** (`steam_api64.lib`, `sdkencryptedappticket64.lib`) from tracked DEF symbol names using MSVC, without bundling Valve DLLs. It does not replace Steamworks functionality. The resulting test folder must have the **compatible original x64 Steamworks runtime DLLs from your licensed GameGuru installation**. If absent, you will see a warning and the rebuilt engine may not launch until compatible licensed runtime DLLs are available. This is not a no-Steam build. Test Steam features separately from core editor functionality.
