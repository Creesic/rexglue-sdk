# FM2 Experiment Board

Purpose: track FM2 audio experiments in one place, especially failed attempts, to avoid repeating blind patches.

Last updated: 2026-05-30

Related notes:
- [[docs/audio_failures]]
- [[docs/audio_failures#Speculative Fixes (Open Brainstorm + Peer Review)]]
- [[docs/FM2-audio-fmod-decode-cadence]]
- [[docs/FM2-audio-decode-throughput]]

## Quick Status
- Current path: FM2 forced to XAudio2 backend (`Fm2App::OnPreSetup`).
- Current result: audio confirmed clean/perfect by runtime listening validation.
- Note: keep FM2 on XAudio2 as the known-good baseline while FMOD-path research remains optional.

## Failed Experiments Log

Use one row per meaningful patch attempt.

| Date | Exp ID | Change Tried | Outcome | Evidence | Rollback/Next |
|---|---|---|---|---|---|
| 2026-05-30 | FE-014 | XAudio2 backend integration + FM2-specific backend force (`Fm2App::OnPreSetup`) + backend activation markers | XAudio2 path confirmed active; service cadence jumped to near target (~93-95/s read/export) | `C:\temp\fm2-clean.log` (`REX_XMA_EXPORT_PERSEC enable~94-95/s`, `FM2_FMOD_READ_PERSEC calls~93-95/s`); live module check shows `XAudio2_9.dll` loaded | Remove temporary activation marker logs; keep XAudio2 as baseline |
| 2026-05-30 | FE-015 | Post-activation listening validation (menu/gameplay) on XAudio2 path | Audio confirmed perfect | User listening report + stable 12s runs with no regression symptoms | Keep FM2 forced to XAudio2 for now |
| 2026-05-27 to 2026-05-30 | FE-001 | FM2 hook cadence/signal forcing variants (`FMOD_DELAY`, producer gate forcing, queue/signal adjustments) | No durable fix; speed sometimes improved, quality remained crunchy; frequent regressions to slow/silent states | `C:\temp\fm2-clean.log` (`decodes ~42-49/s`, `signals ~32-33/s`, `underruns=3/sec`), user runtime reports | Keep out of permanent path unless backed by new evidence |
| 2026-05-28 to 2026-05-30 | FE-002 | Large FM2 hook diagnostic additions (high-volume counters/force logic) | High complexity, little or no audible improvement | Repeated A/B reports from runtime + log checks | Remove/trim noisy diagnostics; keep only decision-grade counters |
| 2026-05-29 to 2026-05-30 | FE-003 | Native FMOD replacement attempts in FM2-only glue path | Reached silent/noise states; not production-ready | Runtime behavior: silent menu/gameplay, intermittent noise | Defer until decode path parity + buffer contract are verified |
| 2026-05-30 | FE-004 | 12s E1-E4 bounded experiments (`REX_FM2_AUDIO_EXP=1..4`) with new diagnostics | No audible improvement; E1 worsened underruns, E2 minimal effect, E3 no meaningful parity correction, E4 severe underrun burst (confounded by menu transition input) | `C:\temp\fm2-exp0.log` .. `C:\temp\fm2-exp4.log` + `FM2_*_PERSEC` lines | Keep instrumentation; do not reuse E1-E4 as-is |
| 2026-05-30 | FE-005 | IDA-guided upstream-route probe: instrumented `0x824344C0` (entry/loop-end/name-compare/match) + slot chain counters | Short-run remained upstream of that chain; new counters stayed zero while `821D03E8` path dominated | `C:\temp\fm2-clean.log` (`FM2_344C0_PERSEC=0`, `FM2_7310_PERSEC=0`, `FM2_ALLOC_BRANCH_PERSEC` dominated by `821d_*`) | Move probes earlier than slot-136 route in short-run flow |
| 2026-05-30 | FE-006 | Safe scheduler-fork probes at `0x823EB998` / `0x823EB9D0` | No audible change, but produced decisive topology data | `FM2_FMOD_UP_PERSEC`: `b998~1681-1692/s`, `cb/disp~187-188/s`; `FM2_SIGSITE_PERSEC a56c~29-30/s`; `FMOD_PERSEC signals~31-33/s`, `decodes~44-47/s`, `underruns=3/sec` | Stop tuning upstream callback cadence; move to downstream gate-branch state tracing around `A4E8/A56C` |
| 2026-05-30 | FE-007 | Added `0x8236C380` callback-owner entry probe (`FM2_FMOD_IRQ_PERSEC`) + cross-thread gate/setevent counters | No behavior change intended; isolated a two-stage cadence reduction | `FM2_FMOD_UP_PERSEC cb/disp~187-188/s`; `FM2_FMOD_IRQ_PERSEC enter~60/s`; `FM2_FMOD_THREAD_PERSEC all(gate)~60/s`; `all(setevent)~29-31/s`; `FMOD_PERSEC signals~32-33/s` | Next: A/B bounded relaxation of `A4E8` divider first, then evaluate whether `8236C380` pacing needs adjustment |
| 2026-05-30 | FE-008 | Env-gated `A4E8` divider bypass test (`REX_FM2_A4E8_FORCE_SET=1`) by preloading `DAT_829C24C8` at gate entry | Strong cadence shift without crashes; behavior changed as expected | With env=off: `signals~32-34/s`, `all(setevent)~30/s`, `decodes~40-47/s`; with env=on: `signals~64-66/s`, `all(setevent)~60/s`, `decodes` broader `~28-87/s`; both runs reached menu in 12s self-run | Keep toggle for controlled A/B; decide by audible quality + stability before making default |
| 2026-05-30 | FE-009 | Added scheduler-arg probe at `0x8236C4F0` + env-gated arg rewrite (`REX_FM2_SCHED_MODE2=1`, `0x114 -> 0x214`) | Arg rewrite took effect (`mode=2`), but signal cadence did not materially change | `FM2_FMOD_SCHED_PERSEC`: baseline arg `0x114`, mode `1`, thr `20`, hits ~29-31/s; rewritten arg `0x214`, mode `2`, hits ~30/s; `FMOD_PERSEC signals` remained ~32-33/s | Do not keep mode rewrite as fix; move upstream to source that limits `0x8236C4F0` invocation rate (~30/s) |
| 2026-05-30 | FE-010 | Upstream wrapper provenance probes (`0x8236C8C8`, `0x8236C948`, `0x8236CB20`) + submit feed counters | Identified active submit wrapper chain and static parameters | `FM2_FMOD_WRAP_PERSEC`: `c8c8~30/s`, `cb20~30/s`, `c948~0`; `FM2_FMOD_SUBMIT_PERSEC`: `a2=0` always, stable source, `last_lr=825ADE8C`; `FM2_FMOD_SCHED_PERSEC`: arg remains `0x114` | Root limiter is upstream wrapper cadence (~30Hz path), not dynamic `a2` |
| 2026-05-30 | FE-011 | Env-gated submit mode injection (`REX_FM2_SUBMIT_MODE3=1`, force `a2=0x200` => arg `0x314`) | Mode rewrite applied but did not materially lift signal/decode cadence | `FM2_FMOD_SCHED_PERSEC` showed `mode=3` correctly; `FMOD_PERSEC` remained roughly `signals~32-34/s`, `decodes~39-48/s` | Discard as non-fix; next focus is increasing wrapper-call cadence, not mode bits |
| 2026-05-30 | FE-012 | Added FM2-only pump wait probes (`0x82381DE4` / `0x82381DFC`) and default 8ms wait clamp (`REX_FM2_PUMP_WAIT_MS`, default ON) | Material runtime improvement in logs; underruns dropped sharply and decode throughput increased | Baseline/off run: `underruns~75/s`, `decodes~46/s`; with 8ms clamp: sustained `underruns~2/s`, `decodes~55-71/s`, `signals~32-33/s`; `FM2_FMOD_PUMP_PERSEC` confirms active override (`ovr>0`, `cfg(wait=8ms,on=1)`) | Keep as current FM2 default; next validate audible quality and fine-tune wait window (6-10ms) if needed |
| 2026-05-30 | FE-013 | Env-gated dispatch cap attempt (`REX_FM2_THROTTLE_DISPATCH30=1`) at `0x823EBF20` using `jump_address_on_true` skip path | Verified dispatch clamp (`cb~31`, `disp~30`) but no audible quality/speed improvement; introduced instability/crash in some runs (`0xC0000005`) | `FM2_FMOD_UP_PERSEC` confirmed cap when enabled; crash repro logged with `state=EXITED code=-1073741819`; starvation/underrun behavior persisted | Removed callback-skip throttle path and manifest jump override; if revisited, use a safer non-control-flow-breaking method |

## Latest 12s Sweep Snapshot (May 30, 2026)

| Mode | Decode avg/s | Signal avg/s | Underrun avg/s | Key diagnostic note |
|---|---:|---:|---:|---|
| `EXP=0` baseline | 42.7 | 32.4 | 1.0 | Baseline ceiling unchanged |
| `EXP=1` wait clamp | 42.6 | 32.4 | 2.0 | Clamp active, no decode gain |
| `EXP=2` enqueue wake | 42.8 | 32.4 | 1.0 | Forced wakes fired (`e2_forced`), no audible gain |
| `EXP=3` reg parity | 43.9 | 32.5 | 2.0 | Target/source params already matched |
| `EXP=4` backpressure | 47.7 | 32.3 | 77.0 | Confounded by menu interaction + unstable underrun spike |

Durable finding:
- `FM2_Q_PERSEC signals` stayed `0` across runs while `FM2_CB_PERSEC setevent_calls` and `FMOD_PERSEC signals` were nonzero.
- Current queue-signal hook coverage is incomplete for the active runtime path.

## Open Thoughts / Try Later

Use this as a parking lot for ideas before they become implementation plans.

| Idea ID | Date Added | Hypothesis | Proposed Probe | Success Signal | Priority | Status |
|---|---|---|---|---|---|---|
| TL-001 | 2026-05-30 | ReX producer scheduling path under-services stream wake cadence vs Xenia | Instrument and compare event/wait cadence and per-handle wake efficiency against Xenia diagnostics | ReX signal/service rates approach Xenia-run baselines | High | queued |
| TL-002 | 2026-05-30 | A second upstream gate throttles callback invocation count beyond visible FMOD cadence hooks | Trace upstream branch conditions around known gate addresses and queue writers (IDA/Ghidra mapped) | Identified branch/counter that caps service rate and can be corrected safely | High | queued |
| TL-003 | 2026-05-30 | Queue pointer/owner transition stall (`q168/q172`) causes long-running starvation phases | Add transition-state logging + stall duration buckets for queue owner/indices | Stall intervals correlate directly with underrun bursts | High | queued |
| TL-004 | 2026-05-30 | Event-handle semantics/lifecycle mismatch causes missed wakeups in ReX path | Audit SetEvent/Wait patterns and handle ownership against Xenia behavior | Missed-wakeup pattern reproduced and eliminated | Medium | queued |
| TL-005 | 2026-05-30 | Native replacement can work only after a validated data contract (format/rate/block shape) per stream | Add one-time stream contract dump + parity check before route switch | No silent/noise regressions in first 30s at menu | Medium | queued |
| TL-006 | 2026-05-30 | Active producer signal path is not fully captured by current hooks (`q_signals` stuck at 0) | Expand FM2-only SetEvent instrumentation to all active stream-thread callsites and correlate by handle | `FM2_Q_PERSEC signals` tracks real signal cadence and is no longer pinned at 0 | High | queued |

## Entry Templates

### Failed Experiment Template

```md
| YYYY-MM-DD | FE-XXX | <change tried> | <outcome> | <log fields / captures / notes> | <rollback condition or next step> |
```

### Try-Later Template

```md
| TL-XXX | YYYY-MM-DD | <hypothesis> | <probe/patch target> | <measurable success signal> | <priority> | queued |
```

## Workflow Rules
- Do not promote a try-later idea to code changes without:
  - measurable acceptance criteria,
  - explicit rollback condition,
  - source evidence link.
- If a patch regresses to black screen, silence, or major instability, log it here immediately with exact build/date.
- Keep this board concise; move durable conclusions into `docs/audio_failures.md`.
