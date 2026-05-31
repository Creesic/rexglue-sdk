// FH1 thread-start trampoline guard for sub_82A7D410.
//
// After the guest thread proc returns, the game stores the result at 80(r31).
// r31 equals r1 after the prologue but is not restored by all callees; use r1.

#include "generated/fh1_init.h"

#include <atomic>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xthread.h>

namespace {

bool GuestResultSlotAccessible(uint32_t addr) {
  if (addr == 0) {
    return false;
  }
  auto* memory = rex::Runtime::instance()->memory();
  if (!memory) {
    return false;
  }
  if (!memory->IsGuestVirtualCommitted(addr)) {
    return false;
  }
  const uint32_t end = addr + 3;
  if (end < addr) {
    return false;
  }
  return memory->IsGuestVirtualCommitted(end);
}

}  // namespace

extern "C" REX_FUNC(sub_82A7D410) {
  REX_FUNC_PROLOGUE();
  uint32_t ea{};

  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  ea = -128 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;

  ctx.r30.u64 = ctx.r3.u64;
  ctx.r29.u64 = ctx.r4.u64;

  const uint32_t result_slot = ctx.r1.u32 + 80;
  REX_STORE_U32(result_slot, 0);

  ctx.r3.s64 = 1;
  ctx.lr = 0x82A7D440;
  sub_82A7C890(ctx, base);

  ctx.r3.u64 = ctx.r29.u64;
  ctx.ctr.u64 = ctx.r30.u64;
  ctx.lr = 0x82A7D44C;
  REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);

  if (GuestResultSlotAccessible(result_slot)) {
    REX_STORE_U32(result_slot, ctx.r3.u32);
  } else if (auto* thread = rex::system::XThread::TryGetCurrentThread()) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      uint32_t limit = thread->stack_limit();
      uint32_t base = thread->stack_base();
      if (auto* kthread = thread->guest_object<rex::system::X_KTHREAD>()) {
        limit = static_cast<uint32_t>(kthread->stack_limit);
        base = static_cast<uint32_t>(kthread->stack_base);
      }
      REXSYS_WARN(
          "FH1 sub_82A7D410: result slot 0x{:08X} not writable (stack [{:08X},{:08X}) "
          "r1=0x{:08X}); leaving prior slot value",
          result_slot, limit, base, ctx.r1.u32);
    }
  }

  ctx.r3.s64 = 0;
  ctx.lr = 0x82A7D458;
  sub_82A7C890(ctx, base);

  if (GuestResultSlotAccessible(result_slot)) {
    ctx.r3.u64 = REX_LOAD_U32(result_slot);
  } else {
    ctx.r3.u64 = 0;
  }
  ctx.lr = 0x82A7D46C;
  __imp__ExTerminateThread(ctx, base);

  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.lr = 0x82A7D47C;
  sub_82A6F0B0(ctx, base);

  ctx.r1.u64 = REX_LOAD_U32(ctx.r1.u32 + 0);
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
}
