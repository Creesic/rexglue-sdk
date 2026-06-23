# Native Renderer Architecture Comparison

Compares FM2's debug replay native renderer against UnleashedRecomp and ReOdyssey,
both of which implement full D3D API interception layers on the same Plume RHI.

---

## How each project intercepts the original game's GPU commands

**UnleashedRecomp** hooks the full D3D9 API surface — `DrawPrimitive`,
`DrawIndexedPrimitive`, `CreateVertexShader`, `CreatePixelShader`,
`SetRenderState`, etc. — and reroutes every call through a
`LocalRenderCommandQueue` that is consumed by a GPU worker thread. It owns the
entire rendering pipeline from the guest perspective.

**ReOdyssey** does the same thing but as a ReXGlue title: hook stubs replace the
D3D entry points; each call records into a ring buffer; `Swap()` triggers a
flush.

**ReXGlue/FM2 (`kShadow`/`kPlumeClear` native renderer)** does neither. It does
not replace the D3D API surface. Instead it intercepts at two specific XEX
addresses:

- `FM2PlumeTraceInstanceHybridDrawEntry` at 0x82539650 — fires per-object per-frame
- `FM2PlumeTracePresent` at 0x824F83D8 — fires at present time

The draw hook decodes the raw Xenos PM4 vertex/index data already assembled by
FM2's own rendering code and builds a `replay_plan`. That plan is submitted
through a separate diagnostic pipeline (`RenderDirectDebugReplayBatchLocked`)
that is entirely independent of FM2's draw state.

---

## Shader handling

Both UnleashedRecomp and ReOdyssey use **XenosRecomp** — a tool that reads Xbox
360 microcode and emits pre-translated DXIL/SPIR-V blobs into a `shader_cache`.
At runtime, each draw hashes the guest microcode and looks up the pre-built
native shader. Specialization (alpha test, format quirks) is handled via spec
constants baked into the cache.

FM2's debug replay bypasses this entirely. The `kDebugRaw32Side12` pipeline in
`RenderDirectDebugReplayBatchLocked` uses a **fixed diagnostic shader** — no
guest microcode translation. Tradeoff: geometry is visible without materials,
lighting, or texture-dependent effects. It is a wireframe/untextured debug view,
not a faithful reproduction of the guest's output.

---

## Vertex/index data flow

UnleashedRecomp and ReOdyssey both wrap guest vertex/index buffers
(`GuestBuffer`) and translate vertex declarations (`GuestVertexDeclaration`) to
native input layouts. The original game's vertex data flows through these
wrappers into D3D12/Vulkan buffers with properly described layouts.

FM2's debug replay reads vertex data from **raw guest memory addresses** captured
at hook time — `replay_plan.streams[0].upload_guest_base` and
`replay_plan.streams[1].upload_guest_base`. The strides are hardcoded:
stream0=32 bytes (Xenos vertex data), stream1=12 bytes (side channel). No layout
translation occurs; the diagnostic shader interprets whatever is at those
offsets.

The two `DirectDrawReplayPipelineLayout` variants:
- `kDebugRaw32Side12` — stream0.stride=32, stream1.stride=12 (debug replay path)
- `kNativePosition28Side12` — stride (28, 12) + valid native state (compare/native path)

---

## Batching and present

| | UnleashedRecomp | ReOdyssey | FM2 debug replay |
|---|---|---|---|
| **Command recording** | Enum-based deferred queue → GPU worker | Direct Plume `RenderCommandList` | `RenderDirectDebugReplayBatchLocked` per decode call |
| **When submitted** | GPU worker thread consumes queue | `FlushRenderState()` on draw, `Present()` on swap | `SubmitDirectDebugReplayBatchForReplayWindow` at end of each `MaybeLogPlumeDirectIndexedDrawDecode` call |
| **Per-frame scope** | Full frame, all draw calls | Full frame, all draw calls | **Per-object** (one present per decode call) |
| **Present mechanism** | Implicit swapchain | Explicit fullscreen blit | `RenderDirectDebugReplayBatchLocked` → acquire → draw → present per call |

This is the key structural difference: UnleashedRecomp and ReOdyssey accumulate
a full frame's worth of draw calls before presenting. FM2's debug replay
presents once per object decode call. The result is rapid cycling through
objects rather than a composited scene — the current architecture has no
frame-level accumulator.

---

## Texture binding model

**UnleashedRecomp**: per-draw descriptor update; textures bound by slot.

**ReOdyssey**: bindless descriptor heap with a dynamic grow-on-demand pool; null
texture sentinels at indices 0–2 (2D/3D/cube); sampler states cached by hash
and updated per-frame.

**FM2 debug replay**: no texture binding at all. The diagnostic pipeline renders
geometry only.

---

## Goal: Replace kXenos with full D3D API interception (ReOdyssey approach)

FM2's `kXenos` mode runs the full Xenos GPU command processor emulator. The
proposed goal was to bypass that entirely and intercept at the D3D9 API layer —
the same approach ReOdyssey uses — so FM2 renders through Plume at native speed
without emulating the command processor.

### Phase 1 — IDA investigation: FM2 hook address survey

**Conducted 2026-06-22 via IDA MCP against `default.xex.i64` (ida37) and
`LostOdyssey/default.xex.i64` (ida38). Method: size match + first-10-instruction
disassembly comparison between LO (ReOdyssey target) and FM2.**

#### FM2's rendering architecture vs. Lost Odyssey

FM2's engine (Turn 10, custom) is architecturally different from Lost Odyssey
(which uses UE3). The key difference is that **FM2's game rendering code does not
call `D3DDevice_DrawIndexedVertices` or `D3DDevice_DrawVertices`**. Instead, it
calls its own internal PM4 packet emitters directly:

| Function | Address | Role |
|---|---|---|
| `FM2_D3D_EmitIndexedDrawPm4Packets` | 0x827313B0 | base indexed draw PM4 emit |
| `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset` | 0x827317A0 | indexed draw with GPU address offset |
| `FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup` | 0x82731C00 | indexed draw + vertex format re-emit |

`FM2_Render_DrawIndexedPrimitive` (0x827221F0) calls the GPU offset emitter
directly — bypassing the public D3D9 draw API. `D3DDevice_Swap` is also absent
as a named function; the present path goes through a vtable slot called from
`FM2_D3D_TryPresentAndUpdateStatus` (0x824F83D8).

**However**, FM2 does call D3D9 API functions for resource management and render
state. Many of these use FM2-specific wrapper names (`FM2_RenderContext_*`)
rather than the raw XDK names that Lost Odyssey exposes. Instruction comparison
confirms they are the same XDK functions at different addresses.

#### Confirmed FM2 addresses for ReOdyssey hook equivalents

Verification method for each: save-register range match + first 8–10
instructions byte-for-byte identical (modulo struct field offsets, which differ
between XDK versions/builds).

**Resource creation — identical D3D9 API, hookable at same level as ReOdyssey:**

| ReOdyssey hook name | FM2 name in IDB | FM2 address | FM2 size | Verified |
|---|---|---|---|---|
| `rex_D3DDevice_CreateVertexBuffer` | `D3DDevice_CreateVertexBuffer` | 0x82369ED8 | 0xC8 | size+range ✓ |
| `rex_D3DDevice_CreateIndexBuffer` | `rex_D3DDevice_CreateIndexBuffer` | 0x8236A000 | 0xAC | size+range ✓ |
| `rex_D3DDevice_CreateTexture` | `rex_D3DDevice_CreateTexture` | 0x8236BEA0 | 0x120 | size+range ✓ |
| `rex_D3DDevice_CreateSurface` | `D3DDevice_CreateSurface` | 0x8236BFC0 | 0x128 | size match |
| `rex_D3DDevice_CreateVertexDeclaration` | `D3DDevice_CreateVertexDeclaration` | 0x8236E240 | 0xE0 | size match |
| `rex_D3DVertexBuffer_Lock` | `rex_D3DVertexBuffer_Lock` | 0x82369FA0 | 0x50 | size+range ✓ |
| `rex_D3DIndexBuffer_Lock` | `rex_D3DIndexBuffer_Lock` | 0x8236A0B0 | 0x48 | size+range ✓ |
| `rex_D3DSurface_LockRect` | `rex_D3DSurface_LockRect` | 0x8236C180 | 0x20 | size+range ✓ |
| `rex_D3DBaseTexture_LockTail` | exact name absent; not a Phase 1 hook | — | 0xC0 | absent |
| `rex_LockSurface_D3D_*` | `rex_D3DSurface_LockRect` / `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | 0x8236C180 / 0x82392090 | 0x20 / 0x4F0 | FM2 substitute |
| `rex_UnlockResource_D3D_*` | `?UnlockResource@D3D@@YAXPAUD3DResource@@PAX1@Z` | 0x82369C88 | 0x104 | name match |
| `rex_D3DSurface_GetDesc` | `rex_D3DSurface_GetDesc` | 0x8236C0E8 | 0x98 | size+range ✓ |
| `rex_D3DDevice_Release` | `rex_D3DDevice_Release` | 0x82369418 | 0x54 | size+range ✓ |
| `rex_XGSetVertexDeclaration` | `D3DDevice_CreateVertexDeclaration` / `FM2_D3D_CreateVertexDeclarationFromElements` | 0x8236E240 / 0x8259F008 | 0xE0 / wrapper | FM2 substitute |
| `rex_XGSetTextureHeaderEx` | `rex_XGSetTextureHeaderEx` | 0x823C6070 | 0x8C | name match |
| `rex_XGSetVertexBufferHeader` | `XGSetVertexBufferHeader` | 0x823C5C90 | 0x94 | name match |
| `rex_XGSetIndexBufferHeader` | `rex_XGSetIndexBufferHeader` | 0x823C5D28 | 0x90 | name match |
| `rex_D3DXCreateTextureFromFileInMemory` | `rex_D3DXCreateTextureFromFileInMemory` | 0x823883C0 | 0x60 | name match |
| `rex_D3DXCreateTextureFromFileInMemoryEx` | `rex_D3DXCreateTextureFromFileInMemoryEx` | 0x82388338 | 0x84 | name match |

**Render state — FM2 wraps these in `FM2_RenderContext_*` functions; same XDK code,
different names; hookable at this wrapper level:**

| ReOdyssey hook name | FM2 wrapper | FM2 address | FM2 size | Verified |
|---|---|---|---|---|
| `rex_D3DDevice_SetVertexShader` | `FM2_RenderContext_SetVertexShaderState` | 0x8236E010 | 0x1CC | **instruction match ✓** |
| `rex_D3DDevice_SetPixelShader` | `FM2_RenderContext_SetPixelShaderState` | 0x8236DD10 | 0x1BC | instruction match ✓ |
| `rex_D3DDevice_SetStreamSource` | `FM2_RenderContext_BindVertexStream` | 0x82370E48 | 0x11C | instruction match ✓ |
| `rex_D3DDevice_SetIndices` | `FM2_RenderContext_BindIndexBuffer` | 0x82370F68 | 0x90 | instruction match ✓ |
| `rex_D3DDevice_SetViewport` | `rex_D3DDevice_SetViewport` | 0x823715C0 | 0x7C | name+size ✓ |
| `rex_D3DDevice_SetTexture` | not present as D3D API call; FM2 uses Xenos fetch constants via `FM2_RenderContext_SetTextureFetchBitsLow/Mid` | — | — | absent |
| `rex_D3DDevice_SetRenderTarget` | `FM2_RenderContext_BindSurfaceInternal` (wraps both RT+DSS) | 0x823716F8 | 0x334 | structural match |
| `rex_D3DDevice_SetDepthStencilSurface` | `FM2_RenderContext_SetBoundSurface` | 0x82371A30 | 0x2EC | structural match |
| `rex_D3DDevice_SetScissorRect` | `FM2_D3D_EmitScissorRegionPackets` (PM4 level) | 0x8236E780 | 0x2E0 | name ✓ |
| `rex_D3DDevice_ClearF` | `FM2_Render_SetClearFlagsAndDirtyBit` + PM4 path | 0x8236EF88 | 0x1C | structural equivalent |
| `rex_D3DDevice_Resolve` | `FM2_D3D_EmitSurfaceResolvePackets` | 0x82382590 | 0x394 | name ✓ |
| Various `SetRenderState_*` | `FM2_RenderContext_SetAlphaBlendEnableBits`, `SetCullEnableState`, `SetAlphaTestState`, `SetDepthCompareBits`, etc. | 0x8236F1F0–0x8236F308 | 0x1C–0x38 | structural match |
| `rex_D3DDevice_SetVertexShaderConstantFN` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | 0x827307E8 / 0x82730DC0 | grouped PM4 path | FM2 substitute |
| `rex_D3DDevice_SetPixelShaderConstantFN` | `FM2_ConstantBuffer_UploadVector4Block` / `FM2_D3D_EmitShaderConstantsBatch` | 0x827307E8 / 0x82730DC0 | grouped PM4 path | FM2 substitute |
| `rex_D3DDevice_SetShaderGPRAllocation` | exact name absent; no useful FM2 native-render hook located | — | 0xE8 | absent |
| `rex_D3DDevice_SetPredication` | exact name absent; no useful FM2 native-render hook located | — | 0x16C | absent |

**Shader creation — ABSENT in FM2.** FM2 does not call `D3DDevice_CreateVertexShader`,
`D3DDevice_CreatePixelShader`, `FXeVertexShader_Init`, or `FXePixelShader_Init` at
runtime. Shaders are compiled offline and loaded through FM2's own resource system
(`FM2_Render_GetOrCreateVertexShaderResourceById` etc.). These ReOdyssey hooks have
no FM2 equivalent.

**Draw calls and present — ABSENT as D3D9 API.** FM2 emits draw calls directly as
PM4 packets:

| ReOdyssey hook | FM2 equivalent | FM2 address |
|---|---|---|
| `rex_D3DDevice_DrawIndexedVertices` | `FM2_Render_DrawIndexedPrimitive` (bypasses D3D API entirely) | 0x827221F0 |
| `rex_D3DDevice_DrawVertices` | no equivalent found | — |
| `rex_D3DDevice_Swap` | `FM2_D3D_TryPresentAndUpdateStatus`; concrete swap target is a runtime vtable slot `+0x3C`, so hook the caller | 0x824F83D8 (caller) |
| `rex_D3DDevice_BlockUntilIdle` | exact name absent; no Phase 1 product hook | — |
| `rex_BlockOnFence_CDevice_D3D_*` | `?BlockOnSecondaryPosition@CDevice@D3D@@QAAXPAKK@Z` | 0x82371D60 |

#### Phase 1 continuation findings: current FM2 hook import triage

**Conducted 2026-06-22 via IDA MCP against FM2 `default.xex.i64`.**
Purpose: check the ReOdyssey-style `REX_IMPORT` names currently present in
`FM2/src/render/d3d_hooks.cpp` before wiring more of the native renderer around
them.

Full per-symbol mapping for all 76 ReOdyssey hook symbols lives in
`docs/FM2-reodyssey-hook-equivalence-map.md`.

The important correction is that raw ReOdyssey `sub_823C...` labels are not
portable between games. In FM2 they land in XGRAPHICS, D3DX/zlib, or texture
untile helpers, not UE3/RHI render-state setters.

| Current imported name in FM2 hook code | IDA result in FM2 | Phase 1 decision |
|---|---|---|
| `__imp__sub_823C10B0` | inside `sub_823C0E58`; bitstream packing fields at `+5808/+5812` | invalid for color-write state |
| `__imp__sub_823C36D8` | inside `sub_823C36A8`; tiny byte/pixel unpack helper | invalid for Z-write state |
| `__imp__sub_823C6308` | inside `rex_XGOffsetResourceAddress` at function start `0x823C62B8` | invalid for cull mode |
| `__imp__sub_823CCAC0` | `UntileSurface(...)` | invalid for rasterizer state |
| `__imp__rex_RHISetDepthState_*` | name absent in FM2 IDB | UE3/ReOdyssey-specific; no direct FM2 equivalent |
| `__imp__rex_RHISetStencilState` | name absent in FM2 IDB | UE3/ReOdyssey-specific; no direct FM2 equivalent |
| `__imp__rex_D3DDevice_SetScissorRect` | name absent in FM2 IDB | use FM2 PM4/scissor path, not public API |
| `__imp__rex_D3DDevice_SetRenderState_ClipPlaneEnable` | name absent in FM2 IDB | use FM2 render-context clip-plane setters |
| `__imp__rex_D3DDevice_SetRenderState_ViewportEnable` | name absent in FM2 IDB | use FM2 render-context state, not ReOdyssey import |
| `__imp__rex_SetPending_ClipPlanes_*` | name absent in FM2 IDB | generic pending render-state emitter exists, but not this helper |
| `__imp__rex_XGSetVertexDeclaration` | name absent in FM2 IDB | use `FM2_D3D_CreateVertexDeclarationFromElements` / `D3DDevice_CreateVertexDeclaration` |
| `__imp__rex_FXeVertexShader_Init` / `__imp__rex_FXePixelShader_Init` | names absent in FM2 IDB | use FM2 shader-resource load path |
| `__imp__rex_D3DBaseTexture_LockTail` | name absent in FM2 IDB | do not import by ReOdyssey name; locate FM2 texture lock path separately |
| `__imp__rex_LockSurface_D3D_*` | name absent in FM2 IDB | use confirmed FM2 surface lock/metadata path instead |

Confirmed FM2-specific state anchors from this pass:

| FM2 function | Address | Observed behavior |
|---|---:|---|
| `rex_D3DDevice_SetViewport` | `0x823715C0` | reads a D3D viewport and forwards six values to `FM2_RenderContext_UploadFloat6Constants` |
| `FM2_RenderContext_SetAlphaBlendEnableBits` | `0x8236F1F0` | updates packed render-state bits and marks dirty `0x800/0x20000` |
| `FM2_RenderContext_SetCullEnableState` | `0x8236F228` | writes cull enable at `ctx+11600`, mirrors bit 0 into `ctx+10420`, marks dirty |
| `FM2_RenderContext_SetAlphaTestState` | `0x8236F268` | updates alpha-test field in packed state |
| `FM2_RenderContext_SetBlendModeBits` | `0x8236F2A0` | updates blend mode field in packed state |
| `FM2_RenderContext_SetDepthCompareBits` | `0x8236F2D0` | updates depth compare field in packed state |
| `FM2_RenderContext_SetStencilOpBits` | `0x8236F308` | updates stencil operation field in packed state |
| `FM2_RenderContext_SetColorWriteMaskBits` | `0x8236F340` | writes bits 14-16 of `ctx+10420`, marks dirty `0x800` |
| `FM2_RenderContext_SetPolygonModeBits` | `0x8236F370` | updates polygon-mode field in packed state |
| `FM2_RenderContext_SetMiscStateBitsA` | `0x8236F410` | updates upper misc state bits in packed state |
| `FM2_RenderContext_SetClipPlane0Enable` | `0x8236F440` | writes clip-plane enable byte at `ctx+10371` |
| `FM2_RenderContext_SetClipPlane1Enable` | `0x8236F460` | writes clip-plane enable byte at `ctx+10370` |
| `FM2_RenderContext_SetClipPlane2Enable` | `0x8236F480` | writes clip-plane enable byte at `ctx+10369` |
| `FM2_RenderContext_SetClipPlane3Enable` | `0x8236F4A0` | writes clip-plane enable byte at `ctx+10367` |
| `FM2_RenderContext_SetDepthStencilEnableState` | `0x8236EAF8` | toggles depth/stencil enable, mirrors packed state to `ctx+10424/+10456/+10460/+10464`, marks dirty bits `0x400/4/2/1` |
| `rex_SetPending_RenderStates_D3D_YAXPAVCDevice_1_KKPAX_Z` | `0x82382928` | generic PM4 TYPE-0 register burst emitter for dirty shadow dwords |
| `FM2_ConstantBuffer_UploadVector4Block` | `0x827307E8` | VMX128 upload of vector constant blocks |
| `FM2_D3D_EmitShaderConstantsBatch` | `0x82730DC0` | emits pending state and shader constant PM4 packets; calls draw-list state and surface resolve emitters when dirty bits require it |

Confirmed FM2 resource/synchronization anchors from this pass:

| FM2 function | Address | Observed behavior |
|---|---:|---|
| `rex_D3DSurface_LockRect` | `0x8236C180` | thin XDK surface lock wrapper; called by FM2 surface upload/texture-create paths |
| `rex_D3DSurface_GetDesc` | `0x8236C0E8` | fills `D3DSURFACE_DESC`, including texture-backed surface handling |
| `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | `0x82392090` | unlocks prior lock-surface object, calls `rex_D3DSurface_GetDesc`, validates optional subrect, and later reaches `D3DSurface_LockRect`; this is the safer FM2 hook surface for texture-from-surface capture |
| `D3D::CDevice::BlockOnSecondaryPosition` | `0x82371D60` | waits on secondary command buffer cursor with `D3D::CBlocker(D3DBLOCKTYPE_SECONDARY_OVERRUN)`; synchronization helper, not a render-state or present hook |

Closed Phase 1 unresolved items:

| ReOdyssey hook | FM2 status |
|---|---|
| `rex_D3DDevice_BlockUntilIdle` | exact name absent in FM2 IDB; no Phase 1 product hook |
| `rex_BlockOnFence_CDevice_D3D_*` | exact ReOdyssey alias absent; FM2 has `D3D::CDevice::BlockOnSecondaryPosition` at `0x82371D60`, but it waits a different secondary-cursor condition and is not a substitute for native rendering |
| `rex_KickOff_CDevice_D3D_*` | exact name absent; FM2 uses its own command-buffer batch/finalize and present paths |
| `rex_D3DDevice_SetPredication` | exact name absent; no useful FM2 native-render hook located |
| `rex_D3DDevice_SetShaderGPRAllocation` | exact name absent; no useful FM2 native-render hook located |

Immediate code implication: do not build the FM2 native renderer around
ReOdyssey raw imports or UE3 `RHI*` hooks. The FM2 pass should use the named
`FM2_RenderContext_*` wrappers above for semantic render state, and the PM4
emit/draw functions only as FM2-specific draw submission boundaries.

#### Phase 1 closeout: manifest/code hook surface

Conducted 2026-06-23 against FM2 `default.xex.i64` via IDA MCP.

Phase 1 is now closed for the ReOdyssey hook-equivalence survey:

- All ReOdyssey hook symbols in
  `docs/FM2-reodyssey-hook-equivalence-map.md` are classified as direct FM2
  XDK helpers, FM2 title-wrapper substitutes, PM4 substitutes, grouped/partial
  substitutes, or absent/not useful for FM2.
- Verified FM2 render-context packed-state helpers are now in
  `FM2/fm2_manifest.toml`, so regenerated code exposes stable names instead of
  `sub_8236EAF8`, `sub_8236F1F0`, `sub_8236F268`, `sub_8236F2D0`,
  `sub_8236F340`, and `sub_8236F440` through `sub_8236F4A0`.
- The remaining active helper imports used by `d3d_hooks.cpp` are also named in
  the manifest: `FM2_D3DVertexBuffer_Lock` at `0x82369FA0` and
  `FM2_D3DSurface_GetDesc` at `0x8236C0E8`.
- `FM2/src/render/d3d_hooks.cpp` now hooks only verified FM2 call-through
  boundaries. Each hook calls the generated FM2 body first through `__imp__*`,
  then mirrors the defensible one-to-one state into Plume.
- Regenerated code no longer exposes `sub_82369FA0` or `sub_8236C0E8` in the
  active FM2 generated/hook surface; callers use the stable manifest names.
- The present slot target under `FM2_D3D_TryPresentAndUpdateStatus` remains a
  runtime vtable dispatch. Static analysis shows it calls vtable slot `+0x3C`
  on the object at `present_chain + 0x18`; `FM2_D3D_TryPresentAndUpdateStatus`
  remains the stable Phase 1 present boundary.

Verification:

- `cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen`
  completed from `FM2/`.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`.
- Root `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  729 assertions in 61 test cases.

#### Summary: what differs from the ReOdyssey model

A full ReOdyssey-style port to FM2 is viable for resource management and most
render state. The key differences:

1. **Draw calls**: Must intercept at `FM2_Render_DrawIndexedPrimitive` or the
   PM4 emitters, not at `D3DDevice_DrawIndexedVertices` (which doesn't exist in FM2).
2. **Shader creation**: Must intercept FM2's resource loading path
   (`FM2_Render_GetOrCreate*ShaderResourceById`), not `D3DDevice_CreateVertexShader`.
3. **Texture state**: Must intercept Xenos fetch constant writes
   (`FM2_RenderContext_SetTextureFetchBitsLow/Mid`), not `D3DDevice_SetTexture`.
4. **Present/Swap**: Must either resolve the vtable slot or hook
   `FM2_D3D_TryPresentAndUpdateStatus` directly.
5. **Wrapper names**: FM2's state-setting functions are named `FM2_RenderContext_*`
   rather than `rex_D3DDevice_*`; the manifest entries and REX_HOOK registrations
   must use FM2's names at FM2's addresses.

### Phase 2 — XenosRecomp shader cache

Run the XenosRecomp tool (vendored in `ReOdyssey/XenosRecomp/`) against FM2's
`default.xex` to extract all vertex/pixel shader microcode and emit a
pre-translated DXIL/SPIR-V `shader_cache`. This cache is compiled into the FM2
binary as a build step, replacing runtime shader translation.

Alternative: adapt `src/graphics/`'s existing runtime shader translator to
produce the same format. Either way the output is: a map from microcode hash →
native shader blob, with spec-constant variants for alpha test etc.

### Phase 3 — Render layer

Port/adapt ReOdyssey's render layer to FM2:

- `GuestBuffer`, `GuestShader`, `GuestTexture`, `GuestVertexDeclaration`,
  `GuestSurface` — host-side wrappers for guest D3D resources
- Per-draw render state machine: `FlushRenderState()` collects dirty state,
  selects/creates pipeline, binds descriptor sets, emits draw
- Pipeline cache keyed on 64-bit hash of (shaders × blend × depth × culling ×
  spec constants)
- Bindless texture descriptor heap (grow-on-demand, null sentinels for 2D/3D/cube)
- `video.cpp` equivalent: device setup, swapchain, copy queue, present path

### Phase 4 — Integration and title-specific debugging

- Set `WantsReXGraphics()` → false unconditionally (disable Xenos emulator)
- Remove `kXenos` fast-path from `fm2_native_renderer.cpp`
- Wire hooks into `fm2_hooks.cpp` via `REX_HOOK` registrations
- FM2-specific rendering: Forza uses cube maps, vehicle paint shaders, tire
  deformation, motion blur, HDR — expect per-feature debugging after the
  foundation works

**Estimated scope:** Phase 1 alone is 4–8 weeks of IDA sessions. Full
implementation is 4–6 months of focused work. ReOdyssey reached its current
state on a rendering-simpler game over a similar timeframe.

---

## What FM2 could borrow for frame-level compositing (debug replay)

The pattern both other projects use for cross-object compositing:

1. Defer all draw submissions into a per-frame list as objects are processed.
2. Submit the entire list once at the frame boundary (their `Present()`/`Swap()`
   hook).

The equivalent in FM2 would be:

- Accumulate `debug_replay_submissions` across multiple
  `MaybeLogPlumeDirectIndexedDrawDecode` calls (across multiple objects) into a
  global per-frame list.
- Submit the entire list once at a reliable frame-end hook.

**Blocker**: `FM2PlumeTracePresent` (0x824F83D8) fires *before* draw hooks in
FM2's frame cycle, leaving the pending list empty when the flush runs. The more
promising candidate is a hook that fires **after** the last draw pass for the
frame — somewhere in FM2's render pass completion path, at a different address
than 0x824F83D8.

The per-decode-call batch (`SubmitDirectDebugReplayBatchForReplayWindow`) is the
current working approach: it produces one present per object rather than one per
record, which eliminates the "glob that appears and vanishes" problem while
compositing across objects remains future work.
