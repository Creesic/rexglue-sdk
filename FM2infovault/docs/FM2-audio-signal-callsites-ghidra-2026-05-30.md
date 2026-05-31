# FM2 Audio Signal Callsites (Ghidra) - 2026-05-30

Date: 2026-05-30
Scope: classify the five `NtSetEvent` audio-related callsites and map them to containing functions/roles.

## Canonical Function Naming (Applied In Ghidra)

- `0x8220A4E8` -> `FM2_SignalGate_GlobalEventMaybeSet`
- `0x825898F8` -> `FM2_QueuePayloadAndSignalStreamEvent`
- `0x82589E88` -> `FM2_QueueBranchThenSignalStreamEvent`
- `0x82586988` -> `FM2_StreamDispatchAndSignalStateTransition`

Additional symbol labels:
- `0x82589BE0` -> `FM2_WorkItemCreateAndAttach_Region_89BE0`
- `0x82586A28` -> `FM2_StreamSetStateAndSignalThunk_Entry_6A28`

## Callsite Differences

### `0x8220A56C` (A56C)
- Inside `FM2_SignalGate_GlobalEventMaybeSet`.
- Reached only if gate branch at `0x8220A564` passes.
- Uses global handle load path (`lis r11,-0x7d64` / `lwz r3,0x24c0(r11)`).
- Behavior class: global cadence gate signal.

### `0x82589968` (9968)
- Inside `FM2_QueuePayloadAndSignalStreamEvent`.
- Comes after queue insertion path (`bl 0x825891A8`) with lock/unlock around queue work.
- Uses per-stream handle (`lwz r3,0x4c(r31)`).
- Behavior class: producer enqueue-complete signal.

### `0x82589D10` (9D10)
- Inside work-item create/attach region `FM2_WorkItemCreateAndAttach_Region_89BE0`.
- In a work-item create/attach flow before vcall cleanup/release sequence.
- Uses per-stream handle (`lwz r3,0x4c(r30)`).
- Behavior class: stream work-item lifecycle signal.

### `0x82589FFC` (9FFC)
- Inside `FM2_QueueBranchThenSignalStreamEvent`.
- Located on conditional branch-success path (`0x82589FB0`/`0x82589FDC` split).
- Uses per-stream handle (`lwz r3,0x4c(r27)`).
- Behavior class: conditional queue/control-path signal.

### `0x82586A40` (6A40)
- Inside state-transition path `FM2_StreamDispatchAndSignalStateTransition` at labeled entry `FM2_StreamSetStateAndSignalThunk_Entry_6A28`.
- Preceded by byte state flips (`stb 0 -> +0x44`, `stb 1 -> +0x50`), then tail branch to event set (`b 0x8240C4F8`).
- Uses per-stream/object handle (`lwz r3,0x4c(r11)`).
- Behavior class: state-transition wake signal thunk.

## Practical Implication

If telemetry shows only `A56C` firing while `9968/9D10/9FFC/6A40` stay idle, FM2 is likely stuck in global gate signaling without normal producer/work-item stream progression.

## Callsite Labels Added

- `0x8220A56C` -> `FM2_SignalGate_GlobalSetEvent_Callsite_A56C`
- `0x82589968` -> `FM2_ProducerQueueSetEvent_Callsite_9968`
- `0x82589D10` -> `FM2_WorkItemSetEvent_Callsite_9D10`
- `0x82589FFC` -> `FM2_ConditionalQueueSetEvent_Callsite_9FFC`
- `0x82586A40` -> `FM2_StateTransitionSetEvent_Callsite_6A40`
