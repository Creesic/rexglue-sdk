# 2026-07-12 — FM2 closer to Unleashed (StretchRect / RT GPU work)

## Done this session

1. **Deferred StretchRect/Resolve** (Unleashed pattern): `GuestSurface::destinationTextures`, `GuestTexture::sourceSurface`, pending sets, `FlushPendingStretchRectCommands()` before Present + FlushRenderState. MSAA uses `resolveTexture`; 1x uses `resolveTextureRegion`.
2. **CreateSurface** honors `multiSample` (2/4/8 → sampleCount).
3. **GPU creates on render thread**: `CreateTexture`, `CreateSurface`, `TranslateGuestTexture` create path via `RenderQueue::Run`.
4. **SetTexture / SetTextureBase** + descriptor binds on render thread; sourceSurface/MSAA pending like Unleashed `ProcSetTexture`.
5. **ExecuteUpload** runs on render thread (copy queue no longer raced from guest alone).

## Also landed (continue pass 2)

- IntermediaryUploadAllocator + async DrawUserPointerVertices via Enqueue
- ScheduleResourceDestruction + DestructTempResources on OnRecordingFrameReady
  (GPU objects freed only after frame fence; magic cleared immediately)

## Still next

- ~~Hook guest D3D Release → ScheduleResourceDestruction~~ (done: `sub_82369E08`/`sub_82369D90`)
- Full typed `RenderCommand` POD enum + `Proc*`
- Drop RecordingMutex once Enqueue covers all GPU mutations
- Shader MSAA resolve pipelines if hardware resolve insufficient
- After next `fm2_codegen`: rename hooks from `sub_*` to `D3DResource_AddRef`/`Release`
