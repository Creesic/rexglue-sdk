# ReXGlue080ForzaHorizon — Agent Guide

Monorepo: **ReXGlue SDK at repo root**, **FH1 title in `FH1/`**. Start from
upstream SDK behavior; add title-specific code only when evidence requires it.

## Layout

```text
ReXGlue080ForzaHorizon/     # SDK (configure/build/install here)
  out/install/win-amd64/    # SDK prefix after install
  FH1/                      # Forza Horizon 1 title project
    fh1_manifest.toml       # Codegen source of truth
    generated/              # Codegen output (do not hand-edit permanently)
    src/                    # Title hooks (keep minimal)
```

## Rules

- Work from logs/captures/disassembly before patching.
- Prefer **SDK fixes** when behavior should match Xbox 360 semantics.
- Prefer **manifest `[functions]` names** for title-specific hooks, not generated C++ edits.
- **`FH1/generated/` is codegen output** — temporary experiments OK; permanent fixes belong in SDK, manifest, or `FH1/src/`.
- Do not copy the old `ReXGlue080FH1` guard/fiber pile wholesale. Re-introduce fixes one at a time with docs in `docs/FH1/`.
- No git commits unless the user asks.

## Build workflow (Windows)

From a **Visual Studio Developer PowerShell** (or after `Launch-VsDevShell.ps1`):

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080ForzaHorizon'

# 1) SDK
cmake --preset win-amd64
cmake --build out/build/win-amd64 --config RelWithDebInfo --target install

# 2) FH1 codegen (long; only when manifest changes)
Set-Location .\FH1
cmake --preset win-amd64-relwithdebinfo
cmake --build out/build/win-amd64-relwithdebinfo --target fh1_codegen

# 3) FH1 app — copy fresh runtime DLL after SDK changes
Copy-Item -LiteralPath '..\out\install\win-amd64\bin\rexruntimerd.dll' `
  -Destination 'out\build\win-amd64-relwithdebinfo\rexruntimerd.dll' -Force
cmake --build out/build/win-amd64-relwithdebinfo --config RelWithDebInfo --target fh1
```

FH1 presets set `REXSDK_DIR` to `${sourceDir}/..` so the title builds against the
in-tree SDK without absolute paths.

## Debugging

- Log file commonly used: `C:\temp\fh1-test.log`
- Roadmap and phase notes: `docs/FH1/README.md`
