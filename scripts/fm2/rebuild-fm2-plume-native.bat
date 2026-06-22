@echo off
setlocal EnableExtensions

rem Rebuild SDK runtime + FM2 after Plume native-renderer or manifest hook changes.
rem Paths are relative to the repo root. FM2's target graph rebuilds the runtime
rem when needed; copy the runtime DLL after the FM2 build so the game never runs
rem with a stale rexruntimerd.dll.

set "REPO_ROOT=%~dp0..\.."
cd /d "%REPO_ROOT%" || (
  echo ERROR: could not cd to repo root: %REPO_ROOT%
  exit /b 1
)

set "PRESET=win-amd64-relwithdebinfo"
set "SDK_DLL_BUILD=out\win-amd64\rexruntimerd.dll"
set "SDK_DLL_INSTALL=out\install\win-amd64\bin\rexruntimerd.dll"
set "FM2_DIR=FM2\out\build\%PRESET%"

echo.
echo === [1/3] FM2 codegen (manifest / midasm hooks) ===
pushd FM2
cmake --build --preset %PRESET% --target fm2_codegen
if errorlevel 1 (
  popd
  echo ERROR: fm2_codegen failed.
  exit /b 1
)

echo.
echo === [2/3] FM2 build ===
cmake --build --preset %PRESET% --target fm2
set "BUILD_RC=%ERRORLEVEL%"
popd
if not "%BUILD_RC%"=="0" (
  echo ERROR: fm2 build failed.
  exit /b %BUILD_RC%
)

echo.
echo === [3/3] Sync fresh rexruntimerd.dll into FM2 build dir ===
if not exist "%FM2_DIR%\" (
  echo ERROR: FM2 build dir missing: %FM2_DIR%
  exit /b 1
)
if exist "%SDK_DLL_BUILD%" (
  set "SDK_DLL_SRC=%SDK_DLL_BUILD%"
) else if exist "%SDK_DLL_INSTALL%" (
  set "SDK_DLL_SRC=%SDK_DLL_INSTALL%"
) else (
  echo ERROR: could not find rexruntimerd.dll in build or install output.
  exit /b 1
)
copy /Y "%SDK_DLL_SRC%" "%FM2_DIR%\rexruntimerd.dll"
if errorlevel 1 (
  echo ERROR: failed to copy rexruntimerd.dll.
  exit /b 1
)
echo Copied from %SDK_DLL_SRC%

echo.
echo OK: FM2 rebuilt with fresh SDK runtime and codegen.
echo Exe: FM2\out\build\%PRESET%\fm2.exe
exit /b 0
