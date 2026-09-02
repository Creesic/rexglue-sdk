// render/video.cpp
//
// Phase 1: bring up a Plume device + swapchain and flip a cleared image once
// per guest D3DDevice_Swap. Deliberately does not touch guest resources,
// render state or draws yet -- the point is to prove the present loop is
// driven at the right cadence before anything depends on it.

#include "video.h"

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include <plume_render_interface.h>
#include <rex/logging.h>

using namespace plume;

namespace plume {
// Backend factories live in the backend TUs, not the interface header.
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
}  // namespace plume

namespace {

// Matches the swapchain image count below; one command list and fence per
// in-flight frame so the CPU can record frame N+1 while N is on the GPU.
constexpr uint32_t kNumFrames = 2;
constexpr RenderFormat kSwapChainFormat = RenderFormat::B8G8R8A8_UNORM;

struct FrameSlot {
  std::unique_ptr<RenderCommandList> commandList;
  std::unique_ptr<RenderCommandFence> fence;

  // Per-frame, not shared. A binary semaphore signalled again before its
  // previous wait has been consumed is a validation error on Vulkan, and with
  // two frames in flight a single shared acquire semaphore does exactly that.
  // D3D12's present ignores semaphores entirely, which is why one shared
  // semaphore *appeared* to work there.
  std::unique_ptr<RenderCommandSemaphore> acquireSemaphore;
  std::unique_ptr<RenderCommandSemaphore> renderFinishedSemaphore;

  bool submitted = false;
};

std::unique_ptr<RenderInterface> g_interface;
std::unique_ptr<RenderDevice> g_device;
std::unique_ptr<RenderCommandQueue> g_queue;
std::unique_ptr<RenderSwapChain> g_swapChain;
std::array<FrameSlot, kNumFrames> g_frames;

// One framebuffer per swapchain image, built lazily and dropped on resize.
std::vector<std::unique_ptr<RenderFramebuffer>> g_framebuffers;

uint32_t g_frameIndex = 0;
bool g_initialized = false;

// Guest D3DDevice_Swap can be reached from more than one guest thread; keep
// present serialised rather than assuming a single producer.
std::mutex g_presentMutex;

float g_clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

void ReleaseFramebuffers() { g_framebuffers.clear(); }

// Framebuffers are 1:1 with swapchain images and must be rebuilt whenever the
// swapchain is recreated, so key them by image index and rebuild the whole set.
RenderFramebuffer* FramebufferFor(uint32_t textureIndex) {
  const uint32_t count = g_swapChain->getTextureCount();
  if (g_framebuffers.size() != count) {
    g_framebuffers.clear();
    g_framebuffers.resize(count);
  }
  if (g_framebuffers[textureIndex] == nullptr) {
    const RenderTexture* attachment = g_swapChain->getTexture(textureIndex);
    RenderFramebufferDesc desc(&attachment, 1);
    g_framebuffers[textureIndex] = g_device->createFramebuffer(desc);
  }
  return g_framebuffers[textureIndex].get();
}

}  // namespace

bool Video::Init(void* nativeWindowHandle, uint32_t width, uint32_t height) {
  if (g_initialized) {
    return true;
  }
  if (nativeWindowHandle == nullptr) {
    REXLOG_ERROR("PGR4 Plume: no native window handle; renderer disabled");
    return false;
  }

  s_viewportWidth = width;
  s_viewportHeight = height;

  // D3D12 first (this is the backend FM2 and Unleashed are proven on), Vulkan
  // as a fallback so a machine without a usable D3D12 device still comes up.
  using Factory = std::unique_ptr<RenderInterface> (*)();
  const Factory factories[] = {
#if defined(_WIN32)
      plume::CreateD3D12Interface,
#endif
      plume::CreateVulkanInterface,
  };

  for (Factory factory : factories) {
    g_interface = factory();
    if (g_interface == nullptr) {
      continue;
    }
    g_device = g_interface->createDevice();
    if (g_device != nullptr) {
      break;
    }
    g_interface.reset();
  }

  if (g_device == nullptr) {
    REXLOG_ERROR("PGR4 Plume: no usable backend (tried D3D12, Vulkan)");
    return false;
  }

  g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);
  if (g_queue == nullptr) {
    REXLOG_ERROR("PGR4 Plume: createCommandQueue failed");
    Shutdown();
    return false;
  }

  RenderSwapChainDesc swapDesc(static_cast<RenderWindow>(nativeWindowHandle), kSwapChainFormat,
                               kNumFrames);
  g_swapChain = g_queue->createSwapChain(swapDesc);
  if (g_swapChain == nullptr) {
    REXLOG_ERROR("PGR4 Plume: createSwapChain failed");
    Shutdown();
    return false;
  }

  for (FrameSlot& slot : g_frames) {
    slot.commandList = g_queue->createCommandList();
    slot.fence = g_device->createCommandFence();
    slot.acquireSemaphore = g_device->createCommandSemaphore();
    slot.renderFinishedSemaphore = g_device->createCommandSemaphore();
    if (slot.commandList == nullptr || slot.fence == nullptr ||
        slot.acquireSemaphore == nullptr || slot.renderFinishedSemaphore == nullptr) {
      REXLOG_ERROR("PGR4 Plume: per-frame command list/fence/semaphore creation failed");
      Shutdown();
      return false;
    }
  }

  g_initialized = true;
  REXLOG_INFO("PGR4 Plume renderer initialized ({}x{}, {} frames)", width, height, kNumFrames);
  return true;
}

bool Video::IsInitialized() { return g_initialized; }

void Video::SetClearColor(float r, float g, float b, float a) {
  g_clearColor[0] = r;
  g_clearColor[1] = g;
  g_clearColor[2] = b;
  g_clearColor[3] = a;
}

bool Video::Present() {
  if (!g_initialized) {
    return false;
  }

  std::lock_guard<std::mutex> lock(g_presentMutex);

  if (g_swapChain->needsResize()) {
    // Framebuffers reference the old images, so they must go before the
    // resize, not after it.
    WaitForGPU();
    ReleaseFramebuffers();
    if (!g_swapChain->resize()) {
      return false;
    }
    s_viewportWidth = g_swapChain->getWidth();
    s_viewportHeight = g_swapChain->getHeight();
  }

  if (g_swapChain->isEmpty()) {
    return false;
  }

  FrameSlot& slot = g_frames[g_frameIndex];
  if (slot.submitted) {
    // Waiting on this slot's fence also guarantees its semaphores' previous
    // signal/wait pair has fully retired before we reuse them below.
    g_queue->waitForCommandFence(slot.fence.get());
    slot.submitted = false;
  }

  uint32_t textureIndex = 0;
  if (!g_swapChain->acquireTexture(slot.acquireSemaphore.get(), &textureIndex)) {
    return false;
  }

  RenderTexture* backBuffer = g_swapChain->getTexture(textureIndex);
  RenderCommandList* cmd = slot.commandList.get();

  cmd->begin();
  {
    const RenderTextureBarrier toColor(backBuffer, RenderTextureLayout::COLOR_WRITE);
    cmd->barriers(RenderBarrierStage::GRAPHICS, nullptr, 0, &toColor, 1);

    cmd->setFramebuffer(FramebufferFor(textureIndex));
    cmd->clearColor(
        0, RenderColor(g_clearColor[0], g_clearColor[1], g_clearColor[2], g_clearColor[3]));

    const RenderTextureBarrier toPresent(backBuffer, RenderTextureLayout::PRESENT);
    cmd->barriers(RenderBarrierStage::GRAPHICS, nullptr, 0, &toPresent, 1);
  }
  cmd->end();

  // Execution waits for the image to be acquired and signals when the clear
  // has finished; present then waits on that signal. On Vulkan present would
  // otherwise run before the clear -- VulkanSwapChain::present honours these
  // semaphores, D3D12SwapChain::present ignores them.
  const RenderCommandList* lists[] = {cmd};
  RenderCommandSemaphore* waits[] = {slot.acquireSemaphore.get()};
  RenderCommandSemaphore* signals[] = {slot.renderFinishedSemaphore.get()};
  g_queue->executeCommandLists(lists, 1, waits, 1, signals, 1, slot.fence.get());
  slot.submitted = true;

  const bool presented = g_swapChain->present(textureIndex, signals, 1);
  g_frameIndex = (g_frameIndex + 1) % kNumFrames;
  return presented;
}

void Video::WaitForGPU() {
  if (g_queue == nullptr) {
    return;
  }
  for (FrameSlot& slot : g_frames) {
    if (slot.submitted && slot.fence != nullptr) {
      g_queue->waitForCommandFence(slot.fence.get());
      slot.submitted = false;
    }
  }
}

void Video::Shutdown() {
  if (g_queue != nullptr) {
    WaitForGPU();
  }
  // Reverse creation order: framebuffers and per-frame objects reference the
  // swapchain and device, so they cannot outlive them.
  ReleaseFramebuffers();
  for (FrameSlot& slot : g_frames) {
    slot.commandList.reset();
    slot.fence.reset();
    slot.acquireSemaphore.reset();
    slot.renderFinishedSemaphore.reset();
    slot.submitted = false;
  }
  g_swapChain.reset();
  g_queue.reset();
  g_device.reset();
  g_interface.reset();
  g_frameIndex = 0;
  g_initialized = false;
}
