@echo off
setlocal EnableExtensions

rem Launch FM2 with the current native Plume side-by-side renderer flags.
rem Extra arguments passed to this script are appended after these defaults.
rem Use --dry-run as the first argument to print the command without launching.

set "REPO_ROOT=%~dp0..\.."
cd /d "%REPO_ROOT%" || (
  echo ERROR: could not cd to repo root: %REPO_ROOT%
  exit /b 1
)

set "PRESET=win-amd64-relwithdebinfo"
set "FM2_WORKDIR=%REPO_ROOT%\FM2"
set "FM2_EXE=%FM2_WORKDIR%\out\build\%PRESET%\fm2.exe"
set "SDK_DLL_BUILD=%REPO_ROOT%\out\win-amd64\rexruntimerd.dll"
set "FM2_DLL=%FM2_WORKDIR%\out\build\%PRESET%\rexruntimerd.dll"
set "DRY_RUN=0"
set "EXTRA_ARGS="

if /I "%~1"=="--dry-run" (
  set "DRY_RUN=1"
  shift /1
)

:collect_args
if "%~1"=="" goto args_done
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift /1
goto collect_args
:args_done

if not exist "%FM2_EXE%" (
  echo ERROR: fm2.exe not found: %FM2_EXE%
  echo Run scripts\fm2\rebuild-fm2-plume-native.bat first.
  exit /b 1
)

if exist "%SDK_DLL_BUILD%" (
  copy /Y "%SDK_DLL_BUILD%" "%FM2_DLL%" >nul
  if errorlevel 1 (
    if exist "%FM2_DLL%" (
      echo WARNING: could not sync fresh rexruntimerd.dll; using existing FM2-local DLL.
      echo          If FM2 is already running, close it before relaunching for a fresh DLL.
    ) else (
      echo ERROR: failed to sync fresh rexruntimerd.dll.
      exit /b 1
    )
  )
) else if not exist "%FM2_DLL%" (
  echo ERROR: rexruntimerd.dll not found in root build output or FM2 build dir.
  echo Run scripts\fm2\rebuild-fm2-plume-native.bat first.
  exit /b 1
)

set "REX_MNK_MODE=1"

rem Present-flushed live replay: hybrid draws queue into a batch and submit on
rem FM2_D3D_TryPresentAndUpdateStatus (FM2PlumeTracePresent). Required cvars:
rem   fm2_plume_mode=shadow
rem   fm2_plume_native_direct_draw=1
rem   fm2_plume_native_direct_draw_live_batch=1
set FM2_ARGS=--fm2_plume_mode shadow ^
 --fm2_plume_native_direct_draw 1 ^
 --fm2_plume_native_direct_draw_limit 0 ^
 --fm2_plume_native_direct_draw_live_batch 1 ^
 --fm2_plume_native_direct_draw_live_batch_size 16 ^
 --fm2_plume_debug_replay_window ^
 --fm2_plume_wireframe ^
 --mnk_mode ^
 --log_level info

echo Launching FM2 native Plume renderer:
echo   Exe:  %FM2_EXE%
echo   Work: %FM2_WORKDIR%
echo   Args: %FM2_ARGS%%EXTRA_ARGS%

if "%DRY_RUN%"=="1" (
  echo Dry run only; FM2 was not launched.
  exit /b 0
)

start "FM2 Plume Native" /D "%FM2_WORKDIR%" "%FM2_EXE%" %FM2_ARGS%%EXTRA_ARGS%
exit /b %ERRORLEVEL%
