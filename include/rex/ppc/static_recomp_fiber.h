/**
 * @file static_recomp_fiber.h
 * @brief Host stack unwind for guest fiber swaps under static recompilation.
 *
 * Guest work-queue fiber swaps (FH1 sub_830ED910) restore guest CPU state and
 * call KeSetCurrentStackPointers, then blr to the job resume address. Generated
 * C++ must not return through the nested host call stack after that point.
 * setjmp/longjmp at the KeSet boundary is replaced by cooperative reentry
 * (TakePendingFiberHostReentry) because MSVC /EHsc cannot unwind through C++
 * frames between RunFiberSwap and KeSet (0xC0000028).
 */
#pragma once

#include <cstdint>

namespace rex::ppc {

inline constexpr uint32_t kWorkQueueFiberStackBase = 0x70000000u;
// FH1 work-queue fiber stacks use the low 0x70xxxxxx arena; guest thread stacks
// live around 0x7Exxxxxx and must not match this check (swap-back uses jmp=2).
inline constexpr uint32_t kWorkQueueFiberStackEnd = 0x78000000u;

inline bool IsWorkQueueFiberGuestStack(uint32_t stack_limit) {
  return stack_limit >= kWorkQueueFiberStackBase &&
         stack_limit < kWorkQueueFiberStackEnd;
}

// True when guest r1 lies on a FH1 work-queue fiber stack (not thread stack).
inline bool IsGuestSpOnWorkQueueFiberStack(uint32_t sp) {
  return sp >= kWorkQueueFiberStackBase && sp < kWorkQueueFiberStackEnd;
}

#if defined(_MSC_VER)

// Host boundary around generated fiber swap (sub_830ED910).
// Returns 0 before generated swap; after KeSet, TakePendingFiberHostReentry()
// yields 1 (swap-to-fiber) or 2 (swap-back). No setjmp/longjmp — MSVC /EHsc
// cannot unwind through C++ frames between RunFiberSwap and KeSet (0xC0000028).
int BeginFiberSwapHostBoundary();
void CompleteFiberSwapWithoutReentry();

// Called from KeSetCurrentStackPointers after guest stack fields are updated.
void NotifyFiberStackPointerSwitch(uint32_t stack_limit);

/// Consume deferred host-boundary reentry (1=swap-to-fiber, 2=swap-back, 0=none).
int TakePendingFiberHostReentry();

void EnterFiberJobDispatch();
void LeaveFiberJobDispatch();

/// Nested depth of active fiber swap / job-dispatch boundaries on this thread.
int GetFiberJobDispatchDepth();

// stack_limit from the most recent NotifyFiberStackPointerSwitch on this thread.
uint32_t GetFiberSwapStackLimit();

// Optional title hook: setjmp/longjmp must live in the same module as the handler.
using FiberStackSwitchHandler = void (*)(uint32_t stack_limit);
void SetFiberStackSwitchHandler(FiberStackSwitchHandler handler);

#else

inline int BeginFiberSwapHostBoundary() { return 0; }
inline void CompleteFiberSwapWithoutReentry() {}
inline void NotifyFiberStackPointerSwitch(uint32_t) {}
inline int TakePendingFiberHostReentry() { return 0; }
inline void EnterFiberJobDispatch() {}
inline void LeaveFiberJobDispatch() {}
inline int GetFiberJobDispatchDepth() { return 0; }
inline uint32_t GetFiberSwapStackLimit() { return 0; }
inline void SetFiberStackSwitchHandler(FiberStackSwitchHandler) {}

#endif

}  // namespace rex::ppc
