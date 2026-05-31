# FM2 Audio Failures Log

Date range: May 29, 2026 - May 30, 2026  
Scope: FM2 audio debugging attempts that were explicitly documented as failed, regressed, or not sufficient.

## May 29, 2026

| Attempted change | Documented outcome | Final status |
|---|---|---|
| Keep `output_buffer_valid=1` across empty output ring | Caused unstable behavior: one-second burst (`decodes=6037`) then sustained collapse (`decodes=0`). | Reverted |
| Force queue signal dispatch hook at `0x8258A634` | Did not prevent deterministic decode collapse around `t=33`. | Not sufficient |
| Additional forced event signaling from keepalive loop | Did not prevent collapse; decode still dropped to zero. | Not sufficient |
| Native copy-byte cadence clamp (`copy_bytes=target_bytes`) | Did not fix collapse behavior. | Not sufficient |
| FM2-only bounded producer-gate force patch (gate hooks + force limits) | Improved early underruns in Phase A, but still collapsed (`decodes=0` around `t=33-36`). | Partial improvement only |
| Disable delay hook (`REX_FM2_DELAY_PATCH=0`) | Worse than delay-enabled run; underruns stayed higher and collapse persisted. | Rejected (keep delay hook enabled) |
| Increase read-heartbeat grace `250ms -> 2000ms` | Worsened underruns (up to `4/sec`) and collapse still occurred. | Reverted |
| Increase gate-force aggressiveness (`min interval 8->4ms`, cap `16->64/sec`) | Signal cadence increased to ~`61-64/s`, but decode cadence stayed ~`42-52/s` (still far below target). | Not sufficient |
| Extra stream-work-loop hook at `0x82588BC4` (`FM2ForceExtraWorkLoop88BC4`) | Hook never triggered on active path (`stream_boost=0`), no effect. | Removed |
| May-27 selective gate replay (`REX_FM2_MAY27_GATE`) | Did not materially change signal rate in replay matrix. | Not sufficient |
| May-27 preserve-valid replay (`REX_FM2_MAY27_PRESERVE_VALID`) | Collapsed decode service to zero (silent path) in tested combinations. | Rejected |

## May 30, 2026

| Investigated area | Documented outcome | Final status |
|---|---|---|
| Re-verify producer/write cadence path in Ghidra (`sub_825898F8`, wrappers, dispatcher, wait path) | Confirmed speed is controlled by producer signal cadence plus stream wait fallback (`Nt_WaitForSingleObject(..., 32ms, 1)`), not a single tunable scalar. | Diagnostic finding (no patch change) |
| FMOD read-size override trial (`REX_FM2_READ4096=1` at `0x82693954`, 12s A/B) | Request bytes doubled, but copy path flipped (`c2 -> c1` partial copy), decode/signal cadence stayed near baseline (`~45/~33 s`), and underruns worsened (`~2/sec -> ~4/sec`). | Rejected |
| Scheduler mode flip trial (`REX_FM2_SCHED_MODE2=1`, 12s A/B) | Scheduler hit rate remained ~`30/s`, queueing bias increased, and decode cadence fell (`~44.5/s -> ~42.5/s`). | Rejected |
| Dispatch cap trial (`REX_FM2_THROTTLE_DISPATCH30=1` at `0x823EBF20` with callback-skip jump) | Cap engaged (`cb~31`, `disp~30`) but audio did not improve and stability regressed (reproducible access violation `0xC0000005` / `-1073741819` in some runs). | Rejected and removed |

## May 30, 2026 (12-second E1-E4 Sweep)

| Attempted change | Documented outcome | Final status |
|---|---|---|
| `EXP=1` wait fallback micro-shim (`32ms -> 8ms` bounded clamp) | Clamp activated (`clamped_sum=625`, `clamp_activations=5`) and reduced wait sample medians, but decode rate stayed flat (`~42.6/s`) and underruns worsened (`~2/s`). | Not sufficient |
| `EXP=2` enqueue transition wake guarantee (bounded extra `SetEvent`) | Forced wakes fired (`q_e2_sum=15`) with minimal decode impact (`~42.8/s`) and no audible improvement. | Not sufficient |
| `EXP=3` registration parity shim | Registration already matched (`target +0x9BC/+0x9C0 == source +0xB68/+0xB6C`), so no meaningful parameter correction to apply; only minor decode variance (`~43.9/s`). | Not sufficient |
| `EXP=4` backpressure relief guard | Run had user interaction confound (menu transition), but also showed severe underrun spike (`underruns` burst to `77` in sample) and no audible improvement. | Rejected for now |
| New diagnostics families (`FM2_CB_PERSEC`, `FM2_WAIT_PERSEC`, `FM2_Q_PERSEC`, `FM2_REG_ONESHOT`) | Emitted successfully and captured callback/wait/queue cadence. | Successful instrumentation |

### Why These Did Not Produce Audible Improvement

- Decode throughput ceiling remained near prior baseline (`~42-44/s`) despite wait and wake interventions.
- `FM2_Q_PERSEC signals=0` across runs, while callback/setevent counters were nonzero (`FM2_CB_PERSEC setevent_calls ~28-30/s`) and `FMOD_PERSEC signals ~32/s`.
- This indicates the current producer-signal accounting hook site is not covering the active signal path, so E2/E4 logic was likely acting on incomplete visibility.
- Next cycle should fix signal-path coverage first (instrument true active SetEvent path for this stream) before retrying bounded interventions.

## Speculative Fixes (Open Brainstorm + Peer Review)

Process note:
- This is an open brainstorm queue (no strict signoff gate).
- Any item promoted to execution must define measurable acceptance criteria.
- Any item promoted to execution must define an explicit rollback condition.
- Any item promoted to execution must link to concrete source evidence.

Template (copy for new entries):
- `Spec ID`:
- `Date added`:
- `Hypothesis`:
- `Proposed change`:
- `Why this might work`:
- `Expected measurable outcome`:
- `Risk/regression notes`:
- `Current disposition`: `idea | reviewed | defer | reject | promote-to-plan`
- `Peer review`:
- `Reviewer`:
- `Date`:
- `Assessment`:
- `Evidence cited`:
- `Recommendation`: `try now | needs instrumentation | reject`

### SF-001
- `Spec ID`: `SF-001`
- `Date added`: `May 30, 2026`
- `Hypothesis`: Producer-side scheduling stops enqueueing work around `t~33` even while stream object/event remain valid.
- `Proposed change`: Instrument and gate patch the producer state transition around queue owner/state path before `sub_825898F8` dispatch (stream scheduler path), not the decode callback.
- `Why this might work`: Existing logs show decode collapse with live stream object and event handle, but queue pointers and producer counters plateau.
- `Expected measurable outcome`: `qpush/qwrite` no longer flatline at collapse point; `FMOD_PERSEC decodes` no longer reaches sustained `0` after `t~33`.
- `Risk/regression notes`: Over-forcing producer path may create runaway queue growth or stutter.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `docs/FM2-audio-fmod-decode-cadence.md` (May 29 deep-dive: queue plateau, collapse timeline).
- `Recommendation`: `needs instrumentation`

### SF-002
- `Spec ID`: `SF-002`
- `Date added`: `May 30, 2026`
- `Hypothesis`: A second cadence gate exists upstream of callback invocation count, separate from `FM2_SignalGate` divider behavior.
- `Proposed change`: Locate and patch upstream cadence decision point that limits callback invocation frequency even when signal cadence rises.
- `Why this might work`: Signal cadence was raised to ~`61-64/s` but decode cadence remained ~`42-52/s`.
- `Expected measurable outcome`: `decode/sig` ratio increases toward parity; decode cadence tracks signal cadence more closely.
- `Risk/regression notes`: Removing upstream gating without bounds could increase CPU load or destabilize timing.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `docs/FM2-audio-fmod-decode-cadence.md` (May 29 gate cap follow-up).
- `Recommendation`: `try now`

### SF-003
- `Spec ID`: `SF-003`
- `Date added`: `May 30, 2026`
- `Hypothesis`: Event semantics/handle lifecycle mismatch causes wake path degradation over time (signal delivered but callback service not sustained).
- `Proposed change`: Add strict handle-lifecycle and event-state diagnostics around stream event (`stream+0x4C`/`+76`) and `Nt_SetEvent`/wait return coupling.
- `Why this might work`: Ghidra-confirmed path uses queued signal plus 32ms wait fallback; long-run collapse may involve event object behavior drift.
- `Expected measurable outcome`: Stable correlation between signal count and wake/callback service across full run; no late-run divergence.
- `Risk/regression notes`: Diagnostics overhead can perturb timing if too verbose.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `docs/FM2-audio-fmod-decode-cadence.md` (May 30 triple-check of `sub_825898F8` and wait path).
- `Recommendation`: `needs instrumentation`

### SF-004
- `Spec ID`: `SF-004`
- `Date added`: `May 30, 2026`
- `Hypothesis`: Queue pointer stall transition (`q168/q172` empty) is tied to owner-state conditions not currently surfaced in telemetry.
- `Proposed change`: Instrument owner/state fields and transitions adjacent to queue reader/writer logic to capture exact branch causing enqueue stop.
- `Why this might work`: Existing telemetry shows queue empty and producer counters frozen while stream object remains alive.
- `Expected measurable outcome`: One explicit state transition marker coincident with `qpush/qwrite` halt and decode collapse.
- `Risk/regression notes`: Might require multiple probes; risk is mostly diagnostic complexity.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `docs/FM2-audio-fmod-decode-cadence.md` (May 29 deep-dive durable observations).
- `Recommendation`: `needs instrumentation`

### SF-005
- `Spec ID`: `SF-005`
- `Date added`: `May 30, 2026`
- `Hypothesis`: Stream wait fallback (`32ms`) plus intermittent missed producer signals leads to cumulative under-service, then deterministic starvation.
- `Proposed change`: Trial a bounded FM2-only wait-policy shim for stream thread wake timing to reduce fallback dependency without globally changing kernel wait behavior.
- `Why this might work`: Ghidra path confirms explicit `32ms` wait; if producer misses cadence windows, service can fall behind even with occasional signals.
- `Expected measurable outcome`: Lower underrun rate and sustained non-zero decodes past prior collapse window; no busy-loop spikes.
- `Risk/regression notes`: Too aggressive wake policy can cause busy-looping and CPU spikes; must include strict cap/rollback trigger.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `docs/FM2-audio-fmod-decode-cadence.md` (stream loop/wait notes and May 30 triple-check section).
- `Recommendation`: `defer`

### SF-006
- `Spec ID`: `SF-006`
- `Date added`: `May 30, 2026`
- `Hypothesis`: Current producer signal hook (`0x82589968`) is not the dominant active path in this run, so queue-signal telemetry and E2/E4 triggers are under-observing reality.
- `Proposed change`: Expand signal-path instrumentation to all active stream-thread `NtSetEvent` callsites in the `82589xxx`/`8240C4F8` path and correlate by handle to isolate FM2 stream signal truth.
- `Why this might work`: New diagnostics showed `FM2_Q_PERSEC signals=0` while `FM2_CB_PERSEC setevent_calls` and `FMOD_PERSEC signals` were nonzero.
- `Expected measurable outcome`: `FM2_Q_PERSEC signals` tracks observed callback/signal cadence and no longer remains pinned at zero.
- `Risk/regression notes`: Primarily diagnostic overhead; low behavior risk if instrumentation-only.
- `Current disposition`: `idea`
- `Peer review`:
- `Reviewer`: `TBD`
- `Date`: `TBD`
- `Assessment`: `TBD`
- `Evidence cited`: `C:\\temp\\fm2-exp0.log` .. `C:\\temp\\fm2-exp4.log` (12s sweep, May 30, 2026).
- `Recommendation`: `try now`

## Notes

- Source of truth for these entries is the dated sections in:  
  `docs/FM2-audio-fmod-decode-cadence.md`
- This file is a failure/outcome index so repeated experiments can be avoided.
