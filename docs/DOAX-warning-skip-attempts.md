# DOAX License Warning Skip - Attempts Log (2026-06-23)

This documents everything tried to skip the boot license/warning scroll (`spWarn.xpr` via
scheduler/UI), what failed, and the working skip. Opening movie A-button skip is a separate
path and must not be auto-skipped via `DOAX_PlayMovie`.

## Boot Order

Observed/user-corrected order: warning -> ninja -> opening. Handler table order is not the
same thing as execution order.

## What The Warning Actually Is

| | Movies (ninja / opening) | License warning |
|---|---|---|
| Asset | `*.sfd` via `DOAX_MovieFilenameTable` | `spWarn.xpr` (`dat/sprite/warning/`) |
| Mechanism | `DOAX_PlayMovie` / SFD playback | Pre-SFD sprite path inside `DOAX_PlayNinjaViHdMovie` |
| Key functions | `DOAX_PlayNinjaViHdMovie`, `DOAX_PlayMovie` | `sub_824D5F08`, `sub_824D6238`, `sub_824D6160`, `sub_8250A500` |

The ninja movie hook at `0x8250AB1C` does not affect the warning scroll because the warning
block runs earlier in the same function.

## Log Evidence (`C:\temp\doax-clean.log`)

- Working warning skip: midasm hook at `0x8250AAA8` jumps to `0x8250AAFC`, bypassing
  `sub_824D5F08(0)` and the wait loop on `sub_824D6238`.
- Final cleaned verification: `DOAX: skipping license warning intro block` logged once,
  `DOAX: skipping ninja_vi_hd.sfd playback` logged once, `indices=256` warning frames dropped
  to 0, and `movie\promotion_video.sfd` opened afterward.
- Scheduler mode during visible warning stayed in `byte_833B8DFC` / sched+4 mode 5, not mode 2.
- Warning scroll window matched `DOAX_PlayNinjaViHdMovie`'s pre-SFD block:
  `sub_824D5F08(0)` -> loop on `sub_824D6238` -> yield through `sub_8250A500`
  (resume PC `0x8250A544`).
- `DOAX_WarningScreenUpdate` fired after ninja skip with `sched_mode=5`, so it was the wrong
  phase for the visible scroll.
- `sub_824C1350(2)` skip never logged during the visible scroll.
- `DOAX_BootWarningFrame` / `sub_8258CD58` / `sub_825A2560` / `sub_825A0FD8` /
  `sub_8250C120` probes did not log during the visible warning loop.
- Opening file reads: `movie\promotion_video.sfd` after ninja; no `opening.sfd` readtrace was
  observed in these logs.
- Input during boot: `XamInputGetState` for user 0 returns `0x00000000`; users 1-3 return
  `0x448F` with `buttons=0`. Movie A-skip uses `sub_824C75F8` (`sub_826E1AF8`), not the
  probed `sub_82782BF0` path.

## Approaches Tried

| # | Approach | Result |
|---|----------|--------|
| 1 | Ninja midasm `0x8250AB1C` -> `0x8250ABA4` | Works - keep this |
| 2 | Auto-skip opening via `DOAX_SkipOpeningMovie` midasm @ `0x826B0CF8` | Failed - breaks A-button skip during opening; removed |
| 3 | Midasm skip license @ `0x8250BD48` (dismiss `sub_824C1350(18, ...)`) | Failed - warning still showed; wrong site |
| 4 | `sub_824C0770` override: skip `sub_824C1350(2)` only | Inconclusive - opening A-skip worked briefly; warning still showed |
| 5 | Draw midasm `0x8250BC3C` -> `0x8250BC48` (`sub_82666EB0`) | Failed alone - never logged; draw path not hit when sched mode != 2 |
| 6 | Fake `byte_833B8DF9` / `byte_833B8DFE` before `sub_8258CD58` in `sub_824C0770` | Failed - black screen after opening; boot state corrupt |
| 7 | Early fast-forward bytes before `sub_8258CD58` + jump over BB60 blocks | Failed - black screen / menu never reached |
| 8 | `DOAX_BootEnterWarningMode` -> call ninja + dismiss (wrong boot order) | Failed - regression |
| 9 | `DOAX_BootWarningDismiss` override (skip draw, arm timer) | Failed - when mode != 5, returned early and blocked stock dismiss; broke opening A-skip |
| 10 | `DOAX_BootWarningDismiss` 1-tick timer, drain-driven `sub_824C08B8` | Failed - warning still showed; opening still broken |
| 11 | `sub_824C1350` entry override skip mode 2 | Failed - never logged; path not taken |
| 12 | `DOAX_WarningScreenUpdate` override (no draw, timer when mode == 2) | Failed - hook runs after ninja, not during scroll; mode never 2 |
| 13 | `sub_824C0770` full partial reimplementation + 1350 skip | Failed - opening A-skip broken; reverted to `__imp__` passthrough |
| 14 | `DOAX_BootEnterWarningMode` partial init, skip `0770(2)`, set boot progress | Failed - warning still showed |
| 15 | `DOAX_BootWarningFrame` (`0x8250AC98`) no-op + fast-forward | Failed - warning still showed; opening still broken |
| 16 | Midasm skip warning intro `0x8250AAA8` -> `0x8250AAFC` | Works - skips `spWarn.xpr` intro block, preserves ninja skip, and proceeds to opening SFD |

## Root Causes

1. Correct hook target: the visible warning is the first block inside `DOAX_PlayNinjaViHdMovie`,
   before `DOAX_PlayMovie(4)`. It starts at `0x8250AAA8` with `sub_824D5F08(0)`, waits on
   `sub_824D6238`, and yields through `sub_8250A500`.
2. Wrong hook targets: mode-2 warning functions (`DOAX_WarningScreenUpdate`,
   `sub_824C1350(2)`, draw at `0x8250BC3C`) are not hit during the visible warning. The
   `DOAX_BootWarningFrame` / `sub_825A*` probe branch also did not fire in the visible loop.
3. Boot state corruption: faking scheduler bytes (`DF9`, `DFE`, `flag9`) or short-circuiting
   `DOAX_BootWarningDismiss` without stock bookkeeping breaks later opening input/skip.
4. Do not auto-skip opening in `DOAX_PlayMovie`; A-skip is in `sub_826E1AF8` via
   `sub_824C75F8` when playback is active.

## Current Implementation

- Keep: warning intro midasm `0x8250AAA8` -> `0x8250AAFC`
  (`DOAX_SkipLicenseWarningIntro`), ninja midasm `0x8250AB1C` -> `0x8250ABA4`, and existing
  fiber/scheduler boot fixes (`sub_824C08B8`, `sub_824C05B8`, etc.).
- Gate: `DOAX_SkipLicenseWarningIntro` is controlled by `kSkipLicenseWarning` in
  `DOAX/src/doax_hooks.cpp`.
- Removed: draw skip `0x8250BC3C`, `sub_824C1350` queue override, boot-state probe, and
  `DOAX_BootWarningFrame` / `sub_825A*` probes.
- Generated: rerun `rexglue codegen doax_manifest.toml` after changing
  `DOAX/doax_manifest.toml`.

## Follow-Up Targets

1. Fix `sub_824C75F8` / user-0 input if opening A-skip still fails on stock paths.
2. Confirm whether `movie\promotion_video.sfd` is the observed opening table entry for this
   build.

## Key Addresses

| Address | Symbol |
|---------|--------|
| `0x8250AA48` | `DOAX_PlayNinjaViHdMovie` |
| `0x8250AAA8` | Start warning intro block (`sub_824D5F08(0)`) |
| `0x8250AAE4` | Warning wait loop yield call to `sub_8250A500` |
| `0x8250AAFC` | Post-warning continuation in `DOAX_PlayNinjaViHdMovie` |
| `0x8250AB1C` | Ninja SFD start argument, current ninja skip hook site |
| `0x8250ABA4` | Post-ninja continuation |
| `0x8250A544` | Resume PC inside `sub_8250A500` |
| `0x824D5F08` | Starts warning sprite path |
| `0x824D6238` | Warning completion poll |
| `0x824D6160` | Warning cleanup / display state reset |
| `0x824C0770` | Scheduler mode transition |
| `0x824C1350` | UI queue (`r3=2` warning, `r3=18` dismiss) |
| `0x8250AC08` | `DOAX_BootEnterWarningMode` |
| `0x8250AC50` | `DOAX_BootWarningDismiss` |
| `0x8250AC98` | `DOAX_BootWarningFrame` |
| `0x8250BB60` | `DOAX_WarningScreenUpdate` |
| `0x8250BC3C` | `sub_82666EB0` draw inside BB60 |
| `0x826E1AF8` | Opening movie tick / A-skip |
| `0x824C75F8` | Input helper used by opening skip |
| `0x833B8DF8` | Scheduler struct (`+4` = mode) |
| `0x8341F9F6` | Boot progress byte |
