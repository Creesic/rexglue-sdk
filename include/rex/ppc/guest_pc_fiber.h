/**
 * @file guest_pc_fiber.h
 * @brief Guest-PC fiber interpreter for static-recomp cooperative fiber swaps.
 *
 * After KeSetCurrentStackPointers the guest blrs to a resume PC that is often not a
 * function entry. Generated C++ cannot represent that continuation; this module
 * drives guest PC through fiber regions with optional title callbacks.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

struct PPCContext;

namespace rex::ppc {

enum class GuestPcRunResult {
  Complete,
  SwapBack,
  StepLimit,
  UnknownPc,
  Error,
};

struct GuestPcFiberRegion {
  uint32_t lo = 0;
  uint32_t hi = 0;
};

struct GuestPcFiberResumeRewrite {
  uint32_t from_pc = 0;
  uint32_t to_pc = 0;
  bool only_on_fiber_stack = true;
};

struct GuestPcFiberNativeSite {
  uint32_t pc = 0;
};

struct GuestPcFiberConfig {
  std::vector<GuestPcFiberRegion> guest_regions;
  std::vector<GuestPcFiberResumeRewrite> resume_rewrites;
  std::vector<GuestPcFiberNativeSite> native_sites;

  /// Optional: recover job function pointer when r31 is clobbered during swap.
  std::function<uint32_t(PPCContext&, uint8_t*, uint32_t job_ctx)> capture_job_fn;

  /// Optional: resolve r3 job argument before dispatching a native resume job.
  std::function<uint32_t(PPCContext&, uint8_t*, uint32_t job_fn)> resolve_job_arg;

  /// Optional: title hook before calling a job entry (e.g. stash work queue).
  std::function<void(PPCContext&, uint8_t*, uint32_t job_fn, uint32_t job_arg)>
      before_dispatch_job;

  /// Optional: run swap-back KeSet tail after job dispatch (saved thread ctx in fiber_slot).
  std::function<bool(PPCContext&, uint8_t*, uint32_t fiber_slot)> run_swap_back_tail;

  /// Optional: handle a registered native resume site (return UnknownPc if unhandled).
  std::function<GuestPcRunResult(PPCContext&, uint8_t*, uint32_t pc)> native_site_handler;
};

void RegisterGuestPcFiberConfig(GuestPcFiberConfig config);
const GuestPcFiberConfig& GetGuestPcFiberConfig();

void InstallGuestPcFiberInterpreter();

bool IsGuestPcFiberActive();
bool ShouldPreserveGuestPcFiberR31(const PPCContext& ctx);

void ReconcileGuestPcFiberFrameStack(const PPCContext& ctx);

/// Continue at @p resume_pc after KeSet swap-to-fiber (uses active RunGuestPc anchor when nested).
void GuestPcFiberResume(PPCContext& ctx, uint8_t* base, uint32_t resume_pc);

/// Swap-back path for codegen host boundary (jmp==2 after KeSet returns to thread stack).
void NotifyGuestPcFiberHostBoundarySwapBack();

GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc);

using FiberSwapImplFn = void (*)(PPCContext& ctx, uint8_t* base);

/// Run a generated fiber swap implementation under guest-PC control.
bool RunFiberSwap(PPCContext& ctx, uint8_t* base, FiberSwapImplFn swap_impl,
                  uint32_t job_ctx, uint32_t fiber_slot_at_entry);

/// Call KeSet (or equivalent) while guest-PC fiber swap_pending is set (swap-back tail).
bool InvokeGuestPcFiberKeSet(
    PPCContext& ctx, uint8_t* base,
    const std::function<void(PPCContext&, uint8_t*)>& keset_call);

}  // namespace rex::ppc
