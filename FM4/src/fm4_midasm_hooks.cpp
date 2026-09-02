// Mid-ASM hooks declared in fm4_config.toml ([[midasm_hook]]).
// See https://github.com/rexglue/rexglue-sdk/wiki/Mid-ASM-Hooks

#include "generated/default/fm4_init.h"

#include <atomic>

#include <rex/logging.h>

#include "gpu/native_video.h"

#if defined(__clang__) || defined(__GNUC__)
#define FM4_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FM4_FORCE_INLINE __forceinline
#else
#define FM4_FORCE_INLINE inline
#endif

namespace {

constexpr uint32_t kTlsCurrentThreadOffset = 0x100;
constexpr uint32_t kXThreadGpuTimerOffset = 0x58;
constexpr uint32_t kXThreadCurrentTokenOffset = 0x14C;

constexpr uint32_t kGpuWaitDeviceOffset = 0x0;
constexpr uint32_t kGpuWaitRingSnapshotOffset = 0x8;
constexpr uint32_t kGpuWaitTimerSnapshotOffset = 0xC;

constexpr uint32_t kD3DFlagsOffset = 0x2B3D;
constexpr uint32_t kD3DRingPtrOffset = 0x2B10;
constexpr uint32_t kD3DCurrentTokenOffset = 0x2B08;
constexpr uint32_t kD3DForceTimerRefreshOffset = 0x2B8C;
constexpr uint32_t kD3DWaitTimeoutTicksAddress = 0x82D74E28;

FM4_FORCE_INLINE uint8_t GuestLoadU8(uint8_t* base, uint32_t addr) {
  return *reinterpret_cast<volatile uint8_t*>(base + addr);
}

FM4_FORCE_INLINE uint32_t GuestLoadU32(uint8_t* base, uint32_t addr) {
  return __builtin_bswap32(*reinterpret_cast<volatile uint32_t*>(base + addr));
}

FM4_FORCE_INLINE void GuestStoreU32(uint8_t* base, uint32_t addr, uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(base + addr) = __builtin_bswap32(value);
}

bool Fm4GpuHangCheck(PPCContext& ctx, uint8_t* base, uint32_t d3d_device) {
  const auto saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;

  ctx.r1.u32 = saved_r1 - 0x70;
  GuestStoreU32(base, ctx.r1.u32, saved_r1);
  ctx.lr = 0x826E841C;
  ctx.r3.u64 = d3d_device;
  __imp__sub_826F7EF0(ctx, base);

  const bool gpu_hung = ctx.r3.u32 != 0;
  ctx.r1.u32 = saved_r1;
  ctx.lr = saved_lr;
  return gpu_hung;
}

}  // namespace

// Native replacement for the D3D GPU wait predicate. This is the hottest function
// in FM4 profiling captures; keep the observed timeout semantics but avoid the
// generated prologue/epilogue and the extra current-thread-token guest call.
extern "C" REX_FUNC(sub_826E8358) {
  if (fm4::gpu::NativeRequested()) {
    ctx.r3.u64 = 0;  // D3D::CBlocker::Check: 0 = stop waiting (callers loop while it returns nonzero)
    return;
  }
  const uint32_t wait = ctx.r3.u32;
  const uint32_t d3d = GuestLoadU32(base, wait + kGpuWaitDeviceOffset);

  if ((GuestLoadU8(base, d3d + kD3DFlagsOffset) & 0x2u) != 0) {
    ctx.r3.u64 = 0;
    return;
  }

  const uint32_t ring_ptr = GuestLoadU32(base, d3d + kD3DRingPtrOffset);
  const uint32_t current_thread = GuestLoadU32(base, ctx.r13.u32 + kTlsCurrentThreadOffset);
  const uint32_t now = GuestLoadU32(base, current_thread + kXThreadGpuTimerOffset);
  const uint32_t ring_value = GuestLoadU32(base, ring_ptr);

  if (GuestLoadU32(base, wait + kGpuWaitRingSnapshotOffset) != ring_value) {
    GuestStoreU32(base, wait + kGpuWaitTimerSnapshotOffset, now);
    GuestStoreU32(base, wait + kGpuWaitRingSnapshotOffset, ring_value);
  }

  if (GuestLoadU32(base, d3d + kD3DCurrentTokenOffset) ==
          GuestLoadU32(base, current_thread + kXThreadCurrentTokenOffset) &&
      GuestLoadU32(base, d3d + kD3DForceTimerRefreshOffset) != 0) {
    GuestStoreU32(base, wait + kGpuWaitTimerSnapshotOffset, now);
  }

  const uint32_t elapsed = now - GuestLoadU32(base, wait + kGpuWaitTimerSnapshotOffset);
  const uint32_t timeout = GuestLoadU32(base, kD3DWaitTimeoutTicksAddress);
  if (elapsed < timeout) {
    ctx.r3.u64 = 1;
    return;
  }

  if (!Fm4GpuHangCheck(ctx, base, d3d)) {
    GuestStoreU32(base, wait + kGpuWaitTimerSnapshotOffset, now);
    ctx.r3.u64 = 1;
    return;
  }

  ctx.r3.u64 = 0;
}

// sub_8293DE28 race-exit teardown: RtlLeaveCriticalSection returns to 0x8293DED0
// where the title has `b .`. Our kernel returns normally, so return to the caller
// (e.g. sub_8293E7A0) which unwinds the -96 stack frame.
void Fm4RaceExitAfterLeave() {
  static std::atomic<uint32_t> count{0};
  const auto n = count.fetch_add(1, std::memory_order_relaxed);
  if (n < 8) {
    REXLOG_WARN(
        "[fm4] sub_8293DE28: RtlLeave returned to spin site 0x8293DED0; returning to "
        "caller (count={})",
        n + 1);
  }
}
