/**
 * @file        rex/ppc/guest_pc_fiber.h
 * @brief       Guest-PC fiber reentry: OS-fiber-backed cooperative swaps
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 *
 * @remarks
 *   Some titles ship their own cooperative-coroutine ("guest fiber") library
 *   that performs a full PPC register save/restore into a guest context block
 *   and then `blr`s to the resumed fiber's PC on a *different* stack (e.g. FH1
 *   `sub_830ED910`, reached via `sub_830EBE90`).
 *
 *   Under static recompilation the trailing guest `blr` becomes a host C++
 *   `return`, which unwinds the *host* call stack instead of jumping to the
 *   resumed guest PC -- and it does so while `ctx.r1` already points at a
 *   different (target) guest stack. The result is corrupted control flow and a
 *   wedged worker (see fh1/patcher/fiber-reentry.md).
 *
 *   This subsystem fixes the swap by backing every guest fiber with a real host
 *   OS fiber (rex::thread::Fiber). Each guest fiber therefore runs on its own
 *   host stack, so resuming it is a plain Fiber::SwitchTo -- the host call stack
 *   remembers exactly where the fiber was suspended and no mid-function PC
 *   dispatch is needed. The guest's own register save/restore routine is reused
 *   verbatim (passed in as `swapImpl`) to move PPCContext between the guest
 *   context blocks and to update the TLS/KTHREAD/PCR stack fields.
 *
 *   The config fields mirror the legacy interpreter-based API so existing
 *   project drivers (e.g. fh1_guest_pc_fiber.cpp) compile and drive this
 *   implementation unchanged. `guest_regions`, `resume_rewrites` and
 *   `native_sites` are retained for diagnostics/compatibility; the OS-fiber
 *   swap does not need PC-dispatch rewriting.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <rex/ppc/context.h>

namespace rex::ppc {

/// Inclusive-start, exclusive-end guest code/data range. Diagnostic only under
/// the OS-fiber backend (the legacy interpreter used these to bound dispatch).
struct GuestFiberRegion {
  uint32_t start;
  uint32_t end;
};

/// Maps a swap routine's guest entry PC to the PC the host should treat as the
/// post-KeSet resume point. Retained for compatibility/diagnostics.
struct GuestFiberResumeRewrite {
  uint32_t from_pc;
  uint32_t to_pc;
  bool tail;
};

/// A guest PC that should be dispatched as a native function entry.
struct GuestFiberNativeSite {
  uint32_t pc;
};

/// Result of dispatching guest code from a given PC on a fiber stack.
struct GuestPcRunResult {
  bool dispatched;   ///< true if a host function was resolved and invoked
  uint32_t start_pc; ///< the PC that was dispatched (or attempted)
};

/// Callbacks are `(ctx, base, ...)`; `base` is the guest virtual membase.
using GuestFiberCaptureJobFn =
    std::function<uint32_t(PPCContext& ctx, uint8_t* base, uint32_t job_ctx)>;
using GuestFiberBeforeDispatchFn =
    std::function<void(PPCContext& ctx, uint8_t* base, uint32_t job_fn, uint32_t job_arg)>;

/// Project-supplied configuration describing the title's guest-fiber library.
struct GuestPcFiberConfig {
  std::vector<GuestFiberRegion> guest_regions;
  std::vector<GuestFiberResumeRewrite> resume_rewrites;
  std::vector<GuestFiberNativeSite> native_sites;

  /// Called when a job/coroutine context is first captured. Diagnostic; the
  /// return value is currently ignored.
  GuestFiberCaptureJobFn capture_job_fn;

  /// Called immediately before a job function is dispatched on a fiber stack.
  GuestFiberBeforeDispatchFn before_dispatch_job;

  /// Job entry PCs that must NOT be relocated onto a freshly allocated job
  /// fiber -- they are run inline on the current fiber instead.
  std::vector<uint32_t> deny_fiber_job_entries;

  /// Guest LR values (swap call sites) where a swap onto a freshly-zeroed target
  /// block must not SwitchTo a new host fiber. The runtime restores the
  /// outgoing callee-saved register file from the source block and continues
  /// inline on the current host stack instead.
  std::vector<uint32_t> deny_fiber_swap_entry_lrs;

  /// Guest LR values at which a ZERO-RESTORE (target block r20==0 after swap)
  /// is repaired by copying the outgoing saved register file (+200..+288, and
  /// r1@+48) from the source block back into PPCContext (and seeding the
  /// target block before swap when it is still zero).
  std::vector<uint32_t> zero_restore_preserve_entry_lrs;

  /// Guest PC of the title's fiber-start trampoline (the routine a freshly
  /// created fiber's saved context resumes into, e.g. FM4 sub_82C7A450). When a
  /// swap restores a target whose resume PC equals this, the target is a BRAND
  /// NEW fiber: any pre-existing Map() entry for its block is stale (the guest
  /// recycled the block address) and must be dropped so a fresh host fiber runs
  /// it from the trampoline -- never an old suspended host stack. 0 disables the
  /// check.
  uint32_t fiber_start_pc = 0;
};

/// Register (replace) the active guest-fiber configuration. Safe to call before
/// the runtime function table exists; takes effect at InstallGuestPcFiberInterpreter.
void RegisterGuestPcFiberConfig(GuestPcFiberConfig config);

/// Activate the OS-fiber-backed guest-fiber subsystem. Idempotent. Must be
/// called once the runtime/function table is available (e.g. OnPreLaunchModule).
void InstallGuestPcFiberInterpreter();

/// True once InstallGuestPcFiberInterpreter has activated the subsystem.
bool IsGuestPcFiberActive();

/// Reconcile host bookkeeping with the guest frame stack for `ctx`. No-op under
/// the OS-fiber backend (host stacks are authoritative); retained for the API.
void ReconcileGuestPcFiberFrameStack(const PPCContext& ctx);

/// Perform one cooperative guest-fiber swap.
///
/// `swapImpl` is the title's own register save/restore routine (the recompiled
/// guest swap, e.g. `__imp__sub_830ED910`). On entry `ctx.r3` holds the target
/// guest context block per the guest swap ABI. The call runs `swapImpl` to move
/// PPCContext to the target fiber, then Fiber::SwitchTo the host fiber backing
/// that target. Returns true if a host-fiber switch occurred.
///
/// `job_ctx` / `fiber_slot` are passed through from the driver for diagnostics.
bool RunFiberSwap(PPCContext& ctx, uint8_t* base, PPCFunc* swapImpl, uint32_t job_ctx,
                  uint32_t fiber_slot);

/// Dispatch guest code starting at `start_pc` on the current (fiber) host
/// stack: resolves the host function for `start_pc` and invokes it.
GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc);

}  // namespace rex::ppc
