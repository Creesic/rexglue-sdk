// render/d3d_hooks.cpp
//
// Guest D3D entry points replaced by the native Plume renderer.
//
// Phase 1 hooks exactly one function: D3DDevice_Swap (0x82695BE0). IDA
// identifies it as the sole caller of the VdSwap import, and it is the same
// XDK library routine FM2 presents from -- 0x594 bytes here against 0x598
// there. Hooking Swap rather than VdSwap itself puts the present trigger at
// the API boundary instead of inside the ring-buffer plumbing.
//
// REX_HOOK is a link-time full replacement with no fall-through to the
// recompiled body, so once this TU is compiled the guest's own swap path is
// gone. That is why the renderer sits behind the PGR4_ENABLE_PLUME build
// option rather than a runtime cvar: with the option OFF this file is not
// compiled at all and the xenos plugin path is untouched.

#include <atomic>
#include <cstdint>

#include <rex/chrono/clock.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>

#include "video.h"

namespace {

// Offsets into the guest's D3D CDevice that the original Swap maintains and
// that other guest code reads back. Taken from the IDA decompile of
// D3DDevice_Swap: it opens with ++*(a1 + 16544), and its CBlocker loop spins on
// (*(a1 + 16544) - *(a1 + 16552)) >= 0xF, i.e. "frames submitted" minus
// "frames retired" as the outstanding-frame count.
constexpr uint32_t kDeviceFramesSubmitted = 16544;
constexpr uint32_t kDeviceFramesRetired = 16552;

// Phase 1 bring-up instrumentation: report swap cadence once per second, so we
// can tell whether the guest actually drives presentation at frame rate and
// whether Present() is silently dropping every frame. Remove once the renderer
// produces real output.
//
// Reached from whichever guest thread calls Swap, outside Video::Present's
// mutex, so the counters are atomic.
void ReportSwapCadence(bool presented) {
  static std::atomic<uint64_t> swaps{0};
  static std::atomic<uint64_t> presents{0};
  static std::atomic<uint64_t> windowStart{0};

  swaps.fetch_add(1, std::memory_order_relaxed);
  if (presented) {
    presents.fetch_add(1, std::memory_order_relaxed);
  }

  const uint64_t now = rex::chrono::Clock::QueryHostTickCount();
  const uint64_t freq = rex::chrono::Clock::QueryHostTickFrequency();
  uint64_t start = windowStart.load(std::memory_order_relaxed);
  if (start == 0) {
    windowStart.compare_exchange_strong(start, now, std::memory_order_relaxed);
    return;
  }
  if (freq != 0 && (now - start) >= freq &&
      windowStart.compare_exchange_strong(start, now, std::memory_order_relaxed)) {
    // Only the thread that won the window reset reports, so a burst of
    // concurrent swaps cannot double-log or double-zero.
    REXLOG_INFO("PGR4 Plume: {} D3DDevice_Swap/sec, {} presented",
                swaps.exchange(0, std::memory_order_relaxed),
                presents.exchange(0, std::memory_order_relaxed));
  }
}

// The original Swap bumps the device's submitted-frame counter and later
// blocks until the GPU has retired enough of them. A native renderer has no
// GPU lag to wait on, so model an always-caught-up GPU: advance "submitted"
// as the original does, and pin "retired" to it. Leaving either alone would
// hand guest frame pacing a permanently growing backlog.
void AdvanceGuestFrameCounters(u32 device_ptr) {
  auto* memory = REX_KERNEL_STATE()->memory();
  uint8_t* submitted = memory->TranslateVirtual<uint8_t*>(device_ptr + kDeviceFramesSubmitted);
  uint8_t* retired = memory->TranslateVirtual<uint8_t*>(device_ptr + kDeviceFramesRetired);

  const uint32_t next = rex::memory::load_and_swap<uint32_t>(submitted) + 1;
  rex::memory::store_and_swap<uint32_t>(submitted, next);
  rex::memory::store_and_swap<uint32_t>(retired, next);
}

// D3DDevice_Swap(CDevice* device, void* frontBuffer,
//                const D3DVIDEO_SCALER_PARAMETERS* scalerParams) -> int
//
// Phase 1 ignores the front buffer and scaler parameters: no guest resource
// translation exists yet, so there is nothing to present but a cleared image.
// The parameters are named for the real signature so Phase 2 has the shape.
u32 SwapHook(u32 device_ptr, u32 front_buffer_ptr, u32 scaler_params_ptr) {
  (void)front_buffer_ptr;
  (void)scaler_params_ptr;

  if (device_ptr != 0) {
    AdvanceGuestFrameCounters(device_ptr);
  }

  // Init failed, or the renderer was never brought up. Returning leaves the
  // guest running with nothing on screen, which beats taking the game down.
  if (!Video::IsInitialized()) {
    ReportSwapCadence(false);
    return 0;
  }

  ReportSwapCadence(Video::Present());
  // The original returns the result of its trailing sub_82690AF8(device, 0)
  // call. Nothing in the decompile inspects it for failure, and returning
  // success is the conservative choice until Phase 2 learns what it means.
  return 0;
}

}  // namespace

REX_HOOK(D3DDevice_Swap, SwapHook)
