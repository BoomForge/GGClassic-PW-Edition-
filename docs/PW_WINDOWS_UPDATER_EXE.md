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

## Windows PowerShell 5.1 D3DX extraction repair (21 Nov 2026)

A local owner screenshot showed `Microsoft.PowerShell.Archive.psm1:421` and the local updater stopping with exit code 1 in `tools/pw-prepare-d3dx.ps1`. The previous implementation expanded the *entire* NuGet D3DX package using `Expand-Archive` under a very long temporary GitHub extraction path; the full underlying error was hidden by the GUI, so a specific long-path root cause was **suspected, not proven**.

Remediation committed:
- `pw-prepare-d3dx.ps1` now streams only official `d3dx11.lib` and `d3dx9.lib` release/x64 entries directly from the Microsoft NuGet archive via .NET ZipArchive. It avoids `Expand-Archive`, checks extracted lengths, offers one clean redownload retry and uses TLS 1.2 for PowerShell 5.1.
- `tools/PWBootstrapper/Program.cs` now moves the GitHub checkout to a shallow `%LOCALAPPDATA%\PW\b-XXXXXXXX\src` directory before compiling to avoid Windows MAX_PATH pressure.
- Every compiler/script output line is written to `%LOCALAPPDATA%\Pixelwolf\GGC-Updater\Logs\update-*.log`, and the error dialog displays the final relevant lines plus the log path.
- `.github/workflows/pw-d3dx-ps51.yml` tests the dependency script using the actual `powershell.exe` 5.1 runtime on Windows Server 2022, rather than only GitHub runner `pwsh` 7. The first Windows PowerShell 5.1 tests passed at GitHub Actions runs 44871381726 and 44871385121.

This does not imply a completed local build or successful Steam installation. The new executable must be tested on the owner's PC, and later missing SDK/link/runtime problems remain possible. The no-write-before-full-build and backup/rollback safeguards remain intact.


## PW Edition identity, launch and installation verification (21 November)

A successful locally compiled install originally copied a new \`Guru-MapEditor.exe\` into the chosen Steam directory but intentionally left Steam's \`GameGuru.exe\` unchanged. Steam's Play action may invoke a stock launcher, and the original editor title was still hardcoded to \`GameGuru - \`. That combination made the upgraded editor appear absent even if a built EXE was present. Source inspection identified this as a delivery/identity oversight; we have no machine access to confirm what the owner actually launched.

The repaired installation now:
- Requires a distinctive compiled \`GameGuru Classic - PW Edition | \` string inside the freshly produced \`Guru-MapEditor.exe\` before **any install**. If missing, refuse to install rather than claim success.
- Gives the editor a PW Edition window title, clearly visible \`PW Edition\` top-level menu, and a labelled About section. The menu can display the currently running executable path.
- Includes a named \`Launch-PW-Edition.cmd\` alongside the rebuilt editor so users can launch the right EXE, not \`GameGuru.exe\`.
- Includes the independently authored \`Pixelwolf-GGC-Updater.exe\` beside \`Guru-MapEditor.exe\`. The editor's update command launches this tool on the same installation (not the old release-only PowerShell helper) after running the existing save/exit guard. The script remains staged for a future supported GitHub release channel.
- Installs \`pw-installed-commit.txt\`, containing the source SHA from the actual checkout, and retains the existing \`pw-edition-version.txt\` version marker.
- Verifies the installed editor's SHA-256 equals the compiled output, then **offers to launch the newly installed editor directly** with its Steam root as working directory.
- Leaves licensed Steam \`GameGuru.exe\`, media, saved levels and user project files unchanged. Original replaced files are still backed up; failed installation attempts roll back.

If the Steam launcher still looks stock, use Steam > Manage > Browse local files and run \`Guru-MapEditor.exe\` or \`Launch-PW-Edition.cmd\`. The PW version has a \`PW Edition\` menu and an \`Edit > Lua Script Editor\` entry. The new internal update command opens the standalone source-building updater; it does not promise instant binary patching or public release availability yet.

Actual editor runtime, Steam launcher routing, user account permissions and in-engine auto-update UX must still be tested locally. CI separately verifies that the compiled editor contains the distinctive PW title string.

## Optional tutorial folder and Steam Play routing (21 November 2026)

- Some licensed Steam installs do not contain `Files/tutorialbank`. The ImGui tutorial scanner previously called `SetDir("tutorialbank")` unconditionally and showed a modal **Path Cannot Be Found** dialog. PW Edition now checks that the optional directory exists before enumerating it; if missing, the tutorial list remains empty. This is an editor-side source fix, not a redistribution of tutorial files.
- Temporary workaround for older builds: create an empty `Files/tutorialbank` directory inside the selected Steam GameGuru Classic folder.
- The original Steam `GameGuru.exe` is deliberately left untouched. To open PW Edition from the normal Steam **Play** button, set **Steam Library > GameGuru Classic > Properties > General > Launch Options** to `"D:\\SteamLibrary\\steamapps\\common\\Game Guru\\Guru-MapEditor.exe" %command%` (adjust the path to the installed editor). This is user-controlled and reversible by clearing that field. It has not yet been verified on the owner's machine. Steam app tracking/overlay behavior may differ from the original launcher.
- The Steam alternative-executable launch option passes the original Steam command as an extra argument; Classic's documented command-line handling recognises named flags such as `-r` and ignores other unrecognised command text in this startup path. Validate ordinary editor open, Test Level, and Steam Play behavior manually.
- Future integrated Steam launcher work should preserve and restore the stock launcher and must not overwrite Steam userdata/settings without explicit approval.

## Visual toolbar fallback repair — 21 November 2026

The earlier updater drew filename text instead of recognisable artwork on 64px icon tiles when Steam lacked ImGui toolbar resources. Owner testing confirmed this was a usability regression.

The next source-building updater draws original icon pictograms locally rather than redistributing GameGuru proprietary artwork. It also recognises the preceding updater's distinct dark tile/blue frame and replaces these old PW-generated text tiles, while preserving non-matching existing user or stock image files.

Actual installation, readability, and icon preservation require owner testing. Repeat build/install is required; compilation alone does not replace already-installed old icons.

## Restoring licensed Classic toolbar art from the user's own game (21 November 2026)

The updater no longer treats PW's turquoise 64px circles as a design goal. During a local install it reads the **installed stock GameGuru.exe** as a resource-only data file, extracts original Classic 42x42 BMP toolbar strips by RT_BITMAP ID, and writes the matching icons under Files/editors/uiv3 in the user's own GameGuru installation. This does not include GameGuru toolbar images in the public update binary, GitHub release or source package. The source contains only C# extraction logic and icon-to-command mapping informed by GameGuru IDE/Editor.rc.

Both earlier filename text tiles and blue/cyan PW vector tiles are recognised for replacement, but other installed/custom resource files are preserved. If the exact Classic bitmap is absent in a user's version, the updater logs this; the remaining unmatched buttons receive neutral fallback graphics, and a runtime screenshot check is still required.

## Original toolbar bitmap GDI entry-point repair (22 November 2026)

Owner's Windows screenshot showed `System.EntryPointNotFoundException` in `PixelwolfGGCUpdater.Program.DeleteOriginalBitmap(IntPtr bitmap)` after GameGuru Core linked and after the updater reported zero original UI icons recovered from previous backups. The compiler's `LNK4286` messages were warnings, not the exception cause. Icon restoration executes before the installation confirmation and `InstallWithRollback`, so the failing attempt did not reach the Steam-file replacement step.

Root cause: a C# P/Invoke wrapper named `DeleteOriginalBitmap` had no explicit `EntryPoint`. GDI32 exports `DeleteObject`, not the wrapper name. Fixed with `[DllImport("gdi32.dll", EntryPoint = "DeleteObject", ExactSpelling = true, SetLastError = true)]`, retaining safe HBITMAP cleanup. New `--self-test-native-icons` command allocates a real GDI bitmap and invokes the compiled updater's native cleanup method. Windows CI workflow now runs this smoke test before publishing an updater artifact. GitHub Actions run 44909513808 succeeded for compilation, native bitmap cleanup, self-replacement and artifact upload.

Corrected updater artifact: [Windows updater workflow](https://github.com/BoomForge/GGClassic-PW-Edition-/actions/runs/44909513808). Runtime acceptance on the owner's licensed Steam install is still required; this fix does **not** prove that all original toolbar images exist locally or that other editor UI issues are resolved.
