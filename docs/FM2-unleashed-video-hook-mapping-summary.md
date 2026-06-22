# FM2 ↔ UnleashedRecomp `video.cpp` Hook Mapping Summary

Date: 2026-06-22  
Context: FM2 Plume native-renderer effort — map UnleashedRecomp guest hooks to FM2
`default.xex` functions for IDA naming, manifest symbols, and future
`FM2/src/fm2_hooks.cpp` mid-asm injection.

## Purpose

UnleashedRecomp's Plume renderer intercepts Sonic Unleashed D3D wrapper calls via
`GUEST_FUNCTION_HOOK` entries in
`UnleashedRecomp/UnleashedRecomp/gpu/video.cpp` (lines ~7798–7862). This document
records the semantic FM2 analogues discovered by working backwards from that hook
table into the FM2 XEX (IDA database `default.xex.i64`).

Related detail logs:

- `docs/FM2-ida-renames-2026-06-22.md` — per-rename evidence table
- `docs/FM2-ida-toml-function-notes.md` — manifest cross-reference
- `docs/FM2-native-renderer-generator-notes.md` — Plume hook product boundary

## Methodology

1. **Unleashed IDA** (port 13338): rename hooked functions `SWA_Video_*` from
   `video.cpp` symbol names.
2. **FM2 IDA** (port 13337): search by behavior, D3D import xrefs, and render-context
   layer — **not** by byte signature.
3. **Evidence rule**: every FM2 name is decompile-based; no heuristic placeholder
   names.

### Why signatures fail

Direct byte signatures for the same D3D runtime imports do **not** match between
Sonic Unleashed and Forza Motorsport 2. Both titles share Microsoft D3D import
thunks (`D3DSurface_LockRect`, `D3DDevice_CreateTexture`, etc.) but wrap them in
different game-specific layers. Mapping must follow **semantic behavior**.

### Architectural difference

| UnleashedRecomp | FM2 |
| --- | --- |
| Relatively flat D3D device API hooks | Render-context setters (`FM2_RenderContext_*`) plus cached command-buffer compilation |
| Host hooks individual `DrawIndexedPrimitive` calls | Primary world geometry often flows through `FM2_Render_BuildDirectIndexedDrawBuffers` before PM4 emit |
| `LockTextureRect` allocates guest heap memory | Texture lock goes through `D3DSurface_LockRect` in create/upload paths |
| `SetResolution` writes device `[46]`/`[47]` at runtime | `VdQueryVideoMode` during present backbuffer setup only |

## Full hook cross-reference

| UnleashedRecomp hook | SWA IDA name | SWA address | FM2 analogue | FM2 address |
| --- | --- | --- | --- | --- |
| `CreateDevice` | `SWA_Video_CreateDevice` | `0x82BD9948` | `FM2_D3D_InitGlobalDeviceSingleton` | `0x824A52C0` |
| Present backbuffer setup | — | — | `FM2_D3D_CreatePresentBackbufferResources` | `0x82374190` |
| `Present` | `SWA_Video_Present` | `0x82BDA6D0` | `FM2_D3D_TryPresentAndUpdateStatus` | `0x824F83D8` |
| `GetBackBuffer` | `SWA_Video_GetBackBuffer` | `0x82BDD2C8` | `FM2_RenderContext_GetBoundSurfaceAndAddRef` | `0x82371040` |
| `SetRenderTarget` / depth | `SWA_Video_SetRenderTargetOrDepth` | `0x82BDD930` | `FM2_RenderContext_SetBoundSurface` | `0x82371A30` |
| `SetViewport` | `SWA_Video_SetViewport` | `0x82BDD440` | `FM2_RenderContext_SetViewportModeAndApply` | `0x823715B0` |
| `SetScissorRect` | `SWA_Video_SetScissorRect` | `0x82BDCF68` | `FM2_D3D_EmitScissorRegionPackets` | `0x8236E780` |
| `SetStreamSource` | `SWA_Video_SetStreamSource` | `0x82BDD058` | `FM2_RenderContext_BindVertexStream` | `0x82370E48` |
| `SetIndices` | `SWA_Video_SetIndices` | `0x82BDD218` | `FM2_RenderContext_BindIndexBuffer` | `0x82370F68` |
| `SetVertexShader` / decl | `SWA_Video_SetVertexDeclOrShader` | `0x82BE0050` | `FM2_RenderContext_SetVertexShaderState` | `0x8236E010` |
| `SetPixelShader` | `SWA_Video_SetPixelShader` | `0x82BDFE10` | `FM2_RenderContext_SetPixelShaderState` | `0x8236DD10` |
| `CreateTexture` | `SWA_Video_D3DResourceDispatch` | `0x82BE9498` | `FM2_D3D_CreateTextureWrapper` / `FM2_D3D_CreateTextureResourceFromFormat` | `0x825A2650` / `0x823852F8` |
| `LockTextureRect` | `SWA_Video_D3DResourceDispatch` | `0x82BE9300` | **Runtime:** `FM2_D3D_GatherSurfaceMetadataForTextureCreate`; **asset/XML:** `FM2_D3D_DeserializeAndUploadTextureSurfaces` | `0x82392090` / `0x825A2CC0` |
| `UnlockTextureRect` | `SWA_Video_UnlockTextureRect` | `0x82BE7780` | `FM2_D3DResource_UnlockForRelease` | `0x8236A2B8` |
| `SetTexture` | `SWA_Video_SetTexture` | `0x82BE9818` | `FM2_RenderContext_SetTextureFetchBitsLow` / `Mid` | `0x8236EA60` / `0x8236EA90` |
| `GetSurfaceDesc` | `SWA_Video_GetSurfaceDesc` | `0x82BE96F0` | `FM2_D3D_CacheSurfaceDescFields` / `FM2_D3D_CacheSurfaceDescFieldsWithSubrect` | `0x825B2EF8` / `0x825B2F80` |
| `CreateVertexBuffer` | `SWA_Video_CreateOrLockVertexBuffer` | `0x82BE6AD0` | `FM2_D3D_CreateVertexBufferWrapper` | `0x825A22F0` |
| `LockVertexBuffer` | `SWA_Video_CreateOrLockVertexBuffer` | `0x82BE6B98` | `FM2_D3D_LockVertexBufferWrapper` → `FM2_D3D_LockGpuBufferRaw` | `0x825A2350` / `0x8236A0B0` |
| `UnlockVertexBuffer` | `SWA_Video_UnlockVertexBuffer` | `0x82BE6BE8` | `FM2_D3D_UnlockVertexBufferWrapper` → `FM2_D3D_UnlockGpuBufferRaw` | `0x825A2368` / `0x8236A0F8` |
| `GetVertexBufferDesc` | `SWA_Video_GetBufferDesc` | `0x82BE61D0` | Fields at holder `+4` (stride) / `+8` (count) set by create wrapper | `0x825A22F0` |
| `CreateIndexBuffer` | `SWA_Video_CreateIndexBuffer` | `0x82BE6BF8` | `FM2_D3D_CreateIndexBufferWrapper` | `0x825A2730` |
| `LockIndexBuffer` | `SWA_Video_LockIndexBuffer` | `0x82BE6CA8` | `FM2_D3D_DeserializeAndLockIndexBuffer` → `FM2_D3D_LockGpuBufferRaw` | `0x825A27D8` / `0x8236A0B0` |
| `UnlockIndexBuffer` | `SWA_Video_UnlockIndexBuffer` | `0x82BE6CF0` | `FM2_D3D_UnlockGpuBufferRaw` | `0x8236A0F8` |
| `GetIndexBufferDesc` | `SWA_Video_GetIndexBufferDesc` | `0x82BE6200` | Fields cached on holder during `CreateIndexBufferWrapper` | `0x825A2730` |
| `CreateSurface` | `SWA_Video_CreateSurface` | `0x82BE95B8` | `FM2_D3D_CreateDepthStencilSurfaceAndTexture` | `0x82515E18` |
| `CreateVertexDeclaration` | `SWA_Video_CreateVertexDeclaration` | `0x82BE0428` | `FM2_D3D_CreateVertexDeclarationFromElements` / `FM2_Render_CreateGpuBuffers` | `0x8259F008` / `0x82721970` |
| `HashVertexDeclaration` | `SWA_Video_HashVertexDeclaration` | `0x82BE0530` | Identity: D3D decl pointer at holder `+76` (Unleashed host also returns pointer) | `0x8259F008` |
| `GetVertexDeclaration` | `SWA_Video_GetVertexDeclaration` | `0x82BE04B0` | Bundled in render-context VS/decl setters | `0x8236E010` |
| `CreateVertexShader` | `SWA_Video_CreateVertexShader` | `0x82BE1A80` | `FM2_Render_GetOrCreateVertexShaderResourceById` | `0x825A16E0` |
| `CreatePixelShader` | `SWA_Video_CreatePixelShader` | `0x82BE1990` | `FM2_Render_GetOrCreatePixelShaderResourceById` | `0x825A1608` |
| `DrawIndexedPrimitive` | `SWA_Video_DrawIndexedPrimitive` | `0x82BE5CF0` | `FM2_Render_DrawIndexedPrimitive` | `0x827221F0` |
| `DrawPrimitive` | `SWA_Video_DrawPrimitive` | `0x82BE5900` | `FM2_Render_DrawIndexedPrimitive` + `FM2_Render_EmitDrawRangeCountPm4` (PM4 `1407`); blit helper: `FM2_Render_EmitIndexedTriangleFanDrawPm4` | `0x827221F0` / `0x823764B0` / `0x8272F650` |
| `DrawPrimitiveUP` | `SWA_Video_DrawPrimitiveUP` | `0x82BE52F8` | `FM2_D3D_EmitIndexedDrawPacket` (memcpy vertex ring → CB) | `0x82383718` |
| `StretchRect` | `SWA_Video_StretchRect` | `0x82BF6400` | `FM2_Render_BlitTiledRegionTriangleFanPm4` | `0x8272FBA0` |
| `Clear` | `SWA_Video_Clear` | `0x82BFE4C8` | **Partial:** `FM2_Render_SetClearColorByteAndDirtyFlag` + `FM2_Render_LiverySectionClearColorAndDraw` | `0x8236EF20` / `0x825D5A70` |
| `D3DXFillTexture` | `SWA_Video_D3DXFillTexture` | `0x82C003B8` | `FM2_RenderResource_FillTextureSurfaceLayoutByFormat` | `0x8272AEB8` |
| `D3DXFillVolumeTexture` | `SWA_Video_D3DXFillVolumeTexture` | `0x82C00910` | Same fill helper via volume mip setup | `0x8272AEB8` |
| `DestructResource` | `SWA_Video_DestructResource` | `0x82BE6210` | `FM2_D3D_ReleaseGpuResourceRef` | `0x8237ED10` |
| `MakePictureData` | — | `0x82E43FC8` | **No FM2 analogue** (Sonic picture/atlas loader) | — |
| `SetResolution` | — | `0x82E9EE38` | `VdQueryVideoMode` inside `FM2_D3D_CreatePresentBackbufferResources` only | `0x82374190` |
| `ScreenShaderInit` | — | `0x82AE2BF8` | **No FM2 analogue**; closest: `FM2_RenderAdapter_InitPresentationVtables_ClearState` | `0x824EA598` |
| FM2-only direct draw | — | — | `FM2_Render_BuildDirectIndexedDrawBuffers` | `0x825380B8` |

## Corrections from the second mapping pass

These supersede weaker first-pass guesses:

| Hook | Wrong / weak guess | Corrected FM2 mapping |
| --- | --- | --- |
| `LockTextureRect` | `FM2_D3D_DeserializeAndUploadTextureSurfaces` only | **Primary:** `FM2_D3D_GatherSurfaceMetadataForTextureCreate` (`0x82392090`) — real `D3DSurface_LockRect` with subrect validation. Deserialize path is asset/XML upload only. |
| `DrawPrimitive` | `FM2_Render_EmitIndexedTriangleFanDrawPm4` as main draw | **Primary:** `FM2_Render_DrawIndexedPrimitive` + `FM2_Render_EmitDrawRangeCountPm4` (PM4 type `1407`). Triangle-fan emit is a blit/stretch helper. |
| `DrawPrimitiveUP` | `FM2_D3D_HandleGpuHang` (hang diagnostic strings mention DrawVertices) | `FM2_D3D_EmitIndexedDrawPacket` — copies vertex bytes from GPU ring memory into the command buffer, then emits sampler/texture-stage packets. |
| `HashVertexDeclaration` | Separate hash function | Unleashed host returns the declaration pointer as the hash; FM2 stores the D3D decl at resource holder `+76`. |

## FM2-only hook (no Unleashed equivalent)

**`FM2_Render_InstanceHybridDrawPath`** (`0x82539650`) is the recommended per-frame
world-geometry hook. It is the **sole direct caller** of
`FM2_Render_BuildDirectIndexedDrawBuffers` and fires every time the hybrid draw path
is taken, without any one-shot guard.

**`FM2_Render_BuildDirectIndexedDrawBuffers`** (`0x825380B8`) is the build-phase
sub-function called from `InstanceHybridDrawPath`. A guard byte at
`direct_render_ctx + 0x48` is set to `1` on first entry, causing every subsequent
call to be skipped — making this a **one-shot initializer per render object, not a
per-frame hook**. Useful for initial VB/IB layout and draw-record discovery; use
`FM2_Render_InstanceHybridDrawPath` for per-frame interception.

Both expose:

- Direct draw records at `direct_render_ctx+0x5A4` / `+0x5A8` (52-byte entries)
- Stream bind via draw-iface slot `+0x64`
- Index bind via draw-iface slot `+0x74`
- Draw issue via draw-iface slot `+0x80` (primitive type 4, start index, count/3)
- Pass constants, bound surface, command-buffer context switch

Unleashed has no direct equivalent because it hooks flat D3D draw calls rather than
FM2's cached direct-draw record compiler.

## Render-context layer (preferred Plume state mirror)

Hook these **above** raw PM4 emitters when building the Plume state mirror:

| Function | Address | Unleashed analogue |
| --- | --- | --- |
| `FM2_RenderContext_SetPixelShaderState` | `0x8236DD10` | `SetPixelShader` |
| `FM2_RenderContext_SetVertexShaderState` | `0x8236E010` | `SetVertexShader` / decl |
| `FM2_RenderContext_BindVertexStream` | `0x82370E48` | `SetStreamSource` |
| `FM2_RenderContext_BindIndexBuffer` | `0x82370F68` | `SetIndices` |
| `FM2_RenderContext_SetBoundSurface` | `0x82371A30` | `SetRenderTarget` / depth |
| `FM2_RenderContext_SetViewportModeAndApply` | `0x823715B0` | `SetViewport` |
| `FM2_RenderContext_SetTextureFetchBitsLow` / `Mid` | `0x8236EA60` / `0x8236EA90` | `SetTexture` |

## Present chain

| Function | Address | Role |
| --- | --- | --- |
| `FM2_D3D_InitGlobalDeviceSingleton` | `0x824A52C0` | Device singleton + worker threads (`CreateDevice`) |
| `FM2_D3D_CreatePresentBackbufferResources` | `0x82374190` | `VdQueryVideoMode`, backbuffer texture/surface, `SetRenderTarget` |
| `FM2_D3D_LazyInitPresentChain` | `0x824F6520` | Present-chain lazy init |
| `FM2_D3D_LazyInitPresentChainInit` | `0x82502268` | Present-chain subscriber ctor |
| `FM2_D3D_TryPresentAndUpdateStatus` | `0x824F83D8` | `Present` |
| `FM2_GpuCommandBuffer_BuildAndSubmit` | `0x8236CB28` | Kicks GPU (`VdSwap`, `VdPersistDisplay`) |

## Lock / unlock pairs

| Resource | Lock | Unlock |
| --- | --- | --- |
| Texture surface (runtime) | `FM2_D3D_GatherSurfaceMetadataForTextureCreate` `0x82392090` | `FM2_D3DResource_UnlockForRelease` `0x8236A2B8` |
| Texture surface (XML asset) | `FM2_D3D_DeserializeAndUploadTextureSurfaces` `0x825A2CC0` | `FM2_D3DResource_UnlockForRelease` `0x8236A2B8` |
| Vertex buffer | `FM2_D3D_LockVertexBufferWrapper` → `FM2_D3D_LockGpuBufferRaw` | `FM2_D3D_UnlockVertexBufferWrapper` → `FM2_D3D_UnlockGpuBufferRaw` |
| Index buffer | `FM2_D3D_DeserializeAndLockIndexBuffer` → `FM2_D3D_LockGpuBufferRaw` | `FM2_D3D_UnlockGpuBufferRaw` |

Shared raw lock thunk `FM2_D3D_LockGpuBufferRaw` (`0x8236A0B0`) tails into
`FM2_AudioRender_CopySurfaceRegionToBuffer`.

## Draw dispatch landmarks

| Function | Address | Notes |
| --- | --- | --- |
| `FM2_Render_InstanceHybridDrawPath` | `0x82539650` | Per-frame hybrid draw entry; sole caller of `BuildDirectIndexedDrawBuffers`; no guard |
| `FM2_Render_BuildDirectIndexedDrawBuffers` | `0x825380B8` | **Build-phase only** — one-shot guard at `direct_render_ctx+0x48`; skips on re-entry |
| `FM2_Render_ExecuteBoundDrawPass` | `0x82723750` | Per-frame per-pass execute gate; called via vtable; wraps `BindPassStateToContext` + `WalkAndDispatchPm4DrawList` |
| `FM2_Render_ExecuteSortedDrawListsCore` | `0x825222b0` | **CB texture/constant fixup only** — not a draw dispatcher; patches `D3DCommandBuffer` via `BeginDynamicFixups`/`SetTexture`/`SetShaderConstantF` |
| `FM2_Render_DrawIndexedPrimitive` | `0x827221F0` | Binds IB, emits indexed PM4 (`8450`); already at PM4 boundary |
| `FM2_Render_EmitDrawRangeCountPm4` | `0x823764B0` | PM4 type `1407` vertex-range count |
| `FM2_D3D_EmitIndexedDrawPacket` | `0x82383718` | UP-like memcpy into CB |
| `FM2_Render_ApplyObjectPassSamplerAndDrawRange` | `0x825303B8` | Object-pass opcode `45` handler |
| `FM2_Render_DispatchObjectPassDrawOpcode` | `0x82530FC8` | Object-pass opcode dispatcher |
| `FM2_Render_DispatchPm4DrawOpcode` | `0x82722808` | 57-entry PM4 opcode jump table |
| `FM2_Render_WalkAndDispatchPm4DrawList` | `0x82722FD8` | Cached draw-list walker (lower-level) |
| `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset` | `0x827317A0` | Raw indexed PM4 emit (`8450`) |

Treat PM4 emitters as **diagnostics or phase-2 fallback**, not the primary Plume
product boundary.

## Open gaps

| Hook | Status | Next search direction |
| --- | --- | --- |
| `Clear` (full D3D clear) | Partial only | PM4 clear packet emitters; `D3DDevice_Clear` import xrefs |
| `MakePictureData` | Sonic-specific | No FM2 analogue expected |
| `ScreenShaderInit` | Sonic movie shaders | FM2 presentation init is unrelated |
| `SetResolution` | Init-time only | No runtime `ResolutionScale` equivalent found |

## Recommended hook order for Plume native renderer

1. **`FM2_Render_InstanceHybridDrawPath`** (`0x82539650`) — per-frame hybrid draw
   entry; no one-shot guard; fires every frame for each hybrid-path object. Use this
   for per-frame world-geometry interception. `FM2_Render_BuildDirectIndexedDrawBuffers`
   (`0x825380B8`) is suitable only for one-time VB/IB layout and draw-record discovery
   (one-shot build guard prevents per-frame use).
2. **Render-context setters** — build Plume state mirror from semantic state
3. **`FM2_D3D_TryPresentAndUpdateStatus`** + **`FM2_D3D_LazyInitPresentChain`** — frame/present ownership
4. **`FM2_Render_ExecuteBoundDrawPass`** (`0x82723750`) — per-frame per-pass execute
   gate; redirects by patching the vtable pointer stored in
   `FM2_Presentation_AllocCarInstanceSlot` / `FM2_Render_InitTlsContextDefaults` (no
   direct `bl` callers — indirect call only). Wraps `FM2_RenderTls_BindPassStateToContext`
   + `FM2_Render_WalkAndDispatchPm4DrawList`. `FM2_Render_DrawIndexedPrimitive`
   (`0x827221F0`) is a secondary signal at the PM4 boundary; state must be read from
   the `dword_82A41BEC` global.
5. **`FM2_D3D_EmitDirtyStateAndDrawList`** — called directly per-renderable inside
   `FM2_Render_ExecuteSortedDrawLists`; a real per-renderable integration point, not
   diagnostics-only. **`FM2_D3D_EmitSurfaceResolvePackets`** — resolve/blit fallback.

## Hook correctness notes (2026-06-22)

- **`FM2_Render_BuildDirectIndexedDrawBuffers` is one-shot, not per-frame.** The guard
  byte at `direct_render_ctx + 0x48` is set to `1` after first entry and blocks all
  re-entry. The previous recommendation of this function as the primary world-geometry
  hook was incorrect. Use `FM2_Render_InstanceHybridDrawPath` (`0x82539650`) instead.

- **`FM2_Render_ExecuteSortedDrawListsCore` (`0x825222b0`) is not a draw dispatcher.**
  Despite the name, it only patches per-frame textures and shader constants into an
  already-compiled `D3DCommandBuffer` (`BeginDynamicFixups` / `SetTexture` /
  `SetShaderConstantF`). The actual draw dispatch is `FM2_D3D_EmitDirtyStateAndDrawList`,
  called immediately after it in `FM2_Render_ExecuteSortedDrawLists`.

- **`FM2_Render_ExecuteBoundDrawPass` (`0x82723750`) has no direct callers.** The
  function pointer is stored into slots by `FM2_Presentation_AllocCarInstanceSlot`
  (`0x8250E5E4`) and `FM2_Render_InitTlsContextDefaults` (`0x82723974`). Hook via
  vtable pointer patch or mid-asm hook at the indirect call site — a plain mid-asm
  hook on the function body itself works since the function prologue is reachable.

## IDA databases and tooling

| Database | Port | Path |
| --- | --- | --- |
| FM2 `default.xex` | 13337 | `D:\Emulation\Games_Xbox_360\Forza2\default.xex.i64` |
| Sonic Unleashed `default.xex` | 13338 | `D:\Emulation\Games_Xbox_360\Sonic_Unleashed\...\default.xex.i64` |

Use IDA38 MCP `select_instance` before cross-database work. FM2 renames and
Unleashed cross-ref comments were applied via the `user-IDA38` MCP server.

## Manifest symbols burned

Key addresses added to `FM2/fm2_manifest.toml` during this work include:

`0x82374190`, `0x824A52C0`, `0x82502268`, `0x825A22F0`, `0x825A2350`,
`0x825A2368`, `0x825A2650`, `0x825A2730`, `0x825A27D8`, `0x825A2CC0`,
`0x82515E18`, `0x8259F008`, `0x825B2EF8`, `0x825B2F80`, `0x8236A0B0`,
`0x8236A0F8`, `0x8236A2B8`, `0x82392090`, `0x823764B0`, `0x82383718`,
`0x825303B8`, `0x8272F650`, `0x8272FBA0`, `0x8272AEB8`, `0x827BAAA8`.

## Coverage estimate

~40 of 43 Unleashed `video.cpp` hooks have a documented FM2 semantic analogue.
Some mappings are multi-function or field-based (buffer desc reads holder fields
rather than a separate export). Three hooks remain without FM2 equivalents
(`MakePictureData`, `ScreenShaderInit`, runtime `SetResolution`); `Clear` is
partial only.

## Next steps

1. Search FM2 for full `Clear` PM4 emit path (beyond clear-color byte setter).
2. Burn confirmed hook points into `FM2/src/fm2_hooks.cpp` per
   `docs/FM2-native-renderer-generator-notes.md`.
3. Optionally correct misleading SWA IDA names where hook EAs land inside D3D
   runtime internals rather than clean game wrapper entry points.
