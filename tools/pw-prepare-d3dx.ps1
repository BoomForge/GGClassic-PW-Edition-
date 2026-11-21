# PW Edition: stage official Microsoft legacy D3DX import libraries for x64 builds.
# Supports Windows PowerShell 5.1 and PowerShell 7+. No full Expand-Archive:
# only the two exact approved x64 .lib entries are streamed from the NuGet ZIP.
# Package: https://www.nuget.org/packages/Microsoft.DXSDK.D3DX/9.29.952.8
param([string]$PackageVersion = "9.29.952.8")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$cache = Join-Path $root ".pw-local-deps"
$archive = Join-Path $cache ("microsoft.dxsdk.d3dx." + $PackageVersion + ".nupkg")
$libraryDir = Join-Path $root "GameGuru Core/SDK/DirectX/Lib/x64"
New-Item -ItemType Directory -Force -Path $cache,$libraryDir | Out-Null

$names = @("d3dx11.lib","d3dx9.lib")
$allReady = $true
foreach ($name in $names) {
    $candidate = Join-Path $libraryDir $name
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf) -or
        (Get-Item -LiteralPath $candidate).Length -lt 1024) {
        $allReady = $false
    }
}
if ($allReady) {
    Write-Host "PW D3DX x64 libraries already staged; no download required."
    return
}

# Older Windows PowerShell may default to pre-TLS 1.2, which NuGet refuses.
[Net.ServicePointManager]::SecurityProtocol =
    [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
$url = "https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/$PackageVersion/microsoft.dxsdk.d3dx.$PackageVersion.nupkg"
if (-not (Test-Path -LiteralPath $archive -PathType Leaf) -or
    (Get-Item -LiteralPath $archive).Length -lt 4096) {
    Write-Host "Retrieving official Microsoft.DXSDK.D3DX $PackageVersion from NuGet.org"
    Invoke-WebRequest -Uri $url -OutFile $archive -MaximumRedirection 5 -UseBasicParsing
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

# The outer package is a ZIP (NuGet .nupkg), but Expand-Archive in Windows
# PowerShell 5.1 can choke on long nested paths or full archive expansion.
# Read just the two required entries directly as streams, with fixed outputs.
for ($attempt = 1; $attempt -le 2; $attempt++) {
    $package = $null
    try {
        $package = [System.IO.Compression.ZipFile]::OpenRead($archive)
        foreach ($name in $names) {
            $target = Join-Path $libraryDir $name
            $entry = $package.Entries | Where-Object {
                $_.FullName -match ("(?i)(^|/)build/native/release/lib/x64/" + [regex]::Escape($name) + "$")
            } | Select-Object -First 1
            if (-not $entry) {
                # Some Microsoft package layouts add an intermediate directory.
                $entry = $package.Entries | Where-Object {
                    $_.FullName -match ("(?i)(^|/)release/lib/x64/" + [regex]::Escape($name) + "$")
                } | Select-Object -First 1
            }
            if (-not $entry -or $entry.Length -lt 1024) {
                throw "Microsoft NuGet package has no valid release x64 $name entry"
            }
            $staged = "$target.pw-part"
            $inputStream = $null
            $outputStream = $null
            try {
                $inputStream = $entry.Open()
                $outputStream = [System.IO.File]::Create($staged)
                $inputStream.CopyTo($outputStream)
            }
            finally {
                if ($outputStream) { $outputStream.Dispose() }
                if ($inputStream) { $inputStream.Dispose() }
            }
            if ((Get-Item -LiteralPath $staged).Length -ne $entry.Length) {
                throw "D3DX package stream size mismatch while extracting $name"
            }
            Move-Item -LiteralPath $staged -Destination $target -Force
            Write-Host "Staged $name from $($entry.FullName)"
        }
        Write-Host "PW D3DX x64 import libraries staged into $libraryDir (not committed)."
        break
    }
    catch {
        $reason = $_.Exception.Message
        foreach ($name in $names) {
            $partial = Join-Path $libraryDir "$name.pw-part"
            if (Test-Path -LiteralPath $partial) {
                Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue
            }
        }
        if ($attempt -ge 2) {
            throw "D3DX dependency preparation failed: $reason (package $archive)"
        }
        Write-Warning "Existing D3DX package may be incomplete: $reason. Downloading a fresh official NuGet package..."
        if ($package) { $package.Dispose(); $package = $null }
        Remove-Item -LiteralPath $archive -Force -ErrorAction SilentlyContinue
        Invoke-WebRequest -Uri $url -OutFile $archive -MaximumRedirection 5 -UseBasicParsing
    }
    finally {
        if ($package) { $package.Dispose() }
    }
}
