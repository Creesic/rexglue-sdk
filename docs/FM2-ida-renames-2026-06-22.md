# FM2 IDA Renames - 2026-06-22

Standalone summary of UnleashedRecomp hook mapping:
`docs/FM2-unleashed-video-hook-mapping-summary.md`.

Context: FM2 native-renderer hook-surface pass for replacing the ReX/Xenos
render path with a ReOdyssey/Unleashed-style Plume renderer.

## Manual Renames

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82522748` | `FM2_Render_PrepareObjectPassTlsState` | Called before object-pass packet walking. It writes current pass/draw pointers into render TLS slots, selects global pass-state pointers, sets the pass sampler binding table, and calls `FM2_RenderTls_BindPassStateToContextInner`. |
| `0x82530FC8` | `FM2_Render_DispatchObjectPassDrawOpcode` | Object-pass opcode dispatcher. It switches on packet opcode byte, forwards normal packets to `FM2_Render_DispatchPm4DrawOpcode`, handles special opcodes `1`, `5`, and `45`, and updates draw-range bookkeeping after dispatch. |
| `0x82531210` | `FM2_Render_WalkObjectPassDrawPacketStream` | Applies the render-state callback block, then repeatedly prefetches and dispatches the object-pass packet stream through `FM2_Render_DispatchObjectPassDrawOpcode` until the dispatcher returns false. |
| `0x82531310` | `FM2_Render_PrepareAndWalkObjectPassDrawPackets` | Thin object-pass wrapper. It prepares TLS pass state via `FM2_Render_PrepareObjectPassTlsState`, invokes the global render callback if set, prefetches the pass packet stream, then calls `FM2_Render_WalkObjectPassDrawPacketStream`. |
| `0x825080E0` | `FM2_Render_BuildFallbackPassCommandBuffers` | Builds fallback/variant pass command buffers when cached buffers are missing. It switches active command-buffer context, sets render state, emits pass draw work for three variants, finalizes batches, and stores cloned command buffers. |
| `0x82508E38` | `FM2_Render_EmitFallbackPassDrawListIfReady` | Readiness wrapper around the fallback command-buffer path. It checks pass-buffer availability, calls `FM2_Render_BuildFallbackPassCommandBuffers`, performs cleanup, then submits the selected cloned draw list through `FM2_D3D_EmitDirtyStateAndDrawList`. |

## Hook-Surface Findings

The first native renderer path should not hook the low-level PM4 emitters as the
primary product boundary. IDA shows a better semantic layer:

- Direct indexed path:
  - `0x825380B8` / `FM2_Render_BuildDirectIndexedDrawBuffers`
  - Calls `FM2_Render_EnsureDirectDrawRecordResources`.
  - Reads direct draw records at `direct_render_ctx+0x5A4/+0x5A8`.
  - Binds stream/resource slots through draw interface slot `+0x64`.
  - Binds index/resource through draw interface slot `+0x74`.
  - Issues draw through draw interface slot `+0x80` with primitive `4`,
    start index from segment `+0x04`, and primitive count from
    segment `+0x06 / 3`.
- Render-context state:
  - `0x8236DD10` / `FM2_RenderContext_SetPixelShaderState`
  - `0x8236E010` / `FM2_RenderContext_SetVertexShaderState`
  - `0x82370E48` / `FM2_RenderContext_BindVertexStream`
  - `0x82370F68` / `FM2_RenderContext_BindIndexBuffer`
  - `0x82371A30` / `FM2_RenderContext_SetBoundSurface`
- Cached draw-list path:
  - `0x82723750` / `FM2_Render_ExecuteBoundDrawPass` binds pass state and
    then walks the packet stream.
  - `0x82722FD8` / `FM2_Render_WalkAndDispatchPm4DrawList` is already a
    title-specific opcode walker, so it is lower-level than the first native
    renderer cut should prefer.
  - `0x827221F0` / `FM2_Render_DrawIndexedPrimitive` binds the index buffer and
    emits indexed PM4 draw packets, useful after cached draw-list replay is
    needed.
- Present/resolve:
  - `0x824F6520` / `FM2_D3D_LazyInitPresentChain` is the present-chain lazy
    initializer.
  - `0x824F83D8` / `FM2_D3D_TryPresentAndUpdateStatus` invokes the present-chain
    vfunc and updates status fields.
  - `0x82382590` / `FM2_D3D_EmitSurfaceResolvePackets` is low-level packet
    emission and should be treated as a later resolve-translation boundary, not
    the first frame-present hook.

## UnleashedRecomp `video.cpp` Hook Cross-Reference

Worked backwards from `UnleashedRecomp/UnleashedRecomp/gpu/video.cpp`
`GUEST_FUNCTION_HOOK` table. Sonic Unleashed functions were renamed in IDA as
`SWA_Video_*`; FM2 analogues were named/commented in the FM2 `default.xex` IDA
database.

Direct byte signatures do not match across titles (same D3D runtime family, different
game wrappers), so mapping is semantic/decompile-based.

| UnleashedRecomp hook | SWA IDA name | SWA func start | FM2 analogue | FM2 address |
| --- | --- | --- | --- | --- |
| `CreateDevice` | `SWA_Video_CreateDevice` | `0x82BD9948` | `FM2_D3D_InitGlobalDeviceSingleton` | `0x824A52C0` |
| `Present` | `SWA_Video_Present` | `0x82BDA6D0` | `FM2_D3D_TryPresentAndUpdateStatus` | `0x824F83D8` |
| `GetBackBuffer` | `SWA_Video_GetBackBuffer` | `0x82BDD2C8` | `FM2_RenderContext_GetBoundSurfaceAndAddRef` | `0x82371040` |
| `SetRenderTarget` / `SetDepthStencilSurface` | `SWA_Video_SetRenderTargetOrDepth` | `0x82BDD930` | `FM2_RenderContext_SetBoundSurface` | `0x82371A30` |
| `SetViewport` | `SWA_Video_SetViewport` | `0x82BDD440` | `FM2_RenderContext_SetViewportModeAndApply` | `0x823715B0` |
| `SetScissorRect` | `SWA_Video_SetScissorRect` | `0x82BDCF68` | `FM2_D3D_EmitScissorRegionPackets` | `0x8236E780` |
| `SetStreamSource` | `SWA_Video_SetStreamSource` | `0x82BDD058` | `FM2_RenderContext_BindVertexStream` | `0x82370E48` |
| `SetIndices` | `SWA_Video_SetIndices` | `0x82BDD218` | `FM2_RenderContext_BindIndexBuffer` | `0x82370F68` |
| `SetVertexShader` / `SetVertexDeclaration` | `SWA_Video_SetVertexDeclOrShader` | `0x82BE0050` | `FM2_RenderContext_SetVertexShaderState` | `0x8236E010` |
| `SetPixelShader` | `SWA_Video_SetPixelShader` | `0x82BDFE10` | `FM2_RenderContext_SetPixelShaderState` | `0x8236DD10` |
| `CreateTexture` | `SWA_Video_D3DResourceDispatch` / `CreateTexture` | `0x82BE9498` | `FM2_D3D_CreateTextureWrapper` / `FM2_D3D_CreateTextureResourceFromFormat` | `0x825A2650` / `0x823852F8` |
| `LockTextureRect` | `SWA_Video_D3DResourceDispatch` / `LockTextureRect` | `0x82BE9300` | `FM2_D3D_GatherSurfaceMetadataForTextureCreate` (`D3DSurface_LockRect`); asset/XML path: `FM2_D3D_DeserializeAndUploadTextureSurfaces` | `0x82392090` / `0x825A2CC0` |
| `UnlockTextureRect` | `SWA_Video_UnlockTextureRect` | `0x82BE7780` | `FM2_D3DResource_UnlockForRelease` | `0x8236A2B8` |
| `SetTexture` | `SWA_Video_SetTexture` | `0x82BE9818` | `FM2_RenderContext_SetTextureFetchBitsLow` / `Mid` | `0x8236EA60` / `0x8236EA90` |
| `GetSurfaceDesc` | `SWA_Video_GetSurfaceDesc` | `0x82BE96F0` | `FM2_D3D_CacheSurfaceDescFields` / `FM2_D3D_CacheSurfaceDescFieldsWithSubrect` | `0x825B2EF8` / `0x825B2F80` |
| `CreateVertexBuffer` | `SWA_Video_CreateOrLockVertexBuffer` | `0x82BE6AD0` | `FM2_D3D_CreateVertexBufferWrapper` | `0x825A22F0` |
| `LockVertexBuffer` | `SWA_Video_CreateOrLockVertexBuffer` | `0x82BE6B98` | `FM2_D3D_LockVertexBufferWrapper` → `FM2_D3D_LockGpuBufferRaw` | `0x825A2350` / `0x8236A0B0` |
| `UnlockVertexBuffer` | `SWA_Video_UnlockVertexBuffer` | `0x82BE6BE8` | `FM2_D3D_UnlockVertexBufferWrapper` → `FM2_D3D_UnlockGpuBufferRaw` | `0x825A2368` / `0x8236A0F8` |
| `GetVertexBufferDesc` | `SWA_Video_GetBufferDesc` | `0x82BE61D0` | No separate export: read `+4/+8` from VB holder after `CreateVertexBufferWrapper` | `0x825A22F0` fields |
| `CreateIndexBuffer` | `SWA_Video_CreateIndexBuffer` | `0x82BE6BF8` | `FM2_D3D_CreateIndexBufferWrapper` | `0x825A2730` |
| `LockIndexBuffer` | `SWA_Video_LockIndexBuffer` | `0x82BE6CA8` | `FM2_D3D_DeserializeAndLockIndexBuffer` → `FM2_D3D_LockGpuBufferRaw` | `0x825A27D8` / `0x8236A0B0` |
| `UnlockIndexBuffer` | `SWA_Video_UnlockIndexBuffer` | `0x82BE6CF0` | `FM2_D3D_UnlockGpuBufferRaw` | `0x8236A0F8` |
| `GetIndexBufferDesc` | `SWA_Video_GetIndexBufferDesc` | `0x82BE6200` | No separate export: read `+4/+8` from IB holder after `CreateIndexBufferWrapper` | `0x825A2730` fields |
| `CreateSurface` | `SWA_Video_CreateSurface` | `0x82BE95B8` | `FM2_D3D_CreateDepthStencilSurfaceAndTexture` | `0x82515E18` |
| `CreateVertexDeclaration` | `SWA_Video_CreateVertexDeclaration` | `0x82BE0428` | `FM2_D3D_CreateVertexDeclarationFromElements` / `FM2_Render_CreateGpuBuffers` | `0x8259F008` / `0x82721970` |
| `HashVertexDeclaration` | `SWA_Video_HashVertexDeclaration` | `0x82BE0530` | Identity via D3D decl pointer stored at holder `+76` by `FM2_D3D_CreateVertexDeclarationFromElements` (Unleashed host also returns pointer) | `0x8259F008` |
| `GetVertexDeclaration` | `SWA_Video_GetVertexDeclaration` | `0x82BE04B0` | Bundled in `FM2_RenderContext_SetVertexShaderState` / pass compile | `0x8236E010` |
| `CreateVertexShader` | `SWA_Video_CreateVertexShader` | `0x82BE1A80` | `FM2_Render_GetOrCreateVertexShaderResourceById` | `0x825A16E0` |
| `CreatePixelShader` | `SWA_Video_CreatePixelShader` | `0x82BE1990` | `FM2_Render_GetOrCreatePixelShaderResourceById` | `0x825A1608` |
| `DrawIndexedPrimitive` | `SWA_Video_DrawIndexedPrimitive` | `0x82BE5CF0` | `FM2_Render_DrawIndexedPrimitive` | `0x827221F0` |
| `DrawPrimitive` | `SWA_Video_DrawPrimitive` | `0x82BE5900` | `FM2_Render_DrawIndexedPrimitive` (main) + `FM2_Render_EmitDrawRangeCountPm4` (PM4 type 1407 vertex-range draw); blit helper: `FM2_Render_EmitIndexedTriangleFanDrawPm4` | `0x827221F0` / `0x823764B0` / `0x8272F650` |
| `DrawPrimitiveUP` | `SWA_Video_DrawPrimitiveUP` | `0x82BE52F8` | `FM2_D3D_EmitIndexedDrawPacket` (memcpy vertex bytes from GPU ring into CB, then draw/sampler emit) | `0x82383718` |
| `StretchRect` | `SWA_Video_StretchRect` | `0x82BF6400` | `FM2_Render_BlitTiledRegionTriangleFanPm4` | `0x8272FBA0` |
| `Clear` | `SWA_Video_Clear` | `0x82BFE4C8` | Color/flags setters + `D3D::SetTileAndDepthClear`; PM4 via `FM2_D3D_CountLeadingDirtyBits` in `FM2_D3D_EmitDirtyStateAndDrawList` | `0x8236EF20` / `0x8236EF88` / `0x8237CBD8` / `0x82382928` |
| `D3DXFillTexture` | `SWA_Video_D3DXFillTexture` | `0x82C003B8` | `FM2_RenderResource_FillTextureSurfaceLayoutByFormat` | `0x8272AEB8` |
| `D3DXFillVolumeTexture` | `SWA_Video_D3DXFillVolumeTexture` | `0x82C00910` | Same fill helper via volume mip setup (`FM2_RenderResource_SetupXgTextureHeaders` dispatch) | `0x8272AEB8` / `0x8272AB90` |
| `DestructResource` | `SWA_Video_DestructResource` | `0x82BE6210` | `FM2_D3D_ReleaseGpuResourceRef` | `0x8237ED10` |
| present backbuffer setup | — | — | `FM2_D3D_CreatePresentBackbufferResources` | `0x82374190` |
| FM2-only direct draw hook | — | — | `FM2_Render_BuildDirectIndexedDrawBuffers` | `0x825380B8` |
| `MakePictureData` | — | `0x82E43FC8` | `FM2_D3D_CreateTextureFromMemoryBuffer` → `FM2_D3D_CreateTextureFromSurfaceLevel` → `FM2_Image_ParseDDSFromMemory` | `0x825A25F8` / `0x82387B08` / `0x8238CEF0` |
| `SetResolution` | — | `0x82E9EE38` | Runtime scaler: `FM2_GpuKick_ComputeScalerViewportRects` + `FM2_GpuKick_SubmitVdScalerCommandBuffer`; init: `FM2_D3D_CreatePresentBackbufferResources` | `0x82378D58` / `0x8237A888` / `0x82374190` |
| `ScreenShaderInit` | — | `0x82AE2BF8` | `FM2_MovieRenderer_InitScreenShaderResources` + `FM2_MovieRenderer_InitMovieShaderResources` (Bink movie VS/PS/decl globals) | `0x827BA780` / `0x827BC5E0` |

### New FM2 manual renames from this pass

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82374190` | `FM2_D3D_CreatePresentBackbufferResources` | Queries `VdQueryVideoMode`, calls `D3DDevice_CreateTexture` / `CreateSurface`, binds via `FM2_RenderContext_SetBoundSurface`. Present-chain backbuffer/resource setup analogous to Unleashed `CreateDevice` backbuffer move. |
| `0x824A52C0` | `FM2_D3D_InitGlobalDeviceSingleton` | Initializes global D3D/device manager singleton (`dword_829F2DF4`), worker threads, critsecs. Closest FM2 match to Unleashed `CreateDevice` singleton setup. |
| `0x82502268` | `FM2_D3D_LazyInitPresentChainInit` | Present-chain subscriber/init helper paired with `FM2_D3D_LazyInitPresentChain`. |
| `0x825A22F0` | `FM2_D3D_CreateVertexBufferWrapper` | Thin wrapper around `D3DDevice_CreateVertexBuffer`. |
| `0x825A2730` | `FM2_D3D_CreateIndexBufferWrapper` | Thin wrapper around `D3DDevice_CreateIndexBuffer` with index-format sizing. |
| `0x827BAAA8` | `FM2_D3D_CreateAndUploadVertexIndexBuffers` | Creates VB+IB via D3D imports, validates handles, memcpy upload from mesh source. |
| `0x825A2650` | `FM2_D3D_CreateTextureWrapper` | Thin `D3DDevice_CreateTexture` wrapper used by texture deserialize/upload path. Unleashed `CreateTexture` analogue. |
| `0x825A2CC0` | `FM2_D3D_DeserializeAndUploadTextureSurfaces` | XML/serializer texture load: `D3DTexture_GetSurfaceLevel` + `D3DSurface_LockRect` per mip, byte upload, `FM2_D3DResource_UnlockForRelease`. Unleashed `LockTextureRect` family. |
| `0x825B2EF8` | `FM2_D3D_CacheSurfaceDescFields` | `D3DSurface_GetDesc` → caches width/height/format/multisample into holder object. Unleashed `GetSurfaceDesc`. |
| `0x8259F008` | `FM2_D3D_CreateVertexDeclarationFromElements` | `D3DDevice_CreateVertexDeclaration` from element pointer at `+68`. |
| `0x82537598` | `FM2_Render_CreateCrowdSpriteVertexDeclaration` | Builds `CrowdSpriteDecl` element layout + `CreateVertexDeclaration`. Example title decl factory. |
| `0x825A2350` | `FM2_D3D_LockVertexBufferWrapper` | Thin wrapper → `FM2_D3D_LockGpuBufferRaw`. |
| `0x825A2368` | `FM2_D3D_UnlockVertexBufferWrapper` | Thin wrapper → `FM2_D3D_UnlockGpuBufferRaw`. |
| `0x825A27D8` | `FM2_D3D_DeserializeAndLockIndexBuffer` | Deserializes index container XML into locked index buffer. |
| `0x8236A0B0` | `FM2_D3D_LockGpuBufferRaw` | Shared lock thunk used by VB/IB upload paths (`FM2_AudioRender_CopySurfaceRegionToBuffer` tail). |
| `0x8236A0F8` | `FM2_D3D_UnlockGpuBufferRaw` | `D3D::UnlockResource` wrapper for buffer lock pairs. |
| `0x82515E18` | `FM2_D3D_CreateDepthStencilSurfaceAndTexture` | `D3DDevice_CreateSurface` + companion `CreateTexture` (D24S8). Unleashed `CreateSurface`. |
| `0x825AF1E0` | `FM2_Render_CreateGridVertexDeclarationAndStream` | Creates position/texcoord decl + grid VB for shader resource setup. |
| `0x825D5A70` | `FM2_Render_LiverySectionClearColorAndDraw` | Livery section path calling `FM2_Render_SetClearColorByteAndDirtyFlag` then draw/PM4 emit. Partial Unleashed `Clear` hook boundary. |
| `0x82392090` | `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | `D3DSurface_GetDesc` + optional subrect validation + `D3DSurface_LockRect`. Primary runtime `LockTextureRect` analogue (caller `FM2_D3D_CreateTextureFromSurfaceLevelBody`). |
| `0x823764B0` | `FM2_Render_EmitDrawRangeCountPm4` | Emits PM4 packet type `1407` with vertex draw-range count. Called from object-pass dispatch (`FM2_Render_ApplyObjectPassSamplerAndDrawRange`, `FM2_Render_DispatchObjectPassDrawOpcode`). |
| `0x82383718` | `FM2_D3D_EmitIndexedDrawPacket` | Memcpy vertex data from GPU cached memory into command buffer, emit sampler/texture-stage packets. Closest FM2 analogue to Unleashed `DrawPrimitiveUP`. |
| `0x825B2F80` | `FM2_D3D_CacheSurfaceDescFieldsWithSubrect` | `D3DSurface_GetDesc` variant caching format plus explicit width/height subrect fields. |
| `0x825303B8` | `FM2_Render_ApplyObjectPassSamplerAndDrawRange` | PM4 opcode `45` handler: applies pass samplers/state, may accumulate draw ranges via `FM2_Render_EmitDrawRangeCountPm4`. |

### Gap-pass renames (IDA37, 2026-06-22)

| Address | New name | Evidence |
| --- | --- | --- |
| `0x8236EF88` | `FM2_Render_SetClearFlagsAndDirtyBit` | Stores 3-bit clear mask at render-ctx `+10428`, sets dirty bit `0x200`. Paired with `FM2_Render_SetClearColorByteAndDirtyFlag` in movie passes. Unleashed `Clear` flags analogue. |
| `0x825A25F8` | `FM2_D3D_CreateTextureFromMemoryBuffer` | Takes texture holder + memory ptr + size; calls `FM2_D3D_CreateTextureFromSurfaceLevelAuto`. Unleashed `MakePictureData` hook boundary. |
| `0x823883C0` | `FM2_D3D_CreateTextureFromSurfaceLevelAuto` | Thin wrapper → `FM2_D3D_CreateTextureFromSurfaceLevel` with auto format dispatch. |
| `0x827BA780` | `FM2_MovieRenderer_InitScreenShaderResources` | `D3DDevice_CreateVertexDeclaration` + VS/PS GPU blocks → `dword_82A48288/8C/90`. Unleashed `ScreenShaderInit` structural match. |
| `0x827BC5E0` | `FM2_MovieRenderer_InitMovieShaderResources` | Second shader set → `dword_82A482D8/DC/E0` for movie pre-render pass. |
| `0x827B89F0` | `FM2_MovieRenderer_InitShaderResourceGlobals` | Calls both movie shader inits; reached from `FM2_MovieRenderer_RegisterAndInitShaders`. |
| `0x827B7E70` | `FM2_MovieRenderer_RegisterAndInitShaders` | CRT static init: zero movie renderer state, register control tags, init shader globals. |
| `0x827BA350` | `FM2_MovieRenderer_SetupScreenDrawPass` | Binds screen shader globals, viewport, `SetClearColor(8)`, `SetClearFlags(6)`, sampler/matrix upload. |
| `0x827BC2D0` | `FM2_MovieRenderer_SetupMoviePreRenderPass` | Binds movie shader globals, `SetClearColor(0)`, `SetClearFlags(6)`, matrix constants. |
| `0x82378EF8` | `FM2_GpuKick_NotifyScalerViewportRects` | `ComputeScalerViewportRects` + `VdCallGraphicsNotificationRoutines`. Unleashed `SetResolution` notify path. |
| `0x8229E198` | `FM2_MovieRenderer_EnqueuePlaylistEntry` | Queues Bink movie playlist entries with timing; not shader init. |

### Clear PM4 emit pass (IDA37, 2026-06-22)

| Address | Name | Evidence |
| --- | --- | --- |
| `0x8237CBD8` | `D3D::SetTileAndDepthClear` | D3D runtime: packs Z+stencil to render-ctx `+0x29A8`/`+0x29AC`, dirty `0x300`. Called from `FM2_AudioMix_SubmitPendingOutputBody`. Unleashed `Clear` Z/stencil half. |
| `0x82382928` | `FM2_D3D_CountLeadingDirtyBits` | Emits PM4 TYPE-0 register bursts from dirty shadow tables. Clear color: PM4 base `0x2100`, shadow `ctx+0x284C` (float at `+0x2884`). Clear flags: base `0x2200`, shadow `ctx+0x28B4`. Z/stencil: base `0x2280`, shadow `ctx+0x28E4`. |
| `0x82375ED0` | `FM2_D3D_EmitDirtyStateAndDrawList` | Orchestrates dirty emit during draw-list submit; calls `CountLeadingDirtyBits` for clear register groups when dirty bits from clear setters are set. |
| `0x8237D158` | `FM2_AudioMix_SubmitPendingOutputBody` | Shared resolve/clear/output flush (misnamed): calls `SetTileAndDepthClear` then runs `CountLeadingDirtyBits` sweeps. Callers include frame pipeline, livery, object-pass blits. |

## Ranked Hook Order

1. `FM2_Render_BuildDirectIndexedDrawBuffers`: first world-geometry prototype.
   It already exposes direct draw records, resources, primitive type, start
   index, index count, pass constants, bound surface, and command-buffer scope.
2. Render-context setters/binders listed above: build the Plume state mirror
   from semantic shader/resource/surface state instead of interpreting emitted
   PM4 bytes.
3. `FM2_D3D_TryPresentAndUpdateStatus` plus `FM2_D3D_LazyInitPresentChain`:
   establish Plume frame/present ownership.
4. `FM2_Render_ExecuteBoundDrawPass` / `FM2_Render_DrawIndexedPrimitive`: add
   cached draw-list support after the direct indexed path is visible in Plume.
5. `FM2_D3D_EmitDirtyStateAndDrawList`, `FM2_D3D_EmitDrawListStatePackets`, and
   `FM2_D3D_EmitSurfaceResolvePackets`: keep as diagnostics or later fallback
   translation points, because they are below the desired product boundary.
