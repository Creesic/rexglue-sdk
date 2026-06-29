# FM2 plume_native — geometry-binding handoff

**Goal:** get Forza Motorsport 2 to render visibly under `--fm2_plume_mode plume_native`
(native Plume RHI; the Xenos command processor is disabled in this mode).

**One-line status:** the whole render pipeline is now healthy (PSOs build, draws
submit, RT bind + present + texture sampling all work) — the screen is black only
because **the scene's index buffer (and large-mesh geometry) is never bound**: it
rides the PM4 command stream as **Xenos vertex/index fetch constants stored in the
render context**, and our D3D9-style `SetStreamSource`/`SetIndices` path receives
NULL. Bind the geometry from the context fetch constants and the scene should draw.

---

## How to build / run

```powershell
# from repo root, after editing FM2 sources:
#  (uses VS18 vcvars; fm2.exe must be stopped before relink)
$bd = "FM2/out/build/win-amd64-relwithdebinfo"
cmake --build $bd --target fm2 --config RelWithDebInfo
```
Run profile: VS launch target **`fm2.exe (plume_native)`** (in
`FM2/.vs/launch.vs.json`), or pass `--fm2_plume_mode plume_native`.
Log: `C:\temp\fm2-clean.log` (clear it before a run to isolate output).

---

## THE NEXT TASK (the one that should make geometry appear)

The game issues real draws — confirmed in the log:
`FM2_PM4_DRAW prim=4 startIndex=37158 indexCount=1302` (a 434-triangle mesh). But
the geometry buffers are **Xenos fetch constants in the render context**, not D3D9
buffers. Bind them at draw time.

**Context layout (context == device == guest 0x4004D100), decoded from IDA:**
- **Index resource** pointer @ `ctx+0x2F7C`. In the resource: fetch base @ `+0x18`,
  format = 16-bit unless `resource[0]` sign bit set ⇒ 32-bit. The draw reads from
  `base + startIndex*stride`. *(Source: `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset`
  0x827317A0 — `v13 = *(ctx+0x2F7C); v20 = v13[6]; v24 = (2*startIndex + v20) & 0x1FFFFFFF`
  for 16-bit, `4*startIndex` for 32-bit, `if (*v13 < 0)` ⇒ 32-bit.)*
- **Vertex stream s** resource ptr @ `ctx+0x2F94 + 4*s`; stride/4 byte @ `ctx+0x2FD8 + s`.
  In the resource: fetch dword0 @ `+0x18` (low 2 bits = endian; phys addr = `& 0x1FFFFFFC`,
  mask `& 0x1FFFFFFF`), fetch dword1 @ `+0x1C` (**size_bytes = `((dword1>>2) & 0xFFFFFF) * 4`**),
  `+0x20` = ASCII debug name. *(Source: `FM2_RenderContext_BindVertexStream` 0x82370E48.)*
- Bases are guest **PHYSICAL** addresses: validate with
  `ghp::GuestMemory()->GetPhysicalHeap()->QueryRangeAccess(physBase, physBase+size-1)`
  and read with `mem->TranslatePhysical<const void*>(physBase)`
  (pattern: `d3d_resource_hooks.cpp` ~820–857).

**Where to bind:** `BindPm4GeometryFromContext(device, context)` already exists in
`FM2/src/render/d3d_hooks.cpp` (called from `SubmitNativeIndexedDrawPm4` and
`Fm2EmitIndexedDrawPm4Base`). It currently reads `res+0x18`/`+0x1C` **raw** (wrong),
so `physReadable` rejects the bogus 256MB size and nothing binds. Fix it to:
1. **Decode** the fetch constants (addr = `res18 & 0x1FFFFFFC`, phys `& 0x1FFFFFFF`;
   size = `((res1C>>2) & 0xFFFFFF) * 4`).
2. Bind vertices via `rr::SetStreamSourceGuestData(device, s, host, size, stride)`
   and indices via `rr::SetIndicesGuestData(device, host, size, stride)`.

**Open sub-problem — `idxRes` reads 0.** `ctx+0x2F7C` came back **0** in the log
(while the vertex slot `ctx+0x2F94` read fine = `0x2E0483C0`). The original draw
emitter reads the index resource from `ctx+0x2F7C`, so it must be valid at draw
time in Xenos — figure out why it's 0 for us:
- Log `ctx+0x2F7C` across *many* draws (not just the first few — early draws may
  precede the bind), and right after the original `FM2_RenderContext_BindIndexBuffer`
  runs (our `Fm2BindIndexBuffer` already calls `g_origFm2BindIndexBuffer` first).
- `Fm2BindIndexBuffer → SetIndicesNative` was logged getting **NULL** resource —
  so the index buffer may be delivered by a PM4 `SET_INDEX_BUFFER` packet the
  disabled CP never processes (same architectural shape as the constants). If so,
  pull the index base from the draw emitter's own path instead: `gpuOffset` (a3)
  and `startIndex` are currently **dropped** by `SubmitNativeIndexedDrawPm4` — thread
  them through and bind index = `v13[6] + startIndex*stride`, size = `indexCount*stride`.

**Note:** stream 0 already binds 384 bytes (24 verts) via an existing path; once the
index buffer binds, verify the *big* scene draws' streams (the 384B may be a small
UI draw). Vertices are likely mostly fine; **index is the gating miss.**

---

## On removing the "D3D9 hooks"

There is no clean, separable set of generic D3D9 hooks — every render hook is on an
FM2 PM4-engine function. The *D3D9 buffer model* lives in these bridge handlers and
should be **replaced (not just deleted)** by the PM4 fetch-constant binding above,
because some of them feed the working texture path:
- `SetStreamSource` / `SetIndices` / `SetIndicesNative` (`render_state.cpp` /
  `d3d_hooks.cpp`) — geometry state; gets NULL for scene draws. **Replace** with the
  fetch-constant bind. Safe to retire once `BindPm4GeometryFromContext` works.
- `CreateVertexBuffer`/`CreateIndexBuffer` aliasing + `VertexBufferLock`/`IndexBufferLock`
  — D3D9 resource model. **Keep until** the fetch-constant path is proven, then prune.
- `SurfaceLockRect` / texture upload / `CreateTexture` paths — **KEEP** (the texture/UI
  path that renders depends on these).
Do this prune as the *last* step of the geometry task, with the debug layer on to
confirm nothing regresses.

---

## Fixes already landed this session (do not redo)

- **Crash fixed** — `plume::D3D12CommandList::setPipeline` was given a pipeline whose
  `ID3D12PipelineState` was null (D3D12Core null-deref at +0xCC). `plume_d3d12.cpp`:
  `D3D12GraphicsPipeline` ctor now checks the `CreateGraphicsPipelineState` HRESULT and
  leaves `d3d` null + logs `PLUME_PSO_CREATE_FAILED`; `createGraphicsPipeline` returns
  nullptr on failure (FM2's `g_pipelineBound` then skips the draw); `setPipeline` guards
  null. *(Note: clang-cl `/EHsc` does NOT let `__try/__except` catch access violations —
  pipeline.cpp is built `/EHa`; the pipeline.cpp SafeDxc* guards chased the wrong bug and
  can be reverted.)*
- **PSO E_INVALIDARG fixed (the big one)** — `ConvertDeclType` (`render_state.cpp` ~1503)
  was missing Xenos vertex formats (`0x10`,`0x11`,`0x22`,`0x23`) → `DXGI_FORMAT_UNKNOWN`
  → `CreateInputLayout` rejected the whole PSO. Added them + a safety net (any leftover
  UNKNOWN → `R32_UINT`, logged `FM2_DECL_UNKNOWN_FMT`). **PLUME_PSO_CREATE_FAILED 1708→0**,
  `FM2_DRAW_OUTCOME ok=~4200 create_fail=0`.
- **Index-format guard** — `CreateIndexBuffer` (`d3d_resource_hooks.cpp:128`) forced to
  `R16_UINT`/`R32_UINT` (was defaulting unrecognized formats to `R8G8B8A8_UNORM`, which
  D3D12 rejects as an index format); plus a coerce guard before `setIndexBuffer`.
- **MSAA** — `pipelineState.sampleCount` forced to `COUNT_1` (`kDebugForceSingleSample`)
  because plume RTs are single-sampled but the surface's intended count is 4 (debug
  id=614/616).
- **Present source** — both present paths default to `GetLastDrawnColorRenderTarget`
  (`kPreferFrontbufferPresent=false`); the guest frontbuffer RAM is black (no CP resolve).
- **Constants are NOT the problem** (red herring) — `device+0x700` has a valid transform
  for real scene draws; the earlier "zero" was sampling the first-10 pre-upload draws.
  `MirrorPassVsConstants` (hook on `0x8236D958`) was added but is redundant.

---

## Debug tooling available (currently ON — turn off for a clean run)

- **D3D12 debug-layer drain** — `plume_d3d12.cpp` (forced on in NDEBUG, break-off),
  drained in `executeCommandLists`, deduped by message ID → `PLUME_D3DMSG` in the log.
  This is what cracked the geometry bug open (it reported `id=211/212/715` index-buffer
  errors). Remaining noise to clean up later: `id=921` (resource deleted before cmd-list
  close — lifetime/UAF), `id=527` (barrier before-state), `id=1422` (RT not initialized),
  `id=538` (discard state), `id=1425` (copy state).
- **Present test grid** — `g_showPresentTestGrid` (`video.cpp`): on-screen grid, cells
  0–7 = sampled textures (these render), cells 8–11 = recent color RTs.
- **Blue clear** — `kDebugClearRtBlue` (clear-on-RT-change) + `kDebugForceCullNone`
  (`render_state.cpp` FlushRenderState). The "geometry on blue?" test.
- **Per-draw traces** — `FM2_PM4_DRAW` (intent), `FM2_SETIDX`/`FM2_SETVTX` (D3D9 binds,
  both NULL), `FM2_DRAWSTATE`+`GEO` (bound state at draw time), `FM2_PM4GEO_IDX/VTX`
  (context fetch-constant reads).

Toggles to flip OFF once geometry works: `g_showPresentTestGrid`, `kDebugClearRtBlue`,
`kDebugForceCullNone`, `kDebugForceSingleSample` (try real sample count), the debug
layer block in `plume_d3d12.cpp`, and the `FM2_*` log spam.

---

## Memory cross-ref

`memory/project_plume_native_black_screen.md` (sessions 6P-2 / 6P-3) has the full
blow-by-blow, including the constant-offset saga and the present-target details.
