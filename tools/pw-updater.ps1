# GGClassic PW Edition update helper for Windows PowerShell 5.1.
# Runs out-of-process, never updates the live editor, and never deletes user project media.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstallDir,
 [int]$EditorPid=0,
 [ValidateSet("Guru-MapEditor.exe","GameGuru.exe")][string]$ExeName="Guru-MapEditor.exe",
 [switch]$Apply,
 [string]$StageDir=""
)
Set-StrictMode -Version Latest
$ErrorActionPreference="Stop"
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$Title="GGClassic PW Edition Updater"
$Repo="BoomForge/GGClassic-PW-Edition-"
$AssetName="GGClassic-PW-Edition-win64.zip"
$ChecksumName="$AssetName.sha256"
$InstallDir=[IO.Path]::GetFullPath($InstallDir).TrimEnd('\')

function Notice([string]$message) {
 [void][System.Windows.Forms.MessageBox]::Show($message,$Title,"OK","Information")
}
function Ask([string]$message) {
 return [System.Windows.Forms.MessageBox]::Show($message,$Title,"YesNo","Question") -eq "Yes"
}
function Require([bool]$condition,[string]$message) {
 if (-not $condition) { throw $message }
}
function Get-Version([string]$versionText) {
 Require ($versionText -match '^v?(\d+\.\d+\.\d+)$') "Invalid version string: $versionText"
 return [version]$Matches[1]
}
function Check-Relative([string]$relative) {
 Require (-not [string]::IsNullOrWhiteSpace($relative)) "Empty package path."
 Require ($relative -notmatch '[:\x00-\x1F]') "Invalid package path."
 $name=$relative.Replace('/','\')
 Require (-not $name.StartsWith('\')) "Absolute package path."
 foreach ($part in $name.Split('\')) {
  Require ($part -ne "" -and $part -ne "." -and $part -ne ".." -and
    -not $part.EndsWith(".") -and -not $part.EndsWith(" ")) "Unsafe package path."
 }
 # Explicitly excludes entitybank, terrainbank, FPM files, levels and local settings.
 $allowed=$name -match '^(?i:(Guru-MapEditor\.exe|GameGuru\.exe|pw-updater\.ps1|pw-edition-version\.txt|[a-z0-9_.-]+\.dll|Files\\effectbank\\reloaded\\[a-z0-9_.-]+\.fx))$'
 Require $allowed "Unapproved update file: $name"
 return $name
}
function Safe-Path([string]$root,[string]$rel) {
 $rel=Check-Relative $rel
 $path=[IO.Path]::GetFullPath((Join-Path $root $rel))
 Require ($path.StartsWith($root.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) "Package path escapes its directory."
 return $path
}
function Verify-Hash([string]$file,[string]$sha) {
 Require ($sha -match '^[a-fA-F0-9]{64}$') "Invalid checksum."
 Require (Test-Path -LiteralPath $file -PathType Leaf) "Missing update file: $file"
 Require ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -eq $sha.ToUpperInvariant()) "SHA-256 mismatch: $file"
}
function Validate-Manifest([string]$dir) {
 $manifestPath=Join-Path $dir "pw-release.json"
 Require (Test-Path -LiteralPath $manifestPath -PathType Leaf) "Release manifest missing."
 $manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
 [void](Get-Version ([string]$manifest.version))
 Require ($null -ne $manifest.files -and $manifest.files.Count -gt 0) "Release manifest has no files."
 $names=@{}
 foreach ($f in $manifest.files) {
  $name=Check-Relative ([string]$f.path)
  Require (-not $names.ContainsKey($name)) "Duplicate release file."
  $names[$name]=$true
  Verify-Hash (Safe-Path $dir $name) ([string]$f.sha256)
 }
 foreach ($required in @("Guru-MapEditor.exe","pw-updater.ps1","pw-edition-version.txt")) {
  Require ($names.ContainsKey($required)) "Required update file missing: $required"
 }
 $marker=(Get-Content -LiteralPath (Join-Path $dir "pw-edition-version.txt") -Raw).Trim()
 Require ($marker -eq [string]$manifest.version) "Version marker does not match update."
 return $manifest
}
function Stage-Zip([string]$zipPath,[string]$dir,[string]$tag) {
 $zip=[IO.Compression.ZipFile]::OpenRead($zipPath)
 try {
  $entries=@{}
  foreach ($entry in $zip.Entries) {
   Require (-not [string]::IsNullOrEmpty($entry.Name)) "ZIP directories are not allowed."
   $name=$entry.FullName.Replace('/','\')
   if ($name -ne "pw-release.json") { [void](Check-Relative $name) }
   Require (-not $entries.ContainsKey($name)) "Duplicate ZIP entry."
   $entries[$name]=$entry
  }
  Require ($entries.ContainsKey("pw-release.json")) "ZIP release manifest missing."
  $reader=[System.IO.StreamReader]::new($entries["pw-release.json"].Open())
  try { $manifest=$reader.ReadToEnd() | ConvertFrom-Json }
  finally { $reader.Dispose() }
  Require ([string]$manifest.version -eq $tag) "Release tag/ZIP version mismatch."
  $names=@{}
  foreach ($f in $manifest.files) {
   $name=Check-Relative ([string]$f.path)
   Require (-not $names.ContainsKey($name)) "Duplicate manifest entry."
   Require ($entries.ContainsKey($name)) "Missing archive item: $name"
   $names[$name]=$true
  }
  Require (($names.Count+1) -eq $entries.Count) "Unexpected ZIP content."
  [IO.File]::WriteAllText((Join-Path $dir "pw-release.json"),($manifest | ConvertTo-Json -Depth 12),([System.Text.UTF8Encoding]::new($false)))
  foreach ($name in $names.Keys) {
   $target=Safe-Path $dir $name
   [void](New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force)
   $source=$entries[$name].Open()
   try {
    $dest=[IO.File]::Open($target,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
    try { $source.CopyTo($dest) } finally { $dest.Dispose() }
   } finally { $source.Dispose() }
  }
 } finally { $zip.Dispose() }
 [void](Validate-Manifest $dir)
}
function Can-Write([string]$dir) {
 $probe=Join-Path $dir (".pw-write-probe-"+[guid]::NewGuid().ToString("N"))
 try { [IO.File]::WriteAllText($probe,"test"); [IO.File]::Delete($probe); return $true }
 catch { return $false }
}
function Apply-Update([string]$dir) {
 Require (Test-Path -LiteralPath (Join-Path $InstallDir "pw-edition-version.txt") -PathType Leaf) "Refusing to overwrite a non-PW Edition installation."
 $manifest=Validate-Manifest $dir
 $backupDir=Join-Path $dir "backup"
 [void](New-Item -ItemType Directory -Path $backupDir -Force)
 $written=[System.Collections.ArrayList]::new()
 try {
  # Back up everything first: a failed copy can then restore the original state.
  foreach ($f in $manifest.files) {
   $rel=Check-Relative ([string]$f.path)
   $dest=Safe-Path $InstallDir $rel
   if (Test-Path -LiteralPath $dest -PathType Leaf) {
    $bak=Safe-Path $backupDir $rel
    [void](New-Item -ItemType Directory -Path (Split-Path -Parent $bak) -Force)
    Copy-Item -LiteralPath $dest -Destination $bak -Force
   }
  }
  foreach ($f in $manifest.files) {
   $rel=Check-Relative ([string]$f.path)
   $dest=Safe-Path $InstallDir $rel
   [void](New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force)
   [void]$written.Add($rel)
   Copy-Item -LiteralPath (Safe-Path $dir $rel) -Destination $dest -Force
   Verify-Hash $dest ([string]$f.sha256)
  }
 } catch {
  $reason=$_.Exception.Message
  for ($i=$written.Count-1;$i -ge 0;$i--) {
   $rel=[string]$written[$i]
   $dest=Safe-Path $InstallDir $rel
   $bak=Safe-Path $backupDir $rel
   try {
    if (Test-Path -LiteralPath $bak -PathType Leaf) {
     Copy-Item -LiteralPath $bak -Destination $dest -Force
    } elseif (Test-Path -LiteralPath $dest -PathType Leaf) {
     Remove-Item -LiteralPath $dest -Force
    }
   } catch { $reason+=" Rollback incomplete: $rel." }
  }
  throw "Installation failed; original files restored where possible. $reason Backup: $backupDir"
 }
 Notice "GGClassic PW Edition $($manifest.version) installed. Your worlds and custom media were left untouched."
 if (Ask "Restart GGClassic PW Edition now?") {
  $exe=Join-Path $InstallDir $ExeName
  Require (Test-Path -LiteralPath $exe -PathType Leaf) "Editor executable missing after install."
  Start-Process -FilePath $exe -WorkingDirectory $InstallDir
 }
}
try {
 Require (Test-Path -LiteralPath $InstallDir -PathType Container) "Editor install folder not found."
 Require (Test-Path -LiteralPath (Join-Path $InstallDir "pw-edition-version.txt") -PathType Leaf) "Not a PW Edition install (missing version marker)."

 if ($Apply) {
  Require (-not [string]::IsNullOrWhiteSpace($StageDir)) "Staging path missing."
  $safeRoot=[IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA "Pixelwolf\GGClassicPW\Updates")).TrimEnd('\')
  $StageDir=[IO.Path]::GetFullPath($StageDir).TrimEnd('\')
  Require ($StageDir.StartsWith($safeRoot+'\', [StringComparison]::OrdinalIgnoreCase)) "Unexpected staging path."
  Apply-Update $StageDir
  exit 0
 }

 $currentText=(Get-Content -LiteralPath (Join-Path $InstallDir "pw-edition-version.txt") -Raw).Trim()
 $current=Get-Version $currentText
 [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
 $headers=@{"User-Agent"="GGClassic-PW-Edition-Updater";"Accept"="application/vnd.github+json"}
 try {
  $release=Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest" -Headers $headers -UseBasicParsing -TimeoutSec 30
 } catch {
  Notice "No PW Edition release is published yet, or GitHub is unavailable. No files were changed."
  exit 0
 }
 $tag=[string]$release.tag_name
 $latest=Get-Version $tag
 if ($latest -le $current) { Notice "GGClassic PW Edition $currentText is already up to date."; exit 0 }

 $zipAsset=$release.assets | Where-Object { $_.name -ceq $AssetName } | Select-Object -First 1
 $shaAsset=$release.assets | Where-Object { $_.name -ceq $ChecksumName } | Select-Object -First 1
 Require ($null -ne $zipAsset -and $null -ne $shaAsset) "Release ZIP or SHA-256 file missing."
 foreach ($asset in @($zipAsset,$shaAsset)) {
  $url=[uri]$asset.browser_download_url
  Require ($url.Scheme -eq "https" -and $url.Host -eq "github.com" -and
   $url.AbsolutePath.StartsWith("/$Repo/releases/download/",[StringComparison]::OrdinalIgnoreCase)) "Unexpected release download host."
 }
 if (-not (Ask "PW Edition $tag is available (installed: $currentText). Download and install?")) { exit 0 }

 $stage=Join-Path (Join-Path $env:LOCALAPPDATA "Pixelwolf\GGClassicPW\Updates") ([guid]::NewGuid().ToString("N"))
 [void](New-Item -ItemType Directory -Path $stage -Force)
 $zipPath=Join-Path $stage $AssetName
 $shaPath=Join-Path $stage $ChecksumName
 Invoke-WebRequest -Uri $zipAsset.browser_download_url -Headers $headers -UseBasicParsing -OutFile $zipPath -TimeoutSec 900
 Invoke-WebRequest -Uri $shaAsset.browser_download_url -Headers $headers -UseBasicParsing -OutFile $shaPath -TimeoutSec 120
 $sha=(Get-Content -LiteralPath $shaPath -Raw).Trim()
 Verify-Hash $zipPath $sha

 $payload=Join-Path $stage "payload"
 [void](New-Item -ItemType Directory -Path $payload -Force)
 Stage-Zip $zipPath $payload $tag.TrimStart('v')
 if (-not (Ask "Update downloaded and verified. SAVE YOUR WORK, then close the editor. Installation starts after the editor exits. Continue?")) { exit 0 }
 if ($EditorPid -gt 0) {
  try { Wait-Process -Id $EditorPid -Timeout 1200 -ErrorAction Stop }
  catch {
   if (Get-Process -Id $EditorPid -ErrorAction SilentlyContinue) {
    throw "Editor did not close within 20 minutes. No files were replaced."
   }
  }
 }

 if (-not (Can-Write $InstallDir)) {
  $argumentLine='-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "'+$PSCommandPath+
   '" -Apply -InstallDir "'+$InstallDir+'" -StageDir "'+$payload+'" -ExeName "'+$ExeName+'"'
  $child=Start-Process -FilePath (Join-Path $PSHOME "powershell.exe") -ArgumentList $argumentLine -Verb RunAs -Wait -PassThru
  Require ($child.ExitCode -eq 0) "Elevation or installation was cancelled/failed."
 } else {
  Apply-Update $payload
 }
} catch {
 [void][System.Windows.Forms.MessageBox]::Show($_.Exception.Message,$Title,"OK","Error")
 exit 1
}
