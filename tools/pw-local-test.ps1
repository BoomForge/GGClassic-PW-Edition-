# Pixelwolf GG Classic: compile locally and stage an ISOLATED test installation.
# Run: powershell -ExecutionPolicy Bypass -File .\tools\pw-local-test.ps1 -SteamGameDir "D:\SteamLibrary\steamapps\common\Game Guru"
param(
 [string]$SteamGameDir = "",
 [string]$TestDir = "",
 [switch]$SkipBuild
)
$ErrorActionPreference="Stop"
$root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($SteamGameDir)) {
  Add-Type -AssemblyName System.Windows.Forms
  $dialog=New-Object System.Windows.Forms.FolderBrowserDialog
  $dialog.Description="Select your Steam GameGuru Classic folder (contains Files)"
  if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
    throw "No GameGuru Classic install folder selected"
  }
  $SteamGameDir=$dialog.SelectedPath
}
$source=(Resolve-Path $SteamGameDir).Path.TrimEnd('\','/')
if (-not (Test-Path (Join-Path $source "Files") -PathType Container) -or
    -not (Test-Path (Join-Path $source "GameGuru.exe") -PathType Leaf)) {
 throw "SteamGameDir must point to your owned GameGuru Classic installation containing Files and GameGuru.exe"
}
if (-not $TestDir) {$TestDir=Join-Path $root "PW-TestInstall"}
$dest=[IO.Path]::GetFullPath($TestDir).TrimEnd('\','/')
if ($dest.Equals($source,[StringComparison]::OrdinalIgnoreCase) -or
    $dest.StartsWith($source+'\',[StringComparison]::OrdinalIgnoreCase) -or
    $source.StartsWith($dest+'\',[StringComparison]::OrdinalIgnoreCase)) {
 throw "Test install must be outside SteamGameDir"
}
# Never allow a mistyped -TestDir to overwrite the checkout or built binaries.
$rootFull=[IO.Path]::GetFullPath($root).TrimEnd('\','/')
$build=Join-Path $root "PW-Build\Release"
$buildFull=[IO.Path]::GetFullPath($build).TrimEnd('\','/')
if ($dest.Equals($rootFull,[StringComparison]::OrdinalIgnoreCase) -or
    $rootFull.StartsWith($dest+'\',[StringComparison]::OrdinalIgnoreCase) -or
    $dest.Equals($buildFull,[StringComparison]::OrdinalIgnoreCase) -or
    $dest.StartsWith($buildFull+'\',[StringComparison]::OrdinalIgnoreCase)) {
 throw "Test install must not overwrite the repository or PW-Build output"
}
if (Test-Path -LiteralPath $dest) {
 $attributes=(Get-Item -LiteralPath $dest -Force).Attributes
 if (($attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
  throw "Refusing a test directory that is a junction or symbolic link"
 }
}
if (-not $SkipBuild) {
 $vswhere=Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) "Microsoft Visual Studio\Installer\vswhere.exe"
 if (-not (Test-Path $vswhere)) {throw "Visual Studio 2022 C++ tools required"}
 $vs=& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
 if (-not $vs) {throw "MSBuild not found"}
 $msbuild=Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe"
 & (Join-Path $PSScriptRoot "pw-prepare-d3dx.ps1")
 & (Join-Path $PSScriptRoot "pw-prepare-steam-imports.ps1")
 if (-not (Test-Path (Join-Path $root "GameGuru Core\SDK\DirectX\Lib\x64\d3dx11.lib"))) {throw "D3DX11 was not staged"}
 foreach ($project in @(
    "GameGuru Core\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj",
    "GameGuru Core\SDK\DirectX\Effect11\Effects11_2022.vcxproj",
    "GameGuru Core\SDK\OGG\ogg_vorbis_static.sln",
    "GameGuru Core\GameGuru.sln")) {
   Write-Host "Compiling $project..."
   & $msbuild (Join-Path $root $project) /t:Build /p:Configuration=Release /p:Platform=x64 /m:2 /nologo /v:minimal
   if ($LASTEXITCODE -ne 0) {throw "Compilation failed: $project (nothing installed)"}
 }
}
# Validate actual editor output before copying even one file.
$requiredBuildFiles=@(
 "Guru-MapEditor.exe",
 "pw-updater.ps1",
 "pw-edition-version.txt",
 "Files\scriptbank\global.lua",
 "Files\scriptbank\pw_transition_state.lua",
 "Files\effectbank\reloaded\apbr_animwithtran_emissive.fx"
)
foreach ($relative in $requiredBuildFiles) {
 if (-not (Test-Path -LiteralPath (Join-Path $build $relative) -PathType Leaf)) {
  throw "Compiled PW build is incomplete (missing $relative). Nothing staged."
 }
}
$editorPath=Join-Path $build "Guru-MapEditor.exe"
# PowerShell 5.1 does not expose Encoding.Latin1; ISO-8859-1 preserves ASCII bytes.
$editorText=[Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes($editorPath))
if (-not $editorText.Contains('GameGuru Classic - PW Edition | ')) {
 throw "Compiled editor is missing the PW Edition branding marker. Nothing staged."
}
$commit="unknown"
try { $commit=(& git -C $root rev-parse HEAD 2>$null).Trim() } catch {}
$provenance=Join-Path $build "pw-local-build-source.txt"
if ($SkipBuild) {
 if ($commit -eq "unknown" -or -not (Test-Path -LiteralPath $provenance -PathType Leaf) -or
     (Get-Content -LiteralPath $provenance -Raw).Trim() -ne $commit) {
  throw "SkipBuild is only safe when the build marker matches the current Git commit. Run without -SkipBuild."
 }
} else {
 [IO.File]::WriteAllText($provenance, $commit + [Environment]::NewLine)
}
$editorHash=(Get-FileHash -LiteralPath $editorPath -Algorithm SHA256).Hash
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Write-Host "Copying your Steam assets to the separate test directory: $dest"
& robocopy.exe $source $dest /E /XJ /R:1 /W:1 /NP /NFL /NDL
if ($LASTEXITCODE -ge 8) {throw "Steam asset staging failed (robocopy exit $LASTEXITCODE)"}
Write-Host "Overlaying locally built Pixelwolf files..."
& robocopy.exe $build $dest /E /XJ /R:1 /W:1 /NP /NFL /NDL
if ($LASTEXITCODE -ge 8) {throw "PW build staging failed (robocopy exit $LASTEXITCODE)"}
$stagedEditor=Join-Path $dest "Guru-MapEditor.exe"
if (-not (Test-Path -LiteralPath $stagedEditor -PathType Leaf) -or
    (Get-FileHash -LiteralPath $stagedEditor -Algorithm SHA256).Hash -ne $editorHash) {
 throw "Staged editor SHA-256 differs from the built editor. Do not launch this test installation."
}
@"
PIXELWOLF GG CLASSIC TEST INSTALL
Commit: $commit
Editor SHA-256: $editorHash
Source: $source
Test folder: $dest
Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz")

Run the GameGuru.exe launcher copied FROM YOUR STEAM folder, or
Guru-MapEditor.exe directly FROM THIS TEST FOLDER. Do not run from Steam.
Use a new or backed-up project. This is an experimental build.
Never run the in-engine updater to install this branch.

ASSET INSPECTOR ACCEPTANCE (22 NOV 2026 REVISION):
1. Launch Guru-MapEditor.exe directly; verify PW Edition title and Classic toolbar icons.
2. Check bottom Asset Browser stays docked; open its separate Asset Inspector.
3. Browse nested folders. Preview PNG and DDS textures without distortion.
4. Preview stock FPE/DBO/X and a small FBX; drag to rotate, wheel to zoom.
5. Insert a stock FPE in front of the editor camera. Verify ground height, undo,
   hierarchy, saved FPM and reopened scene.
6. Insert a simple FBX and check visible textures, thumbnail and selection.
7. Try a missing/invalid model; confirm error, not a ghost entry or crash.
8. In Terrain Tools, import ONE texture for a cloned stock skybox/grass style.
   Confirm no model selector, inspect visuals, mix grass styles and save/reopen.
9. Test Level then return to editor; check panel positions and no lost assets.

REGRESSION CHECKS:
- Repeat old Classic static entity and DDS workflows.
- Confirm GLB is not advertised as working; FBX is the supported test format.
- Try Lua Studio, nested scene hierarchy and a disposable Character Creator entity.
- Log any crash, path error or missing media before testing the updater.

DO NOT run in-engine updates from this test copy or install over Steam.
Do not redistribute this installation or proprietary binaries.

ADVANCED:
Optional environment flag GGCPW_VERIFY_SKELETON_SAVE=1 enables
slower DBO round-trip auditing and debugger output.
The new ImGui rig panel may not appear on non-VRTECH Classic builds.
"@ | Set-Content -Path (Join-Path $dest "PW-TESTING-README.txt") -Encoding UTF8
foreach ($dll in @("steam_api64.dll","sdkencryptedappticket64.dll")) {
 if (-not (Test-Path (Join-Path $dest $dll))) {
  Write-Warning "Missing $dll at test root. Confirm your licensed Steam GameGuru installation supplies the matching x64 runtime before launching."
 }
}
Write-Host "Done. Test editor: $(Join-Path $dest 'Guru-MapEditor.exe')"
Write-Host "If present, try the Steam GameGuru.exe launcher copied to the TEST directory."
Write-Host "Steam installation was not changed."
