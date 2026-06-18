#include "native_renderer/fm2_native_renderer.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <plume_render_interface.h>

#include "shaders/fm2DebugReplayFrag.hlsl.dxil.h"
#include "shaders/fm2DebugReplayVert.hlsl.dxil.h"

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

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay, false, "FM2",
    "Submit ready FM2 direct-draw replay plans through a Plume diagnostic "
    "pipeline.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_shadow_swapchain, false, "FM2",
    "Experimental: create a same-window Plume swapchain in shadow mode for "
    "debug replay while ReX graphics is still enabled.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_window, false, "FM2",
    "Create a separate diagnostic Plume window for shadow-mode debug replay.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_debug_replay_limit, 1, "FM2",
    "Maximum number of FM2 Plume diagnostic direct-draw replays to submit");

REXCVAR_DEFINE_STRING(
    fm2_plume_debug_replay_transform_mode, "local", "FM2",
    "FM2 Plume debug replay transform mode: local, row_major_clip, "
    "column_major_clip, row_major_clip_z_mid")
    .allowed({"local", "row_major_clip", "column_major_clip",
              "row_major_clip_z_mid"});

REXCVAR_DEFINE_STRING(
    fm2_plume_debug_replay_pipeline_topology, "auto", "FM2",
    "FM2 Plume debug replay pipeline topology override: auto, "
    "triangle_list, or triangle_strip")
    .allowed({"auto", "triangle_list", "triangle_strip"});

std::atomic<bool> g_initialized{false};
std::atomic<bool> g_plume_available{false};
std::atomic<bool> g_plume_device_ready{false};
std::atomic<bool> g_swapchain_ready{false};
std::atomic<uint64_t> g_build_object_pass_entries{0};
std::atomic<uint64_t> g_direct_indexed_draw_entries{0};
std::atomic<uint64_t> g_debug_replay_attempts{0};
std::atomic<uint64_t> g_debug_replay_submitted{0};
std::atomic<uint64_t> g_debug_replay_failed{0};
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
  std::unique_ptr<plume::RenderPipelineLayout> debug_replay_pipeline_layout;
  std::unique_ptr<plume::RenderShader> debug_replay_vertex_shader;
  std::unique_ptr<plume::RenderShader> debug_replay_pixel_shader;
  std::unique_ptr<plume::RenderPipeline> debug_replay_pipeline;
  fm2::native_renderer::DirectDrawReplayTopology debug_replay_pipeline_topology =
      fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  HWND debug_replay_window = nullptr;
};

std::mutex g_plume_mutex;
PlumeState g_plume;

constexpr uint32_t kSwapchainBufferCount = 2;
constexpr plume::RenderFormat kSwapchainFormat =
    plume::RenderFormat::B8G8R8A8_UNORM;
constexpr wchar_t kDebugReplayWindowClassName[] =
    L"ReXGlueFM2PlumeDebugReplayWindow";
constexpr wchar_t kDebugReplayWindowTitle[] = L"FM2 Plume Debug Replay";
constexpr uint32_t kDebugReplayWindowClientWidth = 960;
constexpr uint32_t kDebugReplayWindowClientHeight = 540;

enum : uint32_t {
  kDebugReplayTransformModeLocal = 0u,
  kDebugReplayTransformModeRowMajorClip = 1u,
  kDebugReplayTransformModeColumnMajorClip = 2u,
  kDebugReplayTransformModeRowMajorClipZMid = 3u,
};

struct DebugReplayPushConstants {
  float transform_rows[4][4] = {
      {1.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
  };
  uint32_t transform_mode = kDebugReplayTransformModeLocal;
  uint32_t transform_valid = 0;
  uint32_t reserved[2] = {};
};

static_assert((sizeof(DebugReplayPushConstants) % sizeof(uint32_t)) == 0);

bool CreateFramebuffersLocked();
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

constexpr bool ShouldLogPacketSample(uint64_t count, uint32_t interval) {
  return interval != 0 && count != 0 && ((count - 1) % interval) == 0;
}

static_assert(!ShouldLogPacketSample(0, 1));
static_assert(!ShouldLogPacketSample(1, 0));
static_assert(ShouldLogPacketSample(1, 1));
static_assert(ShouldLogPacketSample(1, 120));
static_assert(!ShouldLogPacketSample(2, 120));
static_assert(ShouldLogPacketSample(121, 120));

void MaybeLogPacket(const char* name, uint32_t hook_address, uint64_t count,
                    const fm2::native_renderer::GuestArgs& args) {
  if (!REXCVAR_GET(fm2_plume_trace_packets)) {
    return;
  }
  const uint32_t interval = REXCVAR_GET(fm2_plume_trace_log_interval);
  if (!ShouldLogPacketSample(count, interval)) {
    return;
  }
  REXLOG_INFO(
      "{} count={} hook={:08X} r3={:08X} r4={:08X} r5={:08X} r6={:08X} "
      "r7={:08X} r8={:08X} r9={:08X} r10={:08X}",
      name, count, hook_address, args.r3, args.r4, args.r5, args.r6, args.r7,
      args.r8, args.r9, args.r10);
}

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
LRESULT CALLBACK DebugReplayWindowProc(HWND hwnd, UINT message, WPARAM wparam,
                                       LPARAM lparam) {
  switch (message) {
    case WM_CLOSE:
      ShowWindow(hwnd, SW_HIDE);
      return 0;
    default:
      break;
  }
  return DefWindowProcW(hwnd, message, wparam, lparam);
}

ATOM EnsureDebugReplayWindowClass() {
  static ATOM registered_class = 0;
  if (registered_class != 0) {
    return registered_class;
  }

  WNDCLASSEXW window_class = {};
  window_class.cbSize = sizeof(window_class);
  window_class.style = CS_HREDRAW | CS_VREDRAW;
  window_class.lpfnWndProc = DebugReplayWindowProc;
  window_class.hInstance = GetModuleHandleW(nullptr);
  window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  window_class.lpszClassName = kDebugReplayWindowClassName;

  registered_class = RegisterClassExW(&window_class);
  if (registered_class == 0 && GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
    registered_class = 1;
  }
  return registered_class;
}

void DestroyDebugReplayWindowLocked() {
  if (g_plume.debug_replay_window &&
      IsWindow(g_plume.debug_replay_window) != FALSE) {
    if (DestroyWindow(g_plume.debug_replay_window) == FALSE) {
      REXLOG_WARN("FM2 Plume failed to destroy debug replay window error={}",
                  GetLastError());
    }
  }
  g_plume.debug_replay_window = nullptr;
}

bool CreateDebugReplayWindowLocked() {
  if (g_plume.debug_replay_window &&
      IsWindow(g_plume.debug_replay_window) != FALSE) {
    ShowWindow(g_plume.debug_replay_window, SW_SHOWNOACTIVATE);
    return true;
  }

  if (EnsureDebugReplayWindowClass() == 0) {
    REXLOG_WARN("FM2 Plume failed to register debug replay window class error={}",
                GetLastError());
    return false;
  }

  constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW;
  constexpr DWORD kWindowExStyle = WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
  RECT rect = {0, 0, static_cast<LONG>(kDebugReplayWindowClientWidth),
               static_cast<LONG>(kDebugReplayWindowClientHeight)};
  AdjustWindowRectEx(&rect, kWindowStyle, FALSE, kWindowExStyle);

  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  g_plume.debug_replay_window = CreateWindowExW(
      kWindowExStyle, kDebugReplayWindowClassName, kDebugReplayWindowTitle,
      kWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr,
      nullptr, GetModuleHandleW(nullptr), nullptr);
  if (!g_plume.debug_replay_window) {
    REXLOG_WARN("FM2 Plume failed to create debug replay window error={}",
                GetLastError());
    return false;
  }

  ShowWindow(g_plume.debug_replay_window, SW_SHOWNOACTIVATE);
  UpdateWindow(g_plume.debug_replay_window);
  REXLOG_INFO("FM2 Plume debug replay window created hwnd={:p}",
              static_cast<void*>(g_plume.debug_replay_window));
  return true;
}

void ResetPlumeStateLocked() {
  DestroyDebugReplayWindowLocked();
  g_plume.debug_replay_pipeline.reset();
  g_plume.debug_replay_pipeline_topology =
      fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  g_plume.debug_replay_pixel_shader.reset();
  g_plume.debug_replay_vertex_shader.reset();
  g_plume.debug_replay_pipeline_layout.reset();
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

plume::RenderFormat ReplayIndexFormatToPlume(
    fm2::native_renderer::DirectDrawReplayIndexFormat format) {
  switch (format) {
    case fm2::native_renderer::DirectDrawReplayIndexFormat::kUint16:
      return plume::RenderFormat::R16_UINT;
    case fm2::native_renderer::DirectDrawReplayIndexFormat::kUint32:
      return plume::RenderFormat::R32_UINT;
    case fm2::native_renderer::DirectDrawReplayIndexFormat::kUnknown:
      break;
  }
  return plume::RenderFormat::UNKNOWN;
}

fm2::native_renderer::DirectDrawReplayTopology EffectiveDebugReplayTopology(
    fm2::native_renderer::DirectDrawReplayTopology plan_topology) {
  const std::string topology =
      REXCVAR_GET(fm2_plume_debug_replay_pipeline_topology);
  switch (fm2::native_renderer::ParseDirectDebugReplayPipelineTopology(
      topology)) {
    case fm2::native_renderer::DirectDebugReplayPipelineTopology::kTriangleStrip:
      return fm2::native_renderer::DirectDrawReplayTopology::kTriangleStrip;
    case fm2::native_renderer::DirectDebugReplayPipelineTopology::kTriangleList:
      return fm2::native_renderer::DirectDrawReplayTopology::kTriangleList;
    case fm2::native_renderer::DirectDebugReplayPipelineTopology::kAuto:
      break;
  }
  return plan_topology;
}

plume::RenderPrimitiveTopology ReplayTopologyToPlume(
    fm2::native_renderer::DirectDrawReplayTopology topology) {
  switch (topology) {
    case fm2::native_renderer::DirectDrawReplayTopology::kTriangleStrip:
      return plume::RenderPrimitiveTopology::TRIANGLE_STRIP;
    case fm2::native_renderer::DirectDrawReplayTopology::kTriangleList:
      return plume::RenderPrimitiveTopology::TRIANGLE_LIST;
    case fm2::native_renderer::DirectDrawReplayTopology::kUnknown:
      break;
  }
  return plume::RenderPrimitiveTopology::TRIANGLE_LIST;
}

uint32_t DebugReplayTransformModeValue() {
  const std::string mode = REXCVAR_GET(fm2_plume_debug_replay_transform_mode);
  if (mode == "row_major_clip") {
    return kDebugReplayTransformModeRowMajorClip;
  }
  if (mode == "column_major_clip") {
    return kDebugReplayTransformModeColumnMajorClip;
  }
  if (mode == "row_major_clip_z_mid") {
    return kDebugReplayTransformModeRowMajorClipZMid;
  }
  return kDebugReplayTransformModeLocal;
}

const char* DebugReplayTransformModeName(uint32_t mode) {
  switch (mode) {
    case kDebugReplayTransformModeRowMajorClip:
      return "row_major_clip";
    case kDebugReplayTransformModeColumnMajorClip:
      return "column_major_clip";
    case kDebugReplayTransformModeRowMajorClipZMid:
      return "row_major_clip_z_mid";
    case kDebugReplayTransformModeLocal:
    default:
      return "local";
  }
}

DebugReplayPushConstants BuildDebugReplayPushConstants(
    const fm2::native_renderer::DirectDrawDebugReplayPlan& plan) {
  DebugReplayPushConstants constants;
  const uint32_t requested_mode = DebugReplayTransformModeValue();
  constants.transform_valid = plan.transform.valid ? 1u : 0u;
  constants.transform_mode =
      constants.transform_valid ? requested_mode : kDebugReplayTransformModeLocal;
  if (!plan.transform.valid) {
    return constants;
  }

  for (uint32_t row = 0; row < 4u; ++row) {
    for (uint32_t component = 0; component < 4u; ++component) {
      constants.transform_rows[row][component] =
          plan.transform.rows[row][component];
    }
  }
  return constants;
}

bool EnsureDebugReplayPipelineLocked(
    fm2::native_renderer::DirectDrawReplayTopology topology) {
  if (g_plume.debug_replay_pipeline &&
      g_plume.debug_replay_pipeline_topology == topology) {
    return true;
  }
  if (g_plume.debug_replay_pipeline) {
    g_plume.debug_replay_pipeline.reset();
    g_plume.debug_replay_pipeline_topology =
        fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  }
  if (!g_plume.render_interface || !g_plume.device) {
    return false;
  }

  const plume::RenderShaderFormat shader_format =
      g_plume.render_interface->getCapabilities().shaderFormat;
  if (shader_format != plume::RenderShaderFormat::DXIL) {
    REXLOG_WARN("FM2 Plume debug replay requires DXIL shaders on Windows");
    return false;
  }

  plume::RenderPipelineLayoutDesc layout_desc;
  layout_desc.allowInputLayout = true;
  plume::RenderPushConstantRange push_constant_range;
  push_constant_range.size = sizeof(DebugReplayPushConstants);
  push_constant_range.stageFlags = plume::RenderShaderStageFlag::VERTEX;
  layout_desc.pushConstantRanges = &push_constant_range;
  layout_desc.pushConstantRangesCount = 1;
  g_plume.debug_replay_pipeline_layout =
      g_plume.device->createPipelineLayout(layout_desc);
  if (!g_plume.debug_replay_pipeline_layout) {
    REXLOG_WARN("FM2 Plume debug replay failed to create pipeline layout");
    return false;
  }

  g_plume.debug_replay_vertex_shader = g_plume.device->createShader(
      fm2DebugReplayVertBlobDXIL, fm2DebugReplayVertBlobDXIL_size, "VSMain",
      shader_format);
  g_plume.debug_replay_pixel_shader = g_plume.device->createShader(
      fm2DebugReplayFragBlobDXIL, fm2DebugReplayFragBlobDXIL_size, "PSMain",
      shader_format);
  if (!g_plume.debug_replay_vertex_shader || !g_plume.debug_replay_pixel_shader) {
    REXLOG_WARN("FM2 Plume debug replay failed to create shaders");
    return false;
  }

  std::array<plume::RenderInputSlot, 2> input_slots = {
      plume::RenderInputSlot(0, 0x20u),
      plume::RenderInputSlot(1, 0x0Cu),
  };
  std::array<plume::RenderInputElement, 3> input_elements = {
      plume::RenderInputElement("RAW", 0, 0, plume::RenderFormat::R32G32B32A32_UINT,
                                0, 0),
      plume::RenderInputElement("RAW", 1, 1, plume::RenderFormat::R32G32B32A32_UINT,
                                0, 16),
      plume::RenderInputElement("SIDE", 0, 2, plume::RenderFormat::R32G32B32_UINT,
                                1, 0),
  };

  plume::RenderGraphicsPipelineDesc pipeline_desc;
  pipeline_desc.inputSlots = input_slots.data();
  pipeline_desc.inputSlotsCount = static_cast<uint32_t>(input_slots.size());
  pipeline_desc.inputElements = input_elements.data();
  pipeline_desc.inputElementsCount =
      static_cast<uint32_t>(input_elements.size());
  pipeline_desc.pipelineLayout = g_plume.debug_replay_pipeline_layout.get();
  pipeline_desc.vertexShader = g_plume.debug_replay_vertex_shader.get();
  pipeline_desc.pixelShader = g_plume.debug_replay_pixel_shader.get();
  pipeline_desc.renderTargetFormat[0] = kSwapchainFormat;
  pipeline_desc.renderTargetBlend[0] = plume::RenderBlendDesc::Copy();
  pipeline_desc.renderTargetCount = 1;
  pipeline_desc.primitiveTopology = ReplayTopologyToPlume(topology);
  pipeline_desc.cullMode = plume::RenderCullMode::NONE;

  g_plume.debug_replay_pipeline =
      g_plume.device->createGraphicsPipeline(pipeline_desc);
  if (!g_plume.debug_replay_pipeline) {
    REXLOG_WARN("FM2 Plume debug replay failed to create graphics pipeline");
    return false;
  }

  g_plume.debug_replay_pipeline_topology = topology;
  REXLOG_INFO("FM2 Plume debug replay pipeline initialized topology={}",
              static_cast<unsigned>(topology));
  return true;
}

std::unique_ptr<plume::RenderBuffer> CreateConvertedReplayBufferLocked(
    const char* name, const uint8_t* src, uint32_t byte_count,
    fm2::native_renderer::DirectDrawReplayUploadEndian endian,
    plume::RenderBufferFlags flags) {
  if (!g_plume.device || !src || byte_count == 0) {
    return nullptr;
  }

  auto buffer = g_plume.device->createBuffer(
      plume::RenderBufferDesc::UploadBuffer(byte_count, flags));
  if (!buffer) {
    REXLOG_WARN("FM2 Plume debug replay failed to create {} buffer", name);
    return nullptr;
  }

  void* mapped = buffer->map();
  if (!mapped) {
    REXLOG_WARN("FM2 Plume debug replay failed to map {} buffer", name);
    return nullptr;
  }

  const bool converted = fm2::native_renderer::ConvertDirectDrawReplayUploadBytes(
      src, byte_count, endian, static_cast<uint8_t*>(mapped));
  buffer->unmap();
  if (!converted) {
    REXLOG_WARN("FM2 Plume debug replay failed to convert {} buffer bytes", name);
    return nullptr;
  }

  buffer->setName(name);
  return buffer;
}

bool ResizeSwapchainIfNeededLocked() {
  if (!g_plume.swapchain) {
    return false;
  }
  if (!g_plume.swapchain->needsResize()) {
    return true;
  }
  g_plume.framebuffers.clear();
  if (!g_plume.swapchain->resize() || !CreateFramebuffersLocked()) {
    REXLOG_WARN("FM2 Plume failed to resize swapchain before debug replay");
    return false;
  }
  return true;
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

bool CreateSwapchainForNativeWindowLocked(plume::RenderWindow render_window) {
  if (!render_window || !g_plume.command_queue) {
    REXLOG_WARN(
        "FM2 Plume swapchain creation skipped because native window or queue is "
        "missing");
    return false;
  }

  g_plume.swapchain = g_plume.command_queue->createSwapChain(
      plume::RenderSwapChainDesc(render_window, kSwapchainFormat,
                                 kSwapchainBufferCount));
  if (!g_plume.swapchain || g_plume.swapchain->isEmpty()) {
    g_plume.swapchain.reset();
    REXLOG_WARN("FM2 Plume failed to create a valid swapchain");
    return false;
  }

  if (!g_plume.swapchain->resize()) {
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

bool CreateSwapchainLocked(rex::ui::Window* window) {
  if (!window) {
    REXLOG_WARN("FM2 Plume swapchain creation skipped because window is missing");
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
  return CreateSwapchainForNativeWindowLocked(render_window);
}

bool CreateDebugReplayWindowSwapchainLocked() {
  if (!CreateDebugReplayWindowLocked()) {
    return false;
  }
  return CreateSwapchainForNativeWindowLocked(
      reinterpret_cast<plume::RenderWindow>(g_plume.debug_replay_window));
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

bool RenderDirectDebugReplayLocked(
    const fm2::native_renderer::DirectDrawDebugReplayPlan& plan,
    const fm2::native_renderer::DirectDrawReplaySourceBytes& sources) {
  if (!plan.ready || plan.stream_count != 2u || !sources.stream0 ||
      !sources.stream1 || !sources.index) {
    return false;
  }
  const fm2::native_renderer::DirectDrawReplayTopology effective_topology =
      EffectiveDebugReplayTopology(plan.topology);
  if (effective_topology ==
          fm2::native_renderer::DirectDrawReplayTopology::kUnknown ||
      plan.streams[0].stride != 0x20u || plan.streams[1].stride != 0x0Cu) {
    REXLOG_WARN(
        "FM2 Plume debug replay skipped unsupported replay contract "
        "topology={} s0_stride={} s1_stride={}",
        static_cast<unsigned>(effective_topology), plan.streams[0].stride,
        plan.streams[1].stride);
    return false;
  }
  if (!g_plume.device || !g_plume.command_queue || !g_plume.command_list ||
      !g_plume.swapchain || !g_plume.acquire_semaphore || !g_plume.fence ||
      !ResizeSwapchainIfNeededLocked() ||
      !EnsureDebugReplayPipelineLocked(effective_topology)) {
    return false;
  }

  const plume::RenderFormat index_format =
      ReplayIndexFormatToPlume(plan.index_format);
  if (index_format == plume::RenderFormat::UNKNOWN) {
    return false;
  }

  auto stream0_buffer = CreateConvertedReplayBufferLocked(
      "FM2 debug replay stream0", sources.stream0, plan.streams[0].upload_bytes,
      plan.streams[0].upload_endian, plume::RenderBufferFlag::VERTEX);
  auto stream1_buffer = CreateConvertedReplayBufferLocked(
      "FM2 debug replay stream1", sources.stream1, plan.streams[1].upload_bytes,
      plan.streams[1].upload_endian, plume::RenderBufferFlag::VERTEX);
  auto index_buffer = CreateConvertedReplayBufferLocked(
      "FM2 debug replay index", sources.index, plan.index.upload_bytes,
      plan.index.upload_endian, plume::RenderBufferFlag::INDEX);
  if (!stream0_buffer || !stream1_buffer || !index_buffer) {
    return false;
  }

  uint32_t image_index = 0;
  if (!g_plume.swapchain->acquireTexture(g_plume.acquire_semaphore.get(),
                                         &image_index)) {
    REXLOG_WARN("FM2 Plume debug replay failed to acquire swapchain texture");
    return false;
  }
  if (image_index >= g_plume.framebuffers.size()) {
    REXLOG_WARN("FM2 Plume debug replay acquired invalid image index {}",
                image_index);
    return false;
  }

  plume::RenderTexture* texture = g_plume.swapchain->getTexture(image_index);
  const uint32_t width = g_plume.swapchain->getWidth();
  const uint32_t height = g_plume.swapchain->getHeight();

  std::array<plume::RenderVertexBufferView, 2> vertex_views = {
      plume::RenderVertexBufferView(stream0_buffer.get(),
                                    plan.streams[0].upload_bytes),
      plume::RenderVertexBufferView(stream1_buffer.get(),
                                    plan.streams[1].upload_bytes),
  };
  std::array<plume::RenderInputSlot, 2> input_slots = {
      plume::RenderInputSlot(plan.streams[0].slot, plan.streams[0].stride),
      plume::RenderInputSlot(plan.streams[1].slot, plan.streams[1].stride),
  };
  plume::RenderIndexBufferView index_view(index_buffer.get(),
                                          plan.index.upload_bytes,
                                          index_format);
  const DebugReplayPushConstants push_constants =
      BuildDebugReplayPushConstants(plan);

  g_plume.command_list->begin();
  g_plume.command_list->barriers(
      plume::RenderBarrierStage::GRAPHICS,
      plume::RenderTextureBarrier(texture,
                                  plume::RenderTextureLayout::COLOR_WRITE));
  g_plume.command_list->setFramebuffer(g_plume.framebuffers[image_index].get());
  g_plume.command_list->setViewports(
      plume::RenderViewport(0.0f, 0.0f, float(width), float(height)));
  g_plume.command_list->setScissors(plume::RenderRect(0, 0, width, height));
  g_plume.command_list->clearColor(
      0, plume::RenderColor(0.0f, 0.02f, 0.04f, 1.0f));
  g_plume.command_list->setGraphicsPipelineLayout(
      g_plume.debug_replay_pipeline_layout.get());
  g_plume.command_list->setPipeline(g_plume.debug_replay_pipeline.get());
  g_plume.command_list->setGraphicsPushConstants(0, &push_constants);
  g_plume.command_list->setVertexBuffers(
      0, vertex_views.data(), static_cast<uint32_t>(vertex_views.size()),
      input_slots.data());
  g_plume.command_list->setIndexBuffer(&index_view);
  g_plume.command_list->drawIndexedInstanced(
      plan.draw.index_count, plan.draw.instance_count, plan.draw.start_index,
      plan.draw.base_vertex, plan.draw.start_instance);
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

  REXLOG_INFO(
      "FM2_PLUME_DEBUG_REPLAY_SUBMIT presented={} image={} size={}x{} "
      "index_count={} s0_base={:08X} s0_upload_base={:08X} "
      "s1_base={:08X} s1_upload_base={:08X} "
      "index_base={:08X} index_upload_base={:08X} "
      "s0_bytes={} s1_bytes={} index_bytes={} "
      "s0_hash={:016X} s1_hash={:016X} index_hash={:016X} "
      "topology={} plan_topology={} transform_valid={} transform_first={} "
      "transform_mode={}",
      presented, image_index, width, height, plan.draw.index_count,
      plan.streams[0].guest_base, plan.streams[0].upload_guest_base,
      plan.streams[1].guest_base, plan.streams[1].upload_guest_base,
      plan.index.guest_base, plan.index.upload_guest_base,
      plan.streams[0].upload_bytes, plan.streams[1].upload_bytes,
      plan.index.upload_bytes, plan.streams[0].hash, plan.streams[1].hash,
      plan.index.hash, static_cast<unsigned>(effective_topology),
      static_cast<unsigned>(plan.topology), plan.transform.valid ? 1u : 0u,
      plan.transform.first_constant,
      DebugReplayTransformModeName(push_constants.transform_mode));
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

bool WantsDirectDebugReplay() {
  return REXCVAR_GET(fm2_plume_debug_replay);
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

  const bool wants_debug_replay_swapchain =
      mode == Mode::kShadow && REXCVAR_GET(fm2_plume_debug_replay) &&
      REXCVAR_GET(fm2_plume_debug_replay_shadow_swapchain);
  const bool wants_debug_replay_window =
      mode == Mode::kShadow && REXCVAR_GET(fm2_plume_debug_replay) &&
      REXCVAR_GET(fm2_plume_debug_replay_window);
  if (mode == Mode::kPlumeClear) {
    if (!CreateSwapchainLocked(window)) {
      REXLOG_WARN("FM2 Plume clear mode could not create swapchain");
      return false;
    }
  } else if (wants_debug_replay_window) {
    if (!CreateDebugReplayWindowSwapchainLocked()) {
      REXLOG_WARN(
          "FM2 Plume shadow debug replay could not create diagnostic window "
          "swapchain; continuing without debug replay");
    }
  } else if (wants_debug_replay_swapchain) {
    if (!CreateSwapchainLocked(window)) {
      REXLOG_WARN(
          "FM2 Plume shadow debug replay could not create swapchain; "
          "continuing without debug replay");
    }
  }

  if (mode == Mode::kPlumeClear) {
    if (!g_plume.swapchain) {
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
    REXLOG_INFO(
        "FM2 Plume native renderer shut down build_object_pass={} "
        "direct_indexed_draw={} debug_replay_attempts={} "
        "debug_replay_submitted={} debug_replay_failed={} last_hook={:08X}",
        g_build_object_pass_entries.load(std::memory_order_relaxed),
        g_direct_indexed_draw_entries.load(std::memory_order_relaxed),
        g_debug_replay_attempts.load(std::memory_order_relaxed),
        g_debug_replay_submitted.load(std::memory_order_relaxed),
        g_debug_replay_failed.load(std::memory_order_relaxed),
        g_last_hook_address.load(std::memory_order_relaxed));
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

bool SubmitDirectDebugReplay(const DirectDrawDebugReplayPlan& plan,
                             const DirectDrawReplaySourceBytes& sources) {
  if (!REXCVAR_GET(fm2_plume_debug_replay)) {
    return false;
  }

  const uint64_t attempt =
      g_debug_replay_attempts.fetch_add(1, std::memory_order_relaxed) + 1;
  const uint32_t limit = REXCVAR_GET(fm2_plume_debug_replay_limit);
  if (limit != 0 && attempt > limit) {
    return false;
  }

  const Mode mode = GetMode();
  if (mode != Mode::kPlumeClear && mode != Mode::kShadow) {
    g_debug_replay_failed.fetch_add(1, std::memory_order_relaxed);
    REXLOG_WARN(
        "FM2 Plume debug replay requires fm2_plume_mode=shadow or "
        "plume_clear; current mode={}",
        GetModeName(mode));
    return false;
  }
  if (!plan.ready) {
    g_debug_replay_failed.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  bool submitted = false;
  {
    std::scoped_lock lock(g_plume_mutex);
    submitted = RenderDirectDebugReplayLocked(plan, sources);
  }
  if (submitted) {
    g_debug_replay_submitted.fetch_add(1, std::memory_order_relaxed);
  } else {
    g_debug_replay_failed.fetch_add(1, std::memory_order_relaxed);
  }
  return submitted;
#else
  (void)sources;
  g_debug_replay_failed.fetch_add(1, std::memory_order_relaxed);
  REXLOG_WARN("FM2 Plume debug replay requested without Plume support");
  return false;
#endif
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
  out.debug_replay_attempts =
      g_debug_replay_attempts.load(std::memory_order_relaxed);
  out.debug_replay_submitted =
      g_debug_replay_submitted.load(std::memory_order_relaxed);
  out.debug_replay_failed =
      g_debug_replay_failed.load(std::memory_order_relaxed);
  out.last_hook_address = g_last_hook_address.load(std::memory_order_relaxed);
  {
    std::scoped_lock lock(g_last_args_mutex);
    out.last_args = g_last_args;
  }
  return out;
}

}  // namespace fm2::native_renderer
