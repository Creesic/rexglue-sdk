#include <rex/ppc/static_recomp_fiber.h>

#if defined(_MSC_VER)

#include <atomic>

#include <rex/logging.h>

namespace rex::ppc {
namespace {

struct FiberSwapTls {
  bool pending = false;
  int pending_reentry = 0;  // 0=none, 1=swap-to-fiber, 2=swap-back
  int job_dispatch_depth = 0;
  uint32_t last_stack_limit = 0;
};

FiberSwapTls& State() {
  thread_local FiberSwapTls state;
  return state;
}

FiberStackSwitchHandler g_title_handler = nullptr;

void LogReentry(uint32_t stack_limit, int jmp_val) {
  static std::atomic<uint32_t> count{0};
  if (count.fetch_add(1, std::memory_order_relaxed) >= 8) {
    return;
  }
  REXSYS_WARN(
      "StaticRecompFiber: KeSet reentry limit=0x{:08X} jmp={} depth={}",
      stack_limit, jmp_val, State().job_dispatch_depth);
}

}  // namespace

void SetFiberStackSwitchHandler(FiberStackSwitchHandler handler) {
  g_title_handler = handler;
}

int BeginFiberSwapHostBoundary() {
  auto& state = State();
  state.pending = true;
  state.pending_reentry = 0;
  return 0;
}

void CompleteFiberSwapWithoutReentry() {
  auto& state = State();
  state.pending = false;
  state.pending_reentry = 0;
}

void NotifyFiberStackPointerSwitch(uint32_t stack_limit) {
  if (g_title_handler) {
    g_title_handler(stack_limit);
  }

  auto& state = State();
  if (!state.pending) {
    return;
  }

  state.pending = false;
  state.last_stack_limit = stack_limit;

  const bool on_fiber_stack = IsWorkQueueFiberGuestStack(stack_limit);
  state.pending_reentry =
      (!on_fiber_stack && state.job_dispatch_depth > 0) ? 2 : 1;
  LogReentry(stack_limit, state.pending_reentry);
  // Cooperative reentry: KeSet returns normally; RunFiberSwap /
  // InvokeGuestPcFiberKeSet consume pending_reentry. longjmp is unsafe across
  // MSVC /EHsc frames (0xC0000028).
}

int TakePendingFiberHostReentry() {
  auto& state = State();
  const int val = state.pending_reentry;
  state.pending_reentry = 0;
  return val;
}

void EnterFiberJobDispatch() {
  ++State().job_dispatch_depth;
}

void LeaveFiberJobDispatch() {
  auto& depth = State().job_dispatch_depth;
  if (depth > 0) {
    --depth;
  }
}

int GetFiberJobDispatchDepth() {
  return State().job_dispatch_depth;
}

uint32_t GetFiberSwapStackLimit() {
  return State().last_stack_limit;
}

}  // namespace rex::ppc

#endif
