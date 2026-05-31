// FH1 world-load worker guards for sub_8255AE10 / sub_823ED888 / sub_82480C28.
//
// Loading worker crashes when the 712-byte track-loader object pointer in r31 is
// null or clobbered by callees that do not preserve the PPC non-volatile reg.
// Object global: lis(-31954), -4060(r31).

#include "generated/fh1_init.h"

#include "fh1_guest_pc_fiber.h"
#include "fh1_loader_epoch.h"
#include "fh1_track_loader_alloc.h"

#include <atomic>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <windows.h>
#endif

#include <rex/logging.h>
#include <rex/ppc/static_recomp_fiber.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>
#include <rex/system/xthread.h>

namespace {

// Matches generated lis r31,-31954 / stw r3,-4060(r31) in sub_8255AE10.
constexpr uint32_t kTrackLoaderObjectSlot =
    static_cast<uint32_t>(static_cast<int32_t>(-2094137344) - 4060);

thread_local uint32_t g_track_loader_object = 0;
thread_local bool g_track_loader_worker_ok = false;

bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size) {
  if (size == 0 || guest_address == 0) {
    return false;
  }
  if (!memory->IsGuestVirtualCommitted(guest_address)) {
    return false;
  }
  const uint32_t end = guest_address + size - 1;
  if (end < guest_address) {
    return false;
  }
  return memory->IsGuestVirtualCommitted(end);
}

uint32_t LoadTrackLoaderObject(rex::memory::Memory* memory) {
  if (!GuestRangeReadable(memory, kTrackLoaderObjectSlot, 4)) {
    return 0;
  }
  uint32_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(kTrackLoaderObjectSlot), sizeof(be));
  return __builtin_bswap32(be);
}

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address) {
  if (guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return 0;
  }
  uint32_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(guest_address), sizeof(be));
  return __builtin_bswap32(be);
}

uint64_t LoadGuestU64(rex::memory::Memory* memory, uint32_t guest_address) {
  if (guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return 0;
  }
  uint64_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(guest_address), sizeof(be));
  return __builtin_bswap64(be);
}

void StoreGuestU32(rex::memory::Memory* memory, uint32_t guest_address, uint32_t value) {
  if (!GuestRangeReadable(memory, guest_address, 4)) {
    return;
  }
  const uint32_t be = __builtin_bswap32(value);
  std::memcpy(memory->TranslateVirtual(guest_address), &be, sizeof(be));
}

bool IsKnownPoisonGuestPointer(uint32_t addr) {
  switch (addr) {
    case 0:
    case 0x00BEBEBEu:
    case 0xBEBEBEBEu:
    case 0xCDCDCDCDu:
    case 0xDDDDDDDDu:
    case 0xFEEEFEEEu:
      return true;
    default:
      return false;
  }
}

bool IsOnThreadGuestStack(uint32_t addr) {
  if (addr == 0) {
    return false;
  }
  auto* thread = rex::system::XThread::TryGetCurrentThread();
  if (!thread) {
    return false;
  }
  const uint32_t lo = thread->stack_limit();
  const uint32_t hi = thread->stack_base();
  if (lo == 0 || hi == 0 || hi < lo) {
    return false;
  }
  if (rex::ppc::IsWorkQueueFiberGuestStack(lo)) {
    return false;
  }
  return addr >= lo && addr <= hi;
}

bool IsRegisteredGuestCode(uint8_t* base, uint32_t addr) {
  if (addr == 0) {
    return false;
  }
  if (addr < REX_CODE_BASE ||
      (addr - REX_CODE_BASE) >= REX_CODE_SIZE + REX_THUNK_RESERVE_SIZE) {
    return false;
  }
  return REX_LOOKUP_FUNC(base, addr) != nullptr;
}

bool IsPlausibleGuestCode(uint32_t addr) {
  if (addr == 0) {
    return false;
  }
  return addr >= REX_CODE_BASE &&
         (addr - REX_CODE_BASE) < REX_CODE_SIZE + REX_THUNK_RESERVE_SIZE;
}

bool IsPlausibleGuestImage(uint32_t addr) {
  if (addr == 0) {
    return false;
  }
  return addr >= REX_IMAGE_BASE &&
         (addr - REX_IMAGE_BASE) < REX_IMAGE_SIZE;
}

bool IsPlausibleGuestVtable(uint32_t addr) {
  return IsPlausibleGuestCode(addr) || IsPlausibleGuestImage(addr);
}

uint32_t LoadGuestVtableSlotU32(rex::memory::Memory* memory, uint32_t vtable,
                                uint32_t offset) {
  if (!IsPlausibleGuestVtable(vtable)) {
    return 0;
  }
  return LoadGuestU32(memory, vtable + offset);
}

enum class JobPrepDispatchState {
  Ready,
  /// vtable method at +12 is null — job not ready; leave obj+16 intact.
  PendingMethod,
  Invalid,
};

JobPrepDispatchState ClassifyJobPrepNested(rex::memory::Memory* memory, uint8_t* base,
                                           uint32_t nested) {
  if (IsKnownPoisonGuestPointer(nested)) {
    return JobPrepDispatchState::Invalid;
  }
  if (IsOnThreadGuestStack(nested)) {
    return JobPrepDispatchState::Invalid;
  }
  if (!memory->IsGuestVirtualCommitted(nested)) {
    return JobPrepDispatchState::Invalid;
  }
  const uint32_t vtable = LoadGuestU32(memory, nested + 0);
  if (IsKnownPoisonGuestPointer(vtable) || IsOnThreadGuestStack(vtable)) {
    return JobPrepDispatchState::Invalid;
  }
  if (!IsPlausibleGuestVtable(vtable)) {
    return JobPrepDispatchState::Invalid;
  }
  const uint32_t method = LoadGuestVtableSlotU32(memory, vtable, 12);
  if (method == 0) {
    return JobPrepDispatchState::PendingMethod;
  }
  if (IsRegisteredGuestCode(base, method) || IsPlausibleGuestCode(method)) {
    return JobPrepDispatchState::Ready;
  }
  return JobPrepDispatchState::Invalid;
}

bool CanDispatchJobPrepNested(rex::memory::Memory* memory, uint8_t* base,
                              uint32_t nested) {
  return ClassifyJobPrepNested(memory, base, nested) ==
         JobPrepDispatchState::Ready;
}

bool IsPlausibleJobPrepTarget(rex::memory::Memory* memory, uint32_t obj) {
  if (IsKnownPoisonGuestPointer(obj)) {
    return false;
  }
  if (!GuestRangeReadable(memory, obj, 20)) {
    return false;
  }
  if (IsOnThreadGuestStack(obj)) {
    return false;
  }
  return true;
}

// sub_82A83480 reentrancy globals (lis -31951 / lwz 26868,26872).
constexpr uint32_t kCriticalSectionGlobalBase = 0x832F0000u;
constexpr uint32_t kCriticalSectionActive = 26868;
constexpr uint32_t kCriticalSectionEnabled = 26872;

void ClearStuckCriticalSectionLock(rex::memory::Memory* memory) {
  const uint32_t enabled =
      LoadGuestU32(memory, kCriticalSectionGlobalBase + kCriticalSectionEnabled);
  const uint32_t active =
      LoadGuestU32(memory, kCriticalSectionGlobalBase + kCriticalSectionActive);
  if (enabled == 1 && active == 1) {
    static std::atomic<uint32_t> log_count{0};
    if (log_count.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 sub_82A83480: clearing stuck reentrancy lock (would KeBugCheck(0))");
    }
    StoreGuestU32(memory, kCriticalSectionGlobalBase + kCriticalSectionActive, 0);
  }
}

bool CanDispatchNestedVirtual(rex::memory::Memory* memory, uint32_t owner,
                              uint32_t member_offset) {
  if (owner == 0 || !GuestRangeReadable(memory, owner, 3116)) {
    return false;
  }
  const uint32_t node = LoadGuestU32(memory, owner + 3112);
  if (node == 0 || !GuestRangeReadable(memory, node, 12)) {
    return false;
  }
  const uint32_t target = LoadGuestU32(memory, node + 8);
  if (target == 0 || !GuestRangeReadable(memory, target, member_offset + 4)) {
    return false;
  }
  return LoadGuestU32(memory, target + member_offset) != 0;
}

bool IsTrackLoaderObject(rex::memory::Memory* memory, uint32_t object,
                         uint32_t min_size = 712) {
  return GuestRangeReadable(memory, object, min_size);
}

bool IsPlausibleTrackLoaderSubObject(rex::memory::Memory* memory, uint32_t addr) {
  if (IsKnownPoisonGuestPointer(addr)) {
    return false;
  }
  // Reject values that wrap on adjustor addi +4 (82519B10) into host AV range.
  if (addr >= 0xFFFFFFF0u) {
    return false;
  }
  if (IsOnThreadGuestStack(addr)) {
    return false;
  }
  return memory->IsGuestVirtualCommitted(addr);
}

bool IsTrackLoaderReadyForWorker(rex::memory::Memory* memory, uint32_t object) {
  if (!IsTrackLoaderObject(memory, object)) {
    return false;
  }
  if (!GuestRangeReadable(memory, object + 600, 4)) {
    return false;
  }
  const uint32_t vtable_slot = LoadGuestU32(memory, object + 0);
  if (!IsPlausibleGuestImage(vtable_slot)) {
    return false;
  }
  const uint32_t sub_object = LoadGuestU32(memory, object + 4);
  if (sub_object == 0) {
    return true;
  }
  if (!IsPlausibleTrackLoaderSubObject(memory, sub_object)) {
    return false;
  }
  const uint32_t adjusted = sub_object + 4u;
  if (adjusted < sub_object || !GuestRangeReadable(memory, adjusted, 4)) {
    return false;
  }
  return true;
}

void StashTrackLoaderObject(uint32_t object) {
  if (object != 0) {
    g_track_loader_object = object;
  }
}

uint32_t ResolveTrackLoaderObject(PPCContext& ctx) {
  auto* memory = rex::Runtime::instance()->memory();
  if (g_track_loader_object != 0 &&
      IsTrackLoaderObject(memory, g_track_loader_object)) {
    return g_track_loader_object;
  }
  if (IsTrackLoaderObject(memory, ctx.r3.u32)) {
    return ctx.r3.u32;
  }
  if (IsTrackLoaderObject(memory, ctx.r31.u32)) {
    return ctx.r31.u32;
  }
  const uint32_t global_object = LoadTrackLoaderObject(memory);
  if (IsTrackLoaderObject(memory, global_object)) {
    return global_object;
  }
  return 0;
}

void RestoreTrackLoaderRegs(PPCContext& ctx, uint32_t object) {
  StashTrackLoaderObject(object);
  ctx.r3.u64 = object;
  if (!fh1::fiber::ShouldPreserveGuestR31(ctx)) {
    ctx.r31.u64 = object;
  }
}

void FinishSub8255AE10(PPCContext& ctx, uint32_t entry_r1, uint32_t entry_lr,
                       uint64_t entry_r31) {
  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t cur_sp = static_cast<uint32_t>(ctx.r1.u64);
  uint32_t parent_r1 = entry_r1;

  if (memory != nullptr) {
    if (IsOnThreadGuestStack(cur_sp) && GuestRangeReadable(memory, cur_sp, 96)) {
      parent_r1 = cur_sp + 96;
    } else {
      const uint32_t frame_sp = entry_r1 - 96;
      if (GuestRangeReadable(memory, frame_sp, 4)) {
        const uint32_t back_link = LoadGuestU32(memory, frame_sp);
        if (IsOnThreadGuestStack(back_link)) {
          parent_r1 = back_link;
        }
      }
    }

    if (IsOnThreadGuestStack(parent_r1) && parent_r1 >= 16 &&
        GuestRangeReadable(memory, parent_r1 - 16, 16)) {
      ctx.r12.u64 = LoadGuestU32(memory, parent_r1 - 8);
      ctx.lr = ctx.r12.u64;
      ctx.r31.u64 = LoadGuestU64(memory, parent_r1 - 16);
    } else {
      static std::atomic<bool> logged{false};
      if (!logged.exchange(true)) {
        REXSYS_WARN(
            "FH1 sub_8255AE10: epilogue stack unreadable (sp=0x{:08X} "
            "entry_r1=0x{:08X}); restoring saved lr/r31",
            cur_sp, entry_r1);
      }
      ctx.lr = entry_lr;
      ctx.r31.u64 = entry_r31;
    }
  } else {
    ctx.lr = entry_lr;
    ctx.r31.u64 = entry_r31;
  }

  ctx.r1.u64 = entry_r1;
  ctx.r3.u64 = 0;
  fh1::fiber::ReconcileFiberFrameStack(ctx);
}

void RepairTrackLoaderR31(PPCContext& ctx) {
  const uint32_t object = ResolveTrackLoaderObject(ctx);
  if (object != 0 && !fh1::fiber::ShouldPreserveGuestR31(ctx)) {
    ctx.r31.u64 = object;
  }
}

#define FH1_IMP_WRAPPER(name, guest_pc) \
  extern "C" REX_FUNC(name) {             \
    REX_FUNC_PROLOGUE();                  \
    fh1_load_gate_dispatch_wait(guest_pc); \
    __imp__##name(ctx, base);             \
    RepairTrackLoaderR31(ctx);            \
  }

extern "C" REX_FUNC(sub_8247D7A8) {
  REX_FUNC_PROLOGUE();
  fh1_load_gate_dispatch_wait(0x8247D7A8u);

  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t obj = ctx.r3.u32;
  if (!IsPlausibleJobPrepTarget(memory, obj)) {
    static std::atomic<uint32_t> skip_log{0};
    if (skip_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 sub_8247D7A8: invalid job prep object (r3=0x{:08X}); skipping",
          obj);
    }
    RepairTrackLoaderR31(ctx);
    return;
  }

  const uint32_t nested = LoadGuestU32(memory, obj + 16);
  if (nested == 0) {
    RepairTrackLoaderR31(ctx);
    return;
  }
  if (IsKnownPoisonGuestPointer(nested) || IsOnThreadGuestStack(nested) ||
      !memory->IsGuestVirtualCommitted(nested)) {
    static std::atomic<uint32_t> nested_log{0};
    if (nested_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 sub_8247D7A8: invalid nested ptr 0x{:08X} at obj+16 (obj=0x{:08X}); "
          "clearing and skipping",
          nested, obj);
    }
    StoreGuestU32(memory, obj + 16, 0);
    RepairTrackLoaderR31(ctx);
    return;
  }

  if (!CanDispatchJobPrepNested(memory, base, nested)) {
    const JobPrepDispatchState state =
        ClassifyJobPrepNested(memory, base, nested);
    if (state == JobPrepDispatchState::PendingMethod) {
      static std::atomic<uint32_t> pending_log{0};
      if (pending_log.fetch_add(1, std::memory_order_relaxed) < 8) {
        const uint32_t vtable = LoadGuestU32(memory, nested + 0);
        REXSYS_WARN(
            "FH1 sub_8247D7A8: nested method pending (obj=0x{:08X} nested=0x{:08X} "
            "vtable=0x{:08X}); leaving obj+16 intact",
            obj, nested, vtable);
      }
      RepairTrackLoaderR31(ctx);
      return;
    }

    static std::atomic<uint32_t> dispatch_log{0};
    if (dispatch_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      const uint32_t vtable = LoadGuestU32(memory, nested + 0);
      const uint32_t method = LoadGuestVtableSlotU32(memory, vtable, 12);
      REXSYS_WARN(
          "FH1 sub_8247D7A8: nested dispatch not safe (obj=0x{:08X} nested=0x{:08X} "
          "vtable=0x{:08X} method=0x{:08X}); clearing and skipping",
          obj, nested, vtable, method);
    }
    StoreGuestU32(memory, obj + 16, 0);
    RepairTrackLoaderR31(ctx);
    return;
  }

#if defined(_MSC_VER)
  __try {
    __imp__sub_8247D7A8(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    static std::atomic<uint32_t> av_log{0};
    if (av_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 sub_8247D7A8: access violation (obj=0x{:08X} nested=0x{:08X}); "
          "skipping job prep",
          obj, nested);
    }
    StoreGuestU32(memory, obj + 16, 0);
  }
#else
  __imp__sub_8247D7A8(ctx, base);
#endif

  RepairTrackLoaderR31(ctx);
}

extern "C" REX_FUNC(sub_82A83480) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  ClearStuckCriticalSectionLock(memory);

#if defined(_MSC_VER)
  __try {
    __imp__sub_82A83480(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    static std::atomic<uint32_t> av_log{0};
    if (av_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 sub_82A83480: access violation (r3={} r4={}); skipping critical section",
          ctx.r3.u32, ctx.r4.u32);
    }
    StoreGuestU32(memory, kCriticalSectionGlobalBase + kCriticalSectionActive, 0);
  }
#else
  __imp__sub_82A83480(ctx, base);
#endif
}

}  // namespace

FH1_IMP_WRAPPER(sub_82C0C668, 0x82C0C668u)
FH1_IMP_WRAPPER(sub_82C0EA68, 0x82C0EA68u)
FH1_IMP_WRAPPER(sub_825D0640, 0x825D0640u)
FH1_IMP_WRAPPER(sub_82553BC0, 0x82553BC0u)
FH1_IMP_WRAPPER(sub_82570C98, 0x82570C98u)

extern "C" REX_FUNC(sub_8255AE10) {
  REX_FUNC_PROLOGUE();
  uint32_t ea{};

  const uint32_t entry_r1 = ctx.r1.u32;
  const uint32_t entry_lr = static_cast<uint32_t>(ctx.lr);
  const uint64_t entry_r31 = ctx.r31.u64;

  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;

  uint32_t allocated = Fh1AllocateTrackLoaderObject(ctx, base);

  {
    if (allocated != 0) {
      fh1_load_gate_enter();
    }
    struct LoadGateLeave {
      bool active = false;
      explicit LoadGateLeave(bool enter) : active(enter) {}
      ~LoadGateLeave() {
        if (active) {
          fh1_load_gate_leave();
        }
      }
    } load_gate_scope(allocated != 0);

    if (allocated != 0) {
      ctx.lr = 0x8255AE28;
      ctx.r3.u64 = allocated;
      sub_824839E8(ctx, base);
    }

    ctx.r31.s64 = -2094137344;
    REX_STORE_U32(ctx.r31.u32 + -4060, allocated);
    if (allocated != 0) {
      StashTrackLoaderObject(allocated);
      // Native stw r3,-4060(r31) then bl 824843E8 with the object in r3.
      ctx.r3.u64 = allocated;
      ctx.lr = 0x8255AE48;
      sub_824843E8(ctx, base);
    }
  }

  if (allocated == 0) {
    static std::atomic<bool> skip_worker_logged{false};
    if (!skip_worker_logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_8255AE10: no track-loader object; skipping publish/worker "
          "(824843E8/823ED888 require non-null r3)");
    }
    g_track_loader_object = 0;
    g_track_loader_worker_ok = false;
    FinishSub8255AE10(ctx, entry_r1, entry_lr, entry_r31);
    return;
  }

  ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + -4060);
  g_track_loader_worker_ok = false;
  RestoreTrackLoaderRegs(ctx, ctx.r3.u32);
  sub_823ED888(ctx, base);

  if (!g_track_loader_worker_ok) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_8255AE10: world-load worker incomplete; skipping teardown "
          "and completion signal (avoids TerminateTitle on failed load)");
    }
  } else {
    ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + -4060);
    ctx.lr = 0x8255AE58;
    sub_824866F0(ctx, base);

    ctx.r11.s64 = -2094137344;
    ctx.r4.s64 = 1;
    ctx.r3.s64 = ctx.r11.s64 + 25604;
    ctx.lr = 0x8255AE68;
    sub_82C00E50(ctx, base);
  }

  g_track_loader_object = 0;
  g_track_loader_worker_ok = false;
  FinishSub8255AE10(ctx, entry_r1, entry_lr, entry_r31);
}

extern "C" REX_FUNC(sub_823ED888) {
  REX_FUNC_PROLOGUE();
  fh1_load_gate_dispatch_wait(0x823ED888u);

  g_track_loader_worker_ok = false;

  const uint32_t object = ResolveTrackLoaderObject(ctx);
  if (object == 0) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_ERROR(
          "FH1 sub_823ED888: track-loader object missing (r3=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
    return;
  }

  RestoreTrackLoaderRegs(ctx, object);

  // No __try/__except: the worker calls sub_82C013E0 → BC88 → ED910 → KeSet longjmp.
  // MSVC guest longjmp cannot cross an outer SEH frame (same rule as BC88/ED910);
  // wrapping here prevented GuestPcFiberResume from running (no 830ED900 rewrite logs).
  auto* memory = rex::Runtime::instance()->memory();
  if (!memory || !GuestRangeReadable(memory, object + 600, 4)) {
    static std::atomic<uint32_t> unreadable_log{0};
    if (unreadable_log.fetch_add(1, std::memory_order_relaxed) < 4) {
      REXSYS_WARN(
          "FH1 sub_823ED888: object+600 not readable (object=0x{:08X}); skipping worker",
          object);
    }
    ctx.r3.u64 = 0;
    return;
  }

  if (!IsTrackLoaderReadyForWorker(memory, object)) {
    static std::atomic<uint32_t> not_ready_log{0};
    if (not_ready_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      const uint32_t slot4 = LoadGuestU32(memory, object + 4);
      REXSYS_WARN(
          "FH1 sub_823ED888: track-loader not ready (object=0x{:08X} "
          "+0=0x{:08X} +4=0x{:08X}); skipping worker",
          object, LoadGuestU32(memory, object + 0), slot4);
    }
    ctx.r3.u64 = 0;
    return;
  }

  __imp__sub_823ED888(ctx, base);

  RepairTrackLoaderR31(ctx);
  g_track_loader_worker_ok = true;
}

extern "C" REX_FUNC(sub_82480C28) {
  REX_FUNC_PROLOGUE();

  const uint32_t object = ResolveTrackLoaderObject(ctx);
  if (object == 0) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_82480C28: invalid object (r3=0x{:08X}); treating flag as unset",
          ctx.r3.u32);
    }
    return;
  }

  RestoreTrackLoaderRegs(ctx, object);

#if defined(_MSC_VER)
  __try {
    __imp__sub_82480C28(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    RestoreTrackLoaderRegs(ctx, object);
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_ERROR(
          "FH1 sub_82480C28: access violation (object=0x{:08X}); skipping",
          object);
    }
  }
#else
  __imp__sub_82480C28(ctx, base);
#endif

  RestoreTrackLoaderRegs(ctx, object);
}

extern "C" REX_FUNC(sub_824866F0) {
  REX_FUNC_PROLOGUE();

  const uint32_t object = ResolveTrackLoaderObject(ctx);
  if (object == 0) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_824866F0: track-loader object missing (r3=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
    return;
  }

  RestoreTrackLoaderRegs(ctx, object);

#if defined(_MSC_VER)
  __try {
    __imp__sub_824866F0(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    RestoreTrackLoaderRegs(ctx, object);
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_ERROR(
          "FH1 sub_824866F0: access violation (object=0x{:08X}); skipping teardown",
          object);
    }
    ctx.r3.u64 = 0;
  }
#else
  __imp__sub_824866F0(ctx, base);
#endif
}

extern "C" REX_FUNC(sub_8257E3A0) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  if (!CanDispatchNestedVirtual(memory, ctx.r3.u32, 240)) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_8257E3A0: nested vtable at +3112 unset (owner=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
    return;
  }

#if defined(_MSC_VER)
  __try {
    __imp__sub_8257E3A0(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_ERROR(
          "FH1 sub_8257E3A0: access violation (owner=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
  }
#else
  __imp__sub_8257E3A0(ctx, base);
#endif
}

extern "C" REX_FUNC(sub_8257E3B8) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  if (!CanDispatchNestedVirtual(memory, ctx.r3.u32, 28)) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_8257E3B8: nested vtable at +3112 unset (owner=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
    return;
  }

#if defined(_MSC_VER)
  __try {
    __imp__sub_8257E3B8(ctx, base);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_ERROR(
          "FH1 sub_8257E3B8: access violation (owner=0x{:08X}); skipping",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
  }
#else
  __imp__sub_8257E3B8(ctx, base);
#endif
}
