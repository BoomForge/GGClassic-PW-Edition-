@echo off
title Pixelwolf GG Classic - Build a separate test installation
cd /d "%~dp0"
echo Pixelwolf GG Classic - local Windows test build
echo Select the GameGuru Classic Steam folder when asked.
echo Your Steam installation will NOT be modified.
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\pw-local-test.ps1"
echo.
echo If any error appears above, do not run a previous compiled build.
pause
