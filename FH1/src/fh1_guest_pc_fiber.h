#pragma once

#include <cstdint>

#include <rex/ppc/guest_pc_fiber.h>

struct PPCContext;

namespace fh1::fiber {

using GuestPcRunResult = rex::ppc::GuestPcRunResult;

/// True while a fiber swap or guest-PC run is in progress on this thread.
bool IsFiberInterpreterActive();

/// True when r31 holds a guest code pointer and must not be clobbered by track-loader repair.
bool ShouldPreserveGuestR31(const PPCContext& ctx);

/// Install KeSet hook and register the guest-PC fiber interpreter (idempotent).
void InstallGuestPcFiberInterpreter();

/// Drop stale interpreter frames when idle on the thread stack (leak recovery).
void ReconcileFiberFrameStack(const PPCContext& ctx);

/// Run sub_830ED910 fiber swap under guest-PC control (replaces ad-hoc reentry).
/// Returns true when swap-back to the thread stack completed inside the interpreter.
bool RunFiberSwap(PPCContext& ctx, uint8_t* base, uint32_t job_ctx,
                  uint32_t fiber_slot_at_entry);

GuestPcRunResult RunGuestPc(PPCContext& ctx, uint8_t* base, uint32_t start_pc);

}  // namespace fh1::fiber
