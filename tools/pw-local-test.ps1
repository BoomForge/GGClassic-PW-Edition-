# Pixelwolf GG Classic: compile locally and stage an ISOLATED test installation.
# Run: powershell -ExecutionPolicy Bypass -File .\tools\pw-local-test.ps1 -SteamGameDir "D:\SteamLibrary\steamapps\common\Game Guru"
param(
 [Parameter(Mandatory=$true)][string]$SteamGameDir,
 [string]$TestDir = "",
 [switch]$SkipBuild
)
$ErrorActionPreference="Stop"
$root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source=(Resolve-Path $SteamGameDir).Path.TrimEnd('\','/')
if (-not (Test-Path (Join-Path $source "Files") -PathType Container)) {
 throw "SteamGameDir must point to your owned GameGuru directory containing Files"
}
if (-not $TestDir) {$TestDir=Join-Path $root "PW-TestInstall"}
$dest=[IO.Path]::GetFullPath($TestDir).TrimEnd('\','/')
if ($dest.Equals($source,[StringComparison]::OrdinalIgnoreCase) -or
    $dest.StartsWith($source+'\',[StringComparison]::OrdinalIgnoreCase) -or
    $source.StartsWith($dest+'\',[StringComparison]::OrdinalIgnoreCase)) {
 throw "Test install must be outside SteamGameDir"
}
$build=Join-Path $root "PW-Build\Release"
if (-not $SkipBuild) {
 $vswhere=Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) "Microsoft Visual Studio\Installer\vswhere.exe"
 if (-not (Test-Path $vswhere)) {throw "Visual Studio 2022 C++ tools required"}
 $vs=& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
 if (-not $vs) {throw "MSBuild not found"}
 $msbuild=Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe"
 & (Join-Path $PSScriptRoot "pw-prepare-d3dx.ps1")
 if (-not (Test-Path (Join-Path $root "GameGuru Core\SDK\DirectX\Lib\x64\d3dx11.lib"))) {throw "D3DX11 was not staged"}
 foreach ($project in @(
    "GameGuru Core\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj",
    "GameGuru Core\SDK\DirectX\Effect11\Effects11_2022.vcxproj",
    "GameGuru Core\GameGuru.sln",
    "GameGuru IDE\GameGuru IDE.sln")) {
   Write-Host "Compiling $project..."
   & $msbuild (Join-Path $root $project) /t:Build /p:Configuration=Release /p:Platform=x64 /m:2 /nologo /v:minimal
   if ($LASTEXITCODE -ne 0) {throw "Compilation failed: $project (nothing installed)"}
 }
}
if (-not (Test-Path (Join-Path $build "Guru-MapEditor.exe"))) {
 throw "Compiled Guru-MapEditor.exe not found. Do not use SkipBuild without an existing valid build."
}
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Write-Host "Copying your Steam assets to the separate test directory: $dest"
& robocopy.exe $source $dest /E /XJ /R:1 /W:1 /NP /NFL /NDL
if ($LASTEXITCODE -ge 8) {throw "Steam asset staging failed (robocopy exit $LASTEXITCODE)"}
Write-Host "Overlaying locally built Pixelwolf files..."
& robocopy.exe $build $dest /E /XJ /R:1 /W:1 /NP /NFL /NDL
if ($LASTEXITCODE -ge 8) {throw "PW build staging failed (robocopy exit $LASTEXITCODE)"}
$commit="unknown"
try {$commit=& git -C $root rev-parse HEAD 2>$null} catch {}
@"
PIXELWOLF GG CLASSIC TEST INSTALL
Commit: $commit
Source: $source
Test folder: $dest
Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz")

Run Guru-MapEditor.exe FROM THIS TEST FOLDER, not from Steam.
Use a new or backed-up project. This is an experimental build.
Never run the in-engine updater to install this branch.

TEST PLAN:
1. Launch, create a blank level, save, reopen.
2. Open a small existing level, Test Level, return to editor.
3. Import static FBX with PNG and save DBO/FPE.
4. Try animated Mixamo FBX/X and save/reopen DBO.
5. Try Character Creator on a disposable character.
6. Test level transitions and loading times.
7. Restart editor, inspect the level again.

ADVANCED:
Optional environment flag GGCPW_VERIFY_SKELETON_SAVE=1 enables
slower DBO round-trip auditing and debugger output.
The new ImGui rig panel may not appear on non-VRTECH Classic builds.
Do not redistribute this installation or proprietary binaries.
"@ | Set-Content -Path (Join-Path $dest "PW-TESTING-README.txt") -Encoding UTF8
Write-Host "Done. Test editor: $(Join-Path $dest 'Guru-MapEditor.exe')"
Write-Host "Steam installation was not changed."
