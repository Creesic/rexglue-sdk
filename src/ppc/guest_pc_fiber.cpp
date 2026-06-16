/**
 * @file        ppc/guest_pc_fiber.cpp
 * @brief       OS-fiber-backed implementation of guest-PC fiber reentry
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 *
 * @remarks     See include/rex/ppc/guest_pc_fiber.h and
 *              fh1/patcher/fiber-reentry.md for the design rationale.
 */

#include <rex/ppc/guest_pc_fiber.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/kernel_state.h>
#include <rex/system/thread_state.h>
#include <rex/system/xmemory.h>
#include <rex/system/xthread.h>
#include <rex/thread/fiber.h>

namespace rex::ppc {

namespace {

using rex::system::XThread;

// TLS layout used by the guest fiber library (see sub_830ED910):
//   current fiber context block = *(*(r13 + 0x100) + 0x164)
constexpr uint32_t kTlsBaseOffset = 0x100;      // r13 + 256
constexpr uint32_t kTlsCurrentFiberOffset = 0x164;  // tls + 356

// Generous host stack for guest fibers -- the world-load worker uses a large
// frame and recurses through the asset pipeline.
constexpr size_t kHostFiberStackSize = 1u << 20;  // 1 MiB

/// Host bookkeeping for one guest fiber, keyed by its guest context block.
struct FiberSlot {
  rex::thread::Fiber* host = nullptr;
  uint32_t block = 0;
  bool is_thread_fiber = false;  ///< backed by the thread's main (converted) fiber
};

/// Args handed to a freshly created host fiber's entry trampoline.
struct GuestFiberEntryArgs {
  uint32_t block;
};

// Construct-on-first-use singletons: the project driver may register config and
// install from a static constructor, so these must not depend on this TU's
// global-initialization order. (std::atomic<bool> has constant init and is safe
// as a namespace-scope global.)
std::atomic<bool> g_active{false};

// TEMP diagnostic: trace the first N swaps/dispatches to characterize the
// world-load hang. Remove once the fiber path is confirmed working.
std::atomic<uint32_t> g_swap_seq{0};
constexpr uint32_t kSwapLogCap = 600;

std::mutex& Mtx() {
  static std::mutex m;
  return m;
}

std::unordered_map<uint32_t, std::unique_ptr<FiberSlot>>& Map() {
  static std::unordered_map<uint32_t, std::unique_ptr<FiberSlot>> m;
  return m;
}

GuestPcFiberConfig& Config() {
  static GuestPcFiberConfig c;
  return c;
}

memory::Memory* CurrentMemory(XThread* thread) {
  auto* ks = thread ? thread->kernel_state() : nullptr;
  return ks ? ks->memory() : nullptr;
}

/// Big-endian guest u32 read with a commit guard (the swap path is cold, so the
/// per-read QueryProtect is acceptable and keeps us robust against stale TLS).
uint32_t GuestLoadU32(memory::Memory* mem, uint32_t addr) {
  if (!mem || addr == 0) {
    return 0;
  }
  auto* heap = mem->LookupHeap(addr);
  uint32_t protect = 0;
  if (!heap || !heap->QueryProtect(addr, &protect) ||
      protect == memory::kMemoryProtectNoAccess) {
    return 0;
  }
  return __builtin_bswap32(*mem->TranslateVirtual<uint32_t*>(addr));
}

/// Resolve the guest context block of the fiber currently active on `ctx`.
uint32_t ReadCurrentFiberBlock(const PPCContext& ctx, memory::Memory* mem) {
  const uint32_t tls = GuestLoadU32(mem, static_cast<uint32_t>(ctx.r13.u64) + kTlsBaseOffset);
  if (!tls) {
    return 0;
  }
  return GuestLoadU32(mem, tls + kTlsCurrentFiberOffset);
}

FiberSlot* LookupSlot(uint32_t block) {
  std::lock_guard<std::mutex> lk(Mtx());
  auto it = Map().find(block);
  return it == Map().end() ? nullptr : it->second.get();
}

/// Register (or refresh) the host fiber that backs `block`.
void EnsureSlot(uint32_t block, rex::thread::Fiber* host, bool is_thread_fiber) {
  if (!block || !host) {
    return;
  }
  std::lock_guard<std::mutex> lk(Mtx());
  auto& slot = Map()[block];
  if (!slot) {
    slot = std::make_unique<FiberSlot>();
  }
  slot->host = host;
  slot->block = block;
  slot->is_thread_fiber = is_thread_fiber;
}

void GuestFiberEntry(void* raw_arg);

/// Lazily create the host fiber backing a target block we have never switched
/// to before. Its entry dispatches the guest from the target's resume PC.
FiberSlot* CreateHostFiberForTarget(uint32_t block) {
  auto args = std::make_unique<GuestFiberEntryArgs>(GuestFiberEntryArgs{block});
  auto* host = rex::thread::Fiber::Create(kHostFiberStackSize, &GuestFiberEntry, args.get());
  if (!host) {
    REXKRNL_ERROR("guest_pc_fiber: Fiber::Create failed for block {:#010x}", block);
    return nullptr;
  }
  args.release();  // ownership transferred to GuestFiberEntry

  std::lock_guard<std::mutex> lk(Mtx());
  auto& slot = Map()[block];
  slot = std::make_unique<FiberSlot>();
  slot->host = host;
  slot->block = block;
  slot->is_thread_fiber = false;
  return slot.get();
}

void GuestFiberEntry(void* raw_arg) {
  auto args = std::unique_ptr<GuestFiberEntryArgs>(static_cast<GuestFiberEntryArgs*>(raw_arg));
  auto* thread = XThread::GetCurrentThread();
  if (!thread || !thread->thread_state()) {
    REXKRNL_ERROR("guest_pc_fiber: fiber entry without a bound thread");
    return;
  }
  PPCContext& ctx = *thread->thread_state()->context();
  auto* mem = CurrentMemory(thread);
  uint8_t* base = mem ? mem->virtual_membase() : nullptr;

  // PPCContext already holds THIS fiber's register state -- the switching
  // fiber's swapImpl restored it from this block before Fiber::SwitchTo. The
  // resume PC (target fiber start) is in ctx.lr and the stack in ctx.r1.
  const uint32_t resume_pc = static_cast<uint32_t>(ctx.lr);

  REXKRNL_WARN("gpf fiber-entry block={:#010x} resume_pc={:#010x} sp={:#010x} job@+288={:#010x}",
              args->block, resume_pc, static_cast<uint32_t>(ctx.r1.u64),
              GuestLoadU32(mem, args->block + 288));

  if (Config().capture_job_fn) {
    (void)Config().capture_job_fn(ctx, base, args->block);
  }
  if (Config().before_dispatch_job) {
    Config().before_dispatch_job(ctx, base, resume_pc, static_cast<uint32_t>(ctx.r3.u64));
  }

  RunGuestPc(ctx, base, resume_pc);

  // The guest fiber function returned: the coroutine is finished. Return control
  // to the thread's main fiber so its host stack unwinds normally.
  REXKRNL_WARN("gpf fiber-entry block={:#010x} RunGuestPc RETURNED (coroutine finished) lr={:#010x}",
              args->block, static_cast<uint32_t>(ctx.lr));
  auto* main = thread->main_fiber();
  if (main && main != rex::thread::Fiber::Current()) {
    rex::thread::Fiber::SwitchTo(main);
  }
  REXKRNL_WARN("guest_pc_fiber: fiber for block {:#010x} resumed after completion", args->block);
}

}  // namespace

void RegisterGuestPcFiberConfig(GuestPcFiberConfig config) {
  std::lock_guard<std::mutex> lk(Mtx());
  Config() = std::move(config);
}

void InstallGuestPcFiberInterpreter() {
  if (g_active.exchange(true)) {
    return;  // already active
  }
  REXKRNL_INFO("guest_pc_fiber: OS-fiber-backed reentry active ({} deny entries)",
              Config().deny_fiber_job_entries.size());
}

bool IsGuestPcFiberActive() {
  return g_active.load(std::memory_order_acquire);
}

void ReconcileGuestPcFiberFrameStack(const PPCContext& /*ctx*/) {
  // No-op: under the OS-fiber backend each guest fiber owns a real host stack,
  // so the host call stack is authoritative and needs no reconciliation.
}

bool RunFiberSwap(PPCContext& ctx, uint8_t* base, PPCFunc* swapImpl, uint32_t job_ctx,
                  uint32_t fiber_slot) {
  (void)job_ctx;
  (void)fiber_slot;

  auto* thread = XThread::GetCurrentThread();

  // TEMP diagnostic on the file-captured WARN channel: prove the override is
  // reached and report whether we have an XThread / active subsystem.
  {
    const uint32_t eseq = g_swap_seq.load(std::memory_order_relaxed);
    if (eseq < kSwapLogCap) {
      REXKRNL_WARN(
          "gpf-hook ENTER active={} thread={} tid={} swapImpl={} tgt(r3)={:#010x} lr={:#010x} "
          "r1={:#010x} r13={:#010x}",
          g_active.load(std::memory_order_acquire), static_cast<const void*>(thread),
          thread ? thread->thread_id() : 0u, reinterpret_cast<uintptr_t>(swapImpl),
          static_cast<uint32_t>(ctx.r3.u64), static_cast<uint32_t>(ctx.lr),
          static_cast<uint32_t>(ctx.r1.u64), static_cast<uint32_t>(ctx.r13.u64));
    }
  }

  if (!g_active.load(std::memory_order_acquire) || !thread || !swapImpl) {
    // Subsystem inactive or no thread context: degrade to the legacy inline
    // swap so behavior is unchanged when the fix is not installed.
    if (swapImpl) {
      swapImpl(ctx, base);
    }
    return false;
  }

  auto* mem = CurrentMemory(thread);

  // Snapshot the *source* fiber before swapImpl rewrites the TLS-current slot.
  const uint32_t current_block = ReadCurrentFiberBlock(ctx, mem);
  const uint32_t target_block = static_cast<uint32_t>(ctx.r3.u64);  // guest swap ABI
  const uint32_t entry_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t entry_r1 = static_cast<uint32_t>(ctx.r1.u64);

  rex::thread::Fiber* current_host = rex::thread::Fiber::Current();
  const bool had_current = current_host != nullptr;
  if (!current_host) {
    current_host = thread->main_fiber();
  }

  const uint32_t seq = g_swap_seq.fetch_add(1, std::memory_order_relaxed);
  const bool trace = seq < kSwapLogCap;
  if (trace) {
    REXKRNL_INFO(
        "gpf swap #{} tid={} cur_blk={:#010x} tgt_blk={:#010x} cur_host={} had_cur={} "
        "main={} entry_lr={:#010x} r1={:#010x} r13={:#010x}",
        seq, thread->thread_id(), current_block, target_block,
        static_cast<const void*>(current_host), had_current,
        static_cast<const void*>(thread->main_fiber()), entry_lr, entry_r1,
        static_cast<uint32_t>(ctx.r13.u64));
  }

  // Make the source fiber resumable: register its host fiber under its block so
  // a later swap back to `current_block` finds this host stack.
  if (current_block) {
    EnsureSlot(current_block, current_host, current_host == thread->main_fiber());
  }

  // Reuse the guest's own register save/restore: saves the current registers
  // into the current context block and loads the target's into PPCContext,
  // setting ctx.lr = target resume PC, ctx.r1 = target SP, the TLS-current
  // slot, and the KTHREAD/PCR stack fields. After this, ctx == target state.
  swapImpl(ctx, base);

  FiberSlot* target = LookupSlot(target_block);
  const bool brand_new = (target == nullptr);
  if (trace) {
    REXKRNL_WARN("gpf swap #{} post-swap resume_pc={:#010x} tgt_sp={:#010x} brand_new={}", seq,
                static_cast<uint32_t>(ctx.lr), static_cast<uint32_t>(ctx.r1.u64), brand_new);
  }
  if (brand_new) {
    target = CreateHostFiberForTarget(target_block);
    if (!target) {
      // Cannot back the target with a host fiber -- fall through and let the
      // guest continue inline on the current host stack (best effort).
      if (trace) REXKRNL_ERROR("gpf swap #{} CreateHostFiber FAILED -> inline", seq);
      return false;
    }
  }

  if (!target->host || target->host == current_host) {
    if (trace) {
      REXKRNL_WARN("gpf swap #{} no-switch (host={} cur_host={}) -> inline continue", seq,
                  static_cast<const void*>(target->host), static_cast<const void*>(current_host));
    }
    return false;  // already on the right host stack
  }

  // Hand the host stack to the target fiber. Control returns here only when
  // something switches back to `current_host`; by then a swapImpl on the other
  // side has already restored our register state into PPCContext.
  if (trace) {
    REXKRNL_WARN("gpf swap #{} -> SwitchTo host={} (brand_new={})", seq,
                static_cast<const void*>(target->host), brand_new);
  }
  rex::thread::Fiber::SwitchTo(target->host);
  if (trace) {
    REXKRNL_WARN("gpf swap #{} <- RESUMED on host={} lr={:#010x} r1={:#010x}", seq,
                static_cast<const void*>(current_host), static_cast<uint32_t>(ctx.lr),
                static_cast<uint32_t>(ctx.r1.u64));
  }
  return true;
}

GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc) {
  PPCFunc* fn = nullptr;
  auto* rt = rex::Runtime::instance();
  if (rt && rt->function_dispatcher()) {
    fn = rt->function_dispatcher()->GetFunction(start_pc);
  }
  if (!fn) {
    fn = rex::runtime::ResolveIndirectFunction(start_pc);
  }
  if (!fn) {
    REXKRNL_ERROR("guest_pc_fiber: no host function for resume PC {:#010x}", start_pc);
    return GuestPcRunResult{false, start_pc};
  }
  fn(ctx, base);
  return GuestPcRunResult{true, start_pc};
}

}  // namespace rex::ppc
