// render/video.cpp
//
// Phase 1: bring up a Plume device + swapchain and flip once per guest
// D3DDevice_Swap. Phase 2 (PresentGuestFrame): show the guest's own front
// buffer instead of a clear -- read its Xenos-encoded memory, untile and
// convert to the swapchain format, upload, and copy onto the acquired image.
// The host draws no guest geometry yet; whatever the guest's command stream
// left in that buffer is what appears.

#include "video.h"

#include <array>
#include <chrono>
#include <cstring>
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

// Xenos texture formats this slice can read back (xenos::TextureFormat).
constexpr uint32_t kFormat_8_8_8_8 = 6;
constexpr uint32_t kFormat_2_10_10_10 = 7;
constexpr uint32_t kFormat_8_8_8_8_A = 14;
// The "_AS_16_16_16_16" variants are sampled as 16-bit but stored exactly like
// their base format; PGR4's front buffer is the 2_10_10_10 one (54).
constexpr uint32_t kFormat_8_8_8_8_AS_16 = 50;
constexpr uint32_t kFormat_2_10_10_10_AS_16 = 54;

inline bool IsPacked2_10_10_10(uint32_t format) {
  return format == kFormat_2_10_10_10 || format == kFormat_2_10_10_10_AS_16;
}
inline bool IsSupportedFrontBufferFormat(uint32_t format) {
  return format == kFormat_8_8_8_8 || format == kFormat_8_8_8_8_A ||
         format == kFormat_8_8_8_8_AS_16 || IsPacked2_10_10_10(format);
}

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

  // Phase 2: CPU-visible staging for the converted guest frame. Per slot so a
  // frame still being copied by the GPU is never overwritten by the next one.
  std::unique_ptr<RenderBuffer> upload;
  uint64_t uploadSize = 0;

  bool submitted = false;
};

std::unique_ptr<RenderInterface> g_interface;
std::unique_ptr<RenderDevice> g_device;
std::unique_ptr<RenderCommandQueue> g_queue;
std::unique_ptr<RenderSwapChain> g_swapChain;
std::array<FrameSlot, kNumFrames> g_frames;

// One framebuffer per swapchain image, built lazily and dropped on resize.
std::vector<std::unique_ptr<RenderFramebuffer>> g_framebuffers;

// Phase 2: the guest frame after conversion, in swapchain format, sized to
// the guest surface. Recreated if the guest surface geometry changes.
std::unique_ptr<RenderTexture> g_guestFrame;
uint32_t g_guestFrameWidth = 0;
uint32_t g_guestFrameHeight = 0;

// Scratch for the CPU-side conversion, reused across frames.
std::vector<uint32_t> g_convert;

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

// Resize / empty-swapchain / slot-fence preamble shared by both present paths,
// then acquire an image. Returns false if the frame should be dropped.
bool BeginFrame(FrameSlot*& slot, uint32_t& textureIndex) {
  if (g_swapChain->needsResize()) {
    // Framebuffers reference the old images, so they must go before the
    // resize, not after it.
    Video::WaitForGPU();
    ReleaseFramebuffers();
    if (!g_swapChain->resize()) {
      return false;
    }
    Video::s_viewportWidth = g_swapChain->getWidth();
    Video::s_viewportHeight = g_swapChain->getHeight();
  }
  if (g_swapChain->isEmpty()) {
    return false;
  }

  slot = &g_frames[g_frameIndex];
  if (slot->submitted) {
    // Waiting on this slot's fence also guarantees its semaphores' previous
    // signal/wait pair has fully retired before we reuse them, and that the
    // GPU is done reading its upload buffer.
    g_queue->waitForCommandFence(slot->fence.get());
    slot->submitted = false;
  }

  return g_swapChain->acquireTexture(slot->acquireSemaphore.get(), &textureIndex);
}

// Submit the slot's command list and present. Execution waits for the image
// to be acquired and signals when done; present waits on that signal. On
// Vulkan present would otherwise run before the work -- VulkanSwapChain::present
// honours these semaphores, D3D12SwapChain::present ignores them.
bool EndFrame(FrameSlot& slot, uint32_t textureIndex) {
  const RenderCommandList* lists[] = {slot.commandList.get()};
  RenderCommandSemaphore* waits[] = {slot.acquireSemaphore.get()};
  RenderCommandSemaphore* signals[] = {slot.renderFinishedSemaphore.get()};
  g_queue->executeCommandLists(lists, 1, waits, 1, signals, 1, slot.fence.get());
  slot.submitted = true;

  const bool presented = g_swapChain->present(textureIndex, signals, 1);
  g_frameIndex = (g_frameIndex + 1) % kNumFrames;
  return presented;
}

// Xbox 360 2D tiled addressing (XGAddress2DTiledOffset): 32x32-texel macro
// tiles of 8x2 micro tiles. log2_bpp is log2 of bytes per texel.
inline uint32_t TiledOffset2D(uint32_t x, uint32_t y, uint32_t width, uint32_t log2_bpp) {
  const uint32_t macro = ((x >> 5) + (y >> 5) * ((width + 31) >> 5)) << (log2_bpp + 7);
  const uint32_t micro = ((x & 7) + ((y & 6) << 2)) << log2_bpp;
  const uint32_t offset = macro + ((micro & ~15u) << 1) + (micro & 15) +
                          ((y & 8) << (3 + log2_bpp)) + ((y & 1) << 4);
  return ((offset & ~511u) << 3) + ((offset & 448) << 2) + (offset & 63) + ((y & 16) << 7) +
         (((((y & 8) >> 2) + (x >> 3)) & 3) << 6);
}

inline uint32_t Swap8In32(uint32_t v) {
  return (v >> 24) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) | (v << 24);
}
inline uint32_t Swap8In16(uint32_t v) {
  return ((v >> 8) & 0x00FF00FFu) | ((v << 8) & 0xFF00FF00u);
}
inline uint32_t Swap16In32(uint32_t v) { return (v >> 16) | (v << 16); }

// One guest texel dword with the Xenos memory endianness undone.
inline uint32_t ReadTexel(const uint8_t* data, uint32_t byteOffset, uint32_t endian) {
  uint32_t v;
  std::memcpy(&v, data + byteOffset, sizeof(v));
  switch (endian) {
    case 1:
      return Swap8In16(v);
    case 2:
      return Swap8In32(v);
    case 3:
      return Swap16In32(v);
    default:
      return v;
  }
}

// Xenos 8_8_8_8 texels are R,G,B,A in byte order once endianness is undone;
// the swapchain wants B8G8R8A8. 2_10_10_10 is packed R:10 G:10 B:10 A:2.
inline uint32_t ToBgra8(uint32_t texel, uint32_t format) {
  if (IsPacked2_10_10_10(format)) {
    const uint32_t r = (texel >> 2) & 0xFF;
    const uint32_t g = (texel >> 12) & 0xFF;
    const uint32_t b = (texel >> 22) & 0xFF;
    const uint32_t a = ((texel >> 30) & 0x3) * 85;
    return b | (g << 8) | (r << 16) | (a << 24);
  }
  const uint32_t r = texel & 0xFF;
  const uint32_t g = (texel >> 8) & 0xFF;
  const uint32_t b = (texel >> 16) & 0xFF;
  const uint32_t a = (texel >> 24) & 0xFF;
  return b | (g << 8) | (r << 16) | (a << 24);
}

bool EnsureGuestFrameResources(FrameSlot& slot, uint32_t width, uint32_t height) {
  const uint64_t bytes = uint64_t(width) * height * 4;
  if (slot.upload == nullptr || slot.uploadSize < bytes) {
    slot.upload = g_device->createBuffer(RenderBufferDesc::UploadBuffer(bytes));
    slot.uploadSize = bytes;
    if (slot.upload == nullptr) {
      REXLOG_ERROR("PGR4 Plume: upload buffer ({} bytes) creation failed", bytes);
      return false;
    }
  }
  if (g_guestFrame == nullptr || g_guestFrameWidth != width || g_guestFrameHeight != height) {
    Video::WaitForGPU();  // nothing may still be copying from the old texture
    g_guestFrame =
        g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, kSwapChainFormat));
    g_guestFrameWidth = width;
    g_guestFrameHeight = height;
    if (g_guestFrame == nullptr) {
      REXLOG_ERROR("PGR4 Plume: guest frame texture {}x{} creation failed", width, height);
      return false;
    }
    REXLOG_INFO("PGR4 Plume: guest frame texture {}x{} created", width, height);
  }
  return true;
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

  FrameSlot* slot = nullptr;
  uint32_t textureIndex = 0;
  if (!BeginFrame(slot, textureIndex)) {
    return false;
  }

  RenderTexture* backBuffer = g_swapChain->getTexture(textureIndex);
  RenderCommandList* cmd = slot->commandList.get();
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

  return EndFrame(*slot, textureIndex);
}

bool Video::PresentGuestFrame(const GuestFrame& frame) {
  if (!g_initialized || frame.data == nullptr || frame.width == 0 || frame.height == 0) {
    return false;
  }
  if (!IsSupportedFrontBufferFormat(frame.format)) {
    static bool warned = false;
    if (!warned) {
      warned = true;
      REXLOG_WARN("PGR4 Plume: front buffer format {} not handled yet; presenting clear",
                  frame.format);
    }
    return false;
  }

  std::lock_guard<std::mutex> lock(g_presentMutex);

  FrameSlot* slot = nullptr;
  uint32_t textureIndex = 0;
  if (!BeginFrame(slot, textureIndex)) {
    return false;
  }
  if (!EnsureGuestFrameResources(*slot, frame.width, frame.height)) {
    return false;
  }

  // CPU conversion: guest (tiled / endian / format) -> linear BGRA8.
  const uint32_t pitch = frame.pitch_pixels ? frame.pitch_pixels : frame.width;
  g_convert.resize(size_t(frame.width) * frame.height);
  for (uint32_t y = 0; y < frame.height; ++y) {
    for (uint32_t x = 0; x < frame.width; ++x) {
      const uint32_t offset = frame.tiled ? TiledOffset2D(x, y, pitch, 2) : (y * pitch + x) * 4;
      g_convert[size_t(y) * frame.width + x] =
          ToBgra8(ReadTexel(frame.data, offset, frame.endian), frame.format);
    }
  }

  // Bring-up instrumentation: is there anything in the buffer at all? A count
  // of non-black texels distinguishes "guest wrote nothing" from a broken
  // decode or copy, which look identical on screen.
  {
    static uint64_t frames = 0;
    static uint64_t lastReport = 0;
    ++frames;
    const uint64_t now = std::chrono::steady_clock::now().time_since_epoch().count();
    if (lastReport == 0) {
      lastReport = now;
    } else if (now - lastReport >= 1000000000ull) {
      size_t nonBlack = 0;
      uint32_t sample = 0;
      for (size_t i = 0; i < g_convert.size(); i += 97) {
        if ((g_convert[i] & 0x00FFFFFFu) != 0) {
          ++nonBlack;
          sample = g_convert[i];
        }
      }
      REXLOG_INFO("PGR4 Plume: guest frame {}x{}: ~{}% non-black texels (sample {:08X}), {} frames/sec",
                  frame.width, frame.height,
                  g_convert.empty() ? 0 : (nonBlack * 97 * 100) / g_convert.size(), sample, frames);
      frames = 0;
      lastReport = now;
    }
  }

  void* mapped = slot->upload->map();
  if (mapped == nullptr) {
    REXLOG_ERROR("PGR4 Plume: upload buffer map failed");
    return false;
  }
  std::memcpy(mapped, g_convert.data(), g_convert.size() * sizeof(uint32_t));
  slot->upload->unmap();

  RenderTexture* backBuffer = g_swapChain->getTexture(textureIndex);
  RenderCommandList* cmd = slot->commandList.get();
  cmd->begin();
  {
    // upload -> guest frame texture
    const RenderTextureBarrier toCopyDst(g_guestFrame.get(), RenderTextureLayout::COPY_DEST);
    cmd->barriers(RenderBarrierStage::COPY, nullptr, 0, &toCopyDst, 1);
    cmd->copyTextureRegion(RenderTextureCopyLocation::Subresource(g_guestFrame.get()),
                           RenderTextureCopyLocation::PlacedFootprint(
                               slot->upload.get(), kSwapChainFormat, frame.width, frame.height, 1,
                               frame.width));

    // guest frame texture -> back buffer. Same format; same size for now
    // (1280x720 guest onto a 1280x720 swapchain). Scaling arrives with the
    // blit pipeline in a later slice.
    const RenderTextureBarrier pre[] = {
        RenderTextureBarrier(g_guestFrame.get(), RenderTextureLayout::COPY_SOURCE),
        RenderTextureBarrier(backBuffer, RenderTextureLayout::COPY_DEST)};
    cmd->barriers(RenderBarrierStage::COPY, nullptr, 0, pre, 2);
    if (frame.width == g_swapChain->getWidth() && frame.height == g_swapChain->getHeight()) {
      cmd->copyTexture(backBuffer, g_guestFrame.get());
    } else {
      static bool warned = false;
      if (!warned) {
        warned = true;
        REXLOG_WARN("PGR4 Plume: guest {}x{} vs swapchain {}x{}; copying the overlapping region",
                    frame.width, frame.height, g_swapChain->getWidth(), g_swapChain->getHeight());
      }
      cmd->copyTextureRegion(RenderTextureCopyLocation::Subresource(backBuffer),
                             RenderTextureCopyLocation::Subresource(g_guestFrame.get()));
    }
    const RenderTextureBarrier toPresent(backBuffer, RenderTextureLayout::PRESENT);
    cmd->barriers(RenderBarrierStage::COPY, nullptr, 0, &toPresent, 1);
  }
  cmd->end();

  return EndFrame(*slot, textureIndex);
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
  g_guestFrame.reset();
  g_guestFrameWidth = g_guestFrameHeight = 0;
  for (FrameSlot& slot : g_frames) {
    slot.commandList.reset();
    slot.fence.reset();
    slot.acquireSemaphore.reset();
    slot.renderFinishedSemaphore.reset();
    slot.upload.reset();
    slot.uploadSize = 0;
    slot.submitted = false;
  }
  g_swapChain.reset();
  g_queue.reset();
  g_device.reset();
  g_interface.reset();
  g_frameIndex = 0;
  g_initialized = false;
}
