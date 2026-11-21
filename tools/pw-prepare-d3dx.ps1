# PW Edition: stage official Microsoft legacy D3DX import libraries for x64 builds.
# Does not commit, upload or package proprietary GameGuru or Microsoft SDK binaries.
# Package: https://www.nuget.org/packages/Microsoft.DXSDK.D3DX/9.29.952.8
param([string]$PackageVersion = "9.29.952.8")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$cache = Join-Path $root ".pw-local-deps"
$archive = Join-Path $cache "microsoft.dxsdk.d3dx.$PackageVersion.zip"
$unpacked = Join-Path $cache "Microsoft.DXSDK.D3DX.$PackageVersion"
$libraryDir = Join-Path $root "GameGuru Core/SDK/DirectX/Lib/x64"
New-Item -ItemType Directory -Force -Path $cache,$libraryDir | Out-Null

if (-not (Test-Path $unpacked)) {
    $url = "https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/$PackageVersion/microsoft.dxsdk.d3dx.$PackageVersion.nupkg"
    Write-Host "Retrieving Microsoft.DXSDK.D3DX $PackageVersion from NuGet.org"
    Invoke-WebRequest -Uri $url -OutFile $archive -MaximumRedirection 5
    New-Item -ItemType Directory -Force -Path $unpacked | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $unpacked -Force
}
$libRoot = Join-Path $unpacked "build/native/release/lib/x64"
if (-not (Test-Path $libRoot)) {
    $candidates = Get-ChildItem -Path $unpacked -Recurse -Filter "d3dx11.lib" -File
    $library = $candidates | Where-Object { $_.FullName -match '(?i)[\\/]release[\\/]lib[\\/]x64[\\/]' } | Select-Object -First 1
    if (-not $library) { throw "Microsoft D3DX package has no x64 release D3DX11.lib" }
    $libRoot = $library.Directory.FullName
}
foreach ($lib in @("d3dx11.lib","d3dx9.lib")) {
    $source = Join-Path $libRoot $lib
    if (-not (Test-Path $source)) { throw "Expected Microsoft SDK library missing: $source" }
    Copy-Item -LiteralPath $source -Destination (Join-Path $libraryDir $lib) -Force
}
Write-Host "PW D3DX x64 import libraries staged into $libraryDir (not committed)."
