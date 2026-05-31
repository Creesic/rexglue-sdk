// Guest-PC fiber interpreter (SDK).
//
// Cooperative fiber swaps (KeSet + blr) resume at PCs that are not function entries.
// Static recomp turns blr into C++ return; this module drives guest PC through fiber
// regions with optional title callbacks from GuestPcFiberConfig.

#include <rex/ppc/guest_pc_fiber.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <vector>

#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/ppc/static_recomp_fiber.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/xthread.h>

namespace rex::ppc {
namespace {

GuestPcFiberConfig g_config{};

constexpr int kMaxRunDepth = 32;
constexpr uint32_t kMaxGuestPcSteps = 4 * 1024 * 1024u;

struct FiberFrame {
  uint32_t fiber_slot_at_entry = 0;
  uint32_t job_ctx = 0;
  uint32_t job_fn_at_entry = 0;
};

struct InterpreterTls {
  bool swap_pending = false;
  int run_depth = 0;
  bool keset_to_fiber = false;
  bool keset_swap_back = false;
  bool host_boundary_resume_handled = false;
  bool host_boundary_swap_back = false;
  uint32_t last_stack_limit = 0;
  uint32_t prev_stack_limit = 0;
  std::vector<FiberFrame> frames;
};

thread_local InterpreterTls g_tls;

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address) {
  if (guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return 0;
  }
  uint32_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(guest_address), sizeof(be));
  return __builtin_bswap32(be);
}

void StoreGuestU32(rex::memory::Memory* memory, uint32_t guest_address, uint32_t value) {
  if (guest_address == 0 || !memory->IsGuestVirtualCommitted(guest_address)) {
    return;
  }
  const uint32_t be = __builtin_bswap32(value);
  std::memcpy(memory->TranslateVirtual(guest_address), &be, sizeof(be));
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

uint32_t ResolveTlsThreadLocalState(rex::memory::Memory* memory, const PPCContext& ctx) {
  if (!GuestRangeReadable(memory, ctx.r13.u32 + 256, 4)) {
    return 0;
  }
  return LoadGuestU32(memory, ctx.r13.u32 + 256);
}

bool IsFiberGuestRegion(uint32_t pc) {
  for (const auto& region : g_config.guest_regions) {
    if (pc >= region.lo && pc < region.hi) {
      return true;
    }
  }
  return false;
}

void SyncHostStackSpanFromGuest() {
  auto* host = rex::system::XThread::TryGetCurrentThread();
  if (!host) {
    return;
  }
  auto* kthread = host->guest_object<rex::system::X_KTHREAD>();
  if (!kthread) {
    return;
  }
  const uint32_t limit = static_cast<uint32_t>(kthread->stack_limit);
  const uint32_t base = static_cast<uint32_t>(kthread->stack_base);
  if (limit != 0 && base > limit &&
      (host->stack_limit() != limit || host->stack_base() != base)) {
    host->SetGuestStackSpan(limit, base);
  }
}

bool IsOnFiberStack(const PPCContext& ctx) {
  const uint32_t sp = static_cast<uint32_t>(ctx.r1.u64);
  if (rex::ppc::IsGuestSpOnWorkQueueFiberStack(sp)) {
    return true;
  }
  auto* host = rex::system::XThread::TryGetCurrentThread();
  return host != nullptr &&
         rex::ppc::IsWorkQueueFiberGuestStack(host->stack_limit());
}

uint32_t NormalizeFiberResumePc(PPCContext& ctx, uint32_t resume_pc) {
  for (const auto& rewrite : g_config.resume_rewrites) {
    if (resume_pc != rewrite.from_pc) {
      continue;
    }
    if (rewrite.only_on_fiber_stack && !IsOnFiberStack(ctx)) {
      continue;
    }
    static std::atomic<uint32_t> rewrite_log{0};
    if (rewrite_log.fetch_add(1, std::memory_order_relaxed) < 12) {
      REXSYS_WARN(
          "Guest-PC fiber: rewrite fiber resume 0x{:08X} -> 0x{:08X}",
          resume_pc, rewrite.to_pc);
    }
    ctx.lr = rewrite.to_pc;
    return rewrite.to_pc;
  }
  return resume_pc;
}

PPCFunc* LookupGuestFunction(uint8_t* base, uint32_t guest_address);
void CallGuestFunction(PPCContext& ctx, uint8_t* base, uint32_t guest_address);

bool IsPlausibleGuestCodePointer(uint8_t* base, uint32_t addr) {
  (void)base;
  for (const auto& site : g_config.native_sites) {
    if (site.pc == addr) {
      return false;
    }
  }
  return rex::runtime::ResolveIndirectFunction(addr) != nullptr;
}

uint32_t CaptureJobFunction(PPCContext& ctx, uint8_t* base, rex::memory::Memory* memory,
                            uint32_t job_ctx) {
  if (g_config.capture_job_fn) {
    const uint32_t captured = g_config.capture_job_fn(ctx, base, job_ctx);
    if (captured != 0) {
      return captured;
    }
  }

  const uint32_t from_r31 = static_cast<uint32_t>(ctx.r31.u64);
  if (IsPlausibleGuestCodePointer(base, from_r31)) {
    return from_r31;
  }

  if (job_ctx != 0 && GuestRangeReadable(memory, job_ctx + 288, 4)) {
    const uint32_t from_ctx = LoadGuestU32(memory, job_ctx + 288);
    if (IsPlausibleGuestCodePointer(base, from_ctx)) {
      ctx.r31.u64 = from_ctx;
      return from_ctx;
    }
  }

  static std::atomic<uint32_t> fail_log{0};
  if (fail_log.fetch_add(1, std::memory_order_relaxed) < 12) {
    REXSYS_WARN(
        "Guest-PC fiber: no valid job fn (r31=0x{:08X} job_ctx=0x{:08X})",
        from_r31, job_ctx);
  }
  return 0;
}

uint32_t ResolveFiberJobArgument(rex::memory::Memory* memory, const PPCContext& ctx,
                                 uint32_t job_fn) {
  if (g_config.resolve_job_arg) {
    uint8_t* base = memory ? memory->virtual_membase() : nullptr;
    PPCContext mutable_ctx = ctx;
    return g_config.resolve_job_arg(mutable_ctx, base, job_fn);
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
  return from_ctx;
}

void PopFiberFrame(const char* reason) {
  if (g_tls.frames.empty()) {
    return;
  }
  static std::atomic<uint32_t> pop_log{0};
  if (pop_log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN("Guest-PC fiber: pop frame ({}), depth {} -> {}",
                reason, g_tls.frames.size(), g_tls.frames.size() - 1);
  }
  g_tls.frames.pop_back();
}

void DrainFiberFramesOnThreadStack(const PPCContext& ctx, const char* reason) {
  if (IsOnFiberStack(ctx) || g_tls.frames.empty()) {
    return;
  }
  const size_t depth = g_tls.frames.size();
  g_tls.frames.clear();
  static std::atomic<uint32_t> drain_log{0};
  if (drain_log.fetch_add(1, std::memory_order_relaxed) < 8) {
    REXSYS_WARN("Guest-PC fiber: drained {} stale frame(s) on thread stack ({})",
                depth, reason);
  }
}

// Each RunFiberSwap push must be paired with exactly one pop before returning to
// the caller. Nested BC88→ED910 cycles inside BEC8 must not leave frames behind
// when RunFiberSwap returns — even if guest r1 is still on a fiber stack.
void FinishRunFiberSwap(size_t depth_at_entry, const char* reason) {
  while (g_tls.frames.size() > depth_at_entry) {
    PopFiberFrame(reason);
  }
}

void ReconcileFiberFrameStackImpl(const PPCContext& ctx) {
  if (g_tls.run_depth != 0 || g_tls.swap_pending || IsOnFiberStack(ctx) ||
      g_tls.frames.empty() || rex::ppc::GetFiberJobDispatchDepth() > 0) {
    return;
  }
  DrainFiberFramesOnThreadStack(ctx, "reconcile idle on thread stack");
}

bool DispatchFiberJobLikeEbea0(PPCContext& ctx, uint8_t* base, rex::memory::Memory* memory,
                               uint32_t job_fn) {
  if (!IsPlausibleGuestCodePointer(base, job_fn)) {
    return false;
  }

  const uint32_t job_arg = ResolveFiberJobArgument(memory, ctx, job_fn);

  ctx.r31.u64 = job_fn;
  ctx.r3.u32 = job_arg;
  if (g_config.before_dispatch_job) {
    g_config.before_dispatch_job(ctx, base, job_fn, job_arg);
  }
  CallGuestFunction(ctx, base, job_fn);
  return true;
}

PPCFunc* LookupGuestFunction(uint8_t* base, uint32_t guest_address) {
  (void)base;
  for (const auto& site : g_config.native_sites) {
    if (site.pc == guest_address) {
      return nullptr;
    }
  }
  return rex::runtime::ResolveIndirectFunction(guest_address);
}

void CallGuestFunction(PPCContext& ctx, uint8_t* base, uint32_t guest_address) {
  PPCFunc* fn = LookupGuestFunction(base, guest_address);
  if (!fn) {
    ctx.last_indirect_target = guest_address;
    fn = rex::runtime::ResolveIndirectFunction(guest_address);
  }
  fn(ctx, base);
}

bool RunSwapBackTail(PPCContext& ctx, uint8_t* base, rex::memory::Memory* memory,
                     uint32_t saved_thread_ctx) {
  (void)memory;
  if (!g_config.run_swap_back_tail) {
    return false;
  }
  return g_config.run_swap_back_tail(ctx, base, saved_thread_ctx);
}

void OnKeSetNotify(uint32_t stack_limit) {
  if (!g_tls.swap_pending) {
    return;
  }

  const bool on_fiber = rex::ppc::IsWorkQueueFiberGuestStack(stack_limit);
  const bool was_fiber = rex::ppc::IsWorkQueueFiberGuestStack(g_tls.prev_stack_limit);
  const bool swap_back = !on_fiber && was_fiber;

  g_tls.last_stack_limit = stack_limit;
  g_tls.prev_stack_limit = stack_limit;
  g_tls.keset_to_fiber = on_fiber && !swap_back;
  g_tls.keset_swap_back = swap_back;

  static std::atomic<uint32_t> count{0};
  if (count.fetch_add(1, std::memory_order_relaxed) < 12) {
    REXSYS_WARN(
        "Guest-PC fiber: KeSet limit=0x{:08X} to_fiber={} swap_back={} "
        "frames={} sdk_depth={}",
        stack_limit, g_tls.keset_to_fiber, g_tls.keset_swap_back,
        g_tls.frames.size(), rex::ppc::GetFiberJobDispatchDepth());
  }
}

uint32_t RegNum(uint32_t field) { return field & 31u; }

uint32_t& Gpr(PPCContext& ctx, uint32_t reg) {
  switch (reg) {
    case 0:
      return ctx.r0.u32;
    case 1:
      return ctx.r1.u32;
    case 2:
      return ctx.r2.u32;
    case 3:
      return ctx.r3.u32;
    case 4:
      return ctx.r4.u32;
    case 5:
      return ctx.r5.u32;
    case 6:
      return ctx.r6.u32;
    case 7:
      return ctx.r7.u32;
    case 8:
      return ctx.r8.u32;
    case 9:
      return ctx.r9.u32;
    case 10:
      return ctx.r10.u32;
    case 11:
      return ctx.r11.u32;
    case 12:
      return ctx.r12.u32;
    case 13:
      return ctx.r13.u32;
    case 14:
      return ctx.r14.u32;
    case 15:
      return ctx.r15.u32;
    case 16:
      return ctx.r16.u32;
    case 17:
      return ctx.r17.u32;
    case 18:
      return ctx.r18.u32;
    case 19:
      return ctx.r19.u32;
    case 20:
      return ctx.r20.u32;
    case 21:
      return ctx.r21.u32;
    case 22:
      return ctx.r22.u32;
    case 23:
      return ctx.r23.u32;
    case 24:
      return ctx.r24.u32;
    case 25:
      return ctx.r25.u32;
    case 26:
      return ctx.r26.u32;
    case 27:
      return ctx.r27.u32;
    case 28:
      return ctx.r28.u32;
    case 29:
      return ctx.r29.u32;
    case 30:
      return ctx.r30.u32;
    case 31:
      return ctx.r31.u32;
    default:
      return ctx.r0.u32;
  }
}

uint64_t& Gpr64(PPCContext& ctx, uint32_t reg) {
  switch (reg) {
    case 0:
      return ctx.r0.u64;
    case 1:
      return ctx.r1.u64;
    case 2:
      return ctx.r2.u64;
    case 3:
      return ctx.r3.u64;
    case 4:
      return ctx.r4.u64;
    case 5:
      return ctx.r5.u64;
    case 6:
      return ctx.r6.u64;
    case 7:
      return ctx.r7.u64;
    case 8:
      return ctx.r8.u64;
    case 9:
      return ctx.r9.u64;
    case 10:
      return ctx.r10.u64;
    case 11:
      return ctx.r11.u64;
    case 12:
      return ctx.r12.u64;
    case 13:
      return ctx.r13.u64;
    case 14:
      return ctx.r14.u64;
    case 15:
      return ctx.r15.u64;
    case 16:
      return ctx.r16.u64;
    case 17:
      return ctx.r17.u64;
    case 18:
      return ctx.r18.u64;
    case 19:
      return ctx.r19.u64;
    case 20:
      return ctx.r20.u64;
    case 21:
      return ctx.r21.u64;
    case 22:
      return ctx.r22.u64;
    case 23:
      return ctx.r23.u64;
    case 24:
      return ctx.r24.u64;
    case 25:
      return ctx.r25.u64;
    case 26:
      return ctx.r26.u64;
    case 27:
      return ctx.r27.u64;
    case 28:
      return ctx.r28.u64;
    case 29:
      return ctx.r29.u64;
    case 30:
      return ctx.r30.u64;
    case 31:
      return ctx.r31.u64;
    default:
      return ctx.r0.u64;
  }
}

bool EvalBranchCondition(PPCContext& ctx, uint32_t bo, uint32_t bi) {
  const uint32_t cond = (ctx.cr0.lt ? 0x8u : 0u) | (ctx.cr0.gt ? 0x4u : 0u) |
                        (ctx.cr0.eq ? 0x2u : 0u) | (ctx.cr0.so ? 0x1u : 0u);
  const bool ctr_ok = (bo & 0x4) != 0;
  const bool cond_ok = (bo & 0x10) != 0 || (((cond >> (31u - bi)) & 1u) != 0);
  if (!ctr_ok) {
    ctx.ctr.u64 = ctx.ctr.u64 - 1;
    if (ctx.ctr.u64 != 0) {
      return cond_ok;
    }
    return (bo & 0x2) != 0;
  }
  return cond_ok;
}

enum class StepOutcome {
  Continue,
  ReturnToHost,
  Error,
};

struct StepResult {
  StepOutcome outcome = StepOutcome::Error;
  uint32_t next_pc = 0;
};

StepResult StepGuestInstruction(PPCContext& ctx, uint8_t* base, rex::memory::Memory* memory,
                                uint32_t pc) {
#define REX_PHYS_HOST_OFFSET_SDK(addr) \
  (((static_cast<uint32_t>(addr)) >= 0xE0000000u) ? 0x1000u : 0u)
#define REX_LOAD_U8(x) \
  (*(volatile uint8_t*)(base + (static_cast<uint32_t>(x)) + REX_PHYS_HOST_OFFSET_SDK(x)))
#define REX_LOAD_U32(x) \
  __builtin_bswap32(*(volatile uint32_t*)(base + (static_cast<uint32_t>(x)) + \
                                          REX_PHYS_HOST_OFFSET_SDK(x)))
#define REX_LOAD_U64(x) \
  __builtin_bswap64(*(volatile uint64_t*)(base + (static_cast<uint32_t>(x)) + \
                                          REX_PHYS_HOST_OFFSET_SDK(x)))
#define REX_STORE_U8(x, y) \
  (*(volatile uint8_t*)(base + (static_cast<uint32_t>(x)) + REX_PHYS_HOST_OFFSET_SDK(x)) = (y))
#define REX_STORE_U32(x, y) \
  (*(volatile uint32_t*)(base + (static_cast<uint32_t>(x)) + REX_PHYS_HOST_OFFSET_SDK(x)) = \
       __builtin_bswap32(y))
#define REX_STORE_U64(x, y) \
  (*(volatile uint64_t*)(base + (static_cast<uint32_t>(x)) + REX_PHYS_HOST_OFFSET_SDK(x)) = \
       __builtin_bswap64(y))

  if (!GuestRangeReadable(memory, pc, 4)) {
    return {StepOutcome::Error, pc};
  }

  uint32_t insn = 0;
  std::memcpy(&insn, memory->TranslateVirtual(pc), sizeof(insn));
  insn = __builtin_bswap32(insn);

  const uint32_t op = insn >> 26;
  const uint32_t rd = RegNum(insn >> 21);
  const uint32_t ra = RegNum(insn >> 16);
  const uint32_t rb = RegNum(insn >> 11);
  const uint32_t rs = RegNum(insn >> 21);
  const uint32_t next = pc + 4;

  switch (op) {
    case 18: {
      const bool aa = (insn >> 1) & 1;
      const bool lk = insn & 1;
      int32_t li = static_cast<int32_t>(insn & 0x03FFFFFCu);
      if (li & 0x02000000) {
        li |= static_cast<int32_t>(0xFC000000u);
      }
      const uint32_t target = aa ? static_cast<uint32_t>(li) : static_cast<uint32_t>(pc + li);
      if (lk) {
        ctx.lr = next;
        CallGuestFunction(ctx, base, target);
        return {StepOutcome::Continue, static_cast<uint32_t>(ctx.lr)};
      }
      return {StepOutcome::Continue, target};
    }
    case 16: {
      const uint32_t bo = (insn >> 21) & 0x1F;
      const uint32_t bi = (insn >> 16) & 0x1F;
      int32_t bd = static_cast<int16_t>(insn & 0xFFFCu);
      if (EvalBranchCondition(ctx, bo, bi)) {
        return {StepOutcome::Continue, static_cast<uint32_t>(pc + bd)};
      }
      return {StepOutcome::Continue, next};
    }
    case 19: {
      const uint32_t xo = (insn >> 1) & 0x3FF;
      if (xo == 16) {
        if (insn == 0x4E800020u) {
          return {StepOutcome::ReturnToHost, static_cast<uint32_t>(ctx.lr)};
        }
        const uint32_t bo = (insn >> 21) & 0x1F;
        const uint32_t bi = (insn >> 16) & 0x1F;
        const bool lk = insn & 1;
        if (EvalBranchCondition(ctx, bo, bi)) {
          const uint32_t target = static_cast<uint32_t>(ctx.lr);
          if (lk) {
            ctx.lr = next;
            CallGuestFunction(ctx, base, target);
            return {StepOutcome::Continue, static_cast<uint32_t>(ctx.lr)};
          }
          return {StepOutcome::Continue, target};
        }
        return {StepOutcome::Continue, next};
      }
      if (xo == 528) {
        const uint32_t bo = (insn >> 21) & 0x1F;
        const uint32_t bi = (insn >> 16) & 0x1F;
        const bool lk = insn & 1;
        if (EvalBranchCondition(ctx, bo, bi)) {
          const uint32_t target = ctx.ctr.u32;
          if (lk) {
            ctx.lr = next;
            CallGuestFunction(ctx, base, target);
            return {StepOutcome::Continue, static_cast<uint32_t>(ctx.lr)};
          }
          return {StepOutcome::Continue, target};
        }
        return {StepOutcome::Continue, next};
      }
      if (xo == 339 || xo == 467) {
        const uint32_t spr = ((insn >> 16) & 0x1F) | ((insn >> 6) & 0x3E0);
        if (xo == 339) {
          if (spr == 8) {
            Gpr(ctx, rd) = static_cast<uint32_t>(ctx.lr);
          } else if (spr == 9) {
            Gpr(ctx, rd) = ctx.ctr.u32;
          }
        } else {
          if (spr == 8) {
            ctx.lr = Gpr(ctx, rd);
          } else if (spr == 9) {
            ctx.ctr.u64 = Gpr(ctx, rd);
          }
        }
        return {StepOutcome::Continue, next};
      }
      break;
    }
    case 14: {
      const int32_t si = static_cast<int16_t>(insn & 0xFFFF);
      if (ra == 0) {
        Gpr(ctx, rd) = static_cast<uint32_t>(si);
      } else {
        Gpr(ctx, rd) = Gpr(ctx, ra) + si;
      }
      return {StepOutcome::Continue, next};
    }
    case 15: {
      const int32_t si = static_cast<int16_t>(insn & 0xFFFF);
      if (ra == 0) {
        Gpr(ctx, rd) = static_cast<uint32_t>(si << 16);
      } else {
        Gpr(ctx, rd) = Gpr(ctx, ra) + (si << 16);
      }
      return {StepOutcome::Continue, next};
    }
    case 24: {
      const uint32_t ui = insn & 0xFFFF;
      Gpr(ctx, ra) = Gpr(ctx, rd) | ui;
      return {StepOutcome::Continue, next};
    }
    case 31: {
      const uint32_t xo = (insn >> 1) & 0x3FF;
      switch (xo) {
        case 444:
          Gpr(ctx, ra) = Gpr(ctx, rs);
          return {StepOutcome::Continue, next};
        case 124:
          REX_STORE_U32(Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF), Gpr(ctx, rd));
          return {StepOutcome::Continue, next};
        case 23:
          Gpr(ctx, rd) = REX_LOAD_U32(Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF));
          return {StepOutcome::Continue, next};
        case 101:
          Gpr(ctx, rd) = REX_LOAD_U8(Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF));
          return {StepOutcome::Continue, next};
        case 792:
          REX_STORE_U8(Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF),
                       static_cast<uint8_t>(Gpr(ctx, rd)));
          return {StepOutcome::Continue, next};
        case 149: {
          const uint32_t ea = Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF);
          REX_STORE_U64(ea, Gpr64(ctx, rd));
          return {StepOutcome::Continue, next};
        }
        case 21: {
          const uint32_t ea = Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF);
          Gpr64(ctx, rd) = REX_LOAD_U64(ea);
          return {StepOutcome::Continue, next};
        }
        case 971: {
          Gpr(ctx, rd) = Gpr(ctx, ra) + static_cast<int32_t>(insn & 0xFFFF);
          REX_STORE_U32(Gpr(ctx, rd), Gpr(ctx, ra));
          ctx.r1.u32 = Gpr(ctx, rd);
          return {StepOutcome::Continue, next};
        }
        case 266:
          Gpr(ctx, rd) = Gpr(ctx, ra) + Gpr(ctx, rb);
          return {StepOutcome::Continue, next};
        case 40:
          Gpr(ctx, rd) = Gpr(ctx, rb) - Gpr(ctx, ra);
          return {StepOutcome::Continue, next};
        case 26:
          Gpr(ctx, ra) = Gpr(ctx, rd) == 0 ? 32u : __builtin_clz(Gpr(ctx, rd));
          return {StepOutcome::Continue, next};
        case 24:
          Gpr(ctx, ra) = Gpr(ctx, rd) << rb;
          return {StepOutcome::Continue, next};
        case 536:
          Gpr(ctx, ra) = Gpr(ctx, rd) >> rb;
          return {StepOutcome::Continue, next};
        case 60:
          Gpr(ctx, ra) = Gpr(ctx, rd) & Gpr(ctx, rb);
          return {StepOutcome::Continue, next};
        default:
          break;
      }
      break;
    }
    case 11: {
      const int32_t si = static_cast<int16_t>(insn & 0xFFFF);
      ctx.cr0.compare<int32_t>(static_cast<int32_t>(Gpr(ctx, rd)), si, ctx.xer);
      return {StepOutcome::Continue, next};
    }
    case 10: {
      const uint32_t ui = insn & 0xFFFF;
      ctx.cr0.compare<uint32_t>(Gpr(ctx, rd), ui, ctx.xer);
      return {StepOutcome::Continue, next};
    }
    case 0:
      return {StepOutcome::Continue, next};
    default:
      break;
  }

  static std::atomic<uint32_t> unknown_log{0};
  if (unknown_log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN("Guest-PC fiber: unimplemented insn 0x{:08X} at pc=0x{:08X}",
                insn, pc);
  }
  return {StepOutcome::Error, pc};
#undef REX_STORE_U64
#undef REX_STORE_U32
#undef REX_STORE_U8
#undef REX_LOAD_U64
#undef REX_LOAD_U32
#undef REX_LOAD_U8
#undef REX_PHYS_HOST_OFFSET_SDK
}

uint32_t NativeFiberJobResumePc() {
  if (!g_config.native_sites.empty()) {
    return g_config.native_sites[0].pc;
  }
  return 0;
}

GuestPcRunResult RunFiberJobSite(PPCContext& ctx, uint8_t* base,
                                 rex::memory::Memory* memory) {
  if (g_tls.frames.empty()) {
    return GuestPcRunResult::Error;
  }
  FiberFrame& frame = g_tls.frames.back();
  // Re-capture each native-site visit: nested KeSet may restore a different r31
  // (inner BC88 dispatch) and caching BEC8 caused poll-job redispatch loops.
  frame.job_fn_at_entry = CaptureJobFunction(ctx, base, memory, frame.job_ctx);
  const uint32_t job_fn = frame.job_fn_at_entry;

  static std::atomic<uint32_t> job_log{0};
  if (job_log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN(
        "Guest-PC fiber: native resume job_fn=0x{:08X} r31=0x{:08X} "
        "fiber_slot=0x{:08X} job_ctx=0x{:08X}",
        job_fn, static_cast<uint32_t>(ctx.r31.u64), frame.fiber_slot_at_entry,
        frame.job_ctx);
  }

  if (job_fn == 0) {
    return GuestPcRunResult::Complete;
  }

  if (!DispatchFiberJobLikeEbea0(ctx, base, memory, job_fn)) {
    return GuestPcRunResult::Error;
  }
  SyncHostStackSpanFromGuest();

  if (!IsOnFiberStack(ctx)) {
    static std::atomic<uint32_t> thread_log{0};
    if (thread_log.fetch_add(1, std::memory_order_relaxed) < 12) {
      REXSYS_WARN(
          "Guest-PC fiber: job returned on thread stack (sp=0x{:08X}); "
          "skip swap-back tail",
          static_cast<uint32_t>(ctx.r1.u64));
    }
    return GuestPcRunResult::Complete;
  }

  if (frame.fiber_slot_at_entry != 0 &&
      RunSwapBackTail(ctx, base, memory, frame.fiber_slot_at_entry)) {
    SyncHostStackSpanFromGuest();
    return GuestPcRunResult::SwapBack;
  }
  return GuestPcRunResult::Complete;
}

bool IsGuestSpOnInnerJobFiberStack(rex::memory::Memory* memory,
                                   const PPCContext& ctx, uint32_t job_ctx) {
  if (job_ctx == 0 || !GuestRangeReadable(memory, job_ctx, 16)) {
    return false;
  }
  const uint32_t stack_base = LoadGuestU32(memory, job_ctx + 8);
  const uint32_t stack_limit = LoadGuestU32(memory, job_ctx + 12);
  const uint32_t sp = static_cast<uint32_t>(ctx.r1.u64);
  if (stack_base == 0 || stack_limit == 0 || stack_base <= stack_limit) {
    return false;
  }
  if (rex::ppc::IsGuestSpOnWorkQueueFiberStack(sp)) {
    return false;
  }
  return sp >= stack_limit && sp <= stack_base;
}

void MaybeNestedSwapBackAfterSkip(PPCContext& ctx, uint8_t* base,
                                  rex::memory::Memory* memory,
                                  GuestPcRunResult result) {
  if (result == GuestPcRunResult::SwapBack || g_tls.frames.empty()) {
    return;
  }
  const FiberFrame& frame = g_tls.frames.back();
  if (frame.fiber_slot_at_entry == 0 || !g_config.run_swap_back_tail) {
    return;
  }
  if (!IsGuestSpOnInnerJobFiberStack(memory, ctx, frame.job_ctx)) {
    return;
  }
  static std::atomic<uint32_t> log{0};
  if (log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN(
        "Guest-PC fiber: nested skip on inner job stack sp=0x{:08X} "
        "slot=0x{:08X}; swap-back",
        static_cast<uint32_t>(ctx.r1.u64), frame.fiber_slot_at_entry);
  }
  if (g_config.run_swap_back_tail(ctx, base, frame.fiber_slot_at_entry)) {
    SyncHostStackSpanFromGuest();
    g_tls.keset_swap_back = true;
  }
}

GuestPcRunResult DispatchNativeResumeSite(PPCContext& ctx, uint8_t* base,
                                          rex::memory::Memory* memory, uint32_t pc) {
  if (!g_tls.frames.empty()) {
    const uint32_t job_fn =
        CaptureJobFunction(ctx, base, memory, g_tls.frames.back().job_ctx);
    if (job_fn == 0) {
      static std::atomic<uint32_t> skip_log{0};
      if (skip_log.fetch_add(1, std::memory_order_relaxed) < 12) {
        REXSYS_WARN(
            "Guest-PC fiber: skip native resume pc=0x{:08X} (no job fn) "
            "r31=0x{:08X} job_ctx=0x{:08X}",
            pc, static_cast<uint32_t>(ctx.r31.u64), g_tls.frames.back().job_ctx);
      }
      return GuestPcRunResult::Complete;
    }
    ctx.r31.u64 = job_fn;
  }

  if (g_config.native_site_handler) {
    const GuestPcRunResult handled = g_config.native_site_handler(ctx, base, pc);
    if (handled != GuestPcRunResult::UnknownPc) {
      return handled;
    }
  }
  return RunFiberJobSite(ctx, base, memory);
}

bool IsNativeResumeSite(uint32_t pc) {
  for (const auto& site : g_config.native_sites) {
    if (site.pc == pc) {
      return true;
    }
  }
  return false;
}

// KeSet host-boundary longjmp fires before guest mtlr runs, so ctx.lr is often still
// the fiber swap entry (e.g. 0x830ED910) or the caller return — not the post-KeSet blr stub.
uint32_t ResolveSwapToFiberResumePc(PPCContext& ctx, uint32_t resume_pc) {
  resume_pc = NormalizeFiberResumePc(ctx, resume_pc);
  if (IsNativeResumeSite(resume_pc)) {
    return resume_pc;
  }
  if (g_config.native_sites.empty()) {
    return resume_pc;
  }

  const uint32_t native = g_config.native_sites[0].pc;
  if (resume_pc == native) {
    return native;
  }

  auto* runtime = rex::Runtime::instance();
  uint8_t* base =
      runtime && runtime->memory() ? runtime->memory()->virtual_membase() : nullptr;
  const bool swap_func_entry =
      base != nullptr && IsFiberGuestRegion(resume_pc) &&
      LookupGuestFunction(base, resume_pc) != nullptr;

  // Only rewrite fiber swap function entries (e.g. 0x830ED910). Do not rewrite
  // BC88 return addresses (0x82C0BDB4) when r1 is on a fiber stack.
  if (!swap_func_entry) {
    return resume_pc;
  }

  static std::atomic<uint32_t> log{0};
  if (log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN(
        "Guest-PC fiber: rewrite swap-to-fiber resume 0x{:08X} -> 0x{:08X} "
        "(swap_entry={})",
        resume_pc, native, swap_func_entry);
  }
  ctx.lr = native;
  return native;
}

bool FiberInterpreterActive() {
  return g_tls.run_depth > 0 || !g_tls.frames.empty() || g_tls.swap_pending;
}

bool PreserveGuestR31(const PPCContext& ctx) {
  if (FiberInterpreterActive()) {
    return true;
  }
  auto* runtime = rex::Runtime::instance();
  if (!runtime || !runtime->memory()) {
    return false;
  }
  uint8_t* base = runtime->memory()->virtual_membase();
  return IsPlausibleGuestCodePointer(base, static_cast<uint32_t>(ctx.r31.u64));
}

}  // namespace

bool InvokeGuestPcFiberKeSet(PPCContext& ctx, uint8_t* base,
                             const std::function<void(PPCContext&, uint8_t*)>& keset_call) {
  g_tls.swap_pending = true;
  g_tls.keset_to_fiber = false;
  g_tls.keset_swap_back = false;
  rex::ppc::BeginFiberSwapHostBoundary();
  keset_call(ctx, base);
  g_tls.swap_pending = false;

  const int host_reentry = rex::ppc::TakePendingFiberHostReentry();
  if (host_reentry == 2) {
    g_tls.keset_swap_back = true;
    return true;
  }
  return g_tls.keset_swap_back;
}

void RegisterGuestPcFiberConfig(GuestPcFiberConfig config) {
  g_config = std::move(config);
}

const GuestPcFiberConfig& GetGuestPcFiberConfig() {
  return g_config;
}

void ReconcileGuestPcFiberFrameStack(const PPCContext& ctx) {
  ReconcileFiberFrameStackImpl(ctx);
}

void InstallGuestPcFiberInterpreter() {
  SetFiberStackSwitchHandler(OnKeSetNotify);
}

void GuestPcFiberResume(PPCContext& ctx, uint8_t* base, uint32_t resume_pc) {
  resume_pc = ResolveSwapToFiberResumePc(ctx, resume_pc);

  auto* runtime = rex::Runtime::instance();
  auto* memory = runtime ? runtime->memory() : nullptr;

  bool bootstrap_dispatch = false;
  if (g_tls.frames.empty()) {
    rex::ppc::EnterFiberJobDispatch();
    g_tls.frames.push_back(
        FiberFrame{0, static_cast<uint32_t>(ctx.r3.u32), 0});
    bootstrap_dispatch = true;
  }

  g_tls.host_boundary_resume_handled = true;
  g_tls.keset_to_fiber = false;

  static std::atomic<uint32_t> resume_log{0};
  if (resume_log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN(
        "Guest-PC fiber: host-boundary swap-to-fiber resume pc=0x{:08X} "
        "lr=0x{:08X} sp=0x{:08X} frames={}",
        resume_pc, static_cast<uint32_t>(ctx.lr),
        static_cast<uint32_t>(ctx.r1.u64), g_tls.frames.size());
  }

  GuestPcRunResult run = GuestPcRunResult::Error;
  if (memory != nullptr) {
    run = DispatchNativeResumeSite(ctx, base, memory, resume_pc);
    SyncHostStackSpanFromGuest();
  }
  g_tls.host_boundary_swap_back =
      run == GuestPcRunResult::SwapBack || g_tls.keset_swap_back;

  if (bootstrap_dispatch) {
    FinishRunFiberSwap(0, "GuestPcFiberResume bootstrap exit");
    ReconcileFiberFrameStackImpl(ctx);
    rex::ppc::LeaveFiberJobDispatch();
    g_tls.host_boundary_resume_handled = false;
    g_tls.host_boundary_swap_back = false;
  }
}

void NotifyGuestPcFiberHostBoundarySwapBack() {
  g_tls.host_boundary_resume_handled = true;
  g_tls.host_boundary_swap_back = true;
  g_tls.keset_swap_back = true;
  g_tls.keset_to_fiber = false;
}

bool IsGuestPcFiberActive() {
  return FiberInterpreterActive();
}

bool ShouldPreserveGuestPcFiberR31(const PPCContext& ctx) {
  return PreserveGuestR31(ctx);
}

GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc) {
  auto* runtime = rex::Runtime::instance();
  if (!runtime || !runtime->memory()) {
    return GuestPcRunResult::Error;
  }
  auto* memory = runtime->memory();

  if (g_tls.run_depth >= kMaxRunDepth) {
    return GuestPcRunResult::Error;
  }
  ++g_tls.run_depth;

  uint32_t pc = start_pc;
  uint32_t steps = 0;
  GuestPcRunResult result = GuestPcRunResult::Complete;

  while (steps++ < kMaxGuestPcSteps) {
    if (pc == 0) {
      result = GuestPcRunResult::Error;
      break;
    }

    if (IsNativeResumeSite(pc)) {
      result = DispatchNativeResumeSite(ctx, base, memory, pc);
      if (result == GuestPcRunResult::SwapBack) {
        break;
      }
      if (result != GuestPcRunResult::UnknownPc) {
        break;
      }
    }

    if (PPCFunc* fn = LookupGuestFunction(base, pc)) {
      static std::atomic<uint32_t> fn_log{0};
      if (fn_log.fetch_add(1, std::memory_order_relaxed) < 16) {
        REXSYS_WARN("Guest-PC fiber: call entry 0x{:08X}", pc);
      }
      fn(ctx, base);

      if (g_tls.keset_to_fiber) {
        g_tls.keset_to_fiber = false;
        result = DispatchNativeResumeSite(
            ctx, base, memory,
            ResolveSwapToFiberResumePc(ctx, static_cast<uint32_t>(ctx.lr)));
        SyncHostStackSpanFromGuest();
        if (result == GuestPcRunResult::SwapBack || g_tls.keset_swap_back) {
          result = GuestPcRunResult::SwapBack;
          break;
        }
        if (result != GuestPcRunResult::UnknownPc) {
          break;
        }
        continue;
      }
      if (g_tls.keset_swap_back) {
        result = GuestPcRunResult::SwapBack;
        break;
      }
      result = GuestPcRunResult::Complete;
      break;
    }

    if (!IsFiberGuestRegion(pc)) {
      static std::atomic<uint32_t> oob_log{0};
      if (oob_log.fetch_add(1, std::memory_order_relaxed) < 8) {
        REXSYS_WARN("Guest-PC fiber: pc=0x{:08X} outside fiber regions", pc);
      }
      result = GuestPcRunResult::UnknownPc;
      break;
    }

    StepResult step = StepGuestInstruction(ctx, base, memory, pc);
    switch (step.outcome) {
      case StepOutcome::Continue:
        pc = step.next_pc;
        if (g_tls.keset_to_fiber) {
          g_tls.keset_to_fiber = false;
          result = DispatchNativeResumeSite(
              ctx, base, memory,
              ResolveSwapToFiberResumePc(ctx, static_cast<uint32_t>(ctx.lr)));
          SyncHostStackSpanFromGuest();
          if (result == GuestPcRunResult::SwapBack || g_tls.keset_swap_back) {
            result = GuestPcRunResult::SwapBack;
            --g_tls.run_depth;
            return result;
          }
          if (result != GuestPcRunResult::UnknownPc) {
            --g_tls.run_depth;
            return result;
          }
        } else if (g_tls.keset_swap_back) {
          result = GuestPcRunResult::SwapBack;
          --g_tls.run_depth;
          return result;
        }
        break;
      case StepOutcome::ReturnToHost:
        result = GuestPcRunResult::Complete;
        --g_tls.run_depth;
        return result;
      case StepOutcome::Error:
        result = GuestPcRunResult::Error;
        --g_tls.run_depth;
        return result;
    }
  }

  if (steps >= kMaxGuestPcSteps) {
    result = GuestPcRunResult::StepLimit;
  }

  --g_tls.run_depth;
  return result;
}

bool RunFiberSwap(PPCContext& ctx, uint8_t* base, FiberSwapImplFn swap_impl,
                  uint32_t job_ctx, uint32_t fiber_slot_at_entry) {
  auto* runtime = rex::Runtime::instance();
  auto* memory = runtime ? runtime->memory() : nullptr;

  // BC88 inside BEC8 poll calls ED910 while outer RunGuestPc is still active.
  // Re-entering RunFiberSwap with a new frame/sdk dispatch depth leaks both stacks.
  const bool nested = g_tls.run_depth > 0 || !g_tls.frames.empty();
  const size_t depth_at_entry = g_tls.frames.size();

  if (!nested) {
    rex::ppc::EnterFiberJobDispatch();
    g_tls.frames.push_back(FiberFrame{fiber_slot_at_entry, job_ctx, 0});
  } else if (!g_tls.frames.empty()) {
    FiberFrame& frame = g_tls.frames.back();
    frame.fiber_slot_at_entry = fiber_slot_at_entry;
    frame.job_ctx = job_ctx;
    frame.job_fn_at_entry = 0;
    static std::atomic<uint32_t> nested_log{0};
    if (nested_log.fetch_add(1, std::memory_order_relaxed) < 16) {
      REXSYS_WARN(
          "Guest-PC fiber: nested swap (reuse frame) frames={} run_depth={}",
          g_tls.frames.size(), g_tls.run_depth);
    }
  }

  g_tls.swap_pending = true;
  g_tls.keset_to_fiber = false;
  g_tls.keset_swap_back = false;

  auto* host = rex::system::XThread::TryGetCurrentThread();
  if (host) {
    g_tls.prev_stack_limit = host->stack_limit();
  }

  swap_impl(ctx, base);
  g_tls.swap_pending = false;

  const int host_reentry = rex::ppc::TakePendingFiberHostReentry();
  if (host_reentry == 1) {
    // Swap-to-fiber always resumes at the native job site (0x830ED900). ctx.lr is
    // often stale (e.g. 0x82C0BDB4 after a prior swap-back) because KeSet runs
    // before guest mtlr updates the link register.
    const uint32_t native_pc = NativeFiberJobResumePc();
    const uint32_t resume_pc =
        native_pc != 0 ? native_pc : static_cast<uint32_t>(ctx.lr);
    rex::ppc::CompleteFiberSwapWithoutReentry();
    if (nested) {
      // Nested BC88→ED910 while a fiber frame is active: run native EBEA0 inline
      // and return to the generated caller — do not re-enter GuestPcFiberResume
      // (that re-dispatched BEC8 and overflowed the 256KB thread stack).
      static std::atomic<uint32_t> nested_log{0};
      if (nested_log.fetch_add(1, std::memory_order_relaxed) < 16) {
        REXSYS_WARN(
            "Guest-PC fiber: nested swap-to-fiber EBEA0 resume pc=0x{:08X} "
            "r31=0x{:08X} sp=0x{:08X} frames={}",
            resume_pc, static_cast<uint32_t>(ctx.r31.u64),
            static_cast<uint32_t>(ctx.r1.u64), g_tls.frames.size());
      }
      if (memory != nullptr) {
        const GuestPcRunResult resume =
            DispatchNativeResumeSite(ctx, base, memory, resume_pc);
        MaybeNestedSwapBackAfterSkip(ctx, base, memory, resume);
      }
      SyncHostStackSpanFromGuest();
      return g_tls.keset_swap_back;
    }
    GuestPcFiberResume(ctx, base, resume_pc);
  } else if (host_reentry == 2) {
    NotifyGuestPcFiberHostBoundarySwapBack();
    rex::ppc::CompleteFiberSwapWithoutReentry();
    g_tls.keset_to_fiber = false;
    g_tls.keset_swap_back = false;
    SyncHostStackSpanFromGuest();
    if (!nested) {
      FinishRunFiberSwap(depth_at_entry, "RunFiberSwap swap-back reentry");
      ReconcileFiberFrameStackImpl(ctx);
      rex::ppc::LeaveFiberJobDispatch();
    }
    return true;
  }

  if (g_tls.host_boundary_resume_handled) {
    const bool swap_back =
        g_tls.host_boundary_swap_back || g_tls.keset_swap_back;
    g_tls.host_boundary_resume_handled = false;
    g_tls.host_boundary_swap_back = false;
    g_tls.keset_to_fiber = false;
    g_tls.keset_swap_back = false;
    SyncHostStackSpanFromGuest();
    if (!nested) {
      FinishRunFiberSwap(depth_at_entry, "RunFiberSwap exit");
      ReconcileFiberFrameStackImpl(ctx);
      rex::ppc::LeaveFiberJobDispatch();
    }
    return swap_back;
  }

  if (g_tls.keset_to_fiber) {
    if (!g_tls.frames.empty() && memory != nullptr) {
      g_tls.frames.back().job_fn_at_entry =
          CaptureJobFunction(ctx, base, memory, g_tls.frames.back().job_ctx);
    }
    const uint32_t resume_pc =
        ResolveSwapToFiberResumePc(ctx, static_cast<uint32_t>(ctx.lr));

    if (nested) {
      g_tls.keset_to_fiber = false;
      if (memory != nullptr) {
        const GuestPcRunResult resume =
            DispatchNativeResumeSite(ctx, base, memory, resume_pc);
        MaybeNestedSwapBackAfterSkip(ctx, base, memory, resume);
      }
      SyncHostStackSpanFromGuest();
      return g_tls.keset_swap_back;
    }

    g_tls.keset_to_fiber = false;
    static std::atomic<uint32_t> resume_log{0};
    if (resume_log.fetch_add(1, std::memory_order_relaxed) < 16) {
      REXSYS_WARN(
          "Guest-PC fiber: swap-to-fiber complete, resume pc=0x{:08X} "
          "lr=0x{:08X} sp=0x{:08X}",
          resume_pc, static_cast<uint32_t>(ctx.lr),
          static_cast<uint32_t>(ctx.r1.u64));
    }
    const GuestPcRunResult run = RunGuestPc(ctx, base, resume_pc);
    const bool swap_back =
        run == GuestPcRunResult::SwapBack || g_tls.keset_swap_back;
    g_tls.keset_swap_back = false;
    SyncHostStackSpanFromGuest();
    if (!nested) {
      FinishRunFiberSwap(depth_at_entry, "RunFiberSwap exit");
      ReconcileFiberFrameStackImpl(ctx);
      rex::ppc::LeaveFiberJobDispatch();
    }
    return swap_back;
  }

  if (g_tls.keset_swap_back) {
    g_tls.keset_swap_back = false;
    SyncHostStackSpanFromGuest();
    if (!nested) {
      FinishRunFiberSwap(depth_at_entry, "RunFiberSwap exit");
      ReconcileFiberFrameStackImpl(ctx);
      rex::ppc::LeaveFiberJobDispatch();
    }
    return true;
  }

  SyncHostStackSpanFromGuest();
  if (!nested) {
    FinishRunFiberSwap(depth_at_entry, "RunFiberSwap exit");
    ReconcileFiberFrameStackImpl(ctx);
    rex::ppc::LeaveFiberJobDispatch();
  }
  return false;
}

}  // namespace rex::ppc
