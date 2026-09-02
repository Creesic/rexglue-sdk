// render/guest_gpu.cpp

#include "guest_gpu.h"

#include <array>
#include <chrono>

#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>
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

// GPU register window. Same mapping the xenos plugin registers: 64 KB at
// 0x7FC80000, addressed as dword indices.
constexpr uint32_t kGpuRegisterBase = 0x7FC80000;
constexpr uint32_t kGpuRegisterMask = 0xFFFF0000;
constexpr uint32_t kGpuRegisterSize = 0x0000FFFF;
constexpr uint32_t kGpuRegisterCount = 0x4000;

// Register indices the guest's D3D touches during bring-up.
constexpr uint32_t kRegCpRbRptr = 0x01C4;         // CP_RB_RPTR
constexpr uint32_t kRegCpRbWptr = 0x01C5;         // CP_RB_WPTR
constexpr uint32_t kRegRbEdramTiming = 0x0F00;    // RB_EDRAM_TIMING
constexpr uint32_t kRegRbBcControl = 0x0F01;      // RB_BC_CONTROL
constexpr uint32_t kRegD1ModeVCounter = 0x194C;   // R500_D1MODE_V_COUNTER
constexpr uint32_t kRegInterruptStatus = 0x1951;  // interrupt status
constexpr uint32_t kRegD1ModeViewport = 0x1961;   // AVIVO_D1MODE_VIEWPORT_SIZE

// The guest frame PGR4 renders. Reported for the display-mode registers so the
// same values the xenos plugin derives from VdQueryVideoMode come back here,
// without pulling that dependency in for two constants.
constexpr uint32_t kGuestWidth = 1280;
constexpr uint32_t kGuestHeight = 720;

// Last-written values, so reads of registers we do not special-case return
// what the guest stored -- the same fallback the real register file provides.
std::array<uint32_t, kGpuRegisterCount> g_registers{};

}  // namespace

Pgr4GraphicsSystem::~Pgr4GraphicsSystem() { Shutdown(); }

X_STATUS Pgr4GraphicsSystem::SetupPresentation(rex::ui::WindowedAppContext* app_context) {
  (void)app_context;
  has_presentation_ = true;
  return X_STATUS_SUCCESS;
}

// ---- Vd* surface -----------------------------------------------------------

void Pgr4GraphicsSystem::SetInterruptCallback(uint32_t callback, uint32_t user_data) {
  interrupt_user_data_.store(user_data, std::memory_order_relaxed);
  // Publish the callback last: the worker gates on it, so it must never observe
  // a live callback paired with stale user data.
  interrupt_callback_.store(callback, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: interrupt callback {:08X} (user_data {:08X})", callback, user_data);
}

void Pgr4GraphicsSystem::InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) {
  // Nothing here executes PM4; the ring is only ever *consumed* (see
  // PublishReadPointer). Its location is not needed for that, so it is logged
  // and otherwise ignored. Size follows the real command processor's decode.
  write_ptr_index_.store(0, std::memory_order_relaxed);
  REXLOG_INFO("PGR4 guest GPU: ring buffer at {:08X} ({} bytes); consumed without execution",
              ptr, uint32_t(1) << (size_log2 + 3));
}

void Pgr4GraphicsSystem::EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) {
  (void)block_size_log2;
  // CP_RB_RPTR_ADDR: the physical address the guest polls for how far the GPU
  // has consumed the ring. From here on, every write-pointer update and every
  // vblank republishes "everything consumed" to it.
  read_ptr_writeback_.store(ptr, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: read-pointer writeback at {:08X}", ptr);
  PublishReadPointer();
}

void Pgr4GraphicsSystem::PublishReadPointer() {
  const uint32_t writeback = read_ptr_writeback_.load(std::memory_order_acquire);
  if (writeback == 0 || memory_ == nullptr) {
    return;
  }
  // Mirror the write pointer back as the read pointer. To the guest this is a
  // GPU that has always finished everything it was handed, which is exactly
  // what a native renderer intercepting at the API level looks like from the
  // guest's side. The real command processor does this same store after
  // executing; we do it without executing.
  const uint32_t write_index = write_ptr_index_.load(std::memory_order_acquire);
  rex::memory::store_and_swap<uint32_t>(memory_->TranslatePhysical<uint8_t*>(writeback),
                                        write_index);
}

// ---- GPU register window ---------------------------------------------------

uint32_t Pgr4GraphicsSystem::ReadRegisterThunk(void* ppc_context, void* self, uint32_t addr) {
  (void)ppc_context;
  return static_cast<Pgr4GraphicsSystem*>(self)->ReadRegister(addr);
}

void Pgr4GraphicsSystem::WriteRegisterThunk(void* ppc_context, void* self, uint32_t addr,
                                            uint32_t value) {
  (void)ppc_context;
  static_cast<Pgr4GraphicsSystem*>(self)->WriteRegister(addr, value);
}

uint32_t Pgr4GraphicsSystem::ReadRegister(uint32_t addr) {
  const uint32_t r = (addr & 0xFFFF) / 4;
  switch (r) {
    // The guest polls these during device init. Values match what the xenos
    // plugin answers, so bring-up sees the same hardware it would there.
    case kRegRbEdramTiming:
      return 0x08100748;
    case kRegRbBcControl:
      return 0x0000200E;
    case kRegD1ModeVCounter:
      return kGuestHeight;
    case kRegInterruptStatus:
      return 1;  // vblank
    case kRegD1ModeViewport:
      return (kGuestWidth << 16) | kGuestHeight;
    // Consumption is instantaneous, so the read pointer is always the write
    // pointer -- same answer the writeback location gives.
    case kRegCpRbRptr:
      return write_ptr_index_.load(std::memory_order_acquire);
    default:
      break;
  }
  return r < kGpuRegisterCount ? g_registers[r] : 0;
}

void Pgr4GraphicsSystem::WriteRegister(uint32_t addr, uint32_t value) {
  const uint32_t r = (addr & 0xFFFF) / 4;
  if (r < kGpuRegisterCount) {
    g_registers[r] = value;
  }
  if (r == kRegCpRbWptr) {
    // The guest just advanced its write pointer. Consume immediately so its
    // allocator never sees the ring as full.
    write_ptr_index_.store(value, std::memory_order_release);
    PublishReadPointer();

    // Bring-up instrumentation: whether the guest ever submits ring work is the
    // dividing line between "stalled on a full ring" and "stalled before it
    // ever built a frame". Without this the two are indistinguishable.
    static std::atomic<uint64_t> writes{0};
    if (writes.fetch_add(1, std::memory_order_relaxed) == 0) {
      REXLOG_INFO("PGR4 guest GPU: first CP_RB_WPTR write = {:08X}", value);
    }
    return;
  }

  // Anything else the guest pokes during bring-up that we are not modelling.
  // Logged once per register so a missing behaviour shows up by name instead
  // of as silence.
  static std::array<std::atomic<bool>, kGpuRegisterCount> logged{};
  if (r < kGpuRegisterCount && !logged[r].exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("PGR4 guest GPU: unmodelled register write {:04X} = {:08X}", r, value);
  }
}

// ---- lifecycle ---------------------------------------------------------------

X_STATUS Pgr4GraphicsSystem::SetupGuestGpu(rex::runtime::FunctionDispatcher* function_dispatcher,
                                           rex::system::KernelState* kernel_state) {
  function_dispatcher_ = function_dispatcher;
  memory_ = kernel_state->memory();

  // Without this mapping the guest's CP_RB_WPTR stores land nowhere and its
  // status polls read zero, and D3D device creation never completes.
  if (!memory_->AddVirtualMappedRange(
          kGpuRegisterBase, kGpuRegisterMask, kGpuRegisterSize, this,
          reinterpret_cast<rex::runtime::MMIOReadCallback>(ReadRegisterThunk),
          reinterpret_cast<rex::runtime::MMIOWriteCallback>(WriteRegisterThunk))) {
    REXLOG_ERROR("PGR4 guest GPU: failed to map GPU register window at {:08X}", kGpuRegisterBase);
    return X_STATUS_UNSUCCESSFUL;
  }
  mmio_mapped_ = true;

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

    // Republish consumption every vblank as well as on every write-pointer
    // store, so a guest that polls the writeback rather than the register sees
    // progress even between its own submissions.
    PublishReadPointer();

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
  // The MMIO range is left registered: Memory offers no matching remove, and
  // the guest is gone by the time this runs.
  function_dispatcher_ = nullptr;
  memory_ = nullptr;
}

}  // namespace pgr4::render
