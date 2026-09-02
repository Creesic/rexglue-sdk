#include "gpu/native_video.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

#include <plume_render_interface.h>
#include <rex/chrono/clock.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <rex/ui/window.h>

namespace plume {
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
}

namespace fm4::gpu {
namespace {

constexpr uint32_t kNumFrames = 2;

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

struct State {
  std::mutex mutex;
  rex::ui::Window* window = nullptr;
  std::unique_ptr<plume::RenderInterface> iface;
  std::unique_ptr<plume::RenderDevice> device;
  std::unique_ptr<plume::RenderCommandQueue> queue;
  std::array<std::unique_ptr<plume::RenderCommandList>, kNumFrames> lists;
  std::array<std::unique_ptr<plume::RenderCommandFence>, kNumFrames> fences;
  std::array<bool, kNumFrames> submitted{};
  std::array<std::unique_ptr<plume::RenderCommandSemaphore>, kNumFrames> acquire;
  std::vector<std::unique_ptr<plume::RenderCommandSemaphore>> render;  // one per swapchain image
  std::unique_ptr<plume::RenderSwapChain> swap_chain;
  std::vector<std::unique_ptr<plume::RenderFramebuffer>> framebuffers;
  uint32_t frame = 0;
  plume::RenderColor clear_color{};
  bool clear_logged_once = false;
  std::atomic<uint64_t> presented{0};
};

State& S() {
  static State s;
  return s;
}

plume::RenderColor MakeColor(float r, float g, float b, float a) {
  plume::RenderColor c{};
  c.rgba[0] = r;
  c.rgba[1] = g;
  c.rgba[2] = b;
  c.rgba[3] = a;
  return c;
}

void WaitAllSubmitted(State& s) {
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    if (s.submitted[i]) {
      s.queue->waitForCommandFence(s.fences[i].get());
      s.submitted[i] = false;
    }
  }
}

bool BuildFramebuffers(State& s) {
  s.framebuffers.clear();
  s.render.clear();
  const uint32_t count = s.swap_chain->getTextureCount();
  for (uint32_t i = 0; i < count; ++i) {
    const plume::RenderTexture* color[1] = {s.swap_chain->getTexture(i)};
    plume::RenderFramebufferDesc desc(color, 1);
    auto fb = s.device->createFramebuffer(desc);
    if (!fb) {
      REXLOG_ERROR("native gpu: createFramebuffer failed for image {}", i);
      s.framebuffers.clear();
      s.render.clear();
      return false;
    }
    s.framebuffers.push_back(std::move(fb));
    s.render.push_back(s.device->createCommandSemaphore());
  }
  return true;
}

// Needs the window's native handle, which does not exist when Init runs
// during SDK presentation setup; hence lazy.
bool BuildSwapChain(State& s) {
  if (!s.window) {
    return false;
  }
  auto handle = static_cast<plume::RenderWindow>(s.window->GetNativeWindowHandle());
  if (!handle) {
    return false;
  }
  // kNumFrames + 1: a flip-model swapchain needs one image beyond the frames
  // in flight so acquiring never waits on scanout.
  plume::RenderSwapChainDesc desc(handle, plume::RenderFormat::B8G8R8A8_UNORM, kNumFrames + 1);
  s.swap_chain = s.queue->createSwapChain(desc);
  if (!s.swap_chain || s.swap_chain->isEmpty()) {
    REXLOG_ERROR("native gpu: createSwapChain failed");
    s.swap_chain.reset();
    return false;
  }
  s.swap_chain->setVsyncEnabled(true);
  if (!BuildFramebuffers(s)) {
    s.swap_chain.reset();
    return false;
  }
  REXLOG_INFO("native gpu: swapchain {}x{} with {} images", s.swap_chain->getWidth(),
              s.swap_chain->getHeight(), s.swap_chain->getTextureCount());
  return true;
}

}  // namespace

void SetNativeRequested(bool on) { g_native_requested.store(on); }
bool NativeRequested() { return g_native_requested.load(); }

bool Video::Init(rex::ui::Window* window) {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  s.window = window;
  if (s.device) {
    return true;
  }
  s.iface = plume::CreateD3D12Interface();
  if (!s.iface) {
    REXLOG_ERROR("native gpu: CreateD3D12Interface failed");
    return false;
  }
  s.device = s.iface->createDevice();
  if (!s.device) {
    REXLOG_ERROR("native gpu: createDevice failed");
    return false;
  }
  s.queue = s.device->createCommandQueue(plume::RenderCommandListType::DIRECT);
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    s.lists[i] = s.queue->createCommandList();
    s.fences[i] = s.device->createCommandFence();
    s.acquire[i] = s.device->createCommandSemaphore();
  }
  s.clear_color = MakeColor(1.0f, 0.0f, 1.0f, 1.0f);  // magenta until the guest clears
  REXLOG_INFO("native gpu: D3D12 device '{}'", s.device->getDescription().name);
  BuildSwapChain(s);  // may legitimately fail here; retried on first Present
  return true;
}

void Video::Shutdown() {
  auto& s = S();
  // Must run before the lock: joining the vsync thread pumps guest interrupt
  // callbacks that can re-enter Present()/RequestClear(), which take s.mutex,
  // so holding it here would deadlock the join.
  StopVsyncThread();
  std::lock_guard lock(s.mutex);
  if (!s.device) {
    return;
  }
  WaitAllSubmitted(s);
  s.framebuffers.clear();
  s.render.clear();
  s.swap_chain.reset();
  // Device, queue, lists and fences stay alive until static destruction; the
  // hazard to avoid is a guest thread calling Present() after this, which
  // s.window = nullptr and the swap_chain reset make a no-op.
  s.window = nullptr;
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
  auto& s = S();
  std::lock_guard lock(s.mutex);
  s.clear_color = MakeColor(((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f,
                            (argb & 0xFF) / 255.0f, ((argb >> 24) & 0xFF) / 255.0f);
  if (!s.clear_logged_once) {
    s.clear_logged_once = true;
    REXLOG_INFO("native gpu: first guest clear colour 0x{:08X}", argb);
  }
}

void Video::Present() {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  if (!s.device) {
    return;
  }
  if (!s.swap_chain && !BuildSwapChain(s)) {
    return;
  }
  if (s.swap_chain->needsResize() || s.framebuffers.empty()) {
    WaitAllSubmitted(s);
    s.framebuffers.clear();
    s.render.clear();
    s.swap_chain->resize();
    if (s.swap_chain->isEmpty() || !BuildFramebuffers(s)) {
      return;  // minimised: nothing to draw into; retried next Present
    }
  }

  uint32_t image = 0;
  if (!s.swap_chain->acquireTexture(s.acquire[s.frame].get(), &image)) {
    return;
  }
  plume::RenderTexture* back = s.swap_chain->getTexture(image);
  plume::RenderCommandList* list = s.lists[s.frame].get();

  list->begin();
  list->barriers(plume::RenderBarrierStage::GRAPHICS,
                 plume::RenderTextureBarrier(back, plume::RenderTextureLayout::COLOR_WRITE));
  list->setFramebuffer(s.framebuffers[image].get());
  list->clearColor(0, s.clear_color);
  list->setFramebuffer(nullptr);
  list->barriers(plume::RenderBarrierStage::GRAPHICS,
                 plume::RenderTextureBarrier(back, plume::RenderTextureLayout::PRESENT));
  list->end();

  const plume::RenderCommandList* lists[] = {list};
  plume::RenderCommandSemaphore* waits[] = {s.acquire[s.frame].get()};
  plume::RenderCommandSemaphore* signals[] = {s.render[image].get()};
  s.queue->executeCommandLists(lists, 1, waits, 1, signals, 1, s.fences[s.frame].get());
  s.submitted[s.frame] = true;
  s.swap_chain->present(image, signals, 1);

  // Advance, then wait for the slot about to be reused (one frame old). That
  // gap is the CPU/GPU overlap.
  s.frame = (s.frame + 1) % kNumFrames;
  if (s.submitted[s.frame]) {
    s.queue->waitForCommandFence(s.fences[s.frame].get());
    s.submitted[s.frame] = false;
  }
  s.presented.fetch_add(1, std::memory_order_relaxed);
  g_swaps_to_ack.fetch_add(1, std::memory_order_relaxed);
}

uint64_t Video::PresentedFrames() { return S().presented.load(std::memory_order_relaxed); }

}  // namespace fm4::gpu
