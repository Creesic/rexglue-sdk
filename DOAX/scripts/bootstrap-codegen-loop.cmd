@echo off
setlocal
cd /d "%~dp0.."
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0bootstrap-codegen-loop.ps1" %*
set EXITCODE=%ERRORLEVEL%
if %EXITCODE% NEQ 0 (
  echo.
  echo Script failed with exit code %EXITCODE%.
)
exit /b %EXITCODE%
