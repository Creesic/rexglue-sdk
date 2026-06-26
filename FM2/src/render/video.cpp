

#include "video.h"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <plume_render_interface.h>
#include <plume_render_interface_builders.h>
#include <rex/cvar.h>
#include <rex/logging.h>

#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_state.h"
#include "render/shaders/copy_ps.hlsl.dxil.h"
#include "render/shaders/copy_vs.hlsl.dxil.h"

using namespace plume;

namespace plume {
// Backend factories (defined in thirdparty/plume/plume_d3d12.cpp,
// plume_vulkan.cpp).
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
extern std::unique_ptr<RenderInterface>
CreateVulkanInterface(RenderWindow sdlWindow);
extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
} // namespace plume

namespace fm2::render {
void BeginRenderStateFrame();          // render_state.cpp
void FlushPendingResolvesForPresent(); // render_state.cpp
void EnsureFrameStarted();             // defined below
} // namespace fm2::render

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
fm2::render::GuestBaseTexture *g_presentSource = nullptr;

// Sole present owner thread (0 = unclaimed). FM2's job-system pool drives present
// from ~44 threads racing on the one global command list; once claimed, Present()
// from any other thread is dropped. Stored as a hashed thread id (portable, no
// <windows.h> dependency here).
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
REXCVAR_DEFINE_BOOL(fm2_plume_video_present_trace, true, "FM2",
                    "Emit bounded diagnostics from the active Plume present path.");
REXCVAR_DEFINE_UINT32(
    fm2_plume_video_present_trace_limit, 128, "FM2",
    "Maximum Plume present trace lines to emit in one run; 0 is unlimited.");

uint32_t NextVideoPresentTraceIndex(uint32_t &counter) {
  if (!REXCVAR_GET(fm2_plume_video_present_trace)) {
    return 0;
  }
  const uint32_t index = ++counter;
  const uint32_t limit = REXCVAR_GET(fm2_plume_video_present_trace_limit);
  if (limit != 0 && index > limit) {
    return 0;
  }
  return index;
}

void RebuildFramebuffers() {
  g_framebuffers.clear();
  const uint32_t count = g_swapChain->getTextureCount();
  g_framebuffers.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    const RenderTexture *color = g_swapChain->getTexture(i);
    RenderFramebufferDesc desc(&color, 1);
    g_framebuffers[i] = g_device->createFramebuffer(desc);
  }
}

} // namespace

bool Video::Init(void *nativeWindowHandle, uint32_t width, uint32_t height) {
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
      static_cast<InterfaceFn>([]() { return plume::CreateVulkanInterface(); }),
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
  return true;
}

bool Video::IsInitialized() { return g_initialized; }

namespace fm2::render {

RenderInterface *Interface() { return g_interface.get(); }
RenderDevice *Device() { return g_device.get(); }

void SetPresentSource(GuestBaseTexture *frontBuffer) {
  g_presentSource = frontBuffer;
  static bool s_logged = false;
  if (!s_logged && frontBuffer != nullptr) {
    s_logged = true;
    REXGPU_INFO("Present source: {}x{} format={} (swapchain {}x{} format={})",
                frontBuffer->width, frontBuffer->height,
                int(frontBuffer->format), g_swapChain->getWidth(),
                g_swapChain->getHeight(), int(kBackbufferFormat));
  }
}

void EnsureFrameStarted() {
  if (g_frameOpen || !g_initialized)
    return;

  if (!g_swapChainValid || g_swapChain->needsResize()) {
    // Drain the GPU (and any pending presentation) before resizing. Must not
    // wait on g_commandFence directly: Present() already consumed its signal.
    Video::WaitForGPU();
    g_swapChainValid = g_swapChain->resize();
    if (!g_swapChainValid)
      return;
    RebuildFramebuffers();
  }
  if (!g_swapChain->acquireTexture(g_acquireSemaphore.get(),
                                   &g_backBufferIndex)) {
    g_swapChainValid = false;
    return;
  }
  g_commandList->begin();
  g_frameOpen =
      true; // set before BeginRenderStateFrame (it calls CommandList())
  BeginRenderStateFrame();
}

RenderCommandList *CommandList() {
  EnsureFrameStarted();
  return g_commandList.get();
}

RenderDescriptorSet *TextureDescriptorSet() {
  return g_textureDescriptorSet.get();
}

RenderDescriptorSet *SamplerDescriptorSet() {
  return g_samplerDescriptorSet.get();
}

RenderPipelineLayout *PipelineLayout() { return g_pipelineLayout.get(); }

RenderPipeline *GetBlitPipeline(RenderFormat format) {
  if (g_device == nullptr)
    return nullptr;
  auto &pipeline = g_blitPipelines[uint32_t(format)];
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
  if (index < kNullTextureDescriptorCount)
    return;
  std::lock_guard lock(g_descriptorMutex);
  g_freedDescriptors.push_back(index);
}

void ExecuteUpload(const std::function<void(RenderCommandList *)> &record) {
  std::lock_guard lock(g_copyMutex);
  g_copyCommandList->begin();
  record(g_copyCommandList.get());
  g_copyCommandList->end();
  g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyFence.get());
  g_copyQueue->waitForCommandFence(g_copyFence.get());
}

// ---------------------------------------------------------------------------
// Draw-to-screen TEST GRID (diagnostic). Each cell tests a different mechanism
// so we can SEE which actually produces pixels on the real backbuffer:
//   - 5 solid-color swatches (red/green/blue/white/yellow) via clearColor:
//       baseline -- proves we can write the swapchain at all + color channels.
//   - 1 procedural checkerboard texture we create+upload+sample via the blit
//       pipeline: proves OUR texture upload->sample path works independent of
//       the game (if this cell is black, all our texture uploads are broken).
// Drawn over the final backbuffer just before present.
// ---------------------------------------------------------------------------
std::unique_ptr<RenderTexture> g_testTex;
std::unique_ptr<RenderTextureView> g_testTexView;
uint32_t g_testTexDesc = 0;
bool g_showPresentTestGrid = false; // diagnostic overlay off; use RenderDoc to inspect

void EnsureTestTexture() {
  if (g_testTex != nullptr || g_device == nullptr)
    return;
  const uint32_t W = 64, H = 64;
  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = W;
  desc.height = H;
  desc.depth = 1;
  desc.mipLevels = 1;
  desc.arraySize = 1;
  desc.format = RenderFormat::B8G8R8A8_UNORM;
  desc.flags = RenderTextureFlag::NONE;
  g_testTex = g_device->createTexture(desc);
  if (g_testTex == nullptr)
    return;
  RenderTextureViewDesc vdesc;
  vdesc.format = RenderFormat::B8G8R8A8_UNORM;
  vdesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  vdesc.mipLevels = 1;
  g_testTexView = g_testTex->createTextureView(vdesc);

  std::vector<uint32_t> px(size_t(W) * H);
  for (uint32_t y = 0; y < H; ++y)
    for (uint32_t x = 0; x < W; ++x)
      px[y * W + x] = (((x >> 3) + (y >> 3)) & 1) ? 0xFFFF00FFu  // magenta
                                                  : 0xFF00FFFFu; // cyan
  const uint32_t rowPitch = W * 4;
  const uint32_t dstPitch = (rowPitch + 255u) & ~255u;
  auto upload = g_device->createBuffer(
      RenderBufferDesc::UploadBuffer(size_t(dstPitch) * H));
  auto *mapped = reinterpret_cast<uint8_t *>(upload->map());
  for (uint32_t y = 0; y < H; ++y)
    std::memcpy(mapped + size_t(y) * dstPitch,
                reinterpret_cast<uint8_t *>(px.data()) + size_t(y) * rowPitch,
                rowPitch);
  upload->unmap();
  ExecuteUpload([&](RenderCommandList *cl) {
    cl->barriers(RenderBarrierStage::COPY,
                 RenderTextureBarrier(g_testTex.get(),
                                      RenderTextureLayout::COPY_DEST));
    cl->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(g_testTex.get(), 0),
        RenderTextureCopyLocation::PlacedFootprint(
            upload.get(), RenderFormat::B8G8R8A8_UNORM, W, H, 1, dstPitch / 4));
  });
  g_testTexDesc = AllocTextureDescriptor();
  g_textureDescriptorSet->setTexture(g_testTexDesc, g_testTex.get(),
                                     RenderTextureLayout::SHADER_READ,
                                     g_testTexView.get());
}

void DrawPresentTestGrid(RenderCommandList *cl, RenderTexture *backBuffer,
                         RenderFramebuffer *framebuffer, uint32_t sw,
                         uint32_t sh) {
  if (cl == nullptr || backBuffer == nullptr || framebuffer == nullptr)
    return;
  // backBuffer is in PRESENT layout on entry; flip to COLOR_WRITE to draw.
  cl->barriers(RenderBarrierStage::GRAPHICS,
               RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE));
  cl->setFramebuffer(framebuffer);

  const int cellW = int(sw) / 6;
  const int cellH = int(sh) / 8;
  const RenderColor colors[5] = {
      RenderColor(1, 0, 0, 1), RenderColor(0, 1, 0, 1), RenderColor(0, 0, 1, 1),
      RenderColor(1, 1, 1, 1), RenderColor(1, 1, 0, 1)};
  for (int i = 0; i < 5; ++i) {
    RenderRect r(i * cellW + 4, 4, (i + 1) * cellW - 4, cellH); // l,t,r,b
    cl->clearColor(0, colors[i], &r, 1);
  }

  RenderPipeline *blit = GetBlitPipeline(kBackbufferFormat);
  auto blitDescToCell = [&](RenderTexture *tex, uint32_t desc, float x, float y,
                            float w, float h) {
    if (tex == nullptr || blit == nullptr)
      return;
    cl->barriers(RenderBarrierStage::GRAPHICS,
                 RenderTextureBarrier(tex, RenderTextureLayout::SHADER_READ));
    cl->setGraphicsPipelineLayout(g_pipelineLayout.get());
    cl->setPipeline(blit);
    cl->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    cl->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    cl->setGraphicsPushConstants(0, &desc, 0, sizeof(desc));
    cl->setFramebuffer(framebuffer);
    cl->setViewports(RenderViewport(x, y, w, h));
    cl->setScissors(RenderRect(int(x), int(y), int(x + w), int(y + h)));
    cl->drawInstanced(3, 1, 0, 0);
  };
  auto blitGuestToCell = [&](GuestBaseTexture *g, float x, float y, float w,
                             float h) {
    if (g == nullptr || g->texture == nullptr || g->textureView == nullptr)
      return;
    if (g->descriptorIndex == 0)
      g->descriptorIndex = AllocTextureDescriptor();
    g_textureDescriptorSet->setTexture(g->descriptorIndex, g->texture,
                                       RenderTextureLayout::SHADER_READ,
                                       g->textureView.get());
    blitDescToCell(g->texture, g->descriptorIndex, x, y, w, h);
    g->layout = RenderTextureLayout::SHADER_READ;
  };

  const float ry = float(cellH + 8), rh = float(2 * cellH);
  const int ity = cellH + 8, ith = 2 * cellH;
  // Marker fills: if a cell stays its marker color, that source pointer was
  // null (cell never blitted); if it goes black, the source is valid-but-black.
  RenderRect mB(cellW, ity, 2 * cellW, ity + ith);   // orange = RT marker
  RenderRect mC(2 * cellW, ity, 3 * cellW, ity + ith); // purple = tex marker
  cl->clearColor(0, RenderColor(1.0f, 0.5f, 0.0f, 1.0f), &mB, 1);
  cl->clearColor(0, RenderColor(0.6f, 0.0f, 0.8f, 1.0f), &mC, 1);

  // Cell A: our procedural checkerboard (proven to work as a control).
  EnsureTestTexture();
  blitDescToCell(g_testTex.get(), g_testTexDesc, 0.0f, ry, float(cellW), rh);
  // Cell B: the game's last-drawn color render target (does the game draw?).
  //   orange remains => null pointer; black => valid but the RT is black.
  blitGuestToCell(GetLastDrawnColorRenderTarget(), float(cellW), ry,
                  float(cellW), rh);
  // Cell C: an actual game texture (tiled upload -- the prime suspect).
  //   purple remains => null pointer; black/garbage => upload/detile result.
  blitGuestToCell(GetTestGameTexture(), float(2 * cellW), ry, float(cellW), rh);

  // Bottom row: the 6 most-recent distinct color render targets, so we can see
  // which surface actually holds the rendered scene/UI content (present-source).
  const float by = ry + rh + 8.0f;
  const float bh = float(cellH * 2);
  for (uint32_t i = 0; i < 6u; ++i) {
    const float bx = float(int(i) * cellW);
    RenderRect mark(int(bx), int(by), int(bx) + cellW, int(by + bh));
    cl->clearColor(0, RenderColor(0.0f, 0.3f, 0.3f, 1.0f), &mark, 1); // teal marker
    blitGuestToCell(GetRecentColorRenderTarget(i), bx, by, float(cellW), bh);
  }

  cl->barriers(RenderBarrierStage::GRAPHICS,
               RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
}

} // namespace fm2::render

void Video::ClaimPresentOwner() {
  uint64_t expected = 0;
  g_presentOwnerKey.compare_exchange_strong(expected, CurrentThreadKey(),
                                            std::memory_order_acq_rel);
}

void Video::Present() {
  static uint32_t s_traceCount = 0;
  static std::atomic<uint64_t> s_callCount{0};
  const uint64_t callN = ++s_callCount;
  if (callN <= 5) {
    // Write first few calls to the diagnostic log so we know this is reached.
    if (FILE* f = fopen("C:\\temp\\fm2-videopresent.log", "a")) {
      fprintf(f, "Video::Present n=%llu init=%d swapValid=%d frameOpen=%d src=%p\n",
              (unsigned long long)callN, (int)g_initialized, (int)g_swapChainValid,
              (int)g_frameOpen, (void*)g_presentSource);
      fclose(f);
    }
  }
  // Drop present calls from FM2's other job-system threads: only the latched
  // owner (the real GPU-submit thread) may begin/end/submit the single global
  // command list and drive the swapchain. Before an owner is claimed, allow the
  // call (early boot). Done before the tally below so the open/closed counts
  // reflect only the owner's frames. See fm2_plume_single_thread_present.
  if (REXCVAR_GET(fm2_plume_single_thread_present)) {
    const uint64_t owner = g_presentOwnerKey.load(std::memory_order_acquire);
    if (owner != 0 && owner != CurrentThreadKey()) {
      return;
    }
  }
  // Per-second tally of frameOpen-at-entry (proven clean-log channel). Tells us
  // in steady state whether guest draws opened the frame before present, or the
  // frame is only ever opened inside present (=> empty blit => black screen).
  {
    static std::atomic<uint64_t> s_open{0};
    static std::atomic<uint64_t> s_closed{0};
    static std::atomic<uint64_t> s_lastSec{0};
    (g_frameOpen ? s_open : s_closed).fetch_add(1, std::memory_order_relaxed);
    using clock = std::chrono::steady_clock;
    const uint64_t nowSec = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            clock::now().time_since_epoch())
            .count());
    uint64_t last = s_lastSec.load(std::memory_order_relaxed);
    if (last != 0 && nowSec != last &&
        s_lastSec.compare_exchange_strong(last, nowSec,
                                          std::memory_order_relaxed)) {
      const uint64_t open = s_open.exchange(0, std::memory_order_relaxed);
      const uint64_t closed = s_closed.exchange(0, std::memory_order_relaxed);
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_PRESENT_FRAMEOPEN sec=%llu open=%llu closed=%llu\n",
                     (unsigned long long)nowSec, (unsigned long long)open,
                     (unsigned long long)closed);
        std::fflush(f);
        std::fclose(f);
      }
    } else if (last == 0) {
      s_lastSec.store(nowSec, std::memory_order_relaxed);
    }
  }

  const uint32_t traceIndex = NextVideoPresentTraceIndex(s_traceCount);
  if (traceIndex != 0) {
    REXGPU_INFO("FM2_PLUME_VIDEO_PRESENT n={} stage=entry initialized={} "
                "swapchainValid={} frameOpen={} presentSource={} "
                "presentTexture={}",
                traceIndex, g_initialized, g_swapChainValid, g_frameOpen,
                static_cast<const void *>(g_presentSource),
                g_presentSource ? static_cast<const void *>(g_presentSource->texture)
                                : nullptr);
  }

  if (!g_initialized) {
    return;
  }

  // The guest's draws/clears for this frame have already been recorded into the
  // open command list (targeting the guest's own render-target surfaces). Make
  // sure a frame is open even if the guest issued nothing.
  fm2::render::EnsureFrameStarted();
  if (!g_frameOpen) {
    if (traceIndex != 0) {
      REXGPU_INFO("FM2_PLUME_VIDEO_PRESENT n={} stage=no_frame "
                  "swapchainValid={} frameOpen={}",
                  traceIndex, g_swapChainValid, g_frameOpen);
    }
    return; // acquire failed this frame
  }

  fm2::render::FlushPendingResolvesForPresent();

  RenderTexture *backBuffer = g_swapChain->getTexture(g_backBufferIndex);
  RenderFramebuffer *framebuffer = g_framebuffers[g_backBufferIndex].get();

  g_commandList->setFramebuffer(nullptr);
  RenderPipeline *blitPipeline =
      fm2::render::GetBlitPipeline(kBackbufferFormat);
  const bool blit = g_presentSource != nullptr &&
                    g_presentSource->texture != nullptr &&
                    blitPipeline != nullptr;
  const fm2::render::GuestBaseTexture *presentSource = g_presentSource;
  const bool hadPresentTexture =
      presentSource != nullptr && presentSource->texture != nullptr;
  const char *presentPath = blit ? "blit" : "clear";
  if (traceIndex != 0) {
    REXGPU_INFO("FM2_PLUME_VIDEO_PRESENT n={} stage=record path={} "
                "backBufferIndex={} backBuffer={} framebuffer={} "
                "source={} sourceTexture={} sourceSize={}x{} sourceFormat={} "
                "sourceDescriptor={} blitPipeline={}",
                traceIndex, presentPath, g_backBufferIndex,
                static_cast<const void *>(backBuffer),
                static_cast<const void *>(framebuffer),
                static_cast<const void *>(presentSource),
                hadPresentTexture ? static_cast<const void *>(presentSource->texture)
                                  : nullptr,
                presentSource ? presentSource->width : 0,
                presentSource ? presentSource->height : 0,
                presentSource ? int(presentSource->format) : 0,
                presentSource ? presentSource->descriptorIndex : 0,
                static_cast<const void *>(blitPipeline));
  }
  if (blit) {
    if (g_presentSource->descriptorIndex == 0) {
      g_presentSource->descriptorIndex =
          fm2::render::AllocTextureDescriptor();
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
    // No usable front buffer: clear so we still present.
    g_commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(backBuffer, RenderTextureLayout::COLOR_WRITE));
    g_commandList->setFramebuffer(framebuffer);
    g_commandList->clearColor(0, RenderColor(0.10f, 0.20f, 0.40f, 1.0f));
    g_commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(backBuffer, RenderTextureLayout::PRESENT));
  }
  // Diagnostic draw-to-screen test grid (off by default now that real content
  // renders; flip g_showPresentTestGrid to re-enable for debugging).
  if (fm2::render::g_showPresentTestGrid) {
    fm2::render::DrawPresentTestGrid(g_commandList.get(), backBuffer,
                                     framebuffer, g_swapChain->getWidth(),
                                     g_swapChain->getHeight());
  }
  g_commandList->end();
  g_frameOpen = false;
  g_presentSource = nullptr;

  RenderCommandSemaphore *waitSemaphores[] = {g_acquireSemaphore.get()};
  RenderCommandSemaphore *signalSemaphores[] = {g_renderSemaphore.get()};
  const RenderCommandList *commandLists[] = {g_commandList.get()};
  g_queue->executeCommandLists(commandLists, 1, waitSemaphores, 1,
                               signalSemaphores, 1, g_commandFence.get());
  g_commandsInFlight = true;

  g_swapChainValid =
      g_swapChain->present(g_backBufferIndex, signalSemaphores, 1);
  if (traceIndex != 0) {
    REXGPU_INFO("FM2_PLUME_VIDEO_PRESENT n={} stage=presented path={} "
                "swapchainValid={} backBufferIndex={}",
                traceIndex, presentPath, g_swapChainValid, g_backBufferIndex);
  }

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
