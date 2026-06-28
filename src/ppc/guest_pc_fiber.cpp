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

// TEMP_DIAG: full-run compact fiber-swap trace (one line per swap) to diff the
// fiber execution order between a GOOD and a BAD (black) run and find which
// fiber's activation diverges. resume_pc is the stable cross-run fiber identity.
// High cap so the post-boot menu transition (far past swap #600) is captured.
std::atomic<uint32_t> g_swap_seq{0};
constexpr uint32_t kSwapLogCap = 2000000;

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

  rex::thread::Fiber* current_host = rex::thread::Fiber::Current();
  if (!current_host) {
    current_host = thread->main_fiber();
  }

  const uint32_t seq = g_swap_seq.fetch_add(1, std::memory_order_relaxed);

  // Make the source fiber resumable: register its host fiber under its block so
  // a later swap back to `current_block` finds this host stack.
  if (current_block) {
    EnsureSlot(current_block, current_host, current_host == thread->main_fiber());
  }

  // Preserve the caller (yielding) fiber's non-volatile register set across the
  // host fiber switch. The recompiled guest context switch (swapImpl) does not
  // reliably reload r14-r31 / f14-f31 / v14-v31 on swap-back, so the resuming
  // fiber can come back with stale callee-saved registers -- e.g. a stale
  // pointer in r31 that faults when a menu/island worker fiber resumes. We
  // snapshot them here and restore them on resume (only on the real switch-back
  // path below). This is the uniform runtime replacement for the per-LR
  // doax_hooks GPR band-aid (NeedsFiberCalleeSavePreserve).
  alignas(16) uint8_t saved_nonvolatiles[PPCContext::kNonVolatileSaveSize];
  ctx.SaveNonVolatiles(saved_nonvolatiles);

  // Reuse the guest's own register save/restore: saves the current registers
  // into the current context block and loads the target's into PPCContext,
  // setting ctx.lr = target resume PC, ctx.r1 = target SP, the TLS-current
  // slot, and the KTHREAD/PCR stack fields. After this, ctx == target state.
  swapImpl(ctx, base);

  FiberSlot* target = LookupSlot(target_block);
  const bool brand_new = (target == nullptr);
  if (brand_new) {
    target = CreateHostFiberForTarget(target_block);
  }
  const bool will_switch = target && target->host && target->host != current_host;

  if (!target) {
    // CreateHostFiberForTarget failed: continue inline on the current host stack.
    REXKRNL_ERROR("FIBERSW #{} CreateHostFiber FAILED tgt={:08X} -> inline", seq, target_block);
    return false;
  }
  if (!will_switch) {
    return false;  // already on the right host stack
  }

  // Hand the host stack to the target fiber. Control returns here only when
  // something switches back to `current_host`; by then a swapImpl on the other
  // side has already restored our register state into PPCContext.
  rex::thread::Fiber::SwitchTo(target->host);
  // Resumed on this fiber's host stack. The swap-back's recompiled context
  // switch may not have reloaded our non-volatiles -- force them back to the
  // values this fiber had when it yielded. (Volatile regs incl. ctx.lr/r1 were
  // correctly restored by swapImpl and are intentionally left untouched.)
  ctx.RestoreNonVolatiles(saved_nonvolatiles);
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
