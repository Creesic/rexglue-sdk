# FM2 ReOdyssey Hook Equivalence Map

Date: 2026-06-22

Scope: every symbol listed in
`.cursor/hooks/state/reodyssey_hook_symbols.json`, checked against the FM2 IDA
database through ida37 (`D:\Emulation\Games_Xbox_360\Forza2\default.xex.i64`).

Method:

- Resolve the ReOdyssey symbol name directly in FM2 IDA first.
- If the direct name is absent, locate the FM2 title wrapper or PM4/state path
  already named in `default.xex.i64`.
- Treat raw `sub_823C...` labels as address-collision risks, not portable names.
- Mark a mapping as partial when FM2 packs multiple D3D states into one title
  field or emits the state later through PM4.

## Summary

FM2 shares several low-level XDK resource helpers with ReOdyssey, but most of
the render-state, shader, draw, and present hooks do not exist under ReOdyssey's
public D3D/UE3 names. The usable FM2 native-renderer anchors are mostly:

- XDK resource helpers: create/lock surface, texture, index, vertex, and vertex
  declaration functions.
- FM2 title wrappers: `FM2_RenderContext_*`, `FM2_D3D_*`, and
  `FM2_Render_GetOrCreate*ShaderResourceById`.
- FM2 PM4 emitters: draw, resolve, scissor, pending render states, and shader
  constants.

## Complete Mapping

| ReOdyssey symbol | FM2 equivalent | FM2 address | Status | Evidence / notes |
|---|---|---:|---|---|
| `rex_BlockOnFence_CDevice_D3D_QAAXKW4_D3DBLOCKTYPE_PAUD3DResource_Z` | none found | - | absent | Exact name absent in IDA37. Do not substitute `BlockOnSecondaryPosition`; it waits a different cursor condition. |
| `rex_BlockOnSecondaryPosition_CDevice_D3D_QAAXPAKK_Z` | `D3D::CDevice::BlockOnSecondaryPosition` (`?BlockOnSecondaryPosition@CDevice@D3D@@QAAXPAKK@Z`) | `0x82371D60` | valid sync equivalent | Exact ReOdyssey alias absent, but decorated FM2 XDK method resolves and waits on the secondary command-buffer cursor with `D3DBLOCKTYPE_SECONDARY_OVERRUN`. |
| `rex_D3DBaseTexture_LockTail` | none found | - | absent / unresolved | Exact name absent. For surface uploads, use the confirmed FM2 surface lock path below; texture tail lock still needs a separate locate pass if required. |
| `rex_D3DDevice_BeginVertices` | none found | - | absent | Exact name absent. FM2 does not expose the ReOdyssey immediate-vertex public path in the named render flow. |
| `rex_D3DDevice_BlockUntilIdle` | none found | - | absent | Exact name absent. |
| `rex_D3DDevice_ClearF` | `FM2_Render_SetClearColorByteAndDirtyFlag` + `FM2_Render_SetClearFlagsAndDirtyBit` | `0x8236EF20`, `0x8236EF88` | partial substitute | Exact `ClearF` name absent. FM2 records clear state through title helpers and emits it later through the dirty-state PM4 path. |
| `rex_D3DDevice_CreateIndexBuffer` | `rex_D3DDevice_CreateIndexBuffer`; FM2 title wrapper `FM2_D3D_CreateIndexBufferWrapper` | `0x8236A000`; `0x825A2730` | direct match + wrapper | Exact XDK helper resolves in IDA37. The FM2 wrapper is the title-level creation path. |
| `rex_D3DDevice_CreatePixelShader` | `FM2_Render_GetOrCreatePixelShaderResourceById`; init path `FM2_Render_InitPixelShaderResource` | `0x825A1608`; `0x8259F6F0` | FM2 substitute | Exact public D3D shader creation name absent. FM2 loads shader resources by ID. |
| `rex_D3DDevice_CreateSurface` | `D3DDevice_CreateSurface`; depth wrapper `FM2_D3D_CreateDepthStencilSurfaceAndTexture` | `0x8236BFC0`; `0x82515E18` | direct match + wrapper | Exact XDK surface creation resolves as `D3DDevice_CreateSurface`. Depth surfaces often go through the FM2 title wrapper. |
| `rex_D3DDevice_CreateTexture` | `rex_D3DDevice_CreateTexture`; FM2 title wrapper `FM2_D3D_CreateTextureWrapper` | `0x8236BEA0`; `0x825A2650` | direct match + wrapper | Exact XDK helper resolves in IDA37. |
| `rex_D3DDevice_CreateVertexBuffer` | `D3DDevice_CreateVertexBuffer`; FM2 title wrapper `FM2_D3D_CreateVertexBufferWrapper` | `0x82369ED8`; `0x825A22F0` | direct match + wrapper | Exact helper resolves without `rex_` prefix in FM2. |
| `rex_D3DDevice_CreateVertexDeclaration` | `D3DDevice_CreateVertexDeclaration`; FM2 title wrapper `FM2_D3D_CreateVertexDeclarationFromElements` | `0x8236E240`; `0x8259F008` | direct match + wrapper | Exact XDK declaration creator exists; FM2 wrapper reads the title declaration holder and stores the result. |
| `rex_D3DDevice_CreateVertexShader` | `FM2_Render_GetOrCreateVertexShaderResourceById`; init path `FM2_Render_InitVertexShaderResource` | `0x825A16E0`; `0x8259F750` | FM2 substitute | Exact public D3D shader creation name absent. FM2 loads shader resources by ID. |
| `rex_D3DDevice_DrawIndexedVertices` | `FM2_Render_DrawIndexedPrimitive` | `0x827221F0` | FM2 draw substitute | Public D3D draw name absent. FM2 bypasses that layer and enters its own indexed primitive path before PM4 emit. |
| `rex_D3DDevice_DrawVertices` | none found | - | absent | Exact name absent; no verified non-indexed FM2 draw hook found in this pass. |
| `rex_D3DDevice_DrawVerticesUP` | none found | - | absent | Exact name absent. `FM2_D3D_EmitIndexedDrawPacket` is a packet emitter, not a public UP draw equivalent. |
| `rex_D3DDevice_Resolve` | `FM2_D3D_EmitSurfaceResolvePackets` | `0x82382590` | PM4 substitute | Exact public resolve name absent. FM2 emits resolve packets from dirty state. |
| `rex_D3DDevice_SetDepthStencilSurface` | `FM2_RenderContext_SetBoundSurface`; underlying `FM2_RenderContext_BindSurfaceInternal` | `0x82371A30`; `0x823716F8` | FM2 wrapper substitute | Exact public name absent. FM2 binds render/depth surfaces through the render-context surface path. |
| `rex_D3DDevice_SetIndices` | `FM2_RenderContext_BindIndexBuffer` | `0x82370F68` | FM2 wrapper substitute | Exact public name absent. FM2 render context binds the index resource. |
| `rex_D3DDevice_SetPixelShader` | `FM2_RenderContext_SetPixelShaderState` | `0x8236DD10` | FM2 wrapper substitute | Exact public name absent. FM2 render context stores the pixel shader state and dirty bits. |
| `rex_D3DDevice_SetPixelShaderConstantB` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact constant setter absent. FM2 batches shader constants and emits them through PM4. No one-to-one boolean setter found. |
| `rex_D3DDevice_SetPixelShaderConstantFN` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact float constant setter absent. |
| `rex_D3DDevice_SetPixelShaderConstantI` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact integer constant setter absent. |
| `rex_D3DDevice_SetPredication` | none found | - | absent / unresolved | Exact name absent. No useful FM2 native-render hook located in this pass. |
| `rex_D3DDevice_SetRenderState_AlphaBlendEnable` | `FM2_RenderContext_SetAlphaBlendEnableBits` | `0x8236F1F0` | FM2 packed-state substitute | Writes `(16 * value) & 0x70` into `ctx+10420`; marks dirty `0x800` and `0x20000`. |
| `rex_D3DDevice_SetRenderState_AlphaTestEnable` | `FM2_RenderContext_SetAlphaTestState` | `0x8236F268` | FM2 packed-state substitute | Writes alpha-test bit at `ctx+10420`; marks dirty state. |
| `rex_D3DDevice_SetRenderState_BlendOp` | `FM2_RenderContext_SetBlendModeBits` | `0x8236F2A0` | partial packed-state substitute | Writes a 3-bit blend-mode field at `ctx+10420`. Individual ReOdyssey blend-op semantics are not one-to-one in FM2. |
| `rex_D3DDevice_SetRenderState_BlendOpAlpha` | `FM2_RenderContext_SetBlendModeBits` | `0x8236F2A0` | partial / not separate | No separate FM2 alpha blend-op setter was verified. Treat this as part of the packed blend state until proven otherwise. |
| `rex_D3DDevice_SetRenderState_ClipPlaneEnable` | `FM2_RenderContext_SetClipPlane0Enable` through `SetClipPlane3Enable` | `0x8236F440`-`0x8236F4A0` | FM2 grouped substitute | Exact aggregate name absent. FM2 has individual clip-plane enable byte writers for planes 0-3. |
| `rex_D3DDevice_SetRenderState_ColorWriteEnable` | `FM2_RenderContext_SetColorWriteMaskBits` | `0x8236F340` | FM2 packed-state substitute | Writes bits 14-16 of `ctx+10420`; marks dirty `0x800`. |
| `rex_D3DDevice_SetRenderState_DepthBias` | `FM2_Render_ComputeDepthBiasFromContext` | `0x8255D270` | partial substitute | Exact setter absent. FM2 computes/uploads a depth-bias vector from render context fields and shader constants. |
| `rex_D3DDevice_SetRenderState_DestBlend` | `FM2_RenderContext_SetBlendModeBits` / `FM2_D3D_EmitDrawListStatePackets` | `0x8236F2A0`; `0x82383A70` | partial / not separate | No separate dest-blend setter was verified. Blend factors appear folded into FM2's packed draw-list state. |
| `rex_D3DDevice_SetRenderState_DestBlendAlpha` | `FM2_RenderContext_SetBlendModeBits` / `FM2_D3D_EmitDrawListStatePackets` | `0x8236F2A0`; `0x82383A70` | partial / not separate | No separate alpha dest-blend setter was verified. |
| `rex_D3DDevice_SetRenderState_SlopeScaleDepthBias` | `FM2_Render_ComputeDepthBiasFromContext` | `0x8255D270` | partial substitute | Exact setter absent. FM2 depth-bias handling is computed from context rather than a public render-state setter. |
| `rex_D3DDevice_SetRenderState_SrcBlend` | `FM2_RenderContext_SetBlendModeBits` / `FM2_D3D_EmitDrawListStatePackets` | `0x8236F2A0`; `0x82383A70` | partial / not separate | No separate src-blend setter was verified. |
| `rex_D3DDevice_SetRenderState_SrcBlendAlpha` | `FM2_RenderContext_SetBlendModeBits` / `FM2_D3D_EmitDrawListStatePackets` | `0x8236F2A0`; `0x82383A70` | partial / not separate | No separate alpha src-blend setter was verified. |
| `rex_D3DDevice_SetRenderState_ViewportEnable` | `FM2_RenderContext_SetViewportModeAndApply` | `0x823715B0` | FM2 substitute | Exact render-state name absent. FM2 stores viewport mode at `ctx+11576` and reapplies viewport constants. |
| `rex_D3DDevice_SetRenderState_ZEnable` | `FM2_RenderContext_SetDepthStencilEnableState` | `0x8236EAF8` | FM2 packed-state substitute | Toggles depth/stencil enable and mirrors packed z/stencil state to multiple shadow dwords. |
| `rex_D3DDevice_SetRenderTarget` | `FM2_RenderContext_SetBoundSurface`; underlying `FM2_RenderContext_BindSurfaceInternal` | `0x82371A30`; `0x823716F8` | FM2 wrapper substitute | Exact public name absent. Same FM2 path also covers depth/stencil surface binding. |
| `rex_D3DDevice_SetScissorRect` | `FM2_D3D_EmitScissorRegionPackets` | `0x8236E780` | PM4 substitute | Exact public name absent. FM2 emits scissor PM4 packets from context/viewport state. |
| `rex_D3DDevice_SetShaderGPRAllocation` | none found | - | absent / unresolved | Exact name absent. No useful FM2 native-render hook located in this pass. |
| `rex_D3DDevice_SetStreamSource` | `FM2_RenderContext_BindVertexStream` | `0x82370E48` | FM2 wrapper substitute | Exact public name absent. FM2 render context binds stream slot, resource, byte offset, stride, and dirty mask. |
| `rex_D3DDevice_SetTexture` | `FM2_RenderContext_SetTextureFetchBitsLow`; `FM2_RenderContext_SetTextureFetchBitsMid`; PM4 draw-list state path | `0x8236EA60`; `0x8236EA90`; `0x82383A70` | FM2 substitute | Exact public texture binding name absent. FM2 uses Xenos fetch constants / draw-list state rather than `SetTexture`. |
| `rex_D3DDevice_SetVertexShader` | `FM2_RenderContext_SetVertexShaderState` | `0x8236E010` | FM2 wrapper substitute | Exact public name absent. FM2 render context stores the vertex shader state and dirty bits. |
| `rex_D3DDevice_SetVertexShaderConstantB` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact boolean constant setter absent. |
| `rex_D3DDevice_SetVertexShaderConstantFN` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact float constant setter absent. |
| `rex_D3DDevice_SetVertexShaderConstantI` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | `0x827307E8`; `0x82730DC0` | grouped substitute | Exact integer constant setter absent. |
| `rex_D3DDevice_SetViewport` | `rex_D3DDevice_SetViewport`; also `FM2_RenderContext_SetViewportModeAndApply` | `0x823715C0`; `0x823715B0` | direct match + FM2 helper | Exact viewport function resolves and forwards six viewport values to `FM2_RenderContext_UploadFloat6Constants`. |
| `rex_D3DDevice_Swap` | `FM2_D3D_TryPresentAndUpdateStatus` | `0x824F83D8` | present caller substitute | Exact swap name absent. FM2 presents via vtable slot `+0x3C` from this function; direct target still unresolved. |
| `rex_D3DIndexBuffer_Lock` | `rex_D3DIndexBuffer_Lock`; FM2 wrapper `FM2_D3D_DeserializeAndLockIndexBuffer` | `0x8236A0B0`; `0x825A27D8` | direct match + wrapper | Exact lock helper resolves in IDA37. |
| `rex_D3DSurface_GetDesc` | `rex_D3DSurface_GetDesc` | `0x8236C0E8` | direct match | Exact surface descriptor helper resolves in IDA37. |
| `rex_D3DSurface_LockRect` | `rex_D3DSurface_LockRect`; higher FM2 path `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | `0x8236C180`; `0x82392090` | direct match + wrapper | Exact lock helper resolves. The FM2 metadata helper wraps `GetDesc` and optional subrect validation. |
| `rex_D3DVertexBuffer_Lock` | `rex_D3DVertexBuffer_Lock`; FM2 wrapper `FM2_D3D_LockVertexBufferWrapper` | `0x82369FA0`; `0x825A2350` | direct match + wrapper | Exact lock helper resolves in IDA37. |
| `rex_D3DXCreateTextureFromFileInMemory` | `rex_D3DXCreateTextureFromFileInMemory`; FM2 title path `FM2_D3D_CreateTextureFromMemoryBuffer` | `0x823883C0`; `0x825A25F8` | direct match + wrapper | Exact D3DX helper resolves in IDA37. |
| `rex_D3DXCreateTextureFromFileInMemoryEx` | `rex_D3DXCreateTextureFromFileInMemoryEx`; FM2 title path `FM2_D3D_CreateTextureFromMemoryBuffer` | `0x82388338`; `0x825A25F8` | direct match + wrapper | Exact D3DX helper resolves in IDA37. |
| `rex_FXePixelShader_Init` | none; use `FM2_Render_GetOrCreatePixelShaderResourceById` / `FM2_Render_InitPixelShaderResource` | `0x825A1608`; `0x8259F6F0` | FM2 substitute | Exact FXe init name absent. FM2 shader objects are title resources, not runtime FXe public creations. |
| `rex_FXeVertexShader_Init` | none; use `FM2_Render_GetOrCreateVertexShaderResourceById` / `FM2_Render_InitVertexShaderResource` | `0x825A16E0`; `0x8259F750` | FM2 substitute | Exact FXe init name absent. |
| `rex_KickOff_CDevice_D3D_QAAPAKXZ` | none found | - | absent / unresolved | Exact name absent. |
| `rex_LockSurface_D3D_YAXPAUD3DBaseTexture_IIKPAPAEPAK22_Z` | `rex_D3DSurface_LockRect` / `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | `0x8236C180`; `0x82392090` | FM2 surface substitute | Exact ReOdyssey helper absent. Use the confirmed FM2 surface path. |
| `rex_RHIDrawIndexedPrimitiveUP_YAXIIIIPBXI0I_Z` | none found | - | ReOdyssey / UE3 only | Exact RHI name absent. No FM2 indexed-UP public draw equivalent was verified. |
| `rex_RHISetDepthState_YAXPAVFD3DDepthState_Z` | `FM2_RenderContext_SetDepthStencilEnableState`; `FM2_RenderContext_SetDepthCompareBits`; `FM2_Render_ComputeDepthBiasFromContext` | `0x8236EAF8`; `0x8236F2D0`; `0x8255D270` | FM2 state substitutes | Exact UE3 RHI name absent. FM2 splits depth behavior across packed context fields and computed bias. |
| `rex_RHISetStencilState` | `FM2_RenderContext_SetStencilOpBits`; `FM2_RenderContext_SetDepthStencilEnableState` | `0x8236F308`; `0x8236EAF8` | FM2 state substitutes | Exact UE3 RHI name absent. |
| `rex_SetPending_ClipPlanes_D3D_YAXPAVCDevice_1_K_Z` | `FM2_RenderContext_SetClipPlane0Enable` through `SetClipPlane3Enable`; generic `rex_SetPending_RenderStates_D3D_YAXPAVCDevice_1_KKPAX_Z` | `0x8236F440`-`0x8236F4A0`; `0x82382928` | FM2 grouped substitute | Exact helper absent. FM2 writes clip-plane enables and emits pending render-state bursts generically. |
| `rex_SynchronizeToPresentationInterval_D3D_YAXPAVCDevice_1_K_Z` | none found | - | absent / unresolved | Exact name absent. Present path is `FM2_D3D_TryPresentAndUpdateStatus`. |
| `rex_UnlockResource_D3D_YAXPAUD3DResource_PAX1_Z` | `?UnlockResource@D3D@@YAXPAUD3DResource@@PAX1@Z`; FM2 wrapper `FM2_D3DResource_UnlockForRelease` | `0x82369C88`; `0x8236A2B8` | direct decorated match + wrapper | Exact ReOdyssey alias absent, but decorated XDK unlock helper resolves. |
| `rex_XGSetVertexDeclaration` | `D3DDevice_CreateVertexDeclaration`; `FM2_D3D_CreateVertexDeclarationFromElements` | `0x8236E240`; `0x8259F008` | FM2 substitute | Exact XG helper name absent. FM2 creates vertex declarations through the D3D helper/wrapper. |
| `rex__0FBoundShaderStateRHIRef_QAA_ABV0_Z` | none found | - | ReOdyssey / UE3 only | Exact RHI bound-shader-state constructor absent. |
| `rex__4FBoundShaderStateRHIRef_QAAAAV0_ABV0_Z` | none found | - | ReOdyssey / UE3 only | Exact RHI bound-shader-state assignment absent. |
| `sub_823C10B0` | invalid address collision: inside `sub_823C0E58` | `0x823C10B0` | invalid | FM2 function at this address is bitstream packing using fields around `+5808/+5812`, not color-write/render state. |
| `sub_823C36D8` | invalid address collision: inside `sub_823C36A8` | `0x823C36D8` | invalid | FM2 function is a tiny byte/pixel unpack helper, not Z-write/render state. |
| `sub_823C58E8` | invalid address collision: inside `zlib_inflate_fast_huffman_decode` | `0x823C58E8` | invalid | FM2 function is zlib/D3DX inflate code, not a render hook. |
| `sub_823C5A20` | invalid address collision: inside `zlib_inflate_fast_huffman_decode` | `0x823C5A20` | invalid | FM2 function is zlib/D3DX inflate code, not a render hook. |
| `sub_823C6308` | invalid address collision: inside `rex_XGOffsetResourceAddress` | `0x823C6308` | invalid | FM2 code offsets a D3D resource address, not cull/raster state. |
| `sub_823C9610` | invalid address collision: inside `sub_823C83E8` | `0x823C9610` | invalid | FM2 target is a huge D3DX/XGRAPHICS-style helper, not a verified render-state hook. |
| `sub_823CB2B8` | invalid address collision: inside `sub_823C83E8` | `0x823CB2B8` | invalid | Same address-collision class as `sub_823C9610`; not a verified render-state hook. |
| `sub_823CCAC0` | invalid address collision: `UntileSurface` | `0x823CCAC0` | invalid | FM2 function untile-copies texture/surface data, not rasterizer state. |

## Implementation Implications

- Keep the exact XDK resource helpers and FM2 title wrappers.
- Remove ReOdyssey raw `sub_823C...` imports from FM2 native-renderer code.
- Do not import UE3/RHI symbols for FM2; use FM2 render-context state functions.
- Treat shader constants as a batched FM2 constant/PM4 path, not individual
  public `D3DDevice_Set*ShaderConstant*` hooks.
- Treat draw and present as FM2-specific boundaries:
  `FM2_Render_DrawIndexedPrimitive` and `FM2_D3D_TryPresentAndUpdateStatus`.
