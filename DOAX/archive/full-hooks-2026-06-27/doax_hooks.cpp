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
constexpr uint32_t kDoaxMenuFiberActiveAddr = 0x833B8DE8u;
constexpr uint32_t kDoaxMenuFiberDeaAddr = 0x833B8DEAu;
constexpr uint32_t kDoaxMenuFiberDebAddr = 0x833B8DEBu;
constexpr uint32_t kDoaxMenuItemIdByteAddr = 0x833B8DECu;
constexpr uint32_t kDoaxMenuFiberDefAddr = 0x833B8DEFu;
constexpr uint32_t kDoaxMenuTransitionDf0Addr = 0x833B8DF0u;
constexpr uint32_t kDoaxMenuTransitionDf4Addr = 0x833B8DF4u;
constexpr uint32_t kDoaxMenuTableAddr = 0x82C2910Cu;
constexpr uint32_t kDoaxMenuTableCursorAddr = 0x83983F13u;
constexpr uint32_t kDoaxMenuTableMaskAddr = 0x833B8DE9u;
constexpr uint32_t kDoaxMenuSelectedIndexAddr = 0x833B745Fu;
constexpr uint32_t kDoaxMenuSceneIdTableAddr = 0x833B851Cu;
constexpr uint32_t kDoaxMenuSpritePumpActiveAddr = 0x839472D8u;
constexpr uint32_t kDoaxMenuOptionMainStateAddr = 0x839472E0u;
constexpr uint32_t kDoaxMenuOptionAuxStateAddr = 0x839473A0u;
constexpr uint32_t kDoaxMenuOptionIconStateAddr = 0x839473E0u;
constexpr uint32_t kDoaxMenuSelectionStateAddr = 0x8394743Cu;
constexpr uint32_t kDoaxMenuOptionSlotStateAddr = 0x83947700u;
constexpr uint32_t kDoaxMenuSpriteReadyFlagAddr = 0x833A1D2Cu;
// Per-context mapped menu action words (DOAX_MainMenuRegisterSlots output); 4
// contexts, 44 bytes (11 dwords) each. sub_8250BB60 reads bits 0x10|0x100 of the
// first dword per context as the confirm/select action.
constexpr uint32_t kDoaxMenuActionWordAddr = 0x834A3510u;
constexpr uint32_t kDoaxMenuActionStride = 44u;
constexpr uint32_t kDoaxMenuActionContexts = 4u;
constexpr uint32_t kDoaxMenuConfirmActionBits = 0x110u;

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

constexpr uint32_t kSmallLogCap = 64;
constexpr uint32_t kTimelineLogCap = 240;
constexpr uint32_t kPressStartMenuKickLimit = 4;
constexpr const char* kDoaxHooksBuildTag =
    "doax-hooks-2026-06-26-menu-confirm-release-gate";

// Diagnostic countdown accelerator: per-frame step subtracted from the guest's
// perceived timeline value so a ~900-frame transition reaches frame 30 (def=1)
// then 0 (advance to LABEL_34) in ~0.5s instead of ~27s. Disabled for the
// scheduler-advance probe run so the timeline behaves naturally.
constexpr bool kDoaxDiagTimelineAccel = false;
constexpr uint32_t kDiagAccelStep = 60;

// Diagnostic scheduler-advance probe: log DrainDispatch's mode-advance variables
// (mode/DFD/DFA/DFB/DFE countdown/DFF swap/phase/counter) + menu fiber dea/deb/def
// each tick (throttled on structural change + periodic heartbeat) to pinpoint why
// the scheduler never advances out of mode 2.
constexpr bool kDoaxDiagSchedProbe = false;
constexpr uint32_t kSchedProbeCap = 500;
constexpr uint32_t kSchedProbeHeartbeat = 150;

// Diagnostic experiment: when true, stand down ALL synthetic press-start state
// forcing on the timeline/transition path (no def prime, no timeline r3=0, no
// LABEL_34 force-exit, no cleanup park, no scheduler restore) so the title->menu
// transition animation runs unmolested. We only LOG the animation-slot state at
// each DOAX_MenuTransitionTimeline (sub_824195C8) poll. The run is expected to
// go black as the raw recomp does; we are observing WHY the animation never
// reaches playing-state 3 / frame 30, not fixing it. Flip back to false to
// restore the forcing stack.
constexpr bool kDoaxDiagTimelineStanddown = true;

// Animation-slot field offsets read by sub_824195C8 (DOAX_MenuTransitionTimeline).
// slot = 4144 * dword_839BC4B8 + *(unk_839BC0F0 + 928).
constexpr uint32_t kDoaxAnimManagerSlotArrayPtrOff = 928;
constexpr uint32_t kDoaxAnimSlotStride = 4144;
constexpr uint32_t kDoaxAnimSlotFrameFloatOff = 1940;   // *(float*)(slot+1940)
constexpr uint32_t kDoaxAnimSlotKeyframePtrOff = 1948;  // *(slot+1948)
constexpr uint32_t kDoaxAnimSlotPlayStateOff = 3580;    // must == 3 to return a frame
constexpr uint32_t kDoaxAnimSlotValidFlagOff = 3620;    // 0 -> sub_824195C8 returns -1
constexpr uint32_t kDoaxAnimSlotArm0Off = 2928;         // v4[732], set by sub_82418D10
constexpr uint32_t kDoaxAnimSlotArm1Off = 2932;         // v4[733]=1, set by sub_82418D10

uint32_t g_session_run_id = 0;
uint32_t g_fiber_yield_logs = 0;
uint32_t g_movie_logs = 0;
uint32_t g_transition_logs = 0;
uint32_t g_menu_logs = 0;
uint32_t g_input_logs = 0;
uint32_t g_midasm_logs = 0;
uint32_t g_dismiss_logs = 0;
uint32_t g_menu_kick_logs = 0;
uint32_t g_menu_probe_logs = 0;
uint32_t g_real_menu_logs = 0;
uint32_t g_last_real_menu_sig0 = 0xFFFFFFFFu;
uint32_t g_last_real_menu_sig1 = 0xFFFFFFFFu;

// TEMP_DIAG: FM2 event-dispatch probe (sub_82768270). Resolves whether, in the
// black case, the tween-completion dispatcher is CALLED-but-no-match (a
// registration race) vs NOT-CALLED (an upstream event source stalled). See
// [[doax-menu-fiber-root-cause]] FM2 chain.
uint32_t g_fm2_dispatch_logs = 0;
constexpr uint32_t kFM2DispatchLogCap = 12000;
uint32_t g_timeline_logs = 0;
uint32_t g_last_timeline_sig = 0xFFFFFFFFu;
bool g_diag_accel_active = false;
int32_t g_diag_accel_synth = 0;
// Menu-confirm release-gate state.
bool g_menu_confirm_wait_release = false;
bool g_menu_active_prev = false;
uint32_t g_sched_probe_logs = 0;
uint32_t g_sched_probe_tick = 0;
uint32_t g_last_sched_sig = 0xFFFFFFFFu;
uint32_t g_press_start_menu_kicks = 0;
uint32_t g_promotion_video_plays = 0;
uint32_t g_last_buttons = 0;
uint8_t* g_doax_hook_guest_base = nullptr;
bool g_press_start_gate_active = false;
bool g_press_start_waiting_for_release = false;
bool g_press_start_user_confirm = false;
bool g_press_start_input_sampled = false;
bool g_press_start_overlay_dismiss = false;
bool g_press_start_dismiss_unwound = false;
bool g_press_start_completion_primed = false;
bool g_press_start_label34_exit_forced = false;
bool g_press_start_cleanup_handoff_pending = false;

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

void LogPressStartDismiss(uint8_t* base, const char* site, const char* detail) {
  if (g_dismiss_logs >= kSmallLogCap) {
    return;
  }
  ++g_dismiss_logs;
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX press-start-dismiss site={} detail={} sched f0-7={:02X}{:02X}{:02X}{:02X}"
      "{:02X}{:02X}{:02X}{:02X} mode={}/{} boot_present={} overlay={} present={} "
      "dea={} deb={} def={} item={} df0={} df4={} buttons=0x{:04X}",
      site, detail, snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5,
      snap.flag6, snap.flag7, snap.flag4, snap.flag5, REX_LOAD_U8(kDoaxBootPresentByteAddr),
      REX_LOAD_U8(kDoaxIslandOverlayFlagAddr), REX_LOAD_U32(kDoaxPresentStateIndexAddr),
      REX_LOAD_U8(kDoaxMenuFiberDeaAddr), REX_LOAD_U8(kDoaxMenuFiberDebAddr),
      REX_LOAD_U8(kDoaxMenuFiberDefAddr), REX_LOAD_U8(kDoaxMenuItemIdByteAddr),
      REX_LOAD_U32(kDoaxMenuTransitionDf0Addr), REX_LOAD_U32(kDoaxMenuTransitionDf4Addr),
      g_last_buttons);
}

uint32_t LoadMenuStateByte(uint8_t* base, uint32_t addr, uint32_t index, uint32_t stride) {
  return REX_LOAD_U8(addr + index * stride);
}

void LogRealMenuPumpState(uint8_t* base, const char* site, const char* detail,
                          bool force_log = false) {
  const bool press_start_relevant =
      g_press_start_gate_active || g_press_start_overlay_dismiss || g_press_start_dismiss_unwound;
  if (!press_start_relevant && g_real_menu_logs >= 8) {
    return;
  }
  const uint32_t pump = REX_LOAD_U8(kDoaxMenuSpritePumpActiveAddr);
  const uint32_t opt0 = LoadMenuStateByte(base, kDoaxMenuOptionMainStateAddr, 0, 48);
  const uint32_t opt1 = LoadMenuStateByte(base, kDoaxMenuOptionMainStateAddr, 1, 48);
  const uint32_t opt2 = LoadMenuStateByte(base, kDoaxMenuOptionMainStateAddr, 2, 48);
  const uint32_t opt3 = LoadMenuStateByte(base, kDoaxMenuOptionMainStateAddr, 3, 48);
  const uint32_t aux0 = LoadMenuStateByte(base, kDoaxMenuOptionAuxStateAddr, 0, 16);
  const uint32_t aux1 = LoadMenuStateByte(base, kDoaxMenuOptionAuxStateAddr, 1, 16);
  const uint32_t aux2 = LoadMenuStateByte(base, kDoaxMenuOptionAuxStateAddr, 2, 16);
  const uint32_t aux3 = LoadMenuStateByte(base, kDoaxMenuOptionAuxStateAddr, 3, 16);
  const uint32_t icon0 = LoadMenuStateByte(base, kDoaxMenuOptionIconStateAddr, 0, 20);
  const uint32_t icon1 = LoadMenuStateByte(base, kDoaxMenuOptionIconStateAddr, 1, 20);
  const uint32_t icon2 = LoadMenuStateByte(base, kDoaxMenuOptionIconStateAddr, 2, 20);
  const uint32_t icon3 = LoadMenuStateByte(base, kDoaxMenuOptionIconStateAddr, 3, 20);
  const uint32_t slot0 = LoadMenuStateByte(base, kDoaxMenuOptionSlotStateAddr, 0, 16);
  const uint32_t slot1 = LoadMenuStateByte(base, kDoaxMenuOptionSlotStateAddr, 1, 16);
  const uint32_t slot2 = LoadMenuStateByte(base, kDoaxMenuOptionSlotStateAddr, 2, 16);
  const uint32_t slot3 = LoadMenuStateByte(base, kDoaxMenuOptionSlotStateAddr, 3, 16);
  const uint32_t sel0_state = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 0);
  const uint32_t sel0_item = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 2);
  const uint32_t sel0_aux = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 3);
  const uint32_t sel1_state = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 16);
  const uint32_t sel1_item = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 18);
  const uint32_t sel1_aux = REX_LOAD_U8(kDoaxMenuSelectionStateAddr + 19);
  const uint32_t table0 = REX_LOAD_U32(kDoaxMenuTableAddr + 0);
  const uint32_t table1 = REX_LOAD_U32(kDoaxMenuTableAddr + 4);
  const uint32_t table2 = REX_LOAD_U32(kDoaxMenuTableAddr + 8);
  const uint32_t table3 = REX_LOAD_U32(kDoaxMenuTableAddr + 12);
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  const uint32_t sig0 = pump | (sel0_state << 8) | (sel0_item << 16) | (sel1_state << 24);
  const uint32_t sig1 = snap.flag4 | (snap.flag5 << 4) |
                        (REX_LOAD_U8(kDoaxIslandOverlayFlagAddr) << 8) |
                        (REX_LOAD_U8(kDoaxMenuFiberDeaAddr) << 16) |
                        (REX_LOAD_U8(kDoaxMenuFiberDebAddr) << 24);
  if (!force_log && sig0 == g_last_real_menu_sig0 && sig1 == g_last_real_menu_sig1) {
    return;
  }
  g_last_real_menu_sig0 = sig0;
  g_last_real_menu_sig1 = sig1;
  if (g_real_menu_logs >= kSmallLogCap) {
    return;
  }
  ++g_real_menu_logs;
  REXKRNL_WARN(
      "DOAX real-menu-pump site={} detail={} pump={} opt={},{},{},{} aux={},{},{},{} "
      "icon={},{},{},{} slot={},{},{},{} sel0={}/{}/{} sel1={}/{}/{} ready=0x{:08X} "
      "sched={}/{} overlay={} present={} dea={} deb={} def={} item={} df0={} df4={} "
      "cursor={} mask=0x{:02X} selected={} scenes={}/{} table={:08X},{:08X},{:08X},{:08X} "
      "buttons=0x{:04X}",
      site, detail, pump, opt0, opt1, opt2, opt3, aux0, aux1, aux2, aux3, icon0, icon1, icon2,
      icon3, slot0, slot1, slot2, slot3, sel0_state, sel0_item, sel0_aux, sel1_state, sel1_item,
      sel1_aux, REX_LOAD_U32(kDoaxMenuSpriteReadyFlagAddr), snap.flag4, snap.flag5,
      REX_LOAD_U8(kDoaxIslandOverlayFlagAddr),
      REX_LOAD_U32(kDoaxPresentStateIndexAddr), REX_LOAD_U8(kDoaxMenuFiberDeaAddr),
      REX_LOAD_U8(kDoaxMenuFiberDebAddr), REX_LOAD_U8(kDoaxMenuFiberDefAddr),
      REX_LOAD_U8(kDoaxMenuItemIdByteAddr), REX_LOAD_U32(kDoaxMenuTransitionDf0Addr),
      REX_LOAD_U32(kDoaxMenuTransitionDf4Addr), REX_LOAD_U8(kDoaxMenuTableCursorAddr),
      REX_LOAD_U8(kDoaxMenuTableMaskAddr), REX_LOAD_U8(kDoaxMenuSelectedIndexAddr),
      REX_LOAD_U8(kDoaxMenuSceneIdTableAddr + 0), REX_LOAD_U8(kDoaxMenuSceneIdTableAddr + 1),
      table0, table1, table2, table3, g_last_buttons);
}

void LogMainMenuProbe(uint8_t* base, const char* site, uint32_t caller_lr) {
  const bool press_start_relevant =
      g_press_start_overlay_dismiss || g_press_start_dismiss_unwound;
  if (!press_start_relevant && g_menu_probe_logs >= 8) {
    return;
  }
  if (g_menu_probe_logs >= kSmallLogCap) {
    return;
  }
  ++g_menu_probe_logs;
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX menu-probe site={} lr=0x{:08X} sched f0-7={:02X}{:02X}{:02X}{:02X}"
      "{:02X}{:02X}{:02X}{:02X} boot_present={} overlay={} present={} "
      "dismiss={} unwound={} kicks={}",
      site, caller_lr, snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5,
      snap.flag6, snap.flag7, REX_LOAD_U8(kDoaxBootPresentByteAddr),
      REX_LOAD_U8(kDoaxIslandOverlayFlagAddr), REX_LOAD_U32(kDoaxPresentStateIndexAddr),
      g_press_start_overlay_dismiss ? 1 : 0, g_press_start_dismiss_unwound ? 1 : 0,
      g_press_start_menu_kicks);
}

// Logs the transition animation-slot state read by sub_824195C8 each poll.
// a1 = animation manager addr (&unk_839BC0F0), a2 = slot index (dword_839BC4B8),
// real_ret = the function's unforced return value (frame number, 0, or -1).
// Throttled on change so the case-3 polling loop does not flood the log.
void LogTimelineSlot(uint8_t* base, const char* site, uint32_t a1, uint32_t a2,
                     int32_t real_ret) {
  if (g_timeline_logs >= kTimelineLogCap) {
    return;
  }
  if (a2 >= 8) {
    ++g_timeline_logs;
    REXKRNL_WARN("DOAX timeline-slot site={} a1=0x{:08X} index={} OUT_OF_RANGE ret={}",
                 site, a1, a2, real_ret);
    return;
  }
  const uint32_t slot_array = REX_LOAD_U32(a1 + kDoaxAnimManagerSlotArrayPtrOff);
  if (slot_array == 0) {
    const uint32_t sig = 0x8000u | (a2 & 0xFF);
    if (sig == g_last_timeline_sig) {
      return;
    }
    g_last_timeline_sig = sig;
    ++g_timeline_logs;
    REXKRNL_WARN("DOAX timeline-slot site={} index={} slot_array=NULL ret={}", site, a2,
                 real_ret);
    return;
  }
  const uint32_t slot = slot_array + kDoaxAnimSlotStride * a2;
  const uint32_t valid = REX_LOAD_U32(slot + kDoaxAnimSlotValidFlagOff);
  const uint32_t state = REX_LOAD_U32(slot + kDoaxAnimSlotPlayStateOff);
  const uint32_t frame_bits = REX_LOAD_U32(slot + kDoaxAnimSlotFrameFloatOff);
  const uint32_t kf_ptr = REX_LOAD_U32(slot + kDoaxAnimSlotKeyframePtrOff);
  const uint32_t arm0 = REX_LOAD_U32(slot + kDoaxAnimSlotArm0Off);
  const uint32_t arm1 = REX_LOAD_U32(slot + kDoaxAnimSlotArm1Off);
  // Signature folds the values that decide the def fork: play state, valid flag,
  // the integer part of the frame float, and the return. Only log on change.
  const uint32_t sig = (state & 0xF) | ((valid != 0 ? 1u : 0u) << 4) |
                       ((frame_bits >> 16) << 5) |
                       ((static_cast<uint32_t>(real_ret) & 0xFF) << 21);
  if (sig == g_last_timeline_sig) {
    return;
  }
  g_last_timeline_sig = sig;
  ++g_timeline_logs;
  REXKRNL_WARN(
      "DOAX timeline-slot site={} index={} valid={} play_state={} frame_bits=0x{:08X} "
      "kf_ptr=0x{:08X} arm0={} arm1={} ret={} dea={} deb={} def={} df0={} df4={} present={}",
      site, a2, valid, state, frame_bits, kf_ptr, arm0, arm1, real_ret,
      REX_LOAD_U8(kDoaxMenuFiberDeaAddr), REX_LOAD_U8(kDoaxMenuFiberDebAddr),
      REX_LOAD_U8(kDoaxMenuFiberDefAddr), REX_LOAD_U32(kDoaxMenuTransitionDf0Addr),
      REX_LOAD_U32(kDoaxMenuTransitionDf4Addr), REX_LOAD_U32(kDoaxPresentStateIndexAddr));
}

// Logs DrainDispatch's mode-advance state machine. Throttled on a structural
// signature (mode/DFD/DFA/DFB/DFF/phase/active + menu fiber dea/deb/def) so the
// countdown (DFE) and counter ticking does not flood; a periodic heartbeat still
// emits so DFE/counter progress is visible while parked.
void LogSchedulerAdvance(uint8_t* base, const char* site) {
  if (g_sched_probe_logs >= kSchedProbeCap) {
    return;
  }
  const uint32_t s = kDoaxSchedulerFlagAddr;
  const uint8_t active = REX_LOAD_U8(s + 0);
  const uint8_t phase = REX_LOAD_U8(s + 1);   // DF9
  const uint8_t dfa = REX_LOAD_U8(s + 2);     // advance-now flag
  const uint8_t dfb = REX_LOAD_U8(s + 3);
  const uint8_t mode = REX_LOAD_U8(s + 4);    // DFC current mode
  const uint8_t dfd = REX_LOAD_U8(s + 5);
  const uint8_t dfe = REX_LOAD_U8(s + 6);     // countdown
  const uint8_t dff = REX_LOAD_U8(s + 7);     // swap flag
  const uint32_t c8 = REX_LOAD_U32(s + 8);
  const uint8_t mfa = REX_LOAD_U8(kDoaxMenuFiberActiveAddr);
  const uint8_t dea = REX_LOAD_U8(kDoaxMenuFiberDeaAddr);
  const uint8_t deb = REX_LOAD_U8(kDoaxMenuFiberDebAddr);
  const uint8_t def = REX_LOAD_U8(kDoaxMenuFiberDefAddr);
  const uint32_t sig = (active & 1u) | ((phase & 3u) << 1) | ((dfa & 1u) << 3) |
                       ((dfb & 3u) << 4) | ((mode & 7u) << 6) | ((dfd & 7u) << 9) |
                       ((dff & 3u) << 12) | ((def & 1u) << 14) | ((dea & 7u) << 15) |
                       ((mfa & 1u) << 18) | (static_cast<uint32_t>(deb) << 19);
  ++g_sched_probe_tick;
  const bool changed = sig != g_last_sched_sig;
  const bool heartbeat = (g_sched_probe_tick % kSchedProbeHeartbeat) == 0;
  if (!changed && !heartbeat) {
    return;
  }
  g_last_sched_sig = sig;
  ++g_sched_probe_logs;
  REXKRNL_WARN(
      "DOAX sched-adv site={} {} active={} mode={} dfd={} phase={} dfa={} dfb={} dfe={} "
      "dff={} c8={} | mf_active={} dea={} deb={} def={} present={}",
      site, changed ? "CHANGE" : "hb", active, mode, dfd, phase, dfa, dfb, dfe, dff, c8,
      mfa, dea, deb, def, REX_LOAD_U32(kDoaxPresentStateIndexAddr));
}

// Menu-confirm release-gate: the single A press that dismisses "Press Start" gets
// mapped into the just-opened menu's confirm action (dword_834A3510 & 0x110),
// auto-confirming the default item (Travel) and deadlocking the scheduler. Require
// the confirm button to be released once after the menu opens before any confirm
// is honored. Runs in the DOAX_MainMenuRegisterSlots hook after the input mapping,
// before sub_8250BB60 reads the action word in the same frame.
void ApplyMenuConfirmReleaseGate(uint8_t* base) {
  const bool menu_active = REX_LOAD_U8(kDoaxMenuFiberActiveAddr) != 0;
  const bool confirm_held = (g_last_buttons & kXboxButtonA) != 0;
  if (menu_active && !g_menu_active_prev) {
    // Menu just opened: if the confirm button is still held from the Press Start
    // dismiss, gate confirms until it is released.
    g_menu_confirm_wait_release = confirm_held;
  }
  g_menu_active_prev = menu_active;
  if (!g_menu_confirm_wait_release) {
    return;
  }
  if (!confirm_held) {
    g_menu_confirm_wait_release = false;  // released -> a fresh press may confirm
    return;
  }
  // Still held: strip the mapped confirm action from every context this frame.
  for (uint32_t i = 0; i < kDoaxMenuActionContexts; ++i) {
    const uint32_t addr = kDoaxMenuActionWordAddr + kDoaxMenuActionStride * i;
    const uint32_t v = REX_LOAD_U32(addr);
    if (v & kDoaxMenuConfirmActionBits) {
      REX_STORE_U32(addr, v & ~kDoaxMenuConfirmActionBits);
    }
  }
}

void ArmSchedulerDispatchAfterMenuInit(uint8_t* base, uint32_t caller_lr) {
  (void)base;
  (void)caller_lr;
  return;
}

void ArmPressStartDismissKick(uint8_t* base, const char* site) {
  if (kDoaxDiagTimelineStanddown) {
    return;  // diag: no scheduler flag2 kicks
  }
  if (!g_press_start_overlay_dismiss ||
      g_press_start_menu_kicks >= kPressStartMenuKickLimit) {
    return;
  }
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  if (before.flag0 == 0 || before.flag2 != 0) {
    return;
  }
  const bool press_start_dismiss_mode =
      (before.flag4 == 2 && before.flag5 == 2) ||
      (before.flag4 == 3 && before.flag5 == 3) ||
      (before.flag4 == 2 && before.flag5 == 0);
  if (!press_start_dismiss_mode) {
    return;
  }

  REX_STORE_U8(kDoaxSchedulerFlagAddr + 2, 1);
  ++g_press_start_menu_kicks;
  if (g_menu_kick_logs < kSmallLogCap) {
    ++g_menu_kick_logs;
    const SchedulerSnapshot after = ReadSchedulerSnapshot(base);
    REXKRNL_WARN(
        "DOAX press-start-dismiss-kick site={} armed flag2 kick={}/{} mode={}/{} -> {}/{} "
        "boot_present={} overlay={} present={} dea={} deb={} def={} item={}",
        site, g_press_start_menu_kicks, kPressStartMenuKickLimit, before.flag4, before.flag5,
        after.flag4, after.flag5, REX_LOAD_U8(kDoaxBootPresentByteAddr),
        REX_LOAD_U8(kDoaxIslandOverlayFlagAddr), REX_LOAD_U32(kDoaxPresentStateIndexAddr),
        REX_LOAD_U8(kDoaxMenuFiberDeaAddr), REX_LOAD_U8(kDoaxMenuFiberDebAddr),
        REX_LOAD_U8(kDoaxMenuFiberDefAddr), REX_LOAD_U8(kDoaxMenuItemIdByteAddr));
  }
}

void PrimePressStartDismissCompletion(uint8_t* base, const char* site) {
  if (kDoaxDiagTimelineStanddown) {
    return;  // diag: let def evolve naturally so the animation must drive it
  }
  if (!g_press_start_overlay_dismiss || g_press_start_dismiss_unwound ||
      g_press_start_completion_primed) {
    return;
  }
  REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
  REX_STORE_U32(kDoaxMenuTransitionDf4Addr, 0);
  REX_STORE_U8(kDoaxIslandOverlayFlagAddr, 1);
  g_press_start_completion_primed = true;
  LogPressStartDismiss(base, site, "snap-fade-complete");
}

void MarkPressStartDismissUnwound(uint8_t* base, const char* site) {
  if (!g_press_start_overlay_dismiss || g_press_start_dismiss_unwound) {
    return;
  }
  g_press_start_gate_active = false;
  g_press_start_user_confirm = false;
  g_press_start_waiting_for_release = (g_last_buttons & kPressStartButtons) != 0;
  g_press_start_dismiss_unwound = true;
  LogPressStartDismiss(base, site, "dismiss-unwound");
  LogRealMenuPumpState(base, site, "dismiss-unwound", true);
}

void RestorePostPressStartMenuState(uint8_t* base, const char* site, const char* detail) {
  if (kDoaxDiagTimelineStanddown) {
    LogPressStartDismiss(base, site, detail);  // diag: log only, write nothing
    return;
  }
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 2, 0);
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 3, 0);
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 4, 5);
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 5, 1);
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 6, 0);
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 7, 0);
  // Nonzero frame counters keep DOAX_SchedulerDrainDispatch from re-running
  // mode 5's start callback, which recreates the Press Start fade.
  REX_STORE_U32(kDoaxSchedulerFlagAddr + 8, 1);
  REX_STORE_U32(kDoaxSchedulerFlagAddr + 12, 1);
  REX_STORE_U8(kDoaxIslandOverlayFlagAddr, 1);
  REX_STORE_U8(kDoaxMenuFiberActiveAddr, 1);
  // Park only after the guest's cleanup handoff has run. dea > 4 takes the
  // menu-fiber default tail; with deb nonzero it yields instead of returning or
  // cycling case 0 to the next table entry.
  REX_STORE_U8(kDoaxMenuFiberDeaAddr, 5);
  REX_STORE_U8(kDoaxMenuFiberDebAddr, 1);
  REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
  REX_STORE_U32(kDoaxMenuTransitionDf0Addr, 1);
  REX_STORE_U32(kDoaxMenuTransitionDf4Addr, 0);
  LogPressStartDismiss(base, site, detail);
  LogRealMenuPumpState(base, site, detail, true);
}

void RestoreMenuWorkFiberLoopRegisters(PPCRegister& r14, PPCRegister& r15, PPCRegister& r16,
                                       PPCRegister& r17, PPCRegister& r18, PPCRegister& r19,
                                       PPCRegister& r20, PPCRegister& r21, PPCRegister& r22,
                                       PPCRegister& r23, PPCRegister& r24, PPCRegister& r25,
                                       PPCRegister& r26, PPCRegister& r27, PPCRegister& r28,
                                       PPCRegister& r31) {
  // Matches the original DOAX_MenuWorkFiberLoop prologue before loc_824C15D0.
  r14.s64 = -2093219840;
  r15.s64 = -2086928384 + -16144;
  r16.s64 = -2101477376 + -12460;
  r17.s64 = -2093219840;
  r18.s64 = -2093285376;
  r19.s64 = kDoaxWorkQueueSegBase;
  r20.s64 = kDoaxWorkQueueSegBase;
  r21.s64 = kDoaxWorkQueueTableBase;
  r22.s64 = -2099052544 + -8944;
  r23.s64 = -2093219840 + -31460;
  r24.s64 = 1;
  r25.s64 = -2087190528;
  r26.s64 = -2093285376;
  r27.s64 = -2101149696 + -28404;
  r28.s64 = -2087190528;
  r31.s64 = -2093219840 + -29208;
}

void EnterPostPressStartMenuState(uint8_t* base, const char* site, const char* detail) {
  if (!g_press_start_overlay_dismiss) {
    return;
  }
  g_press_start_gate_active = false;
  g_press_start_user_confirm = false;
  g_press_start_waiting_for_release = (g_last_buttons & kPressStartButtons) != 0;
  g_press_start_dismiss_unwound = true;
  RestorePostPressStartMenuState(base, site, detail);
}

void ArmPressStartGate(uint8_t* base, const char* site) {
  g_press_start_gate_active = true;
  g_press_start_user_confirm = false;
  g_press_start_input_sampled = false;
  g_press_start_waiting_for_release = (g_last_buttons & kPressStartButtons) != 0;
  g_press_start_overlay_dismiss = false;
  g_press_start_dismiss_unwound = false;
  g_press_start_completion_primed = false;
  g_press_start_label34_exit_forced = false;
  g_press_start_cleanup_handoff_pending = false;
  g_press_start_menu_kicks = 0;
  g_last_real_menu_sig0 = 0xFFFFFFFFu;
  g_last_real_menu_sig1 = 0xFFFFFFFFu;
  LogPressStartGate(base, site, "armed");
}

void NoteControllerButtons(uint8_t* base, uint32_t buttons, uint32_t user, uint32_t caller_lr) {
  const uint32_t previous = g_last_buttons;
  const bool previous_confirm_down = (previous & kPressStartButtons) != 0;
  const bool confirm_down = (buttons & kPressStartButtons) != 0;
  g_last_buttons = buttons;

  if (g_press_start_dismiss_unwound) {
    if (g_press_start_waiting_for_release) {
      if (!confirm_down) {
        g_press_start_waiting_for_release = false;
        LogPressStartGate(base, "input", "post-ready-released");
      }
      return;
    }
    if (confirm_down && !previous_confirm_down && !g_press_start_user_confirm) {
      g_press_start_user_confirm = true;
      LogPressStartGate(base, "input", "post-ready-confirm");
    }
  }

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
    g_press_start_overlay_dismiss = true;
    // Prime before DOAX_MenuWorkFiberLoop tests def at LABEL_34; doing this
    // inside DOAX_MenuSceneTransition is too late and queues the island load.
    PrimePressStartDismissCompletion(base, "input-fresh-confirm");
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
  g_dismiss_logs = 0;
  g_menu_kick_logs = 0;
  g_menu_probe_logs = 0;
  g_real_menu_logs = 0;
  g_last_real_menu_sig0 = 0xFFFFFFFFu;
  g_last_real_menu_sig1 = 0xFFFFFFFFu;
  g_timeline_logs = 0;
  g_last_timeline_sig = 0xFFFFFFFFu;
  g_diag_accel_active = false;
  g_diag_accel_synth = 0;
  g_menu_confirm_wait_release = false;
  g_menu_active_prev = false;
  g_sched_probe_logs = 0;
  g_sched_probe_tick = 0;
  g_last_sched_sig = 0xFFFFFFFFu;
  g_press_start_menu_kicks = 0;
  g_promotion_video_plays = 0;
  g_last_buttons = 0;
  g_doax_hook_guest_base = nullptr;
  g_press_start_gate_active = false;
  g_press_start_waiting_for_release = false;
  g_press_start_user_confirm = false;
  g_press_start_input_sampled = false;
  g_press_start_overlay_dismiss = false;
  g_press_start_dismiss_unwound = false;
  g_press_start_completion_primed = false;
  g_press_start_label34_exit_forced = false;
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
  if (kDoaxDiagSchedProbe) {
    LogSchedulerAdvance(base, "drain");
  }
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

extern "C" REX_FUNC(DOAX_MainMenuRegisterSlots) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  g_doax_hook_guest_base = base;
  LogMainMenuProbe(base, "CEF0-enter", caller_lr);
  __imp__DOAX_MainMenuRegisterSlots(ctx, base);
  ApplyMenuConfirmReleaseGate(base);
  ArmSchedulerDispatchAfterMenuInit(base, caller_lr);
  LogMainMenuProbe(base, "CEF0-return", caller_lr);
}

extern "C" REX_FUNC(sub_8266E618) {
  g_doax_hook_guest_base = base;
  LogRealMenuPumpState(base, "E618-enter", "guest-call");
  __imp__sub_8266E618(ctx, base);
  LogRealMenuPumpState(base, "E618-return", "guest-return");
}

extern "C" REX_FUNC(sub_82670E10) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t slot = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t item = static_cast<uint32_t>(ctx.r4.u64);
  const uint32_t arg3 = static_cast<uint32_t>(ctx.r5.u64);
  g_doax_hook_guest_base = base;
  if (g_real_menu_logs < kSmallLogCap &&
      (g_press_start_gate_active || g_press_start_overlay_dismiss || g_press_start_dismiss_unwound)) {
    ++g_real_menu_logs;
    REXKRNL_WARN("DOAX real-menu-activate site=70E10-enter slot={} item={} arg3={} lr=0x{:08X}",
                 slot, item, arg3, caller_lr);
  }
  __imp__sub_82670E10(ctx, base);
  LogRealMenuPumpState(base, "70E10-return", "activation-return", true);
}

extern "C" REX_FUNC(sub_8258E000) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  g_doax_hook_guest_base = base;
  LogMainMenuProbe(base, "E000-enter", caller_lr);
  __imp__sub_8258E000(ctx, base);
  LogMainMenuProbe(base, "E000-return", caller_lr);
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
  MarkPressStartDismissUnwound(base, "menu-fiber-exit");
}

extern "C" REX_FUNC(DOAX_MenuTransitionPlayMovie) {
  g_doax_hook_guest_base = base;
  LogSchedulerSnapshot(base, "transition-play-movie-enter");
  __imp__DOAX_MenuTransitionPlayMovie(ctx, base);
  LogSchedulerSnapshot(base, "transition-play-movie-exit");
}

extern "C" REX_FUNC(DOAX_MenuTransitionOverlaySetup) {
  g_doax_hook_guest_base = base;
  if (g_press_start_overlay_dismiss && !g_press_start_dismiss_unwound) {
    LogPressStartDismiss(base, "overlay-setup", "guest-call");
  }
  __imp__DOAX_MenuTransitionOverlaySetup(ctx, base);
}

// TEMP_DIAG: FM2 event dispatcher (sub_82768270) — the site that signals the
// animation-tween completion events. In a black run the dispatcher's signal
// rate collapses (44x good -> 2x bad); this logs every CALL with the dispatcher
// object, raw event args, and the live subscriber-ring count, so the good-vs-bad
// diff shows CALLED-but-no-subscriber (registration race) vs NOT-CALLED
// (upstream event source stalled). See [[doax-menu-fiber-root-cause]].
extern "C" REX_FUNC(DOAX_FM2EventDispatch) {
  g_doax_hook_guest_base = base;
  if (g_fm2_dispatch_logs < kFM2DispatchLogCap) {
    ++g_fm2_dispatch_logs;
    const uint32_t obj = static_cast<uint32_t>(ctx.r3.u64);
    const uint64_t a2 = ctx.r4.u64;
    const uint64_t a3 = ctx.r5.u64;
    // Subscriber ring per sub_82768270: gate *(obj+16), sentinel obj+100,
    // first *(obj+104), next *(node+4). Count only (bounded), read-only.
    uint32_t subs = 0;
    if (obj && REX_LOAD_U32(obj + 16) != 0) {
      const uint32_t sentinel = obj + 100;
      for (uint32_t n = REX_LOAD_U32(obj + 104); n != sentinel && subs < 256;
           n = REX_LOAD_U32(n + 4)) {
        ++subs;
      }
    }
    REXKRNL_WARN("DOAX fm2-dispatch obj={:08X} subs={} a2={:016X} a3={:016X} n={}", obj, subs, a2,
                 a3, g_fm2_dispatch_logs);
  }
  __imp__DOAX_FM2EventDispatch(ctx, base);
}

extern "C" REX_FUNC(DOAX_IslandSceneLoad) {
  g_doax_hook_guest_base = base;
  if (g_press_start_overlay_dismiss && !g_press_start_dismiss_unwound) {
    LogPressStartDismiss(base, "island-scene-load", "guest-call");
  }
  __imp__DOAX_IslandSceneLoad(ctx, base);
}

extern "C" REX_FUNC(DOAX_MenuTransitionMoviePoll) {
  g_doax_hook_guest_base = base;
  if (g_press_start_dismiss_unwound && !g_press_start_user_confirm) {
    LogPressStartDismiss(base, "moviepoll-after-unwind", "guest-call");
    LogRealMenuPumpState(base, "moviepoll-after-unwind", "guest-call", true);
  }
  __imp__DOAX_MenuTransitionMoviePoll(ctx, base);
  if (g_press_start_dismiss_unwound && !g_press_start_user_confirm) {
    RestorePostPressStartMenuState(base, "moviepoll-after-unwind", "post-ready-restored");
    LogRealMenuPumpState(base, "moviepoll-after-unwind", "guest-return", true);
  }
  if (g_movie_logs < kSmallLogCap) {
    ++g_movie_logs;
    REXKRNL_WARN("DOAX movie-probe site=MoviePoll result={} plays={} handler_done={}",
                 static_cast<uint32_t>(ctx.r3.u64), g_promotion_video_plays,
                 REX_LOAD_U8(static_cast<uint32_t>(ctx.r31.u64) + 2));
  }
}

extern "C" REX_FUNC(DOAX_MenuTransitionFadeAlpha) {
  g_doax_hook_guest_base = base;
  __imp__DOAX_MenuTransitionFadeAlpha(ctx, base);
  PrimePressStartDismissCompletion(base, "fade-alpha");
}

extern "C" REX_FUNC(DOAX_MenuTransitionTimeline) {
  g_doax_hook_guest_base = base;
  // Capture args before the impl runs (it may clobber r3/r4).
  const uint32_t a1 = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t a2 = static_cast<uint32_t>(ctx.r4.u64);
  __imp__DOAX_MenuTransitionTimeline(ctx, base);
  const int32_t real_ret = static_cast<int32_t>(ctx.r3.u64);
  LogTimelineSlot(base, "timeline-poll", a1, a2, real_ret);
  if (kDoaxDiagTimelineStanddown) {
    if (!kDoaxDiagTimelineAccel) {
      return;  // standdown only: observe the natural timeline, no forcing
    }
    // Countdown accelerator (diagnostic): collapse the ~27s transition into ~0.5s
    // by overriding the guest's perceived timeline value. While the transition is
    // running (def==0), race it down to exactly 30 so the guest's `v14==30` check
    // sets def=1; once def is set, present 0 so case 3 falls through to LABEL_34
    // and the menu fiber returns to its idle/menu state. This tests whether
    // COMPLETING the transition actually brings up the next screen.
    if (!g_press_start_overlay_dismiss) {
      return;  // only the user-triggered Press Start -> Travel transition
    }
    const uint8_t def = REX_LOAD_U8(kDoaxMenuFiberDefAddr);
    int32_t presented = real_ret;
    if (def == 0 && real_ret > 30) {
      if (!g_diag_accel_active) {
        g_diag_accel_active = true;
        g_diag_accel_synth = real_ret;
      }
      int32_t synth = g_diag_accel_synth - static_cast<int32_t>(kDiagAccelStep);
      if (synth <= 30) {
        synth = 30;  // land exactly on 30 so v14==30 fires
      }
      g_diag_accel_synth = synth;
      presented = synth;
    } else if (def != 0 && real_ret > 0) {
      presented = 0;  // def set: collapse the tail so case 3 reaches LABEL_34
      g_diag_accel_active = false;
    }
    if (presented != real_ret) {
      ctx.r3.u64 = static_cast<uint32_t>(presented);
      if (g_timeline_logs < kTimelineLogCap) {
        ++g_timeline_logs;
        REXKRNL_WARN("DOAX timeline-accel real={} -> presented={} def={}", real_ret,
                     presented, def);
      }
    }
    return;
  }
  if (g_press_start_overlay_dismiss && !g_press_start_dismiss_unwound) {
    PrimePressStartDismissCompletion(base, "timeline");
    ctx.r3.u64 = 0;
  }
}

extern "C" REX_FUNC(DOAX_PostPromotionCleanup) {
  g_doax_hook_guest_base = base;
  if (g_press_start_overlay_dismiss || g_press_start_dismiss_unwound) {
    LogPressStartDismiss(base, "promotion-cleanup", "skip-reentry");
    return;
  }
  REXKRNL_WARN("DOAX promotion-cleanup: plays={} boot_present={} overlay={}",
               g_promotion_video_plays, REX_LOAD_U8(kDoaxBootPresentByteAddr),
               REX_LOAD_U8(kDoaxIslandOverlayFlagAddr));
  __imp__DOAX_PostPromotionCleanup(ctx, base);
  if (g_press_start_overlay_dismiss || g_press_start_dismiss_unwound) {
    LogPressStartDismiss(base, "promotion-cleanup-return", "skip-rearm");
    return;
  }
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
  if (g_press_start_gate_active || g_press_start_overlay_dismiss || g_press_start_dismiss_unwound) {
    LogRealMenuPumpState(base, "menu-confirm-return", "guest-return", true);
  }
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
  const bool accepted_press_start_label34 =
      g_press_start_gate_active && press_start_scene && g_press_start_user_confirm &&
      caller_lr == kMenuSceneTransitionLabel34Lr;
  if (accepted_press_start_label34) {
    g_press_start_overlay_dismiss = true;
    PrimePressStartDismissCompletion(base, "label34-accepted");
    g_press_start_gate_active = false;
    REXKRNL_WARN("DOAX scene-transition label=LABEL_34 scene_id={} lr=0x{:08X} guest-call=1",
                 scene_id, caller_lr);
    LogPressStartGate(base, "scene-transition", "accepted-user");
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
  if (kDoaxDiagTimelineStanddown) {
    return false;  // diag: block nothing
  }
  uint8_t* base = g_doax_hook_guest_base;
  if (!base || !g_press_start_dismiss_unwound || g_press_start_user_confirm) {
    return false;
  }
  LogPressStartDismiss(base, "timeout-prelude-standdown",
                       "blocked post-ready timeout prelude");
  LogRealMenuPumpState(base, "timeout-prelude-standdown",
                       "blocked post-ready timeout prelude", true);
  return true;
}

void DOAX_ForcePressStartLabel34Exit() {
  if (kDoaxDiagTimelineStanddown) {
    return;  // diag: do not force deb=0 exit at 0x824C1908
  }
  uint8_t* base = g_doax_hook_guest_base;
  if (!base || !g_press_start_overlay_dismiss || g_press_start_dismiss_unwound ||
      g_press_start_label34_exit_forced) {
    return;
  }
  g_press_start_label34_exit_forced = true;
  g_press_start_gate_active = false;
  g_press_start_user_confirm = false;
  g_press_start_waiting_for_release = (g_last_buttons & kPressStartButtons) != 0;
  g_press_start_dismiss_unwound = true;
  g_press_start_cleanup_handoff_pending = true;
  LogPressStartDismiss(base, "label34-exit", "allow-cleanup-handoff");
  LogRealMenuPumpState(base, "label34-exit", "allow-cleanup-handoff", true);
}

void DOAX_KeepPressStartCleanupActive() {
  if (kDoaxDiagTimelineStanddown) {
    return;  // diag: no pre-yield scheduler restore
  }
  uint8_t* base = g_doax_hook_guest_base;
  if (!base || !g_press_start_cleanup_handoff_pending) {
    return;
  }
  RestorePostPressStartMenuState(base, "cleanup-pre-cdf8", "pre-yield-restore");
}

bool DOAX_ParkPressStartMenuFiberAfterCleanup(PPCRegister& r14, PPCRegister& r15,
                                              PPCRegister& r16, PPCRegister& r17,
                                              PPCRegister& r18, PPCRegister& r19,
                                              PPCRegister& r20, PPCRegister& r21,
                                              PPCRegister& r22, PPCRegister& r23,
                                              PPCRegister& r24, PPCRegister& r25,
                                              PPCRegister& r26, PPCRegister& r27,
                                              PPCRegister& r28, PPCRegister& r31) {
  if (kDoaxDiagTimelineStanddown) {
    return false;  // diag: do not park; let the worker run its natural epilogue
  }
  uint8_t* base = g_doax_hook_guest_base;
  if (!base || !g_press_start_cleanup_handoff_pending) {
    return false;
  }
  g_press_start_cleanup_handoff_pending = false;
  RestorePostPressStartMenuState(base, "cleanup-handoff", "park-after-cdf8");
  RestoreMenuWorkFiberLoopRegisters(r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24,
                                    r25, r26, r27, r28, r31);
  return true;
}

bool DOAX_SkipPressStartAutoSceneTransition() {
  return false;
}
