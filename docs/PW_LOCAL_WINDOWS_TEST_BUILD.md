# Pixelwolf GameGuru Classic — local Windows test build

GameGuru Classic's original engine is proprietary. This public GitHub repository does not distribute any compiled GameGuru executables or Steam media. Build and test only with your own licensed GameGuru Classic installation.

## Requirements

- Windows 10/11 x64, GameGuru Classic installed through Steam
- Git checkout of BoomForge/GGClassic-PW-Edition- branch work/pw-community-integration-20261121
- Visual Studio 2022 with Desktop Development with C++ / MSBuild and Windows SDK installed
- Internet connection once to retrieve the official Microsoft D3DX 9.29.952.8 NuGet SDK

## Build and stage in one command

Open PowerShell **in the cloned repository** and run:

    powershell -ExecutionPolicy Bypass -File .\tools\pw-local-test.ps1 -SteamGameDir "D:\SteamLibrary\steamapps\common\Game Guru"

Substitute the actual path containing your Steam GameGuru Classic Files directory. No Steam file or installer is modified. The script:

1. Downloads official Microsoft.DXSDK.D3DX import libraries into a local ignored SDK folder.
2. Compiles DirectXTex, Effects11, the GameGuru x64 solution, and the IDE launcher.
3. Verifies the compiled editor exists.
4. Copies the owned Steam installation into a **separate** PW-TestInstall directory (this can require substantial storage and copy time).
5. Overlays locally compiled PW-Build/Release files.
6. Writes a PW-TESTING-README.txt with a basic manual smoke-test sequence.

Run PW-TestInstall\Guru-MapEditor.exe (not the one in Steam). The script stops on compilation errors rather than mixing an outdated build with new source. It does not automatically start the editor.

## Smoke testing

1. Start with a fresh or backed-up project and test editor launch/save/reopen.
2. Import a simple FBX with PNG textures, save entity and reopen it.
3. Import an animated FBX or X model, keep original animations, save/reload DBO.
4. Save a disposable Character Creator model.
5. Run a level, switch between levels, return to editor, and restart. Note load times/crashes.
6. Optional: set environment flag GGCPW_VERIFY_SKELETON_SAVE=1 before launching to enable the slower skeletal export/reload audit. It logs warnings through OutputDebugStringA. The experimental ImGui panel is guarded and may be unavailable in non-VRTECH Classic builds.

This is experimental engine software; full runtime testing, skeletal retargeting, and updated Character Creator UI are still outstanding. No public binaries are provided. Do not use the in-engine updater to install this development branch into Steam.
