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
