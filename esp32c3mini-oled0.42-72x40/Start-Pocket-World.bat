@echo off
cd /d "%~dp0"
if exist "bin\PocketWorld.exe" (
  start "" "bin\PocketWorld.exe"
) else if exist "dist\native\PocketWorld.exe" (
  start "" "dist\native\PocketWorld.exe"
) else if exist ".venv\Scripts\pythonw.exe" (
  start "" ".venv\Scripts\pythonw.exe" "controller\app.py"
) else (
  echo Build the controller with tools\build_native_controller.ps1 first.
  pause
)
