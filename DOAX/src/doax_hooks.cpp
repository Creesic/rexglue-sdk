#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>
#include <utility>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;
constexpr uint32_t kDoaxSchedulerFlagAddr = 0x833B8DF8u;
constexpr uint32_t kDoaxBootPresentByteAddr = 0x833BB763u;
constexpr uint32_t kDoaxIslandOverlayFlagAddr = 0x833B8514u;
constexpr uint32_t kDoaxPresentStateIndexAddr = 0x833B84C8u;

constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;
constexpr uint32_t kSchedulerHeaderBytes = 16;

constexpr uint32_t kDispatcherFiberYieldLr = 0x824C0600u;
constexpr uint32_t kCdf8FiberYieldLr = 0x8258CE4Cu;
constexpr uint32_t kMenuWorkFiberYieldLr = 0x824C15F4u;
constexpr uint32_t kDrainInnerFiberYieldLr = 0x824C0C3Cu;
constexpr uint32_t kAltMenuWorkFiberYieldLr = 0x825A25E0u;
constexpr uint32_t kMenuSceneTransitionLabel12Lr = 0x824C1770u;
constexpr uint32_t kMenuSceneTransitionLabel34Lr = 0x824C191Cu;
constexpr uint32_t kMenuTransitionPlayMovieReturnLr = 0x824C12BCu;
constexpr uint32_t kDoaxTravelMenuItemId = 15u;
constexpr uint32_t kXboxButtonStart = 0x0010u;
constexpr uint32_t kXboxButtonA = 0x1000u;
constexpr uint32_t kPressStartButtons = kXboxButtonA | kXboxButtonStart;

constexpr uint32_t kSmallLogCap = 24;
constexpr const char* kDoaxHooksBuildTag = "doax-hooks-2026-06-26-clean-label12-gate";

uint32_t g_session_run_id = 0;
uint32_t g_fiber_yield_logs = 0;
uint32_t g_movie_logs = 0;
uint32_t g_transition_logs = 0;
uint32_t g_menu_logs = 0;
uint32_t g_input_logs = 0;
uint32_t g_midasm_logs = 0;
uint32_t g_promotion_video_plays = 0;
uint32_t g_last_buttons = 0;
uint8_t* g_doax_hook_guest_base = nullptr;
bool g_press_start_gate_active = false;
bool g_press_start_waiting_for_release = false;
bool g_press_start_user_confirm = false;
bool g_press_start_input_sampled = false;

struct SchedulerSnapshot {
  uint8_t flag0 = 0;
  uint8_t flag1 = 0;
  uint8_t flag2 = 0;
  uint8_t flag3 = 0;
  uint8_t flag4 = 0;
  uint8_t flag5 = 0;
  uint8_t flag6 = 0;
  uint8_t flag7 = 0;
  uint32_t word8 = 0;
  uint32_t word12 = 0;
};

SchedulerSnapshot ReadSchedulerSnapshot(uint8_t* base) {
  SchedulerSnapshot snap{};
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  snap.flag0 = REX_LOAD_U8(sched + 0);
  snap.flag1 = REX_LOAD_U8(sched + 1);
  snap.flag2 = REX_LOAD_U8(sched + 2);
  snap.flag3 = REX_LOAD_U8(sched + 3);
  snap.flag4 = REX_LOAD_U8(sched + 4);
  snap.flag5 = REX_LOAD_U8(sched + 5);
  snap.flag6 = REX_LOAD_U8(sched + 6);
  snap.flag7 = REX_LOAD_U8(sched + 7);
  snap.word8 = REX_LOAD_U32(sched + 8);
  snap.word12 = REX_LOAD_U32(sched + 12);
  return snap;
}

void PreserveSchedulerExceptBytes45(uint8_t* base, const SchedulerSnapshot& before) {
  const SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  if (after.flag0 != before.flag0) {
    REX_STORE_U8(sched + 0, before.flag0);
  }
  if (after.flag1 != before.flag1) {
    REX_STORE_U8(sched + 1, before.flag1);
  }
  if (after.flag2 != before.flag2) {
    REX_STORE_U8(sched + 2, before.flag2);
  }
  if (after.flag3 != before.flag3) {
    REX_STORE_U8(sched + 3, before.flag3);
  }
  if (after.flag6 != before.flag6) {
    REX_STORE_U8(sched + 6, before.flag6);
  }
  if (after.flag7 != before.flag7) {
    REX_STORE_U8(sched + 7, before.flag7);
  }
  if (after.word8 != before.word8) {
    REX_STORE_U32(sched + 8, before.word8);
  }
  if (after.word12 != before.word12) {
    REX_STORE_U32(sched + 12, before.word12);
  }
}

void LogSchedulerSnapshot(uint8_t* base, const char* site) {
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX clean-probe site={} sched f0-7={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} "
      "w8={} w12={} boot_present={} overlay={} present={}",
      site, snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5,
      snap.flag6, snap.flag7, snap.word8, snap.word12, REX_LOAD_U8(kDoaxBootPresentByteAddr),
      REX_LOAD_U8(kDoaxIslandOverlayFlagAddr), REX_LOAD_U32(kDoaxPresentStateIndexAddr));
}

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
    case kMenuWorkFiberYieldLr:
    case kDrainInnerFiberYieldLr:
    case kAltMenuWorkFiberYieldLr:
      return true;
    default:
      return false;
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

void SaveSchedulerHeaderBytes(uint8_t* base, uint8_t* out) {
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    out[i] = REX_LOAD_U8(kDoaxSchedulerFlagAddr + i);
  }
}

void WriteSchedulerHeaderBytes(uint8_t* base, const uint8_t* in) {
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    REX_STORE_U8(kDoaxSchedulerFlagAddr + i, in[i]);
  }
}

void SchedulerGuardedIndirectCall(PPCContext& ctx, uint8_t* base, uint32_t target,
                                  uint64_t return_lr) {
  uint8_t sched_before[kSchedulerHeaderBytes];
  SaveSchedulerHeaderBytes(base, sched_before);
  const uint64_t saved_r30 = ctx.r30.u64;
  const uint64_t saved_r31 = ctx.r31.u64;
  ctx.lr = return_lr;
  REX_CALL_INDIRECT_FUNC(target);
  bool reg_fix = false;
  if (saved_r31 != 0 && ctx.r31.u64 != saved_r31) {
    ctx.r31.u64 = saved_r31;
    reg_fix = true;
  }
  if (saved_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = saved_r30;
    reg_fix = true;
  }
  if (reg_fix) {
    WriteSchedulerHeaderBytes(base, sched_before);
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

void FiberSwapImpl82785670(PPCContext& ctx, uint8_t* base) {
  __imp__DOAX_FiberContextSwitch(ctx, base);
}

void RegisterDoaxGuestPcFiberConfig(const rex::PPCImageInfo& image_info) {
  rex::ppc::GuestPcFiberConfig config;
  config.guest_regions.push_back(
      {image_info.image_base, image_info.image_base + image_info.image_size});
  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

void LogMovieEvent(uint8_t* base, const char* site, uint32_t movie_idx, uint32_t caller_lr) {
  if (g_movie_logs >= kSmallLogCap) {
    return;
  }
  ++g_movie_logs;
  REXKRNL_WARN(
      "DOAX movie-probe site={} idx={} plays={} lr=0x{:08X} boot_present={} overlay={}",
      site, movie_idx, g_promotion_video_plays, caller_lr, REX_LOAD_U8(kDoaxBootPresentByteAddr),
      REX_LOAD_U8(kDoaxIslandOverlayFlagAddr));
}

void LogPressStartGate(uint8_t* base, const char* site, const char* detail) {
  if (g_input_logs >= kSmallLogCap) {
    return;
  }
  ++g_input_logs;
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX press-start-gate site={} detail={} active={} user={} wait_release={} "
      "buttons=0x{:04X} sched={}/{} boot_present={} overlay={}",
      site, detail, g_press_start_gate_active ? 1 : 0, g_press_start_user_confirm ? 1 : 0,
      g_press_start_waiting_for_release ? 1 : 0, g_last_buttons, snap.flag4, snap.flag5,
      REX_LOAD_U8(kDoaxBootPresentByteAddr), REX_LOAD_U8(kDoaxIslandOverlayFlagAddr));
}

void ArmPressStartGate(uint8_t* base, const char* site) {
  g_press_start_gate_active = true;
  g_press_start_user_confirm = false;
  g_press_start_input_sampled = false;
  g_press_start_waiting_for_release = (g_last_buttons & kPressStartButtons) != 0;
  LogPressStartGate(base, site, "armed");
}

void NoteControllerButtons(uint8_t* base, uint32_t buttons, uint32_t user, uint32_t caller_lr) {
  const uint32_t previous = g_last_buttons;
  const bool previous_confirm_down = (previous & kPressStartButtons) != 0;
  const bool confirm_down = (buttons & kPressStartButtons) != 0;
  g_last_buttons = buttons;

  if (!g_press_start_gate_active || g_press_start_user_confirm) {
    return;
  }

  if (!g_press_start_input_sampled) {
    g_press_start_input_sampled = true;
    g_press_start_waiting_for_release = confirm_down;
    LogPressStartGate(base, "input", confirm_down ? "initial-held" : "initial-up");
    return;
  }

  if (g_press_start_waiting_for_release) {
    if (!confirm_down) {
      g_press_start_waiting_for_release = false;
      LogPressStartGate(base, "input", "released");
    }
    return;
  }

  if (confirm_down && !previous_confirm_down) {
    g_press_start_user_confirm = true;
    if (g_input_logs < kSmallLogCap) {
      ++g_input_logs;
      REXKRNL_WARN(
          "DOAX press-start-gate site=input detail=fresh-confirm user={} lr=0x{:08X} "
          "buttons=0x{:04X}",
          user, caller_lr, buttons);
    }
  }
}

}  // namespace

void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info) {
  RegisterDoaxGuestPcFiberConfig(image_info);
  rex::ppc::InstallGuestPcFiberInterpreter();
  ++g_session_run_id;
  g_fiber_yield_logs = 0;
  g_movie_logs = 0;
  g_transition_logs = 0;
  g_menu_logs = 0;
  g_input_logs = 0;
  g_midasm_logs = 0;
  g_promotion_video_plays = 0;
  g_last_buttons = 0;
  g_doax_hook_guest_base = nullptr;
  g_press_start_gate_active = false;
  g_press_start_waiting_for_release = false;
  g_press_start_user_confirm = false;
  g_press_start_input_sampled = false;
  REXLOG_INFO("DOAX guest-PC fiber: swap override active for 0x{:08X}", kGuestFiberSwapAddress);
  REXKRNL_WARN("DOAX session-start run={} hooks={}", g_session_run_id, kDoaxHooksBuildTag);
}

extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64), 0);
}

extern "C" REX_FUNC(DOAX_FiberYield) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t target = static_cast<uint32_t>(ctx.r3.u64);
  SchedulerFiberGprs saved_gprs{};
  const bool preserve_gprs = NeedsFiberCalleeSavePreserve(caller_lr);
  if (preserve_gprs) {
    SaveSchedulerFiberGprs(ctx, saved_gprs);
  }
  DOAX_FiberContextSwitch(ctx, base);
  if (preserve_gprs) {
    RestoreSchedulerFiberGprs(ctx, saved_gprs);
  }
  if (g_fiber_yield_logs < kSmallLogCap &&
      (preserve_gprs || caller_lr == kDispatcherFiberYieldLr || caller_lr == kCdf8FiberYieldLr)) {
    ++g_fiber_yield_logs;
    REXKRNL_WARN("DOAX fiber-yield site=0x{:08X} target=0x{:08X} preserve={} resume=0x{:08X}",
                 caller_lr, target, preserve_gprs ? 1 : 0, static_cast<uint32_t>(ctx.lr));
  }
}

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
  DOAX_FiberYield(ctx, base);
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

extern "C" REX_FUNC(DOAX_SchedulerFiberSwap) {
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  __imp__DOAX_SchedulerFiberSwap(ctx, base);
  PreserveSchedulerExceptBytes45(base, before);
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
  DOAX_FiberYield(ctx, base);
  ctx.lr = 0x824C0604u;
  DOAX_SchedulerDrainDispatch(ctx, base);
  goto loc_824C05DC_hook;
}

extern "C" REX_FUNC(DOAX_PlayMovie) {
  g_doax_hook_guest_base = base;
  const uint32_t movie_idx = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  if (movie_idx == 1 && caller_lr == kMenuTransitionPlayMovieReturnLr) {
    ++g_promotion_video_plays;
  }
  LogMovieEvent(base, "PlayMovie", movie_idx, caller_lr);
  __imp__DOAX_PlayMovie(ctx, base);
}

extern "C" REX_FUNC(DOAX_MenuWorkFiberLoop) {
  g_doax_hook_guest_base = base;
  __imp__DOAX_MenuWorkFiberLoop(ctx, base);
}

extern "C" REX_FUNC(DOAX_MenuTransitionPlayMovie) {
  g_doax_hook_guest_base = base;
  LogSchedulerSnapshot(base, "transition-play-movie-enter");
  __imp__DOAX_MenuTransitionPlayMovie(ctx, base);
  LogSchedulerSnapshot(base, "transition-play-movie-exit");
}

extern "C" REX_FUNC(DOAX_MenuTransitionMoviePoll) {
  g_doax_hook_guest_base = base;
  __imp__DOAX_MenuTransitionMoviePoll(ctx, base);
  if (g_movie_logs < kSmallLogCap) {
    ++g_movie_logs;
    REXKRNL_WARN("DOAX movie-probe site=MoviePoll result={} plays={} handler_done={}",
                 static_cast<uint32_t>(ctx.r3.u64), g_promotion_video_plays,
                 REX_LOAD_U8(static_cast<uint32_t>(ctx.r31.u64) + 2));
  }
}

extern "C" REX_FUNC(DOAX_PostPromotionCleanup) {
  g_doax_hook_guest_base = base;
  REXKRNL_WARN("DOAX promotion-cleanup: plays={} boot_present={} overlay={}",
               g_promotion_video_plays, REX_LOAD_U8(kDoaxBootPresentByteAddr),
               REX_LOAD_U8(kDoaxIslandOverlayFlagAddr));
  __imp__DOAX_PostPromotionCleanup(ctx, base);
  ArmPressStartGate(base, "promotion-cleanup");
  LogSchedulerSnapshot(base, "promotion-cleanup-return");
}

extern "C" REX_FUNC(DOAX_MenuItemConfirm) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t item_id = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t arg2 = static_cast<uint32_t>(ctx.r4.u64);
  if (g_menu_logs < kSmallLogCap) {
    ++g_menu_logs;
    REXKRNL_WARN("DOAX menu-confirm item={} arg={} lr=0x{:08X}", item_id, arg2, caller_lr);
  }
  __imp__DOAX_MenuItemConfirm(ctx, base);
}

extern "C" REX_FUNC(DOAX_MenuSceneTransition) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t scene_id = static_cast<uint32_t>(ctx.r3.u64);
  const bool press_start_scene =
      scene_id == kDoaxTravelMenuItemId &&
      (caller_lr == kMenuSceneTransitionLabel12Lr || caller_lr == kMenuSceneTransitionLabel34Lr);
  if (g_press_start_gate_active && press_start_scene && !g_press_start_user_confirm) {
    LogPressStartGate(base, "scene-transition", "blocked-auto");
    return;
  }
  if (g_transition_logs < kSmallLogCap) {
    ++g_transition_logs;
    const char* label = caller_lr == kMenuSceneTransitionLabel12Lr
                            ? "LABEL_12"
                            : (caller_lr == kMenuSceneTransitionLabel34Lr ? "LABEL_34" : "other");
    REXKRNL_WARN("DOAX scene-transition label={} scene_id={} lr=0x{:08X}", label, scene_id,
                 caller_lr);
    LogSchedulerSnapshot(base, "scene-transition-enter");
  }
  __imp__DOAX_MenuSceneTransition(ctx, base);
  if (g_press_start_gate_active && press_start_scene && g_press_start_user_confirm) {
    g_press_start_gate_active = false;
    LogPressStartGate(base, "scene-transition", "accepted-user");
  }
}

extern "C" REX_FUNC(sub_82782BF0) {
  const uint32_t user = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t state_ptr = ctx.r4.u32;
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  ctx.r5.u64 = ctx.r4.u64;
  ctx.r4.s64 = 0;
  __imp__XamInputGetState(ctx, base);
  uint32_t buttons = 0;
  if (state_ptr != 0) {
    buttons = REX_LOAD_U16(state_ptr + 4);
  }
  NoteControllerButtons(base, buttons, user, caller_lr);
}

bool DOAX_SkipLicenseWarningIntro() {
  if (g_midasm_logs < kSmallLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip license warning");
  }
  return true;
}

bool DOAX_SkipNinjaViHdMovie() {
  if (g_midasm_logs < kSmallLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip ninja_vi_hd");
  }
  return true;
}

bool DOAX_SkipPromotionVideoReplay(PPCRegister& r3) {
  if (r3.u32 != 1 || g_promotion_video_plays == 0) {
    return false;
  }
  if (g_midasm_logs < kSmallLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip promotion replay plays={}", g_promotion_video_plays);
  }
  return true;
}

bool DOAX_SkipAutoPressStartSceneTransition(PPCRegister& r31) {
  (void)r31;
  if (!g_press_start_gate_active) {
    return false;
  }
  if (uint8_t* base = g_doax_hook_guest_base) {
    const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
    const bool pre_overlay = REX_LOAD_U8(kDoaxIslandOverlayFlagAddr) == 0;
    const bool bringup_22 = snap.flag4 == 2 && snap.flag5 == 2;
    if (!g_press_start_user_confirm || pre_overlay || bringup_22) {
      LogPressStartGate(base, "midasm-176C", "skipped-label12");
      return true;
    }
    return false;
  }
  return !g_press_start_user_confirm;
}

bool DOAX_SkipPostReadyAutoScenePrelude() {
  return false;
}

bool DOAX_SkipPressStartAutoSceneTransition() {
  return false;
}
