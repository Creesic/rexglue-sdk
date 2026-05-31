// FH1 work-queue dispatch guards for sub_82C0BC88 / sub_82C0BDC8.
//
// Loading worker crashes were AVs through the job ring at *(queue+0). Queue
// layout uses [+0,+4) as the ring byte range (end pointer at +4), 72 bytes per
// slot. Guards commit the full ring when reserved-but-uncommitted, commit the
// fiber job context block before dispatch, and restore PPCContext on SEH.

#include "fh1_guest_pc_fiber.h"
#include "fh1_loader_epoch.h"
#include "fh1_workqueue.h"
#include "generated/fh1_init.h"

#include <atomic>
#include <cstring>

#if defined(_MSC_VER)
#include <windows.h>
#endif

#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>
#include <rex/system/xthread.h>

namespace {

constexpr uint32_t kFiberContextBytes =
    static_cast<uint32_t>(sizeof(rex::system::X_FIBER_CONTEXT));

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address);
bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size);

#if defined(_MSC_VER)
struct SehFaultInfo {
  DWORD code = 0;
  uintptr_t address = 0;
};

#define FH1_SEH_FILTER(info)                                                     \
  ((info).code = GetExceptionCode(),                                           \
   (info).address =                                                             \
       (GetExceptionInformation()->ExceptionRecord->NumberParameters >= 2)      \
           ? static_cast<uintptr_t>(GetExceptionInformation()                   \
                                         ->ExceptionRecord                      \
                                         ->ExceptionInformation[1])             \
           : 0,                                                                 \
   EXCEPTION_EXECUTE_HANDLER)

void LogSehFault(rex::memory::Memory* memory, const char* site, const SehFaultInfo& fault) {
  static std::atomic<uint32_t> count{0};
  const uint32_t n = count.fetch_add(1, std::memory_order_relaxed);
  if (n >= 8 && (n & 0xFF) != 0) {
    return;
  }
  const uint32_t guest =
      fault.address != 0
          ? memory->HostToGuestVirtual(reinterpret_cast<const void*>(fault.address))
          : 0;
  REXSYS_WARN(
      "FH1 {}: SEH code=0x{:08X} host=0x{:X} guest=0x{:08X} (#{})",
      site, static_cast<uint32_t>(fault.code), fault.address, guest, n + 1);
}
#endif

void LogGuestRegion(rex::memory::Memory* memory, const char* label, uint32_t addr,
                    uint32_t need_bytes) {
  if (addr == 0) {
    REXSYS_WARN("FH1 mem {}: addr=0 (null)", label);
    return;
  }

  rex::memory::HeapAllocationInfo info{};
  auto* heap = memory->LookupHeap(addr);
  const bool start_ok = memory->IsGuestVirtualCommitted(addr);
  const uint32_t end_addr = addr + need_bytes - 1;
  const bool end_ok =
      need_bytes > 0 && end_addr >= addr && memory->IsGuestVirtualCommitted(end_addr);
  uint32_t region_size = 0;
  uint32_t state = 0;
  if (heap && heap->QueryRegionInfo(addr, &info)) {
    region_size = info.region_size;
    state = info.state;
  }

  REXSYS_WARN(
      "FH1 mem {}: addr=0x{:08X} need=0x{:X} start_ok={} end_ok={} region=0x{:X} state=0x{:X}",
      label, addr, need_bytes, start_ok, end_ok, region_size, state);
}

uint32_t ResolveTlsThreadLocalState(rex::memory::Memory* memory, const PPCContext& ctx) {
  if (!GuestRangeReadable(memory, ctx.r13.u32 + 256, 4)) {
    return 0;
  }
  return LoadGuestU32(memory, ctx.r13.u32 + 256);
}

void LogFiberSwapContext(rex::memory::Memory* memory, const PPCContext& ctx,
                         uint32_t job_ctx) {
  static std::atomic<uint32_t> count{0};
  if (count.fetch_add(1, std::memory_order_relaxed) >= 3) {
    return;
  }

  const uint32_t tls = ResolveTlsThreadLocalState(memory, ctx);
  const uint32_t fiber_slot = tls ? LoadGuestU32(memory, tls + 356) : 0;

  uint32_t stack_alloc = 0;
  uint32_t stack_base = 0;
  uint32_t stack_limit = 0;
  uint64_t stack_sp = 0;
  if (job_ctx != 0 && GuestRangeReadable(memory, job_ctx, 56)) {
    stack_alloc = LoadGuestU32(memory, job_ctx + 4);
    stack_base = LoadGuestU32(memory, job_ctx + 8);
    stack_limit = LoadGuestU32(memory, job_ctx + 12);
    if (GuestRangeReadable(memory, job_ctx + 48, 8)) {
      std::memcpy(&stack_sp, memory->TranslateVirtual(job_ctx + 48), sizeof(stack_sp));
      stack_sp = __builtin_bswap64(stack_sp);
    }
  }

  auto* host_thread = rex::system::XThread::TryGetCurrentThread();
  REXSYS_WARN(
      "FH1 fiber swap: job_ctx=0x{:08X} tls=0x{:08X} fiber_slot=0x{:08X} "
      "stack=[0x{:08X},0x{:08X}) alloc=0x{:08X} sp=0x{:08X} host_stack=[0x{:08X},0x{:08X})",
      job_ctx, tls, fiber_slot, stack_limit, stack_base, stack_alloc,
      static_cast<uint32_t>(stack_sp),
      host_thread ? host_thread->stack_limit() : 0u,
      host_thread ? host_thread->stack_base() : 0u);

  LogGuestRegion(memory, "fiber_slot", fiber_slot, kFiberContextBytes);
  LogGuestRegion(memory, "job_ctx", job_ctx, kFiberContextBytes);
  if (stack_limit != 0 && stack_base > stack_limit) {
    LogGuestRegion(memory, "job_stack", stack_limit, stack_base - stack_limit);
  }
}

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address);
bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size);

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

bool EnsureGuestRangeCommitted(rex::memory::Memory* memory, uint32_t address,
                               uint32_t size) {
  if (GuestRangeReadable(memory, address, size)) {
    return true;
  }
  if (address == 0 || size == 0) {
    return false;
  }

  auto* heap = memory->LookupHeap(address);
  uint32_t guest_page = 4096u;
  if (heap && heap->page_size() != 0) {
    guest_page = heap->page_size();
  }

  const uint32_t page_mask = guest_page - 1;
  const uint32_t range_end = address + size - 1;
  if (range_end < address) {
    return false;
  }
  // BaseHeap::AllocFixed requires base aligned to the heap page size (often 64KB).
  const uint32_t page_base = address & ~page_mask;
  const uint32_t page_end = range_end | page_mask;
  const uint32_t commit_size = page_end - page_base + 1;

  if (heap) {
    constexpr uint32_t kProt =
        rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite;
    constexpr uint32_t kCommit = rex::memory::kMemoryAllocationCommit;
    constexpr uint32_t kReserveCommit =
        rex::memory::kMemoryAllocationReserve |
        rex::memory::kMemoryAllocationCommit;
    if (heap->AllocFixed(page_base, commit_size, guest_page, kCommit, kProt) &&
        GuestRangeReadable(memory, address, size)) {
      return true;
    }
    if (heap->AllocFixed(page_base, commit_size, guest_page, kReserveCommit, kProt) &&
        GuestRangeReadable(memory, address, size)) {
      return true;
    }
  }

  return rex::memory::AllocFixed(
             memory->TranslateVirtual(page_base), commit_size,
             rex::memory::AllocationType::kCommit,
             rex::memory::PageAccess::kReadWrite) != nullptr &&
         GuestRangeReadable(memory, address, size);
}

uint32_t ResolveFiberJobContext(rex::memory::Memory* memory,
                                const PPCContext& ctx) {
  if (!GuestRangeReadable(memory, ctx.r13.u32 + 256, 4)) {
    return 0;
  }
  const uint32_t tls_base = LoadGuestU32(memory, ctx.r13.u32 + 256);
  if (tls_base == 0 || !GuestRangeReadable(memory, tls_base, 360 + 4)) {
    return 0;
  }
  return LoadGuestU32(memory, tls_base + 356);
}

void TryCommitFiberJobContext(rex::memory::Memory* memory, PPCContext& ctx) {
  const uint32_t fiber_ctx = ResolveFiberJobContext(memory, ctx);
  if (fiber_ctx == 0) {
    return;
  }
  // sub_830ED910 stvlxl128 saves up to offset 0xA10 (+16) on r10=r5+0x30.
  constexpr uint32_t kFiberContextBytesLocal = kFiberContextBytes;
  if (EnsureGuestRangeCommitted(memory, fiber_ctx, kFiberContextBytesLocal)) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXSYS_WARN(
        "FH1 work-queue: failed to commit fiber job context at 0x{:08X}",
        fiber_ctx);
  }
}

void TryCommitJobFiberStack(rex::memory::Memory* memory, uint32_t job_ctx) {
  if (job_ctx == 0 || !GuestRangeReadable(memory, job_ctx, 56)) {
    return;
  }

  const uint32_t stack_alloc = LoadGuestU32(memory, job_ctx + 4);
  const uint32_t stack_base = LoadGuestU32(memory, job_ctx + 8);
  const uint32_t stack_limit = LoadGuestU32(memory, job_ctx + 12);

  if (stack_limit != 0 && stack_base > stack_limit) {
    const uint32_t stack_bytes = stack_base - stack_limit;
    if (stack_bytes <= 16 * 1024 * 1024) {
      if (!EnsureGuestRangeCommitted(memory, stack_limit, stack_bytes)) {
        static std::atomic<bool> logged{false};
        if (!logged.exchange(true)) {
          REXSYS_WARN(
              "FH1 work-queue: failed to commit fiber stack [{:08X},{:08X}) "
              "for job_ctx=0x{:08X}",
              stack_limit, stack_base, job_ctx);
        }
      }
    }
  }

  if (stack_alloc != 0 && stack_base > stack_alloc) {
    const uint32_t alloc_bytes = stack_base - stack_alloc;
    if (alloc_bytes <= 16 * 1024 * 1024) {
      EnsureGuestRangeCommitted(memory, stack_alloc, alloc_bytes);
    }
  }
}

void TryCommitJobContext(rex::memory::Memory* memory, uint32_t job_ctx) {
  if (job_ctx == 0) {
    return;
  }
  // sub_830ED910 restores job ctx from r3 with the same vector footprint.
  if (EnsureGuestRangeCommitted(memory, job_ctx, kFiberContextBytes)) {
    TryCommitJobFiberStack(memory, job_ctx);
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXSYS_WARN(
        "FH1 work-queue: failed to commit job context struct at 0x{:08X}",
        job_ctx);
  }
}

void LogDispatchContextOnce(rex::memory::Memory* memory, uint32_t queue,
                            uint32_t ring_start, uint32_t ring_bytes,
                            const PPCContext& ctx) {
  static std::atomic<bool> logged{false};
  if (logged.exchange(true)) {
    return;
  }

  const uint32_t slot_count = ring_bytes / 72;
  const uint32_t head = LoadGuestU32(memory, queue + 16);
  const uint32_t probe = slot_count ? ((head + 1) % slot_count) : 0;
  const uint32_t slot = ring_start + probe * 72;
  const uint32_t job_fn =
      GuestRangeReadable(memory, slot, 4) ? LoadGuestU32(memory, slot) : 0;
  const uint8_t ready =
      GuestRangeReadable(memory, slot + 64, 1)
          ? memory->TranslateVirtual<const uint8_t*>(slot + 64)[0]
          : 0;
  const uint32_t nested =
      GuestRangeReadable(memory, slot + 48, 4) ? LoadGuestU32(memory, slot + 48)
                                               : 0;
  const uint32_t slot_obj =
      GuestRangeReadable(memory, slot + 32, 4) ? LoadGuestU32(memory, slot + 32) : 0;
  const uint32_t slot_obj_nested =
      slot_obj && GuestRangeReadable(memory, slot_obj + 16, 4)
          ? LoadGuestU32(memory, slot_obj + 16)
          : 0;

  uint32_t stack_limit = 0;
  uint32_t stack_base = 0;
  uint64_t stack_sp = 0;
  if (job_fn != 0 && GuestRangeReadable(memory, job_fn, 56)) {
    stack_limit = LoadGuestU32(memory, job_fn + 12);
    stack_base = LoadGuestU32(memory, job_fn + 8);
    if (GuestRangeReadable(memory, job_fn + 48, 8)) {
      std::memcpy(&stack_sp, memory->TranslateVirtual(job_fn + 48), sizeof(stack_sp));
      stack_sp = __builtin_bswap64(stack_sp);
    }
  }

  REXSYS_WARN(
      "FH1 sub_82C0BC88: dispatch ctx queue=0x{:08X} head={} probe_slot={} "
      "job_ctx=0x{:08X} ready={} nested=0x{:08X} slot_obj=0x{:08X} slot_obj+16=0x{:08X} "
      "fiber_ctx=0x{:08X} tls=0x{:08X} "
      "job_stack=[0x{:08X},0x{:08X}) sp=0x{:08X}",
      queue, head, probe, job_fn, ready, nested, slot_obj, slot_obj_nested,
      ResolveFiberJobContext(memory, ctx), ResolveTlsThreadLocalState(memory, ctx),
      stack_limit, stack_base, static_cast<uint32_t>(stack_sp));
}

bool PrepareWorkQueueRing(rex::memory::Memory* memory, uint32_t queue,
                          uint32_t* out_start, uint32_t* out_bytes) {
  if (!GuestRangeReadable(memory, queue, 20)) {
    return false;
  }

  const uint32_t start = LoadGuestU32(memory, queue + 0);
  const uint32_t end = LoadGuestU32(memory, queue + 4);
  if (start == 0 || end <= start) {
    return false;
  }

  const uint32_t bytes = end - start;
  if (bytes < 72 || (bytes % 72) != 0) {
    return false;
  }

  if (!EnsureGuestRangeCommitted(memory, start, bytes)) {
    return false;
  }
  if (!GuestRangeReadable(memory, start, bytes)) {
    return false;
  }

  *out_start = start;
  *out_bytes = bytes;
  return true;
}

thread_local uint32_t g_fh1_active_work_queue = 0;

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

bool IsPlausibleWorkQueuePointer(rex::memory::Memory* memory, uint32_t queue) {
  if (IsKnownPoisonGuestPointer(queue)) {
    return false;
  }
  uint32_t ring_start = 0;
  uint32_t ring_bytes = 0;
  return PrepareWorkQueueRing(memory, queue, &ring_start, &ring_bytes);
}

uint32_t SanitizeWorkQueuePointer(rex::memory::Memory* memory, uint32_t queue) {
  if (IsPlausibleWorkQueuePointer(memory, queue)) {
    g_fh1_active_work_queue = queue;
    return queue;
  }
  if (g_fh1_active_work_queue != 0 &&
      IsPlausibleWorkQueuePointer(memory, g_fh1_active_work_queue)) {
    static std::atomic<uint32_t> recover_log{0};
    if (recover_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 work-queue: recovered queue 0x{:08X} (bad ptr 0x{:08X})",
          g_fh1_active_work_queue, queue);
    }
    return g_fh1_active_work_queue;
  }
  return 0;
}

void StashActiveWorkQueue(uint32_t queue) {
  if (queue != 0) {
    g_fh1_active_work_queue = queue;
  }
}

void LogWorkQueueRetry(const char* where, uint32_t queue, uint32_t ring_start,
                       uint32_t ring_bytes, const char* detail) {
  static std::atomic<uint32_t> count{0};
  const uint32_t n = count.fetch_add(1, std::memory_order_relaxed);
  if (n < 8 || (n & 0x3FF) == 0) {
    REXSYS_WARN(
        "FH1 {}: queue=0x{:08X} ring=[0x{:08X},+0x{:X}) {}; retry #{}",
        where, queue, ring_start, ring_bytes, detail, n + 1);
  }
}

bool GuestStackFrameValid(uint32_t r1, uint32_t frame_bytes) {
  auto* thread = rex::system::XThread::TryGetCurrentThread();
  if (!thread) {
    return true;
  }
  if (r1 < thread->stack_limit() || r1 > thread->stack_base()) {
    return false;
  }
  if (frame_bytes > r1 - thread->stack_limit()) {
    return false;
  }
  return true;
}

void TryCommitProbeSlotJobContext(rex::memory::Memory* memory, uint32_t queue,
                                  uint32_t ring_start, uint32_t ring_bytes) {
  const uint32_t slot_count = ring_bytes / 72;
  if (slot_count == 0) {
    return;
  }
  const uint32_t head = LoadGuestU32(memory, queue + 16);
  const uint32_t probe = (head + 1) % slot_count;
  const uint32_t slot = ring_start + probe * 72;
  const uint32_t job_ctx =
      GuestRangeReadable(memory, slot, 4) ? LoadGuestU32(memory, slot) : 0;
  TryCommitJobContext(memory, job_ctx);
  EnsureGuestRangeCommitted(memory, slot, 72);
}

void DispatchWorkQueueOrRetry(PPCContext& ctx, uint8_t* base) {
  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t queue = SanitizeWorkQueuePointer(memory, ctx.r3.u32);
  const uint64_t caller_r1 = ctx.r1.u64;

  if (queue == 0) {
    LogWorkQueueRetry("sub_82C0BC88", ctx.r3.u32, 0, 0, "invalid queue pointer");
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u32 = queue;

  if (!GuestStackFrameValid(static_cast<uint32_t>(caller_r1), 112)) {
    ctx.r3.u64 = 0;
    return;
  }

  uint32_t ring_start = 0;
  uint32_t ring_bytes = 0;
  if (!PrepareWorkQueueRing(memory, queue, &ring_start, &ring_bytes)) {
    LogWorkQueueRetry("sub_82C0BC88", queue, ring_start, ring_bytes,
                      "ring not ready");
    ctx.r3.u64 = 0;
    return;
  }

  LogDispatchContextOnce(memory, queue, ring_start, ring_bytes, ctx);
  TryCommitProbeSlotJobContext(memory, queue, ring_start, ring_bytes);
  TryCommitFiberJobContext(memory, ctx);

  // No __try/__except: sub_830ED910 KeSet longjmp reenters inside this call
  // tree; MSVC reports SEH code=0 or 0xC0000028 if an outer __try wraps it.
  __imp__sub_82C0BC88(ctx, base);
}

void PollWorkQueueOrRetry(PPCContext& ctx, uint8_t* base) {
  auto* memory = rex::Runtime::instance()->memory();
  const uint64_t entry_r1 = ctx.r1.u64;
  const uint32_t queue = SanitizeWorkQueuePointer(memory, ctx.r3.u32);

  if (queue == 0) {
    LogWorkQueueRetry("sub_82C0BDC8", ctx.r3.u32, 0, 0, "invalid queue pointer");
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u32 = queue;

  if (!GuestStackFrameValid(static_cast<uint32_t>(entry_r1), 128)) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      auto* thread = rex::system::XThread::TryGetCurrentThread();
      REXSYS_ERROR(
          "FH1 sub_82C0BDC8: guest r1=0x{:08X} outside stack [{:08X},{:08X}]; retry",
          static_cast<uint32_t>(entry_r1),
          thread ? thread->stack_limit() : 0u, thread ? thread->stack_base() : 0u);
    }
    ctx.r3.u64 = 0;
    return;
  }

  // No __try/__except: fiber KeSet longjmp must not cross SEH frames (0xC0000028).
  __imp__sub_82C0BDC8(ctx, base);
}

uint32_t ActiveWorkQueueSnapshot() {
  return g_fh1_active_work_queue;
}

}  // namespace

namespace fh1::workqueue {

bool IsPlausibleWorkQueue(rex::memory::Memory* memory, uint32_t queue) {
  return IsPlausibleWorkQueuePointer(memory, queue);
}

uint32_t RecoverWorkQueue(rex::memory::Memory* memory, uint32_t hint) {
  return SanitizeWorkQueuePointer(memory, hint);
}

void StashWorkQueue(uint32_t queue) {
  StashActiveWorkQueue(queue);
}

}  // namespace fh1::workqueue

extern "C" REX_FUNC(sub_82C0BC88) {
  REX_FUNC_PROLOGUE();
  fh1_load_gate_dispatch_wait(0x82C0BC88u);
  DispatchWorkQueueOrRetry(ctx, base);
}

extern "C" REX_FUNC(sub_82C0BDC8) {
  REX_FUNC_PROLOGUE();
  fh1_load_gate_dispatch_wait(0x82C0BDC8u);
  fh1::fiber::ReconcileFiberFrameStack(ctx);
  PollWorkQueueOrRetry(ctx, base);
}

extern "C" REX_FUNC(sub_82C0BEC8) {
  REX_FUNC_PROLOGUE();
  fh1_load_gate_dispatch_wait(0x82C0BEC8u);

  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t queue = SanitizeWorkQueuePointer(memory, ctx.r3.u32);
  if (queue == 0) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXSYS_WARN(
          "FH1 sub_82C0BEC8: invalid queue pointer 0x{:08X}; skipping fiber poll job",
          ctx.r3.u32);
    }
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u32 = queue;

  __imp__sub_82C0BEC8(ctx, base);
}

extern "C" REX_FUNC(sub_82C013E0) {
  REX_FUNC_PROLOGUE();
  // Same longjmp/reentry chain as sub_82C0BC88 — avoid SEH frames here too.
  __imp__sub_82C013E0(ctx, base);
}

extern "C" REX_FUNC(sub_830EBE90) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t job_ctx = ctx.r3.u32;
  LogFiberSwapContext(memory, ctx, job_ctx);
  TryCommitJobContext(memory, job_ctx);
  TryCommitFiberJobContext(memory, ctx);

  if (memory != nullptr && job_ctx != 0 &&
      GuestRangeReadable(memory, job_ctx + 288, 4)) {
    const uint32_t at288 = LoadGuestU32(memory, job_ctx + 288);
    if (at288 >= 0x40000000u && at288 < 0x50000000u &&
        GuestRangeReadable(memory, at288, 712) &&
        GuestRangeReadable(memory, job_ctx, 4)) {
      const uint32_t be = __builtin_bswap32(at288);
      std::memcpy(memory->TranslateVirtual(job_ctx + 0), &be, sizeof(be));
    }
  }

  __imp__sub_830EBE90(ctx, base);
}

extern "C" REX_FUNC(sub_830ED910) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t job_ctx = ctx.r3.u32;
  const uint32_t fiber_slot_at_entry = ResolveFiberJobContext(memory, ctx);
  LogFiberSwapContext(memory, ctx, job_ctx);
  TryCommitJobContext(memory, job_ctx);
  TryCommitFiberJobContext(memory, ctx);

  const bool swap_back =
      fh1::fiber::RunFiberSwap(ctx, base, job_ctx, fiber_slot_at_entry);
  if (swap_back) {
    static std::atomic<uint32_t> done_log{0};
    if (done_log.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXSYS_WARN(
          "FH1 fiber swap: guest-PC interpreter swap-back complete "
          "(fiber_slot=0x{:08X} lr=0x{:08X})",
          fiber_slot_at_entry, static_cast<uint32_t>(ctx.lr));
    }
  }
}

uint32_t Fh1GetActiveWorkQueue() {
  return ActiveWorkQueueSnapshot();
}
