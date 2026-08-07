@echo off
REM Wrapper batch script to launch the PowerShell build script on Windows

cd /d "%~dp0"

where.exe pwsh >nul 2>&1
if not errorlevel 1 (
    pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-vs2002.ps1"
    if errorlevel 1 exit /b %ERRORLEVEL%
    pwsh -NoProfile -Command "Copy-Item .\build\source\CircleShoot\Zuma.exe .\TestGame\Zuma.exe -Force; Start-Process -FilePath '.\TestGame\Zuma.exe' -WorkingDirectory '.\TestGame'"
    exit /b %ERRORLEVEL%
)

where.exe powershell >nul 2>&1
if not errorlevel 1 (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-vs2002.ps1"
    if errorlevel 1 exit /b %ERRORLEVEL%
    powershell.exe -NoProfile -Command "Copy-Item .\build\source\CircleShoot\Zuma.exe .\TestGame\Zuma.exe -Force; Start-Process -FilePath '.\TestGame\Zuma.exe' -WorkingDirectory '.\TestGame'"
    exit /b %ERRORLEVEL%
)

echo Error: PowerShell is not found on this system.
echo Please install PowerShell or ensure it is in your PATH.
exit /b 1
