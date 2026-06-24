#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

constexpr uint32_t kDoaxSchedulerFlagAddr =
    static_cast<uint32_t>(static_cast<int32_t>(-2093219840 + -29192));

constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;

constexpr uint32_t kSchedulerHeaderBytes = 16;

constexpr uint32_t kDoaxBootPresentByteAddr = 0x833BB763u;
constexpr uint8_t kDoaxMainMenuReadyPresent = 5u;

constexpr uint32_t kDoaxMenuFiberDeaAddr = 0x833B8DEAu;
constexpr uint32_t kDoaxMenuFiberDebAddr = 0x833B8DEBu;
constexpr uint32_t kDoaxMenuFiberDefAddr = 0x833B8DEFu;
constexpr uint32_t kDoaxIslandOverlayFlagAddr = 0x833B8514u;
constexpr uint32_t kDoaxPresentStateIndexAddr = 0x833B84C8u;

constexpr uint32_t kDispatcherFiberYieldLr = 0x824C0600u;
constexpr uint32_t kCdf8FiberYieldLr = 0x8258CE4Cu;
constexpr uint32_t kMenuWorkFiberYieldLr = 0x824C15F4u;

constexpr uint32_t kMenuFiberProbeLogCap = 16;
constexpr uint32_t kMenuFiberYieldLogCap = 48;
constexpr uint32_t kCef0ProbeLogCap = 8;
constexpr uint32_t kDrainMenuProbeLogCap = 32;
constexpr uint32_t kDrainStuckProbeInterval = 200;

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

struct MenuFiberGlobals {
  uint8_t dea = 0;
  uint8_t deb = 0;
  uint8_t def = 0;
  uint8_t overlay = 0;
  uint8_t present = 0;
};

struct MenuFiberProbeState {
  uint32_t enter_count = 0;
  uint32_t log_count = 0;
};

struct MenuFiberYieldProbeState {
  uint32_t log_count = 0;
};

struct Cef0ProbeState {
  uint32_t call_count = 0;
  uint32_t log_count = 0;
};

struct DrainMenuProbeState {
  uint64_t call_count = 0;
  uint32_t log_count = 0;
  uint64_t stuck_calls_since_log = 0;
};

MenuFiberProbeState g_menu_fiber_probe;
MenuFiberYieldProbeState g_menu_fiber_yield_probe;
Cef0ProbeState g_cef0_probe;
DrainMenuProbeState g_drain_menu_probe;

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

MenuFiberGlobals ReadMenuFiberGlobals(uint8_t* base) {
  (void)base;
  MenuFiberGlobals g{};
  g.dea = REX_LOAD_U8(kDoaxMenuFiberDeaAddr);
  g.deb = REX_LOAD_U8(kDoaxMenuFiberDebAddr);
  g.def = REX_LOAD_U8(kDoaxMenuFiberDefAddr);
  g.overlay = REX_LOAD_U8(kDoaxIslandOverlayFlagAddr);
  g.present = REX_LOAD_U8(kDoaxPresentStateIndexAddr);
  return g;
}

void LogSchedulerFlags(const char* prefix, const SchedulerSnapshot& snap) {
  REXKRNL_WARN(
      "{} f0-7={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} w8={} w12={}", prefix,
      snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5, snap.flag6,
      snap.flag7, snap.word8, snap.word12);
}

void LogMenuFiberGlobals(const char* prefix, const MenuFiberGlobals& g) {
  REXKRNL_WARN("{} dea={} deb={} def={} overlay={} present={}", prefix, g.dea, g.deb, g.def,
               g.overlay, g.present);
}

void MaybeLogMenuFiberSite(const char* site, uint8_t* base, const PPCContext& ctx) {
  if (g_menu_fiber_probe.log_count >= kMenuFiberProbeLogCap) {
    return;
  }
  ++g_menu_fiber_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  const MenuFiberGlobals globals = ReadMenuFiberGlobals(base);
  REXKRNL_WARN(
      "DOAX menu-fiber-probe site={} enter={} r28=0x{:08X} r30=0x{:08X} r31=0x{:08X} "
      "boot_present={}",
      site, g_menu_fiber_probe.enter_count, ctx.r28.u32, ctx.r30.u32, ctx.r31.u32,
      REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX menu-fiber-probe sched", sched);
  LogMenuFiberGlobals("DOAX menu-fiber-probe", globals);
}

void MaybeLogMenuFiberYield(uint8_t* base, uint32_t yield_index, uint64_t r28_before,
                            uint64_t r28_after, bool preserved_gprs) {
  if (g_menu_fiber_yield_probe.log_count >= kMenuFiberYieldLogCap) {
    return;
  }
  ++g_menu_fiber_yield_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  const MenuFiberGlobals globals = ReadMenuFiberGlobals(base);
  REXKRNL_WARN(
      "DOAX menu-fiber-yield n={} r28 0x{:08X}->0x{:08X} gpr_preserve={} boot_present={}",
      yield_index, static_cast<uint32_t>(r28_before), static_cast<uint32_t>(r28_after),
      preserved_gprs ? 1 : 0, REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX menu-fiber-yield sched", sched);
  LogMenuFiberGlobals("DOAX menu-fiber-yield", globals);
}

void MaybeLogCef0Probe(uint8_t* base, uint32_t caller_lr, const char* phase) {
  if (g_cef0_probe.log_count >= kCef0ProbeLogCap) {
    return;
  }
  ++g_cef0_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  REXKRNL_WARN("DOAX cef0-probe phase={} call={} lr=0x{:08X} boot_present={}", phase,
               g_cef0_probe.call_count, caller_lr, REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX cef0-probe sched", sched);
  LogMenuFiberGlobals("DOAX cef0-probe", ReadMenuFiberGlobals(base));
}

bool IsStuckMainMenuDrain(const SchedulerSnapshot& snap) {
  return snap.flag0 != 0 && snap.flag2 == 0 && snap.flag4 == 5 && snap.flag5 == 1;
}

void MaybeLogDrainMenuProbe(uint8_t* base, const SchedulerSnapshot& before,
                            const SchedulerSnapshot& after, uint32_t caller_lr,
                            const char* reason) {
  if (g_drain_menu_probe.log_count >= kDrainMenuProbeLogCap) {
    return;
  }
  ++g_drain_menu_probe.log_count;
  REXKRNL_WARN(
      "DOAX drain-menu-probe call={} reason={} caller_lr=0x{:08X} boot_present={}",
      g_drain_menu_probe.call_count, reason, caller_lr,
      REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX drain-menu-probe before", before);
  LogSchedulerFlags("DOAX drain-menu-probe after ", after);
  LogMenuFiberGlobals("DOAX drain-menu-probe", ReadMenuFiberGlobals(base));
}

void ProbeDrainMenuDispatch(uint8_t* base, const SchedulerSnapshot& before,
                            const SchedulerSnapshot& after, uint32_t caller_lr) {
  auto& probe = g_drain_menu_probe;
  ++probe.call_count;

  const bool boot_present =
      REX_LOAD_U8(kDoaxBootPresentByteAddr) == kDoaxMainMenuReadyPresent;
  const bool menu_armed = before.flag2 != 0 || after.flag2 != 0;
  const bool flag2_cleared = before.flag2 != 0 && after.flag2 == 0;
  const bool stuck = boot_present && IsStuckMainMenuDrain(after);

  if (menu_armed) {
    MaybeLogDrainMenuProbe(base, before, after, caller_lr,
                           flag2_cleared ? "flag2_cleared" : "flag2_active");
    probe.stuck_calls_since_log = 0;
    return;
  }

  if (stuck) {
    ++probe.stuck_calls_since_log;
    if (probe.call_count <= 8 || probe.stuck_calls_since_log >= kDrainStuckProbeInterval) {
      probe.stuck_calls_since_log = 0;
      MaybeLogDrainMenuProbe(base, before, after, caller_lr, "stuck_flag2_zero");
    }
  }
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
    reg_fix = true;
    ctx.r31.u64 = saved_r31;
  }
  if (saved_r30 != 0 && ctx.r30.u64 == 0) {
    reg_fix = true;
    ctx.r30.u64 = saved_r30;
  }
  if (reg_fix) {
    WriteSchedulerHeaderBytes(base, sched_before);
  }
}

void RestoreDrainCallerRegs(PPCContext& ctx, uint64_t caller_lr, uint64_t caller_r29,
                            uint64_t caller_r30, uint64_t caller_r31) {
  if (static_cast<uint32_t>(caller_lr) == 0x824C0604) {
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
  const bool is_menu_fiber_yield = caller_lr == kMenuWorkFiberYieldLr;
  const uint64_t r28_before = ctx.r28.u64;
  static uint32_t menu_fiber_yield_index = 0;

  SchedulerFiberGprs saved_gprs{};
  const bool preserve_gprs = NeedsFiberCalleeSavePreserve(caller_lr);
  if (preserve_gprs) {
    SaveSchedulerFiberGprs(ctx, saved_gprs);
  }
  DOAX_FiberContextSwitch(ctx, base);
  if (preserve_gprs) {
    RestoreSchedulerFiberGprs(ctx, saved_gprs);
  }

  if (is_menu_fiber_yield) {
    ++menu_fiber_yield_index;
    MaybeLogMenuFiberYield(base, menu_fiber_yield_index, r28_before, ctx.r28.u64,
                           preserve_gprs);
  }
}

extern "C" REX_FUNC(DOAX_WorkQueueSlotWake) {
  const uint64_t caller_r31 = ctx.r31.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  __imp__DOAX_WorkQueueSlotWake(ctx, base);
  if (caller_r31 != 0) {
    ctx.r31.u64 = caller_r31;
  } else {
    ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  }
  if (caller_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = caller_r30;
  }
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
  if (!ctx.cr6.eq) {
    goto loc_824C0910_hook;
  }
  ctx.r11.s64 = -2101149696;
  ctx.r10.u64 = REX_LOAD_U8(ctx.r31.u32 + 4);
  ctx.r3.u64 = ctx.r31.u64;
  ctx.r11.s64 = ctx.r11.s64 + -28360;
  ctx.r10.u64 = __builtin_rotateleft32(ctx.r10.u32, 4);
  ctx.r11.s64 = ctx.r11.s64 + 12;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r11.u32);
  ctx.ctr.u64 = ctx.r11.u64;
  ctx.lr = 0x824C0900;
  SchedulerGuardedIndirectCall(ctx, base, ctx.ctr.u32, 0x824C0900);
  ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  ctx.r3.s64 = 1;
  ctx.lr = 0x824C0908;
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
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  __imp__DOAX_SchedulerDrainDispatch(ctx, base);
  const SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  ProbeDrainMenuDispatch(base, before, after, static_cast<uint32_t>(caller_lr));
  RestoreDrainCallerRegs(ctx, caller_lr, caller_r29, caller_r30, caller_r31);
}

extern "C" REX_FUNC(DOAX_MainMenuRegisterSlots) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  ++g_cef0_probe.call_count;
  MaybeLogCef0Probe(base, caller_lr, "before");
  __imp__DOAX_MainMenuRegisterSlots(ctx, base);
  MaybeLogCef0Probe(base, caller_lr, "after");
}

extern "C" REX_FUNC(DOAX_MenuWorkFiberLoop) {
  ++g_menu_fiber_probe.enter_count;
  MaybeLogMenuFiberSite("enter", base, ctx);
  __imp__DOAX_MenuWorkFiberLoop(ctx, base);
  MaybeLogMenuFiberSite("exit", base, ctx);
}

extern "C" REX_FUNC(DOAX_WorkQueueDispatchLoop) {
  REX_FUNC_PROLOGUE(DOAX_WorkQueueDispatchLoop);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  ctx.lr = 0x824C05C0;
  __savegprlr_28(ctx, base);
  ea = -128 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.lr = 0x824C05C8;
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
  ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) &
                0xFFFFFFFFFFFFFFFB;
  REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
  ctx.lr = 0x824C0600;
  DOAX_FiberYield(ctx, base);
  ctx.lr = 0x824C0604;
  DOAX_SchedulerDrainDispatch(ctx, base);
  goto loc_824C05DC_hook;
}
