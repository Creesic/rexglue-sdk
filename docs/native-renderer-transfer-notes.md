# Native (Plume) renderer transfer notes

Written after bringing FM2P's Plume renderer across to PGR4 (branch `PGR4`,
`c987a54..919186a`). Two purposes: a checklist for the next game, and a record
of what actually went wrong so nobody re-derives it.

## Where PGR4 stands (2026-09-02)

| Commit | State |
|---|---|
| `7f0aba8` | Minimal PM4 executor in `guest_gpu.cpp`; guest reaches `D3DDevice_Swap` at ~144/sec |
| `c68633e` | Front-buffer read-back path (decode fetch constant, untile, upload, copy to swapchain). Proven end to end; buffer is empty because draws don't execute yet |
| `919186a` | FM2's full renderer ported, builds and links (37 of 83 hooks live). **Crashes in guest D3D init: write to guest `0x000FE000` on the main thread.** Not diagnosed |
| 2026-09-02 evening | Functional port (see section below): no faults, 922/922 shaders from the regenerated cache, draws execute, XDK swap machine cycles (`submitted == retired`), frames presented at ~22/s. Boot video still a flat colour: Bink plane headers parse as `fmt=0` and bind null |

Next step is bisecting that crash: dormant the resource-creation hooks
(`CreateTexture` / `CreateSurface` / `CreateVertexBuffer`) first, or sample the
main thread's `lr` just before device init (the machinery is in
`guest_gpu.cpp`'s worker). After that: run with the empty shader cache so every
shader misses and is dumped to `missed_shaders/*.bin`, then
`cmake --build out/build/win-amd64-plume --target pgr4_regenerate_shader_cache`,
rebuild, and draws execute.

Also open: the process self-exits after ~19 s ("Window closing"); source-1
CPU interrupts are deferred because the callback slot at
`*(device+10900)+16` (`1FB9B010`) still holds the `0x0BADF00D` sentinel; FM2P's
two-sided-stencil plume patch is not on `Creesic/plume` `PGR4` yet.

## What to build into the SDK before the next transfer (priority order)

1. **A `NativeGpuSystem` in the SDK.** Everything in `guest_gpu.{h,cpp}` is
   game-independent: the vblank worker, the GPU register window at
   `0x7FC80000`, scratch-register writeback, the `COHER_STATUS_HOST`
   handshake, `INDIRECT_BUFFER` / `REG_RMW` / `MEM_WRITE` / `EVENT_WRITE_SHD` /
   `INTERRUPT`, and executing the ring **asynchronously on the worker** so it
   trails the CPU. FM2's design (clear `gpu_plugin`, hook D3D at the API)
   leaves the `Vd*` kernel surface dead and the guest deadlocks before its
   first present. This cost most of a day to find; the next game should
   inherit it as a `RuntimeConfig` option.

2. **`rexglue init --native-renderer`.** Scaffold the render sources,
   `guest_gpu`, the CMake block (plume, zstd, dxc-bin, blit shaders, DXC
   runtime DLL copy, `regenerate_shader_cache`), an empty `shader_cache.cpp`,
   `assets/missed_shaders/`, and the app-header wiring. All of it was assembled
   by hand from FM2's tree this time.

3. **Split portable hooks from engine hooks in the source.** The XDK D3D
   library is the same in every title; `FM2_RenderContext_*` and the
   command-buffer batching are Forza's engine. Keep the portable layer in its
   own files with a names-only manifest fragment. Make the no-op
   `ImportFunction` stand-in (`Pgr4NoopGuestFn` in `d3d_hooks.cpp`) an SDK
   feature (`REX_IMPORT_OPTIONAL`) instead of a regex pass.

4. **Automate the address map.** FM2's signatures were already applied to the
   IDB (~50 functions pre-named, some with misleading `FM2_` prefixes). The
   rest fell out of **layout offsets**: `ZFunc → ClipPlaneEnable` is `0x310`
   bytes in both games, `AlphaRef` is `SeparateAlphaBlendEnable + 0x98` in
   both. A script anchored on a few named functions per block can emit the
   whole XDK surface, verified by `define_func` sizes.

5. **Fork hygiene from day one.** plume's `4cc792a` had vanished from upstream
   and existed only on two local disks. Every renderer dependency (plume and
   its four `contrib/` submodules, dxc-bin, XenosRecomp and its four,
   zstd) belongs on org forks with per-game branches, and CMake must check the
   *nested* sources, not just the top-level `CMakeLists.txt`.

6. **Cvar-gated diagnostics in the SDK.** Thread `lr` sampling, the blocker
   probe, the opcode histogram, scratch logging and swap cadence found every
   stall. They are ad hoc and committed; they should be one `--gpu_diag`.

7. Document that `REX_HOOK` has **no fall-through** to the original body (it
   is a link-time replacement), and note that `func_profile.caller_count` is
   wrong for this IDB (use `xrefs_to`).

## Transfer checklist

1. Vendor: plume (+ `contrib/*`), dxc-bin, XenosRecomp (+ `thirdparty/*`),
   zstd, `renderdoc_app.h`. Pin to the reference game's commits; put them on
   forks; verify every pin is publicly reachable.
2. Manifest: start from the reference game's, keep names, blank addresses.
   Fill from the IDB (signatures first, then layout offsets). Run codegen and
   let `Validate` prove every address is a function start.
3. Bring the guest-GPU layer up **before** any D3D hooks. Confirm in the log:
   interrupt callback registered, ring kicked, `SCRATCH_ADDR`/`UMSK` set,
   progress word advancing, `D3DDevice_Swap` cadence.
4. Copy the render sources. Rename `FM2_D3D*` library hooks to clean names
   (including the `__imp__FM2_` import forms — `\bFM2_` misses those). Stub
   every `REX_IMPORT` the manifest doesn't export. Expect two fork deltas:
   two-sided stencil in plume, a kernel vblank helper in the SDK.
5. Build, run with an empty shader cache, dump, regenerate, rebuild.

## Pitfalls that burned real time

- **Exit codes lie.** `start /wait` returned 0 on interrupted builds; the task
  harness reported 0 on a failed link. Read the log, always.
- **`cmd /c start /affinity`** gets Ctrl+C'd immediately in this environment.
  Use `Start-Process -PassThru` + `ProcessorAffinity` + `PriorityClass`.
- **VS 18 clang + VS 2022 `LIB`** = undefined `__std_*` symbols. Run under VS
  18's `vcvars64.bat` and pin cmake to `C:\Program Files\CMake` (vcvars
  prepends VS's own).
- **Stale `.git/index.lock`** appears after interrupted git; check for live
  `git.exe`, then remove if 0 bytes.
- **Gitlinks are recorded at `submodule add` time.** Checking out a pin
  afterwards does not update them; `git add <path>` again or the wrong commit
  ships.
- **Vblank is a hardware signal, not a frame cap.** "Uncapped" fired it at
  4M/sec and starved the guest. Keep it at display refresh; lift frame caps at
  the swapchain (`setVsyncEnabled`).
- **MMIO contract is host byte order.** `CheckLoad` passes the callback value
  through unswapped; the recompiler's non-MMIO path bswaps raw memory.
- **Executing the ring synchronously inside the guest's `WPTR` store**
  delivers `PM4_INTERRUPT` before the guest installs its callback → trap on
  `0x0BADF00D`. The CP must trail the CPU.
- **Logging only the first kick** hid three more submissions and produced a
  wrong "single submission" conclusion. Count, don't sample.

## Useful PGR4 facts

- Front buffer: 1280x720, tiled, format 54 (`k_2_10_10_10_AS_16_16_16_16`),
  endian 0, physical `1F1F7000`; fetch constant at `front_buffer + 28`.
- Ring: `1FB9E000` (32 KB); RPTR writeback `1FB9D03C`; scratch page
  `1FB9C000` (`SCRATCH_UMSK = 0x20033`); CPU-interrupt block `1FB9B000`.
- Device fields: vblank count `+16532`, vblank callback `+16528`, frames
  submitted `+16544`, retired `+16552`, status-block ptr `+10896`,
  interrupt-block ptr `+10900`.
- Main loop `sub_822F6BE0` renders only while `byte_82A61024 != 0`;
  end-of-frame present is `sub_82293828`.
- The guest's real vblank/swap callback is `rex_SwapCallback_D3D` at
  `0x826955A0` (arrives via `SCRATCH_REG4`).

## Init-crash bisect (2026-09-02, after 919186a)

Method: rename a hook's `REX_HOOK(<name>` to `REX_HOOK(PGR4_Dormant_<name>`
(dead symbol), rebuild, run 15 s, check the log for "access violation".

| Live hooks | Result |
|---|---|
| All 37 | write of guest `0x000FE000` (main thread) |
| All but the 4 creation hooks (`CreateTexture/Surface/VertexBuffer/IndexBuffer`) | write gone; **read of guest `0x00000002`** appears instead |
| Only `D3DDevice_Swap` | clean |
| `Swap` + all `SetRenderState_*` | clean |
| All but `AddRef`/`Release` | read of `0x2` persists — not them |
| All but `D3D_SetViewport` | read of `0x2` persists — not it |

So: the creation hooks are one fault (FM2 allocation/layout assumptions), and a
second fault lives in {`SetTexture`, `ClearF`, `Resolve`, `SetScissorRect`,
`SetViewport`, `D3D_CBlocker_Check`, `DrawVertices`, `DrawIndexedVertices`,
`DrawVerticesUP`}. A read of exactly `0x2` looks like a small integer argument
dereferenced as a pointer, i.e. a hook whose PGR4 signature differs from
FM2's. Next: halve that set (state/binding vs the three draws), then compare
the culprit's PGR4 prototype in IDA against the hook's argument list.

## Functional port (2026-09-02, after the bisect)

Root cause of the bisect's "read of guest `0x2`": FM2's `GuestDevice` layout
applied to PGR4's `D3DDevice`. The two XDK builds differ; every offset below
was taken from PGR4's own setters in IDA and is pinned by `static_assert` in
`guest_device.h`.

| Field | FM2 | PGR4 | Source |
|---|---|---|---|
| VS / PS float files | `0x700` / `0x1700` | `0x780` / `0x1780` | `Set*ShaderConstantFN` |
| bool / int files | `0x2700..0x2760` | `0x2780..0x27E0` | `Set*ShaderConstantB/I` |
| bool/int dirty bit | `m_Mask[4]` bit 56 | bit 24 | same |
| vertex declaration | `0x2D14` | `0x2E24` | `D3DDevice_SetVertexDeclaration` @ `0x82694E38` |
| index buffer | `0x2F7C` | `0x308C` | `SetIndices` |
| stream ptrs / strides | `0x2F94` / `0x2FD8` | `0x30A4` / `0x30E8` | `SetStreamSource` |
| vertex fetch consts | `0x6F8 - 8N` | `0x778 - 8N` | same |
| PS / VS handle | -- | `0x318C` / `0x3190` | `SetPending_Shaders` |
| viewport | `0x3168` | `0x3160` | `D3D::SetViewport` |

The "write of guest `0x000FE000`" was `D3DIndexBuffer_Lock` (`0x8269A3B0`,
not in the manifest): the original ran on an FM2 `GuestBuffer` whose
`Address` field is zero, returned guest pointer 0, and the game wrote index
data upward from address 0 until the first unmapped page.

Corrections to earlier notes:

- **`REX_HOOK` does fall through.** `DEFINE_REX_FUNC(name)` emits the body as
  `__imp__name` with `name` as a weak alias, so `REX_IMPORT(__imp__X, ...)`
  reaches the original from inside a hook. The "no fall-through" claim above
  is wrong; a runtime renderer switch and observe-only hooks are both possible.
- `func_profile.caller_count` remains unreliable; `xrefs_to` is not.

What PGR4 does differently from FM2 (all now hooked at the XDK level, no
engine-level render context exists):

- Shaders come from shader packs through `XGRegisterVertexShader` /
  `XGRegisterPixelShader` (`0x82694B28` / `0x826948E8`); `D3DDevice_Create*Shader`
  is never reached. VS object: cached part at `+872`, ucode pointer at `+32`;
  PS: `+40` / `+24`. The hook reassembles the contiguous container and aliases
  the raw handle (`RegisterShaderAlias`).
- Every frame is XDK predicated tiling (`D3DDevice_BeginTiling` `0x8269C6D0`,
  `D3DDevice_EndTiling` `0x8269CB68`, tile count `dword_82A60FB0`). The per-tile
  clear and the EDRAM -> frontbuffer resolve live there, not in
  `ClearF`/`Resolve`; the single-tile path (`PGR4_EndTilingOrResolve`) is the
  only one that calls `D3DDevice_Resolve` with the frontbuffer.
- The main backbuffer / depth surfaces are raw `XGSetSurfaceHeader` objects
  (`dword_82A60EDC`), bound through `D3DDevice_SetRenderTarget`.
  `TranslateRawSurface` decodes `+0x18` (msaa bits 16-17), `+0x24` (packed
  size), `+0x28` (D3DFORMAT) and creates a native surface once per header.
- Vertex declarations, shaders, stream/index sources and render targets are
  bound through `D3DDevice_Set*` directly (hooks call the original, then
  mirror into `rr::`).
- Boot renders Bink video: `PGR4_Bink_DrawFrameQuad` (`0x82825510`) binds 3-4
  planes and draws a 4-vertex strip through `DrawVerticesUP`.

XenosRecomp (fork branch `PGR4`) needed two fixes for PGR4's 922 dumped
shaders: TANGENT0-3 added to the VS->PS interpolator ABI, and the `s16..s31`
vertex-texture aliases now resolve to a reflected sampler's constant-table
name instead of `s<N>` (which is undeclared when `<N>` is reflected).

State after the functional port (run 042, 20 s): 0 access violations, 0
`WAIT_REG_MEM` timeouts, 922 shader-cache hits / 0 misses, every frame draws
the Bink quad and resolves, `D3DDevice_Swap` runs the original after
presenting (XDK swap bookkeeping: `submitted 446 == retired 446`), and the
present fallback shows the last resolve destination.

Ring executor additions that made the swap machine work: `WAIT_REG_MEM` on
guest memory now blocks (bounded 50 ms) while delivering pending source-1
interrupts *and* due vblanks -- the XDK's `D3D::InsertCallback` protocol has
the CP wait for the CPU handler's ack before resetting the `0x0BADF00D`
sentinel, and the swap-pending word is cleared by the vblank handler.
`SCRATCH_ADDR` is the interrupt block's GPU address; the SDK's `0xE0000000`
4 KB offset is why `*(device+10900)` prints as `FFB9B000` while the writeback
page is `1FB9C000`.

Open:

- Bink Y/Cb/Cr planes: `SetTexture` gets raw headers at `*(obj+16+32*frame)`
  (`PGR4_Bink_DrawFrameQuad`) whose fetch constant parses as `fmt=0`,
  `1024x2048` -- wrong object layout or a format we mis-decode; they bind null
  so the video shows as one colour.
- The `Swap` present fallback (`LookupLastResolveDestination`) is a heuristic
  for the boot phase; the frontend's real composite (`EndTiling` into the
  frontbuffer) has not been observed yet.
- `ConvertFormat` lacks `0x182800B6` (gpu 54, the 10:10:10:2 main RT) and
  `0x2DA2ABA4` (gpu 36, R32F); both default to RGBA8.
- One shader (`CC80228A287F34B7`) faults inside XenosRecomp and is skipped.
- The ~10-20 s self-exit (a window-close event, not the guest) has not
  recurred since the interrupt/wait changes; cause still unknown.

## Boot video on screen (2026-09-03)

Three defects sat between the first presented frame and a correct picture:

- **Window-coordinate draws.** PGR4 draws its 2D passes with
  `D3DRS_VIEWPORTENABLE = 0` and vertex positions in pixels; the vertex shader
  is a passthrough, so the host saw NDC 0..1 (one quadrant). FM2's port only
  special-cased unit quads. Fix: a 16-byte extension of the shared-constants
  ABI (`g_NdcScale`/`g_NdcOffset` at bytes 496/504, `c31`) applied in every VS
  epilogue by XenosRecomp, set to `(2/W, -2/H), (-1, 1)` of the bound target
  while the viewport is disabled. Any ABI change means regenerating the cache.
- **Header bases are CPU virtual aliases.** A D3D texture / buffer header
  stores the `0xA/0xC/0xE` virtual address; the `0xE0000000` range has a 4 KB
  offset that the SDK emulates (`Memory::GetPhysicalAddress`), so masking with
  `& 0x1FFFFFFF` reads one page early -- the Bink luma plane came out rotated by
  4096 mod 1280 = 256 texels. `ghp::HeaderBaseToPhysical` is the one conversion;
  device vertex fetch constants are already physical (the XDK converts).
- **Linear texture footprints.** The readability check rounded the height to
  32 rows, which overran the Bink chroma allocation and skipped its upload
  (green cast). Uploads are now page-granular.

Also: the present fallback only accepts frame-sized resolve destinations
(the bloom chain resolves into 64x64..4x4), DXN (49) decodes to BC5, and Xenos
format 54 (10:10:10:2) has no plume texture format yet.

## Intro videos and menus on screen (2026-09-03)

State: legal warning, title, Bink intro videos (Microsoft, Bizarre, New York
intro) and the main menu all present through the real front-buffer path
(`Swap kind=aperture`), with the menu's blend-state pipeline failure fixed
(see open items for what is left).

### EDRAM aliasing (the "intros went black" regression)

The front-buffer aperture path resolves `0x400D22FC` from whichever surface
is bound at `PGR4_EndFrame_ResolveAndSwap`; the Bink quad is drawn into the raw
backbuffer header `0x4082D1E0` (A8R8G8B8, tile 0) but the end-of-frame resolve
reads an `X8R8G8B8` surface the game created with NULL parameters, also at tile
0. On hardware both are the same EDRAM; on the host they were two textures, so
the front buffer stayed black.

- `rr::CreateSurface(w, h, format, msaa, edramBase)` now keeps a registry keyed
  by `(base, w, h, msaa, depth?)` and hands out one host surface per key; the
  registry holds its own refcount so a guest `Release` of one alias cannot
  free the shared surface. The format is deliberately not in the key: the
  first format seen decides the host format (PGR4's 720p HDR target
  `0x1A2201BF` = `D3DFMT_A2B10G10R10F_EDRAM`, 32 bpp, maps to 16F on the host
  and gets aliased by the 8-bit backbuffer and video target).
- `CreateSurfaceHook` passes `D3DSURFACE_PARAMETERS::Base` (game surfaces all
  carry explicit bases: colour at 0, depth at `XGSurfaceSize` = 0x2D0 for
  720p). NULL parameters go through the XDK's first-fit allocator
  (`D3D::AllocateEdramMemory`), which only ever sees NULL-parameter surfaces;
  PGR4 keeps one alive so it lands at tile 0 -- modelled as base 0.
- `TranslateRawSurface` (XGSetSurfaceHeader surfaces) feeds the same registry
  with `colorInfo & 0xFFF`.

### Immediate-mode draws (the missing UI)

The frame trace showed one hooked draw per frame while the ring carried 24-39
`DRAW_INDX` packets (counted by the PM4 executor and printed as `pm4draws=`
on the Swap line). PGR4's own immediate-mode layer
(`PGR4_ImmBeginVertices_WithDecl` 0x8229CF38, `PGR4_ImmBeginVertices`
0x8229CFE0, `PGR4_ImmAddVertex` 0x8229D0B8, `PGR4_ImmEndVertices` 0x8229D040,
and the screen manager via `j_D3DDevice_EndVertices` 0x82837F70) calls
`D3DDevice_BeginVertices` (0x8269A4F8), which emits the draw packet into the
ring and returns the write-combined slot, then `D3DDevice_EndVertices`
(0x8269A998) which only commits the ring write pointer. Neither reached the
renderer. Both are now in the manifest and hooked: Begin hands out a guest
scratch buffer and records (prim, count, stride); End issues the buffer
through `DrawVerticesUPHook`. That is what put the warning box, title and
menu text on screen.

### Rectangle lists

`D3DPT_RECTLIST` (3 vertices per rectangle) was drawn as a triangle list, so
every UI rectangle lost its second triangle (the menu cloud was cut along the
diagonal). `DrawUserPointerVertices` now expands rect lists on the CPU: the
right-angle corner is found from the POSITION element (perpendicular edges),
the fourth vertex is `va + vb - vc` per float element and per byte
(saturating) for everything else. Vertex-buffer rect lists (`DrawVertices`
with primitive 8) are not expanded -- a wedge in the title/menu background
(lower-left) is probably one of those.

### Scripted controller input for unattended runs

`--pgr4_pad_script=14:START,18:START,30:A` (src/pad_script.cpp) hooks the
game's `XInputGetState` wrapper (0x82673A90, manifest name `XInputGetState`)
with fall-through and ORs scripted presses into the returned state (200 ms
holds, seconds since the first poll). Needed because keyboard emulation
(`mnk_mode`, default off in this SDK) is not driving the game any more and a
virtual-pad driver is the wrong layer.

### XenosRecomp

- Vertex shader `CC80228A287F34B7` (writes r63) fails only when compiled in
  the multi-threaded batch (structured exception or NUL bytes inside the HLSL
  handed to DXC), passes alone and with `--jobs 1`; ASan finds nothing. The
  tool now retries failed shaders serially after the parallel pass
  (`Retried N shaders serially`), so the cache is complete (953/953). Root
  cause still open. The SEH filter now reports code/address.
- The thread-local recompiler reuse was replaced by a fresh instance per
  shader on the way (harmless either way).
- `--dump-hlsl` and the batch path share `tryRecompile(..., failure)`; on a
  DXC failure the batch path writes `failed_<hash>.hlsl` next to the cache.

### Diagnostics added

- FrameTrace prints `skipped=N [nores nodecl psofail psocreate]`; Swap prints
  `pm4draws=`; `D3DDevice_SetRenderTarget` logs the FM2 surface size/format/
  host pointer; `CreateGraphicsPipeline` warns when plume returns null with
  the VS/PS hashes and formats.

### Open items

- (fixed) ~750 menu draws per frame were skipped because
  `Device()->createGraphicsPipeline` returned null: with `PGR4_GPU_DEBUG=1`
  the info queue (now drained on that path via `Video::DumpD3D12InfoQueue`)
  said "DestBlendAlpha is trying to use a D3D11_BLEND value (0x3) that
  manipulates color". PGR4's UI sets `D3DRS_DESTBLENDALPHA = SRCCOLOR`, legal
  on Xbox; `SanitizePipelineState` now maps colour factors in the alpha slots
  to their alpha equivalents. The second cause, "input signature expects
  NORMAL/1 ... but the declaration doesn't provide a matching name" (the
  menu cloud shader `C197ACCF5468DF05`), is handled in
  `CreateGraphicsPipeline`: any semantic in the vertex shader's header
  element list missing from the declaration gets a dummy element on the
  zero-stride slot 15 for that pipeline.
- Vertex-buffer `RECTLIST` draws are still half rectangles.
- Unsupported texture formats seen in the frontend: 4 (1_5_5_5), 5 (5_6_5),
  23 (24_8_FLOAT); surface formats 0x182800B6 (gpu 54) and 0x2DA2ABA4 (gpu 36)
  fall back to R8G8B8A8.
- Keyboard input regression (`mnk_mode` default) not investigated.

