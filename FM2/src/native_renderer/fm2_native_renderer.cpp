#include "native_renderer/fm2_native_renderer.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/ui/window.h>

#ifndef FM2_HAS_PLUME
#define FM2_HAS_PLUME 0
#endif

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
#include <plume_render_interface.h>

#if defined(_MSC_VER)
#pragma comment(lib, "d3d12.lib")
#endif

namespace plume {
std::unique_ptr<RenderInterface> CreateD3D12Interface();
}
#endif

namespace {

REXCVAR_DEFINE_STRING(
    fm2_plume_mode, "xenos", "FM2",
    "FM2 Plume renderer mode: xenos, shadow, plume_clear")
    .allowed({"xenos", "shadow", "plume_clear"});

REXCVAR_DEFINE_BOOL(
    fm2_plume_trace_packets, false, "FM2",
    "Log FM2 Plume shadow packet capture samples");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_log_interval, 120, "FM2",
    "Log one FM2 Plume packet sample every N captures when tracing is enabled");

REXCVAR_DEFINE_BOOL(
    fm2_plume_clear_on_init, true, "FM2",
    "In fm2_plume_mode=plume_clear, issue one Plume clear/present during FM2 "
    "setup");

std::atomic<bool> g_initialized{false};
std::atomic<bool> g_plume_available{false};
std::atomic<bool> g_plume_device_ready{false};
std::atomic<bool> g_swapchain_ready{false};
std::atomic<uint64_t> g_build_object_pass_entries{0};
std::atomic<uint64_t> g_direct_indexed_draw_entries{0};
std::atomic<uint32_t> g_last_hook_address{0};

std::mutex g_last_args_mutex;
fm2::native_renderer::GuestArgs g_last_args;

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
struct PlumeState {
  std::unique_ptr<plume::RenderInterface> render_interface;
  std::unique_ptr<plume::RenderDevice> device;
  std::unique_ptr<plume::RenderCommandQueue> command_queue;
  std::unique_ptr<plume::RenderCommandList> command_list;
  std::unique_ptr<plume::RenderCommandFence> fence;
  std::unique_ptr<plume::RenderSwapChain> swapchain;
  std::unique_ptr<plume::RenderCommandSemaphore> acquire_semaphore;
  std::vector<std::unique_ptr<plume::RenderCommandSemaphore>>
      release_semaphores;
  std::vector<std::unique_ptr<plume::RenderFramebuffer>> framebuffers;
};

std::mutex g_plume_mutex;
PlumeState g_plume;

constexpr uint32_t kSwapchainBufferCount = 2;
constexpr plume::RenderFormat kSwapchainFormat =
    plume::RenderFormat::B8G8R8A8_UNORM;
#endif

fm2::native_renderer::Mode ParseMode(const std::string& value) {
  if (value == "shadow") {
    return fm2::native_renderer::Mode::kShadow;
  }
  if (value == "plume_clear") {
    return fm2::native_renderer::Mode::kPlumeClear;
  }
  return fm2::native_renderer::Mode::kXenos;
}

void StoreLastArgs(uint32_t hook_address,
                   const fm2::native_renderer::GuestArgs& args) {
  g_last_hook_address.store(hook_address, std::memory_order_relaxed);
  std::scoped_lock lock(g_last_args_mutex);
  g_last_args = args;
}

void MaybeLogPacket(const char* name, uint32_t hook_address, uint64_t count,
                    const fm2::native_renderer::GuestArgs& args) {
  if (!REXCVAR_GET(fm2_plume_trace_packets)) {
    return;
  }
  const uint32_t interval = REXCVAR_GET(fm2_plume_trace_log_interval);
  if (interval == 0 || (count % interval) != 1) {
    return;
  }
  REXLOG_INFO(
      "{} count={} hook={:08X} r3={:08X} r4={:08X} r5={:08X} r6={:08X} "
      "r7={:08X} r8={:08X} r9={:08X} r10={:08X}",
      name, count, hook_address, args.r3, args.r4, args.r5, args.r6, args.r7,
      args.r8, args.r9, args.r10);
}

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
void ResetPlumeStateLocked() {
  g_plume.framebuffers.clear();
  g_plume.release_semaphores.clear();
  g_plume.acquire_semaphore.reset();
  g_plume.swapchain.reset();
  g_plume.fence.reset();
  g_plume.command_list.reset();
  g_plume.command_queue.reset();
  g_plume.device.reset();
  g_plume.render_interface.reset();
  g_swapchain_ready.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_plume_available.store(false, std::memory_order_relaxed);
}

bool CreateFramebuffersLocked() {
  g_plume.framebuffers.clear();
  if (!g_plume.device || !g_plume.swapchain) {
    return false;
  }

  for (uint32_t i = 0; i < g_plume.swapchain->getTextureCount(); ++i) {
    const plume::RenderTexture* color_attachment =
        g_plume.swapchain->getTexture(i);
    plume::RenderFramebufferDesc fb_desc;
    fb_desc.colorAttachments = &color_attachment;
    fb_desc.colorAttachmentsCount = 1;
    fb_desc.depthAttachment = nullptr;
    auto framebuffer = g_plume.device->createFramebuffer(fb_desc);
    if (!framebuffer) {
      g_plume.framebuffers.clear();
      return false;
    }
    g_plume.framebuffers.push_back(std::move(framebuffer));
  }

  return !g_plume.framebuffers.empty();
}

bool CreatePlumeDeviceLocked() {
  g_plume.render_interface = plume::CreateD3D12Interface();
  if (!g_plume.render_interface) {
    REXLOG_WARN("FM2 Plume failed to create D3D12 render interface");
    return false;
  }
  g_plume_available.store(true, std::memory_order_relaxed);

  g_plume.device = g_plume.render_interface->createDevice();
  if (!g_plume.device) {
    REXLOG_WARN("FM2 Plume failed to create render device");
    return false;
  }

  g_plume.command_queue =
      g_plume.device->createCommandQueue(plume::RenderCommandListType::DIRECT);
  g_plume.command_list =
      g_plume.command_queue ? g_plume.command_queue->createCommandList()
                            : nullptr;
  g_plume.fence = g_plume.device->createCommandFence();
  g_plume.acquire_semaphore = g_plume.device->createCommandSemaphore();

  if (!g_plume.command_queue || !g_plume.command_list || !g_plume.fence ||
      !g_plume.acquire_semaphore) {
    REXLOG_WARN("FM2 Plume failed to create command resources");
    return false;
  }

  g_plume_device_ready.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume device initialized backend=D3D12");
  return true;
}

bool CreateSwapchainLocked(rex::ui::Window* window) {
  if (!window || !g_plume.command_queue) {
    REXLOG_WARN(
        "FM2 Plume swapchain creation skipped because window or queue is "
        "missing");
    return false;
  }

  void* native_window = window->GetNativeWindowHandle();
  if (!native_window) {
    REXLOG_WARN(
        "FM2 Plume swapchain creation skipped because native window handle is "
        "null");
    return false;
  }

  auto render_window = reinterpret_cast<plume::RenderWindow>(native_window);
  g_plume.swapchain = g_plume.command_queue->createSwapChain(
      plume::RenderSwapChainDesc(render_window, kSwapchainFormat,
                                 kSwapchainBufferCount));
  if (!g_plume.swapchain || !g_plume.swapchain->resize()) {
    REXLOG_WARN("FM2 Plume failed to create or resize swapchain");
    return false;
  }

  if (!CreateFramebuffersLocked()) {
    REXLOG_WARN("FM2 Plume failed to create swapchain framebuffers");
    return false;
  }

  g_swapchain_ready.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume swapchain initialized size={}x{} textures={}",
              g_plume.swapchain->getWidth(), g_plume.swapchain->getHeight(),
              g_plume.swapchain->getTextureCount());
  return true;
}

bool RenderClearOnceLocked() {
  if (!g_plume.device || !g_plume.command_queue || !g_plume.command_list ||
      !g_plume.swapchain || !g_plume.acquire_semaphore || !g_plume.fence) {
    return false;
  }

  if (g_plume.swapchain->needsResize()) {
    g_plume.framebuffers.clear();
    if (!g_plume.swapchain->resize() || !CreateFramebuffersLocked()) {
      REXLOG_WARN("FM2 Plume failed to resize swapchain before clear");
      return false;
    }
  }

  uint32_t image_index = 0;
  if (!g_plume.swapchain->acquireTexture(g_plume.acquire_semaphore.get(),
                                         &image_index)) {
    REXLOG_WARN("FM2 Plume failed to acquire swapchain texture");
    return false;
  }
  if (image_index >= g_plume.framebuffers.size()) {
    REXLOG_WARN("FM2 Plume acquired invalid swapchain image index {}",
                image_index);
    return false;
  }

  plume::RenderTexture* texture = g_plume.swapchain->getTexture(image_index);
  const uint32_t width = g_plume.swapchain->getWidth();
  const uint32_t height = g_plume.swapchain->getHeight();

  g_plume.command_list->begin();
  g_plume.command_list->barriers(
      plume::RenderBarrierStage::GRAPHICS,
      plume::RenderTextureBarrier(texture,
                                  plume::RenderTextureLayout::COLOR_WRITE));
  g_plume.command_list->setFramebuffer(g_plume.framebuffers[image_index].get());
  g_plume.command_list->setViewports(
      plume::RenderViewport(0.0f, 0.0f, float(width), float(height)));
  g_plume.command_list->setScissors(
      plume::RenderRect(0, 0, width, height));
  g_plume.command_list->clearColor(
      0, plume::RenderColor(0.02f, 0.0f, 0.08f, 1.0f));
  g_plume.command_list->barriers(
      plume::RenderBarrierStage::NONE,
      plume::RenderTextureBarrier(texture,
                                  plume::RenderTextureLayout::PRESENT));
  g_plume.command_list->end();

  while (g_plume.release_semaphores.size() <
         g_plume.swapchain->getTextureCount()) {
    g_plume.release_semaphores.emplace_back(
        g_plume.device->createCommandSemaphore());
  }

  const plume::RenderCommandList* command_list = g_plume.command_list.get();
  plume::RenderCommandSemaphore* wait_semaphore =
      g_plume.acquire_semaphore.get();
  plume::RenderCommandSemaphore* signal_semaphore =
      g_plume.release_semaphores[image_index].get();

  g_plume.command_queue->executeCommandLists(
      &command_list, 1, &wait_semaphore, 1, &signal_semaphore, 1,
      g_plume.fence.get());
  const bool presented =
      g_plume.swapchain->present(image_index, &signal_semaphore, 1);
  g_plume.command_queue->waitForCommandFence(g_plume.fence.get());

  REXLOG_INFO("FM2 Plume clear/present result={} image={} size={}x{}",
              presented, image_index, width, height);
  return presented;
}
#endif

}  // namespace

namespace fm2::native_renderer {

Mode GetMode() {
  return ParseMode(REXCVAR_GET(fm2_plume_mode));
}

const char* GetModeName(Mode mode) {
  switch (mode) {
    case Mode::kXenos:
      return "xenos";
    case Mode::kShadow:
      return "shadow";
    case Mode::kPlumeClear:
      return "plume_clear";
  }
  return "xenos";
}

bool WantsReXGraphics() {
  return GetMode() != Mode::kPlumeClear;
}

bool Initialize(rex::ui::Window* window) {
  const Mode mode = GetMode();
  if (mode == Mode::kXenos) {
    return true;
  }

  g_initialized.store(true, std::memory_order_relaxed);

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  std::scoped_lock lock(g_plume_mutex);
  ResetPlumeStateLocked();

  if (!CreatePlumeDeviceLocked()) {
    REXLOG_WARN("FM2 Plume initialization incomplete mode={}",
                GetModeName(mode));
    return mode != Mode::kPlumeClear;
  }

  if (mode == Mode::kPlumeClear) {
    if (!CreateSwapchainLocked(window)) {
      REXLOG_WARN("FM2 Plume clear mode could not create swapchain");
      return false;
    }
    if (REXCVAR_GET(fm2_plume_clear_on_init)) {
      return RenderClearOnceLocked();
    }
  }

  REXLOG_INFO("FM2 Plume native renderer initialized mode={}",
              GetModeName(mode));
  return true;
#else
  (void)window;
  g_plume_available.store(false, std::memory_order_relaxed);
  REXLOG_WARN("FM2 Plume mode {} requested, but this build has no Plume support",
              GetModeName(mode));
  return mode != Mode::kPlumeClear;
#endif
}

void Shutdown() {
  const bool was_initialized =
      g_initialized.exchange(false, std::memory_order_relaxed);
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  {
    std::scoped_lock lock(g_plume_mutex);
    ResetPlumeStateLocked();
  }
#else
  g_plume_available.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_swapchain_ready.store(false, std::memory_order_relaxed);
#endif
  if (was_initialized) {
    REXLOG_INFO("FM2 Plume native renderer shut down");
  }
}

bool RenderClearOnce() {
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  std::scoped_lock lock(g_plume_mutex);
  return RenderClearOnceLocked();
#else
  REXLOG_WARN("FM2 Plume clear requested, but this build has no Plume support");
  return false;
#endif
}

void RecordBuildObjectPassEntry(const GuestArgs& args) {
  const uint64_t count =
      g_build_object_pass_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  StoreLastArgs(0x82531370u, args);
  MaybeLogPacket("FM2_PLUME_BUILD_OBJECT_PASS", 0x82531370u, count, args);
}

void RecordDirectIndexedDrawEntry(const GuestArgs& args) {
  const uint64_t count = g_direct_indexed_draw_entries.fetch_add(
                             1, std::memory_order_relaxed) +
                         1;
  StoreLastArgs(0x825380B8u, args);
  MaybeLogPacket("FM2_PLUME_DIRECT_INDEXED_DRAW", 0x825380B8u, count, args);
}

Stats GetStatsSnapshot() {
  Stats out;
  out.initialized = g_initialized.load(std::memory_order_relaxed);
  out.plume_available = g_plume_available.load(std::memory_order_relaxed);
  out.plume_device_ready = g_plume_device_ready.load(std::memory_order_relaxed);
  out.swapchain_ready = g_swapchain_ready.load(std::memory_order_relaxed);
  out.build_object_pass_entries =
      g_build_object_pass_entries.load(std::memory_order_relaxed);
  out.direct_indexed_draw_entries =
      g_direct_indexed_draw_entries.load(std::memory_order_relaxed);
  out.last_hook_address = g_last_hook_address.load(std::memory_order_relaxed);
  {
    std::scoped_lock lock(g_last_args_mutex);
    out.last_args = g_last_args;
  }
  return out;
}

}  // namespace fm2::native_renderer
