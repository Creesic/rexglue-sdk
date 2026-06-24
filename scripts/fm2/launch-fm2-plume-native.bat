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

rem Debug replay: intercept a direct-draw plan each frame and render it into a
rem separate side-by-side window using the diagnostic pipeline (no textures).
rem
rem fm2_plume_direct_replay_transform_source selects which VS float constant
rem block is used as the camera matrix:
rem   auto         - heuristic scan of register file (may miss)
rem   c28          - register c28, known FM2 VP matrix slot (try first)
rem   c0           - register c0
rem   c36_mul_c28  - c36 * c28 combined transform
rem
rem fm2_plume_debug_replay_transform_mode tells the shader how to read the matrix:
rem   column_major_clip  - column-major interpretation (try first)
rem   row_major_clip     - row-major interpretation
rem   row_major_clip_z_mid - row-major + D3D Z=[0,1] mid-range remap
rem
rem fm2_plume_debug_replay_limit: 0 = unlimited (window updates every frame)
rem                                1 = freeze on first draw (single-shot diagnostic)
rem
rem To switch to the native-packet batch path instead, replace the debug_replay
rem block below with:
rem   --fm2_plume_native_direct_draw 1
rem   --fm2_plume_native_direct_draw_limit 0
rem   --fm2_plume_native_direct_draw_live_batch 1
rem   --fm2_plume_native_direct_draw_live_batch_size 16
set FM2_ARGS=--fm2_plume_mode plume_native ^
 --fm2_plume_debug_replay 1 ^
 --fm2_plume_debug_replay_limit 0 ^
 --fm2_plume_debug_replay_window ^
 --fm2_plume_debug_replay_side_by_side ^
 --fm2_plume_direct_replay_transform_source c36_mul_c28 ^
 --fm2_plume_debug_replay_transform_mode row_major_clip ^
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
