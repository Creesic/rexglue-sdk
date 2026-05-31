# Portable workspace (copy/paste friendly)

ReXGlue is meant to live in a **relocatable folder tree**: SDK at the repo root,
title apps (`FM2/`, `FH1/`, …) as siblings. You should be able to copy the
whole directory to another drive or PC and rebuild after a short reset — not
chase hardcoded `C:\Users\...` paths.

## Expected layout

```text
ReXGlue080/                 # SDK source (this repo)
  cmake/
  include/rex/
  out/install/win-amd64/    # SDK install prefix (after `cmake --install`)
  FM2/                      # title project
    assets/default.xex
    generated/rexglue.cmake
    out/build/...
  FH1/
```

Title projects resolve the SDK in this order:

1. `REXSDK_DIR` CMake cache (optional override)
2. Parent directory if it looks like the SDK (`../cmake/rexglueConfig.cmake.in`)
3. Installed package via `find_package(rexglue)` and `CMAKE_PREFIX_PATH`

`FM2/CMakePresets.json` sets `CMAKE_PREFIX_PATH` to
`${sourceDir}/../out/install/win-amd64` so an installed SDK is found without
absolute paths.

## What breaks after copy/paste

| Problem | Cause | Fix |
| --- | --- | --- |
| CMake errors about wrong `CMAKE_HOME_DIRECTORY` | Stale `out/build/*/CMakeCache.txt` | Run `rex-relocate`, then re-configure |
| SDK not found | No install under `out/install/` yet | Build/install SDK at new location |
| `REXSDK_DIR` still points at old path | Old preset or cache | Remove from `CMakePresets.json`; use `rex-relocate` |
| Codegen cannot open XEX | Absolute `file_path` in `*_config.toml` | Use paths relative to the title dir (e.g. `assets/default.xex`) |
| Compiler not found | Machine-specific paths in committed presets | Copy `CMakeUserPresets.json.example` → `CMakeUserPresets.json` and edit |

## Automated repair

Import `scripts/PSReX` (PowerShell 7+). Use **`rex-repair`** to scan and fix issues
without waiting for a failed build:

```powershell
Import-Module .\scripts\PSReX -Force
rex-repair              # apply fixes
rex-repair -WhatIf      # preview only
```

**Auto-repair on failure:** `rex-configure`, `rex-build`, and **`rex-cmake`**
(cmake from a title directory) run `rex-repair` once and retry if cmake fails.
Disable with:

```powershell
$env:REXGLUE_SKIP_AUTO_REPAIR = '1'
```

`rex-repair` checks:

- Stale `out/build/*/CMakeCache.txt` (wrong or missing `CMAKE_HOME_DIRECTORY`, bad `REXSDK_DIR`)
- Absolute `REXSDK_DIR` in title `CMakePresets.json`
- Missing `${sourceDir}/../out/install/win-amd64` on `windows-amd64-base`
- Absolute `file_path` / `game_root` in title `*.toml`
- Outdated `generated/rexglue.cmake` (patches portable parent-SDK discovery when possible)
- Missing root `CMakeUserPresets.json` (copies from `.example`)
- Stale `rexruntimerd.dll` in title build dirs (sync from SDK install)

## After moving the folder

From PowerShell (repo root):

```powershell
Import-Module .\scripts\PSReX -Force
rex-repair

# SDK (auto-repair on failure)
rex-configure
rex-build -Config RelWithDebInfo

# Title (example FM2) — use rex-cmake so failures trigger repair
Set-Location .\FM2
rex-cmake -Preset win-amd64-relwithdebinfo
rex-cmake -Preset win-amd64-relwithdebinfo -BuildOnly -Config RelWithDebInfo
```

Copy the fresh runtime DLL into the title build dir when you change the SDK
(see `AGENTS.md` build workflow).

## Machine-specific toolchain (not path-specific)

Committed `CMakePresets.json` at the repo root may list Visual Studio paths for
one machine. Per-machine overrides belong in **`CMakeUserPresets.json`** (gitignored):

```powershell
Copy-Item CMakeUserPresets.json.example CMakeUserPresets.json
# Edit compiler and LIBPATH entries for your VS install
```

Title app presets intentionally use `clang-cl` by name or `${sourceDir}`-relative
prefix paths — avoid baking your username into `CMakePresets.json`.

## Optional: ISO and game assets

`rexglue_companion_workspace.json` and game ISO paths are **local** and not required
for building. Keep ISOs outside the repo or update that JSON after a move.

## Checklist

- [ ] Whole tree copied (SDK + title + `out/install` if you want offline rebuild)
- [ ] `rex-relocate` (or delete `*/out/build/*` configure dirs manually)
- [ ] `CMakeUserPresets.json` for your compiler (root SDK build)
- [ ] Title `file_path` / manifest paths are relative to the title directory
- [ ] No `REXSDK_DIR` pointing at the old absolute SDK path
