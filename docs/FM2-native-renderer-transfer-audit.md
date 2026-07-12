# FM2 Native Renderer Transfer Audit

Date: 2026-07-12  
Branch: `plume`  
Source: `C:\Users\Tera\Documents\GitHub\ReXGlue080plume` (`FM2/src/native_renderer/` + `FM2/src/render/`)  
Dest: this repo (`FM2/src/render/` only)

Related: [migration-from-plume.md](migration-from-plume.md)

## Verdict

**Intentionally cleaned, incomplete for full gameplay rendering.**

`ReXFM2P` rebuilt the production Plume path into a cleaned `FM2/src/render/`.
The ~5k-line `FM2/src/native_renderer/` diagnostic overlay from the sibling
repo was **deliberately dropped** — do not re-port it.

Several **load-bearing pieces from SOURCE `render/`** (not from
`native_renderer/`) were not carried forward. Commit `64d7a2a8` notes visual
output is still black; those gaps are the likely cause.

## What landed (commits)

| Commit | What |
|--------|------|
| `d3748a68` | Phases 1–3: device/present, resources, render state + shaders |
| `34a4bf8a` | Phase 4: draw dispatch + float-constant upload from `GuestDevice` |
| `64d7a2a8` | Present wiring, vsync interrupt, DXC pin, hang-watchdog, audio; still black |

Architecture choice: FM2-local direct `REX_HOOK` D3D replacements against
vendored `thirdparty/plume`. No `GPU_PLUGINS xenos`. Present is driven by
`D3DDevice_Swap` → `PrepareFramePresent()` → `Video::Present()`, not by
`FM2PlumeTraceVdSwap` midasm hooks.

## Scale comparison

| File / area | SOURCE | DEST |
|-------------|--------|------|
| `d3d_hooks.cpp` | ~8,471 lines | ~772 lines |
| `render_state.cpp` | ~6,547 lines | ~1,366 lines |
| `native_renderer/` | ~11 files (~5k lines incl. shaders) | **absent** |
| `fm2_hooks.cpp` | ~3,645 lines (incl. PlumeTrace) | ~242 lines (crash guards only) |

## Capability matrix

| Capability | SOURCE | DEST | Risk if missing |
|------------|--------|------|-----------------|
| Plume device / swapchain / present | `render/video.cpp` | Present (rebuilt + vsync worker) | — |
| Present trigger | `FM2PlumeTraceVdSwap` midasm | **Replaced** by `REX_HOOK(D3DDevice_Swap, …)` | Low |
| Resource create / lock / unlock | `d3d_*_hooks` | Present (pure-replace) | — |
| Render state (blend / Z / clip / bool) | both | Present (subset) | Low |
| Shader create + cache | both | Present | — |
| Draw dispatch | complex + late hooks | Present (`DrawVertices*` full replace) | — |
| Float constant transport | PM4 shadow + multiple SetF / UploadMatrix / GpuBegin hooks | **Mostly OK** — unhooked `SetVertexShaderConstantFN` writes GuestDevice; GpuBeginF4 is font-only | Medium (font/UI niche) |
| Texture binding | Fetch translate + `SetTexture` in dirty sibling | **Present** — `REX_HOOK(D3DDevice_SetTexture)` → `SetTexture` / `SetTextureBase` (XG headers still null) | Medium (XG gap) |
| Viewport / scissor | Wired via render-context hooks | **Present** (`D3D_SetViewport` / `SetScissorRect` hooked) | Low |
| `D3DDevice_ClearF` → RT clear | `rr::Clear(...)` | **Present** (`ClearF` → `rr::Clear` + fallback) | Low |
| `D3DDevice_Resolve` | Resolve + aperture tracking | **Missing hook** (`ResolveToTexture` declared only) | High (compositing) |
| Recorded batch (showroom / car) | Complex replay | Degraded: placeholder PS inside batch | Medium (intentional) |
| GPU hang watchdog | Producer-guard hooks | Present (`D3D_CBlocker_Check`) | — |
| `native_renderer/` debug overlay | Full | **Intentionally dropped** | None for production |
| `SubmitDirectDebugReplay` / `SubmitNativeDirectDraw` / `Flush*OnPresent` | `native_renderer/` | Absent | None |
| `FM2PlumeTrace*` / `RecordNative*` | `fm2_hooks` + manifest | Absent | None (replaced by `REX_HOOK`) |
| PM4 shadow scanner (`kScannerApplies` / `kGpuBeginHookFeedsShadow`) | `d3d_hooks` / `render_state` | Intentionally dropped | High only if alt constant paths aren't hooked |
| `fm2_plume_mode` / debug-replay cvars | `native_renderer` | Absent (only `fm2_plume_single_thread_present`) | None |
| Debug replay HLSL | `native_renderer/shaders/` | Absent | None |
| `shader_probe_window` / dump / analysis | diagnostic TUs | Absent | None |

## Intentional cleanup (do not re-port)

1. Entire `FM2/src/native_renderer/` — debug replay, compare window, native-direct-draw experiments
2. All 18 `FM2PlumeTrace*` midasm hooks and `fm2_plume_*` mode/trace cvars
3. PM4 ring scanner / `g_pm4VsConstants` shadow machinery
4. `shader_probe_window`, `constant_trace_state`, `DumpLoadedShaderResource`, `fm2_shader_analysis`
5. Diagnostic midasm hooks, crash-logger VEH, hardcoded `C:\temp\fm2-clean.log` tracing
6. `GPU_PLUGINS xenos` — replaced by FM2-local Plume renderer
7. Debug replay HLSL shaders (`fm2_debug_replay*.hlsl`)

## Likely accidental / still needed from SOURCE `render/` / Unleashed

1. **`D3DDevice_SetTexture` hook** → `rr::SetTexture` (IDA: 65 callers; Unleashed pattern). Optionally keep fetch-bit / aperture helpers as secondary.
2. **`D3DDevice_Resolve` + resolve-aperture tracking** — so present sees composited front buffer
3. **Sampler-state dirty tracking** — Unleashed uploads from `device->samplerStates[]`
4. **`GpuBeginShaderConstantF4` only if font/UI constants are wrong** — main VS path already writes `GuestDevice` via unhooked `SetVertexShaderConstantFN`
5. **`/EHa` on `pipeline.cpp`** — SOURCE used SEH around DXC link crashes
6. Clear/viewport were listed here earlier; both are now wired on the branch

## CMake / manifest notes

| Item | SOURCE | DEST |
|------|--------|------|
| Plume path | `../plume` | `../thirdparty/plume` |
| `native_renderer` sources | Compiled | Not compiled |
| Shaders | `copy_*` + debug-replay HLSL | `copy_*` + `placeholder_ps.hlsl` |
| DXC | Windows Kits glob | Pinned `thirdparty/dxc-bin` |
| `rexglue_setup_target` | default | `AUDIO_BACKENDS xaudio2`, no `GPU_PLUGINS` |
| Midasm hooks | 126 incl. 18 PlumeTrace | 12 crash/QoL/FMOD only |

No dangling `#include "native_renderer/..."` in DEST.

## Uncommitted WIP at audit time

`+51` lines across `d3d_hooks.cpp`, `pipeline.cpp`, `render_state.cpp`, `video.cpp` —
present/pipeline diagnostic logging only, not structural port work. (Clear/viewport
hooks have since landed on the branch; see IDA section below.)

## Comparison to UnleashedRecomp

Baseline: `C:\Users\Tera\Documents\GitHub\UnleashedRecomp\UnleashedRecomp\gpu\`
(mature Plume native renderer for Sonic Unleashed).

### Shared DNA

Both are **Plume + XenosRecomp**: offline shader cache → runtime DXC link with
spec constants → PSO cache → host draw. Guest shapes (`GuestDevice`,
`GuestTexture`, `GuestBuffer`, `FlushRenderState`) are the same lineage.
Unleashed keeps it in one ~7.9k-line `gpu/video.cpp`; FM2 splits into
`video` / `d3d_hooks` / `d3d_resource_hooks` / `render_state` / `pipeline`.

### Side-by-side

| Dimension | UnleashedRecomp | ReXFM2P (`plume`) |
|-----------|-----------------|-------------------|
| Size | ~8k LOC monolith + imgui/MSAA shaders | ~4.2k LOC across 11 files |
| Hooks | ~42 `GUEST_FUNCTION_HOOK`s on raw addrs | ~52 `REX_HOOK`s on named + FM2 RenderContext |
| Present | `sub_82BDA8C0` → `Video::Present` | `D3DDevice_Swap` → `PrepareFramePresent` → Present |
| Threading | Dedicated render thread + command queue | Sync on guest threads + present-owner latch |
| Textures | Hooked `SetTexture` + StretchRect resolve | `SetTexture` API exists, **not hooked** |
| Constants | Dirty-flag ranged uploads | Full `GuestDevice` float file every draw |
| Resolve / MSAA | Full StretchRect + MSAA resolve shaders | `ResolveToTexture` declared, **not hooked** |
| Clear / viewport | Full | Wired (`ClearF` → `rr::Clear`, `D3D_SetViewport` / scissor) |

### What to copy from Unleashed vs keep FM2-specific

**Copy patterns (not addresses):**

1. Hook `D3DDevice_SetTexture` → `rr::SetTexture` / bindless descriptor update
   (IDA confirms FM2 uses this heavily — see below).
2. StretchRect / resolve / pending-MSAA pattern for compositing.
3. Sampler dirty tracking from `device->samplerStates[]`.
4. Consider a render-thread command queue if draw-time races persist.

**Must stay FM2-specific:**

- `FM2_RenderContext_*` surface bind / stream / shader-state hooks
- Declaration recovery (FM2 does not bind decl via the device field Unleashed uses)
- `D3DDevice_Swap` present trigger + `ClaimPresentOwner` + vsync interrupt worker
- Recorded-batch placeholder PS (`FM2_Render_ScopedBatchBegin/Finalize`)
- `kFm2ResourceMagic` + passthrough for XG-built resources
- No `native_renderer/` diagnostic overlay

---

## IDA verification (2026-07-12, IDA37 / `FM2.xex.i64`)

Checked against the live IDB to validate (and correct) the transfer-audit
assumptions about how FM2 itself binds textures, presents, and uploads
constants.

### Present — confirmed: `D3DDevice_Swap` owns it

| Symbol | Addr | Evidence |
|--------|------|----------|
| `D3DDevice_Swap` | `0x8236CB28` | Size `0x598`. At `0x8236CD74`: `bl VdSwap` (import `0x8294E338`). Callers include audio-pump / submit-bridge paths (3 code xrefs). |
| `FM2_D3D_TryPresentAndUpdateStatus` | `0x824F83D8` | Only 3 xrefs; not the live present path. |
| `VdSwap` | import `0x8294E338` | Called **from inside** `D3DDevice_Swap`, not as a separate game entry. Midasm-hooking `VdSwap` alone was the old diagnostic approach; the full-replace `Swap` hook is the correct native-renderer present trigger. |

### Texture binding — **correction**: FM2 *does* use `D3DDevice_SetTexture`

Earlier audit/comparison text claimed FM2 “rarely calls D3D `SetTexture`” and
that only fetch-constant translation mattered. **IDA contradicts that.**

| Symbol | Addr | Evidence |
|--------|------|----------|
| `D3DDevice_SetTexture` | `0x8236C208` | **65 code xrefs.** Writes sampler texture pointer into the device and updates pending/sampler state. Callers include material setup, object-pass draws, PM4 draw dispatch, post-effects. |
| High-value callers | — | `FM2_Render_ApplyPassSamplerBindings` (`0x82728980`), `FM2_Render_ApplyPassShaderConstantsAndTextureBindings` (`0x82728CE0`), `FM2_Render_DrawPassMaterialSetupBodyA`, `FM2_Render_ApplyObjectPassSamplerAndDrawRange`, `FM2_Render_InstanceHybridDrawPath`, `FM2_Render_DispatchPm4DrawOpcode`. |
| `FM2_RenderContext_SetTextureFetchBitsLow/Mid` | `0x8236EA60` / `0x8236EA90` | Tiny dirty-flag setters on `ctx+10440` (8 / 6 xrefs). **Do not bind textures.** Often called with `0` to clear bits (e.g. `FM2_Render_UploadPassTransformConstants`). Secondary to `SetTexture`, not a replacement for it. |

**Implication for the port:** Unleashed’s `SetTexture` hook pattern **is** the
right primary graft for FM2. Port `REX_HOOK(D3DDevice_SetTexture, …)` →
`rr::SetTexture` (with `IsFm2Resource` / translate guards for XG-built
headers). Fetch-bit helpers may still matter for dirty tracking / apertures,
but they are not the main bind path. Prior “only port
`TranslateGuestTextureFetch`” guidance was incomplete.

### Constants — device file is the main path; ring path is niche

| Symbol | Addr | What it does | Call volume |
|--------|------|--------------|-------------|
| `D3DDevice_SetVertexShaderConstantFN` | `0x8236D958` | VMX-copies float4s into `pDevice->m_Constants.Fetch[…]` (the GuestDevice VS float file) and sets `m_Pending.m_Mask[0]`. | **Very high** — object-pass setup, sorted draw lists, instance hybrid draw, screen transforms, etc. (40+ xrefs, truncated). |
| `D3DDevice_GpuBeginShaderConstantF4` | `0x82803358` | Allocates PM4 ring space via `D3D::CDevice::BeginRingBig` — constants go to the **GPU ring**, not the device file. | **Niche** — 1 meaningful code xref (`FM2_Font_ComputeGlyphTransformAndDraw`). |

**Implication:** Reading `device->vertexShaderFloatConstants` at draw time
(current Phase 4 approach) matches how the bulk of FM2 uploads VS floats,
*as long as* `SetVertexShaderConstantFN` is allowed to run (it is currently
unhooked, so the original body writes the device file — correct). The font
`GpuBeginShaderConstantF4` path bypasses that file and would need a separate
hook if glyph/UI constants are wrong. Do **not** re-port the full PM4 shadow
scanner for the main path.

### Resolve — real and load-bearing

| Symbol | Addr | Evidence |
|--------|------|----------|
| `D3DDevice_Resolve` | `0x8237D158` | Size `0xDF4`, **21 code xrefs** — draw-batch resolve, object-pass-to-surface, audio-mix blit regions, post paths. |

`ResolveToTexture` exists in DEST `render_state` but has no `REX_HOOK` yet —
still a real gap for composited present sources.

### Naming note (manifest vs IDA)

`fm2_manifest.toml` maps `0x82371A30` as `FM2_RenderContext_SetBoundSurface`;
IDA currently names that address `D3DDevice_SetDepthStencilSurface`. Treat
manifest names as the hook surface of record for codegen, but verify
semantics in IDA before assuming RT vs depth bind.

---

## Follow-ups (priority, revised after IDA)

1. ~~**Hook `D3DDevice_SetTexture`** → `rr::SetTexture` / descriptor bind~~ **Done (2026-07-12):**
   `REX_HOOK(D3DDevice_SetTexture, SetTextureHook)` + `SetTextureBase` for RT/depth
   SRVs; manifest `0x8236C208` renamed; codegen regenerated. Non-FM2 (XG) textures
   still bind null until `TranslateGuestTexture` is ported.
2. Wire sampler-state dirty tracking (Unleashed `dirtyFlags[3]` pattern) if needed.
3. Validate VS constants at a live scene draw (should already be in the device file via unhooked `SetVertexShaderConstantFN`); only then consider `GpuBeginShaderConstantF4` for font/UI.
4. Optional: harden `D3DDevice_Resolve` present-source tracking (hook already exists).
5. Optional: render-thread command queue if multi-threaded draw races show up after textures work.
6. Build hygiene: `/EHa` on `pipeline.cpp`; commit or drop diagnostic WIP deliberately.

## Bottom line

The transfer correctly moved the **architecture** from `native_renderer`
diagnostics + midasm tracing to a **clean direct-hook Plume renderer** in the
same family as UnleashedRecomp, but it is **not rendering-complete**.

IDA confirms: present via `D3DDevice_Swap`→`VdSwap` is correct; **textures
should follow Unleashed’s `SetTexture` hook**, not a fetch-bits-only path;
constants largely already land in `GuestDevice` via `SetVertexShaderConstantFN`;
`Resolve` is still missing. Do not bring back `native_renderer/`.
