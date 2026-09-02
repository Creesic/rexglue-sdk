#include "gpu/native_video.h"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <plume_render_interface.h>
#include <rex/logging.h>
#include <rex/ui/window.h>

namespace plume {
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
}

namespace fm4::gpu {
namespace {

constexpr uint32_t kNumFrames = 2;

std::atomic<bool> g_native_requested{false};

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
}

uint64_t Video::PresentedFrames() { return S().presented.load(std::memory_order_relaxed); }

}  // namespace fm4::gpu
