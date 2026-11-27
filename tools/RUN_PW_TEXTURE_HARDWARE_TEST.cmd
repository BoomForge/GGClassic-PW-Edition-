@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%LOCALAPPDATA%\Pixelwolf\GGClassicPW\TextureBenchmark"
if not exist "%OUT%" mkdir "%OUT%"
where cl.exe >nul 2>nul
if errorlevel 1 (
    if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto :noCompiler
    for /f "usebackq tokens=*" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%I"
    if not defined VS goto :noCompiler
    call "%VS%\VC\Auxiliary\Build\vcvars64.bat"
    if errorlevel 1 goto :noCompiler
)
pushd "%OUT%"
cl /nologo /EHsc /std:c++17 /O2 /I"%ROOT%\GameGuru Core\GameGuru\Include" "%ROOT%\tools\test_pw_loadtrace_win.cpp" /Fe:pw_texture_hardware.exe > build.log 2>&1
if errorlevel 1 (
    echo Hardware probe compilation failed. See "%OUT%\build.log".
    popd
    exit /b 1
)
set "GGCPW_REQUIRE_TILED_TEST=1"
pw_texture_hardware.exe > hardware-result.log 2>&1
set "RESULT=%ERRORLEVEL%"
type hardware-result.log
echo Report: "%OUT%\hardware-result.log"
popd
if not "%RESULT%"=="0" echo Hardware probe failed or unsupported. Do not enable live tiled streaming.
exit /b %RESULT%
:noCompiler
echo Visual Studio C++ x64 Build Tools not found. Install the Desktop development with C++ workload.
exit /b 2
