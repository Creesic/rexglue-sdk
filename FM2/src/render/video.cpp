#include "video.h"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <plume_render_interface.h>
#include <plume_render_interface_builders.h>
#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>

#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/shaders/copy_ps.hlsl.dxil.h"
#include "render/shaders/copy_vs.hlsl.dxil.h"

using namespace plume;

namespace plume {
// Backend factories (defined in thirdparty/plume/plume_d3d12.cpp,
// plume_vulkan.cpp).
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
}  // namespace plume

namespace {

constexpr RenderFormat kBackbufferFormat = RenderFormat::R8G8B8A8_UNORM;

std::unique_ptr<RenderInterface> g_interface;
std::unique_ptr<RenderDevice> g_device;
std::unique_ptr<RenderCommandQueue> g_queue;
std::unique_ptr<RenderCommandList> g_commandList;
std::unique_ptr<RenderCommandFence> g_commandFence;
std::unique_ptr<RenderCommandSemaphore> g_acquireSemaphore;
std::unique_ptr<RenderCommandSemaphore> g_renderSemaphore;
std::unique_ptr<RenderSwapChain> g_swapChain;

// One framebuffer per swapchain backbuffer, keyed by texture index.
std::vector<std::unique_ptr<RenderFramebuffer>> g_framebuffers;

// Dedicated copy queue for synchronous resource uploads.
std::unique_ptr<RenderCommandQueue> g_copyQueue;
std::unique_ptr<RenderCommandList> g_copyCommandList;
std::unique_ptr<RenderCommandFence> g_copyFence;
std::mutex g_copyMutex;

std::unique_ptr<RenderDescriptorSet> g_textureDescriptorSet;
std::mutex g_descriptorMutex;
uint32_t g_descriptorCapacity = fm2::render::kNullTextureDescriptorCount;
std::vector<uint32_t> g_freedDescriptors;
std::array<std::unique_ptr<RenderTexture>,
           fm2::render::kNullTextureDescriptorCount>
    g_nullTextures;
std::array<std::unique_ptr<RenderTextureView>,
           fm2::render::kNullTextureDescriptorCount>
    g_nullTextureViews;

// Bindless sampler set + the graphics pipeline layout (XenosRecomp ABI).
std::unique_ptr<RenderDescriptorSet> g_samplerDescriptorSet;
std::unique_ptr<RenderSampler> g_defaultSampler;
std::unique_ptr<RenderPipelineLayout> g_pipelineLayout;

std::unique_ptr<RenderShader> g_blitVertexShader;
std::unique_ptr<RenderShader> g_blitPixelShader;
std::unordered_map<uint32_t, std::unique_ptr<RenderPipeline>> g_blitPipelines;

bool g_initialized = false;
bool g_swapChainValid = false;
bool g_frameOpen = false;
// Whether g_commandFence has a pending, unconsumed signal. The D3D12 fence
// event is auto-reset: one executeCommandLists signal pairs with exactly one
// waitForCommandFence. Waiting without a pending signal blocks forever.
bool g_commandsInFlight = false;
uint32_t g_backBufferIndex = 0;
RenderWindow g_window{};

// The guest's final front-buffer surface to blit this frame (D3DDevice_Swap).
fm2::render::GuestBaseTexture* g_presentSource = nullptr;

// Guest's most recent D3DDevice_ClearF color, used when there is no real
// present source yet (see Video::SetFallbackClearColor).
RenderColor g_fallbackClearColor{0.0f, 0.0f, 0.0f, 1.0f};

// Sole present owner thread (0 = unclaimed). FM2's job-system pool drives
// present from many threads racing on the one global command list; once
// claimed, Present() from any other thread is dropped. Stored as a hashed
// thread id (portable, no <windows.h> dependency here).
std::atomic<uint64_t> g_presentOwnerKey{0};

inline uint64_t CurrentThreadKey() {
  return static_cast<uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

REXCVAR_DEFINE_BOOL(
    fm2_plume_single_thread_present, true, "FM2",
    "Pin Video::Present()/command-list submit to a single owner thread "
    "(the real GPU-submit thread); drop present calls from FM2's other "
    "job-system threads to stop them racing on the global command list.");

void RebuildFramebuffers() {
  g_framebuffers.clear();
  const uint32_t count = g_swapChain->getTextureCount();
  g_framebuffers.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    const RenderTexture* color = g_swapChain->getTexture(i);
    RenderFramebufferDesc desc(&color, 1);
    g_framebuffers[i] = g_device->createFramebuffer(desc);
  }
}

// FM2 runs in detached mode (no IGraphicsSystem/gpu_plugin), so
// GraphicsSystem's own vsync worker -- which normally fires the guest's
// registered graphics interrupt callback at the video mode's refresh rate --
// never exists. The game's own per-frame pacing (and, per FM2's FMOD/XMA
// signal-gate cadence, its audio mixer tick) waits on that interrupt, so
// without a substitute the guest hangs forever after its first frame: black
// screen, no audio, no forward progress. This mirrors GraphicsSystem's vsync
// worker exactly, just driving rex::kernel::xboxkrnl::DispatchGraphicsInterruptCallback
// directly instead of a GraphicsSystem instance.
std::atomic<bool> g_vsyncWorkerRunning{false};
rex::system::object_ref<rex::system::XHostThread> g_vsyncWorkerThread;

void StartVsyncWorker() {
  g_vsyncWorkerRunning = true;
  g_vsyncWorkerThread = rex::system::object_ref<rex::system::XHostThread>(
      new rex::system::XHostThread(REX_KERNEL_STATE(), 128 * 1024, 0, [] {
        rex::system::X_VIDEO_MODE video_mode;
        rex::kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
        double refresh_rate_hz =
            std::max(1.0, double(float(video_mode.refresh_rate)));
        uint64_t guest_tick_frequency = rex::chrono::Clock::guest_tick_frequency();
        uint64_t vsync_interval_ticks = std::max(
            uint64_t(1), uint64_t(double(guest_tick_frequency) / refresh_rate_hz));
        uint64_t last_frame_time = rex::chrono::Clock::QueryGuestTickCount();
        while (g_vsyncWorkerRunning) {
          uint64_t current_time = rex::chrono::Clock::QueryGuestTickCount();
          while (current_time - last_frame_time >= vsync_interval_ticks) {
            rex::kernel::xboxkrnl::DispatchGraphicsInterruptCallback();
            last_frame_time += vsync_interval_ticks;
          }
          rex::thread::Sleep(std::chrono::milliseconds(1));
        }
        return 0;
      }));
  g_vsyncWorkerThread->set_name("FM2 Native VSync");
  g_vsyncWorkerThread->Create();
}

void StopVsyncWorker() {
  if (!g_vsyncWorkerThread) return;
  g_vsyncWorkerRunning = false;
  g_vsyncWorkerThread->Wait(0, 0, 0, nullptr);
  g_vsyncWorkerThread.reset();
}

}  // namespace

bool Video::Init(void* nativeWindowHandle, uint32_t width, uint32_t height) {
  if (g_initialized) {
    return true;
  }

  g_window = reinterpret_cast<RenderWindow>(nativeWindowHandle);
  s_viewportWidth = width;
  s_viewportHeight = height;

  // Prefer D3D12 on Windows, fall back to Vulkan.
  using InterfaceFn = std::unique_ptr<RenderInterface> (*)();
  const std::array<InterfaceFn, 2> factories = {
      plume::CreateD3D12Interface,
      plume::CreateVulkanInterface,
  };

  for (InterfaceFn factory : factories) {
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
    REXGPU_ERROR("Video::Init - failed to create a Plume render device");
    return false;
  }

  REXGPU_INFO("Video::Init - device created: {}",
              g_device->getDescription().name);

  g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);
  g_commandList = g_queue->createCommandList();
  g_commandFence = g_device->createCommandFence();
  g_acquireSemaphore = g_device->createCommandSemaphore();
  g_renderSemaphore = g_device->createCommandSemaphore();

  RenderSwapChainDesc swapChainDesc(g_window, kBackbufferFormat, 2);
  g_swapChain = g_queue->createSwapChain(swapChainDesc);
  g_swapChainValid = !g_swapChain->needsResize();
  if (g_swapChainValid) {
    RebuildFramebuffers();
  }

  g_copyQueue = g_device->createCommandQueue(RenderCommandListType::COPY);
  g_copyCommandList = g_copyQueue->createCommandList();
  g_copyFence = g_device->createCommandFence();

  {
    using fm2::render::kNullTexture2DDescriptor;
    using fm2::render::kNullTexture3DDescriptor;
    using fm2::render::kNullTextureCubeDescriptor;
    using fm2::render::kNullTextureDescriptorCount;
    using fm2::render::kSamplerDescriptorSize;
    using fm2::render::kTextureDescriptorSize;

    RenderDescriptorSetBuilder textureSetBuilder;
    textureSetBuilder.begin();
    textureSetBuilder.addTexture(0, kTextureDescriptorSize);
    textureSetBuilder.end(true, kTextureDescriptorSize);
    g_textureDescriptorSet = textureSetBuilder.create(g_device.get());

    for (uint32_t i = 0; i < kNullTextureDescriptorCount; ++i) {
      RenderTextureDesc desc;
      desc.width = 1;
      desc.height = 1;
      desc.depth = 1;
      desc.mipLevels = 1;
      desc.format = RenderFormat::R8_UNORM;

      RenderTextureViewDesc viewDesc;
      viewDesc.format = desc.format;
      viewDesc.componentMapping =
          RenderComponentMapping(RenderSwizzle::ZERO, RenderSwizzle::ZERO,
                                 RenderSwizzle::ZERO, RenderSwizzle::ZERO);
      viewDesc.mipLevels = 1;

      switch (i) {
        case kNullTexture2DDescriptor:
          desc.dimension = RenderTextureDimension::TEXTURE_2D;
          desc.arraySize = 1;
          viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
          break;
        case kNullTexture3DDescriptor:
          desc.dimension = RenderTextureDimension::TEXTURE_3D;
          desc.arraySize = 1;
          viewDesc.dimension = RenderTextureViewDimension::TEXTURE_3D;
          break;
        case kNullTextureCubeDescriptor:
          desc.dimension = RenderTextureDimension::TEXTURE_2D;
          desc.arraySize = 6;
          desc.flags = RenderTextureFlag::CUBE;
          viewDesc.dimension = RenderTextureViewDimension::TEXTURE_CUBE;
          break;
      }

      g_nullTextures[i] = g_device->createTexture(desc);
      g_nullTextureViews[i] = g_nullTextures[i]->createTextureView(viewDesc);
      g_textureDescriptorSet->setTexture(i, g_nullTextures[i].get(),
                                         RenderTextureLayout::SHADER_READ,
                                         g_nullTextureViews[i].get());
    }

    RenderDescriptorSetBuilder samplerSetBuilder;
    samplerSetBuilder.begin();
    samplerSetBuilder.addSampler(0, kSamplerDescriptorSize);
    samplerSetBuilder.end(true, kSamplerDescriptorSize);
    g_samplerDescriptorSet = samplerSetBuilder.create(g_device.get());
    g_defaultSampler = g_device->createSampler(RenderSamplerDesc());
    g_samplerDescriptorSet->setSampler(0, g_defaultSampler.get());

    RenderPipelineLayoutBuilder layoutBuilder;
    layoutBuilder.begin(false, true);
    layoutBuilder.addDescriptorSet(textureSetBuilder);
    layoutBuilder.addDescriptorSet(textureSetBuilder);
    layoutBuilder.addDescriptorSet(textureSetBuilder);
    layoutBuilder.addDescriptorSet(samplerSetBuilder);
    // D3D12: VS/PS/shared constants as root CBVs; small PS push constant.
    layoutBuilder.addRootDescriptor(0, 4,
                                    RenderRootDescriptorType::CONSTANT_BUFFER);
    layoutBuilder.addRootDescriptor(1, 4,
                                    RenderRootDescriptorType::CONSTANT_BUFFER);
    layoutBuilder.addRootDescriptor(2, 4,
                                    RenderRootDescriptorType::CONSTANT_BUFFER);
    layoutBuilder.addPushConstant(3, 4, 4, RenderShaderStageFlag::PIXEL);
    layoutBuilder.end();
    g_pipelineLayout = layoutBuilder.create(g_device.get());
  }

  g_blitVertexShader = g_device->createShader(
      g_copy_vs_dxil, sizeof(g_copy_vs_dxil), "main", RenderShaderFormat::DXIL);
  g_blitPixelShader = g_device->createShader(
      g_copy_ps_dxil, sizeof(g_copy_ps_dxil), "main", RenderShaderFormat::DXIL);

  g_initialized = true;
  REXGPU_INFO("Video::Init - swapchain {}x{} valid={}", g_swapChain->getWidth(),
              g_swapChain->getHeight(), g_swapChainValid);

  StartVsyncWorker();
  return true;
}

bool Video::IsInitialized() { return g_initialized; }

void Video::SetFallbackClearColor(float r, float g, float b, float a) {
  g_fallbackClearColor = RenderColor(r, g, b, a);
}

namespace fm2::render {

RenderInterface* Interface() { return g_interface.get(); }
RenderDevice* Device() { return g_device.get(); }

void SetPresentSource(GuestBaseTexture* frontBuffer) {
  g_presentSource = frontBuffer;
}

void EnsureFrameStarted() {
  if (g_frameOpen || !g_initialized) return;

  if (!g_swapChainValid || g_swapChain->needsResize()) {
    // Drain the GPU (and any pending presentation) before resizing. Must not
    // wait on g_commandFence directly: Present() already consumed its signal.
    Video::WaitForGPU();
    g_swapChainValid = g_swapChain->resize();
    if (!g_swapChainValid) return;
    RebuildFramebuffers();
  }
  if (!g_swapChain->acquireTexture(g_acquireSemaphore.get(),
                                   &g_backBufferIndex)) {
    g_swapChainValid = false;
    return;
  }
  g_commandList->begin();
  g_frameOpen = true;
}

RenderCommandList* CommandList() {
  EnsureFrameStarted();
  return g_commandList.get();
}

RenderDescriptorSet* TextureDescriptorSet() {
  return g_textureDescriptorSet.get();
}

RenderDescriptorSet* SamplerDescriptorSet() {
  return g_samplerDescriptorSet.get();
}

RenderPipelineLayout* PipelineLayout() { return g_pipelineLayout.get(); }

RenderPipeline* GetBlitPipeline(RenderFormat format) {
  if (g_device == nullptr) return nullptr;
  auto& pipeline = g_blitPipelines[uint32_t(format)];
  if (pipeline == nullptr) {
    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = g_blitVertexShader.get();
    desc.pixelShader = g_blitPixelShader.get();
    desc.renderTargetFormat[0] = format;
    desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
    desc.renderTargetCount = 1;
    pipeline = g_device->createGraphicsPipeline(desc);
  }
  return pipeline.get();
}

uint32_t AllocTextureDescriptor() {
  std::lock_guard lock(g_descriptorMutex);
  if (!g_freedDescriptors.empty()) {
    uint32_t v = g_freedDescriptors.back();
    g_freedDescriptors.pop_back();
    return v;
  }
  return g_descriptorCapacity++;
}

void FreeTextureDescriptor(uint32_t index) {
  if (index < kNullTextureDescriptorCount) return;
  std::lock_guard lock(g_descriptorMutex);
  g_freedDescriptors.push_back(index);
}

void ExecuteUpload(const std::function<void(RenderCommandList*)>& record) {
  std::lock_guard lock(g_copyMutex);
  g_copyCommandList->begin();
  record(g_copyCommandList.get());
  g_copyCommandList->end();
  g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyFence.get());
  g_copyQueue->waitForCommandFence(g_copyFence.get());
}

}  // namespace fm2::render

void Video::ClaimPresentOwner() {
  uint64_t expected = 0;
  g_presentOwnerKey.compare_exchange_strong(expected, CurrentThreadKey(),
                                            std::memory_order_acq_rel);
}

void Video::Present() {
  // Drop present calls from FM2's other job-system threads: only the latched
  // owner (the real GPU-submit thread) may begin/end/submit the single global
  // command list and drive the swapchain. Before an owner is claimed, allow
  // the call (early boot). See fm2_plume_single_thread_present.
  if (REXCVAR_GET(fm2_plume_single_thread_present)) {
    const uint64_t owner = g_presentOwnerKey.load(std::memory_order_acquire);
    if (owner != 0 && owner != CurrentThreadKey()) {
      return;
    }
  }

  if (!g_initialized) {
    return;
  }

  // The guest's draws/clears for this frame have already been recorded into
  // the open command list (targeting the guest's own render-target
  // surfaces). Make sure a frame is open even if the guest issued nothing.
  fm2::render::EnsureFrameStarted();
  if (!g_frameOpen) {
    return;  // acquire failed this frame
  }

  RenderTexture* backBuffer = g_swapChain->getTexture(g_backBufferIndex);
  RenderFramebuffer* framebuffer = g_framebuffers[g_backBufferIndex].get();

  g_commandList->setFramebuffer(nullptr);
  RenderPipeline* blitPipeline = fm2::render::GetBlitPipeline(kBackbufferFormat);
  const bool blit = g_presentSource != nullptr &&
                    g_presentSource->texture != nullptr &&
                    blitPipeline != nullptr;
  if (blit) {
    if (g_presentSource->descriptorIndex == 0) {
      g_presentSource->descriptorIndex = fm2::render::AllocTextureDescriptor();
    }
    g_textureDescriptorSet->setTexture(
        g_presentSource->descriptorIndex, g_presentSource->texture,
        RenderTextureLayout::SHADER_READ, g_presentSource->textureView.get());

    RenderTextureBarrier toBlit[] = {
        RenderTextureBarrier(g_presentSource->texture,
                             RenderTextureLayout::SHADER_READ),
        RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE),
    };
    g_commandList->barriers(RenderBarrierStage::GRAPHICS, toBlit, 2);
    g_presentSource->layout = RenderTextureLayout::SHADER_READ;

    const uint32_t descriptorIndex = g_presentSource->descriptorIndex;
    g_commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    g_commandList->setPipeline(blitPipeline);
    g_commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    g_commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    g_commandList->setGraphicsPushConstants(0, &descriptorIndex, 0,
                                            sizeof(descriptorIndex));
    g_commandList->setFramebuffer(framebuffer);
    g_commandList->setViewports(
        RenderViewport(0.0f, 0.0f, float(g_swapChain->getWidth()),
                       float(g_swapChain->getHeight())));
    g_commandList->setScissors(
        RenderRect(0, 0, g_swapChain->getWidth(), g_swapChain->getHeight()));
    g_commandList->drawInstanced(3, 1, 0, 0);
    g_commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
  } else {
    // No usable front buffer yet (resource hooks not wired up): clear to the
    // guest's own most recent D3DDevice_ClearF color so we still present.
    g_commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE));
    g_commandList->setFramebuffer(framebuffer);
    g_commandList->clearColor(0, g_fallbackClearColor);
    g_commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
  }
  g_commandList->end();
  g_frameOpen = false;
  g_presentSource = nullptr;

  RenderCommandSemaphore* waitSemaphores[] = {g_acquireSemaphore.get()};
  RenderCommandSemaphore* signalSemaphores[] = {g_renderSemaphore.get()};
  const RenderCommandList* commandLists[] = {g_commandList.get()};
  g_queue->executeCommandLists(commandLists, 1, waitSemaphores, 1,
                               signalSemaphores, 1, g_commandFence.get());
  g_commandsInFlight = true;

  g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, 1);

  // Fully serialized for now; frame-in-flight pipelining comes later.
  g_queue->waitForCommandFence(g_commandFence.get());
  g_commandsInFlight = false;
}

void Video::WaitForGPU() {
  if (!g_initialized) {
    return;
  }
  if (g_commandsInFlight) {
    g_queue->waitForCommandFence(g_commandFence.get());
    g_commandsInFlight = false;
  }

  assert(!g_frameOpen);
  g_commandList->begin();
  g_commandList->end();
  g_queue->executeCommandLists(g_commandList.get(), g_commandFence.get());
  g_queue->waitForCommandFence(g_commandFence.get());
}

void Video::Shutdown() {
  if (!g_initialized) {
    return;
  }
  StopVsyncWorker();
  WaitForGPU();
  g_blitPipelines.clear();
  g_blitPixelShader.reset();
  g_blitVertexShader.reset();
  g_pipelineLayout.reset();
  g_defaultSampler.reset();
  g_samplerDescriptorSet.reset();
  g_textureDescriptorSet.reset();
  g_copyFence.reset();
  g_copyCommandList.reset();
  g_copyQueue.reset();
  g_framebuffers.clear();
  g_swapChain.reset();
  g_renderSemaphore.reset();
  g_acquireSemaphore.reset();
  g_commandFence.reset();
  g_commandList.reset();
  g_queue.reset();
  g_device.reset();
  g_interface.reset();
  g_initialized = false;
}
