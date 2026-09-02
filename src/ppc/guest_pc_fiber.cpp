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
  // DIAGNOSTIC: the OS thread that created this host fiber. GuestFiberEntry binds
  // the guest PPCContext& to whatever thread first ran the fiber, so a SwitchTo
  // from any *other* thread resumes the coroutine against the wrong thread's
  // register file -> callee-saved corruption (the r20==0 multi-thread crash).
  uint32_t creator_tid = 0;
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
constexpr uint32_t kSwapLogCap = 2500;

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

uint64_t GuestLoadU64(memory::Memory* mem, uint32_t addr) {
  const uint32_t hi = GuestLoadU32(mem, addr);
  const uint32_t lo = GuestLoadU32(mem, addr + 4);
  return (uint64_t(hi) << 32) | lo;
}

void GuestStoreU32(memory::Memory* mem, uint32_t addr, uint32_t value) {
  if (!mem || addr == 0) {
    return;
  }
  auto* heap = mem->LookupHeap(addr);
  uint32_t protect = 0;
  if (!heap || !heap->QueryProtect(addr, &protect) ||
      protect == memory::kMemoryProtectNoAccess) {
    return;
  }
  *mem->TranslateVirtual<uint32_t*>(addr) = __builtin_bswap32(value);
}

bool IsConfiguredPc(const std::vector<uint32_t>& list, uint32_t pc) {
  if (pc == 0) {
    return false;
  }
  for (uint32_t entry : list) {
    if (entry == pc) {
      return true;
    }
  }
  return false;
}

void WriteTlsCurrentFiberBlock(memory::Memory* mem, uint32_t r13, uint32_t block) {
  const uint32_t tls = GuestLoadU32(mem, r13 + kTlsBaseOffset);
  if (!tls) {
    return;
  }
  GuestStoreU32(mem, tls + kTlsCurrentFiberOffset, block);
}

/// Reload outgoing callee-saved GPRs (+152..+288) and r1 (+48) from a block.
void RestoreOutgoingGprsFromBlock(PPCContext& ctx, memory::Memory* mem, uint32_t block) {
  if (!mem || block == 0) {
    return;
  }
  ctx.r1.u64 = GuestLoadU64(mem, block + 48);
  ctx.r14.u64 = GuestLoadU64(mem, block + 152);
  ctx.r15.u64 = GuestLoadU64(mem, block + 160);
  ctx.r16.u64 = GuestLoadU64(mem, block + 168);
  ctx.r17.u64 = GuestLoadU64(mem, block + 176);
  ctx.r18.u64 = GuestLoadU64(mem, block + 184);
  ctx.r19.u64 = GuestLoadU64(mem, block + 192);
  ctx.r20.u64 = GuestLoadU64(mem, block + 200);
  ctx.r21.u64 = GuestLoadU64(mem, block + 208);
  ctx.r22.u64 = GuestLoadU64(mem, block + 216);
  ctx.r23.u64 = GuestLoadU64(mem, block + 224);
  ctx.r24.u64 = GuestLoadU64(mem, block + 232);
  ctx.r25.u64 = GuestLoadU64(mem, block + 240);
  ctx.r26.u64 = GuestLoadU64(mem, block + 248);
  ctx.r27.u64 = GuestLoadU64(mem, block + 256);
  ctx.r28.u64 = GuestLoadU64(mem, block + 264);
  ctx.r29.u64 = GuestLoadU64(mem, block + 272);
  ctx.r30.u64 = GuestLoadU64(mem, block + 280);
  ctx.r31.u64 = GuestLoadU64(mem, block + 288);
}

/// Copy outgoing saved r20..r31 (+200..+288) into a zeroed target block.
void SeedTargetCalleeSavedFromSource(memory::Memory* mem, uint32_t target_block,
                                     uint32_t source_block) {
  if (!mem || target_block == 0 || source_block == 0) {
    return;
  }
  for (uint32_t off = 200; off <= 288; off += 8) {
    const uint32_t tgt_hi = GuestLoadU32(mem, target_block + off);
    const uint32_t src_hi = GuestLoadU32(mem, source_block + off);
    if (tgt_hi == 0 && src_hi != 0) {
      GuestStoreU32(mem, target_block + off, src_hi);
      GuestStoreU32(mem, target_block + off + 4, GuestLoadU32(mem, source_block + off + 4));
    }
  }
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
void EnsureSlot(uint32_t block, rex::thread::Fiber* host, bool is_thread_fiber,
                uint32_t owner_tid) {
  if (!block || !host) {
    return;
  }
  std::lock_guard<std::mutex> lk(Mtx());
  auto& slot = Map()[block];
  if (!slot) {
    slot = std::make_unique<FiberSlot>();
    slot->creator_tid = owner_tid;
  }
  slot->host = host;
  slot->block = block;
  slot->is_thread_fiber = is_thread_fiber;
}

void GuestFiberEntry(void* raw_arg);

/// Lazily create the host fiber backing a target block we have never switched
/// to before. Its entry dispatches the guest from the target's resume PC.
FiberSlot* CreateHostFiberForTarget(uint32_t block, uint32_t owner_tid) {
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
  slot->creator_tid = owner_tid;
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

  {
    const uint32_t r13 = static_cast<uint32_t>(ctx.r13.u64);
    const uint32_t cur_thread = GuestLoadU32(mem, r13 + 0x100);
    const uint32_t cur_fiber = cur_thread ? GuestLoadU32(mem, cur_thread + 0x164) : 0;
    REXKRNL_WARN(
        "gpf fiber-entry block={:#010x} resume_pc={:#010x} sp={:#010x} job@+288={:#010x} "
        "r13={:#010x} cur_thread={:#010x} cur_fiber={:#010x} block[+0]={:#010x}",
        args->block, resume_pc, static_cast<uint32_t>(ctx.r1.u64),
        GuestLoadU32(mem, args->block + 288), r13, cur_thread, cur_fiber,
        GuestLoadU32(mem, args->block + 0));
  }

  if (Config().capture_job_fn) {
    Config().capture_job_fn(ctx, base, args->block);
  }
  if (Config().before_dispatch_job) {
    Config().before_dispatch_job(ctx, base, resume_pc, static_cast<uint32_t>(ctx.r3.u64));
  }

  if (IsConfiguredPc(Config().deny_fiber_job_entries, resume_pc)) {
    auto* main = thread->main_fiber();
    if (main && main != rex::thread::Fiber::Current()) {
      REXKRNL_WARN(
          "gpf DENY-JOB fiber-entry resume_pc={:#010x} block={:#010x} -> main fiber",
          resume_pc, args->block);
      rex::thread::Fiber::SwitchTo(main);
    }
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

  // r13 is the per-OS-thread PCR pointer and MUST equal this XThread's real
  // pcr_address. If it has drifted to another value the guest swap would compute
  // its save/restore block from `[[r13+0x100]+0x164]` against a wrong/broken PCR,
  // losing the saved registers and restoring zeros (observed: callee-saved regs
  // come back 0 -> r20==0 / r31==0 null derefs downstream). Detect it (uncapped,
  // ERROR flushes) and CORRECT it back to the real PCR before swapImpl runs.
  if (thread) {
    const uint32_t real_pcr = thread->pcr_ptr();
    const uint32_t cur_r13 = static_cast<uint32_t>(ctx.r13.u64);
    if (real_pcr && cur_r13 != real_pcr) {
      REXKRNL_ERROR("gpf R13-FIX tid={} bad_r13={:#010x} -> real_pcr={:#010x} tgt(r3)={:#010x} lr={:#010x}",
                   thread->thread_id(), cur_r13, real_pcr, static_cast<uint32_t>(ctx.r3.u64),
                   static_cast<uint32_t>(ctx.lr));
      ctx.r13.u64 = real_pcr;
    }
  }

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
      const uint32_t pre_r20 = static_cast<uint32_t>(ctx.r20.u64);
      swapImpl(ctx, base);
      if (pre_r20 != 0 && ctx.r20.u64 == 0) {
        REXKRNL_ERROR(
            "gpf ZERO-RESTORE(fallback) active={} thread={} r13={:#010x} tgt(r3-was)={:#010x} "
            "pre_r20={:#010x} lr={:#010x} r1={:#010x}",
            g_active.load(std::memory_order_acquire), static_cast<const void*>(thread),
            static_cast<uint32_t>(ctx.r13.u64), static_cast<uint32_t>(ctx.r3.u64), pre_r20,
            static_cast<uint32_t>(ctx.lr), static_cast<uint32_t>(ctx.r1.u64));
      }
    }
    return false;
  }

  auto* mem = CurrentMemory(thread);

  // Snapshot the *source* fiber before swapImpl rewrites the TLS-current slot.
  const uint32_t current_block = ReadCurrentFiberBlock(ctx, mem);
  const uint32_t target_block = static_cast<uint32_t>(ctx.r3.u64);  // guest swap ABI
  const uint32_t entry_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t entry_r1 = static_cast<uint32_t>(ctx.r1.u64);

  // ALWAYS-ON (bypasses kSwapLogCap): a swap whose source fiber can't be read
  // from the guest TLS chain (current_block == 0) cannot be made resumable --
  // EnsureSlot is skipped below, so when the target later swaps back to this
  // thread there is no registered host fiber and the guest swap restores a
  // zero/garbage context (observed: r14-r31 all 0, r13 -> zeroed PCR, then
  // r20==0 deref in sub_82C9D190). Capture the full PCR/TLS chain so we can see
  // WHY the chain is broken for this thread.
  if (current_block == 0) {
    const uint32_t r13 = static_cast<uint32_t>(ctx.r13.u64);
    const uint32_t cur_thread = GuestLoadU32(mem, r13 + kTlsBaseOffset);
    const uint32_t cur_fiber =
        cur_thread ? GuestLoadU32(mem, cur_thread + kTlsCurrentFiberOffset) : 0;
    REXKRNL_ERROR(
        "gpf NULL-SRC swap: tid={} r13={:#010x} [r13+0x100](cur_thread)={:#010x} "
        "[cur_thread+0x164](cur_fiber)={:#010x} tgt_blk={:#010x} lr={:#010x} r1={:#010x} "
        "had_cur={}",
        thread->thread_id(), r13, cur_thread, cur_fiber, target_block, entry_lr, entry_r1,
        rex::thread::Fiber::Current() != nullptr);
  }

  rex::thread::Fiber* current_host = rex::thread::Fiber::Current();
  const bool had_current = current_host != nullptr;
  if (!current_host) {
    // The thread is NOT in fiber mode (Fiber::Current()/tls_current_ == null).
    // Fiber::SwitchTo unconditionally dereferences the *current* fiber to save
    // its context (swapcontext(&from->context_, ...)), so switching from a bare
    // thread stack would deref null -> the brand-new target fiber's entry never
    // runs and the guest job the caller is waiting on never signals -> hang
    // (observed as the "Saving content" stall: a save job re-entered the
    // scheduler via sub_8310C640 from an unconverted thread base). The Fiber API
    // contract is "ConvertCurrentThread() must be called once before any
    // SwitchTo" -- honour it here. We adopt the converted fiber as the thread's
    // main fiber so the completion path (GuestFiberEntry -> SwitchTo(main)) and
    // the source-block slot below resume back onto this exact context.
    // See FixesThatNeedImplemented.md #7.
    current_host = rex::thread::Fiber::ConvertCurrentThread();
    if (!current_host) {
      REXKRNL_ERROR("gpf: ConvertCurrentThread failed; falling back to inline swap");
      swapImpl(ctx, base);
      return false;
    }
    thread->set_main_fiber(current_host);
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
    EnsureSlot(current_block, current_host, current_host == thread->main_fiber(),
               thread->thread_id());
  }

  const bool preserve_zero_restore =
      IsConfiguredPc(Config().zero_restore_preserve_entry_lrs, entry_lr);
  const bool target_had_zero_blk =
      target_block != 0 && GuestLoadU32(mem, target_block + 200) == 0;
  if (preserve_zero_restore && current_block && target_block) {
    const uint32_t tgt_r20 = GuestLoadU32(mem, target_block + 200);
    const uint32_t cur_r20 = GuestLoadU32(mem, current_block + 200);
    if (tgt_r20 == 0 && cur_r20 != 0) {
      SeedTargetCalleeSavedFromSource(mem, target_block, current_block);
      if (trace) {
        REXKRNL_WARN(
            "gpf PRESERVE-SEED #{} entry_lr={:#010x} cur_blk={:#010x} tgt_blk={:#010x} "
            "blk@+200={:#010x} job@+288={:#010x}",
            seq, entry_lr, current_block, target_block,
            GuestLoadU32(mem, target_block + 200), GuestLoadU32(mem, target_block + 288));
      }
    }
  }

  // Reuse the guest's own register save/restore: saves the current registers
  // into the current context block and loads the target's into PPCContext,
  // setting ctx.lr = target resume PC, ctx.r1 = target SP, the TLS-current
  // slot, and the KTHREAD/PCR stack fields. After this, ctx == target state.
  const uint32_t pre_r20 = static_cast<uint32_t>(ctx.r20.u64);
  swapImpl(ctx, base);

  // ALWAYS-ON (bypasses kSwapLogCap, ERROR forces a log flush): catch the
  // deterministic "restored a ZERO context" corruption (r14/r20/r31 all 0 after
  // the restore) that surfaces downstream as r20==0 in sub_82C9D190. Dump the
  // source/target blocks and the bytes the guest swap restored r20 from
  // ([target_block+200]) plus what it saved for the source ([current_block+200]),
  // so we can tell whether the target block was never populated or the source
  // was saved to the wrong block.
  if (pre_r20 != 0 && ctx.r20.u64 == 0) {
    REXKRNL_ERROR(
        "gpf ZERO-RESTORE #{} tid={} r13={:#010x} cur_blk={:#010x} tgt_blk={:#010x} pre_r20={:#010x} "
        "post(r14={:#010x} r31={:#010x} lr={:#010x} r1={:#010x}) tgt[+200]={:#010x} cur[+200]={:#010x} "
        "entry_lr={:#010x}",
        seq, thread->thread_id(), static_cast<uint32_t>(ctx.r13.u64), current_block, target_block,
        pre_r20, static_cast<uint32_t>(ctx.r14.u64), static_cast<uint32_t>(ctx.r31.u64),
        static_cast<uint32_t>(ctx.lr), static_cast<uint32_t>(ctx.r1.u64),
        GuestLoadU32(mem, target_block + 204), GuestLoadU32(mem, current_block + 204), entry_lr);
    if (preserve_zero_restore && current_block) {
      RestoreOutgoingGprsFromBlock(ctx, mem, current_block);
      WriteTlsCurrentFiberBlock(mem, static_cast<uint32_t>(ctx.r13.u64), current_block);
      REXKRNL_WARN(
          "gpf ZERO-RESTORE-PRESERVE #{} entry_lr={:#010x} cur_blk={:#010x} "
          "blk@+200={:#010x} job@+288={:#010x} r1={:#010x}",
          seq, entry_lr, current_block, static_cast<uint32_t>(ctx.r20.u64),
          static_cast<uint32_t>(ctx.r31.u64), static_cast<uint32_t>(ctx.r1.u64));
    }
  }

  FiberSlot* target = LookupSlot(target_block);
  bool brand_new = (target == nullptr);

  // TRAMPOLINE-RESUME GUARD (the missed stale-block variant behind the recurring
  // r31==0 / broken-r13 crash): swapImpl just restored ctx.lr from the target
  // block. If that resume PC is the fiber-start trampoline, the guest has
  // (re)initialised this block as a FRESH fiber (block+28 = trampoline). A live,
  // already-running fiber would resume at its yield PC (a swap point), never the
  // trampoline. So any pre-existing Map() entry for this block is STALE -- the
  // guest recycled the block address from an earlier fiber. The old host fiber
  // is suspended deep in that earlier fiber's call chain; SwitchTo-ing it would
  // resume that OLD code with the freshly-restored (mostly-zero) register file,
  // zeroing non-volatiles (r31 'this' == 0) and cascading into broken PCR/r13.
  // Drop the stale slot and let CreateHostFiberForTarget spin up a fresh host
  // that runs the trampoline cleanly. This generalises the host==current_host
  // guard below, which only caught the same-host alias (a different host slips
  // through: observed swap #631 brand_new=false resume_pc=0x82c7a450).
  const uint32_t resume_pc_now = static_cast<uint32_t>(ctx.lr);
  const uint32_t fiber_start_pc = Config().fiber_start_pc;
  if (!brand_new && fiber_start_pc != 0 && resume_pc_now == fiber_start_pc) {
    if (trace) {
      REXKRNL_WARN(
          "gpf swap #{} STALE recycled block (trampoline-resume pc={:#010x}) tgt={:#010x} "
          "old_host={} cur_host={} -> recreate",
          seq, resume_pc_now, target_block, static_cast<const void*>(target->host),
          static_cast<const void*>(current_host));
    }
    brand_new = true;
    target = nullptr;
  }

  // STALE-ENTRY GUARD (root cause of the deterministic r20==0 crash in
  // sub_82C9D190): a target whose mapped host is the CURRENT host but whose
  // block differs from current_block is NOT a live fiber on that host -- the
  // host has already moved on to executing current_block's call stack. The guest
  // recycled this block address for a *new* fiber (its just-restored context is
  // freshly zero -> resume PC is the fiber-start trampoline). The old "no-switch
  // -> inline continue" path would then run that zeroed context on the wrong
  // host stack and return up the caller (sub_82C9EF18 -> sub_82C9D190) with a
  // wiped register file. Treat it as brand-new: drop the stale slot and spin up
  // a fresh host fiber that runs the block cleanly from its resume PC.
  if (!brand_new && target_block != current_block && target->host == current_host) {
    if (trace) {
      REXKRNL_WARN("gpf swap #{} STALE recycled block tgt={:#010x} (cur={:#010x}) host={} -> recreate",
                  seq, target_block, current_block, static_cast<const void*>(current_host));
    }
    brand_new = true;
    target = nullptr;
  }

  if (trace) {
    REXKRNL_WARN("gpf swap #{} post-swap resume_pc={:#010x} tgt_sp={:#010x} brand_new={}", seq,
                static_cast<uint32_t>(ctx.lr), static_cast<uint32_t>(ctx.r1.u64), brand_new);
  }

  const bool deny_inline_swap =
      IsConfiguredPc(Config().deny_fiber_swap_entry_lrs, entry_lr) && fiber_start_pc != 0 &&
      resume_pc_now == fiber_start_pc && target_had_zero_blk;
  if (deny_inline_swap) {
    if (current_block) {
      RestoreOutgoingGprsFromBlock(ctx, mem, current_block);
      WriteTlsCurrentFiberBlock(mem, static_cast<uint32_t>(ctx.r13.u64), current_block);
    }
    REXKRNL_WARN(
        "gpf DENY-INLINE-SWAP #{} entry_lr={:#010x} tgt_blk={:#010x} resume_pc={:#010x} "
        "blk@+200={:#010x} job@+288={:#010x}",
        seq, entry_lr, target_block, resume_pc_now, static_cast<uint32_t>(ctx.r20.u64),
        static_cast<uint32_t>(ctx.r31.u64));
    return false;
  }

  if (IsConfiguredPc(Config().deny_fiber_job_entries, resume_pc_now)) {
    REXKRNL_WARN("gpf DENY-JOB inline swap #{} resume_pc={:#010x} tgt_blk={:#010x}", seq,
                resume_pc_now, target_block);
    RunGuestPc(ctx, base, resume_pc_now);
    if (current_block) {
      RestoreOutgoingGprsFromBlock(ctx, mem, current_block);
      WriteTlsCurrentFiberBlock(mem, static_cast<uint32_t>(ctx.r13.u64), current_block);
    }
    return false;
  }

  if (brand_new) {
    target = CreateHostFiberForTarget(target_block, thread->thread_id());
    if (!target) {
      // Cannot back the target with a host fiber -- fall through and let the
      // guest continue inline on the current host stack (best effort).
      if (trace) REXKRNL_ERROR("gpf swap #{} CreateHostFiber FAILED -> inline", seq);
      return false;
    }
  }

  if (!target->host || target->host == current_host) {
    // Reached only for a genuine self-swap (target_block == current_block): the
    // stale-entry guard above already redirected recycled-block aliases to a
    // fresh host. swapImpl restored our own saved state, so continue inline.
    if (trace) {
      REXKRNL_WARN("gpf swap #{} self-swap/no-switch (host={} cur_host={}) -> inline continue", seq,
                  static_cast<const void*>(target->host), static_cast<const void*>(current_host));
    }
    return false;  // already on the right host stack
  }

  // Hand the host stack to the target fiber. Control returns here only when
  // something switches back to `current_host`; by then a swapImpl on the other
  // side has already restored our register state into PPCContext.
  //
  // ALWAYS-ON (bypasses kSwapLogCap): if we are about to resume a fiber that was
  // created on a DIFFERENT OS thread, GuestFiberEntry bound its PPCContext& to
  // that other thread -- resuming it here runs the coroutine against the wrong
  // thread's register file. This is the suspected root of the r20==0 multi-thread
  // crash. Log every occurrence so we can confirm it correlates with the fault.
  if (!brand_new && target->creator_tid != 0 &&
      target->creator_tid != thread->thread_id()) {
    REXKRNL_ERROR(
        "gpf CROSS-THREAD switch #{}: tid={} resuming host={} created by tid={} "
        "(tgt_blk={:#010x} cur_blk={:#010x} is_thread_fiber={})",
        seq, thread->thread_id(), static_cast<const void*>(target->host),
        target->creator_tid, target_block, current_block, target->is_thread_fiber);
  }
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
