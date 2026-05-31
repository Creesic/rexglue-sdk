# FM2 Performance Notes

## Command Processor Telemetry

FM2 reaches gameplay, but the press-start/menu path drops well below the intro-video frame rate.
The old Tracy capture `FM2/out/build/win-amd64-relwithdebinfo/intro-to-pressstart.tracy` has CPU
zones only; it does not contain real GPU timestamp contexts. Tracy zones named `gpu` in that capture
are CPU-side ReXGlue command processor scopes, not proof that the hardware GPU is saturated.

Current diagnostic patch:

- Added `gpu_command_stats` cvar in `src/graphics/command_processor.cpp`.
- Added `gpu_command_stats_min_us` and `gpu_command_stats_interval` cvars.
- Added per-frame counters for primary buffers, indirect buffers, packet mix, top PM4 type-3
  opcodes, draw packets, copy packets, `WAIT_REG_MEM` polls/sleeps/yields, D3D12 submissions, and
  D3D12 fence waits.
- Added `WAIT_REG_MEM` target aggregation (`wait_targets=[...]`) and command processor starvation
  timing (`stall=... stall_us=...`) to distinguish guest-side waits from the CP thread idling while
  waiting for new commands.
- Temporary local experiment: `gpu_wait_reg_mem_yield_short_waits` defaults to enabled. It treats
  `WAIT_REG_MEM` waits of exactly `0x100` as short spin waits instead of 1 ms sleeps, because FM2's
  menu repeatedly waits on `mem:1BCA5006 == 0`, and Windows was stretching those nominal sleeps into
  much larger delays.
- Temporary local experiment: Windows `rex::thread::MaybeYield()` now uses `Sleep(0)` instead of
  `SwitchToThread()`, matching Xenia Canary's intent to yield across processors rather than
  potentially spinning on the current core. This is meant to reduce audio starvation while keeping
  the short `WAIT_REG_MEM` wait improvement.
- Added D3D12 backend hooks for `EndSubmission`, `IssueCopy`, and `CheckSubmissionFence`.
- Added producer-side `CP_RB_WPTR` telemetry so slow frames now log how often the guest advances
  the ring write pointer (`wptr_updates`), repeated writes (`same`), queued dword deltas
  (`wptr_dwords`, `max`), producer quiet time (`gap_us`, `max_gap_us`), and final ring state
  (`rptr`, `wptr`, `queued_dw`). These counters are atomic because the write pointer is updated
  by guest code while frame stats are emitted by the command processor thread.
- Added `vsync_off_vblank_hz` cvar to tune guest vblank pacing when `--vsync=false`.
  The old no-vsync path was hardcoded to 1000 Hz; now it is configurable for
  title-specific framerate unlock and stability tuning.
- Added a codegen/runtime experiment for Xenon `db16cyc`: generated code now emits
  `rex::ppc::delay_execution()`, implemented as `simde_mm_pause()`. Xenia Canary treats this
  instruction as a spin-loop delay (`pause` by default, optional `MaybeYield`), while ReXGlue had
  previously emitted nothing. FM2's hot producer wait helper `sub_82369340` contains eight
  consecutive `db16cyc` instructions, so this is a candidate fix for menu CPU burn and audio
  starvation.
- May 22 result: the `db16cyc` pause patch is a real improvement, but not a full fix. In the
  press-start/menu path, `max_gap_us` dropped from roughly `p50=29219/p90=42634` to
  `p50=8476/p90=9854`, `stall_us` dropped from `p50=19945/p90=30460` to
  `p50=4772/p90=10969`, and `queued_dw` rose from `p50=36` to `p50=61`. The remaining frame time
  is dominated by `WAIT_REG_MEM` on `mem:1BCA5006 == 0`.
- May 22 patch: the short `WAIT_REG_MEM wait=0x100` fast path now uses
  `rex::ppc::delay_execution()` for most failed polls and only yields once every
  `gpu_wait_reg_mem_short_wait_yield_interval` polls, defaulting to `256`. CP stats now include
  `wait_pauses` globally and inside each `wait_targets=[...]` entry, so the next log can separate
  pause spins from scheduler yields.
- May 22 first result with the default interval `256`: the patch mechanically worked, but it was
  not a clean perf win. In the fresh press-start/menu window, `wait_yields` dropped and
  `wait_pauses` appeared as expected, while `WAIT_REG_MEM` wait time was roughly
  `p50=11786/p90=17596`. However, producer starvation came back: `stall_us` rose to roughly
  `p50=20295/p90=25134` and `max_gap_us` to roughly `p50=27865/p90=39728`, with menu frame time
  around `p50=31734/p90=47597`.
- Next test: keep the pause-spin path enabled, but force
  `--gpu_wait_reg_mem_short_wait_yield_interval 16` in `FM2/.vs/launch.vs.json`. This should show
  whether the `256` interval simply starves the guest producer/audio threads by spinning too long.
- May 22 result with interval `16`: the cvar is active, with `wait_pauses / wait_yields` very close
  to `15`, but it still is not a clean win. In the fresh press-start/menu window, frame time was
  roughly `p50=31537/p90=47151`, `stall_us` stayed high at roughly `p50=20258/p90=27778`, and
  `max_gap_us` was roughly `p50=28985/p90=29919`. `WAIT_REG_MEM` wait time improved a little
  (`p50=7982/p90=17150`), but the producer/audio starvation did not recover. The next clean test is
  interval `1`, which makes the new path behave like yield-every-poll and should confirm whether
  pause-spinning is the wrong direction for FM2's menu.
- Repeated interval `16` run around 11:34-11:35 reproduced the same pattern: menu frame time was
  roughly `p50=31517/p90=47660`, `stall_us` was roughly `p50=20244/p90=28972`, and
  `wait_pauses / wait_yields` stayed almost exactly `15`. Switched `FM2/.vs/launch.vs.json` to
  `--gpu_wait_reg_mem_short_wait_yield_interval 1` for the next run.
- May 22 result with interval `1`: the yield-every-poll path is active (`wait_pauses=0` and
  `wait_yields` almost equals `wait_polls`), but it still does not restore the strong post-`db16cyc`
  numbers. In the fresh press-start/menu window, frame time was roughly `p50=31567/p90=51174`,
  `stall_us` stayed around `p50=20717/p90=30513`, `max_gap_us` around `p50=27905/p90=41443`, and
  `WAIT_REG_MEM` wait time around `p50=9950/p90=17042`. This suggests the remaining menu problem is
  not primarily the short `WAIT_REG_MEM` delay policy; the CP is often waiting for the guest producer
  to submit more work.
- Temporary local diagnostic setting: `gpu_command_stats` defaults to enabled and logs through the
  GPU error logger so the stats appear in the current error-only `C:\temp\fm2-clean.log` sink.
- Enabled the diagnostics in `FM2/.vs/launch.vs.json` with:

```text
--gpu_command_stats --gpu_command_stats_min_us 20000
```

May 22 writer-trace diagnostic:

- Added a GPU packet memory-write watch in `src/graphics/command_processor.cpp` through
  `gpu_mem_write_trace_addr` and `gpu_mem_write_trace_size`.
- Enabled that watch in `FM2/.vs/launch.vs.json` for decimal address `466243584`
  (`0x1BCA5000`) and size `16`, covering the hot wait target `mem:1BCA5006`.
- Added a temporary generated-header CPU store watch in `FM2/generated/fm2_init.h` for the same
  `0x1BCA5000..0x1BCA500F` window. This is intentionally test-only and will be overwritten if FM2
  is regenerated; if it proves useful, move it into an SDK/codegen-controlled diagnostic.
- New log lines to compare against `CP stats`:

```text
gpu_mem_write_watch source=...
guest_store_watch addr=...
```

Expected log line prefix:

```text
CP stats frame=...
```

The next comparison should be between CP stats lines from normal intro-video frames and slow
press-start/menu frames. The most important fields are `ib`, `draws`, `copies`, `wait_reg_mem`,
`d3d12_submit`, `fence_waits`, `fence_us`, `stall_us`, `wptr_updates`, `wptr_dwords`,
`max_gap_us`, `queued_dw`, and `top_type3`.

## Empty Resolve No-Op

The slow press-start/menu log in `C:\temp\fm2-clean.log` had no CP stats because the current log
sink was error-only, but it did show `19859` repeated pairs of:

```text
Resolve region is empty
PM4_DRAW_INDX_2(3, 8, 2): Failed in backend (... edram_mode=6)
```

That path is an EDRAM copy-mode draw. ReXGlue was treating an empty or inverted resolve rectangle
as a backend failure in `draw_util::GetResolveInfo`, so the menu could spam synchronous GPU errors.
Xenia Canary treats the same case as a legal no-op and returns a zero-sized resolve; the D3D12 and
Vulkan render target caches already skip zero-sized resolves.

Ported the no-op behavior to `src/graphics/util/draw.cpp` so empty clipped resolves set
`width_div_8 = 0`, `height_div_8 = 0`, and return success instead of logging an error.

## May 31 Load-Time Pass

- Reduced deferred overlapped completion latency by replacing the fixed
  `kDeferredOverlappedDelayMillis = 100` sleep in `src/system/kernel_state.cpp`
  with a cvar:
  `--deferred_overlapped_delay_ms` (default `1`, `0` disables sleeping).
- Rationale: content and enumerator async wrappers (`XamContentCreate*`,
  `XamEnumerate`) use deferred overlapped completion and were paying a fixed
  artificial delay per operation.
- Added a sequential-read optimization in
  `src/filesystem/devices/stfs_container_file.cpp`:
  `StfsContainerFile::ReadSync` now caches the last block index/source offset
  and starts future scans from that location when possible, avoiding repeated
  block-list rescans from index 0 during streaming loads.

## May 31 FMOD IRQ Profiling Hygiene

- Gameplay CPU captures still showed large time in
  `rex::memory::QueryProtect` / `VirtualQuery` from
  `anonymous namespace::GuestReadableByte`, reached through
  `FM2FmodIrqDispatch8236C380`.
- Root cause: FM2 sig-site diagnostics were still probing guest memory on the
  FMOD IRQ hot path, even when `REX_FM2_SIGSITE_DIAG=0`.
- Mitigation in `FM2/src/fm2_hooks.cpp`: FMOD IRQ diag hooks now early-out when
  diagnostics are disabled, preserving optional force overrides
  (`REX_FM2_SCHED_MODE2`, `REX_FM2_SUBMIT_MODE3`) but removing expensive
  per-call guest readability checks from normal profiling runs.

## May 31 Producer Guard Outcome Counters

- Added targeted counters for `0x82369340` (`FM2_ProducerProgressGuard_82369340`)
  using hook points at:
  - `0x823693F8` (`wait_ret1` path),
  - `0x82369400` (timeout/recovery call site before `sub_82373E38`),
  - `0x82369408` (`ret0` path).
- New cvar:
  - `--fm2_prod_guard_stats` (default off).
- When enabled, `C:\temp\fm2-clean.log` emits:
  - `FM2_PROD_GUARD_PERSEC sec=... total=... wait_ret1=... ret0=... timeout_call=... ret0_non_timeout=... wait_pct=...`
- Purpose: quantify how much time this hotspot spends in keep-wait vs timeout/recovery behavior before attempting behavior changes.

## May 31 Producer Guard Throttle A/B

- Added cvar-gated throttling on the `0x823693F8` wait-return path of
  `FM2_ProducerProgressGuard_82369340`:
  - `--fm2_prod_guard_wait_pause_count N`
    - Executes `N` calls to `rex::ppc::delay_execution()` per wait-hit (`0` disables, capped at `64`).
  - `--fm2_prod_guard_wait_yield_interval N`
    - Calls `rex::thread::MaybeYield()` every `N` wait-hits (`0` disables).
- Defaults keep behavior unchanged (`0`, `0`), allowing safe A/B against baseline.

### May 31 results

- Baseline with stats enabled (no throttle) showed the guard as a pure busy-wait:
  `wait_pct=100` and roughly `1.6M..1.9M` wait returns per second.
- First tuned run:
  - `--fm2_prod_guard_wait_pause_count 4`
  - `--fm2_prod_guard_wait_yield_interval 16384`
  - Observed major drop in guard hit rate (typically around `0.5M..0.8M`/sec with some lower windows),
    while preserving `wait_pct=100` and improving subjective gameplay smoothness.
- Follow-up A/B run:
  - `--fm2_prod_guard_wait_pause_count 8` (same yield interval) for additional testing.
- Follow-up A/B run:
  - `--fm2_prod_guard_wait_pause_count 16` (same yield interval) for additional testing.

## May 31 Producer Guard Deeper Trace

- Added deeper decision telemetry for `0x82369340`
  (`FM2_ProducerProgressGuard_82369340`) by instrumenting:
  - function entry caller LR,
  - early return when status flag bit is set,
  - cursor compare (`r9` vs `*(*r29+10768)`),
  - delta bucket at wait path (`delta < 5000`),
  - delta on timeout path (`delta >= 5000`).
- New cvars:
  - `--fm2_prod_guard_trace` (default off)
  - `--fm2_prod_guard_trace_sample_interval N` (default `0`, disabled)
- When both `--fm2_prod_guard_stats=true` and `--fm2_prod_guard_trace=true` are enabled,
  `C:\temp\fm2-clean.log` now also emits:
  - `FM2_PROD_GUARD_TRACE_PERSEC ...`
    - Includes caller LR histogram, early-flag blocks, cursor eq/ne counts, wait-delta buckets,
      timeout-delta metrics, and last-seen values.
  - Optional sampled lines when `--fm2_prod_guard_trace_sample_interval > 0`:
    - `FM2_PROD_GUARD_WAIT_SAMPLE ...`
    - One detailed wait-path state snapshot every `N` wait hits.

- Added caller-loop telemetry for `sub_823729E0` (`LR 0x82372A70` site) so we can
  inspect producer fields surrounding the wait loop:
  - `FM2_PROD_WAITLOOP_72A70 sec=... hits=... ret1=... ret0=... obj0=... lim0=... cur0=... tgt0=... need_lt_avail=... need_ge_avail=... last(...)`
  - Now also includes `small_break=...` and `call73078(...)` pre/post-change stats.

- Added experimental small-gap break for `sub_823729E0` wait-loop:
  - `--fm2_prod_waitloop_spin_min_gap N` (default `0` = off / original behavior)
  - If `N > 0`, the loop only continues spinning when `(avail - need) > N`; otherwise
    it exits the spin loop early.
  - Intended for A/B testing when producer loop sits in persistent tiny-gap churn
    (for example steady `gap` around `2`).

- Added additional low-risk pacing control in the same wait-loop:
  - `--fm2_prod_waitloop_yield_interval N` (default `0` = off)
  - When enabled, calls `rex::thread::MaybeYield()` every `N` spin iterations
    without changing functional readiness conditions.
  - `FM2_PROD_WAITLOOP_72A70` now includes `yields=` and `yield_int=`.

### May 31 2026 Wait-Loop Yield A/B/C (45s gameplay windows)

- Captures:
  - `C:\temp\fm2-clean-yield0-20260531-160500.log`
  - `C:\temp\fm2-clean-yield256-20260531-160555.log`
  - `C:\temp\fm2-clean-yield2048-20260531-160830.log`
- Baseline run args:
  - `--fm2_prod_guard_stats=true --fm2_prod_guard_trace=true --fm2_prod_guard_trace_sample_interval=0 --fm2_prod_waitloop_spin_min_gap=0`
  - with `--fm2_prod_waitloop_yield_interval={0|256|2048}`
- Aggregate comparison from those windows:
  - `FM2_PROD_GUARD_PERSEC total` avg:
    - `yield=0`: `411,914`
    - `yield=256`: `346,356` (best)
    - `yield=2048`: `373,446`
  - `FM2_PROD_WAITLOOP_72A70 hits` avg:
    - `yield=0`: `420,223`
    - `yield=256`: `349,609` (best)
    - `yield=2048`: `381,823`
  - CP stats `frame_us` avg:
    - `yield=0`: `206,176`
    - `yield=256`: `189,069` (best)
    - `yield=2048`: `231,161` (worst)
  - CP stats `stall_us` avg:
    - `yield=0`: `156,506`
    - `yield=256`: `145,538` (best)
    - `yield=2048`: `179,784` (worst)

Current recommendation:
- Keep `--fm2_prod_waitloop_yield_interval=256` for now.

Note:
- This is temporary experiment instrumentation in generated FM2 output plus
  `FM2/src/fm2_hooks.cpp` for root-cause isolation, and should be moved to permanent
  hook/codegen paths once conclusions are confirmed.

## May 31 VSync Default

- Changed `vsync` cvar default from enabled to disabled in
  `src/graphics/command_processor.cpp`:
  - `REXCVAR_DEFINE_BOOL(vsync, false, "GPU", "Enable vertical sync");`
- Effect: FM2 now launches with VSYNC off unless explicitly overridden.

## May 31 CP Stall-Loop Tuning Knobs

- Added command-processor stall-loop cvars in
  `src/graphics/command_processor.cpp` for profiling-driven pacing tests:
  - `--gpu_cp_stall_spin_threshold` (default `500`)
    - Poll count before switching from spin/yield to timed waits.
  - `--gpu_cp_stall_wait_ms` (default `5`)
    - Timed wait duration after threshold is exceeded (`0` disables timed waits).
- Defaults preserve prior behavior; these are for A/B tuning based on Tracy
  `CommandProcessor::Stall` and CP stats `stall_us` / `stall_waits`.

## May 31 APU Mix Core Instrumentation

- Added targeted instrumentation for `0x82697F08`
  (`FM2_ApuMixRenderCore_82697F08`) using midasm hooks at:
  - entry: `0x82697F08`
  - exit path A: `0x826983A0`
  - exit path B: `0x826983C0`
- New cvar:
  - `--fm2_apu_mix_stats` (default off).
- When enabled, `C:\temp\fm2-clean.log` emits:
  - `FM2_APU_MIX_PERSEC sec=... calls=... exits=... exit_a=... exit_b=... total_us=... avg_us=... max_us=... inflight_delta=... unmatched_exit=... stack_ovf=...`
- Purpose: quantify how much real CPU time this hotspot consumes per second,
  and verify whether it is a meaningful optimization target versus wait-heavy
  scheduler/fence paths.
