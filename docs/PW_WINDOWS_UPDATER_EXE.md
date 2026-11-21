# Pixelwolf GameGuru Classic Windows updater EXE

Source: `tools/PWBootstrapper/`. Windows CI: `.github/workflows/pw-bootstrapper-exe.yml`.

The public Windows workflow publishes **only** the original `Pixelwolf-GGC-Updater.exe` bootstrapper, not GameGuru's copyrighted compiled editor, paid Steam assets, Steamworks runtimes or media. It is a self-contained .NET 8 Windows x64 executable.

## User requirements

- A licensed Steam GameGuru Classic install on Windows x64.
- Visual Studio 2022 **Desktop development with C++**, x64 MSBuild/MFC and Windows SDK, **or the free Visual Studio Build Tools 2022 with equivalent components**. If missing, the new updater offers to download and open Microsoft's official Build Tools installer with the C++ workload, MFC and Windows 11 SDK preselected. Complete Microsoft setup, then rerun the PW updater. The latest PW source is downloaded automatically.
- Working internet for GitHub source archive and official Microsoft D3DX NuGet.
- Sufficient disk space to unpack and compile Classic, and write access to the selected install folder. If Steam is protected under Program Files, Windows may require launching the updater as Administrator; it is unsigned.

## Operation

1. Launch `Pixelwolf-GGC-Updater.exe`. Select the existing Steam GameGuru Classic directory containing `Files` and the original launcher/editor. If MSVC v143/MFC/MSBuild is missing, choose whether to open the **official Microsoft Build Tools 2022 setup**; do not proceed until its installation finishes and you relaunch the updater. Steam remains unchanged during this step.
2. The program checks VS2022 and asks permission to download/build. It pins the fork's active `work/pw-community-integration-20261121` branch to a specific GitHub commit SHA for this invocation.
3. It downloads and extracts that commit's source snapshot into a temporary folder and runs the same proven x64 build order as CI: prepare DirectX and Steam import metadata, build DirectXTex, Effects11, Ogg/Vorbis and `GameGuru.sln`.
4. If compilation or expected output verification fails, the Steam folder is not modified.
5. The program checks that GameGuru is closed and requests **a second confirmation** before installation.
6. It backs up every existing target file under `%USERPROFILE%\PW-Edition-Backups\<timestamp>`, writes an install manifest, then installs exactly these six allowlisted files from the build:
   - `Guru-MapEditor.exe`
   - `pw-updater.ps1`
   - `pw-edition-version.txt`
   - `Files/scriptbank/global.lua`
   - `Files/scriptbank/pw_transition_state.lua`
   - `Files/effectbank/reloaded/apbr_animwithtran_emissive.fx`
7. It verifies per-file SHA-256 after staging/installing and attempts automatic rollback for failures. Original `GameGuru.exe`, models, textures, projects and purchased game media are never intentionally overwritten. Build source is removed from the temp folder when complete.

The launcher is currently **not tested against a real Steam installation**. The GitHub Actions job compiles the installer, but does not perform a UI, UAC, Steam runtime, asset or gameplay test. Keep a separate backup of your own projects and do first testing with a small disposable map.

This is a source-building install helper, not a magic download of compiled proprietary GameGuru binaries. The in-engine GitHub **release updater** remains separately implemented and will need a proper licensed release/test cycle.


## Missing compiler troubleshooting (21 November)

The original bootstrapper showed `Install Visual Studio 2022 Desktop development with C++, x64 MFC and Windows SDK first. Steam is unchanged.` and exited. That is a machine prerequisite, **not a GameGuru crash**. A replacement bootstrapper detects VS2022 Build Tools as well as the full IDE and offers an optional download of Microsoft's installer from https://aka.ms/vs/17/release/vs_buildtools.exe . The user remains in control of Microsoft setup and may decline it without touching Steam. Required component IDs are documented at https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022 .

This still compiles the proprietary editor on the licensed local Windows machine; the public workflow uploads only this independently authored bootstrapper, not GameGuru runtime binaries. The new guided flow has **compiled under Windows CI**, but the Microsoft installer UX, permissions, disk usage, Steam overwrite safeguards and compiled engine launch have not been exercised on the owner's Windows computer yet.
