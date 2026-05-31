# FM2 Audio Producer-Gate Instrumentation - 2026-05-30

Date: 2026-05-30
Scope: FM2-only diagnostics pass to verify whether producer/work-item paths execute while `A56C` remains active.

## Instrumentation Added

- New per-second line: `FM2_PROD_PERSEC`
  - producer entry counters: `p898f8`, `p89be0`, `p89e88`, `p86988`
  - helper counters: `h87678`, `h637f8`, `h53d718`
  - bailout counters: `bail_missing`, `bail_allocfail`
  - one-shot registration counter: `reg_oneshot`
- New one-shot line: `FM2_REG_ONESHOT`
  - snapshots: `obj`, `src`, `dst9bc`, `dst9c0`, `srcb68`, `srcb6c`
  - callback constant logged: `cb=8220A4E8`
- Existing `FM2_SIGSITE_PERSEC` retained.

## 12s Local Run Result (same day)

Observed in `C:\temp\fm2-clean.log`:

- `FM2_SIGSITE_PERSEC`:
  - `a56c` active (~29-31/sec)
  - `s9968=0`, `s9d10=0`, `s9ffc=0`, `s86a40=0`
- `FM2_PROD_PERSEC`:
  - `p898f8=0`, `p89be0=0`, `p89e88=0`, `p86988=0` (all zero every second)
  - `h637f8` very high (hundreds to hundreds of thousands/sec depending on second)
  - `h53d718` non-zero early, then mostly near zero
  - `h87678=0`
  - `bail_missing=0`, `bail_allocfail=0`
- `FM2_REG_ONESHOT` emitted once.

## Interpretation

- This confirms global gate signaling (`A56C`) is alive, but expected producer/work-item entry paths are not executing at all in this run.
- The hot path appears to be upstream allocation/container helper traffic (`0x823637F8`) rather than producer enqueue/signal paths.
- Next step should target earlier control flow than the signal callsites, focusing on who repeatedly calls the helper path and why producer entries stay cold.

## Caller-Chain Followup (same day)

Additional instrumentation moved `0x82363768` hook to `after_instruction=true` and sampled LR (`r12` after `mfspr r12,LR`).

Observed dominant return addresses feeding the helper chain:
- `0x821D0448` (callsite `0x821D0444`) in `sub_821D03E8`
- `0x8259F3A0` (callsite `0x8259F39C`) in `sub_8259F340`
- `0x825345A8` (callsite `0x825345A4`) in `sub_82534548`
- `0x822097C8` (callsite `0x822097C4`) in `sub_82209038`

Callsite snippets (from disassembly):
- `0x821D0444: bl 0x823637f8`
- `0x8259F39C: bl 0x823637f8`
- `0x825345A4: bl 0x823637f8`
- `0x822097C4: bl 0x823637f8`

Key implication:
- The hot activity is concentrated in generic alloc/helper chains before any FMOD producer/work-item enqueue entrypoints.
- Producer entry counters remain zero, so fixing audio cadence at `A56C` cannot solve starvation by itself.

## Canonical Names Applied

Applied in Ghidra and mirrored into `FM2/fm2_manifest.toml`:
- `0x821D03E8` -> `FM2_AllocPoolPath_821D03E8`
- `0x8259F340` -> `FM2_AllocPoolPath_8259F340_Mul8`
- `0x82534548` -> `FM2_AllocPoolPath_82534548_Mul34`
- `0x82209038` -> `FM2_AllocPoolPath_82209038_D4`
- `0x82363768` -> `FM2_AllocPoolAcquireOrInit`
- `0x82367F60` -> `FM2_AllocPoolTryAcquire`
- `0x82363538` -> `FM2_AllocPoolInit`

Callsite labels applied in Ghidra:
- `0x821D0444` -> `FM2_AllocCallsite_1D0444`
- `0x8259F39C` -> `FM2_AllocCallsite_59F39C`
- `0x825345A4` -> `FM2_AllocCallsite_5345A4`
- `0x822097C4` -> `FM2_AllocCallsite_2097C4`

## Upstream Route Correction (IDA, same date)

Follow-up tracing showed the active branch does not flow into `0x8258A528` in runtime:
- `FM2_A528_PERSEC entry=0`
- `FM2_SLOT136_PERSEC ... last_target=82437310`
- `FM2_7310_R148_PERSEC ... last_target=824344C0`

Meaning:
- Active path is `sub_82209038 -> slot136: 0x82437310 -> slot148: 0x824344C0`.
- This explains why direct producer writer counters (`0x825898F8/0x82589BE0/0x82589E88`) can stay zero while gate signaling remains active.

See: [[docs/FM2-audio-upstream-dispatch-chain-2026-05-30]]
