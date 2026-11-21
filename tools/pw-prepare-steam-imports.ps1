# Create x64 MSVC import-library metadata from the Steamworks API functions
# referenced by this GameGuru source. NO DLL or proprietary SDK binary is
# downloaded, embedded or committed. Runtime requires a licensed compatible
# steam_api64.dll and sdkencryptedappticket64.dll from the user's installation.
$ErrorActionPreference="Stop"
$root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$vswhere=Join-Path ([Environment]::GetFolderPath("ProgramFilesX86")) "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {throw "Visual Studio 2022 C++ tools are needed to produce local import libraries"}
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) {throw "MSVC x64 compiler tools are missing"}
$toolsets=Get-ChildItem (Join-Path $vs "VC\Tools\MSVC") -Directory | Sort-Object Name -Descending
$libexe=$null
foreach ($version in $toolsets) {
 $candidate=Join-Path $version.FullName "bin\Hostx64\x64\lib.exe"
 if (Test-Path $candidate) {$libexe=$candidate;break}
}
if (-not $libexe) {throw "No x64 MSVC lib.exe found"}
$out=Join-Path $root "GameGuru Core\SDK\Steamworks SDK\redistributable_bin\win64"
New-Item -ItemType Directory -Force -Path $out | Out-Null
foreach ($name in @("steam_api64","sdkencryptedappticket64")) {
 $manifest = if ($name -eq "steam_api64") { "steam-api64.def" } else { "$name.def" }
 $definition=Join-Path $PSScriptRoot $manifest
 $target=Join-Path $out ("$name.lib")
 if (-not (Test-Path $definition)) {throw "Missing GameGuru Steam symbol manifest $definition"}
 Write-Host "Generating locally linked import records for $name (no DLL supplied)"
 & $libexe "/nologo" "/machine:x64" "/def:$definition" "/out:$target"
 if ($LASTEXITCODE -ne 0 -or -not (Test-Path $target)) {throw "Steam import metadata generation failed: $name"}
}
Write-Host "Link metadata ready. The actual Steamworks DLLs are NOT included by this script."
