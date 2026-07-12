# 2026-07-12 — FM2 immediate crash after SetTexture build

## Confirmed (x64dbg on real `fm2.exe`, not runner.exe)

Launch: `fm2.exe --game_data_root …\Forza2extracted --no-fullscreen --audio_backend=xaudio2`  
Video init OK: `swapchain 1280x720 valid=true`

### Crash cascade (each fix exposed the next)

1. **`plume::D3D12SwapChain::resize`** — `textures[i].d3d->Release()` with no null check (WER `fm2+0x2a683d8`). Stack: `D3DDevice_Swap` → `EnsureFrameStarted` → `resize`.
2. **`clearColor`** — `targetFramebuffer == nullptr`. Stack: `D3DDevice_ClearF` → `Clear`. Race: Present `setFramebuffer(nullptr)` vs Clear on job threads.
3. **`setRootDescriptor` / `setDescriptorSet`** — null pipeline layout after Present ends the command list; next `EnsureFrameStarted` reopened the list without rebinding root signature before Flush/Draw.
4. **`checkTopology` / `drawInstanced`** — `activeGraphicsPipeline == nullptr` because `begin()`/`end()` cleared PSO tracking between Flush and draw.

## Fixes applied

- `thirdparty/plume/plume_d3d12.cpp`: null-safe resize/GetBuffer/clear*/setDescriptorSet/setRootDescriptor/draw*; `begin()` resets host-side pipeline state like `end()`
- `FM2/src/render/video.cpp`: `RecordingMutex`; rebind layout+descriptor sets when opening a frame; lock `WaitForGPU`
- `FM2/src/render/render_state.cpp`: mutex on Clear/Flush/Draw; fix 64-bit FB cache key; skip clear without host texture; rebind PSO before UP draws

## VS crash (confirmed): `D3D12Buffer::map` null `d3d`

Stack: `UnlockResource` → `UploadBufferSwapped` → `upload->map()` with `this->d3d == nullptr`.

Cause: Plume `CreateResource` failed (often after `0x887A0005` DEVICE_REMOVED) but still returned a `unique_ptr<D3D12Buffer>` with null `d3d`. Staging upload only null-checked the smart pointer.

Fix: `map`/`unmap` null-safe; `createBuffer`/`createTexture` return `nullptr` on failed CreateResource; `UploadBufferSwapped` bails if staging create/map fails; log `GetDeviceRemovedReason` on device-removed.

## VS crash (2026-07-12): `D3D12CommandList::barriers` Line 1974

Stack: `Clear` → `FlushBarriers` → `barriers` AV reading `0x00000A5A9AA9C410` (`interfaceTexture->resourceStates`).

Cause: `g_barrierMap` keyed by `RenderTexture*` with a **non-null garbage** texture pointer (not a simple nullptr). `AddBarrier` only skipped `texture->texture == nullptr`, so a GuestTexture with a stale/corrupt raw `texture` (or a destroyed host object still in the map) reached plume and crashed on deref. Plume null-guards alone cannot help against a garbage non-null pointer.

Fix:
- Key `g_barrierMap` by `GuestBaseTexture*`; resolve `texture` at flush after `IsLiveHostTexture` (magic + `texture == textureHolder.get()`).
- `Clear` drops RT/DS bindings that fail that check before width/barrier use.
- Plume `barriers`: skip null buffer/texture and null `d3d`.

Rebuild: `FM2/out/build/win-amd64-relwithdebinfo` fm2 OK.

## VS crash (2026-07-12): `D3D12CommandList::setPipeline` Line 2088

Stack: `DrawIndexedVertices` → `FlushRenderState` → `setPipeline` → `D3D12Core!SetPipelineState` AV at `0xCC`.

Cause: same as buffer/texture creates — `CreateGraphicsPipelineState` failed but plume still returned a `D3D12GraphicsPipeline` with `d3d == nullptr`. `GetPipeline` cached it and every draw called `SetPipelineState(nullptr)`.

Fix: check HRESULT on graphics/compute PSO create; `createGraphicsPipeline`/`createComputePipeline` return `nullptr` on failure (so `GetPipeline` marks `g_failedPipelines`); `setPipeline` skips null command-list / null PSO.
