#pragma once
/**
 * @file        ppc/guest_pc_fiber.h
 * @brief       OS-fiber-backed guest-PC reentry for guest coroutine/fiber swaps
 */

#include <cstdint>
#include <functional>
#include <unordered_set>
#include <vector>

#include <rex/ppc/context.h>

namespace rex::ppc {

struct GuestPcRunResult {
  bool ran = false;
  uint32_t start_pc = 0;
};

struct GuestRegion {
  uint32_t start = 0;
  uint32_t end = 0;
};

struct ResumeRewrite {
  uint32_t site_pc = 0;
  uint32_t resume_pc = 0;
  bool enabled = false;
};

struct GuestPcFiberConfig {
  std::vector<GuestRegion> guest_regions;
  std::vector<ResumeRewrite> resume_rewrites;
  std::vector<uint32_t> native_sites;
  std::unordered_set<uint32_t> deny_fiber_job_entries;

  std::function<uint32_t(PPCContext& ctx, uint8_t* base, uint32_t job_ctx)> capture_job_fn;
  std::function<void(PPCContext& ctx, uint8_t* base, uint32_t job_fn, uint32_t job_arg)>
      before_dispatch_job;
};

void RegisterGuestPcFiberConfig(GuestPcFiberConfig config);
void InstallGuestPcFiberInterpreter();
bool IsGuestPcFiberActive();
void ReconcileGuestPcFiberFrameStack(const PPCContext& ctx);
bool RunFiberSwap(PPCContext& ctx, uint8_t* base, PPCFunc* swap_impl, uint32_t job_ctx,
                  uint32_t fiber_slot);
GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc);

}  // namespace rex::ppc
