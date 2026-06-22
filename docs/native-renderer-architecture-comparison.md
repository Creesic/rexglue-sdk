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
| `rex_D3DBaseTexture_LockTail` | not yet located | — | 0xC0 | — |
| `rex_LockSurface_D3D_*` | not yet located | — | 0xC4 | — |
| `rex_UnlockResource_D3D_*` | `?UnlockResource@D3D@@YAXPAUD3DResource@@PAX1@Z` | 0x82369C88 | 0x104 | name match |
| `rex_D3DSurface_GetDesc` | `rex_D3DSurface_GetDesc` | 0x8236C0E8 | 0x98 | size+range ✓ |
| `rex_D3DDevice_Release` | `rex_D3DDevice_Release` | 0x82369418 | 0x54 | size+range ✓ |
| `rex_XGSetVertexDeclaration` | not yet located | — | 0xEC | — |
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
| `rex_D3DDevice_SetVertexShaderConstantFN` | not yet verified | — | 0xE8 | — |
| `rex_D3DDevice_SetPixelShaderConstantFN` | not yet verified | — | 0xE8 | — |
| `rex_D3DDevice_SetShaderGPRAllocation` | not yet verified | — | 0xE8 | — |
| `rex_D3DDevice_SetPredication` | not yet verified | — | 0x16C | — |

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
| `rex_D3DDevice_Swap` | called via vtable slot 0x3C from `FM2_D3D_TryPresentAndUpdateStatus`; target address not yet resolved | 0x824F83D8 (caller) |
| `rex_D3DDevice_BlockUntilIdle` | not yet verified | — |
| `rex_BlockOnFence_CDevice_D3D_*` | `?BlockOnSecondaryPosition@CDevice@D3D@@QAAXPAKK@Z` | 0x82371D60 |

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
