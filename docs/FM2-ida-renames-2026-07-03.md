# FM2 IDA renames — 2026-07-03 (played-command-buffer transport session)

Evidence-based renames from the press-A texture investigation
(`docs/FM2-handoff-2026-07-02-session5.md` parts 7–10). IDB saved.
Both names burned into `FM2/fm2_manifest.toml` (take effect on next codegen).

## Renames

| Address | Old | New | Evidence |
|---|---|---|---|
| 0x82381FD0 | sub_82381FD0 | `D3D_FlushRingSegmentAndKickScheduler` | Decompile: GetPendingCommandCount → D3D_AllocateCommandBuffer → FM2_GpuKick_AppendSchedulerPm4Packets → CRingAllocList::Finalize ×3 (+13000/+12960/+12980) → D3D_SubmitCommandBuffer(pending) + D3D_SubmitCommandBuffer(scheduler segment). Sole caller 0x827328E8 (resolve-blit family). |
| 0x8245D510 | sub_8245D510 | `FM2_DeferredRenderCmdPool_Construct` | Decompile: constructor of the 0x4001CA20 deferred render-cmd pool — intrusive lists +28/+40, critsecs +64/+92, counters +120 (retire) / +124 (enqueue) / +128 (backlog) / +132 (threshold, init −1) / +140 (credits), flags +144 (exec-now) / +145 (backpressure drop latch). Field map matches every live FM2_POOLSTATE observation. |

## Proof comments added (already-named functions)

- `D3D_SubmitCommandBuffer` (0x82372920): r5 = segment guest address
  (alias-windowed), r6 = size in dwords; every played command buffer passes
  through here; segments contain nested INDIRECT_BUFFER calls. Hooked in
  plume (`ScanSubmittedSegment`).
- `FM2_D3D_SubmitCommandBufferChain` (0x823724C8): node = {sizeDwords, addr};
  writes PM4 INDIRECT_BUFFER `0xC0013F00` packets into the primary ring;
  strips 0x40000000/0x41000000 alias windows; MMIO kick at 0x7FC80714.
- `FM2_DeferredRenderCmdPool_Construct` (0x8245D510): full field map (above).
- `D3D_FlushRingSegmentAndKickScheduler` (0x82381FD0): flush semantics (above).

## Deliberately NOT renamed (insufficient evidence)

- `sub_825D7560` (0x1014 bytes): only known fact = calls the resolve blitter;
  too big to characterize without a dedicated pass.
- `sub_82374658`, `sub_82372198`: touched in xref walks only.
- CParams execute thunks (0x82279F20, 0x8227A0D0, 0x8227A200, 0x821D4648, …):
  all are 5-instruction virtual-dispatch trampolines
  (`this->vtbl[2](node->payload)`); naming each requires resolving the owning
  CParams class via its vtable — a dedicated pass (would also finally identify
  the play-command-buffer op the drop latch kills).

## Stale-name debt spotted (not fixed)

The `FM2_AudioMix*` / `FM2_AudioRender*` / `FM2_AudioPumpThread_DispatchPm4Commands`
family (0x825ADB18, 0x827328E8, 0x82369A50, 0x823818D8) is confirmed-misnamed
render/present machinery (the session-5 "audio-mix misattribution"). Renaming
needs a dedicated evidence pass over each.

Cross-reference: `docs/FM2-ida-toml-function-notes.md`.

## Session 6 (deferred-pool pacing root cause) — 20 renames

Evidence: full decompile of the deferred render-cmd pool family + the frame
pacing chain (see docs/FM2-handoff-2026-07-02-session5.md session-6 section).

| Address | Old | New |
|---|---|---|
| 0x8245D5C0 | audio_thread_link_shutdown | FM2_DeferredRenderCmdPool_SubmitOpenBuffer |
| 0x8245D448 | sub_8245D448 | FM2_DeferredRenderCmdPool_PopPendingBuffer |
| 0x8245D048 | sub_8245D048 | FM2_DeferredRenderCmdPool_ExecuteBufferNodes |
| 0x8245D740 | sub_8245D740 | FM2_DeferredRenderCmdPool_RetireExecutedBuffer |
| 0x8245CD30 | sub_8245CD30 | FM2_DeferredRenderCmdPool_GetSubmitSeq (returns +120) |
| 0x8245CD38 | sub_8245CD38 | FM2_DeferredRenderCmdPool_HasPendingWork (+60 or +128) |
| 0x8245CD60 | sub_8245CD60 | FM2_DeferredRenderCmdPool_OpenNewBuffer (stamps node+8 = seq) |
| 0x8245CD58 | FM2_AudioManager_SetFieldAt132 | FM2_DeferredRenderCmdPool_SetDropThreshold (init call passes 3) |
| 0x8245D188 | sub_8245D188 | FM2_DeferredRenderCmdPool_PopCreditMarker (-1 to +140) |
| 0x825817D0 | audio_thread_process_deferred_queue_step | FM2_DeferredCmdPoolThread_ProcessQueueStep (shared render/audio wrapper) |
| 0x8220A5E0 | sub_8220A5E0 | FM2_MainLoop_OpenRenderCmdBuffer |
| 0x8220A628 | sub_8220A628 | FM2_MainLoop_SubmitRenderCmdBuffer |
| 0x82277BF0 | sub_82277BF0 | FM2_GraphicsManager_OpenDeferredPoolBuffer_Thunk (this+0x8B0) |
| 0x82277BF8 | sub_82277BF8 | FM2_GraphicsManager_SubmitDeferredPoolBuffer_Thunk (this+0x8B0) |
| 0x82277C00 | sub_82277C00 | FM2_GraphicsManager_PushDeferredPoolCredit_Thunk (+1 to pool+140) |
| 0x82277C08 | sub_82277C08 | FM2_GraphicsManager_PopDeferredPoolCredit_Thunk |
| 0x822172E8 | sub_822172E8 | FM2_MainThread_GameLoop (waits gate event 829C24C0 when params+1056==1) |
| 0x82371FD8 | sub_82371FD8 | FM2_D3D_GraphicsInterruptCallback (src 0 = vblank, 1 = CP) |
| 0x8236D0C0 | FM2_AudioInterrupt_SetSignalCallback | D3DDevice_SetVerticalBlankCallback (device+0x3F28) |
| 0x8220A4E8 | FM2_SignalGate | FM2_VBlank_SignalGameLoopGate (PulseEvent 829C24C0) |
