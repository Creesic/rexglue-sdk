#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

// Dispatcher/cdf8 yields reload their own callee-saves after return; restoring
// r14-r31 there breaks post-swap scheduler state.
constexpr uint32_t kDispatcherFiberYieldLr = 0x824C0600u;
constexpr uint32_t kCdf8FiberYieldLr = 0x8258CE4Cu;

struct SchedulerFiberGprs {
  uint64_t r14 = 0;
  uint64_t r15 = 0;
  uint64_t r16 = 0;
  uint64_t r17 = 0;
  uint64_t r18 = 0;
  uint64_t r19 = 0;
  uint64_t r20 = 0;
  uint64_t r21 = 0;
  uint64_t r22 = 0;
  uint64_t r23 = 0;
  uint64_t r24 = 0;
  uint64_t r25 = 0;
  uint64_t r26 = 0;
  uint64_t r27 = 0;
  uint64_t r28 = 0;
  uint64_t r29 = 0;
  uint64_t r30 = 0;
  uint64_t r31 = 0;
};

bool NeedsFiberCalleeSavePreserve(uint32_t caller_lr) {
  switch (caller_lr) {
    case kDispatcherFiberYieldLr:
    case kCdf8FiberYieldLr:
      return false;
    default:
      return true;
  }
}

void SaveSchedulerFiberGprs(PPCContext& ctx, SchedulerFiberGprs& saved) {
  saved.r14 = ctx.r14.u64;
  saved.r15 = ctx.r15.u64;
  saved.r16 = ctx.r16.u64;
  saved.r17 = ctx.r17.u64;
  saved.r18 = ctx.r18.u64;
  saved.r19 = ctx.r19.u64;
  saved.r20 = ctx.r20.u64;
  saved.r21 = ctx.r21.u64;
  saved.r22 = ctx.r22.u64;
  saved.r23 = ctx.r23.u64;
  saved.r24 = ctx.r24.u64;
  saved.r25 = ctx.r25.u64;
  saved.r26 = ctx.r26.u64;
  saved.r27 = ctx.r27.u64;
  saved.r28 = ctx.r28.u64;
  saved.r29 = ctx.r29.u64;
  saved.r30 = ctx.r30.u64;
  saved.r31 = ctx.r31.u64;
}

void RestoreSchedulerFiberGprs(PPCContext& ctx, const SchedulerFiberGprs& saved) {
  ctx.r14.u64 = saved.r14;
  ctx.r15.u64 = saved.r15;
  ctx.r16.u64 = saved.r16;
  ctx.r17.u64 = saved.r17;
  ctx.r18.u64 = saved.r18;
  ctx.r19.u64 = saved.r19;
  ctx.r20.u64 = saved.r20;
  ctx.r21.u64 = saved.r21;
  ctx.r22.u64 = saved.r22;
  ctx.r23.u64 = saved.r23;
  ctx.r24.u64 = saved.r24;
  ctx.r25.u64 = saved.r25;
  ctx.r26.u64 = saved.r26;
  ctx.r27.u64 = saved.r27;
  ctx.r28.u64 = saved.r28;
  ctx.r29.u64 = saved.r29;
  ctx.r30.u64 = saved.r30;
  ctx.r31.u64 = saved.r31;
}

void FiberSwapImpl82785670(PPCContext& ctx, uint8_t* base) {
  __imp__DOAX_FiberContextSwitch(ctx, base);
}

void RegisterDoaxGuestPcFiberConfig(const rex::PPCImageInfo& image_info) {
  rex::ppc::GuestPcFiberConfig config;
  config.guest_regions.push_back(
      {image_info.image_base, image_info.image_base + image_info.image_size});
  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

}  // namespace

void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info) {
  RegisterDoaxGuestPcFiberConfig(image_info);
  rex::ppc::InstallGuestPcFiberInterpreter();
  REXLOG_INFO("DOAX guest-PC fiber: swap override active for 0x{:08X}",
              kGuestFiberSwapAddress);
}

extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64),
                         0);
}

extern "C" REX_FUNC(DOAX_FiberYield) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  SchedulerFiberGprs saved_gprs{};
  const bool preserve_gprs = NeedsFiberCalleeSavePreserve(caller_lr);
  if (preserve_gprs) {
    SaveSchedulerFiberGprs(ctx, saved_gprs);
  }
  DOAX_FiberContextSwitch(ctx, base);
  if (preserve_gprs) {
    RestoreSchedulerFiberGprs(ctx, saved_gprs);
  }
}
