# FM2 Native Renderer Cross-Repo Gap Analysis

Date: 2026-06-23
Scope: FM2 (ReXGlue080plume) vs ReOdyssey vs UnleashedRecomp
Method: Direct file comparison across all three local repos.

This document records durable structural findings from comparing the native
renderer implementations. It complements `FM2-native-renderer-generator-notes.md`
(which tracks the FM2-internal prototype work) by documenting what is shared
across repos and where FM2 diverges.

## Shared Render Module Lineage (FM2 <-> ReOdyssey)

`FM2/src/render/` and `ReOdyssey/src/render/` share a common ancestor. The
following files are byte-identical except for namespace (`fm2::render` vs
`reodyssey::render`):

- `render/video.h` — identical `Video` struct (Init/Present/WaitForGPU/Shutdown)
- `render/guest_device.h` — identical `GuestDevice` layout (0x5E00 bytes),
  identical enum tables (GuestRenderState, GuestCullMode, GuestBlendMode,
  GuestPrimitiveType, GuestDeclType, GuestDeclUsage, GuestTextureFilterType,
  GuestTextureAddress), identical static_asserts at the same offsets
- `render/render_internal.h` — identical Plume interface accessors
  (Interface/Device/TextureDescriptorSet/SamplerDescriptorSet/PipelineLayout/
  GetBlitPipeline/LoadShader), identical bindless table sizes (65536 textures,
  1024 samplers, 3 null descriptors)
- `render/render_state.h` — identical rr:: API declarations (SetRenderState,
  SetTexture, SetVertexShader, SetPixelShader, SetStreamSource, SetIndices,
  SetViewport, SetScissorRect, SetRenderTarget, SetDepthStencilSurface, Clear,
  StretchRect, DrawPrimitive, DrawIndexedPrimitive, DrawPrimitiveUP,
  DrawIndexedPrimitiveUP)
- `render/render_state.cpp` — PSO creation `CreateGraphicsPipeline` is
  byte-identical at lines 1575-1649 in both repos (both 3471 lines total).
  Shader loading via `LoadShader` at line 1585/1599 in both.
- `render/pipeline.cpp` — byte-identical `LoadShader` at line 298 in both
  (both 382 lines total). Same DXC runtime (dxcompiler.dll), same zstd cache,
  same `generated/shader_cache.h` include.
- `render/render_patches.cpp`, `render/guest_resources.h`, `render/guest_heap.h`
  — shared file set in both repos.

This shared `render/` module IS the generic ReXGlue SDK -> Plume bridge. It is
production-quality code inherited from a common template.

## The Fundamental Gap: Mirror vs Replace

The entire gap between FM2 and ReOdyssey lives in `d3d_hooks.cpp`.

### ReOdyssey: Replace architecture

ReOdyssey hooks are unconditional. The hook IS the implementation. There is no
parallel backend.

```cpp
// ReOdyssey d3d_hooks.cpp line 377
void SetTexture(GuestDevice *device, uint32_t sampler, GuestTexture *texture, ...) {
    rr::SetTexture(device, sampler, reo);  // Plume is the only renderer
}
REX_HOOK(rex_D3DDevice_SetTexture, SetTexture);  // line 1042
```

ReOdyssey hooks ~46 `rex_D3DDevice_*` SDK-universal symbols (lines 1019-1112).
None of these hooks call an "original" function — Plume is the sole renderer.

### FM2: Mirror architecture

FM2 hooks ~30 `FM2_*` title-specific symbols (lines 1835-1884). Every hook
calls the original FM2 function first, then conditionally mirrors into rr::.

```cpp
// FM2 d3d_hooks.cpp line 349
void Fm2SetPixelShaderState(uint32_t renderContext, uint32_t shader) {
    g_origFm2SetPixelShaderState(renderContext, shader);  // ALWAYS: Xenos
    if (!ShouldMirrorPlumeRenderState()) return;          // gate
    SetPixelShaderNative(device, ghp::ToHost<GuestShader>(shader));  // Plume
}
```

The gate chain (d3d_hooks.cpp line 192, fm2_native_renderer.cpp line 1763):
```cpp
bool ShouldMirrorPlumeRenderState() { return !nr::WantsReXGraphics(); }
bool WantsReXGraphics() { return GetMode() != Mode::kPlumeClear; }
```

Mode behavior:
- `kXenos` (default): mirror OFF. Only Xenos runs. Production.
- `kShadow`: mirror OFF. Xenos runs; Plume device init'd for diagnostics.
  The `native_renderer/` overlay captures state for debug replay.
- `kPlumeClear`: mirror ON. Hooks call original AND mirror into rr::. But Xenos
  is disabled at present level; user sees only a Plume clear screen. The rr::
  path receives mirrored state but rendering is not yet visually correct.

## Hook Target Mapping (FM2-specific abstraction layer)

FM2's guest code calls `FM2_RenderContext_*` functions (a render-context
abstraction above D3D), not `rex_D3DDevice_*`. The hooks must resolve
render-context-specific handles to GuestShader/GuestBuffer pointers.

| Operation | ReOdyssey hook | FM2 hook | FM2 rr:: call |
| --- | --- | --- | --- |
| Set pixel shader | `rex_D3DDevice_SetPixelShader` | `FM2_RenderContext_SetPixelShaderState` | `rr::SetPixelShader` |
| Set vertex shader | `rex_D3DDevice_SetVertexShader` | `FM2_RenderContext_SetVertexShaderState` | `rr::SetVertexShader` |
| Bind vertex stream | `rex_D3DDevice_SetStreamSource` | `FM2_RenderContext_BindVertexStream` | `rr::SetStreamSource` |
| Bind index buffer | `rex_D3DDevice_SetIndices` | `FM2_RenderContext_BindIndexBuffer` | `rr::SetIndices` |
| Set render target | `rex_D3DDevice_SetRenderTarget` | `FM2_RenderContext_SetBoundSurface` | `rr::SetRenderTarget` |
| Depth stencil enable | `rex_D3DDevice_SetRenderState_ZEnable` | `FM2_RenderContext_SetDepthStencilEnableState` | `rr::SetRenderState(D3DRS_ZENABLE)` |
| Alpha blend | `rex_D3DDevice_SetRenderState_AlphaBlendEnable` | `FM2_RenderContext_SetAlphaBlendEnableBits` | `rr::SetRenderState(D3DRS_ALPHABLENDENABLE)` |
| Draw indexed | `rex_D3DDevice_DrawIndexedVertices` | `FM2_D3D_EmitIndexedDrawPm4Packets` | `rr::DrawIndexedPrimitive` |
| Present | `rex_D3DDevice_Swap` | `FM2_D3D_TryPresentAndUpdateStatus` | `Video::Present` |

## The native_renderer/ Overlay

`FM2/src/native_renderer/` (6 files) is a diagnostic overlay, NOT the render
path:

- `fm2_native_state.h/cpp` — `NativeStateRecorder`, a parallel state tracker
  fed by FM2_RenderContext_* hooks. Tracks shader bindings, stream bindings,
  surface bindings, viewport, texture fetch, clear, pass boundaries.
- `fm2_direct_draw_decode.h` — decode/hash/replay scaffolding for
  `FM2_Render_BuildDirectIndexedDrawBuffers`. ~1500 lines of constexpr decode
  logic for guest memory layouts, shader ucode hashing, compiled-state table
  parsing.
- `fm2_native_draw.h` — `NativeDrawPacket` / `NativePassCommand` structures for
  batched debug replay. Only supports 2 hardcoded pipeline layouts:
  `kDebugRaw32Side12` and `kNativePosition28Side12`.
- `fm2_native_renderer.h/cpp` — mode management, Plume device init, debug
  replay window, compare window.

All gated by cvars: `fm2_plume_debug_replay`, `fm2_plume_native_direct_draw`,
`fm2_plume_compare_window`. ReOdyssey has no equivalent overlay.

## UnleashedRecomp: Different Toolchain

UnleashedRecomp (`UnleashedRecomp/UnleashedRecomp/gpu/video.cpp`, 7882 lines)
is architecturally separate from both ReXGlue titles:

- Uses `PPCFunc*` table swap via `kernel/function.h` (Xenia-derived XenosRecomp
  toolchain), not `REX_HOOK`.
- Pre-baked DXIL + SPIR-V shader caches compiled offline by XenosRecomp.
- Monolithic single file: ImGui, achievements, MSAA resolve, gamma, motion
  blur, CSD, movie rendering all baked in.
- Not a ReXGlue SDK title. Cannot share code with FM2.

UnleashedRecomp demonstrates the end goal (production native renderer) but its
code is not portable to FM2. The correct "make FM2 similar" target is ReOdyssey.

## Plume: The Shared Host Interface

All three projects use Plume (`plume_render_interface.h`) as the host GPU
abstraction (D3D12/Vulkan/Metal):

- ReXGlue080plume: `plume/` at repo root (plume_d3d12.cpp, plume_vulkan.cpp,
  plume_metal.cpp, plume_apple.mm)
- ReOdyssey: `thirdparty/plume/` (same file set)
- UnleashedRecomp: extern `plume::CreateD3D12Interface` / `CreateVulkanInterface`

Plume is the only piece of true common infrastructure across all three.

## Gap-Closure Path

The path to production native rendering for FM2 is NOT a rewrite. It is a
transition from mirror mode to replace mode, plus validation:

1. Validate shader cache: confirm `generated/shader_cache.h` is populated for
   FM2 guest shaders. PSO creation rejects pipelines where
   `vertexShader->shaderCacheEntry == nullptr` (render_state.cpp line 1581).
2. Audit mirror completeness: verify every `Fm2Set*` / `Fm2Bind*` / `Fm2Emit*`
   hook correctly translates to rr:: calls. Cross-reference against
   ReOdyssey's hook set for missing translations.
3. Add a production Plume mode (e.g. `kPlumeNative`): mirror ON, Xenos OFF at
   present level, rr:: draws produce the visible frame. Validate visual output
   against Xenos.
4. Once rr:: path is proven, remove or debug-gate the `native_renderer/`
   overlay (~5000 lines of diagnostic code that duplicates state tracking).

## Key File Paths

- FM2 d3d_hooks.cpp: `FM2/src/render/d3d_hooks.cpp` (1884 lines)
- FM2 native_renderer: `FM2/src/native_renderer/` (6 files)
- ReOdyssey d3d_hooks.cpp: `ReOdyssey/src/render/d3d_hooks.cpp` (1115 lines)
- Shared render_state.cpp: both repos, 3471 lines, PSO at line 1575
- Shared pipeline.cpp: both repos, 382 lines, LoadShader at line 298
- UnleashedRecomp video.cpp: `UnleashedRecomp/UnleashedRecomp/gpu/video.cpp`
  (7882 lines)
