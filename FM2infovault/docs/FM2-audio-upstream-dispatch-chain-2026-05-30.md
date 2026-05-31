# FM2 Audio Upstream Dispatch Chain (IDA-confirmed) - 2026-05-30

Date: 2026-05-30  
Scope: identify why producer enqueue writers (`0x825898F8 / 0x82589BE0 / 0x82589E88`) are not reached on the active FM2 runtime path.

## Confirmed Runtime + RE Facts

1. `0x8220A56C` is active (`FM2_SignalGate` path), but producer writers still stay cold.
2. `sub_8258A528` (mode dispatcher that can route to producer wrappers) is not executed in active runs:
   - `FM2_A528_PERSEC entry=0`
3. Active slot-136 target selected by `sub_82209038` is `0x82437310`, not `0x8258A528`:
   - `FM2_SLOT136_PERSEC ... last_target=82437310`
4. Inside `sub_82437310`, slot-148 target resolves to `0x824344C0`:
   - `FM2_7310_R148_PERSEC ... last_target=824344C0`

## IDA Decomp Highlights

## `sub_82437310` (active slot-136 target)
- Early-return gate returns `0` when any are true:
  - `*a3 == 0`
  - `!sub_82437D48(a2)`
  - virtual call `(*a3 vtbl + 132)(*a3)` is non-zero
- If gates pass, it builds request state, then calls virtual `a1 + 148`.
- Important branch:
  - if `(slot148_call != 1)`, it performs update/queue-side work and can return success (`v11=1`).
  - our runtime traces showed `ne1` dominating (`eq1=0` in sampled active second), but producer writer counters still stayed zero.

## `sub_824344C0` (active slot-148 target in traces)
- Performs lock/refcount semantics on `a1[2]`.
- Iterates an entry array (`a1[4]..a1[5]`, stride 9 dwords).
- Match condition requires:
  - `i[1] == a2[1]`
  - case-insensitive name equality via `stricmp` on entry name fields.
- Returns `1` on match, else `0`.

Practical meaning:
- This path is a keyed lookup/selection gate, not a direct producer enqueue function.
- If selection fails or routes elsewhere, the known producer writers can remain untouched even while signaling appears active.

## Why Previous Gate-Force Experiments Missed

- Prior experiments assumed the bottleneck was near `0x8220A564/0x8220A56C`.
- New evidence shows the hot path is upstream and virtual-dispatched through `0x82437310 -> 0x824344C0`.
- Forcing around `A56C` cannot fix missing producer routing if the wrong object/mode/selection path is active.

## Next Target (Untried, Evidence-Driven)

Instrument `0x824344C0` decision outcomes:
- per-second counters:
  - loop iterations
  - `i[1] == a2[1]` hits/misses
  - `stricmp` match/mismatch counts
  - return `1` vs `0`
- one-shot snapshots:
  - selected `a2[1]` key and current candidate `i[1]` values
  - name pointers/short string samples (bounded, safe logging)

Acceptance signal:
- identify whether producer starvation is caused by systematic key/name mismatch (selection never resolving to the expected stream work item) versus a later queue stage.

## Evidence Source

- Runtime log: `C:\temp\fm2-clean.log`
- Relevant lines:
  - `FM2_A528_PERSEC`
  - `FM2_SLOT136_PERSEC`
  - `FM2_7310_PERSEC`
  - `FM2_7310_R148_PERSEC`

## Follow-up Probe (same day, 12s self-run)

Added new counters/hook points:
- `FM2_344C0_PERSEC` with hooks at:
  - `0x824344C0` (entry)
  - `0x82434578` (loop end)
  - `0x824345F0` (name compare result)
  - `0x8243460C` (match path)

Observed:
- `FM2_344C0_PERSEC entry=0 loop_end=0 name_eq=0 name_ne=0 match=0`
- `FM2_7310_PERSEC` and `FM2_SLOT136_PERSEC` also stayed at zero in this short run.
- Active traffic still concentrated in:
  - `FM2_ALLOC_BRANCH_PERSEC`: `821d_nz/ge1` only
  - `FM2_63768_LR_PERSEC`: dominated by `lr821d0448`

Interpretation update:
- In this run window, execution never reached the `0x82209038 -> 0x82437310 -> 0x824344C0` chain at all.
- Upstream limiter appears even earlier (currently `0x821D03E8`-dominated path), so further probes must move above slot-136 dispatch for short-run diagnosis.

## Alloc/String Upstream Chain (extended, same day)

New caller mapping from runtime LR samples:
- `0x821D03E8` (`FM2_AllocPoolPath`) caller is exclusively `0x821D0E74`
- `0x821D0E10` (`FM2_AllocGrowAndCopy`) caller is exclusively `0x821D15B0`
- `0x821D1568` (`FM2_AllocEnsureCapacity`) heavy callers:
  - `0x821D266C` (`sub_821D25C0` copy-assign)
  - `0x821D2550` (`sub_821D24D8` substr-assign)
  - `0x82430CF0` (`sub_82430C10` replace/fill helper)
- String-core callers sampled:
  - `0x821D25C0` called from `0x821D2878` (`sub_821D2820`, C-string assign)
  - `0x821D24D8` called from `0x821D2704` (`sub_821D26C8`, string assign)
  - `0x82430C10` called from `0x824318F0` (path slash replacement loop)
  - transient caller observed: `0x825CF430` inside `sub_825CF298`

Observed rates:
- early burst windows: `0x821D03E8` up to ~13k-21k calls/sec
- same windows show `FMOD_PERSEC decodes=0..15` and underruns present
- steady windows still show persistent string-core activity (roughly `f25c0~1.6k/s`, `f24d8~450/s`)

Thread note:
- sampled string-core hooks ran on host TID `102080` in latest run.
- corresponding `FMOD_PERSEC` lines in that run used host thread `t137064`.
- this suggests the string-core storm is likely not executing on the FMOD worker thread directly (may still cause global CPU contention).

## FMOD Thread-Scoped Probe (same day, later pass)

Added FM2-only hooks/counters for:
- `0x825890F8` worker-loop entry (bind candidate)
- `0x8220A4E8` signal gate entry
- `0x8220A56C` setevent path
- `0x8258916C` wait callsite (expected `NtWaitForSingleObject(...,32,1)`)
- `0x82588C10` work tick entry
- `0x826938E8` codec read entry

New log line:
- `FM2_FMOD_THREAD_PERSEC ... hooks(...) stage_tids(...) ...`

Observed across multiple short self-runs:
- `0x8220A4E8` and `0x8220A56C` are active immediately.
- `0x826938E8` also active immediately, but on a different host thread than gate/setevent.
- Sample:
  - `stage_tids(gate=86244 setevent=86244 decode=77684)`
  - and `FMOD_PERSEC: timer thread started` was logged on `t77684`.
- `0x825890F8` does trigger, but later in startup (after first `FMOD_PERSEC t=1` line in these short runs), binding to yet another thread id.
- `0x8258916C` and `0x82588C10` remained cold (`wait=0`, `work=0`) in the captured startup windows.

Interpretation update:
- The active startup path is split across at least two host threads:
  - gate/setevent thread
  - decode/timer thread
- The previously assumed single-thread FMOD worker model is incomplete for this stage.
- The expected wait/work callsites (`0x8258916C` / `0x82588C10`) are either:
  - not yet entered in this window, or
  - not the dominant active loop for the path currently producing the audible starvation.

Next upstream action from this finding:
- Trace call origins around `0x8220A4E8` and `0x826938E8` by caller LR buckets, then align with the first thread handoff to `0x825890F8`.
- Goal: identify the exact handoff condition where callback-rate appears low (`signals ~33/s`) while decode path is active.

## Scheduler Fork Probe (same day, later pass: `0x823EB998` / `0x823EB9D0`)

Added safe function-entry counters:
- `0x823EB998` (`FM2FmodSchedEB998`)
- `0x823EB9D0` (`FM2FmodSchedEB9D0`)

12s self-run result (`C:\temp\fm2-clean.log`):
- `FM2_FMOD_UP_PERSEC` stabilized around:
  - `cb=187..188/s`
  - `disp=186..188/s`
  - `b998=1681..1692/s`
  - `b9d0=187..188/s`
  - `dthunk/fill=41..43/s`
- At the same time:
  - `FM2_SIGSITE_PERSEC a56c=29..30/s`
  - `FMOD_PERSEC signals=31..33/s`
  - `FMOD_PERSEC decodes=44..47/s`
  - `underruns=3/sec`

Interpretation:
- Upstream scheduler callback activity is high and stable (`823EBF20 -> 823E83C8 -> 823EB998`).
- The cadence collapse occurs downstream, where the high-rate scheduler path is reduced to ~33/s signal gate cadence (`0x8220A56C`) and ~45/s decode cadence.
- This narrows the next probe to branch outcomes/guards on the `A4E8/A56C` path and its immediate caller state, not the upstream render scheduler itself.

## Callback Owner Probe (`0x8236C380`) (same day, latest pass)

New FM2-only safe entry hook added:
- `0x8236C380` (`FM2FmodIrqDispatch8236C380`)

Per-second output:
- `FM2_FMOD_IRQ_PERSEC sec=... enter=... eq=... cur=... tgt=... pend=...`

12s self-run observations:
- `FM2_FMOD_UP_PERSEC cb/disp` remained ~`187-188/s`
- `FM2_FMOD_IRQ_PERSEC enter` stabilized near ~`60/s`
- `FM2_FMOD_THREAD_PERSEC all(gate)` stabilized near ~`60/s`
- `FM2_FMOD_THREAD_PERSEC all(setevent)` stayed near ~`29-31/s`
- `FMOD_PERSEC signals` stayed near ~`32-33/s`

Interpretation update:
- The major drop from scheduler activity to gate calls is now pinned at/above `0x8236C380`:
  - `~188/s` (`823E83C8` path) -> `~60/s` (`8236C380` entry / `A4E8` gate calls)
- `0x8220A4E8` then applies an additional divider-like reduction:
  - `~60/s` gate calls -> `~30/s` `NtSetEvent` calls (`A56C`)

Practical conclusion:
- This is a two-stage cadence reduction, not a single gate issue.
- Next fix should target one stage at a time with bounded A/B:
  1. `A4E8` divider relaxation (raise setevent from ~30 toward ~60), then
  2. if needed, adjust `8236C380`-owned pacing fields/logic that cap dispatch near ~60/s.

## Bounded A/B: `A4E8` Divider Relaxation (env-gated)

Implemented test-only toggle in FM2 hooks:
- `REX_FM2_A4E8_FORCE_SET=1`
- behavior: at `FM2FmodGateEntry8220A4E8`, preload `DAT_829C24C8=2` before gate logic, making setevent eligibility effectively every gate call.

12s self-run comparison:
- Env OFF:
  - `FMOD_PERSEC signals ~32-34/s`
  - `FM2_FMOD_THREAD_PERSEC all(setevent) ~30/s`
  - `FMOD_PERSEC decodes ~40-47/s`
- Env ON:
  - `FMOD_PERSEC signals ~64-66/s`
  - `FM2_FMOD_THREAD_PERSEC all(setevent) ~60/s`
  - `FMOD_PERSEC decodes` became wider (`~28-87/s` in the sampled window)

Result:
- The divider stage is now proven causal for the `~60 -> ~30` reduction.
- Quality outcome still needs audible validation, but this is the first controlled patch with a large, deterministic signal-cadence change and no startup crash.
