# How ReOdyssey replaces Xenos emulation with a native renderer

A reference for porting the same approach to other titles (e.g. DOAX). Based on
the `ReOdyssey` project (`C:\Users\Tera\Documents\GitHub\ReOdyssey`), a ReXGlue
recompilation of *Lost Odyssey*.

## TL;DR

ReOdyssey does **not** run the game's graphics through ReXGlue's emulated Xenos
GPU (the command processor / PM4 packet translation / EDRAM path used by FM2's
`plume_native` work). Instead it:

1. **Turns the emulated GPU off entirely** (`config.graphics.reset()`).
2. Stands up its **own** plume-based D3D12/Vulkan renderer (`Video::Init`).
3. **Hooks the game's guest D3D9-on-360 (`D3DDevice_*`) and UE3 RHI entry
   points** so each guest draw/state/resource call is reimplemented natively in
   C++ against plume, bypassing the Xenos layer completely.

This is the same architecture as Sonic Unleashed Recomp / the "Recomp"-family
ports: the game thinks it is calling the 360 D3D API, but every one of those
calls is intercepted and serviced by a hand-written native renderer.

Contrast with FM2's `plume_native`, which still feeds the *emulated* Xenos
command processor and tries to make the GPU emulation render. ReOdyssey skips the
emulation and re-implements the API surface.

---

## 1. The architectural switch (disable emulated GPU, own presentation)

`src/reodyssey_app.h`:

```cpp
void OnPreSetup(rex::RuntimeConfig& config) override {
  config.input_factory = reodyssey::CreateInputSystem;
  // Disable the rex emulated GPU; the native Plume renderer owns presentation.
  // rex sets config.graphics just before this hook, so resetting here wins.
  config.graphics.reset();
}

// Window exists by now (created in SetupPresentation); stand up the native
// renderer on its HWND before the guest starts issuing D3D calls.
void OnPreLaunchModule() override {
  if (auto* w = window()) {
    Video::Init(w->GetNativeWindowHandle(), 1280, 720);
  }
}

void OnShutdown() override { Video::Shutdown(); }
```

Two key lifecycle overrides on `rex::ReXApp`:

- **`OnPreSetup`** → `config.graphics.reset()` removes the SDK's emulated GPU
  subsystem. Nothing in the Xenos translation layer is created or run. The
  comment notes timing matters: rex assigns `config.graphics` immediately
  before this hook, so resetting here is what actually disables it.
- **`OnPreLaunchModule`** → `Video::Init(hwnd, 1280, 720)` creates the native
  plume device/swapchain on the real window **before** the guest issues its
  first D3D call.

`Video` (`src/render/video.cpp`) owns the actual presentation device — it is the
native renderer's backbone (see §4).

---

## 2. The hook mechanism (how a guest API call becomes native C++)

Three layers cooperate. Understanding this is the whole point if you want to
replicate it.

### 2a. Manifest/config names the guest function

`reodyssey_config.toml` maps the XEX address of each D3D entry point to a stable
symbol name:

```toml
0x823BA578 = { name = "rex_D3DDevice_SetTexture" }
0x823C6860 = { name = "rex_D3DDevice_DrawIndexedVertices" }
0x823F2428 = { name = "rex_FXeVertexShader_Init" }
0x827AC7B8 = { name = "rex_D3DDevice_CreateTexture" }
0x827B4E50 = { name = "rex_D3DDevice_Swap" }
```

When codegen runs, each named address produces a recompiled C++ function whose
linker symbol is that name (the `rex_*` / `sub_*` form). These live in
`generated/reodyssey_recomp.*.cpp`.

### 2b. `REX_HOOK` replaces the generated body with native code

`include/rex/hook.h`:

```cpp
#define REX_HOOK(subroutine, function)                  \
  extern "C" REX_FUNC(subroutine) {                     \
    rex::ppc::HostToGuestFunction<function>(ctx, base); \
  }
```

`REX_HOOK(rex_D3DDevice_SetTexture, SetTexture)` **defines** the recompiled
function symbol itself, so the title's definition wins at link time over the
generated one. `HostToGuestFunction<>` auto-marshals PPC registers
(`ctx`/`base`) into the native C++ signature, so the native function can take
plain types (`GuestDevice*`, `uint32_t`, pointers) and ignore the PPC ABI.

Net effect: when the game calls `D3DDevice::SetTexture`, control lands in
ReOdyssey's native `SetTexture()` instead of an emulated-GPU path.

### 2c. `REX_IMPORT` calls back into the *original* recompiled function

Some hooks still need the guest's own bookkeeping to run (e.g. to update the
guest device struct), then layer native behavior on top:

```cpp
REX_IMPORT(__imp__rex_RHISetDepthState_YAXPAVFD3DDepthState_Z,
           g_origRHISetDepthState, void(rr::GuestDevice *, void *));

void RHISetDepthState(GuestDevice *device, void *depthStateGuest) {
  FlushImmediateVertices();
  g_origRHISetDepthState(device, depthStateGuest); // run the guest's original
  // ...then read the now-populated state and mirror it into the native renderer
  rr::SetDepthState(zEnable, zWrite, cmpFunc);
}
```

`REX_IMPORT(symbol, callable, sig)` gives a typed callable that invokes the
original recompiled function (`__imp__...` symbol) with an isolated PPC context.
This is the "call original, then extend" pattern.

### 2d. Mid-asm hooks for in-function patches

`[[midasm_hook]]` entries in `reodyssey_config.toml` inject a C++ callback at a
specific instruction address (not a whole-function replace). Used for render
*tweaks/patches* rather than the renderer itself — see §6.

**Where the hooks live:** every `REX_HOOK`/`REX_IMPORT` for the renderer is in
**`src/render/d3d_hooks.cpp`** (the single hook-registration translation unit,
~95 hook/import macros). The other render TUs (`render_state.cpp`,
`d3d_resource_hooks.cpp`, `pipeline.cpp`, `video.cpp`) contain the native
implementation those hooks call into.

---

## 3. The full list of hooked guest functions

All from `src/render/d3d_hooks.cpp` (`REX_HOOK(guest_symbol, native_fn)`).

### Resource creation
| Guest function | Native handler | Purpose |
|---|---|---|
| `rex_D3DDevice_CreateVertexBuffer` | `CreateVertexBuffer` | native VB |
| `rex_D3DDevice_CreateIndexBuffer` | `CreateIndexBuffer` | native IB |
| `rex_D3DDevice_CreateTexture` | `CreateTexture` | native texture |
| `rex_D3DDevice_CreateSurface` | `CreateSurface` | native RT/surface |
| `rex_D3DDevice_CreateVertexDeclaration` | `CreateVertexDeclaration` | input layout |
| `rex_XGSetVertexDeclaration` | `XGSetVertexDeclaration` | registers decl alias (calls original first) |
| `rex_D3DDevice_CreateVertexShader` | `CreateVertexShader` | translate VS microcode |
| `rex_D3DDevice_CreatePixelShader` | `CreatePixelShader` | translate PS microcode |
| `rex_FXeVertexShader_Init` | `FXeVertexShaderInit` | UE3 FXe VS → alias (IMPORT original) |
| `rex_FXePixelShader_Init` | `FXePixelShaderInit` | UE3 FXe PS → alias (IMPORT original) |
| `rex_D3DXCreateTextureFromFileInMemory` | `D3DXCreateTextureFromFileInMemory` | decode image → native texture |
| `rex_D3DXCreateTextureFromFileInMemoryEx` | `D3DXCreateTextureFromFileInMemoryEx` | same, extended params |

### Resource lock / unlock
| Guest function | Native handler |
|---|---|
| `rex_D3DVertexBuffer_Lock` | `VertexBufferLock` |
| `rex_D3DIndexBuffer_Lock` | `IndexBufferLock` |
| `rex_D3DSurface_LockRect` | `SurfaceLockRect` |
| `rex_D3DBaseTexture_LockTail` | `BaseTextureLockTail` |
| `rex_D3DSurface_GetDesc` | `SurfaceGetDesc` |
| `rex_LockSurface_D3D_...` | `LockSurface` |
| `rex_UnlockResource_D3D_...` | `UnlockResourceHook` |

These all use `IsReoResource()` to branch: native (reo) resources are serviced
natively; genuine guest resources fall through to the original
(`g_origVertexBufferLock`, etc.) via `REX_IMPORT`.

### Shader / pipeline state
| Guest function | Native handler |
|---|---|
| `rex_D3DDevice_SetTexture` | `SetTexture` |
| `rex_D3DDevice_SetVertexShader` | `SetVertexShader` |
| `rex_D3DDevice_SetPixelShader` | `SetPixelShader` |
| `rex_D3DDevice_SetVertexShaderConstantFN` | `SetVertexShaderConstantFN` |
| `rex_D3DDevice_SetPixelShaderConstantFN` | `SetPixelShaderConstantFN` |
| `rex_D3DDevice_SetVertexShaderConstantB/I` | `SetVertexShaderConstantB/I` |
| `rex_D3DDevice_SetPixelShaderConstantB/I` | `SetPixelShaderConstantB/I` |
| `rex_D3DDevice_SetStreamSource` | `SetStreamSource` |
| `rex_D3DDevice_SetIndices` | `SetIndices` |
| `rex_D3DDevice_SetViewport` | `SetViewport` |
| `rex_D3DDevice_SetScissorRect` | `SetScissorRect` (IMPORT original + native) |
| `rex_D3DDevice_SetRenderTarget` | `SetRenderTarget` |
| `rex_D3DDevice_SetDepthStencilSurface` | `SetDepthStencilSurface` |
| `rex_RHISetDepthState_...` | `RHISetDepthState` (IMPORT original + native) |
| `rex_RHISetStencilState` | `RHISetStencilState` (IMPORT original + native) |

### UE3 RHI bound-shader-state (input-layout + VS + PS bundle)
| Guest function | Native handler |
|---|---|
| `rex_RHICreateBoundShaderState_...` | `RHICreateBoundShaderState` |
| `sub_823C9610` | `CreateBoundShaderStateResource` |
| `rex__4FBoundShaderStateRHIRef_...` | `AssignBoundShaderStateRef` |
| `rex__0FBoundShaderStateRHIRef_...` | `CopyConstructBoundShaderStateRef` |
| `sub_823CB2B8` | `ReleaseBoundShaderStateRef` |
| `sub_823C58E8` | `SetVertexDeclarationBind` |
| `sub_823C5A20` | `SetBoundShaderState` |

### Render-state setters (D3DRS_*)
Generated via the `RENDER_STATE_HOOK` macro → `rr::SetRenderState(device,
D3DRS_*, value)`:
`AlphaBlendEnable, AlphaTestEnable, BlendOp, BlendOpAlpha, ColorWriteEnable,
DepthBias, DestBlend, DestBlendAlpha, SlopeScaleDepthBias, SrcBlend,
SrcBlendAlpha, ZEnable`.

Plus dedicated state setters that IMPORT the original then mirror native:
| Guest function | Native handler |
|---|---|
| `sub_823C10B0` | `SetColorWriteEnable` |
| `sub_823C36D8` | `SetZWriteEnable` |
| `sub_823CCAC0` | `ApplyRasterizerState` (cull mode) |
| `sub_823C6308` | `SetCullMode` |
| `rex_D3DDevice_SetRenderState_ClipPlaneEnable` | `RsClipPlaneEnable` |
| `rex_D3DDevice_SetRenderState_ViewportEnable` | `RsViewportEnable` |
| `rex_SetPending_ClipPlanes_D3D_...` | `SetPendingClipPlanes` |

### Draw calls
| Guest function | Native handler |
|---|---|
| `rex_D3DDevice_DrawVertices` | `DrawVertices` |
| `rex_D3DDevice_DrawIndexedVertices` | `DrawIndexedVertices` |
| `rex_D3DDevice_DrawVerticesUP` | `DrawVerticesUP` |
| `rex_D3DDevice_BeginVertices` | `BeginVertices` (immediate-mode ring) |
| `rex_RHIDrawIndexedPrimitiveUP_...` | `RHIDrawIndexedPrimitiveUP` |

### Clear / resolve
| Guest function | Native handler |
|---|---|
| `rex_D3DDevice_ClearF` | `ClearF` |
| `rex_D3DDevice_Resolve` | `Resolve` (→ `rr::StretchRect`) |

### Present + GPU synchronization (neutralized / redirected)
| Guest function | Native handler | Behavior |
|---|---|---|
| `rex_D3DDevice_Swap` | `Swap` | flush immediate verts, set present source, `Video::Present()` |
| `rex_BlockOnFence_...` | `BlockOnFence` | **no-op** (native renderer owns sync) |
| `rex_D3DDevice_BlockUntilIdle` | `BlockUntilIdle` | **no-op** |
| `rex_D3DDevice_SetShaderGPRAllocation` | `SetShaderGPRAllocation` | **no-op** (Xenos-specific) |
| `rex_SynchronizeToPresentationInterval_...` | `SynchronizeToPresentationInterval` | **no-op** |
| `rex_D3DDevice_SetPredication` | `SetPredication` | **no-op** |
| `rex_KickOff_CDevice_...` | `KickOff` | seeds a scratch ring buffer in the guest device struct |
| `rex_BlockOnSecondaryPosition_...` | `BlockOnSecondaryPosition` | **no-op** |
| `rex_RHIGetOcclusionQueryResult` (via handler) | `RHIGetOcclusionQueryResult` | returns a fixed "visible" result |

The no-op'd functions are exactly the Xenos/EDRAM/GPU-fence primitives that only
make sense for the emulated GPU; with the native renderer owning the timeline
they must be neutralized.

---

## 4. The native renderer that backs the hooks

The hooks call into the `reodyssey::render` (`rr::`) namespace, implemented on
**plume** (the vendored RHI, D3D12 on Windows / Vulkan fallback).

| File | Responsibility |
|---|---|
| `src/render/video.cpp` | `Video::Init/Present/Shutdown/WaitForGPU`; owns plume device, queue, swapchain, framebuffers, bindless descriptor sets, sampler set, pipeline layout, copy queue, blit pipeline. The presentation backbone. |
| `src/render/render_state.cpp` | Largest TU (~3.5k lines). The live render-state machine: `SetRenderState`, draw submission (`DrawPrimitive*`, `DrawIndexedPrimitive*`, `DrawPrimitiveUP`), stream/index binding, RT/depth binding, clears, `StretchRect` resolves, constant upload, frame begin/resolve-on-present. |
| `src/render/d3d_resource_hooks.cpp` | Native resource objects + format translation: `CreateVertexBuffer/IndexBuffer/Texture/Surface`, vertex declarations, `TranslateGuestTexture/Surface`, lock/unlock, `LoadTextureFromMemory`, Xenos `D3DFORMAT` → plume `RenderFormat`. |
| `src/render/pipeline.cpp` | Shader loading/translation (microcode → DXIL via the shader cache + dxc), PSO assembly. |
| `src/render/guest_*.h` | `GuestDevice`, `GuestResource`/`GuestTexture`/`GuestBuffer`/`GuestSurface` layouts, the guest heap (`ghp::ToHost/ToGuest/GuestAllocRaw`), `IsReoResource()` tagging. |
| `src/render/render_internal.h` | Shared plume accessors: `Interface()`, `Device()`, `CommandList()`, descriptor sets, pipeline layout, blit pipeline, descriptor alloc. |
| `src/render/shaders/copy_vs/ps.hlsl` | Fullscreen blit shaders used to copy the guest front buffer onto the swapchain backbuffer. |

### Key native-renderer concepts

- **Resource tagging (`IsReoResource`)** — native objects carry a tag so hooks
  can tell "our" resources from genuine guest D3D structs and branch
  accordingly (native path vs. call original via `REX_IMPORT`).
- **Guest-data fast path** — `SetStreamSource`/`SetIndices` can read straight
  from guest memory (parsing the guest VB/IB header) when the buffer wasn't
  created through the native `Create*` path (`SetStreamSourceGuestData` /
  `SetIndicesGuestData`).
- **Immediate-mode ring** — `BeginVertices`/`KickOff` implement the 360's
  immediate-draw ring allocation; `FlushImmediateVertices()` is called at the
  top of nearly every state/draw hook to flush pending immediate geometry
  before state changes.
- **Bindless ABI** — `Video::Init` builds 3 large texture descriptor sets + a
  sampler set + root CBVs matching the **XenosRecomp shader ABI**, so
  translated shaders address resources the same way they did on console.

### Present path (`Video::Present`, called from the `Swap` hook)
1. Guest draws/clears for the frame are already recorded into the open plume
   command list, targeting the guest's own RT surfaces.
2. `FlushPendingResolvesForPresent()` resolves any pending EDRAM-style resolves.
3. The guest's final front buffer (`g_presentSource`, set in `Swap`) is bound as
   a bindless texture and **blitted** with the fullscreen copy pipeline onto the
   acquired swapchain backbuffer (or a clear color if no front buffer).
4. Command list executed with acquire/render semaphores; `swapChain->present()`.
   Currently fully serialized (`waitForCommandFence`) — frame pipelining is a
   TODO.

---

## 5. End-to-end flow for one guest API call

```
Game (guest PPC) calls D3DDevice::SetTexture
        │  (XEX addr 0x823BA578)
        ▼
config.toml: 0x823BA578 = "rex_D3DDevice_SetTexture"
        │  codegen emits recompiled symbol `rex_D3DDevice_SetTexture`
        ▼
d3d_hooks.cpp: REX_HOOK(rex_D3DDevice_SetTexture, SetTexture)
        │  REX_FUNC defines that symbol → HostToGuestFunction marshals regs
        ▼
native SetTexture(GuestDevice*, sampler, GuestTexture*, mask)
        │  FlushImmediateVertices(); translate/lookup texture
        ▼
rr::SetTexture(device, sampler, reo)   [render_state.cpp]
        │  records into plume command list (no Xenos emulation involved)
        ▼
Video::Present()  blits guest front buffer → swapchain  [video.cpp]
```

---

## 6. Mid-asm render/quality patches (adjacent, not the renderer itself)

`reodyssey_config.toml` `[[midasm_hook]]` entries + `src/render/render_patches.cpp`
implement *optional* graphics tweaks gated by cvars (`lo_patch_*`). These are
in-function instruction patches, not API hooks — included for completeness
because they're "rendering" but are independent of the native-renderer switch:

- `LoPatch60FpsHook` (0x827B4A0C) — force 60 FPS presentation selector.
- `LoPatchDisableOcclusionQueries*` (0x823BAD3C, 0x823BD618) — stability.
- `LoPatchPostProcessingUpscalingFix*` (0x8295B62C, 0x82698F50, 0x8271C49C).
- `LoPatchDisableDepthOfField` (0x82305D74) / `LoPatchDisableMotionBlur`
  (0x826E1884) — branch-skip via `jump_address_on_true`.
- `LoPatchAnisotropicFiltering16x` (0x823B93CC).
- `LoPatchDisableDynamicShadows*` (0x823D5A98, 0x823D5FCC, 0x823DE090).
- `LoPatchAspectRatio*` (0x82486320, 0x82730094, 0x8248635C, 0x8264F558,
  0x8230065C, 0x823007AC) — ultrawide / 16:10 support.
- `LoPatchPartialDebugMenu*` (0x828F9ED4/ED8/F4C).
- `LoMouseSupport*` (many) — mouse/keyboard UI hit-testing, in
  `src/input/reodyssey_mnk_input.cpp`.

Mid-asm hook authoring is documented in `docs/midasm-hooks-guide.md`.

---

## 7. Checklist to replicate this for another title (e.g. DOAX)

1. **Identify the title's graphics API surface.** ReOdyssey is UE3-on-360, so it
   has both raw `D3DDevice_*` calls *and* a UE3 `RHI*`/`FXe*` layer. Map every
   resource-create / lock / state-set / draw / present / sync entry point in
   IDA and name them in the config (`0xADDR = { name = "rex_..." }`). Reuse the
   `rex_D3DDevice_*` naming so the SDK's known D3D symbols line up.
2. **Disable the emulated GPU** in the app's `OnPreSetup`:
   `config.graphics.reset();`
3. **Init a native plume renderer** in `OnPreLaunchModule` on the window HWND;
   shut it down in `OnShutdown`. (Copy `video.cpp` as the starting point.)
4. **Write the hook TU** (`d3d_hooks.cpp` equivalent): `REX_HOOK` every guest
   entry point to a native handler; `REX_IMPORT` the originals you still need to
   run for guest-side bookkeeping; **no-op** all Xenos/EDRAM/fence primitives.
5. **Port the native renderer** (`render_state.cpp` / `d3d_resource_hooks.cpp` /
   `pipeline.cpp`): format translation, resource objects, state machine, draw
   submission, shader translation (XenosRecomp ABI), constant upload, present
   blit.
6. **Tag native resources** so hooks can distinguish them from genuine guest
   structs and branch correctly.
7. Add any per-title quality patches as `[[midasm_hook]]` entries last.

The big lift is steps 4–5 (the renderer + per-title shader/format/RHI quirks);
steps 1–3 are mechanical.

---

## Source map (ReOdyssey)

- `src/reodyssey_app.h` — disables emulated GPU, inits native renderer.
- `src/render/d3d_hooks.cpp` — **all** `REX_HOOK`/`REX_IMPORT` registrations.
- `src/render/video.cpp` — plume device/swapchain/present.
- `src/render/render_state.cpp` — render-state machine + draw submission.
- `src/render/d3d_resource_hooks.cpp` — native resources + format translation.
- `src/render/pipeline.cpp` — shader translation + PSO.
- `src/render/render_patches.cpp` + `src/input/reodyssey_mnk_input.cpp` —
  mid-asm gameplay/render/input patches.
- `reodyssey_config.toml` — address→name map (`rex_D3DDevice_*`) +
  `[[midasm_hook]]` list.
- `include/rex/hook.h` (SDK) — `REX_HOOK` / `REX_IMPORT` / `REX_FUNC` macros.

---

## Appendix A: Xbox 360 D3D9 surface — verified against the Lost Odyssey XEX

Verified by querying the LO IDA database directly
(`D:\Emulation\Games_Xbox_360\LostOdyssey\LostOdyssey\LostOdyssey.xex.i64`, the
`ida40` MCP server). Two facts established:

1. **LO statically links the Xbox 360 D3D9 library into its XEX, with C++
   symbols.** Functions are named `rex_D3DDevice_*` and many carry MSVC-mangled
   suffixes (e.g. `rex_BlockOnFence_CDevice_D3D_QAAXKW4_D3DBLOCKTYPE_PAUD3DResource_Z`),
   so these are *real linked symbols*, not RE guesses. This is why the
   native-renderer approach works at all: there is concrete recompiled code at
   each address to `REX_HOOK`.
2. **The canonical XDK D3D9 export list is only *partially* present** — by
   design (see Appendix B). You hook what is linked out-of-line; the rest is
   inlined into callers.

### A.1 Every function ReOdyssey hooks exists in the LO XEX

All addresses in `reodyssey_config.toml` resolve to named functions. The helpers
ReOdyssey hooks by raw `sub_` address are renamed `rex_LO_*` in the database:

| ReOdyssey hook (config `sub_`) | LO symbol | Addr |
|---|---|---|
| `sub_823C9610` | `rex_LO_CreateBoundShaderStateResource` | 0x823C9610 |
| `sub_823C5A20` | `rex_LO_SetBoundShaderState` | 0x823C5A20 |
| `sub_823C58E8` | `rex_LO_SetVertexDeclarationBind` | 0x823C58E8 |
| `sub_823CB2B8` | `rex_LO_ReleaseBoundShaderStateRef` | 0x823CB2B8 |
| `sub_823C10B0` | `rex_LO_SetColorWriteEnable` | 0x823C10B0 |
| `sub_823C36D8` | `rex_LO_SetZWriteEnable` | 0x823C36D8 |
| `sub_823CCAC0` | `rex_LO_ApplyRasterizerState` | 0x823CCAC0 |
| `sub_823C6308` | `rex_LO_SetCullMode` | 0x823C6308 |

The named entry points (`rex_D3DDevice_Swap` 0x827B4E50, `rex_KickOff_CDevice_…`
0x823CDEF8, `rex_BlockOnFence_CDevice_…` 0x823B62A0, `rex_FXeVertexShader_Init`
0x823F2428, `rex_RHISetDepthState_…` 0x823C3638, etc.) all resolve as well.

LO also links a broad `RHI*` surface beyond what ReOdyssey currently hooks
(~26 functions: `RHISetRenderTarget`, `RHISetBlendState`, `RHICopyToResolveTarget`,
`RHICopyFromResolveTarget`, `RHIMSAA*`, `RHICreateTargetableSurface`,
`RHICreateSamplerState`, `RHIBeginDrawingViewport`, …) — available if a future
pass needs finer-grained interception.

## Appendix B: Canonical exports that are *absent* as standalone functions

These appear in the SDK's `src/kernel/xam/export_table.inc` canonical list but do
**not** exist as standalone functions in LO. This is expected for a release-built,
statically-linked title: on the 360 they are `__forceinline` header accessors or
debug-only wrappers, so the compiler inlined them into callers or the linker
dead-stripped them. **They are not hook points** — they operate directly on the
guest device struct, which the native renderer mirrors.

| Canonical export | Why absent |
|---|---|
| `D3DDevice_GetRenderState_*` (ZEnable, CullMode, BlendOp, …) | inline header getters |
| `*_ParameterCheck` (Set{VS,PS}ConstantF / SamplerState / RenderState) | debug-only param-check wrappers, stripped in release |
| `D3DVertexBuffer_Unlock`, `D3DIndexBuffer_Unlock`, `D3DTexture_UnlockRect` | inline; folded into generic `rex_UnlockResource_D3D_…` (present, 0x823F1458) |
| `D3DDevice_GetViewport / GetRenderTarget / GetDepthStencilSurface` | inline getters |
| `D3DDevice_DrawIndexedVerticesUP` | LO routes through `RHIDrawIndexedPrimitiveUP` / `BeginIndexedVertices` instead |
| `XGSetTextureHeader` (non-Ex), `XGGetTextureDesc` | only `…HeaderEx` + `XGOffsetResourceAddress` linked |
| `D3DDevice_SetSamplerState_AddressU/V` | inlined/folded into sampler state |
| per-type `D3D{VertexShader,PixelShader,VertexDeclaration}_AddRef/Release` | only generic `rex_D3DResource_AddRef/Release` linked |

### Takeaway for the DOAX RE pass

When mapping DOAX's graphics surface (config step 1), expect the *same shape*:
only the out-of-line D3D functions the title actually calls will be present, and
the inline accessor category will be missing. Hook the present ones; do not hunt
for the inlined getters/unlocks — mirror their effect in the native render-state
struct instead. Note DOAX is **not** UE3, so it will share the core
`D3DDevice_*` surface but will **not** have LO's `RHI*` / `FXe*` wrapper layer —
that interception layer must be re-identified against whatever engine DOAX uses,
and its internal helpers (LO's `rex_LO_*` set) will live at different addresses
under different names. Whether DOAX shipped with symbols (as LO did) determines
how much naming you get for free vs. must reconstruct.
