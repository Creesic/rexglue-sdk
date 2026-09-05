#include "video.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <plume_render_interface.h>
#include <plume_render_interface_builders.h>
#if defined(_WIN32)
#include <plume_d3d12.h>
#include <renderdoc_app.h>
#endif
#include <rex/chrono/clock.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>

#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/shaders/copy_ps.hlsl.dxil.h"
#include "render/shaders/copy_msaa_ps.hlsl.dxil.h"
#include "render/shaders/copy_vs.hlsl.dxil.h"

using namespace plume;

namespace plume {
// Backend factories (defined in thirdparty/plume/plume_d3d12.cpp,
// plume_vulkan.cpp).
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
}  // namespace plume

namespace fm2::render {
// Not part of render_internal.h's public surface (only used within this
// TU); forward-declared here so ExecuteCommandListImpl (below) can call it ahead of
// its definition.
// its definition further down this file.
void EnsureFrameStarted();
}  // namespace fm2::render

namespace {

constexpr RenderFormat kBackbufferFormat = RenderFormat::R8G8B8A8_UNORM;

// 2-frame pipelining: while slot N's command list is submitted and the GPU
// is chewing on it, slot N+1 is free to record the next frame. Present()
// only blocks on a slot's fence right before reusing it, not right after
// submitting it.
constexpr uint32_t kNumFrames = fm2::render::kNumFrames;

std::unique_ptr<RenderInterface> g_interface;
std::unique_ptr<RenderDevice> g_device;
bool g_d3d12Backend = false;
std::unique_ptr<RenderCommandQueue> g_queue;
std::array<std::unique_ptr<RenderCommandList>, kNumFrames> g_commandLists;
std::array<std::unique_ptr<RenderCommandFence>, kNumFrames> g_commandFences;
std::array<std::unique_ptr<RenderCommandSemaphore>, kNumFrames>
    g_acquireSemaphores;
std::array<std::unique_ptr<RenderCommandSemaphore>, kNumFrames>
    g_renderSemaphores;
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
std::unique_ptr<RenderShader> g_blitMsaaPixelShader;
std::unordered_map<uint32_t, std::unique_ptr<RenderPipeline>> g_blitPipelines;

bool g_initialized = false;
bool g_swapChainValid = false;
bool g_frameOpen = false;
// Per-slot: whether that slot's command list has been submitted to the GPU
// and not yet waited on. The D3D12 fence event is auto-reset: one
// executeCommandLists signal pairs with exactly one waitForCommandFence.
// Waiting on a slot without a pending signal blocks forever.
std::array<bool, kNumFrames> g_commandListSubmitted{false, false};
// Slot currently being recorded into, and the slot Present() will hand off
// to next. Advanced once per Present() call (guest PresentAndAdvanceFrame).
uint32_t g_frame = 0;
uint32_t g_nextFrame = 1;
uint32_t g_backBufferIndex = 0;
RenderWindow g_window{};

// One-shot GPU-lost latch (DEVICE_REMOVED / create failures observed
// anywhere, e.g. resource creation on guest threads). Once set, Present()
// and EnsureFrameStarted() bail out cheaply instead of continuing to poke a
// dead device/swapchain. Reset on the next successful Video::Init(); left
// set across Shutdown() otherwise.
std::atomic<bool> g_deviceLost{false};

// Present-storm coalescing: FM2's job-system pool can call Video::Present()
// far faster than the GPU retires frames. If a Present is already
// recording/submitting (on the render thread, via RenderQueue::Run), drop
// this call instead of piling up work behind it.
std::atomic<bool> g_presentBusy{false};

// Set by ExecuteCommandListImpl when a frame was submitted; consumed by the
// guest-thread PresentAndAdvanceFrame half.
bool g_presentSubmitOk = false;

// The guest's final front-buffer surface to blit this frame (D3DDevice_Swap).
fm2::render::GuestBaseTexture* g_presentSource = nullptr;

// Guest's most recent D3DDevice_ClearF color, used when there is no real
// present source yet (see Video::SetFallbackClearColor).
RenderColor g_fallbackClearColor{0.0f, 0.0f, 0.0f, 1.0f};

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

// Records the present blit into the open command list and submits it.
// Swapchain present + 2-frame advance run on the guest thread (Unleashed).
// Only ever runs on the dedicated render thread.
void ExecuteCommandListImpl() {
  std::lock_guard lock(fm2::render::RecordingMutex());
  g_presentSubmitOk = false;
  if (fm2::render::IsDeviceLost()) return;

  // The guest's draws/clears for this frame have already been recorded into
  // the open command list (targeting the guest's own render-target
  // surfaces). Make sure a frame is open even if the guest issued nothing.
  fm2::render::EnsureFrameStarted();
  if (!g_frameOpen) {
    return;  // acquire failed this frame
  }

  // Unleashed: ExecutePendingStretchRectCommands before present blit.
  fm2::render::FlushPendingStretchRectCommands();

  // Prefer resolve-assembled frontbuffer (Swap aperture lookup) over sticky
  // color RTs — tile bands land on the resolve dest, not the last SetRT.
  // Format-mismatched StretchRect leaves the aperture empty: prefer the
  // override (live scene RT) recorded when the copy was skipped.
  {
    static bool loggedFirstTarget = false;
    static uint64_t presentSourceChecks = 0;
    ++presentSourceChecks;
    fm2::render::GuestBaseTexture* rt = fm2::render::ConsumeStretchRectPresentOverride();
    const char* kind = "stretch-src";
    if (rt == nullptr || rt->texture == nullptr) {
      rt = fm2::render::ConsumeFrontbufferPresentSource();
      kind = "aperture";
    }
    if (rt == nullptr || rt->texture == nullptr) {
      rt = fm2::render::GetCurrentColorRenderTarget();
      kind = "sticky-rt";
    }
    if (rt != nullptr && !loggedFirstTarget) {
      loggedFirstTarget = true;
      REXGPU_INFO("ExecuteCommandList: first non-null present source ({}) after {} present(s)",
                  kind, presentSourceChecks);
    } else if (rt == nullptr && presentSourceChecks % 300 == 0) {
      REXGPU_WARN("ExecuteCommandList: still no render target after {} present(s)",
                  presentSourceChecks);
    }
    if (presentSourceChecks % 300 == 1 && rt != nullptr) {
      REXGPU_INFO("ExecuteCommandList: present kind={} {}x{} fmt={}", kind, rt->width, rt->height,
                  int(rt->format));
    }
    fm2::render::SetPresentSource(rt);
  }

  RenderCommandList* commandList = g_commandLists[g_frame].get();
  RenderTexture* backBuffer = g_swapChain->getTexture(g_backBufferIndex);
  RenderFramebuffer* framebuffer = g_framebuffers[g_backBufferIndex].get();

  commandList->setFramebuffer(nullptr);
  RenderPipeline* blitPipeline = fm2::render::GetBlitPipeline(kBackbufferFormat);
  const bool blit = g_presentSource != nullptr &&
                    g_presentSource->texture != nullptr &&
                    blitPipeline != nullptr;
  {
    static uint64_t presentCallCount = 0;
    ++presentCallCount;
    if (presentCallCount % 300 == 1) {
      if (blit) {
        REXGPU_INFO(
            "Video::Present: blitting present-source {}x{} format={} (present call {})",
            g_presentSource->width, g_presentSource->height, int(g_presentSource->format),
            presentCallCount);
      } else {
        REXGPU_WARN(
            "Video::Present: no blit this call (source={} texture={} pipeline={}) -- clearing to "
            "fallback color instead (present call {})",
            g_presentSource != nullptr,
            g_presentSource != nullptr && g_presentSource->texture != nullptr, blitPipeline != nullptr,
            presentCallCount);
      }
    }
  }
  if (blit) {
    if (g_presentSource->descriptorIndex == 0) {
      g_presentSource->descriptorIndex = fm2::render::AllocTextureDescriptor();
    }
    g_textureDescriptorSet->setTexture(
        g_presentSource->descriptorIndex, g_presentSource->texture,
        RenderTextureLayout::SHADER_READ, g_presentSource->textureView.get());

    RenderTextureBarrier toBlit[] = {
        RenderTextureBarrier(g_presentSource->texture, RenderTextureLayout::SHADER_READ),
        RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE),
    };
    commandList->barriers(RenderBarrierStage::GRAPHICS, toBlit, 2);
    g_presentSource->layout = RenderTextureLayout::SHADER_READ;

    const uint32_t descriptorIndex = g_presentSource->descriptorIndex;
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setPipeline(blitPipeline);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    commandList->setGraphicsPushConstants(0, &descriptorIndex, 0, sizeof(descriptorIndex));
    commandList->setFramebuffer(framebuffer);
    commandList->setViewports(RenderViewport(0.0f, 0.0f, float(g_swapChain->getWidth()),
                                             float(g_swapChain->getHeight())));
    commandList->setScissors(
        RenderRect(0, 0, g_swapChain->getWidth(), g_swapChain->getHeight()));
    commandList->drawInstanced(3, 1, 0, 0);
    commandList->barriers(RenderBarrierStage::GRAPHICS,
                          RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
  } else {
    // No usable front buffer yet: clear to the guest's most recent ClearF color.
    commandList->barriers(RenderBarrierStage::GRAPHICS,
                          RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE));
    commandList->setFramebuffer(framebuffer);
    commandList->clearColor(0, g_fallbackClearColor);
    commandList->barriers(RenderBarrierStage::GRAPHICS,
                          RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
  }
  commandList->end();
  g_frameOpen = false;
  g_presentSource = nullptr;

  RenderCommandSemaphore* waitSemaphores[] = {g_acquireSemaphores[g_frame].get()};
  RenderCommandSemaphore* signalSemaphores[] = {g_renderSemaphores[g_frame].get()};
  const RenderCommandList* submittedLists[] = {commandList};
  g_queue->executeCommandLists(submittedLists, 1, waitSemaphores, 1, signalSemaphores, 1,
                               g_commandFences[g_frame].get());
  g_commandListSubmitted[g_frame] = true;
  g_presentSubmitOk = true;
}

// Swapchain present + 2-frame advance. Runs on the render thread as part of
// the atomic present job (see ProcExecuteCommandList). Caller must hold
// RecordingMutex so guest threads cannot race g_frame.
void PresentAndAdvanceFrame() {
  if (!g_presentSubmitOk) return;

  RenderCommandSemaphore* signalSemaphores[] = {g_renderSemaphores[g_frame].get()};
  if (g_swapChainValid) {
    g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, 1);
  }

  // 2-frame pipelining: don't block on this slot's fence now. Advance to the
  // other slot -- only wait there if it still has an unretired submission
  // from two Presents ago.
  g_frame = g_nextFrame;
  g_nextFrame = (g_frame + 1) % kNumFrames;
  if (g_commandListSubmitted[g_frame]) {
    g_queue->waitForCommandFence(g_commandFences[g_frame].get());
    g_commandListSubmitted[g_frame] = false;
  }
  g_presentSubmitOk = false;
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

#if defined(_WIN32)
  // Opt-in GPU diagnostics: set FM2_GPU_DEBUG=1 to enable the D3D12 debug
  // layer and DRED (auto-breadcrumbs + page-fault tracking) BEFORE device
  // creation, so a later DEVICE_REMOVED names the exact faulting command.
  // Mirrors the old renderer's d3d12_debug cvar path
  // (src/ui/d3d12/d3d12_provider.cpp); NoteDeviceLost reads the results.
  {
    const char* dbg = std::getenv("FM2_GPU_DEBUG");
    if (dbg != nullptr && dbg[0] != '\0' && dbg[0] != '0') {
      ID3D12Debug* debugController = nullptr;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->Release();
        REXGPU_INFO("FM2_GPU_DEBUG: D3D12 debug layer enabled");
      } else {
        REXGPU_WARN("FM2_GPU_DEBUG: D3D12 debug layer unavailable");
      }
      ID3D12DeviceRemovedExtendedDataSettings* dredSettings = nullptr;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)))) {
        dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->Release();
        REXGPU_INFO("FM2_GPU_DEBUG: DRED breadcrumbs + page-fault tracking enabled");
      } else {
        REXGPU_WARN("FM2_GPU_DEBUG: DRED settings unavailable");
      }
    }
  }
#endif

  // Prefer D3D12 on Windows, fall back to Vulkan.
  using InterfaceFn = std::unique_ptr<RenderInterface> (*)();
  const std::array<InterfaceFn, 2> factories = {
      plume::CreateD3D12Interface,
      plume::CreateVulkanInterface,
  };

  g_d3d12Backend = false;
  for (size_t factoryIndex = 0; factoryIndex < factories.size(); ++factoryIndex) {
    g_interface = factories[factoryIndex]();
    if (g_interface == nullptr) {
      continue;
    }
    g_device = g_interface->createDevice();
    if (g_device != nullptr) {
      g_d3d12Backend = factoryIndex == 0;
      break;
    }
    g_interface.reset();
  }

  if (g_device == nullptr) {
    REXGPU_ERROR("Video::Init - failed to create a Plume render device");
    return false;
  }

#if defined(_WIN32)
  // With FM2_GPU_DEBUG on, keep the InfoQueue useful: FM2 clears RTs/DS every
  // frame without creation clear-values, and those two warnings (820/821) fire
  // hundreds of times, pushing the actual fatal ERROR out of the stored-message
  // window before NoteDeviceLost can dump it. Deny-list them at storage level.
  if (g_d3d12Backend && std::getenv("FM2_GPU_DEBUG") != nullptr) {
    auto* d3dDevice = static_cast<plume::D3D12Device*>(g_device.get());
    ID3D12InfoQueue* infoQueue = nullptr;
    if (d3dDevice->d3d != nullptr &&
        SUCCEEDED(d3dDevice->d3d->QueryInterface(IID_PPV_ARGS(&infoQueue))) &&
        infoQueue != nullptr) {
      D3D12_MESSAGE_ID denyIds[] = {
          D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
          D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
          // Post-failure fallout spam: once a frame fails to open, Procs
          // recording into the closed list emit thousands of these and push
          // the ORIGINAL removal-causing error out of the storage window.
          D3D12_MESSAGE_ID_COMMAND_LIST_CLOSED,
      };
      D3D12_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = uint32_t(std::size(denyIds));
      filter.DenyList.pIDList = denyIds;
      infoQueue->PushStorageFilter(&filter);
      infoQueue->Release();
      REXGPU_INFO("FM2_GPU_DEBUG: InfoQueue clear-value warning spam muted");
    }
  }
#endif

  REXGPU_INFO("Video::Init - device created: {}",
              g_device->getDescription().name);

  g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    g_commandLists[i] = g_queue->createCommandList();
    g_commandFences[i] = g_device->createCommandFence();
    g_acquireSemaphores[i] = g_device->createCommandSemaphore();
    g_renderSemaphores[i] = g_device->createCommandSemaphore();
  }
  g_frame = 0;
  g_nextFrame = 1;
  g_commandListSubmitted.fill(false);

  RenderSwapChainDesc swapChainDesc(g_window, kBackbufferFormat, 2);
  g_swapChain = g_queue->createSwapChain(swapChainDesc);
  // createSwapChain sizes from the HWND client rect; Video::Init's width/height
  // args are only the guest viewport hint. If they diverge, resize once now
  // (no framebuffers hold buffer refs yet) so EnsureFrameStarted does not
  // thrash ResizeBuffers every CommandList() open.
  if (g_swapChain->needsResize()) {
    g_swapChainValid = g_swapChain->resize();
  } else {
    g_swapChainValid = true;
  }
  if (g_swapChainValid) {
    Video::s_viewportWidth = g_swapChain->getWidth();
    Video::s_viewportHeight = g_swapChain->getHeight();
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
  g_blitMsaaPixelShader = g_device->createShader(
      g_copy_msaa_ps_dxil, sizeof(g_copy_msaa_ps_dxil), "main", RenderShaderFormat::DXIL);

  g_initialized = true;
  REXGPU_INFO("Video::Init - swapchain {}x{} valid={}", g_swapChain->getWidth(),
              g_swapChain->getHeight(), g_swapChainValid);

  // A fresh device is not lost; clear any latch left over from a prior
  // Init/Shutdown cycle.
  g_deviceLost.store(false, std::memory_order_release);

  fm2::render::RenderQueue::Start();
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    fm2::render::OnRecordingFrameReady(i);
  }
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

uint32_t CurrentRecordingFrame() { return g_frame; }

void SetPresentSource(GuestBaseTexture* frontBuffer) {
  g_presentSource = frontBuffer;
}

void EnsureFrameStarted() {
  if (g_frameOpen || !g_initialized || IsDeviceLost()) return;

  // After repeated ResizeBuffers failures (often DEVICE_REMOVED), stop
  // retrying every Clear/Draw/Present — that path WaitForGPU-spins and feels
  // like a startup hang.
  static int s_resizeFailStreak = 0;
  constexpr int kMaxResizeFails = 8;

  if ((!g_swapChainValid || g_swapChain->needsResize()) && s_resizeFailStreak < kMaxResizeFails) {
    // Drain the GPU (and any pending presentation) before resizing. Must not
    // wait on a slot's fence directly: Present() already consumed its signal.
    Video::WaitForGPU();
    // Drop framebuffer views that alias swapchain buffers *before* resize()
    // Releases those ID3D12Resources — otherwise the next submit/present hits
    // DXGI_ERROR_INVALID_CALL / DEVICE_REMOVED and the frame stays black.
    g_framebuffers.clear();
    g_swapChainValid = g_swapChain->resize();
    if (!g_swapChainValid) {
      ++s_resizeFailStreak;
      return;
    }
    s_resizeFailStreak = 0;
    Video::s_viewportWidth = g_swapChain->getWidth();
    Video::s_viewportHeight = g_swapChain->getHeight();
    RebuildFramebuffers();
  }
  if (!g_swapChainValid) return;
  if (!g_swapChain->acquireTexture(g_acquireSemaphores[g_frame].get(),
                                   &g_backBufferIndex)) {
    g_swapChainValid = false;
    return;
  }
  g_commandLists[g_frame]->begin();
  // Present() ends the list and clears plume's active root signature. Any
  // subsequent Clear/Draw that opens a frame via CommandList() must rebind
  // layout + descriptor sets before setGraphicsRootDescriptor.
  if (g_pipelineLayout != nullptr) {
    g_commandLists[g_frame]->setGraphicsPipelineLayout(g_pipelineLayout.get());
    if (g_textureDescriptorSet != nullptr) {
      g_commandLists[g_frame]->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
      g_commandLists[g_frame]->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 1);
      g_commandLists[g_frame]->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 2);
    }
    if (g_samplerDescriptorSet != nullptr) {
      g_commandLists[g_frame]->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    }
  }
  g_frameOpen = true;
}

std::recursive_mutex& RecordingMutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

RenderCommandList* CommandList() {
  EnsureFrameStarted();
  return g_commandLists[g_frame].get();
}

bool IsDeviceLost() { return g_deviceLost.load(std::memory_order_acquire); }

void NoteDeviceLost(const char* why) {
  if (!g_deviceLost.exchange(true, std::memory_order_acq_rel)) {
    REXGPU_ERROR("GPU device lost latch set ({})", why);
#if defined(_WIN32)
    // Plume also writes these details to stderr, but FM2 normally runs without
    // a visible console. Mirror the removal reason and the last severe debug
    // messages into the persistent game log used for manual QA.
    if (g_d3d12Backend && g_device != nullptr) {
      auto* device = static_cast<plume::D3D12Device*>(g_device.get());
      if (device->d3d != nullptr) {
        const HRESULT reason = device->d3d->GetDeviceRemovedReason();
        REXGPU_ERROR("D3D12 GetDeviceRemovedReason=0x{:08X}", uint32_t(reason));

        // DRED (populated only when FM2_GPU_DEBUG enabled it at Init): name
        // the exact op the GPU faulted on instead of just the HRESULT.
        ID3D12DeviceRemovedExtendedData* dred = nullptr;
        if (SUCCEEDED(device->d3d->QueryInterface(IID_PPV_ARGS(&dred))) && dred != nullptr) {
          D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs = {};
          if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput(&breadcrumbs))) {
            for (const D3D12_AUTO_BREADCRUMB_NODE* node = breadcrumbs.pHeadAutoBreadcrumbNode;
                 node != nullptr; node = node->pNext) {
              if (node->pLastBreadcrumbValue == nullptr || node->pCommandHistory == nullptr ||
                  *node->pLastBreadcrumbValue == 0 ||
                  *node->pLastBreadcrumbValue >= node->BreadcrumbCount) {
                continue;  // untouched or fully-retired list
              }
              REXGPU_ERROR("DRED breadcrumb: completed {} of {} ops", *node->pLastBreadcrumbValue,
                           node->BreadcrumbCount);
              const uint32_t last = *node->pLastBreadcrumbValue;
              const uint32_t start = last > 3 ? last - 3 : 0;
              const uint32_t end = std::min(last + 2, node->BreadcrumbCount);
              for (uint32_t i = start; i < end; ++i) {
                REXGPU_ERROR("  [{}] op type {}{}", i, int(node->pCommandHistory[i]),
                             i == last ? " <-- FAULT" : "");
              }
            }
          }
          D3D12_DRED_PAGE_FAULT_OUTPUT pageFault = {};
          if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&pageFault)) &&
              pageFault.PageFaultVA != 0) {
            REXGPU_ERROR("DRED page fault at VA 0x{:016X}", pageFault.PageFaultVA);
          }
          dred->Release();
        }

        ID3D12InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(device->d3d->QueryInterface(IID_PPV_ARGS(&infoQueue))) &&
            infoQueue != nullptr) {
          // Dump the FIRST stored messages too: the original invalid call that
          // triggered removal is at the head; the tail is usually thousands of
          // identical post-removal fallout lines (e.g. closed-command-list).
          const UINT64 messageCount = infoQueue->GetNumStoredMessages();
          const UINT64 headEnd = std::min<UINT64>(messageCount, 16);
          const UINT64 tailStart =
              messageCount > headEnd + 16 ? messageCount - 16 : headEnd;
          auto dumpMessage = [&](UINT64 index, const char* which) {
            SIZE_T messageSize = 0;
            infoQueue->GetMessage(index, nullptr, &messageSize);
            std::vector<uint8_t> bytes(messageSize);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
            if (FAILED(infoQueue->GetMessage(index, message, &messageSize)) ||
                message->Severity > D3D12_MESSAGE_SEVERITY_WARNING) {
              return;
            }
            REXGPU_ERROR("D3D12 InfoQueue[{} #{}] id={} severity={}: {}", which, index,
                         uint32_t(message->ID), uint32_t(message->Severity),
                         message->pDescription);
          };
          for (UINT64 index = 0; index < headEnd; ++index) dumpMessage(index, "head");
          for (UINT64 index = tailStart; index < messageCount; ++index) dumpMessage(index, "tail");
          infoQueue->Release();
        }
      }
    }
#endif
  }
}

RenderDescriptorSet* TextureDescriptorSet() {
  return g_textureDescriptorSet.get();
}

RenderDescriptorSet* SamplerDescriptorSet() {
  return g_samplerDescriptorSet.get();
}

RenderPipelineLayout* PipelineLayout() { return g_pipelineLayout.get(); }

RenderPipeline* GetBlitPipeline(RenderFormat format, bool multisampled) {
  if (g_device == nullptr) return nullptr;
  auto* pixelShader = multisampled ? g_blitMsaaPixelShader.get() : g_blitPixelShader.get();
  if (pixelShader == nullptr || g_blitVertexShader == nullptr) return nullptr;
  auto& pipeline = g_blitPipelines[(uint32_t(format) << 1) | uint32_t(multisampled)];
  if (pipeline == nullptr) {
    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = g_blitVertexShader.get();
    desc.pixelShader = pixelShader;
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

void ProcCopyBufferFromUpload(void* dst, void* src, uint64_t size) {
  if (dst == nullptr || src == nullptr || size == 0) return;
  auto* dstBuf = static_cast<RenderBuffer*>(dst);
  auto* srcBuf = static_cast<RenderBuffer*>(src);
  std::lock_guard lock(g_copyMutex);
  g_copyCommandList->begin();
  g_copyCommandList->copyBufferRegion(dstBuf->at(0), srcBuf->at(0), size);
  g_copyCommandList->end();
  g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyFence.get());
  g_copyQueue->waitForCommandFence(g_copyFence.get());
}

void ProcCopyTextureFromUpload(void* dst, void* src, uint32_t format, uint32_t width, uint32_t height,
                                uint32_t rowTexels, uint32_t mip, uint32_t arrayIndex,
                                uint64_t srcOffset) {
  const TextureUploadRegion region{width, height, rowTexels, mip, arrayIndex, srcOffset};
  ProcCopyTextureSubresourcesFromUpload(dst, src, format, &region, 1);
}

void ProcCopyTextureSubresourcesFromUpload(void* dst, void* src, uint32_t format,
                                           const TextureUploadRegion* regions,
                                           uint32_t regionCount) {
  if (dst == nullptr || src == nullptr || regions == nullptr || regionCount == 0) return;
  auto* dstTex = static_cast<RenderTexture*>(dst);
  auto* srcBuf = static_cast<RenderBuffer*>(src);
  const auto fmt = static_cast<RenderFormat>(format);
  std::lock_guard lock(g_copyMutex);
  g_copyCommandList->begin();
  g_copyCommandList->barriers(RenderBarrierStage::COPY,
                              RenderTextureBarrier(dstTex, RenderTextureLayout::COPY_DEST));
  for (uint32_t i = 0; i < regionCount; ++i) {
    const TextureUploadRegion& region = regions[i];
    g_copyCommandList->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(dstTex, region.mip, region.arrayIndex),
        RenderTextureCopyLocation::PlacedFootprint(srcBuf, fmt, region.width, region.height, 1,
                                                   region.rowTexels, region.srcOffset));
  }
  g_copyCommandList->end();
  g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyFence.get());
  g_copyQueue->waitForCommandFence(g_copyFence.get());
}

}  // namespace fm2::render

bool Video::Present(fm2::render::GuestBaseTexture* frontBuffer) {
  if (!g_initialized || fm2::render::IsDeviceLost()) {
    return false;
  }

#if defined(_WIN32)
  // Auto-capture: with the RenderDoc dll injected (renderdoccmd capture) and
  // FM2_RDOC_CAPTURE_AT=<frame>, trigger a one-frame capture at that Present.
  {
    static RENDERDOC_API_1_0_0* rdocApi = [] {
      RENDERDOC_API_1_0_0* api = nullptr;
      if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
        auto getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
        if (getApi != nullptr) getApi(eRENDERDOC_API_Version_1_0_0, reinterpret_cast<void**>(&api));
      }
      return api;
    }();
    static const uint64_t captureAt = [] {
      const char* v = std::getenv("FM2_RDOC_CAPTURE_AT");
      return v != nullptr ? std::strtoull(v, nullptr, 10) : 0ull;
    }();
    static uint64_t presentIndex = 0;
    ++presentIndex;
    if (presentIndex == 1) {
      REXGPU_INFO("RenderDoc probe: api={} captureAt={}", rdocApi != nullptr, captureAt);
    }
    if (rdocApi != nullptr && captureAt != 0 && presentIndex == captureAt) {
      rdocApi->TriggerCapture();
      REXGPU_INFO("RenderDoc: TriggerCapture at present {}", presentIndex);
    }
  }
#endif

  // Present-storm coalescing: FM2's job-system pool can call this far
  // faster than the GPU retires frames. If a Present is already recording/
  // submitting, drop this call rather than piling up work behind it.
  bool expected = false;
  if (!g_presentBusy.compare_exchange_strong(expected, true,
                                             std::memory_order_acq_rel)) {
    static uint64_t droppedBusyCount = 0;
    ++droppedBusyCount;
    if (droppedBusyCount % 300 == 1) {
      REXGPU_WARN(
          "Video::Present: dropped, previous present still in flight ({} dropped so far)",
          droppedBusyCount);
    }
    return false;
  }

  // A dropped Swap must not replace the frontbuffer of the frame that won
  // admission. A null lookup deliberately preserves the existing sticky
  // fallback selected by ExecuteCommandListImpl.
  if (frontBuffer != nullptr && frontBuffer->texture != nullptr) {
    fm2::render::SetFrontbufferPresentSource(frontBuffer);
  }

  // One atomic render-thread job: submit + swapchain present + slot advance +
  // frame reopen (see ProcExecuteCommandList). Present/advance must not run on
  // the guest thread: any Enqueue'd job that lands between submit and present
  // reopens the frame against a stale back-buffer index and D3D12 removes the
  // device (INVALID_CALL, "not the current back buffer").
  fm2::render::RenderCommand cmd{};
  cmd.type = fm2::render::RenderCommandType::ExecuteCommandList;
  fm2::render::RenderQueue::Run(cmd);

  g_presentBusy.store(false, std::memory_order_release);
  return true;
}

void Video::WaitForGPU() {
  if (!g_initialized) {
    return;
  }
  // Hold RecordingMutex for the *whole* call, not just inside the lambda
  // below: the lambda may run synchronously on the render thread while this
  // thread blocks inside RenderQueue::Run, so re-locking a recursive_mutex
  // from that other thread would deadlock against a caller that already
  // holds it on this thread (e.g. EnsureFrameStarted's resize-fail path,
  // reached from a guest thread that's already holding the lock via
  // CommandList()).
  std::lock_guard lock(fm2::render::RecordingMutex());
  fm2::render::RenderCommand cmd{};
  cmd.type = fm2::render::RenderCommandType::WaitForGpu;
  fm2::render::RenderQueue::Run(cmd);
}

namespace fm2::render {

void ProcExecuteCommandList() {
  // Atomic frame transaction, all on the render thread: submit the frame,
  // present the swapchain, advance the slot, and reopen bookkeeping. Splitting
  // these across guest/render threads let a guest-side Enqueue land between
  // submit and present: it opened the NEXT recording frame against the OLD
  // g_backBufferIndex, and D3D12 kills the device with DXGI_ERROR_INVALID_CALL
  // ("attempted write buffer is not the current back buffer") -> the
  // CreateSurface device-lost latch after the press-start screen.
  ExecuteCommandListImpl();
  {
    std::lock_guard lock(RecordingMutex());
    PresentAndAdvanceFrame();
  }
  // Safe to reuse this slot's upload scratch now that its prior GPU work
  // has retired (Unleashed BeginCommandList / allocator reset).
  OnRecordingFrameReady(g_frame);
  // Restore per-frame dirty/descriptor/frame-index bookkeeping that Swap used
  // to drive via BeginRenderStateFrame (removed to avoid nested-Run freezes).
  fm2::render::NotifyRenderFrameBegin();
}

void ProcBeginCommandList() {
  // Kept for command compatibility; the work now happens at the end of
  // ProcExecuteCommandList so present/advance/reopen cannot interleave with
  // guest-enqueued jobs.
}

void ProcWaitForGpu() {
  // Finish any open recording list before Reset/begin — otherwise we Reset an
  // open allocator and later ExecuteCommandLists reports "must be closed".
  if (g_frameOpen) {
    RenderCommandList* openList = g_commandLists[g_frame].get();
    openList->end();
    g_frameOpen = false;
    g_queue->executeCommandLists(openList, g_commandFences[g_frame].get());
    g_commandListSubmitted[g_frame] = true;
  }

  for (uint32_t i = 0; i < kNumFrames; ++i) {
    if (g_commandListSubmitted[i]) {
      g_queue->waitForCommandFence(g_commandFences[i].get());
      g_commandListSubmitted[i] = false;
    }
    OnRecordingFrameReady(i);
  }

  // Empty command list drain (Unleashed uses slot 0) — both slots are closed.
  g_commandLists[0]->begin();
  g_commandLists[0]->end();
  g_queue->executeCommandLists(g_commandLists[0].get(), g_commandFences[0].get());
  g_queue->waitForCommandFence(g_commandFences[0].get());
}

}  // namespace fm2::render

void Video::Shutdown() {
  if (!g_initialized) {
    return;
  }
  StopVsyncWorker();
  // Stop dispatching to (and drain/join) the render thread before the final
  // full-GPU drain below, so WaitForGPU()'s RenderQueue::Run falls back to
  // running inline once the queue is torn down (see its Init/Shutdown-edge
  // comment in render_queue.cpp).
  fm2::render::RenderQueue::Stop();
  WaitForGPU();
  g_blitPipelines.clear();
  g_blitPixelShader.reset();
  g_blitMsaaPixelShader.reset();
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
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    g_renderSemaphores[i].reset();
    g_acquireSemaphores[i].reset();
    g_commandFences[i].reset();
    g_commandLists[i].reset();
  }
  g_queue.reset();
  g_device.reset();
  g_interface.reset();
  g_d3d12Backend = false;
  g_initialized = false;
}
