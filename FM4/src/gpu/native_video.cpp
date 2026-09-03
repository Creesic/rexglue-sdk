#include "gpu/native_video.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>

#include <rex/chrono/clock.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <rex/ui/window.h>

#include "render/video.h"

namespace fm4::gpu {
namespace {

std::atomic<bool> g_native_requested{false};

// Vblank delivery for detached mode. FM4's D3D::InterruptCallback(source, device)
// (ida40 0x826E46D0) handles source 0 by reading GPU MMIO 0x7FC86544 (register
// 0x1951, interrupt status) and running VerticalBlankInterrupt when bit 0 is
// set; that advances the frame counters, signals the vblank event and calls the
// game's own vblank callback. The SDK maps that MMIO window and raises the
// interrupt only inside a GPU plugin, so the native path does both here.
constexpr uint32_t kGuestInterruptCallback = 0x826E46D0;  // D3D::InterruptCallback
constexpr uint32_t kGpuMmioBase = 0x7FC80000;
constexpr uint32_t kRegInterruptStatus = 0x1951;
constexpr double kVsyncHz = 60.0;

uint32_t GpuMmioRead(void*, void*, uint32_t addr) {
  return ((addr & 0xFFFFu) / 4u) == kRegInterruptStatus ? 1u : 0u;  // vblank pending
}

void GpuMmioWrite(void*, void*, uint32_t, uint32_t) {
  // VerticalBlankInterrupt pokes a display register (0x7FC86110); nothing to do.
}

std::atomic<bool> g_vsync_running{false};
std::atomic<uint32_t> g_interrupt_device{0};
std::atomic<uint32_t> g_swaps_to_ack{0};  // presented frames awaiting a source-1 interrupt
// g_vsync_mutex serialises the only creator (OnGraphicsInterruptRegistered) and
// the only stopper (Shutdown via StopVsyncThread). Never take s.mutex while
// holding it: the vsync thread calls Present(), which takes s.mutex, and
// StopVsyncThread joins that thread.
std::mutex g_vsync_mutex;
rex::system::object_ref<rex::system::XHostThread> g_vsync_thread;

// Thread contract for the native path:
//
//  * Guest job threads call the D3D hooks. Those enqueue RenderCommands and
//    return; only Present, resource creation and WaitForGPU use the blocking
//    RenderQueue::Run.
//  * The render-queue thread (render_queue.cpp ThreadMain) is the only thread
//    that touches Plume. It is started at the end of ::Video::Init and stopped
//    at the top of ::Video::Shutdown, BEFORE the GPU drain, so a WaitForGPU
//    issued during shutdown falls back to inline dispatch on the caller
//    (render/video.cpp: RenderQueue::Stop() then WaitForGPU()).
//  * The vblank worker (VsyncThreadMain, below) runs guest code through the
//    function dispatcher. It must NEVER call into fm4::render or ::Video: the
//    guest interrupt handler it invokes can itself call D3D entry points, and
//    those go through the ordinary hook path from that thread like any other
//    guest thread. A direct renderer call here would deadlock against
//    RecordingMutex.
//  * Start order: OnGraphicsInterruptRegistered (vblank worker) may fire before
//    or after ::Video::Init. Both orders are safe: the worker only raises guest
//    interrupts, and the draw/clear/resolve entry points in render_state.cpp
//    return immediately until ::Video::IsInitialized().
//  * Stop order: Video::Shutdown stops the vblank worker first, so no guest
//    interrupt can arrive while the render thread is being joined.
//  * Lock order: g_vsync_mutex is outermost. Never take a render-side mutex
//    (::Video's s.mutex, RecordingMutex, the render queue's) while holding it --
//    the vsync thread's ExecuteInterrupt can re-enter Present -> RenderQueue::Run,
//    and StopVsyncThread joins that thread while holding g_vsync_mutex.
int VsyncThreadMain() {
  auto* kernel_state = REX_KERNEL_STATE();
  auto* thread = rex::system::XThread::GetCurrentThread();
  thread->SetActiveCpu(2);
  const uint64_t frequency = rex::chrono::Clock::guest_tick_frequency();
  const uint64_t interval = std::max<uint64_t>(1, uint64_t(double(frequency) / kVsyncHz));
  uint64_t last = rex::chrono::Clock::QueryGuestTickCount();
  while (g_vsync_running.load(std::memory_order_relaxed)) {
    const uint64_t now = rex::chrono::Clock::QueryGuestTickCount();
    while (now - last >= interval) {
      uint64_t args[] = {0, g_interrupt_device.load(std::memory_order_relaxed)};
      kernel_state->function_dispatcher()->ExecuteInterrupt(thread->thread_state(),
                                                            kGuestInterruptCallback, args, 2);
      last += interval;
    }
    // Swap completion: on hardware VdSwap ends with the GPU raising interrupt
    // source 1, and InterruptCallback(1) runs the library's swap callback that
    // releases the frame pacing. Deliver one per presented frame.
    while (g_swaps_to_ack.load(std::memory_order_relaxed) > 0) {
      g_swaps_to_ack.fetch_sub(1, std::memory_order_relaxed);
      uint64_t args[] = {1, g_interrupt_device.load(std::memory_order_relaxed)};
      kernel_state->function_dispatcher()->ExecuteInterrupt(thread->thread_state(),
                                                            kGuestInterruptCallback, args, 2);
    }
    rex::thread::Sleep(std::chrono::milliseconds(1));
  }
  return 0;
}

void StopVsyncThread() {
  std::lock_guard lock(g_vsync_mutex);
  if (!g_vsync_thread) {
    return;
  }
  g_vsync_running.store(false);
  g_vsync_thread->Wait(0, 0, 0, nullptr);
  g_vsync_thread.reset();
}

std::atomic<uint64_t> g_presented{0};
std::atomic<bool> g_initialized{false};

}  // namespace

void SetNativeRequested(bool on) { g_native_requested.store(on); }
bool NativeRequested() { return g_native_requested.load(); }

bool Video::Init(rex::ui::Window* window) {
  if (!window) {
    REXLOG_ERROR("native gpu: no window at Init");
    return false;
  }
  void* handle = window->GetNativeWindowHandle();
  if (!handle) {
    REXLOG_ERROR("native gpu: window has no native handle at Init");
    return false;
  }
  // 1280x720 is a hint only: ::Video::Init resizes to the HWND client rect and
  // publishes the real size in s_viewportWidth/Height.
  if (!::Video::Init(handle, 1280, 720)) {
    REXLOG_ERROR("native gpu: render video init failed");
    return false;
  }
  g_initialized.store(true);
  REXLOG_INFO("native gpu: render video up at {}x{}", ::Video::s_viewportWidth,
              ::Video::s_viewportHeight);
  return true;
}

void Video::Shutdown() {
  StopVsyncThread();
  if (g_initialized.exchange(false)) {
    ::Video::Shutdown();
  }
}

void Video::OnGraphicsInterruptRegistered(uint32_t device_va) {
  std::lock_guard lock(g_vsync_mutex);
  g_interrupt_device.store(device_va, std::memory_order_relaxed);
  if (g_vsync_thread) {
    return;  // re-registration: the thread picks up the new device
  }
  auto* kernel_state = REX_KERNEL_STATE();
  kernel_state->memory()->AddVirtualMappedRange(kGpuMmioBase, 0xFFFF0000u, 0x0000FFFFu, nullptr,
                                                GpuMmioRead, GpuMmioWrite);
  g_vsync_running.store(true);
  g_vsync_thread = rex::system::object_ref<rex::system::XHostThread>(
      new rex::system::XHostThread(kernel_state, 128 * 1024, 0, VsyncThreadMain));
  g_vsync_thread->set_name("fm4 native vsync");
  g_vsync_thread->Create();  // construction alone does not start an XHostThread
  REXLOG_INFO("native gpu: vblank worker started for guest D3DDevice 0x{:08X}", device_va);
}

void Video::RequestClear(uint32_t argb) {
  ::Video::SetFallbackClearColor(((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f,
                                 (argb & 0xFF) / 255.0f, ((argb >> 24) & 0xFF) / 255.0f);
  // Kept from the milestone-1 body: this is the one line that proves the guest's
  // D3DDevice_ClearF hook still reaches the renderer, and it is what tells a
  // black window apart from a window clearing to the colour the guest asked for.
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("native gpu: first guest clear colour 0x{:08X}", argb);
  }
}

void Video::Present() {
  if (!g_initialized.load()) {
    return;
  }
  // Present-source selection lands in Task 9; until then the blit has no source
  // and ::Video::Present falls back to the clear colour.
  if (::Video::Present(nullptr)) {
    g_presented.fetch_add(1, std::memory_order_relaxed);
    g_swaps_to_ack.fetch_add(1, std::memory_order_relaxed);
  }
}

uint64_t Video::PresentedFrames() { return g_presented.load(std::memory_order_relaxed); }

}  // namespace fm4::gpu
