// FH1 guest-PC fiber registration — delegates to rex::ppc guest-PC fiber SDK.

#include "fh1_guest_pc_fiber.h"

#include "fh1_workqueue_shared.h"
#include "generated/fh1_init.h"

#include <atomic>
#include <cstring>

#include <rex/logging.h>
#include <rex/ppc/guest_pc_fiber.h>
#include <rex/runtime.h>

namespace fh1::fiber {
namespace {

constexpr uint32_t kEd910PostKeSetResumeLr = 0x830ED900u;
constexpr uint32_t kFiberPollJobEntry = 0x82C0BEC8u;
constexpr uint32_t kWorldLoadWorkerEntry = 0x823ED888u;
constexpr uint32_t kTrackLoaderObjectSlot = 0x830DF024u;
constexpr uint32_t kTrackLoaderObjectBytes = 712u;
constexpr uint32_t kGuestHeapLo = 0x40000000u;
constexpr uint32_t kGuestHeapHi = 0x50000000u;
thread_local uint32_t g_pending_fiber_job_object = 0;

void FiberSwapImplEd910(PPCContext& ctx, uint8_t* base) {
  __imp__sub_830ED910(ctx, base);
}

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address) {
  if (guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return 0;
  }
  uint32_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(guest_address), sizeof(be));
  return __builtin_bswap32(be);
}

bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size) {
  if (size == 0 || guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return false;
  }
  const uint32_t end = guest_address + size - 1;
  if (end < guest_address) {
    return false;
  }
  return memory->IsGuestVirtualCommitted(end);
}

uint32_t ResolveTlsThreadLocalState(rex::memory::Memory* memory, const PPCContext& ctx) {
  if (!GuestRangeReadable(memory, ctx.r13.u32 + 256, 4)) {
    return 0;
  }
  return LoadGuestU32(memory, ctx.r13.u32 + 256);
}

bool IsPlausibleGuestCodePointer(uint32_t addr) {
  if (addr < REX_CODE_BASE || addr >= REX_CODE_BASE + REX_CODE_SIZE) {
    return false;
  }
  return rex::runtime::ResolveIndirectFunction(addr) != nullptr;
}

bool IsReadableTrackLoaderObject(rex::memory::Memory* memory, uint32_t object) {
  return object != 0 && GuestRangeReadable(memory, object, kTrackLoaderObjectBytes);
}

uint32_t LoadGlobalTrackLoaderObject(rex::memory::Memory* memory) {
  if (!GuestRangeReadable(memory, kTrackLoaderObjectSlot, 4)) {
    return 0;
  }
  return LoadGuestU32(memory, kTrackLoaderObjectSlot);
}

uint32_t ResolveTrackLoaderWorkerObject(rex::memory::Memory* memory, uint32_t hint) {
  if (IsReadableTrackLoaderObject(memory, hint)) {
    return hint;
  }
  if (IsReadableTrackLoaderObject(memory, g_pending_fiber_job_object)) {
    return g_pending_fiber_job_object;
  }
  const uint32_t global = LoadGlobalTrackLoaderObject(memory);
  if (IsReadableTrackLoaderObject(memory, global)) {
    return global;
  }
  return 0;
}

uint32_t InferJobFnFromFiberContext288(rex::memory::Memory* memory, uint32_t ctx288) {
  if (IsPlausibleGuestCodePointer(ctx288)) {
    return ctx288;
  }
  // job_ctx+288 may hold a saved r31 track-loader object, not a code pointer.
  // Do not map heap objects to sub_823ED888 — 8255AE10 runs that worker after
  // 824843E8 publish; premature fiber dispatch AVs in sub_82492C90 (object+4).
  if (ctx288 >= kGuestHeapLo && ctx288 < kGuestHeapHi &&
      IsReadableTrackLoaderObject(memory, ctx288)) {
    g_pending_fiber_job_object = ctx288;
    static std::atomic<uint32_t> defer_log{0};
    if (defer_log.fetch_add(1, std::memory_order_relaxed) < 12) {
      REXSYS_WARN(
          "FH1 guest-PC fiber: defer world-load worker (track-loader object "
          "0x{:08X} at +288, not job fn)",
          ctx288);
    }
  }
  return 0;
}

bool RunFh1FiberSwapBackTail(PPCContext& ctx, uint8_t* base,
                             uint32_t saved_thread_ctx) {
  auto* runtime = rex::Runtime::instance();
  auto* memory = runtime ? runtime->memory() : nullptr;
  if (!memory) {
    return false;
  }

  const uint32_t tls = ResolveTlsThreadLocalState(memory, ctx);
  if (tls == 0 || saved_thread_ctx == 0 ||
      !GuestRangeReadable(memory, saved_thread_ctx, 56)) {
    return false;
  }

  const uint32_t tls_store = __builtin_bswap32(saved_thread_ctx);
  std::memcpy(memory->TranslateVirtual(tls + 356), &tls_store, sizeof(tls_store));

  ctx.r5.u64 = LoadGuestU32(memory, saved_thread_ctx + 4);
  ctx.r6.u64 = LoadGuestU32(memory, saved_thread_ctx + 8);
  ctx.r7.u64 = LoadGuestU32(memory, saved_thread_ctx + 12);
  uint64_t stack_sp = 0;
  if (GuestRangeReadable(memory, saved_thread_ctx + 48, 8)) {
    std::memcpy(&stack_sp, memory->TranslateVirtual(saved_thread_ctx + 48),
                sizeof(stack_sp));
    stack_sp = __builtin_bswap64(stack_sp);
  }
  ctx.r3.u64 = stack_sp;

  return rex::ppc::InvokeGuestPcFiberKeSet(
      ctx, base, [](PPCContext& c, uint8_t* b) {
        __imp__KeSetCurrentStackPointers(c, b);
      });
}

void RegisterFh1GuestPcFiberConfig() {
  rex::ppc::GuestPcFiberConfig config{};
  config.guest_regions = {
      {0x82C0B800u, 0x82C10000u},
      {0x830EBE00u, 0x830EF000u},
  };
  config.resume_rewrites = {
      // KeSet fires before guest mtlr; lr is still sub_830ED910 entry.
      {0x830ED910u, kEd910PostKeSetResumeLr, false},
  };
  config.native_sites = {
      {kEd910PostKeSetResumeLr},
  };

  config.native_site_handler =
      [](PPCContext& ctx, uint8_t* base, uint32_t pc) -> rex::ppc::GuestPcRunResult {
        if (pc != kEd910PostKeSetResumeLr) {
          return rex::ppc::GuestPcRunResult::UnknownPc;
        }
        if (!IsPlausibleGuestCodePointer(static_cast<uint32_t>(ctx.r31.u64))) {
          static std::atomic<uint32_t> bad_log{0};
          if (bad_log.fetch_add(1, std::memory_order_relaxed) < 12) {
            REXSYS_WARN(
                "FH1 guest-PC fiber: skip EBEA0 (invalid r31=0x{:08X}) sp=0x{:08X}",
                static_cast<uint32_t>(ctx.r31.u64),
                static_cast<uint32_t>(ctx.r1.u64));
          }
          return rex::ppc::GuestPcRunResult::Complete;
        }
        static std::atomic<uint32_t> dispatch_log{0};
        if (dispatch_log.fetch_add(1, std::memory_order_relaxed) < 16) {
          REXSYS_WARN(
              "FH1 guest-PC fiber: EBEA0 dispatch r31=0x{:08X} sp=0x{:08X}",
              static_cast<uint32_t>(ctx.r31.u64),
              static_cast<uint32_t>(ctx.r1.u64));
        }

        ctx.r3.u64 = ctx.r31.u64;
        __imp__sub_830EBEA0(ctx, base);
        return rex::ppc::GuestPcRunResult::Complete;
      };

  config.capture_job_fn = [](PPCContext& ctx, uint8_t* base, uint32_t job_ctx) -> uint32_t {
    (void)base;
    auto* runtime = rex::Runtime::instance();
    auto* memory = runtime ? runtime->memory() : nullptr;
    if (!memory) {
      return 0;
    }

    g_pending_fiber_job_object = 0;

    const uint32_t from_r31 = static_cast<uint32_t>(ctx.r31.u64);
    if (IsPlausibleGuestCodePointer(from_r31)) {
      return from_r31;
    }

    if (job_ctx != 0 && GuestRangeReadable(memory, job_ctx + 288, 4)) {
      const uint32_t from_ctx288 = LoadGuestU32(memory, job_ctx + 288);
      const uint32_t inferred = InferJobFnFromFiberContext288(memory, from_ctx288);
      if (inferred != 0) {
        ctx.r31.u64 = inferred;
        return inferred;
      }
    }

    const uint32_t tls = ResolveTlsThreadLocalState(memory, ctx);
    const uint32_t tls_ctx =
        tls != 0 && GuestRangeReadable(memory, tls + 356, 4)
            ? LoadGuestU32(memory, tls + 356)
            : 0u;
    if (tls_ctx != 0 && tls_ctx != job_ctx &&
        GuestRangeReadable(memory, tls_ctx + 288, 4)) {
      const uint32_t from_tls288 = LoadGuestU32(memory, tls_ctx + 288);
      const uint32_t inferred = InferJobFnFromFiberContext288(memory, from_tls288);
      if (inferred != 0) {
        ctx.r31.u64 = inferred;
        return inferred;
      }
    }

    const uint32_t inferred_r31 = InferJobFnFromFiberContext288(memory, from_r31);
    if (inferred_r31 != 0) {
      ctx.r31.u64 = inferred_r31;
      return inferred_r31;
    }

    return 0;
  };

  config.resolve_job_arg = [](PPCContext& ctx, uint8_t* base, uint32_t job_fn) -> uint32_t {
    (void)base;
    auto* runtime = rex::Runtime::instance();
    auto* memory = runtime ? runtime->memory() : nullptr;
    if (!memory) {
      return 0;
    }

    const uint32_t tls = ResolveTlsThreadLocalState(memory, ctx);
    const uint32_t ctx_block =
        tls != 0 && GuestRangeReadable(memory, tls + 356, 4)
            ? LoadGuestU32(memory, tls + 356)
            : 0u;
    const uint32_t from_ctx =
        ctx_block != 0 && GuestRangeReadable(memory, ctx_block, 4)
            ? LoadGuestU32(memory, ctx_block + 0)
            : 0u;

    if (job_fn == kFiberPollJobEntry) {
      const uint32_t queue = fh1::workqueue::RecoverWorkQueue(memory, from_ctx);
      if (queue != 0) {
        return queue;
      }
    }

    if (job_fn == kWorldLoadWorkerEntry) {
      const uint32_t object = ResolveTrackLoaderWorkerObject(memory, from_ctx);
      if (object != 0) {
        return object;
      }
      if (ctx_block != 0 && GuestRangeReadable(memory, ctx_block + 288, 4)) {
        const uint32_t from288 = LoadGuestU32(memory, ctx_block + 288);
        if (IsReadableTrackLoaderObject(memory, from288)) {
          return from288;
        }
      }
    }

    return from_ctx;
  };

  config.run_swap_back_tail = RunFh1FiberSwapBackTail;

  config.before_dispatch_job =
      [](PPCContext& ctx, uint8_t* base, uint32_t job_fn, uint32_t job_arg) {
        (void)ctx;
        (void)base;
        if (job_fn == kFiberPollJobEntry && job_arg != 0) {
          fh1::workqueue::StashWorkQueue(job_arg);
        }
      };

  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

struct Fh1FiberBoot {
  Fh1FiberBoot() {
    RegisterFh1GuestPcFiberConfig();
    rex::ppc::InstallGuestPcFiberInterpreter();
  }
};

const Fh1FiberBoot g_boot;

}  // namespace

bool IsFiberInterpreterActive() {
  return rex::ppc::IsGuestPcFiberActive();
}

bool ShouldPreserveGuestR31(const PPCContext& ctx) {
  return rex::ppc::ShouldPreserveGuestPcFiberR31(ctx);
}

void InstallGuestPcFiberInterpreter() {
  RegisterFh1GuestPcFiberConfig();
  rex::ppc::InstallGuestPcFiberInterpreter();
}

void ReconcileFiberFrameStack(const PPCContext& ctx) {
  rex::ppc::ReconcileGuestPcFiberFrameStack(ctx);
}

bool RunFiberSwap(PPCContext& ctx, uint8_t* base, uint32_t job_ctx,
                  uint32_t fiber_slot_at_entry) {
  return rex::ppc::RunFiberSwap(ctx, base, FiberSwapImplEd910, job_ctx,
                                fiber_slot_at_entry);
}

GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc) {
  return rex::ppc::RunGuestPc(ctx, base, start_pc);
}

}  // namespace fh1::fiber
