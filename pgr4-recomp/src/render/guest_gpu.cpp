// render/guest_gpu.cpp

#include "guest_gpu.h"

#include <chrono>
#include <string>

#include <fmt/format.h>
#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/thread.h>

// This models a hardware vblank, so it must stay near display refresh. It is
// NOT the frame-rate cap.
//
// Free-running this was measured at ~4,000,000 interrupts/sec, which starves
// the guest's own threads and stalls it before it ever reaches a swap. Removing
// an FPS limit is a separate concern -- that belongs to swapchain vsync
// (RenderSwapChain::setVsyncEnabled) and to not throttling the guest's own
// frame work, neither of which is served by spinning this signal.
//
// <= 0 falls back to kDefaultVsyncHz rather than free-running, so a stray 0
// cannot reintroduce that stall.
REXCVAR_DEFINE_INT32(pgr4_vsync_hz, 60, "GPU",
                     "Guest graphics-interrupt (vblank) rate in Hz. Models display refresh; "
                     "not a frame-rate cap. <= 0 falls back to 60.");

namespace pgr4::render {

// X_STATUS_SUCCESS is a macro that casts to an unqualified X_STATUS, so the
// type has to be visible here for it to expand.
using rex::X_STATUS;

namespace {
// The guest's interrupt callback takes two arguments:
//   r3 = source, 0 for a normal vsync interrupt
//   r4 = the user_data handed to VdSetGraphicsInterruptCallback
constexpr uint64_t kInterruptSourceVsync = 0;

// Fallback when the cvar is unset or nonsensical. 60 matches the rate the
// title was written for.
constexpr int32_t kDefaultVsyncHz = 60;
}  // namespace

Pgr4GraphicsSystem::~Pgr4GraphicsSystem() { Shutdown(); }

rex::X_STATUS Pgr4GraphicsSystem::SetupPresentation(rex::ui::WindowedAppContext* app_context) {
  (void)app_context;
  has_presentation_ = true;
  return X_STATUS_SUCCESS;
}

void Pgr4GraphicsSystem::SetInterruptCallback(uint32_t callback, uint32_t user_data) {
  interrupt_user_data_.store(user_data, std::memory_order_relaxed);
  // Publish the callback last: the worker gates on it, so it must never observe
  // a live callback paired with stale user data.
  interrupt_callback_.store(callback, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: interrupt callback {:08X} (user_data {:08X})", callback, user_data);
}

void Pgr4GraphicsSystem::InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) {
  // Accepted and ignored: nothing consumes PM4 on the native path. Logged so the
  // difference from the xenos path stays visible in a log.
  REXLOG_INFO("PGR4 guest GPU: ring buffer at {:08X} (2^{} bytes) ignored; D3D is hooked directly",
              ptr, size_log2);
}

void Pgr4GraphicsSystem::EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) {
  (void)ptr;
  (void)block_size_log2;
}

rex::X_STATUS Pgr4GraphicsSystem::SetupGuestGpu(rex::runtime::FunctionDispatcher* function_dispatcher,
                                           rex::system::KernelState* kernel_state) {
  function_dispatcher_ = function_dispatcher;

  worker_running_.store(true, std::memory_order_release);
  worker_thread_ = rex::system::object_ref<rex::system::XHostThread>(
      new rex::system::XHostThread(kernel_state, 128 * 1024, 0, [this]() {
        WorkerMain();
        return 0;
      }));
  worker_thread_->set_name("PGR4 Vsync Worker");
  worker_thread_->Create();

  const int32_t hz = REXCVAR_GET(pgr4_vsync_hz);
  REXLOG_INFO("PGR4 guest GPU: vsync worker started ({} Hz{})", hz > 0 ? hz : kDefaultVsyncHz,
              hz > 0 ? "" : ", cvar unset/invalid");
  return X_STATUS_SUCCESS;
}

void Pgr4GraphicsSystem::WorkerMain() {
  uint64_t next_tick = 0;

  while (worker_running_.load(std::memory_order_acquire)) {
    const uint32_t callback = interrupt_callback_.load(std::memory_order_acquire);
    if (callback == 0) {
      // The guest has not registered yet. Idle rather than spin.
      rex::thread::Sleep(std::chrono::milliseconds(1));
      continue;
    }

    uint64_t args[] = {kInterruptSourceVsync,
                       interrupt_user_data_.load(std::memory_order_relaxed)};

    // Bring-up instrumentation. Without this there is no way to tell a worker
    // that never fires from a guest callback that returns without advancing the
    // frame loop -- the two look identical from outside.
    static bool logged_first = false;
    if (!logged_first) {
      logged_first = true;
      REXLOG_INFO("PGR4 guest GPU: dispatching first interrupt to {:08X}", callback);
    }

    function_dispatcher_->Execute(worker_thread_->thread_state(), callback, args,
                                  rex::countof(args));

    {
      static uint64_t fires = 0;
      static uint64_t window = 0;
      ++fires;
      const uint64_t f = rex::chrono::Clock::QueryHostTickFrequency();
      const uint64_t t = rex::chrono::Clock::QueryHostTickCount();
      if (window == 0) {
        window = t;
      } else if (f != 0 && (t - window) >= f) {
        REXLOG_INFO("PGR4 guest GPU: {} guest interrupts/sec", fires);
        fires = 0;
        window = t;
      }
    }

    int32_t hz = REXCVAR_GET(pgr4_vsync_hz);
    if (hz <= 0) {
      hz = kDefaultVsyncHz;
    }

    // Advance an absolute deadline rather than sleeping a fixed interval each
    // pass, so the cost of the callback itself does not compound into
    // ever-growing drift.
    const uint64_t freq = rex::chrono::Clock::QueryHostTickFrequency();
    if (freq == 0) {
      continue;
    }
    const uint64_t now = rex::chrono::Clock::QueryHostTickCount();
    const uint64_t period = freq / static_cast<uint64_t>(hz);
    if (next_tick == 0 || now > next_tick + period) {
      next_tick = now;  // first pass, or we fell far enough behind to resync
    }
    next_tick += period;
    if (next_tick > now) {
      const uint64_t remaining = next_tick - now;
      rex::thread::Sleep(std::chrono::microseconds(remaining * 1000000ULL / freq));
    }
  }
}

void Pgr4GraphicsSystem::Shutdown() {
  if (!worker_running_.exchange(false, std::memory_order_acq_rel)) {
    return;  // already shut down, or never started
  }
  if (worker_thread_) {
    worker_thread_->Wait(0, 0, 0, nullptr);
    worker_thread_.reset();
  }
  function_dispatcher_ = nullptr;
}

}  // namespace pgr4::render
