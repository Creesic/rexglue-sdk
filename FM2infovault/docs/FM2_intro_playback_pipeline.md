# FM2 Intro Playback Pipeline and Timing Constants

This note captures the intro movie playback path discovered while debugging FM2 startup and intro bypass behavior.

## High-level playback flow

1. Startup builds splash playlists:
   - `sub_8220EAC8` -> intro sequence (`Turn10`, `MSGS`, `Dolby`, `Forza`)
   - `sub_8220E900` -> loading sequence (`ForzaLogo` + `LoadingAnim.bik`)
2. Builder submits playlist through render-thread vtable `+0x44` -> `sub_822892C0`.
3. `sub_822892C0` copies the playlist blob via `sub_82288460` and sets pending flag `+0x720`.
4. `sub_822884C8` processes pending entries and calls `sub_8229E198` per item.
5. For each entry, `sub_822884C8` passes two doubles (`lfd f1,-24(r31)` and `lfd f2,-16(r31)`), which are the per-file timing values.

## Known intro media timing values

These are interpreted as timing parameters per clip (effectively start/end or start/timeout style values):

- `Turn10_Logo.bik`: `0.0 -> 3.0`
- `MSGS_Logo.bik`: `0.0 -> 10.0`
- `Dolby_Corona_Intro.bik`: `0.0 -> 5.0`
- `Forza_Intro.bik`: `0.0 -> 79.0`
- `LoadingAnim.bik`: `0.0 -> -1.0` (sentinel-style value)

## Primary generated code anchors

- `FM2/generated/fm2_recomp.2.cpp`
  - `DEFINE_REX_FUNC(sub_8220E900)` (loading playlist builder)
  - `DEFINE_REX_FUNC(sub_8220EAC8)` (intro playlist builder)
  - Double constant stores for timing fields in those builders
- `FM2/generated/fm2_recomp.5.cpp`
  - `DEFINE_REX_FUNC(sub_822892C0)` (playlist submit)
  - `DEFINE_REX_FUNC(sub_82288460)` (playlist blob copy)
  - `DEFINE_REX_FUNC(sub_822884C8)` (queue processing/play dispatch)
  - Per-entry timing read immediately before `sub_8229E198` call:
    - `lfd f2,-16(r31)`
    - `lfd f1,-24(r31)`

## Working intro-skip fix (2026-05-27) - detailed forensic notes

Result: startup intros are skipped reliably. Current observed behavior is a short black transition, then normal startup/menu flow.

### Problem statement

We needed to skip startup movies without breaking startup state transitions. Earlier bypass attempts either:

- still played intro movies, or
- caused black-screen / loading stalls.

The core requirement became: **keep native startup state-machine success paths intact**, but make intro playlist timing complete immediately.

### How the root cause was isolated

1. We mapped startup gating in generated code (`FM2/generated/fm2_recomp.2.cpp`):
   - `0x82214F30` loop calls `sub_8220AE30`.
   - The loop exit checks `clrlwi. r11,r3,24` and branches back on zero.
   - Hook site `0x82214F38` is inside this wait loop.
2. We verified this with log instrumentation from `FM2SkipStartupIntroWait`:
   - Example from `C:\temp\fm2-clean.3.log` (`2026-05-27 16:28:04`):
     - `INTRO_SKIP wait-loop diagnostic call=1 ... r3=00000000`
     - `INTRO_SKIP wait-loop diagnostic call=2 ... r3=00000001`
   - This showed the loop did naturally progress; forcing this gate was not the right long-term skip mechanism.
3. We mapped playlist playback dispatch in generated code (`FM2/generated/fm2_recomp.5.cpp`):
   - `sub_822884C8` is the playlist queue processor.
   - Immediately before `sub_8229E198`, code loads:
     - `lfd f2,-16(r31)`
     - `lfd f1,-24(r31)`
   - These are per-entry timing parameters.
4. We already knew clip timing values from builder analysis:
   - `Forza_Intro.bik`: `0.0 -> 79.0`
   - `LoadingAnim.bik`: `0.0 -> -1.0` (sentinel-style / special semantics)
5. Conclusion:
   - Best leverage point is timing clamp at dispatch callsite.
   - Do not bypass builders, submit path, or wait-loop side effects.

### Exact patch that shipped

#### 1) Diagnostic-only wait-loop hook (no behavior forcing)

File: `FM2/src/fm2_hooks.cpp`  
Function: `FM2SkipStartupIntroWait`  
Hook address: `0x82214F38`

Behavior:

- keeps counters/logging for startup diagnostics;
- always returns `false`, so no forced branch/jump behavior.

Reason:

- this gate is part of startup success transitions;
- forcing progression here can skip side effects and produce black-screen stalls.

#### 2) New timing fast-forward hook at dispatch

File: `FM2/src/fm2_hooks.cpp`  
Function: `FM2FastForwardSplashTiming`  
Hook address: `0x822888F0`

Manifest entry in `FM2/fm2_manifest.toml`:

```toml
[[entrypoint.midasm_hook]]
address = 0x822888F0
name = "FM2FastForwardSplashTiming"
registers = ["f1", "f2", "r7", "r31"]
after_instruction = false
jump_address_on_true = 0x822888F4
```

Algorithm implemented in hook:

- Read incoming `f1`, `f2` doubles.
- `kFastForwardDurationSec = 0.001`.
- If either input is non-finite: do nothing.
- If `f2 < 0.0`: do nothing (preserve sentinel negative timing).
- If `f2 <= f1 + 0.001`: do nothing (already short).
- Otherwise set `f2 = f1 + 0.001`.
- Return `false` so native call flow continues.

This preserves queue ownership, callback ordering, and state-machine transitions, while making positive-duration intro clips finish almost immediately.

### Generated-code proof the hook is active

File: `FM2/generated/fm2_recomp.5.cpp` around `0x822888F0` now contains:

- `if (FM2FastForwardSplashTiming(ctx.f1, ctx.f2, ctx.r7, ctx.r31)) { goto loc_822888F4; }`
- followed by normal `sub_8229E198(ctx, base);`

So the hook mutates timing arguments in place, but still executes the original dispatch function.

### Why the patch initially appeared not to work

We hit a build-tree trap:

- Manifest + hook source edits were correct.
- But codegen was not regenerated in the active FM2 build tree.
- Building from repository root can miss FM2-local codegen refresh.

Concrete clue:

- Logs showed `INTRO_SKIP wait-loop ...` (old hook active),
- but no `INTRO_SKIP timing ...` lines (new hook missing from binary).

### Required build sequence (must run in FM2 subproject)

Working directory:

- `C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2`

Commands:

```powershell
cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Sanity check:

- If `fm2_codegen` says unknown target, you are in the wrong build tree.

### Runtime verification evidence

After correct regenerate+build, `C:\temp\fm2-clean.log` contains timing-hook lines:

- `INTRO_SKIP timing ...`
- `INTRO_SKIP timing summary ... clamped=5 sentinel_negative=1 ...`

Example observed (`2026-05-27 16:33:03`):

- `f1_in=0.000000 f2_in=-1.000000` for sentinel entries (not clamped)
- summary confirmed both clamped positive entries and preserved negative sentinel entries.

### Why this patch is stable

This approach avoids brittle startup forcing and instead respects native flow:

- builders still create playlist entries;
- queue submit/consume still runs;
- startup wait loop still exits via native success;
- only playback timing window for positive intro durations is collapsed.

That is why we get both behaviors together:

1. intros effectively skipped;
2. startup continues correctly instead of black-screen stalling.
