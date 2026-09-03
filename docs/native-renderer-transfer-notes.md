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
