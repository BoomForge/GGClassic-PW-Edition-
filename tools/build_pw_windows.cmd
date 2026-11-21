@echo off
setlocal EnableExtensions
cd /d "%~dp0.."
set "PW_VERSION=%~1"
if not defined PW_VERSION set "PW_VERSION=0.1.0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: Install Visual Studio 2022 Desktop C++ with x64 MFC.
  exit /b 1
)
set "VS_PATH="
for /f "tokens=*" %%I in ('"%VSWHERE%" -latest -products * -property installationPath') do set "VS_PATH=%%I"
if not defined VS_PATH (
  echo ERROR: No Visual Studio installation found.
  exit /b 1
)
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 (
  echo ERROR: Could not initialise the x64 compiler.
  exit /b 1
)
where msbuild >nul 2>&1
if errorlevel 1 exit /b 1
if not exist "PW-Build" mkdir "PW-Build"
echo Building full GGClassic PW Edition Release x64...
msbuild "GameGuru Core\GameGuru.sln" /t:Build /p:Configuration=Release /p:Platform=x64 /m:2 /nologo /v:minimal /fl /flp:logfile=PW-Build\pw-windows-build.log;verbosity=normal
if errorlevel 1 (
  echo BUILD FAILED. See PW-Build\pw-windows-build.log. Steam files are unchanged.
  exit /b 1
)
if not exist "PW-Build\Release\Guru-MapEditor.exe" (
  echo ERROR: Build did not produce the editor executable.
  exit /b 1
)
where py >nul 2>&1
if errorlevel 1 (
  echo ERROR: Python 3 launcher missing; compilation succeeded but package was not created.
  exit /b 1
)
py -3 tools\make_pw_steam_overlay.py --build-dir "PW-Build\Release" --version "%PW_VERSION%" --out-dir dist
if errorlevel 1 (
  echo ERROR: Could not package the Steam overlay.
  exit /b 1
)
echo COMPLETE: Local patch, SHA-256 checksum and installer are in dist.
echo No changes were made to your Steam installation.
exit /b 0
