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

## Menu car and textures (2026-09-03, evening)

RenderDoc capture of the main menu (`renderdoccmd capture` with
`PGR4_RDOC_CAPTURE_AT=<present>`, then the renderdoc MCP) showed the frame
structure: shadow map (512x512 depth), a 256x256 reflection pass, the menu
scene into a 1280x480 tile target with ~2300 draws (that is where the car is),
then the 720p UI/tonemap passes. Every car draw reported zero samples passed
and `decode_post_vs_outputs` gave the same clip position for every vertex.

- **Positions**: PGR4 car meshes use `SHORT4` positions (k_16_16_16_16,
  signed, integer flag set; the shader scales them with its own constants).
  FM2's `ConvertPositionDeclType` fell back to `R16G16B16A16_SINT` and the
  translated shader bitcast its uint4 input to float, so the integers read
  as ~0 and the whole mesh collapsed to one clip-space point (RenderDoc:
  identical SV_Position for every vertex, zero samples passed). New
  `SPEC_CONSTANT_POSITION_INT16` (bit 8, `hasInt16Position` on the decl):
  `tfetchPos3N` converts the sign/zero-extended integers with the same
  `.yxwz` half-order fix-up as the FLOAT16 path. Normalized 16-bit positions
  (SHORT4N/USHORT4N) map to SNORM/UNORM instead, so the bitcast sees floats.
- **Packed texcoords / basis vectors**: the car declarations carry
  `UDHEN3N` (k_10_11_11 unsigned normalized) and `UHEND3` texcoords and a
  `DEC4N` (k_2_10_10_10) tangent, which FM2 fed as raw `R32_UINT` with no
  decode (texcoords) or the wrong decode (tangent). New generic path: the
  host publishes a 4-bit mode per texcoord index (`packedTexcoordsLo/Hi`,
  shared constants 484/488) and per basis slot (`packedBasis`, 492);
  XenosRecomp wraps every fetched texcoord/normal/tangent/binormal input in
  `unpackTexcoord` / `unpackBasis` (`unpackVertexMode` in shader_common.h:
  mode = 1 + family*3 + kind, families 10:11:11 / 11:11:10 / 2:10:10:10,
  kinds unorm / uint / snorm). Mode 0 is a passthrough, so FM2's paths are
  unchanged. `PackedVertexMode` in render_state.cpp derives the mode from the
  decl type (bits 0-5 format, bit 8 signed, bit 9 integer).
- **Tile surfaces**: PGR4 tiles 720p as 1280x480 + 1280x240 (FM2: 1280x256);
  `ProcCreateSurfaceHost` now grows any frame-wide surface shorter than the
  frame.
- **Texture formats**: 4 (k_1_5_5_5) and 5 (k_5_6_5) are expanded to
  RGBA8 on upload (`expand16From`); 23 (k_24_8_FLOAT) becomes a
  `D32_FLOAT_S8_UINT` texture filled by depth resolves, never uploaded.
- **Vertex-buffer rect lists**: `DrawVertices` reads stream 0 back on the
  guest thread and routes `D3DPT_RECTLIST` through the user-pointer path so
  the CPU expansion applies there too.
- **Exposure**: the 1x1 luminance target (D3DFORMAT 0x2DA2ABA4, gpu 36) was
  created as RGBA8 while its texture view is R32_FLOAT, so the tonemapper read
  garbage exposure. `ConvertFormat` maps it to R32_FLOAT (and 0x182800B6 /
  gpu 54 explicitly to RGBA8). The exposure now reads ~0.01 for the menu.
- **Cube maps**: fetch dimension 3 (`fc[5]` bits 9-10) now creates a 6-slice
  texture with a `TEXTURE_CUBE` view and uploads the six consecutive faces
  (`UploadGuestTextureFace`, subresource == face). Not yet observed to change
  the menu, kept because cube slots were all on the null descriptor before.

### What the RenderDoc traces say about the remaining look

- The car renders (SHORT4 positions fixed). The title/menu background
  buildings and sky are drawn with a constant-colour pixel shader
  (`SV_Target = c20`, alpha-tested) and the game sets c20 to black for the
  front end, so the black skyline is the game's own look, not a texture
  failure.
- What still differs: the auto-exposure blows out the car and the grey
  backdrop wedges. The scene target's mean is dark (mostly black), the 1x1
  exposure value is ~0.01 and the tonemap scales the 0.13 grey backdrop to
  ~2.2. Whether hardware ends up at the same exposure is unverified; the
  luminance downsample chain (64/16/4/1 targets) and the tonemap constants
  are the next things to compare against a reference capture.
- `debug_pixel` in the renderdoc MCP cannot attach to these bindless draws;
  `pixel_history`, `decode_post_vs_outputs`, `debug_vertex`,
  `read_constant_buffer` and `get_shader` (disassembly) worked and were
  enough.

## Title screen against the reference (2026-09-03, late)

Reference: light grey backdrop, red/black silhouette skyline, black crowd
behind a fence, properly lit red car. From the user-provided captures
(`renderdoccaps/pgr4_ps1.rdc`, `pgr4_ps2.rdc`):

- **Streetlamps white** (EID 14719): the lamp texcoords are `SHORT2`
  (integer 16-bit) and were bit-cast to NaN; `Int16VertexMode` publishes
  unpack modes 10/11 (signed/unsigned int16 -> float) through the same
  `unpackTexcoord` path. Their cube-map slots also sat on the null
  descriptor before cube maps were translated.
- **Crowd stretched off screen** (EID 16295 / 22691): the crowd shader takes
  a per-draw FLOAT16x4 matrix in POSITION1..3 from a stride-0 stream; the
  16-bit lanes need the same `.yxwz` half-order fix-up as POSITION0.
  `g_SwappedPositions` (shared constant 316, the former half-pixel padding)
  carries a per-usage-index mask, set for 16-bit POSITION1+ elements, and
  XenosRecomp wraps those fetches in `swapFloats`.
- **Half rectangles** (EID ~26247 and the diagonal split): the rectangle
  expansion emitted the synthesized triangle with reversed winding, so
  with culling on one half of every expanded rectangle (the scene-to-UI
  composite quad among them) was culled. Fixed per corner case; indexed
  rect lists (`DrawIndexedVertices`) are now de-indexed and expanded too.
- The XenosRecomp serial retry now makes up to four passes (one pass still
  left the flaky r63 shader out of one regen).
- Capture policy: no automated RenderDoc runs; the user drops captures in
  `renderdoccaps/`. `PGR4_RDOC_CAPTURE_DRAWS=<n>` triggers one capture at the
  first frame issuing that many draws if a manual run wants it.
- **Blown-out title scene (exposure)**: the tonemap draw scales the scene by
  a factor derived from a 1x1 R32_FLOAT exposure texture. The adaptation
  draw computes `prev + (1 - exp2(-0.874 * dt)) * (avgLum - prev)`, but
  `prev` sampled as 0 every frame: both 1x1 textures are resolve
  destinations whose guest memory holds zeros, and the once-per-frame
  guest refresh (`FindAndRefreshGuestTexture`) re-uploaded that memory over
  the GPU-resolved value. Exposure therefore stayed at 2% of the target
  (0.01 instead of ~0.5) and the tonemap gain was ~20x. `GuestBaseTexture::
  gpuResolved` is set in `ResolveHook` and `NeedsGuestUpload` skips such
  textures. Ceiling: a texture the game resolves into once and later
  rewrites from the CPU would go stale (none seen so far).
- **Crowd as tiny far-away meshes** (`pgr4_ps3.rdc` EID 17561 and every
  other crowd draw): the crowd instance matrix (3 x FLOAT16_4 rows in
  POSITION1..3) comes from a stride-0 raw stream whose fetch base carries the
  per-draw byte offset. The draw snapshot dropped stride-0 streams, so the
  D3D12 slot kept a stale binding and every person used the same parked
  matrix (translation -768 on all axes, 1100 units away). Stride-0 raw streams
  are now snapshotted per draw (clamped to 256 bytes) and bound with stride 0.
- **Shadow without its mask** (EID 26204): the mask is a stacked texture
  (fetch dimension 3D + stacked bit, `size_stack.depth` slices) sampled as a
  Texture2DArray through the 3D index table. It was translated as a single
  2D image and only published in the 2D slot, so the shader read the null
  3D descriptor. `XenosTextureInfo::arraySize` now covers cube (6) and
  stacked (depth + 1) textures; slices upload like cube faces and
  `BindTextureDescriptor` publishes stacked textures in the 3D slot only.
  Ceiling: slices are assumed to be mip-0 footprints back to back.

## Whole-capture sweep (2026-09-03, `rdoc_sweep.py`)

`tools/rdoc_sweep.py <capture.rdc> <report.txt>` (run with the RenderDoc
source build python, `renderdoc/x64/Development/python/python.exe`, DLL dir +
`pymodules` on the path) walks every draw and flags: used vertex slots that
are unbound or stride 0, 16-bit integer attributes, null descriptors actually
sampled (the 1x1 R8 null textures), all-zero textures sampled, and post-VS
positions that are NaN, all behind the camera, collapsed to a point, or all
off screen. On `pgr4_ps3.rdc` (2028 draws, pre-fix build) it found:

- 1555 draws bind a stride-0 instance slot (crowd + most environment draws),
  so the stride-0 snapshot fix is wider than the crowd.
- 636 draws collapse to a point: the crowd (fixed), and the skinned car in the
  shadow-mask pass (EID 24769.., 5b9e5198/61a77895/c47ce4ce/99dffcba) and the
  skinned depth-only casters (EID 2291.., de39b69c/373746df).
- 67 draws (f8dfd57a) have every vertex behind the camera: a stride-0 slot
  holding a stale 221760-byte crowd table; covered by the stride-0 fix.
- 18 draws sample a null descriptor: 13 with the stacked mask (fixed), the
  rest the tonemap reading a 1x1 game default texture.
- 861 draws read a shader input from the dummy slot 15: inputs the matched
  declaration lacks (NORMAL1/2, COLOR0 on environment draws). On Xenos a
  missing element fetches zero, so this is by design, not a mismatch.

## Vertex textures (2026-09-03)

The skinned car (shadow-mask and shadow-map passes) fetches its bone palette
from a texture in the vertex shader: `SampleLevel(u = 3 * boneIndex, v = 0.5)`
with `tx_coord_denorm` set, through vertex sampler 0. The game binds those
palettes with `D3DDevice_SetTexture(16..19)` (D3DVERTEXTEXTURESAMPLER0..3;
confirmed by tracing the title frame: five different textures at samplers 16
and 19). Two host gaps made the mesh collapse to a point:

- `SetTextureHook` dropped samplers >= 16, and the earlier stopgap in
  XenosRecomp aliased vertex fetch constants 16..31 onto s0..s15, so the
  vertex shader read the pixel stage's slot 0 (the game's 1x1 default).
  SharedConstants now carries a vertex table at 512 (2D), 528 (3D/stacked),
  544 (cube), 560 (samplers), 4 slots each (struct is 576 bytes). XenosRecomp
  `textureIndexOffset`/`samplerIndexOffset` route vertex-shader samplers
  n < 4 there (`reg` from the constant table, or the s16+n alias). The host
  binds samplers 16..19 through `BindVertexTextureDescriptor` with a
  point/clamp sampler (`VertexPointSamplerDescriptor`).
- The recompiler ignored `tx_coord_denorm`; `denormCoord2D`/`2DArray` divide
  texel coordinates by the texture size before sampling.

Unverified until the next capture: the shadow-mask pass output and the crowd
after the stride-0 fix. Re-run the sweep on the new capture.

## pgr4_ps4.rdc round (2026-09-03, late)

- **Under-car shadow (EID 26407)**: the multiply pass samples a 512x512 D32S8
  shadow map that had no writer. `ProcResolveToTexture` always took the color
  RT as its source, so a depth-format destination was a format mismatch and
  was dropped. Depth destinations now read the bound depth surface and copy
  immediately (no deferred StretchRect alias). Verified: the title car casts
  a soft shadow that matches the reference.
- **Scene sampled while bound**: a 1x resolve of the scene was deferred into an
  alias of the live RT, so draws that sample the resolved scene while it is
  still the render target (tail-light refraction, EID 6601) read the target
  being written. `ProcSetTexture` now routes such textures through the
  pending copy. The tail lights still render near-white: pixel history shows
  the lens shader itself outputs (0.90, 0.99, 1.00) with only the scene RT
  and the environment cube bound, so the red tint is not a texture; the
  shader's DXIL cannot be disassembled by RenderDoc (resource-tracking gap),
  so the constant or vertex-colour path it uses is still unknown.
- **Vertex index register**: Xenos starts vertex shaders with r0.x = vertex
  index; XenosRecomp only seeded it in the Unleashed build. The crowd shader
  picks its constant-palette matrix with `trunc((r0.x + 0.5) / 32) * 4`, so
  every person used instance 0. PGR4 vertex shaders now declare
  `SV_VertexID` and seed r0 with it (regen required). Unverified on a capture.
- **Skinned car still collapses**: the 156x1 RGBA16F bone-palette textures
  bound at vertex sampler 16 refresh every frame but guest memory at their
  base (0xEAE12000, 0xEBEBF000, ...) reads all zero, so the CPU never writes
  them. XenosRecomp has no memexport support and the PM4 parser only applies
  MEM_WRITE packets, so a GPU-side palette writer (memexport or a small
  render-to-texture) is the leading suspect. Open.
- The crowd stride-0 stream is an FM2 buffer bound with offset 0 (the whole
  221760-byte table); the per-instance selection comes from the vertex index
  above, not from the stream offset.

## Pixel shader constant file is 256 registers (2026-09-03)

The port inherited FM2's assumption that pixel shaders only use c0..c223.
PGR4's car material (`5B9D6A2FADFF7ADA`, paired with VS `BD0D9DDACFF4A6CA`)
reads `c255`, and the guest device reserves a full 4096-byte float constant
file per stage (VertexShaderF at +0x780, PixelShaderF at +0x1780, VS bool at
+0x2780 -- exactly 256 float4 apart). Two consequences of the old 224 cap:

- XenosRecomp clamped every pixel constant index to 223, so `c255` silently
  read `c223`.
- `ProcSetShaderConstants` drops a write whose range does not fit, so any
  single `SetPixelShaderConstantF` spanning the limit (e.g. c128..c255) was
  discarded whole, not truncated.

`g_pixelShaderConstants` is now 0x400 dwords, `kPsFloatConstantBytes` is
256 * 16, and the recompiler exposes 256 registers for both stages.
`PendingShaderConstantFile::kRegisterCount` was already 256, so the merged
snapshot arrays still fit exactly. No visible regression on the title screen;
this did not by itself fix the tail lights.

### Tail lights: still open

`g_DiffuseColour1` (c128) is (1,1,1,1) and `g_ReflectionColour1` (c130) is
(0.294,0.294,0.294) at the tail-light draw in `pgr4_ps4.rdc`, and the pixel
shader binds no albedo texture (only the scene RT and a 2D array), so the
white lenses follow from the constants the shader is fed. Whether the game
really sets white there, or an earlier constant write is still being lost,
is unresolved. Note `g_pixelShaderConstants` holds *big-endian* guest dwords
(the swap happens in `UploadAndBindRootDescriptor`), so any host-side probe
of those registers must byte-swap before interpreting them.

### Crowd: fix is in, still unverified

All 358 vertex shaders now take `SV_VertexID` and seed r0.x with it, which is
what the crowd shader needs (`trunc((r0.x + 0.5) / c55.y) * 4 + 53` selects
the per-person matrix from the constant palette; c55.y is 32, the per-person
vertex count). The old `0.5 / c55.y` seen in captured DXIL was that expression
constant-folded with r0.x = 0. The crowd is still absent from the title
screen in this build, so either the draws still collapse or something else
hides them -- a fresh capture is needed to tell which.

### Tooling

`tools/rdoc_sweep.py` (whole-capture bug sweep) is joined by two focused
scripts kept in the session scratchpad pattern: dump a draw's bound shader
reflection/disassembly, and dump named constant registers for an event. Both
use the RenderDoc source build's python
(`renderdoc/x64/Development/python/python.exe`, with that directory added as a
DLL dir and `pymodules` on `sys.path`). `XenosRecomp --dump-hlsl <input dir>
<out dir> <shader_common.h>` writes the recompiled HLSL for all 953 shaders,
which is how the car material was identified when RenderDoc could not
disassemble the runtime-linked variant.

## Crowd, evidence from `pgr4-ps1.rdc` (2026-09-03)

Draw 21041 is one crowd person. What is now confirmed *correct*:

- `SV_VertexID` reaches the shader and drives the palette index
  (`trunc((vid + 0.5) / c55.y) * 4`, with the rows read at +52/+53/+54).
- The per-draw palette really is per person and really is uploaded: three
  neighbouring crowd draws give rotations plus translations
  (-46.04, 0.09, -829.67), (-41.33, 0.06, -829.04), (-47.71, 0.07, -825.15).
- The stride-0 instance rows (POSITION1..3, FLOAT16_4) decode through
  `swapFloats` to a clean orthonormal basis, so the 16-bit lane swap is right.
  Their .w lanes are 0xE1FF / 0xE201 / 0xE1FF. The earlier interpretation as
  counters was incorrect: log 149 later traced these to corrupted bone
  translations (see the packed-vector decoder correction below).

What is still wrong: the people land about 1144 units from the camera at
ndc y about -1.0 (draw 21041) and the group summary for the first draw of the
big crowd batch shows only 80 of 4096 sampled vertices with w > 0, i.e. most
crowd vertices are behind the camera. So the mesh is not collapsing, it is
being placed far away, below the frame, and partly behind the eye. The next
step is to find what supplies the crowd's world placement (the palette
translation of about z = -827 is the suspect) and compare it against an object
that lands correctly in the same frame.

## Superseded crowd workaround: dropping matrix translations (2026-09-03)

The crowd became visible but remained in a T-pose. The per-person transform is composed in the
vertex shader as `world = palette * (instanceRot * localPos) + paletteT +
palette * T_instance`, where `T_instance` is the 4th component of the three
FLOAT16_4 instance rows (POSITION1..3) on the stride-0 stream.

Those 4th halves read 0xE1FF / 0xE201 / 0xE1FF, i.e. -767.5 / -768.5 / -767.5,
in the captures examined then. They are corrupted bone translations, not
padding. Feeding them through pushed every person about 1144 units from the
camera and below the frame. Dropping them temporarily hid that corruption,
but also discarded valid translation data. This workaround is now removed;
POSITION1+ uses `swapFloats` and preserves all four components.

Corroborating numbers from `pgr4-ps1.rdc` draw 21041: camera (c24) is
(-41.68, 0.99, -842.23); the palette translation for that person is
(-46.04, 0.09, -829.67), 13.4 units away. The first three lanes of each row
form a clean orthonormal basis, so the 16-bit lane swap itself was right.

The observation above established visibility only, not correct skinning.

## Sampling the bound render target

`pgr4_ps4.rdc` draw 6601 had the same host texture as both SRV and RTV -- the
tail-light lens samples the resolved scene, and EDRAM aliasing maps that onto
the live colour target. Sampling a bound target is undefined. `GuestSurface`
now carries a `selfSampleTexture` scratch copy; when a slot would bind the
surface that is currently the render target, the binding is redirected to that
copy and registered as a deferred StretchRect, which `FlushRenderState`
already drains before it binds render targets. The D3D12 debug layer reports
no hazard afterwards. This did **not** change the tail lights.

### Tail lights: what is now ruled out

Both the car body draw (6287) and the tail-light draw (6623) run the same
material with `g_DiffuseColour1` = (1,1,1,1); the body gets its red from an
albedo texture, and the lens draw binds no albedo texture at all -- only the
scene target and a 2D array. So the lens is being fed white and paints white.
Ruled out so far: the 224-register constant cap, stale/missing constants
(c128..c132 verified live per draw), and sampling the bound target. Still to
check: where the red is meant to come from for a draw with no albedo texture
-- the remaining candidates are the 2D array binding and the vertex colour
path (the VS forwards only COLOR0.x, with a decoded normal in TEXCOORD6.yzw).

## Earlier palette-texture hypothesis (2026-09-04, unresolved)

The crowd is placed correctly now but every figure stands in its bind pose and
the flags do not move. Two separate findings:

1. **UBYTE4 texcoords were a vertex format type mismatch (fixed).** The crowd
   declaration carries bone indices in TEXCOORD2 as `D3DDECLTYPE_UBYTE4`
   (0x1A2286) and weights in TEXCOORD3 as UBYTE4N. `ConvertDeclType` bound
   UBYTE4 as `R8G8B8A8_UINT`, but every recompiled shader declares its texcoord
   inputs as `float4`; a UINT vertex format against a float input register is a
   type-class mismatch and delivers nothing usable. UBYTE4 texcoords now bind
   `R8G8B8A8_UNORM` and publish unpack mode 12, which multiplies by 255 in the
   shader to recover the integer. Honest note: this did **not** change the
   crowd visually, so it was not the T-pose cause -- it is a correctness fix
   that stands on its own.

2. **The bone palette textures contain nothing.** The skinned passes sample a
   156x1 R16G16B16A16_FLOAT texture as a vertex texture (e.g. `pgr4-ps1.rdc`
   draw 2506, resource 4394). Its only usages in the whole frame are a CopyDst
   at event 80 -- our own `UploadGuestTextureData` -- and a barrier. Nothing
   renders into it, and the guest memory it uploads from reads all zero every
   frame (logged across frames 438-445 at bases 0xEAE12000 / 0xEBEBF000 /
   0xEBEC3000). So the palettes are zero and the skinning matrices are zero.

   This may explain the skinned car collapsing in shadow passes, but it does
   not establish the crowd's cause. Log 149 confirms that the crowd's CPU
   vertex-buffer bone palette is written, with corrupt animation inputs.

   **Memory export is ruled out** (2026-09-04). A diagnostic build of
   XenosRecomp scanned all 953 shaders for ALU exports to registers outside
   the interpolator range: exactly one shader hits that path, and its target
   is register 63 (point size / edge flag / kill vertex), not eA/eM0..4
   (32..37). No PGR4 shader uses memory export, so the palettes are not
   written by the GPU that way. Remaining candidates: the palette base address
   we parse from the fetch constant is not where the game writes, or the game
   populates it through a path the port does not intercept.

   Note the crowd's *main* pass (draw 21041) binds no textures at all, so
   confirm which crowd pass consumes the palette before building this.

## Export register 63 was undefined behaviour (fixed 2026-09-04)

The one shader that writes Xenos export register 63 fell through the vertex
export switch into `interpolators.find(...)`, whose result was dereferenced
without checking for `end()`. That is what intermittently produced a shader
full of NUL bytes and dropped `CC80228A287F34B7` from the cache, leaving 952
of 953 entries and a flaky 4-attempt retry. Unhandled export registers now
write to a declared `exportSink` local, and the cache regenerates at 953
deterministically. Point size and kill-vertex are discarded, which is what the
previous behaviour intended; nothing in the title screen uses them.

## Packed-vector decoder correction (2026-09-04, flags confirmed)

`pgr4_recompiled_149.log` captures a 22-bone crowd palette at the writer
`sub_823DFAA0`: local translations are already (-768, -768, -768), with NaNs
in bones 10 and 13. Propagation carries those NaNs into the arms, half packing
turns them into 0x7FFF, and `sub_823D2B20` reads them as positions near 131008
for the flags. Log 148 separately confirms that cloth initialization is
healthy and the vertex writer faithfully copies the corrupt simulation data.

The translation evaluator `sub_823BD9D8` uses `vupkd3d128` type 6. Its shared
code generator read the wrong 64-bit half, shifted a 32-bit value by 44,
reversed output lanes, and converted to numeric floats instead of adding the
required mantissa biases (3.0 for signed XYZ, 1.0 for unsigned W). The corrected
emission snapshots guest words 2-3, extracts signed 20-bit XYZ and unsigned
4-bit W, and handles the reserved negative endpoint as QNaN. No PGR4 pose
override is used. The shader workaround that discarded matrix .w is removed.

Five assembly regression cases exercise zeros, signed values, limits, reserved
values, and aliased/separate registers: the old generated code fails 20
assertions; the corrected code passes all. An isolated harness of the actual
PGR4 translation evaluator returns (-768, -768, -768) before the fix, then
correctly decodes and interpolates positive and negative (1,2,3) to (2,4,6)
tracks after it. The installed generator, generated guest code, shader cache,
and Plume executable have been rebuilt. The user confirms the flags are back.
Log 150 reports finite attachment positions for all 12 flags (around 1.7-1.9
units high), no saturated palette samples, and no cloth anomalies. The user
confirmed that the crowd still T-poses; the fence and tail-light fixes were
confirmed earlier.

## Indexed bone-matrix vertex fetch (2026-09-04, crowd visibility confirmed)

The crowd shader `1FA9B696872B63EA` computes `r0.z = boneIndex + paletteBase`
and uses it as the source of vfetch instructions 17, 18 and 19 (POSITION1..3).
XenosRecomp discarded that source and read ordinary input-assembler attributes,
so fixing the CPU animation did not make the crowd use its animated bones.

Direct disassembly of `sub_823D3078` confirms stride 24 at `0x823D30BC` and
byte offset `24 * paletteOffset` before `D3DDevice_SetStreamSource`. The
decompiler's apparent stride zero is misleading. The bound declaration uses
stream 1, offsets 0/8/16, FLOAT16_4, matching three rows per bone.

Non-default-index POSITION1..3 fetches now read shader-visible vertex buffers
using the source register, declaration format/offset, and bound stream stride.
The D3D12 renderer binds three root SRVs to its existing geometry views and
publishes range metadata at SharedConstants offset 576. Half-floats decode in
guest component order from DWORD-swapped uploads. Bounds are checked before
address multiplication. Missing streams and unsupported formats return zero.
Snapshots retain the full palette. No shader hash selects this behavior.

Scope: indexed FLOAT16_2/4 and FLOAT1..4 position inputs. Other indexed semantics
are not implemented. DXIL and SPIR-V compile the buffer-fetch path; this PGR4
renderer binds it on D3D12. The Metal output retains its previous IA path.

`tools/XenosRecomp/tests/indexed_position_fetch.cpp` runs the actual HLSL helper
on D3D11 WARP: 21 cases cover two distinct bone transforms, all supported float
formats, zero stride, missing/short buffers, invalid indices, and unsupported
formats. An optional crowd HLSL dump checks that all three rows consume r0.z.
The user confirms the crowd is now visible. Animation correctness was not
separately confirmed.

## Arcade chapter-card UI stride (2026-09-04, runtime confirmation pending)

Compared `pgr4-arcade1.rdc` with the known-good `pgr4-arcadexenos.rdc`.
Bad EID 46029 composites texture 1729, copied from offscreen card target 1688
at EID 4511. The corruption starts earlier: EIDs 4398-4483 overlay text and
borders after the stride-36 medal meshes. The font and border atlases match
the good capture (bad textures 393/378 versus good 2837/2834 at EIDs 474/498).

Those bad UI draws bind stride-44 vertex data: FLOAT4 position at 0, packed
color at 16, and FLOAT2 texcoords at 20/28/36. Their pipeline instead reads
FLOAT3 position at 0, FLOAT4 color at 12, and FLOAT2 texcoord at 28. This
feeds position.w and packed-color bits into float color, and selects the wrong
UV channel, explaining the discoloration and unrelated atlas fragments.

`ProcDrawPrimitiveUP` overrode only the PSO stride before `FlushRenderState`.
`EffectiveStream0Stride` prefers the input-slot stride, so the previous mesh's
36 bytes rejected the queued 44-byte declaration and selected a 36-byte
fallback. The UP buffer was bound only after that pipeline was chosen.

The shared UP path now installs the uploaded buffer view and both stride
mirrors before flushing. Declaration selection, indexed-fetch metadata, and
vertex binding therefore see the same geometry. It restores the previous
stream and dirties the binding after the draw, including failed pipeline setup.
This also covers Begin/EndVertices and rectangle draws routed through UP.

`python scripts/tests/test_up_vertex_stream.py` compiles the actual UP draw,
declaration selection, and stride validation functions against the real renderer
types with a fake upload/command backend. All 32 transitions pass, covering
previous strides 0/8/36/44, direct/indexed draws, failed upload/setup, stream
restoration, and preservation of the palette stream. Device-lost early exit
also passes. A temporary negative control retaining the stale input-slot
stride fails declaration selection as expected. The Plume executable rebuild
passes. The same Arcade menu still needs user visual confirmation.

## Race freeze and upload duplication (2026-09-04, race retest pending)

The user reports audio continuing while the last loading-screen frame remains.
Run 153's surviving logs contain continuous 64 MiB upload-chunk allocation
failures and small texture-staging allocation failures. More than 100 MB of
rotated logs cover only the final 25 seconds, so the original failure is gone.
These logs establish failed GPU resource creation, but do not distinguish
memory exhaustion from an earlier device-removal fault.

Direct replay inspection of the existing Arcade captures found:

| Measurement | Native `pgr4-arcade1.rdc` | Xenos `pgr4-arcadexenos.rdc` |
|---|---:|---:|
| File bytes | 697,824,975 | 150,500,697 |
| Buffer resource bytes | 1,244,515,152 | 675,927,102 |
| Texture resource bytes | 129,682,756 | 448,441,152 |
| Serialized initial contents bytes | 1,331,122,832 | 1,107,820,894 |
| Coherent mapped write chunks / bytes | 8 / 536,871,264 | 27 / 48,569,826 |

The native frame section decompresses to 1,893,318,912 bytes. Eighteen captured
buffers are 64 MiB each. Reading the vertex/index binding ranges from the
structured capture and comparing their actual bytes gives 8,142 ranges with
459,966,180 aggregate bytes, but only 1,788 distinct payloads totaling
27,157,188 bytes. This is repeated binding content, not a measurement of the
new build's frame time or final capture size.

`ProcSetDrawGeometrySnapshot` uploaded every full raw vertex/index snapshot on
every draw. Shader constants were also uploaded afresh each time. The per-GPU-
slot upload allocator now shares identical immutable payloads, using XXH3 for
lookup and an exact byte comparison before reuse. A changed final bone creates
a new version; no palette is shortened to the visible draw's vertex count.
Recorded command batches share their identical owned payloads too. GPU cache
entries clear only when the owning slot's fence has retired. Producer-side
guest snapshots keep their existing staging path and lifetime.

Upload buffers now map with an empty read range and unmap their actual written
range before normal or wait-for-GPU submission. Reusing the slot remaps as
needed. This avoids reporting every byte of an entire persistent mapping as
newly written. UploadStats reports written/reused MiB at the existing trace
cadence. Allocation failures stop further upload attempts for that slot and
query the actual D3D12 removal reason, latching confirmed removal through the
existing diagnostic path. Draws with failed constant uploads are skipped.
Texture-staging failures use the same removal check and bounded logging.

`python scripts/tests/test_upload_cache.py` compiles the actual allocator and
snapshot/batch types with a memory-backed GPU substitute. Reuse across 2,000
full-palette requests, changed-byte versions, endian tails, exact written
ranges, slot reset/remap, create/map failures, and recorded ownership pass.
Disabling cache lookup in a temporary negative control fails the reuse check.
The previous 32 UP stride cases still pass, and the Plume executable rebuilds.
No game launch was automated. Race presentation, FPS, and a new capture's size
remain to be measured on the rebuilt executable; per-draw producer copies and
texture refresh work remain performance candidates.

The user's subsequent run still has abysmal FPS. No performance improvement
is confirmed; the race presentation result was not separately reported. This
is a checkpoint of the rendering fixes and upload work, with performance open.

## 2026-09-04: first Tracy capture and texture upload CPU reads

`renderdoccaps/pgr4_main.tracy` (26,251,987 bytes, captured at 17:51:41)
contains 6.132 seconds of recorded CPU work. Main XThread, OS TID 20204,
spends 5.215 seconds in 33,851 `D3DDevice_SetTexture` calls (85.1% of the
recorded interval), followed by 0.536 seconds in indexed draws. Scheduling
data reports 5.050 seconds actually running on this thread. Of its 40,071
periodic CPU samples, 29,468 land in the texture upload's inlined 16-bit
byte swap. Stacks lead through `FindAndRefreshGuestTexture` and
`UploadGuestTextureData`, confirming CPU texture conversion as the first
optimization target. Other game threads have much less running time. Tracy's
own symbol worker also runs for 4.789 seconds; allow symbol resolution to
settle before the next measurement.

Non-expanded textures were untiled directly into a mapped D3D12 upload heap,
then byte-swapped in place. Upload heap memory is write-combined, making those
CPU reads expensive. The row scratch already used for 16-bit expansion now
serves every format. Untiling and swapping happen in ordinary CPU memory,
followed by writes to the upload heap. Sparse-page zeros, mip layout, expansion,
and refresh frequency are preserved. Mapping declares no CPU reads, and
unmapping reports the complete written range.

`python scripts/tests/test_texture_upload_rows.py` passes 84 cases covering
linear/tiled layouts, endian modes, 16-bit expansion, unreadable pages, row
padding and partial endian groups. It executes the production row loop and
rejects byte-swapping the upload destination. A separate Windows
`PAGE_WRITECOMBINE` microbenchmark of the old/new production loops, with a
1 MiB output and 16-bit swapping, measured median 12.725 ms versus 0.464 ms
(27.4x), with identical bytes. This measures conversion, not gameplay FPS.
Raw analysis and benchmark artifacts are under `out/tracy-pgr4-main/`.

This capture has no GPU zones or native game frame markers; its default frame
set contains initialization, missed time, and the entire recorded interval.
Successful native swapchain presentation now calls `Profiler::Flip()` so the
next capture can be analyzed by game frame. The RelWithDebInfo Plume game
rebuild passed. Visual correctness and gameplay FPS await a fresh user run.

## 2026-09-04: second Tracy capture and unused diagnostic hashing

`renderdoccaps/pgr4_main2.tracy` (25,915,046 bytes, captured at 18:04:31)
records 3.742 seconds. Native presentation markers now work: excluding the
partial first/last intervals, 31 complete frames average 119.10 ms (8.40 FPS).
The final 11 complete frames average 101.13 ms (9.89 FPS). Tracy's symbol worker
still has 2.910 seconds of recorded running time, so this includes profiling
startup overhead and is not an unprofiled FPS measurement.

`SetTexture` averages 49.15 us over 45,629 calls, versus 154.05 us in the first
capture (3.13x lower time per call). It still occupies 2.243 seconds, or 59.9%
of the recorded interval. Indexed draws occupy 0.890 seconds. In representative
GUI frame 432 (95.10 ms), texture binds take 56.63 ms and indexed draws 22.60 ms.
Main-thread samples now show geometry snapshot byte swaps as a major CPU cost;
the former texture byte-swap hotspot has dropped substantially.

Render thread 73624 has 2.293 seconds of recorded running time. At least 3,016
of its 17,974 periodic samples (16.78%) have `MixFrameTrace` on their stack.
This is a lower bound from the top 500 stacks, covering 64.3% of its samples.
The code hashed complete vertex/index payloads and draw state every frame,
although diagnostic output only used frames 1..64, 301, 601, etc. Collection
now follows that same cadence. Rendering, upload-cache content verification,
draw counters, and explicit capture triggers retain their previous behavior.

`python scripts/tests/test_frame_trace_cadence.py` exercises the production
collection and hashing functions: 67 of the first 1,000 frames hash data, and
the other 933 do not access their payloads. The RelWithDebInfo Plume executable
rebuild passed. Evidence is in `out/tracy-pgr4-main2/`; the effect of this
diagnostic change still requires a same-scene recapture. Texture refresh work
and repeated full geometry snapshots remain performance targets.

## 2026-09-04: third Tracy capture and producer geometry reuse

`renderdoccaps/pgr4_main3.tracy` (26,487,990 bytes) records 3.701 seconds,
from 148189587855 to 151890379172 ns. Its 35 complete native frames (indices
3..37, excluding initialization, missed time and partial edges) average
104.82 ms / 9.54 FPS, versus 119.10 ms / 8.40 FPS in main2. The median is
103.13 ms. These are separate short captures with variable frame times,
not a controlled measurement of the diagnostic change's FPS effect. There
are still no GPU zones, and Tracy's symbol worker ran for 2.416 seconds.

Main XThread 42972 spends 2.147 seconds in 51,056 texture binds (42.05 us
per call) and 0.934 seconds in 69,894 indexed draws (13.37 us per call).
GUI frame 4227 takes 103.13 ms: 59.39 ms in texture binds and 26.92 ms in
indexed draws. The main thread has 15,629 periodic samples; 4,096 (26.2%)
land in the dword byte-swap symbol. Top stacks trace that work through
`QueueDrawStateSnapshots` and its full raw geometry snapshot conversion.

Render thread 62776 has 18,073 samples. Its top 500 stacks cover 65.1%:
only 11 samples have `MixFrameTrace` in the stack, versus 3,016 in main2's
top 500. At least 6,797 (37.6% of all render-thread samples) still have
`UploadAllocator` on their stack, including content hashing and byte checks.
These stack counts are lower bounds, and overlapping categories must not
be added. Full evidence is saved under `out/tracy-pgr4-main3/`.

`SnapshotRawPhysicalBuffer` now uses the existing `ByteSnapshotCache` owned
by the intermediary allocator. Each draw still validates access and hashes
the entire guest range, followed by exact byte comparison before reuse.
Unchanged geometry reuses its immutable converted bytes; any changed byte
creates a separate version for later draws. The cache now supports both
16-bit indices and 32-bit data, preserving trailing bytes. Its mutex and
queue-drain reset follow the intermediary allocator's existing lifetime;
recorded batches retain their own copies. Full palettes, texture refreshes
and the GPU upload cache's content checks remain intact.

`python scripts/tests/test_upload_cache.py` passes with normal hashes and
with forced collisions. It executes the production guest snapshot helper,
CPU/GPU allocators and recorded batch type, checking concurrent producer
reuse, changed final palette elements, endian modes, invalid ranges, lost
access, reset and independent batch lifetime. A seven-measurement median
microbenchmark of 2,000 unchanged 221,760-byte snapshots with the actual
old/new helpers measures 27.879 ms versus 16.713 ms, with identical bytes.
This is snapshot throughput, not a gameplay FPS result. The existing
RelWithDebInfo Plume target rebuilt successfully with the throttled launcher.
Gameplay performance and crowd/flag correctness require the next user run.

## 2026-09-04: fourth Tracy capture and repeated-source hash avoidance

The user confirmed the preceding build's visuals and supplied
`renderdoccaps/pgr4_main4.tracy` (27,214,187 bytes). It records 3.721 seconds,
from 18140679077 to 21862140884 ns. Complete native frames 3..38 average
101.15 ms / 9.89 FPS, versus 104.82 ms / 9.54 FPS in main3; the median is
99.98 ms. This modest difference between separate captures is not proof of
a material gameplay improvement. There are no GPU zones, and Tracy's symbol
worker still ran for 2.469 seconds during the captured interval.

Main XThread 87192 spends 1.887 seconds in 53,085 texture binds (35.55 us per
call) and 1.005 seconds in 72,985 indexed draws (13.77 us per call, versus
13.37 us in main3). GUI frame 379 takes 99.89 ms: texture binds occupy
48.09 ms and indexed draws 28.57 ms. The top 500 main-thread stacks cover
47.9% of its 17,474 periodic samples. Dword-swap stacks drop from 4,075
samples (26.1% of all samples) in main3 to 223 (1.3%) in main4. The cache
now accounts for substantial hashing and byte comparison work: 1,605 samples
have XXH3 on their stack. Render thread 76660 has 17,554 samples; its top
500 stacks cover 61.7%, including 2,197 XXH3 samples and 6,243 samples in
the upload allocator. These categories overlap and the counts are lower
bounds. Raw capture evidence is under `out/tracy-pgr4-main4/`.

The shared byte snapshot cache now remembers a source address's previous
content hash as a lookup hint. It compares size and every byte against the
immutable cached payload before reuse, avoiding another full hash pass for
unchanged sources. Misses still hash and perform exact collision checks.
When a source's hash changes, its next lookup hashes first until the content
stabilizes, avoiding an additional comparison against the prior version on
every changing draw. Hints contain hashes rather than entry pointers, so
cache copies remain independent; Clear removes hints with their entries.
Existing producer locking and queue/GPU/batch ownership remain in force.

The upload-cache check passes with real hashes and forced collisions. Added
checks cover hash avoidance, size changes and address aliases, cache copying,
reset, and a changing source becoming stable again. A seven-measurement
median benchmark of 2,000 producer-plus-GPU snapshots of 221,760 bytes shows
the unchanged case at 33.989 ms before versus 14.121 ms after, reducing
4,000 hashes to 2 with identical output. The mutation-heavy case (last byte
changes every draw, cycling through 256 versions) regressed from 84.316 to
100.096 ms despite fewer hashes. Additional alternating executions varied
from 83.192..91.818 ms before and 89.608..94.618 ms after. A producer-only
probe was approximately neutral for mutations (51.253 vs 52.693 ms), putting
the remaining tradeoff in consumer validation of many distinct snapshots.
This is a candidate optimization for the observed repeated-buffer workload;
the benchmark does not establish a gameplay gain. The existing Plume
RelWithDebInfo target rebuilt successfully through the throttled launcher.
The next same-scene user capture must decide the end-to-end result.

## 2026-09-04: fifth Tracy capture and per-texture GPU fence stalls

`renderdoccaps/pgr4_main5.tracy` (26,788,702 bytes) records 3.387 seconds,
19797161718..23184598238 ns. Complete native frames 3..38 average 77.38 ms
/ 12.92 FPS, versus main4's 101.15 ms / 9.89 FPS; the median is 72.93 ms.
The trailing 541.19 ms interval has no closing present and is excluded.
The full 31% FPS difference cannot be attributed to the cache change:
later frames have fewer draws, and recorded Tracy symbol-worker running
time drops from 2.469 to 0.723 seconds. Main/render scheduling data also
leaves 1.219 seconds uncovered, so it cannot describe the entire interval.
There are still no GPU zones.

Hashing fell in the sampled hot stacks: main-thread XXH3 stacks account for
230/10,186 samples (2.26%) versus 1,605/17,474 (9.19%) in main4; render-thread
XXH3 stacks account for 171/10,706 (1.60%) versus 2,197/17,554 (12.52%).
These are lower bounds from the top 500 stacks (48.3% main / 57.9% render
coverage in main5). Content comparisons and memory copies remain prominent.

Texture binds occupy 1.522 seconds across 49,092 calls (31.00 us/call),
and indexed draws occupy 0.705 seconds across 69,844 calls (10.09 us/call).
GUI frame 350 has 1,435 texture binds / 1,987 indexed draws, comparable to
main4's frame 379 with 1,436 / 1,989. Its frame duration is 81.32 versus
99.89 ms; texture binds take 43.97 versus 48.09 ms, indexed draws 18.37
versus 28.57 ms. Later main5 frame 364 takes 58.06 ms but has only 1,188
texture binds / 1,737 indexed draws. Evidence and complete thread analysis
are in `out/tracy-pgr4-main5/`; one zero-sample thread has no scheduling data.

The remaining texture-upload path submits a separate copy command list and
waits for its GPU fence for every translated texture. The shared texture-copy
handler now records copies on the existing graphics frame command list and
transfers staging ownership to `RetainTempUploadBuffer`. Buffers are released
by the existing frame-slot retirement path after that slot's fence completes.
Copies stay ordered with draws and subsequent updates to the same texture.
`RenderQueue::Run` still consumes the borrowed region array synchronously,
but no longer waits for a texture's GPU copy. Translated base/mip/array uploads,
DDS mip chains and the dedicated LockRect staging fallback all transfer their
buffer exactly once; DDS levels are submitted as one region batch. Byte
conversion, guest refresh frequency and GPU-resolved texture protection are
unchanged. Buffer-copy commands keep their existing synchronous semantics.

`test_texture_upload_lifetime.py` executes the production copy handlers,
retention, slot retirement and recording classification with a delayed command
executor. It verifies draw/copy ordering, mip/array footprints, borrowed region
consumption, survival across retirement of the other frame slot, exact cleanup
after the owning slot retires, and cleanup on invalid arguments or device loss.
The 84 texture-row conversion cases also pass. The RelWithDebInfo Plume game
rebuilt through the throttled launcher. No game launch was automated; visual
correctness and the effect of removing GPU waits require the next user run.
Staging memory now remains live for its GPU frame rather than one upload.

## 2026-09-04: sixth Tracy capture and immutable geometry upload identities

`renderdoccaps/pgr4_main6.tracy` (26,391,414 bytes) records 3.348 seconds,
14399027768..17746707694 ns. Its 49 complete native frames (indices 3..51)
average 57.55 ms / 17.38 FPS, versus main5's 77.38 ms / 12.92 FPS; the median
is 58.90 ms. The first partial and final unclosed 497.60 ms intervals are
excluded. Later frames still draw a lighter workload, so the overall FPS
ratio is not a controlled measurement of just the upload change. GPU zones
remain absent; Tracy's symbol worker records 1.756 seconds of running time.

Texture binds take 1.071 seconds across 66,406 calls (16.12 us/call versus
31.00 us in main5). Indexed draws take 0.860 seconds across 93,505 calls
(9.19 us/call versus 10.09 us). Main thread 84716 records 0.491 seconds
waiting versus main5's 0.861 seconds over nearly equal scheduling coverage
(2.141 versus 2.169 seconds); 1.206 seconds of main6 is uncovered. GUI frame
326 has 1,436 texture binds / 1,990 indexed draws and takes 67.97 ms, with
25.97 ms in texture binds and 19.45 ms in indexed draws. Main5's comparable
frame 350 had 1,435 / 1,987 calls, taking 81.32 ms overall and 43.97 ms in
texture binds. Evidence is saved under `out/tracy-pgr4-main6/`.

Render thread 81164 has 11,497 periodic samples. Its top 500 stacks cover
67.8%, with at least 4,500 samples (39.1% of all samples) under the upload
allocator and 1,701 (14.8%) under the byte snapshot cache. These categories
overlap. The largest stack is the exact byte comparison from
`ByteSnapshotCache::Find` through `UploadAllocator::UploadCached`. The
consumer still makes and validates a second CPU copy of geometry that the
producer already snapshotted immutably.

The snapshot cache now optionally supplies a process-wide identity for each
immutable payload and endian variant. IDs survive ordinary reuse and are
never based on a guest address; new cache generations get new IDs, preventing
staging address reuse from aliasing an old GPU upload. Draw snapshots carry
these host identities, and recorded batches assign identities to their own
owned copies. The GPU frame allocator uploads each identity directly once
and reuses its buffer reference without a second CPU copy/hash/byte scan.
It clears these references only at the existing frame-fence retirement.
Identity zero retains the full content-checking path for other callers.
All guest access and full-payload validation still occur on the producer.

The production upload-cache check passes with normal hashes and forced
collisions. Added checks cover changed final palette elements, endian-variant
identity separation, recorded ownership, source-cache retirement, GPU reuse,
zero-ID fallback and device loss. The RelWithDebInfo Plume target rebuilt
with the throttled launcher. A seven-measurement median of 2,000 producer-plus-
GPU snapshots of 221,760 bytes measures 13.796 -> 7.704 ms for unchanged data
and 85.331 -> 67.378 ms when the last byte changes every draw. Output bytes
match in both cases. These are isolated CPU/upload benchmarks; the next user
run must confirm gameplay performance and graphics correctness.

## 2026-09-04: seventh Tracy capture and render queue overhead

`renderdoccaps/pgr4_main7.tracy` (31,232,738 bytes) records 4.928 seconds,
17092618092..22020311861 ns. Its 76 complete native frames (indices 3..78)
average 64.60 ms / 15.48 FPS, versus main6's 57.55 ms / 17.38 FPS; median
is 62.98 ms. Initialization, missed frames, the first partial interval and
the final unclosed interval are excluded. This capture does not establish an
overall FPS gain from the immutable geometry upload change. GUI frame 409
has 1,438 texture binds and 1,990 indexed draws in 67.56 ms, close to main6's
frame 326 (1,436 / 1,990 calls in 67.97 ms). Frame 430 has nearly the same
call counts but takes 83.86 ms. Scene cost and scheduling vary within a run.

SetTexture takes 1.864 seconds over 109,848 calls (16.97 us/call versus
16.12 us in main6). Indexed draws take 1.504 seconds over 151,366 calls
(9.93 us/call versus 9.19 us). Main thread 99892 has 3.711 seconds recorded
running and 0.839 seconds waiting; render thread 48656 has 2.694 seconds
running and 1.856 seconds waiting. Each has about 0.378 seconds uncovered,
versus main6's 1.206 seconds, so raw running/waiting totals are not directly
comparable. Tracy's symbol worker records 3.446 seconds running. GPU zones
remain absent. Raw capture, frame, zone and stack evidence is saved under
`out/tracy-pgr4-main7/`.

The render thread's top 500 stacks cover 13,110 of 20,435 periodic samples.
ByteSnapshotCache now accounts for at least 813 samples (3.98% of all samples,
down from 14.80%); XXH3 accounts for 20 (0.10%, down from 2.52%). Remaining
cache work includes shader constants. UploadAllocator accounts for 4,468
(21.86%, down from 39.14%). These are overlapping lower bounds, not additive
time percentages. Queue mutex acquisition/release and condition-variable
paths are now prominent; the largest individual stack is the worker's mutex
release. The main thread still spends substantial sampled work capturing
geometry and converting/uploading textures.

Every queue Job previously embedded and moved a 3,808-byte deferred execution
snapshot, including ordinary state changes that never used it. The existing
Job now owns an optional snapshot allocation only for replays. The worker
swaps the pending deque into its local deque under the queue mutex, then
dispatches and retires jobs in FIFO order outside that mutex. This removes
per-command dequeue locking, default initialization and moves. Producer bulk
atomicity, synchronous Run completion, nested render-thread dispatch and
recording/GPU mutex behavior are preserved. Replay templates, execution-time
snapshots and payloads keep their independent ownership.

`python scripts/tests/test_render_queue.py` compiles the actual queue with
stub dispatch handlers and passes FIFO, concurrent bulk ordering, synchronous
and nested Run, queued command copies, replay ownership after input mutation
and recording replacement, shutdown drain and restart checks. The
RelWithDebInfo Plume game target rebuilt with the throttled launcher. Jobs
shrink from 4,544 to 736 bytes. Seven measured runs of 200,000 commands give
median queue-only times of 127.707 -> 8.830 ms for single-command submissions
and 128.058 -> 15.862 ms for eight-command batches, with matching checksums.
These isolated tests do not include GPU dispatch or prove gameplay speed;
the next user-driven capture must measure that and confirm visual behavior.

## 2026-09-04: eighth Tracy capture and contiguous texture copies

`renderdoccaps/pgr4_main8.tracy` (26,421,359 bytes) records 3.297 seconds,
12615251864..15911993425 ns. Its 57 complete native frames (indices 3..59)
average 56.82 ms / 17.60 FPS, versus main7's 64.60 ms / 15.48 FPS, with a
56.10 ms median. Initialization, missed frames, the first partial interval
and the final unclosed interval are excluded. The measured FPS increases
13.7%; scene variation and profiler overhead prevent attributing the entire
increase to the queue change. GPU zones remain absent. GUI frame 298 takes
56.52 ms for 1,439 texture binds and 1,988 indexed draws, versus main7's frame
409 at 67.56 ms for 1,438 / 1,990 calls. Texture binds in those frames take
23.95 versus 24.69 ms; indexed draws take 15.87 versus 21.62 ms.

Across the capture, SetTexture takes 1.353 seconds over 83,685 calls
(16.16 us/call versus 16.97 us in main7). Indexed draws take 0.937 seconds
over 115,466 calls (8.12 us/call versus 9.93 us). Main thread 89028 records
2.325 seconds running, 0.590 seconds waiting and 0.382 seconds uncovered;
render thread 80132 records 1.633 seconds running, 1.284 seconds waiting and
0.380 seconds uncovered. Tracy's symbol worker records 1.619 seconds running.
Capture/zone/frame evidence and sample stacks are under `out/tracy-pgr4-main8/`.

The render thread's top 500 stacks cover 7,461 of 12,321 periodic samples.
Mutex acquisition/release paths fall to at least 2.87% / 2.92% of all samples,
from main7's 7.41% / 6.13%. Remaining lock stacks include command recording;
the former worker dequeue-lock hotspot has receded. The main thread's top
500 stacks cover 9,238 of 18,067 samples. Geometry snapshots account for at
least 5,282 samples (29.24%) and UploadGuestTextureData for 2,452 (13.57%).
These categories are overlapping lower bounds. Of the texture-upload samples,
1,948 land at the tiny per-block memcpy on line 1215 and 238 at the tiled
address calculation. This motivates reducing block-copy calls without changing
texture freshness or resource lifetime.

The upload row loop now copies contiguous source runs: full readable linear
rows, or the SDK's documented tiled X runs (8 bytes for 1-byte blocks,
16 bytes otherwise). Packed offsets bound each tiled run correctly. Partial
extents and unreadable pages fall back to the previous per-block validity and
zero-fill behavior. The loop specializes the five supported block sizes once
per region so the compiler can inline the small copies and simplify address
arithmetic. Endian conversion still happens in cacheable CPU row memory, and
only writes reach the GPU upload mapping. Existing layout, per-frame refresh,
GPU-resolved texture protection and upload-buffer retirement remain in place.

The production row-conversion check passes 504 cases, including linear/tiled
layouts, all existing endian/16-bit expansion modes, packed and slice offsets,
truncated extents, unaligned page crossings, sparse pages and output padding.
The RelWithDebInfo Plume target rebuilt with the throttled launcher. Seven
measured runs converting eight 1024x1024-block surfaces give median tiled
times of 23.819 -> 2.530 ms (1-byte blocks), 23.886 -> 3.364 ms (2-byte),
27.260 -> 6.319 ms (4-byte), 28.572 -> 16.920 ms (8-byte), and 37.696 ->
35.877 ms (16-byte). Linear cases improve from 16.265..24.060 ms to
0.325..9.160 ms, depending on block size. Before/after output checksums match.
These are CPU conversion benchmarks; the next gameplay capture must establish
the real speed change and confirm the rendered textures remain correct.

## 2026-09-04: ninth capture, VSync off and recycled surface headers

`renderdoccaps/pgr4_main9.tracy` (32,136,250 bytes) spans
17129900620..21928790911 ns. Its 80 complete native frames (indices 3..82)
average 59.21 ms / 16.89 FPS, with a 57.73 ms median, versus main8's
56.82 ms / 17.60 FPS. This does not demonstrate a whole-game speed gain.
GPU zones remain absent; evidence is in `out/tracy-pgr4-main9/analysis.json`.
Native swapchain VSync is now disabled through `setVsyncEnabled(false)`.
Guest VBlank workers remain active because the guest depends on their events.

ReXFM2P HEAD `592e5da3` and its newer uncommitted renderer changes were
inspected read-only. FM2's surface-header repair preserves native objects
when XGSetSurfaceHeader rewrites guest headers. PGR4 instead translates raw
headers into separate native proxies, but its translation cache identified
each surface only by its address and never checked for header reuse.

IDA38's PGR4 colour allocator `0x828389A8` and depth allocator `0x82838AF8`
both obtain headers from the free list at `0x82BD0D54`, protected by the
lock at `0x82BD0D5C`, then call XGSetSurfaceHeader (`0x826DE870`) and
XGOffsetSurfaceAddress (`0x826DEDD0`). The internal layout writer at
`0x82692D50` writes size at +0x24, full format at +0x28, samples at +0x18
and EDRAM base at +0x1C. An address can therefore change from colour to depth.

The latest pre-fix runtime log, `pgr4_recompiled_169.log`, records two failed
pipelines at 20:32:32.841 with `rt=10 ds=10`, followed immediately by a failed
353,024-byte staging allocation and `GetDeviceRemovedReason=0x887A0001`.
The depth slot contains a colour format. Stale surface translation explains
this invalid state; a new race run must confirm the device-loss outcome.

TranslateRawSurface now compares width, height, full format, sample count and
EDRAM base on every bind. Changed headers acquire the appropriate object
through the existing EDRAM registry and release the previous cache reference.
The registry retains old objects for queued draws and continues sharing the
compatible colour aliases used by Bink. Texture-level headers cannot return
a stale standalone proxy. The first 16 replacements are logged.

`python scripts/tests/test_surface_header_reuse.py` compiles the production
translation, format conversion, EDRAM registry and release code with guest
memory/GPU creation stubbed out. It passes colour/depth reuse, dimensions,
MSAA, tile base, compatible-format aliases, A-to-B-to-A reuse, queued-object
lifetime and balanced references. Substituting the pre-fix translation fails
at the colour-to-depth assertion. The RelWithDebInfo/Tracy Plume executable
rebuilt through `cmake-throttled.cmd`; build evidence is
`out/tracy-pgr4-main9/game-build.log`. Gameplay visibility remains user-QA
pending; the game was not launched automatically.

## 2026-09-04: visible gameplay and first race profile

The user confirms gameplay is now visible. `pgr4_recompiled_170.log` also
confirms the surface-reuse diagnosis: at 20:51:33.192, header `0x4082D3C0`
changes from colour format `0x1A2201BF` to depth `0x1A220197` at 1280x720,
EDRAM base `0x2D0`. Other recycled headers change dimensions and formats.
This run has no device-loss or failed-pipeline messages.

`renderdoccaps/pgr4_race1.tracy` (35,791,314 bytes) spans
36801287230..44029013556 ns, 7.228 seconds. Excluding initialization, missed
frames and both partial intervals, its 38 complete native frames (indices
3..40) average 184.14 ms / 5.43 FPS, with a 182.88 ms median. This is the
first visible-race baseline with native VSync disabled; the earlier menu
captures are different workloads. GPU zones remain absent.

Main guest thread 86232 records 4.920 seconds running, 1.461 seconds waiting
and 0.846 seconds uncovered, with 38,124 periodic samples. Render thread
82124 records 3.241 seconds running, 3.141 seconds waiting and 0.846 seconds
uncovered, with 24,453 samples. Tracy's symbol worker records 5.679 seconds
running, so profiling overhead remains material. Capture/thread evidence is
saved under `out/tracy-pgr4-race1/analysis.json`; sampled stacks are in
`stacks.jsonl` and zone evidence is in `zone-evidence.json`.

Texture binds total 3.735 seconds over 151,094 calls (24.72 us/call).
Indexed draws total 1.132 seconds over 185,842 calls (6.09 us/call), while
1,539 nonindexed draws total 1.076 seconds (699.00 us/call). GUI frame 756
takes 192.25 ms: 3,797 texture binds consume 102.22 ms, 4,694 indexed draws
28.81 ms and 39 nonindexed draws 28.07 ms. The main thread's top 500 stacks
cover 16,738 samples; geometry snapshots account for at least 26.92% of all
samples and texture upload for at least 11.72%. These are overlapping lower
bounds, not additive wall-clock percentages.

ByteSnapshotCache now uses the existing runtime SIMD copy-and-swap helpers
for its 16/32-bit converted payloads instead of the scalar variable-stride
loop. It still validates complete content, retains immutable versions for
queued draws, preserves trailing bytes and copies full shader-addressable
palettes. No draw ranges or texture freshness rules changed.

`test_upload_cache.py` passes normal and forced-hash-collision runs, including
520 new byte-level cases around SIMD boundaries, odd lengths and unaligned
sources. `test_render_queue.py` passes its ordering/lifetime checks. Seven
alternating before/after benchmark runs have identical output checksums:
for 221,760-byte payloads, 16-bit conversion changes 20.699 -> 15.694 ms and
32-bit conversion 18.530 -> 14.617 ms; for 2 MiB payloads, the corresponding
times are 60.631 -> 51.084 ms and 55.529 -> 49.459 ms. These times include
cache insertion, allocation, hashing, conversion and retirement across the
benchmark workload, not a single-buffer or whole-game timing.
`benchmark.py` and `benchmark-results.json` retain the experiment. The
RelWithDebInfo/Tracy game rebuilt through the throttled launcher. The next
race capture must establish the FPS impact; repeated texture conversion and
upload remains the larger follow-up target.

## 2026-09-04: oracle-guided persistent textures and pooled uploads

Read-only comparison used reblue HEAD `957ac62` and UnleashedRecomp HEAD
`5e8695a`. reblue's `src/gpu/native_texture_mirror.cpp:396` builds mirrors at
allocation/replacement, while its lookup path at 546 returns the existing
object. `src/gpu/hooks/resource.cpp:514` uploads dynamic textures on unlock;
the allocation hook at 550 creates the asset mirror. Its
`src/gpu/texture_upload.cpp:107` uses the shared upload arena with 0x200
alignment. UnleashedRecomp's `UnleashedRecomp/gpu/video.cpp:465` retains
upload buffers across frames; `ProcUnlockTextureRect` at 2190 allocates from
that pool. `SetTexture`/`ProcSetTexture` at 3757/3807 bind existing textures.

PGR4 was instead converting and uploading raw XG textures once each frame,
with one committed staging buffer per upload. Native images now retain their
contents across frames. Full decoded layout identity detects backing-address,
pitch, format, endian, tiling, mip and array changes. XXH3-128 signatures cover
all readable base/mip/array source bytes and page readability; unchanged
signatures skip conversion and the synchronous upload command. Inaccessible
pages preserve the existing zero-fill behavior. Failed uploads remain retryable.

IDA38 verifies `XGOffsetResourceAddress` at `0x826DEE98` assigns backing
addresses after header construction in raw allocators `0x82838CF8`,
`0x82293CE8` and cube helper `0x82292FB8`. These hooks invalidate cached
contents after the original guest call. The existing `D3D_UnlockResource`
hook (`0x82699F80`) now invalidates raw textures after unlock, including a
previous resolve destination rewritten by the CPU. Upload is deferred until
use because callers may still populate the newly assigned backing memory.
The once-per-frame full-data check remains for writers bypassing these hooks.
Validation and invalidation share the texture mutex, including frame-marker
publication; the render-frame counter and resolve flag are atomic. Existing
storage retains replaced native objects for queued references.

Changed raw textures use reusable CPU scratch and the existing frame upload
arena. The synchronous command consumes scratch and subresource descriptions
before returning; GPU bytes live until the recording frame's fence retires.
Texture placements request 512-byte alignment, including the existing native
UnlockRect path; chunk-capacity checks account for alignment padding.

Focused checks passed: `test_native_texture_cache.py` exercises production
parsing/layout/conversion, unchanged-frame skips, base/mip/array mutations,
sparse mappings, layout reuse, explicit invalidation, resolve protection and
failure retries. `test_upload_cache.py` passes normal/collision variants and
mixed 256/512-byte placements at chunk boundaries. Texture upload lifetime,
render queue, 504 row-conversion cases and surface-header reuse checks pass.

`out/tracy-pgr4-race1/texture-cache-benchmark.py` measures 120 unchanged frames
over seven alternating runs using the production texture path with fake guest
memory and GPU queue. Forced refresh versus retained signatures: 128x128
linear BGRA8, 0.5843 -> 0.2070 ms; 256x256 tiled BGRA8, 8.6938 -> 0.7856 ms;
1024x1024 tiled BC1, 17.7138 -> 1.5652 ms. Each retained run submits zero
uploads after initialization versus 120 forced uploads, with identical output
checksums. Forced refresh includes hashing; these are CPU workload timings,
not measurements of D3D allocation costs, GPU execution or whole-game FPS.
Results are in `texture-cache-benchmark-results.json` in the same directory.

The RelWithDebInfo/Tracy Plume target builds through `cmake-throttled.cmd`;
build evidence is `out/tracy-pgr4-race1/oracle-build.log`. Native VSync remains
disabled. The next same-race run must confirm texture updates, visual
correctness and frame-time improvement; no game launch was automated.

## 2026-09-04: race2 texture gains and geometry snapshot storage reuse

`renderdoccaps/pgr4_race2.tracy` (27,855,723 bytes) spans
31077377154..34845341229 ns. Its 49 complete native frames (indices 3..51,
excluding initialization, missed frames and both partial intervals) average
74.46 ms / 13.43 FPS, with a 73.21 ms median. Race1 averaged 184.14 ms /
5.43 FPS: race2 is 2.47x faster with 59.6% lower average frame time.
These are separate race runs, not deterministic replays. No GPU zones exist.

SetTexture falls from 24.72 to 2.30 us/call: race2 has 182,693 calls totaling
420.45 ms. Nonindexed draws still average 605.08 us/call (1,812 calls,
1.096 s); indexed draws average 4.54 us/call (224,493 calls, 1.020 s).
Median frame index 34 / GUI 782 takes 73.21 ms: 35 nonindexed draws cost
21.41 ms, 4,750 indexed draws cost 21.07 ms, and 3,858 texture binds cost
6.88 ms. Evidence is under `out/tracy-pgr4-race2/`: `analysis.json`,
`zone-evidence.json`, `median-frame.json` and `stacks.jsonl`.

Main guest thread 51532 has 18,111 periodic samples. Its top 500 stacks
cover 10,314 samples; QueueDrawStateSnapshots accounts for at least 49.72%
of all samples, with copying, conversion and comparison in CopyCached.
Render thread 67904 has 13,572 samples; IntermediaryUploadAllocator::Reset
accounts for at least 14.68%, with heap release stacks. These inclusive
percentages overlap and are lower bounds, not additive time fractions.
Context switches cover about 2.76 s, leaving about 1.01 s uncovered:
main running/waiting 2.262/0.498 s, render 1.741/1.020 s. Tracy's symbol
worker consumes 1.474 s running, so profiler overhead remains material.
Matching runtime log `pgr4_recompiled_172.log` has no device-loss/upload-failure
messages; it does report draws skipped for missing vertex/pixel shaders.
The capture alone cannot establish visual correctness.

The next change follows UnleashedRecomp's retained intermediary storage
(`gpu/video.cpp:533..578`) and reblue's reset-by-offset frame upload storage
(`src/gpu/constant_buffers.cpp:222`). ByteSnapshotCache previously destroyed
all raw/converted payload vectors at frame retirement. It now resets lookup
state and reuses those vectors on subsequent frames. Slot indices preserve
lookup validity when the owning vector grows and when the cache is deep-copied.
Converted variants are invalidated per new payload and written directly from
the raw snapshot, removing the preliminary full memcpy. Complete comparisons,
forced-hash-collision behavior, full geometry palettes, immutable in-frame
versions, fresh identities and recorded-batch lifetime remain intact.
Per-slot peak capacities are retained; varying workloads can raise retained
memory, as with other frame upload pools.

The added allocation regression fails against the pre-change cache and passes
afterward: large byte allocations stop after warm-up across repeated retired
generations, while contents and identities refresh. `test_upload_cache.py`
passes normal and forced-collision variants, including SIMD boundaries and
deep copies; `test_render_queue.py` passes FIFO and recorded ownership checks.
Seven alternating before/after CPU benchmark runs have identical checksums:
4 KiB payloads, 25.618 -> 5.402 ms (16-bit swap), 25.448 -> 5.532 ms (32-bit);
221,760 bytes, 17.471 -> 3.327 ms and 16.858 -> 3.454 ms; 2 MiB,
49.998 -> 20.046 ms and 52.163 -> 21.798 ms. These timings cover the same
eight-generation cache workload including hashing, conversion and retirement;
they do not measure whole-game FPS. `benchmark.py`, `benchmark-results.json`
and `byte_snapshot_cache-before.h` preserve the experiment.

The RelWithDebInfo/Tracy target rebuilt through `cmake-throttled.cmd`; evidence
is `out/tracy-pgr4-race2/game-build.log`. This new snapshot-storage change
needs a race3 capture and user visual confirmation. No game was launched.

## 2026-09-04: race3 workload comparison and windowed startup

`renderdoccaps/pgr4_race3.tracy` (33,794,573 bytes) spans
39433800637..44609163963 ns, 5.175 seconds. The 57 complete native frames
(indices 3..59, excluding initialization, missed frames and both partial
intervals) average 89.12 ms / 11.22 FPS, median 88.85 ms. Race2 averaged
74.46 ms / 13.43 FPS, so this run is slower overall. The workloads differ:
race3's median frame (index 46 / GUI 940) has 88 nonindexed draws versus 35
in race2's median frame; indexed counts remain close, 4,781 versus 4,750.
This is not a controlled before/after performance comparison.

Nonindexed draws average 410.69 us/call versus race2's 605.08 us/call;
race3 has 5,104 calls totaling 2.096 s. Indexed draws average 5.39 us/call
(278,027 calls, 1.499 s). SetTexture remains about 2.26 us/call
(230,107 calls, 519.17 ms), retaining the earlier texture-path improvement.
The median frame spends 34.62 ms in nonindexed draws, 26.39 ms in indexed
draws and 9.94 ms in texture binds. Evidence is in
`out/tracy-pgr4-race3/analysis.json`, `zone-evidence.json` and `stacks.jsonl`.

The main guest thread (82036) has 33,935 periodic samples, 22,184 covered
by its top 500 stacks. QueueDrawStateSnapshots accounts for at least 58.83%
of all samples; 7,003 samples in one stack alone are the 32-bit copy/swap
called from ByteSnapshotCache. Render thread 97480 has 24,008 samples;
UploadSnapshot accounts for at least 32.36%. IntermediaryUploadAllocator::Reset
has 180 samples (0.75%) versus race2's 1,992 (14.68%); retirement no longer
dominates the sampled render-thread work. Percentages are inclusive lower
bounds and cannot be added. Main running/waiting time is 4.237/0.194 s;
render 3.000/1.431 s, with about 0.744 s uncovered on each. Tracy's symbol
worker consumes 3.178 s running. No GPU zones are recorded.

Matching log `pgr4_recompiled_173.log` at frame 901 reports 752 MiB requested,
207 MiB written and 544 MiB reused in the frame upload pool. Vertex/pixel
shader-miss draw skips remain logged. The next performance target is the
volume of geometry snapshots and uploads, preserving shader-addressable
palette ranges; the differing workload does not justify reverting storage
reuse or claiming an overall speedup from race3.

At the user's request, Pgr4RecompiledApp::OnPostInitLogging now changes the
fullscreen cvar to false only when its source is the compiled default.
ReXApp loads config before this hook and creates the window afterward, so
PGR4 starts windowed while explicit config/environment/command-line choices
retain precedence. The shared runtime default is unchanged. The build folder
has no PGR4 TOML override, and this task's environment has no REX_FULLSCREEN.
The RelWithDebInfo/Tracy Plume target rebuilt successfully through the
throttled launcher (`out/tracy-pgr4-race3/game-build.log`). This build changes
startup mode; no additional renderer optimization was applied during race3
analysis, and no game launch was automated.

## 2026-09-04: bound nonindexed geometry snapshots to the draw

Race3's hottest copy stack reaches `sub_823DB2C8` through `sub_823DD7E0`.
IDA38 confirms this caller binds the shared buffer returned by `sub_823D4D78`
at byte offset `28 * sliceStart`, clears indices, and issues a QUADLIST draw
of `4 * quadCount` vertices (at most 2,048 quads). Its setup in `sub_823DBA38`
binds an explicit vertex declaration. `D3DDevice_SetStreamSource` at 0x82690618
sets fetch size to buffer size minus offset. Copying that whole suffix on
every slice was unnecessary work on both producer and upload threads.

The producer now retains the live queued declaration alongside geometry
bindings. For nonindexed draws with a compatible explicit declaration, each
ordinary raw stream ends at the last vertex's declared attribute footprint,
rounded to a dword and clamped to the fetch range. The prefix and startVertex
are preserved, including for generated quad/fan indices. One byte size drives
the immutable CPU snapshot, identity, GPU upload and vertex-buffer view.
Raw physical snapshots now take decoded byte sizes; every caller was updated.

POSITION1+ streams retain their full shader-indexed palettes. Unknown formats,
methods, incompatible/absent declarations and implicit fallback slot 15 keep
full ranges. Indexed, user-pointer and recorded draws also retain full ranges;
replay can inherit another declaration. Recorded declaration binds do not
alter the producer's live mirror, matching replay's render-state restoration.
The existing memory access checks and immutable queued versions remain.

`test_upload_cache.py` exercises the production geometry snapshot producer
with normal and forced-collision hashes: nonzero starts, fetch flags, partial
footprints, endian rounding, huge inputs, unknown declarations, palettes on
streams 0/1/7/15, recording isolation, mutations inside/outside the copied
range, memory-backed GPU uploads, and recorded ownership after retirement.
`test_render_queue.py` passes FIFO, concurrent batches and replay ownership.

Five alternating before/after benchmark runs use the actual snapshot and
upload allocators with memory-backed GPU buffers: 88 slices of a 1 MiB shared
VB per frame, stride 28, four timed frames after warm-up. Median total times:
4 vertices, 70.228 -> 0.085 ms; 64, 69.698 -> 0.139 ms; 1,024,
69.350 -> 1.361 ms; 8,192, 75.139 -> 15.091 ms. Snapshot bytes per frame fall
from 85,414,912 to 13,552 / 161,392 / 2,526,832 / 20,188,784 respectively.
All drawn-byte checksums match. These are isolated CPU workload measurements,
not whole-game FPS or GPU timings. Evidence and pre-change source are in
`out/tracy-pgr4-race3/geometry-benchmark.py`, `geometry-benchmark-results.json`
and `render_state-before.cpp`.

The RelWithDebInfo/Tracy Plume target rebuilt via `cmake-throttled.cmd`
(`out/tracy-pgr4-race3/performance-build.log`). Windowed startup remains the
default and native VSync remains off. Same-scene race FPS, gameplay visuals,
crowd and flags still require user verification; no game was launched.

## 2026-09-04: race4 results and draw-batch dispatch

The user reports gameplay is getting much better. `pgr4_race4.tracy`
(56,636,870 bytes) spans 36995998902..46564792200 ns. Its 193 complete native
frames (indices 3..195) average 49.27 ms / 20.30 FPS, median 48.07 ms, versus
race3's 89.12 ms / 11.22 FPS. Scene workloads still differ: median frame 170 /
GUI 893 has 56 nonindexed and 5,126 indexed draws versus race3's 88 / 4,781.
The nonindexed hook averages 2.37 us (15,229 calls, 36.11 ms total), down from
410.69 us. Indexed draws average 4.93 us (960,297 calls, 4.736 s); SetTexture
averages 2.08 us (776,971 calls, 1.618 s). The median frame spends 23.47 ms in
indexed draws, 7.71 ms in texture binds and 0.228 ms in nonindexed draws.

Main thread 80080 has 66,576 periodic samples; the top 500 stacks cover
28,484. QueueDrawGeometrySnapshot accounts for at least 30.87% and
ByteSnapshotCache::Find 21.06%. The largest single stack (4,622 samples)
passes through DrawIndexedVertices and `sub_823DCD20`. IDA38 identifies this
as crowd rendering, binding mesh stream 0 and a separate stream 1; indexed
palette preservation remains necessary. Render thread 50484 has 52,616
samples; 2,706 acquire and 2,106 release samples explicitly reach the outer
DispatchRenderCommand recording mutex through ExecuteJob. UploadSnapshot
accounts for at least 3.85%, versus 32.36% in race3. These are inclusive
sample lower bounds, not additive elapsed-time measurements. Tracy's symbol
worker runs for 7.285 s of this 9.569 s capture, so profiler overhead remains
material. There are no GPU zones. Matching log `pgr4_recompiled_175.log`
reports late-race uploads around 651 MiB requested / 39 MiB written / 611 MiB
reused, with varying draw workloads. Evidence is saved in
`out/tracy-pgr4-race4/{analysis.json,stacks.jsonl,zone-evidence.json}`.

The next change keeps each LocalRenderCommandQueue draw batch intact as one
owning FIFO job. Previously EnqueueBulk split it into one large deque node
per command, and each dispatch acquired RecordingMutex. A batch now owns a
contiguous command vector and dispatches ordinary commands under one lock.
Single-command jobs keep their existing inline storage. WaitForGpu and
CreateTranslatedTextureHost release the batch's lock before their existing
exception paths; replay retains its own lock and saved-state restoration.
This follows the oracle's bulk-command processing (`UnleashedRecomp/gpu/
video.cpp:5260`) while preserving PGR4's cross-thread synchronization rules.

The production queue/dispatcher check passes batch ownership, FIFO and
concurrent producer ordering, exact lock scopes, both mutex-held synchronous
exceptions, nested Run, replay ownership, draining Stop and restart. The
upload/snapshot checks also pass normal and forced-collision variants.
Seven alternating queue benchmark runs submit 20,000 batches after warm-up,
with GPU operations replaced by checksum work: 2 commands/batch takes
9.834 -> 3.304 ms; 4 takes 19.506 -> 4.518 ms; 8 takes 38.813 -> 5.865 ms.
Median allocations fall from 38,017 / 78,583 / 152,018 to
20,000 / 20,112 / 20,048. Output checksums match. These are isolated FIFO and
dispatch measurements, not a whole-game FPS prediction. The scripts, full
results and pre-change sources are in `out/tracy-pgr4-race4/`.

The RelWithDebInfo/Tracy Plume target rebuilt with the throttled launcher
(`performance-build.log`). Windowed startup and disabled native VSync remain.
Race5 and gameplay visual confirmation are pending; no game was launched.

## 2026-09-05: race5 results and watched geometry snapshots

`pgr4_race5.tracy` (44,485,473 bytes) contains 136 complete native frames
(indices 3..138), averaging 49.388 ms / 20.248 FPS, median 48.628 ms.
Race4 averaged 49.27 ms / 20.30 FPS: batch dispatch did not improve measured
whole-game throughput. Nonindexed draws remain cheap (2.431 us/call);
indexed draws average 5.080 us and SetTexture 2.022 us. Frame 132 / GUI 1041
takes 48.701 ms, with 4,867 indexed draws totaling 23.585 ms and 4,102 texture
binds totaling 7.536 ms. Main thread 98040 has 44,650 samples; its top 500
stacks put at least 30.67% in QueueDrawGeometrySnapshot and 20.63% in
ByteSnapshotCache::Find. The latter repeatedly compares full indexed crowd
mesh/palette buffers. Render thread 93720 still spends samples in constant
uploads (UploadAndBindRootDescriptor >=14.17%, UploadCached >=13.48%). These
inclusive sampled percentages are lower bounds and are not additive frame
timings. The capture has no GPU zones; Tracy's symbol worker ran for 4.465 s.
Evidence: `out/tracy-pgr4-race5/{analysis.json,stacks.jsonl,zone-evidence.json}`.

Large raw physical snapshots now use the SDK's existing guest-memory write
callbacks to retain a revision for unchanged contents. A matching revision,
source and byte count can reuse an immutable cache entry without hashing or
comparing the entire palette. Untagged sources, small snapshots, and failed
protection retain full byte validation. Revisions are checked around arming
and snapshot lookup; concurrent invalidation forces untagged validation on
that draw. Full indexed/palette ranges, endian views, immutable queued
versions, and recorded ownership remain intact. Write callbacks use atomics
only; arming happens outside the upload allocator lock.

The real-memory tests exposed two SDK gaps that would have made reuse stale:
initially read-only pages lacked invalidation flags, and recommitting an
allocation could restore host write access without retiring its flags. The
SDK now tracks read-only transitions, invalidates recommits/new aliases,
reports protection failure, and clears failed protection flags for retry.
`NotifyPhysicalMemoryWritten` covers direct native PM4 and XMA output stores
that bypass guest alias protection. The existing physical file-read path
already triggers invalidation after its direct stores.

`test_upload_cache.py` passes normal and forced-collision variants, including
write-during-arm and protection-failure fallback, sizes/endian views, deep
copies, reset and previous palette/mutation tests. `test_render_queue.py`
passes. `test_physical_write_watch.py` uses the actual SDK DLL and Windows
page faults through A/C/E aliases; it checks thread writes, direct stores,
initial read-only protection, recommit, decommit, release and another alias
mapping the same physical allocation. No game is launched by these tests.

Its optional benchmark runs the production SnapshotRawPhysicalBuffer and
IntermediaryUploadAllocator against actual guest memory: 50 frames with a
221,760-byte payload, mutation/reset each frame, and 2,000 repeated snapshots
per frame, including faults and endian conversion. Across three alternating
runs, median time is 195.629 ms with tracking disabled and 38.552 ms enabled
(5.07x); checksums match at 2,450,000. This is an isolated CPU workload, not
a gameplay FPS prediction. Results: `out/tracy-pgr4-race5/write-watch-test.log`.

The SDK and Plume target rebuilt successfully using the throttled launcher;
SDK/build logs are saved beside that evidence. `pgr4_recompiled.exe` is
51,193,856 bytes, built at 2026-09-05 10:38:29 local time. The built,
installed and deployed `rexruntimerd.dll` hashes match
(`201A5DAB4CF3BD6AED0DB51218EC25344CA6C774671B7F0B0FCC45E4FADBCCDF`).
The next same-scene capture must establish game performance and confirm
gameplay, crowd/flags and audio. Windowed startup and disabled native VSync
remain in place.

## 2026-09-05: race6 results and shader constant uploads

`pgr4_race6.tracy` (40,557,941 bytes) spans 42904690317..48078895852 ns.
Its 129 complete frames (indices 3..131, excluding capture boundaries) average
39.783 ms / 25.136 FPS, median 38.773 ms. Race5 averaged 49.388 ms / 20.248
FPS: throughput improved 24.1%. Indexed draw hooks average 3.118 us, down
from 5.080 us; nonindexed draws average 2.614 us and SetTexture 2.116 us.
Median frame 78 / GUI 1250 has 4,781 indexed draws totaling 13.572 ms and
3,956 texture binds totaling 9.368 ms. Race5's representative frame had
4,867 indexed draws totaling 23.585 ms, so draw counts are close but the
captures are still separate workloads.

Main thread 68404 has 34,364 samples; its top 500 stacks cover 10,890.
QueueDrawGeometrySnapshot accounts for at least 15.78% (race5: 30.67%).
The render thread 16884 has 29,245 samples, with 19,170 in its top 500 stacks;
UploadAndBindRootDescriptor accounts for at least 18.44% and UploadCached
17.62%. Constant upload hashing, content-map allocation and reset now form
a larger render-thread cost. These inclusive samples are not additive frame
times. There are no GPU zones, and Tracy's symbol worker runs for 3.765 s.
Evidence is saved under `out/tracy-pgr4-race6/`. Log 177 shows late-race upload
slots around 585 MiB requested / 40 MiB written / 544 MiB reused.

The next change follows Unleashed's dirty vertex/pixel constant files
(`UnleashedRecomp/gpu/video.cpp:4520`): ProcSetShaderConstants compares the
written range and marks only the changed stage. Changed constants convert
directly into the existing upload arena; clean draws reuse that stage's
allocation, without CPU snapshots, hashing or content-map nodes. Bindings
are still issued on every draw because pipeline/command-list changes may
invalidate them. The arena clears both cached references after its frame
fence; frame begin and recorded replay mark constants dirty. Replay restores
the register files through the same setter. Shared constants and geometry
retain their existing content/identity caches.

The extended upload check passes normal and forced-collision variants:
independent stage updates, high registers, repeated unchanged updates,
immutable prior draw bytes, replay-style restoration, frame-slot retirement,
invalid ranges and failure retry. The render-queue check passes too.
The isolated benchmark executes both versions' production allocator and
constant setter over 40 frames of 2,000 draws. Three-run median times for
changes every 1 / 8 / 64 draws are 178.734 -> 15.698 ms,
29.399 -> 6.471 ms, and 22.542 -> 5.615 ms; uploaded byte counts and checksums
match for these workloads. Alternating between two old states every draw
takes 39.711 -> 17.631 ms but uploads 655,360,000 rather than 655,360 bytes
over the 40 frames. That is the deliberate tradeoff: CPU/map overhead falls,
while repeating an older changed state uses another arena allocation.
Script/results: `constant-benchmark.py`, `constant-benchmark-results.json`.
These isolated measurements do not predict gameplay FPS.

The throttled Plume build succeeds (`performance-build.log`), producing
`pgr4_recompiled.exe` at 2026-09-05 11:00:40 local time, 51,194,368 bytes.
Windowed startup and native VSync-off remain. Race7 and gameplay visual/audio
confirmation are pending; no game was launched.

## 2026-09-05: race7 results and indexed vertex snapshot bounds

`pgr4_race7.tracy` (38,449,991 bytes) spans 43061221873..47858954106 ns.
Its 101 complete frames (indices 3..103) average 47.112 ms / 21.226 FPS,
median 45.132 ms. Race6 averaged 39.783 ms / 25.136 FPS, so overall throughput
regressed despite the constant-upload CPU reduction. Render thread 72776's
top 500 stacks contain UploadAndBindRootDescriptor in at least 2.91% of its
22,765 samples (race6: 18.44%), and UploadCached in 2.17% (race6: 17.62%).
Queue waiting accounts for at least 15.87%; the thread ran for 2.897 s and
waited for 1.282 s. Log 178 reports late-race upload slots around 585 MiB
requested / 48 MiB written / 537 MiB reused, versus race6's 40 MiB written.
The capture does not establish the cause of the total regression.

Main thread 58852 has 31,057 samples, with 9,085 in its top 500 stacks.
QueueDrawGeometrySnapshot accounts for at least 14.73%, CopyCached 10.34%,
and ByteSnapshotCache::Find 0.72%. Large stacks copy/hash/convert geometry
through `sub_8238FC78`. Indexed draw hooks average 3.824 us over 488,037 calls
(race6: 3.118 us); SetTexture averages 2.471 us (race6: 2.116 us). Median
frame 7 / GUI 884 contains 4,791 indexed draws totaling 14.773 ms and 3,949
texture binds totaling 8.227 ms. Race6's representative frame had 4,781
indexed draws, but these remain separate workloads. Sample percentages are
inclusive lower bounds, not additive frame times. There are no GPU zones;
Tracy's symbol worker ran for 3.007 s. Evidence is under
`out/tracy-pgr4-race7/{analysis.json,stacks.jsonl,zone-evidence.json}`.

Ordinary indexed draws now reuse the existing vertex snapshot sizing helper.
The producer first owns and endian-converts the index buffer, then scans only
the indices consumed by this draw to bound ordinary raw vertex streams.
The queued draw uses those same immutable indices. The original vertex
prefix, startIndex and signed baseVertexIndex remain unchanged. Full ranges
remain for POSITION1+ palettes, recordings, UP draws, native index buffers,
missing/incompatible declarations, unknown layouts/topologies, invalid
ranges and restart/sentinel indices. This extends the earlier nonindexed
optimization while retaining its palette and replay safeguards.

`test_upload_cache.py` passes normal and forced-hash-collision variants,
including actual producer snapshots with 16/32-bit unaligned index sources,
start offsets, signed bases, invalid-range fallbacks, guest mutations,
palette-tail visibility and recorded ownership after staging retirement.
`test_render_queue.py` also passes. Their logs are in the evidence directory.

The isolated benchmark uses production before/after snapshot code with the
actual SDK DLL, guest physical memory and Windows write faults. It mutates
32 mesh buffers and a separate full palette each frame, retires staging,
and checks consumed vertices over 20 timed frames of 128 draws. Three
alternating runs give these medians for 221,760-byte meshes, stride 28:

| Vertex span | Before ms | After ms | Before snapshot bytes | After snapshot bytes |
| --- | ---: | ---: | ---: | ---: |
| 64 | 12.445 | 3.490 | 141,926,400 | 1,146,880 |
| 1,024 | 12.896 | 7.281 | 141,926,400 | 18,350,080 |
| 4,096 | 12.917 | 9.628 | 141,926,400 | 73,400,320 |
| 7,920 (full) | 13.423 | 13.667 | 141,926,400 | 141,926,400 |

Checksums match. Scanning adds about 1.8% in this full-buffer case; smaller
ranges reduce snapshot work. These CPU measurements do not predict gameplay
FPS or establish visual correctness. Script, sources and results are saved
as `indexed-benchmark*` under the evidence directory.

The throttled Plume build succeeds (`performance-build.log`), producing
`pgr4_recompiled.exe` at 2026-09-05 11:52:42 local time, 51,194,880 bytes.
Windowed startup and disabled native VSync remain. Race8 and gameplay,
crowd/flags and audio confirmation are pending; no game was launched.

## 2026-09-05: race8 regression and removal of indexed snapshot bounds

`pgr4_race8.tracy` (41,117,975 bytes) spans 31643517364..37775646501 ns.
Its 82 complete frames (indices 3..84) average 74.072 ms / 13.500 FPS,
median 74.277 ms, versus race7's 47.112 ms / 21.226 FPS. Representative
frame 20 / GUI 766 has 4,791 indexed draws totaling 35.494 ms (race7:
4,791 / 14.773 ms) and 3,948 texture binds totaling 13.636 ms. Across the
capture, indexed hooks average 7.620 us, nonindexed 4.316 us and SetTexture
3.222 us. Log 179 reports late-race uploads around 484 MiB requested /
56 MiB written / 427 MiB reused: requested bytes fell from race7's 585 MiB,
but actual writes rose from 48 MiB.

Main thread 94496 has 44,250 samples; its top 500 stacks cover 13,206.
QueueDrawGeometrySnapshot accounts for at least 18.53%, CopyCached 15.38%,
ByteSnapshotCache::Find 3.27%, and the new index scan 2.22%. Race7's
corresponding first three shares were 14.73%, 10.34%, and 0.72%. Render
thread 93068 ran for 4.002 s and waited for 1.843 s; constant-upload samples
remain low (UploadAndBindRootDescriptor >=2.45%). These inclusive sample
shares are lower bounds, not additive frame times. Tracy's symbol worker
ran for 5.735 s, versus race7's 3.007 s, and there are no GPU zones. The
whole frame-time regression cannot be assigned entirely to one change.
Evidence: `out/tracy-pgr4-race8/{analysis.json,stacks.jsonl,zone-evidence.json}`.

The indexed bounds introduced a concrete cache regression. Source hints
remember only one size at each address. Alternating draw prefixes therefore
evict each other's revision hint and repeatedly hash unchanged bytes.
A focused hash-count assertion reproduced this; an address-and-size hint
experiment passed it. However, that experiment still retains multiple
overlapping immutable payloads and performs the per-draw index scan.

The new production-path benchmark uses the actual SDK and Windows physical
write faults with 32 distinct 221,760-byte meshes, stride 28, a separate
full palette, 384 indices per draw, and 20 timed frames of 1,024 draws.
It checks all consumed vertex bytes and compares three alternating runs.
Unlike the race7 benchmark, four ranges alternate at each mesh address:
1,024 / 4,096 / 2,048 / 7,920 vertices. Median CPU times are:

| Workload | Full snapshots | Indexed bounds | Bounds with per-size hints |
| --- | ---: | ---: | ---: |
| One range | 39.284 ms | 25.878 ms | 26.793 ms |
| Four alternating ranges | 45.526 ms | 164.723 ms | 58.925 ms |

For the mixed workload, hash calls are 680 / 20,520 / 2,600, covering
146,423,680 / 2,167,512,960 / 274,874,240 bytes respectively. Checksums match.
The fixed-size race7 benchmark missed this reuse pattern. These isolated
CPU results reproduce a mechanism, not the exact game workload or GPU cost.
`range-benchmark.py` retains the original and experimental headers/sources
so all three variants remain reproducible after restoring production code.

The indexed range optimization has been removed, restoring the exact
race7 producer source. The per-size hint experiment is also excluded.
Nonindexed bounds, physical write tracking, dirty shader constants and
full palette/replay ownership remain. The existing upload-cache check
passes normal and forced-collision variants; the render-queue check passes.
Final logs use `*-final-test.log` in the evidence directory.

The throttled Plume build compiled and linked successfully, producing
`pgr4_recompiled.exe` at 2026-09-05 12:13:06 local time, 51,194,368 bytes
(`performance-build.log`). Windowed startup and disabled native VSync
remain. Race9 must establish recovered gameplay performance and confirm
visuals/audio; no game was launched.

## 2026-09-05: race9 follow-up saved as pgr4_race1.tracy

The original `pgr4_race9.tracy` repeatedly failed to load with `bad allocation`;
the user supplied the follow-up as `pgr4_race1.tracy` (36,354,259 bytes,
modified 2026-09-05 12:37:51). This replaces the earlier race1 filename and
loads successfully. Its 111 complete frames average 40.647 ms / 24.602 FPS,
median 40.892 ms. Throughput recovered from race8's 13.500 FPS and is close
to race6's 25.136 FPS. This is observed recovery after removing indexed
bounds, not proof that all differences between captures came from that change.

Representative frame 101 / GUI 866 contains 4,781 indexed draws totaling
15.308 ms and 3,956 texture binds totaling 8.190 ms. Indexed hooks average
3.303 us over 531,054 calls; SetTexture averages 2.109 us over 440,166 calls.
Main thread 54880 has 29,629 samples; its top 500 stacks put at least 14.98%
in QueueDrawGeometrySnapshot, 10.38% in CopyCached and 0.67% in
ByteSnapshotCache::Find. Render thread 3208's constant-upload share stays
low (UploadAndBindRootDescriptor >=3.08% of 22,202 samples); it ran for
2.827 s and waited for 1.108 s. Geometry staging and texture binding remain
useful CPU targets. Samples are inclusive lower bounds, not additive timings.
There are no GPU zones, and the Tracy symbol worker ran for 3.287 s.

Evidence is saved under `out/tracy-pgr4-race9/`, with `analyze-resaved.py`
pointing to the user-confirmed race1 filename. No renderer changes or new
build were made during this capture review; visual/audio QA remains separate.
