# FM2 Audio Cadence Chain (IDA + Ghidra) - 2026-05-30

Date: 2026-05-30  
Scope: map the active upstream chain that feeds FM2 audio submit cadence and explain why ReX still behaves near ~30/s on the producer side.

## Confirmed Call Chain

1. `FM2_AudioRenderFrame_PathA` (`0x8227FF20`) and `FM2_AudioRenderFrame_PathB` (`0x82287400`) both call:
   - `FM2_AudioFrameService_Update` (`0x8227E580`)
2. `FM2_AudioFrameService_Update` calls:
   - `FM2_AudioSubmitBridge_Guard` (`0x825ADE70`)
3. `FM2_AudioSubmitBridge_Guard` (when arg2 != 0) calls:
   - `FM2_AudioSubmitBridge_Process` (`0x825ADE20`)
4. `FM2_AudioSubmitBridge_Process` calls:
   - `FM2_GpuKick_DefaultSubmit` (`0x8236CB20`) and
   - `FM2_GpuCommandBuffer_BuildAndSubmit` (`0x8236CB28`)
5. `FM2_GpuKick_DefaultSubmit` routes into:
   - `FM2_GpuKick_QueueWithScheduler` (`0x8236C688`)
6. `FM2_GpuKick_QueueWithScheduler` installs/uses scheduler callback:
   - `0x8236C4F0` (currently named `FM2_AudioInterrupt_ProcessSignalQueue` in IDA)

## High-Value Behavioral Findings

- `0x82381D60` (`FM2_AudioPumpThread_WaitAndDispatch`) has an explicit timeout:
  - `LARGE_INTEGER = -300000` (~30ms).
  - On timeout (`KeWait... == 258`), it executes submit work (`0x8236CB20` + `0x8236CB28`) in a loop.
- This matches observed ReX cadence shape:
  - producer/signal side repeatedly clustering near `~32-33/s` in logs.
- `0x8236C4F0` uses an internal frequency divisor:
  - `VdGlobalDevice + 0x52B8` (default fallback `60`).
  - Computes a percent-like pacing term and chooses immediate vs queued signaling paths.
  - This is not a simple "always emit more" branch; it is thresholded and stateful.

## Cross-Tool Consistency

- IDA and Ghidra decomp agree on:
  - `0x8227E580 -> 0x825ADE70` callsite (`0x8227E60C`),
  - timeout service behavior in `0x82381D60`,
  - scheduler callback registration path in `0x8236C688`.

## Practical Implication

- Forcing downstream submit mode bits (`mode2/mode3`) or callback-dispatch caps does not address the root cadence source.
- Root cadence control is upstream in the frame-service + wait/scheduler interaction, especially:
  - `0x8227E580` invocation rate,
  - `0x82381D60` timeout/event balance,
  - `0x8236C4F0` thresholded progression logic.

## Next Untried Instrumentation Targets

1. Count per-second callsite hits to `0x8227E580` from:
   - `0x82280098` (PathA)
   - `0x822879CC` (PathB)
2. Count branch outcomes in `0x8227E580` around:
   - `*(a1+2520)` guard before `0x8227E60C`,
   - success/fail of `0x825ADE70` return path.
3. Count signaled-vs-timeout work loops in `0x82381D60`:
   - how often work is done from timeout loop vs normal event wake path.

If one of these shows a hard ~30/s limiter, patch there first (FM2-only, env-gated).
