@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0windows-wizard.ps1"
if errorlevel 1 (
  echo DLSS-NR setup could not start. Keep this message for your bug report.
  pause
)
exit /b %errorlevel%
