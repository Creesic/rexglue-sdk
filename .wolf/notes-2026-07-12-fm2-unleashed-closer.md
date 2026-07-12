# 2026-07-12 — FM2 closer to Unleashed (StretchRect / RT GPU work)

## Done this session

1. **Deferred StretchRect/Resolve** (Unleashed pattern): `GuestSurface::destinationTextures`, `GuestTexture::sourceSurface`, pending sets, `FlushPendingStretchRectCommands()` before Present + FlushRenderState. MSAA uses `resolveTexture`; 1x uses `resolveTextureRegion`.
2. **CreateSurface** honors `multiSample` (2/4/8 → sampleCount).
3. **GPU creates on render thread**: `CreateTexture`, `CreateSurface`, `TranslateGuestTexture` create path via `RenderQueue::Run`.
4. **SetTexture / SetTextureBase** + descriptor binds on render thread; sourceSurface/MSAA pending like Unleashed `ProcSetTexture`.
5. **ExecuteUpload** runs on render thread (copy queue no longer raced from guest alone).

## Still next

- Full typed `RenderCommand` POD enum + `Proc*` (vs `std::function` Enqueue)
- Intermediary upload allocator for async DrawUP / constant patches
- DestructResource on render thread
- Shader MSAA resolve pipelines if hardware resolve insufficient
- Drop RecordingMutex once Enqueue covers all GPU mutations

## Also landed (continue pass)

- Per-frame `g_uploadAllocators[kNumFrames]`; reset in `OnRecordingFrameReady` after fence
- `RenderQueue::Enqueue` fire-and-forget for SetTexture/RT/DS/viewport/scissor
- Present source snapshot moved into `PresentImpl` (FIFO after Enqueue'd SetRT)
- Removed `ClaimPresentOwner` / `fm2_plume_single_thread_present`; keep `g_presentBusy`
