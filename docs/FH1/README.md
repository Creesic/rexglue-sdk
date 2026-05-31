# FH1 (Forza Horizon 1) — clean bring-up

Date: 2026-05-31

This repo replaces the experimental `ReXGlue080FH1` tree. Goal: **vanilla SDK +
minimal title hooks**, adding fixes only when a crash/hang is understood.

## Phase plan

| Phase | Goal | Status |
| --- | --- | --- |
| 0 | SDK configures and builds; `rexglue` CLI works | Done (local) |
| 1 | `rexglue init` → bare `FH1/` (manifest + CMake + `main.cpp`) | Done |
| 2 | First full codegen (`fh1_codegen`) + link `fh1.exe` | Next |
| 3 | Boot to first frame / title menu (no custom guards) | Pending |
| 4 | Re-add targeted fixes from old repo **one issue at a time** | Pending |

## Game assets

Manifest paths point at the extracted game folder (not copied into git):

- Game root: `D:/Emulation/Games_Xbox_360/ForzaHorizon/ForzaHorizon`
- Entrypoint: `default.xex`
- Facades: `XMediaFacade_default.xex`, `SpeechFacade_default.xex`

To relocate, edit `FH1/fh1_manifest.toml` (or re-run `rexglue init` with new paths).

## What we are *not* importing yet

From the old FH1 tree, keep these out until the baseline runs:

- Guest-PC fiber interpreter / world-load worker inference
- Work-queue dispatch wrappers
- Track-loader / loader-epoch guards
- Large `[functions]` manifest tables

Document each reintroduction in this folder with addresses, log lines, and the
minimal fix.

## Useful old-repo references

When a phase-3+ bug matches prior work, read (do not bulk-copy):

- `ReXGlue080FH1/docs/FM2-xam-notes.md` (sign-in patterns; FH1 may differ)
- `ReXGlue080FH1/FH1/fh1_manifest.toml` (named functions, fiber sites)
- `ReXGlue080FH1/docs/FH1-guest-pc-fiber-interpreter.md`
