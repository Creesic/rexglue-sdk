#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>
#include <utility>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

// ============================================================================
// DOAX hooks — CLEAN BASELINE + Group 1 (guest-PC fiber reentry) ONLY.
//
// Restart from a from-scratch baseline. The complete prior hook set is archived
// at DOAX/archive/full-hooks-2026-06-27/ (see its README for the 6 groups +
// restore workflow). This file restores ONLY the load-bearing fiber reentry, in
// MINIMAL form:
//   - DOAX_FiberContextSwitch (0x82785670) routed through the OS-fiber-backed
//     rex::ppc::RunFiberSwap. The static recomp needs this because the guest
//     context switch rewrites LR + the stack (KeSetCurrentStackPointers), which
//     a straight-line recompile cannot represent.
//   - Guest-region config registration + interpreter install.
//
// DELIBERATELY OMITTED vs the archive (suspected cruft, not core fiber):
//   * The DOAX_FiberYield hook. Guest 0x82783210 is a thunk -> the context
//     switch, so the generated yield already routes through the hook below.
//   * The callee-saved GPR (r14-r31) preserve band-aid that was gated to 3
//     specific menu/scheduler yield LRs (NeedsFiberCalleeSavePreserve). If a
//     menu fiber yield corrupts r14-r31 across a swap-back, that is a real bug
//     in the recompiled context switch / RunFiberSwap to fix UNIFORMLY in the
//     runtime — not three per-LR patches here.
//   * All press-start / menu / scheduler state-forcing and the ~40 g_* resets.
// ============================================================================

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

// Group 4 (partial): boot intro skips. Bounded logging so we can confirm each
// midasm hook fires and correlate with the on-screen sequence
// (legal/health warning -> Team NINJA movie -> Press Start).
uint32_t g_midasm_logs = 0;
constexpr uint32_t kMidasmLogCap = 32;

// The guest's own register save/restore (the recompiled context switch). Passed
// to RunFiberSwap, which uses it to move PPCContext between the current and the
// target fiber's guest context block on either side of the host-fiber switch.
void FiberSwapImpl82785670(PPCContext& ctx, uint8_t* base) {
  __imp__DOAX_FiberContextSwitch(ctx, base);
}

void RegisterDoaxGuestPcFiberConfig(const rex::PPCImageInfo& image_info) {
  rex::ppc::GuestPcFiberConfig config;
  config.guest_regions.push_back(
      {image_info.image_base, image_info.image_base + image_info.image_size});
  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

// --- Groups 2 + 3: scheduler / work-queue register handling --------------------
// The recompiled scheduler/work-queue code loses callee-saved registers (r29-r31)
// across its embedded fiber yields and ends up dispatching a worker fiber with a
// null "job" (guest r31 == 0), which faults writing through it in DrainWake.
// These reimplementations re-derive r29-r31 from the work-queue table constants
// rather than trusting them across the yield. (Combed from the archive: dropped
// the logging/diag, the SchedulerSnapshot state-forcing, and the GPR band-aid
// that the runtime RunFiberSwap non-volatile preserve now handles.)
constexpr uint32_t kDoaxSchedulerFlagAddr = 0x833B8DF8u;
constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;
constexpr uint32_t kSchedulerHeaderBytes = 16;

void SchedulerGuardedIndirectCall(PPCContext& ctx, uint8_t* base, uint32_t target,
                                  uint64_t return_lr) {
  const uint64_t saved_r30 = ctx.r30.u64;
  const uint64_t saved_r31 = ctx.r31.u64;
  ctx.lr = return_lr;
  REX_CALL_INDIRECT_FUNC(target);
  // Re-derive callee-saved r30/r31 if the embedded fiber yield in the callee
  // dropped them (the null-deref crash guard). Register-only; the header is left
  // exactly as the callee wrote it.
  if (saved_r31 != 0 && ctx.r31.u64 != saved_r31) {
    ctx.r31.u64 = saved_r31;
  }
  if (saved_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = saved_r30;
  }
}

void RestoreDrainCallerRegs(PPCContext& ctx, uint64_t caller_lr, uint64_t caller_r29,
                            uint64_t caller_r30, uint64_t caller_r31) {
  if (static_cast<uint32_t>(caller_lr) == 0x824C0604u) {
    ctx.r29.u64 = kDoaxWorkQueueSegBase;
    ctx.r30.u64 = kDoaxWorkQueueSegBase;
    ctx.r31.u64 = kDoaxWorkQueueTableBase;
  } else {
    if (caller_r29 != 0) {
      ctx.r29.u64 = caller_r29;
    }
    if (caller_r30 != 0) {
      ctx.r30.u64 = caller_r30;
    }
    if (caller_r31 != 0) {
      ctx.r31.u64 = caller_r31;
    }
  }
  if (caller_lr != 0) {
    ctx.lr = caller_lr;
  }
}

}  // namespace

void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info) {
  RegisterDoaxGuestPcFiberConfig(image_info);
  rex::ppc::InstallGuestPcFiberInterpreter();
  REXKRNL_WARN("DOAX Group-1 baseline: guest-PC fiber reentry only (swap override 0x{:08X})",
               kGuestFiberSwapAddress);
}

extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64), 0);
}

// --- Group 4 (partial): boot intro skips (midasm hooks) ------------------------
// Each returns true so the recomp jumps to the hook's jump_address_on_true,
// skipping the intro's display/wait. Confirmed against the on-screen sequence.

bool DOAX_SkipLicenseWarningIntro() {
  if (g_midasm_logs < kMidasmLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip legal/health warning");
  }
  return true;
}

bool DOAX_SkipNinjaViHdMovie() {
  if (g_midasm_logs < kMidasmLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip Team NINJA movie");
  }
  return true;
}

// --- Groups 2 + 3: scheduler / work-queue reimplementations --------------------
// Embedded yields call DOAX_FiberContextSwitch directly (the runtime RunFiberSwap
// preserves non-volatiles); register re-derivation handles the null-r31 dispatch.

extern "C" REX_FUNC(DOAX_WorkQueueSlotWake) {
  const uint64_t caller_r31 = ctx.r31.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  __imp__DOAX_WorkQueueSlotWake(ctx, base);
  ctx.r31.u64 = caller_r31 != 0 ? caller_r31 : kDoaxSchedulerFlagAddr;
  if (caller_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = caller_r30;
  }
}

extern "C" REX_FUNC(sub_8258CDF8) {
  REX_FUNC_PROLOGUE(sub_8258CDF8);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.r10.s64 = kDoaxWorkQueueSegBase;
  ctx.r11.s64 = kDoaxWorkQueueTableBase;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r10.u32 + 13560);
  ctx.r10.s64 = static_cast<int64_t>(ctx.r10.u64 * static_cast<uint64_t>(28));
  ctx.r31.u64 = ctx.r10.u64 + ctx.r11.u64;
  const uint64_t work_entry = ctx.r31.u64;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 24);
  if (ctx.r11.u32 != 0) {
    SchedulerGuardedIndirectCall(ctx, base, ctx.r11.u32, 0x8258CE34u);
  }
  ctx.r31.u64 = work_entry;
  ctx.r11.s64 = kDoaxWorkQueueSegBase;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
  ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 12884);
  ctx.r11.u64 = ctx.r10.u64 | 8;
  REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
  ctx.lr = 0x8258CE4Cu;
  DOAX_FiberContextSwitch(ctx, base);
  ctx.r31.u64 = work_entry;
  ctx.r1.s64 = ctx.r1.s64 + 96;
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
  ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
}

extern "C" REX_FUNC(DOAX_SchedulerDrainWake) {
  REX_FUNC_PROLOGUE(DOAX_SchedulerDrainWake);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.r11.s64 = -2093219840;
  ctx.r31.s64 = ctx.r11.s64 + -29192;
  ctx.r11.u64 = REX_LOAD_U8(ctx.r31.u32 + 0);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 1, ctx.xer);
  if (!ctx.cr6.eq) goto loc_824C0910_hook;
  ctx.r11.s64 = -2101149696;
  ctx.r10.u64 = REX_LOAD_U8(ctx.r31.u32 + 4);
  ctx.r3.u64 = ctx.r31.u64;
  ctx.r11.s64 = ctx.r11.s64 + -28360;
  ctx.r10.u64 = __builtin_rotateleft32(ctx.r10.u32, 4);
  ctx.r11.s64 = ctx.r11.s64 + 12;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r11.u32);
  ctx.ctr.u64 = ctx.r11.u64;
  SchedulerGuardedIndirectCall(ctx, base, ctx.ctr.u32, 0x824C0900u);
  ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  ctx.r3.s64 = 1;
  ctx.lr = 0x824C0908u;
  DOAX_WorkQueueSlotWake(ctx, base);
  ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  ctx.r11.s64 = 0;
  REX_STORE_U8(ctx.r31.u32 + 0, ctx.r11.u8);
loc_824C0910_hook:
  ctx.r1.s64 = ctx.r1.s64 + 96;
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
  ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
}

extern "C" REX_FUNC(DOAX_SchedulerDrainDispatch) {
  const uint64_t caller_lr = ctx.lr;
  const uint64_t caller_r29 = ctx.r29.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  const uint64_t caller_r31 = ctx.r31.u64;
  __imp__DOAX_SchedulerDrainDispatch(ctx, base);
  RestoreDrainCallerRegs(ctx, caller_lr, caller_r29, caller_r30, caller_r31);
}

extern "C" REX_FUNC(DOAX_WorkQueueDispatchLoop) {
  REX_FUNC_PROLOGUE(DOAX_WorkQueueDispatchLoop);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  ctx.lr = 0x824C05C0u;
  __savegprlr_28(ctx, base);
  ea = -128 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.lr = 0x824C05C8u;
  sub_824C0608(ctx, base);
  ctx.r28.s64 = 1;
loc_824C05DC_hook:
  ctx.r29.s64 = kDoaxWorkQueueSegBase;
  ctx.r30.s64 = kDoaxWorkQueueSegBase;
  ctx.r31.s64 = kDoaxWorkQueueTableBase;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 13560);
  ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 12884);
  ctx.r11.s64 = static_cast<int64_t>(ctx.r11.u64 * static_cast<uint64_t>(28));
  ctx.r11.u64 = ctx.r11.u64 + ctx.r31.u64;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
  REX_STORE_U32(ctx.r11.u32 + 4, ctx.r28.u32);
  ctx.r10.u64 =
      __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFFFFFFFFFB;
  REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
  ctx.lr = 0x824C0600u;
  DOAX_FiberContextSwitch(ctx, base);
  ctx.lr = 0x824C0604u;
  DOAX_SchedulerDrainDispatch(ctx, base);
  goto loc_824C05DC_hook;
}
