#include "native_renderer/fm2_native_renderer.h"

#include "native_renderer/fm2_native_draw.h"
#include "native_renderer/fm2_native_state.h"

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
#include "shaders/fm2DebugReplayNative28Vert.hlsl.dxil.h"
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
    fm2_plume_native_direct_draw, false, "FM2",
    "Submit ready FM2 direct indexed draw plans through the native Plume "
    "direct-draw path.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_shadow_swapchain, false, "FM2",
    "Experimental: create a same-window Plume swapchain in shadow mode for "
    "debug replay while ReX graphics is still enabled.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_window, false, "FM2",
    "Create a separate diagnostic Plume window for shadow-mode debug replay.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_side_by_side, true, "FM2",
    "Place the separate Plume debug replay window beside the FM2 game window.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_debug_replay_window_gap, 24, "FM2",
    "Pixel gap between the FM2 game window and the separate Plume replay window.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_wireframe, false, "FM2",
    "Render the FM2 Plume replay/native direct-draw comparison window in "
    "wireframe mode.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_compare_window, false, "FM2",
    "Create a separate Plume comparison window in shadow mode and submit "
    "replayable native draws to it.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_debug_replay_limit, 1, "FM2",
    "Maximum number of FM2 Plume diagnostic direct-draw replays to submit");

REXCVAR_DEFINE_BOOL(
    fm2_plume_debug_replay_live_batch, true, "FM2",
    "Accumulate FM2 debug replay draws per frame and present as one batch at "
    "present time; requires fm2_plume_debug_replay_limit=0.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_native_direct_draw_limit, 1, "FM2",
    "Maximum number of FM2 native Plume direct draws to submit; 0 disables the "
    "limit.");

REXCVAR_DEFINE_BOOL(
    fm2_plume_native_direct_draw_live_batch, true, "FM2",
    "When unlimited native direct draw submission is enabled, queue several "
    "ready FM2 direct draw records and present them as one Plume frame.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_native_direct_draw_live_batch_size, 16, "FM2",
    "Number of queued FM2 native direct draw submissions to present together; "
    "0 presents each accepted submission immediately.");

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
std::atomic<uint64_t> g_instance_hybrid_draw_entries{0};
std::atomic<uint64_t> g_present_entries{0};
std::atomic<uint64_t> g_debug_replay_attempts{0};
std::atomic<uint64_t> g_debug_replay_submitted{0};
std::atomic<uint64_t> g_debug_replay_failed{0};
std::atomic<uint64_t> g_native_direct_draw_attempts{0};
std::atomic<uint64_t> g_native_direct_draw_submitted{0};
std::atomic<uint64_t> g_native_direct_draw_failed{0};
std::atomic<uint32_t> g_last_hook_address{0};

std::mutex g_last_args_mutex;
fm2::native_renderer::GuestArgs g_last_args;

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
struct OwnedDirectReplaySubmission {
  fm2::native_renderer::DirectDrawDebugReplayPlan plan;
  fm2::native_renderer::NativeDrawPacket native_packet;
  std::vector<uint8_t> stream0;
  std::vector<uint8_t> stream1;
  std::vector<uint8_t> index;
};

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
  fm2::native_renderer::DirectDrawReplayPipelineLayout
      debug_replay_pipeline_layout_kind =
          fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported;
  fm2::native_renderer::DebugReplayFillMode debug_replay_pipeline_fill_mode =
      fm2::native_renderer::DebugReplayFillMode::kSolid;
  HWND debug_replay_window = nullptr;
  std::vector<OwnedDirectReplaySubmission> native_direct_draw_pending;
  std::vector<OwnedDirectReplaySubmission> debug_replay_pending;
};

std::mutex g_plume_mutex;
PlumeState g_plume;

constexpr uint32_t kSwapchainBufferCount = 2;
constexpr plume::RenderFormat kSwapchainFormat =
    plume::RenderFormat::B8G8R8A8_UNORM;
constexpr wchar_t kDebugReplayWindowClassName[] =
    L"ReXGlueFM2PlumeDebugReplayWindow";
constexpr wchar_t kDebugReplayWindowTitle[] = L"FM2 Plume Debug Replay";
constexpr wchar_t kCompareWindowTitle[] = L"FM2 Plume Compare";
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

struct PreparedDirectDebugReplayDraw {
  fm2::native_renderer::DirectDrawDebugReplayPlan plan;
  fm2::native_renderer::DirectDrawReplayTopology effective_topology =
      fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  fm2::native_renderer::DirectDrawReplayPipelineLayout layout =
      fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported;
  plume::RenderFormat index_format = plume::RenderFormat::UNKNOWN;
  std::unique_ptr<plume::RenderBuffer> stream0_buffer;
  std::unique_ptr<plume::RenderBuffer> stream1_buffer;
  std::unique_ptr<plume::RenderBuffer> index_buffer;
};

struct PreparedNativePassDraw {
  fm2::native_renderer::NativeDrawPacket packet;
  fm2::native_renderer::DirectDrawDebugReplayPlan plan;
  plume::RenderFormat index_format = plume::RenderFormat::UNKNOWN;
  std::unique_ptr<plume::RenderBuffer> stream0_buffer;
  std::unique_ptr<plume::RenderBuffer> stream1_buffer;
  std::unique_ptr<plume::RenderBuffer> index_buffer;
};

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

fm2::native_renderer::NativeWindowRect NativeWindowRectFromHwnd(HWND hwnd) {
  if (!hwnd || IsWindow(hwnd) == FALSE) {
    return {};
  }
  RECT rect = {};
  if (GetWindowRect(hwnd, &rect) == FALSE) {
    return {};
  }
  return {.x = rect.left,
          .y = rect.top,
          .width = rect.right - rect.left,
          .height = rect.bottom - rect.top};
}

bool CreateDebugReplayWindowLocked(HWND host_window) {
  const wchar_t* title = REXCVAR_GET(fm2_plume_compare_window)
                             ? kCompareWindowTitle
                             : kDebugReplayWindowTitle;
  if (g_plume.debug_replay_window &&
      IsWindow(g_plume.debug_replay_window) != FALSE) {
    SetWindowTextW(g_plume.debug_replay_window, title);
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
  int x = CW_USEDEFAULT;
  int y = CW_USEDEFAULT;
  fm2::native_renderer::NativeWindowRect host_rect = {};
  if (REXCVAR_GET(fm2_plume_debug_replay_side_by_side)) {
    host_rect = NativeWindowRectFromHwnd(host_window);
    const fm2::native_renderer::NativeWindowRect companion_rect =
        fm2::native_renderer::PlaceCompanionWindowBesideHost(
            host_rect, width, height,
            static_cast<int32_t>(
                REXCVAR_GET(fm2_plume_debug_replay_window_gap)));
    x = companion_rect.x;
    y = companion_rect.y;
  }
  g_plume.debug_replay_window = CreateWindowExW(
      kWindowExStyle, kDebugReplayWindowClassName, title,
      kWindowStyle, x, y, width, height, nullptr,
      nullptr, GetModuleHandleW(nullptr), nullptr);
  if (!g_plume.debug_replay_window) {
    REXLOG_WARN("FM2 Plume failed to create debug replay window error={}",
                GetLastError());
    return false;
  }

  ShowWindow(g_plume.debug_replay_window, SW_SHOWNOACTIVATE);
  UpdateWindow(g_plume.debug_replay_window);
  REXLOG_INFO(
      "FM2 Plume debug replay window created hwnd={:p} x={} y={} size={}x{} "
      "side_by_side={} host_x={} host_y={} host_size={}x{}",
      static_cast<void*>(g_plume.debug_replay_window), x, y, width, height,
      REXCVAR_GET(fm2_plume_debug_replay_side_by_side) ? 1u : 0u,
      host_rect.x, host_rect.y, host_rect.width, host_rect.height);
  return true;
}

void ResetPlumeStateLocked() {
  g_plume.native_direct_draw_pending.clear();
  g_plume.debug_replay_pending.clear();
  DestroyDebugReplayWindowLocked();
  g_plume.debug_replay_pipeline.reset();
  g_plume.debug_replay_pipeline_topology =
      fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  g_plume.debug_replay_pipeline_layout_kind =
      fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported;
  g_plume.debug_replay_pipeline_fill_mode =
      fm2::native_renderer::DebugReplayFillMode::kSolid;
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

plume::RenderFillMode DebugReplayFillModeToPlume(
    fm2::native_renderer::DebugReplayFillMode fill_mode) {
  switch (fill_mode) {
    case fm2::native_renderer::DebugReplayFillMode::kSolid:
      return plume::RenderFillMode::SOLID;
    case fm2::native_renderer::DebugReplayFillMode::kWireframe:
      return plume::RenderFillMode::WIREFRAME;
  }
  return plume::RenderFillMode::SOLID;
}

const char* DebugReplayFillModeName(
    fm2::native_renderer::DebugReplayFillMode fill_mode) {
  switch (fill_mode) {
    case fm2::native_renderer::DebugReplayFillMode::kSolid:
      return "solid";
    case fm2::native_renderer::DebugReplayFillMode::kWireframe:
      return "wireframe";
  }
  return "solid";
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
    fm2::native_renderer::DirectDrawReplayTopology topology,
    fm2::native_renderer::DirectDrawReplayPipelineLayout layout) {
  if (layout ==
      fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported) {
    return false;
  }
  const fm2::native_renderer::DebugReplayFillMode fill_mode =
      fm2::native_renderer::DebugReplayFillModeForWireframe(
          REXCVAR_GET(fm2_plume_wireframe));
  if (g_plume.debug_replay_pipeline &&
      g_plume.debug_replay_pipeline_topology == topology &&
      g_plume.debug_replay_pipeline_layout_kind == layout &&
      g_plume.debug_replay_pipeline_fill_mode == fill_mode) {
    return true;
  }
  if (g_plume.debug_replay_pipeline) {
    g_plume.debug_replay_pipeline.reset();
    g_plume.debug_replay_pipeline_topology =
        fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
    g_plume.debug_replay_pipeline_layout_kind =
        fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported;
    g_plume.debug_replay_pipeline_fill_mode =
        fm2::native_renderer::DebugReplayFillMode::kSolid;
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

  const unsigned char* vertex_shader_blob = fm2DebugReplayVertBlobDXIL;
  size_t vertex_shader_blob_size = fm2DebugReplayVertBlobDXIL_size;
  if (layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                    kNativePosition28Side12) {
    vertex_shader_blob = fm2DebugReplayNative28VertBlobDXIL;
    vertex_shader_blob_size = fm2DebugReplayNative28VertBlobDXIL_size;
  }

  g_plume.debug_replay_vertex_shader = g_plume.device->createShader(
      vertex_shader_blob, vertex_shader_blob_size, "VSMain", shader_format);
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
      plume::RenderInputElement("RAW", 0, 0,
                                plume::RenderFormat::R32G32B32A32_UINT, 0, 0),
      plume::RenderInputElement("RAW", 1, 1,
                                plume::RenderFormat::R32G32B32A32_UINT, 0, 16),
      plume::RenderInputElement("SIDE", 0, 2,
                                plume::RenderFormat::R32G32B32_UINT, 1, 0),
  };
  uint32_t input_element_count = 3u;
  if (layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                    kNativePosition28Side12) {
    input_slots = {
        plume::RenderInputSlot(0, 28u),
        plume::RenderInputSlot(1, 12u),
    };
    input_elements = {
        plume::RenderInputElement("NPOS", 0, 0,
                                  plume::RenderFormat::R32G32B32_UINT, 0, 0),
        plume::RenderInputElement("SIDE", 0, 1,
                                  plume::RenderFormat::R32G32B32_UINT, 1, 0),
        plume::RenderInputElement("UNUSED", 0, 0,
                                  plume::RenderFormat::UNKNOWN, 0, 0),
    };
    input_element_count = 2u;
  }

  plume::RenderGraphicsPipelineDesc pipeline_desc;
  pipeline_desc.inputSlots = input_slots.data();
  pipeline_desc.inputSlotsCount = static_cast<uint32_t>(input_slots.size());
  pipeline_desc.inputElements = input_elements.data();
  pipeline_desc.inputElementsCount = input_element_count;
  pipeline_desc.pipelineLayout = g_plume.debug_replay_pipeline_layout.get();
  pipeline_desc.vertexShader = g_plume.debug_replay_vertex_shader.get();
  pipeline_desc.pixelShader = g_plume.debug_replay_pixel_shader.get();
  pipeline_desc.renderTargetFormat[0] = kSwapchainFormat;
  pipeline_desc.renderTargetBlend[0] = plume::RenderBlendDesc::Copy();
  pipeline_desc.renderTargetCount = 1;
  pipeline_desc.primitiveTopology = ReplayTopologyToPlume(topology);
  pipeline_desc.cullMode = plume::RenderCullMode::NONE;
  pipeline_desc.fillMode = DebugReplayFillModeToPlume(fill_mode);

  g_plume.debug_replay_pipeline =
      g_plume.device->createGraphicsPipeline(pipeline_desc);
  if (!g_plume.debug_replay_pipeline) {
    REXLOG_WARN("FM2 Plume debug replay failed to create graphics pipeline");
    return false;
  }

  g_plume.debug_replay_pipeline_topology = topology;
  g_plume.debug_replay_pipeline_layout_kind = layout;
  g_plume.debug_replay_pipeline_fill_mode = fill_mode;
  REXLOG_INFO(
      "FM2 Plume debug replay pipeline initialized topology={} layout={} "
      "fill={}",
      static_cast<unsigned>(topology),
      fm2::native_renderer::DirectDrawReplayPipelineLayoutName(layout),
      DebugReplayFillModeName(fill_mode));
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

bool CreateDebugReplayWindowSwapchainLocked(rex::ui::Window* host_window) {
  HWND host_hwnd = nullptr;
  if (host_window) {
    host_hwnd = static_cast<HWND>(host_window->GetNativeWindowHandle());
  }
  if (!CreateDebugReplayWindowLocked(host_hwnd)) {
    return false;
  }
  return CreateSwapchainForNativeWindowLocked(
      reinterpret_cast<plume::RenderWindow>(g_plume.debug_replay_window));
}

plume::RenderColor ReplayClearColorLocked() {
  const plume::RenderColor kDefault(0.0f, 0.02f, 0.04f, 1.0f);
  const fm2::native_renderer::NativeStateSnapshot snapshot =
      fm2::native_renderer::SnapshotLastNativeState();
  if (!snapshot.clear.valid) {
    return kDefault;
  }
  const float grayscale =
      static_cast<float>(snapshot.clear.clear_color_byte) * (1.0f / 255.0f);
  return plume::RenderColor(grayscale, grayscale, grayscale, 1.0f);
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
  const fm2::native_renderer::DirectDrawReplayPipelineLayout layout =
      fm2::native_renderer::DirectDrawReplayPipelineLayoutForPlan(plan);
  if (effective_topology ==
          fm2::native_renderer::DirectDrawReplayTopology::kUnknown ||
      layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                    kUnsupported) {
    REXLOG_WARN(
        "FM2 Plume debug replay skipped unsupported replay contract "
        "topology={} layout={} s0_stride={} s1_stride={}",
        static_cast<unsigned>(effective_topology),
        fm2::native_renderer::DirectDrawReplayPipelineLayoutName(layout),
        plan.streams[0].stride,
        plan.streams[1].stride);
    return false;
  }
  if (!g_plume.device || !g_plume.command_queue || !g_plume.command_list ||
      !g_plume.swapchain || !g_plume.acquire_semaphore || !g_plume.fence ||
      !ResizeSwapchainIfNeededLocked() ||
      !EnsureDebugReplayPipelineLocked(effective_topology, layout)) {
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
  g_plume.command_list->clearColor(0, ReplayClearColorLocked());
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
      "topology={} plan_topology={} layout={} transform_valid={} "
      "transform_first={} transform_mode={}",
      presented, image_index, width, height, plan.draw.index_count,
      plan.streams[0].guest_base, plan.streams[0].upload_guest_base,
      plan.streams[1].guest_base, plan.streams[1].upload_guest_base,
      plan.index.guest_base, plan.index.upload_guest_base,
      plan.streams[0].upload_bytes, plan.streams[1].upload_bytes,
      plan.index.upload_bytes, plan.streams[0].hash, plan.streams[1].hash,
      plan.index.hash, static_cast<unsigned>(effective_topology),
      static_cast<unsigned>(plan.topology),
      fm2::native_renderer::DirectDrawReplayPipelineLayoutName(layout),
      plan.transform.valid ? 1u : 0u, plan.transform.first_constant,
      DebugReplayTransformModeName(push_constants.transform_mode));
  return presented;
}

bool RenderDirectDebugReplayBatchLocked(
    const fm2::native_renderer::DirectDrawReplaySubmission* submissions,
    uint32_t submission_count, const char* submit_log_name) {
  if (!submissions || submission_count == 0 || !g_plume.device ||
      !g_plume.command_queue || !g_plume.command_list || !g_plume.swapchain ||
      !g_plume.acquire_semaphore || !g_plume.fence ||
      !ResizeSwapchainIfNeededLocked()) {
    return false;
  }

  std::vector<PreparedDirectDebugReplayDraw> draws;
  draws.reserve(submission_count);
  uint32_t skipped = 0;
  uint32_t native_state_draws = 0;
  uint32_t native_packet_draws = 0;
  uint32_t native_layout_draws = 0;
  fm2::native_renderer::DirectDrawReplayTopology batch_topology =
      fm2::native_renderer::DirectDrawReplayTopology::kUnknown;
  fm2::native_renderer::DirectDrawReplayPipelineLayout batch_layout =
      fm2::native_renderer::DirectDrawReplayPipelineLayout::kUnsupported;

  for (uint32_t i = 0; i < submission_count; ++i) {
    const auto& submission = submissions[i];
    const auto& plan = submission.plan;
    const auto& sources = submission.sources;
    if (!plan.ready || plan.stream_count != 2u || !sources.stream0 ||
        !sources.stream1 || !sources.index) {
      ++skipped;
      continue;
    }

    const fm2::native_renderer::DirectDrawReplayTopology effective_topology =
        EffectiveDebugReplayTopology(plan.topology);
    const fm2::native_renderer::DirectDrawReplayPipelineLayout layout =
        fm2::native_renderer::DirectDrawReplayPipelineLayoutForPlan(plan);
    if (effective_topology ==
            fm2::native_renderer::DirectDrawReplayTopology::kUnknown ||
        layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                      kUnsupported) {
      ++skipped;
      continue;
    }
    if (batch_topology ==
        fm2::native_renderer::DirectDrawReplayTopology::kUnknown) {
      batch_topology = effective_topology;
    } else if (batch_topology != effective_topology) {
      ++skipped;
      continue;
    }
    if (batch_layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                            kUnsupported) {
      batch_layout = layout;
    } else if (batch_layout != layout) {
      ++skipped;
      continue;
    }

    const plume::RenderFormat index_format =
        ReplayIndexFormatToPlume(plan.index_format);
    if (index_format == plume::RenderFormat::UNKNOWN) {
      ++skipped;
      continue;
    }

    PreparedDirectDebugReplayDraw prepared;
    prepared.plan = plan;
    prepared.effective_topology = effective_topology;
    prepared.layout = layout;
    prepared.index_format = index_format;
    prepared.stream0_buffer = CreateConvertedReplayBufferLocked(
        "FM2 compare stream0", sources.stream0, plan.streams[0].upload_bytes,
        plan.streams[0].upload_endian, plume::RenderBufferFlag::VERTEX);
    prepared.stream1_buffer = CreateConvertedReplayBufferLocked(
        "FM2 compare stream1", sources.stream1, plan.streams[1].upload_bytes,
        plan.streams[1].upload_endian, plume::RenderBufferFlag::VERTEX);
    prepared.index_buffer = CreateConvertedReplayBufferLocked(
        "FM2 compare index", sources.index, plan.index.upload_bytes,
        plan.index.upload_endian, plume::RenderBufferFlag::INDEX);
    if (!prepared.stream0_buffer || !prepared.stream1_buffer ||
        !prepared.index_buffer) {
      ++skipped;
      continue;
    }
    if (plan.native_state.valid) {
      ++native_state_draws;
    }
    if (fm2::native_renderer::BuildNativeDrawPacket(plan).ready) {
      ++native_packet_draws;
    }
    if (layout == fm2::native_renderer::DirectDrawReplayPipelineLayout::
                      kNativePosition28Side12) {
      ++native_layout_draws;
    }
    draws.push_back(std::move(prepared));
  }

  if (draws.empty() ||
      !EnsureDebugReplayPipelineLocked(batch_topology, batch_layout)) {
    return false;
  }

  uint32_t image_index = 0;
  if (!g_plume.swapchain->acquireTexture(g_plume.acquire_semaphore.get(),
                                         &image_index)) {
    REXLOG_WARN("FM2 Plume compare replay failed to acquire swapchain texture");
    return false;
  }
  if (image_index >= g_plume.framebuffers.size()) {
    REXLOG_WARN("FM2 Plume compare replay acquired invalid image index {}",
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
  g_plume.command_list->setScissors(plume::RenderRect(0, 0, width, height));
  g_plume.command_list->clearColor(0, ReplayClearColorLocked());
  g_plume.command_list->setGraphicsPipelineLayout(
      g_plume.debug_replay_pipeline_layout.get());
  g_plume.command_list->setPipeline(g_plume.debug_replay_pipeline.get());

  for (PreparedDirectDebugReplayDraw& draw : draws) {
    std::array<plume::RenderVertexBufferView, 2> vertex_views = {
        plume::RenderVertexBufferView(draw.stream0_buffer.get(),
                                      draw.plan.streams[0].upload_bytes),
        plume::RenderVertexBufferView(draw.stream1_buffer.get(),
                                      draw.plan.streams[1].upload_bytes),
    };
    std::array<plume::RenderInputSlot, 2> input_slots = {
        plume::RenderInputSlot(draw.plan.streams[0].slot,
                               draw.plan.streams[0].stride),
        plume::RenderInputSlot(draw.plan.streams[1].slot,
                               draw.plan.streams[1].stride),
    };
    plume::RenderIndexBufferView index_view(draw.index_buffer.get(),
                                            draw.plan.index.upload_bytes,
                                            draw.index_format);
    const DebugReplayPushConstants push_constants =
        BuildDebugReplayPushConstants(draw.plan);

    g_plume.command_list->setGraphicsPushConstants(0, &push_constants);
    g_plume.command_list->setVertexBuffers(
        0, vertex_views.data(), static_cast<uint32_t>(vertex_views.size()),
        input_slots.data());
    g_plume.command_list->setIndexBuffer(&index_view);
    g_plume.command_list->drawIndexedInstanced(
        draw.plan.draw.index_count, draw.plan.draw.instance_count,
        draw.plan.draw.start_index, draw.plan.draw.base_vertex,
        draw.plan.draw.start_instance);
  }

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
      "{} presented={} image={} size={}x{} "
      "draws={} skipped={} native_state_draws={} native_packet_draws={} "
      "native_layout_draws={} topology={} layout={}",
      submit_log_name ? submit_log_name : "FM2_PLUME_DEBUG_REPLAY_BATCH_SUBMIT",
      presented, image_index, width, height, draws.size(), skipped,
      native_state_draws, native_packet_draws, native_layout_draws,
      static_cast<unsigned>(batch_topology),
      fm2::native_renderer::DirectDrawReplayPipelineLayoutName(batch_layout));
  return presented;
}

bool CopyReplaySourceBytes(const uint8_t* source, uint32_t byte_count,
                           std::vector<uint8_t>& out) {
  if (!source || byte_count == 0) {
    return false;
  }
  out.assign(source, source + byte_count);
  return true;
}

bool BuildOwnedDirectReplaySubmission(
    const fm2::native_renderer::DirectDrawDebugReplayPlan& plan,
    const fm2::native_renderer::DirectDrawReplaySourceBytes& sources,
    const fm2::native_renderer::NativeDrawPacket& native_packet,
    OwnedDirectReplaySubmission& out) {
  if (!plan.ready || plan.stream_count != 2u || !sources.stream0 ||
      !sources.stream1 || !sources.index || !native_packet.ready) {
    return false;
  }

  OwnedDirectReplaySubmission pending;
  pending.plan = plan;
  pending.native_packet = native_packet;
  if (!CopyReplaySourceBytes(sources.stream0, plan.streams[0].upload_bytes,
                             pending.stream0) ||
      !CopyReplaySourceBytes(sources.stream1, plan.streams[1].upload_bytes,
                             pending.stream1) ||
      !CopyReplaySourceBytes(sources.index, plan.index.upload_bytes,
                             pending.index)) {
    return false;
  }

  out = std::move(pending);
  return true;
}

bool QueueNativeDirectDrawLocked(
    const fm2::native_renderer::DirectDrawDebugReplayPlan& plan,
    const fm2::native_renderer::DirectDrawReplaySourceBytes& sources,
    const fm2::native_renderer::NativeDrawPacket& native_packet) {
  OwnedDirectReplaySubmission pending;
  if (!BuildOwnedDirectReplaySubmission(plan, sources, native_packet,
                                        pending)) {
    return false;
  }

  g_plume.native_direct_draw_pending.push_back(std::move(pending));
  return true;
}

bool RenderNativePassCommandLocked(
    const fm2::native_renderer::NativePassCommand& command,
    const OwnedDirectReplaySubmission* submissions, uint32_t submission_count,
    const char* submit_log_name) {
  const fm2::native_renderer::NativePassSubmitPlan submit_plan =
      fm2::native_renderer::BuildNativePassSubmitPlan(command);
  if (!submit_plan.ready) {
    REXLOG_WARN(
        "FM2_PLUME_NATIVE_PASS_SUBMIT_REJECT reason={} command_reason={} "
        "draws={}",
        fm2::native_renderer::NativePassSubmitRejectReasonName(
            submit_plan.reject_reason),
        fm2::native_renderer::NativePassCommandRejectReasonName(
            submit_plan.command_reject_reason),
        command.draw_count);
    return false;
  }
  if (!submissions || submission_count != submit_plan.draw_count) {
    REXLOG_WARN(
        "FM2_PLUME_NATIVE_PASS_SUBMIT_REJECT reason=submission_mismatch "
        "submissions={} draws={}",
        submission_count, submit_plan.draw_count);
    return false;
  }

  const fm2::native_renderer::DirectDrawReplayTopology effective_topology =
      EffectiveDebugReplayTopology(submit_plan.topology);
  const plume::RenderFormat index_format =
      ReplayIndexFormatToPlume(submit_plan.index_format);
  if (effective_topology ==
          fm2::native_renderer::DirectDrawReplayTopology::kUnknown ||
      index_format == plume::RenderFormat::UNKNOWN) {
    return false;
  }
  if (!g_plume.device || !g_plume.command_queue || !g_plume.command_list ||
      !g_plume.swapchain || !g_plume.acquire_semaphore || !g_plume.fence ||
      !ResizeSwapchainIfNeededLocked() ||
      !EnsureDebugReplayPipelineLocked(effective_topology,
                                       submit_plan.layout)) {
    return false;
  }

  std::vector<PreparedNativePassDraw> draws;
  draws.reserve(submission_count);
  uint32_t skipped = 0;
  for (uint32_t i = 0; i < submission_count; ++i) {
    const OwnedDirectReplaySubmission& pending = submissions[i];
    const fm2::native_renderer::NativeDrawPacket& packet =
        submit_plan.draws[i];
    if (pending.native_packet.source_sequence != packet.source_sequence ||
        !fm2::native_renderer::NativeDrawPassKeysEqual(
            pending.native_packet.pass, packet.pass)) {
      ++skipped;
      continue;
    }
    if (pending.stream0.size() <
            packet.resources.streams[0].replay_upload_bytes ||
        pending.stream1.size() <
            packet.resources.streams[1].replay_upload_bytes ||
        pending.index.size() < packet.resources.index.replay_upload_bytes) {
      ++skipped;
      continue;
    }

    PreparedNativePassDraw prepared;
    prepared.packet = packet;
    prepared.plan = pending.plan;
    prepared.index_format = index_format;
    prepared.stream0_buffer = CreateConvertedReplayBufferLocked(
        "FM2 native pass stream0", pending.stream0.data(),
        packet.resources.streams[0].replay_upload_bytes,
        packet.resources.streams[0].replay_upload_endian,
        plume::RenderBufferFlag::VERTEX);
    prepared.stream1_buffer = CreateConvertedReplayBufferLocked(
        "FM2 native pass stream1", pending.stream1.data(),
        packet.resources.streams[1].replay_upload_bytes,
        packet.resources.streams[1].replay_upload_endian,
        plume::RenderBufferFlag::VERTEX);
    prepared.index_buffer = CreateConvertedReplayBufferLocked(
        "FM2 native pass index", pending.index.data(),
        packet.resources.index.replay_upload_bytes,
        packet.resources.index.replay_upload_endian,
        plume::RenderBufferFlag::INDEX);
    if (!prepared.stream0_buffer || !prepared.stream1_buffer ||
        !prepared.index_buffer) {
      ++skipped;
      continue;
    }

    draws.push_back(std::move(prepared));
  }

  if (draws.empty()) {
    return false;
  }

  uint32_t image_index = 0;
  if (!g_plume.swapchain->acquireTexture(g_plume.acquire_semaphore.get(),
                                         &image_index)) {
    REXLOG_WARN("FM2 Plume native pass failed to acquire swapchain texture");
    return false;
  }
  if (image_index >= g_plume.framebuffers.size()) {
    REXLOG_WARN("FM2 Plume native pass acquired invalid image index {}",
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
  g_plume.command_list->setScissors(plume::RenderRect(0, 0, width, height));
  g_plume.command_list->clearColor(0, ReplayClearColorLocked());
  g_plume.command_list->setGraphicsPipelineLayout(
      g_plume.debug_replay_pipeline_layout.get());
  g_plume.command_list->setPipeline(g_plume.debug_replay_pipeline.get());

  for (PreparedNativePassDraw& draw : draws) {
    const fm2::native_renderer::NativeDrawPacket& packet = draw.packet;
    std::array<plume::RenderVertexBufferView, 2> vertex_views = {
        plume::RenderVertexBufferView(
            draw.stream0_buffer.get(),
            packet.resources.streams[0].replay_upload_bytes),
        plume::RenderVertexBufferView(
            draw.stream1_buffer.get(),
            packet.resources.streams[1].replay_upload_bytes),
    };
    std::array<plume::RenderInputSlot, 2> input_slots = {
        plume::RenderInputSlot(
            packet.resources.streams[0].slot,
            packet.resources.streams[0].native_stride_bytes),
        plume::RenderInputSlot(
            packet.resources.streams[1].slot,
            packet.resources.streams[1].native_stride_bytes),
    };
    plume::RenderIndexBufferView index_view(
        draw.index_buffer.get(), packet.resources.index.replay_upload_bytes,
        draw.index_format);
    const DebugReplayPushConstants push_constants =
        BuildDebugReplayPushConstants(draw.plan);

    g_plume.command_list->setGraphicsPushConstants(0, &push_constants);
    g_plume.command_list->setVertexBuffers(
        0, vertex_views.data(), static_cast<uint32_t>(vertex_views.size()),
        input_slots.data());
    g_plume.command_list->setIndexBuffer(&index_view);
    g_plume.command_list->drawIndexedInstanced(
        packet.draw.index_count, packet.draw.instance_count,
        packet.draw.start_index, packet.draw.base_vertex,
        packet.draw.start_instance);
  }

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
      "{} presented={} image={} size={}x{} draws={} skipped={} topology={} "
      "layout={} submit_object={:08X} pass_marker={} first_sequence={} "
      "last_sequence={}",
      submit_log_name ? submit_log_name : "FM2_PLUME_NATIVE_PASS_SUBMIT",
      presented ? 1u : 0u, image_index, width, height, draws.size(), skipped,
      static_cast<unsigned>(effective_topology),
      fm2::native_renderer::DirectDrawReplayPipelineLayoutName(
          submit_plan.layout),
      submit_plan.pass.submit_object, submit_plan.pass.pass_marker,
      command.first_source_sequence, command.last_source_sequence);
  return presented;
}

bool FlushNativeDirectDrawBatchLocked(const char* reason) {
  if (g_plume.native_direct_draw_pending.empty()) {
    return true;
  }

  std::vector<fm2::native_renderer::NativeDrawPacket> native_packets;
  native_packets.reserve(g_plume.native_direct_draw_pending.size());
  for (const OwnedDirectReplaySubmission& pending :
       g_plume.native_direct_draw_pending) {
    native_packets.push_back(pending.native_packet);
  }

  const fm2::native_renderer::NativePassCommand native_pass =
      fm2::native_renderer::BuildNativePassCommand(
          native_packets.data(), static_cast<uint32_t>(native_packets.size()));
  const uint32_t queued_count = static_cast<uint32_t>(native_packets.size());
  if (!native_pass.ready) {
    REXLOG_WARN(
        "FM2_PLUME_NATIVE_PASS_REJECT reason={} queued={} "
        "packet_reject={} first_sequence={} last_sequence={}",
        fm2::native_renderer::NativePassCommandRejectReasonName(
            native_pass.reject_reason),
        queued_count,
        fm2::native_renderer::NativeDrawPacketRejectReasonName(
            native_pass.first_packet_reject_reason),
        native_pass.first_source_sequence, native_pass.last_source_sequence);
    g_plume.native_direct_draw_pending.clear();
    return false;
  }

  const bool submitted = RenderNativePassCommandLocked(
      native_pass, g_plume.native_direct_draw_pending.data(), queued_count,
      "FM2_PLUME_NATIVE_LIVE_BATCH_SUBMIT");
  REXLOG_INFO(
      "FM2_PLUME_NATIVE_LIVE_BATCH_RESULT reason={} queued={} submitted={} "
      "pass_draws={} first_sequence={} last_sequence={} submit_object={:08X} "
      "pass_marker={}",
      reason ? reason : "unknown", queued_count, submitted ? 1u : 0u,
      native_pass.draw_count, native_pass.first_source_sequence,
      native_pass.last_source_sequence, native_pass.pass.submit_object,
      native_pass.pass.pass_marker);
  g_plume.native_direct_draw_pending.clear();
  return submitted;
}

bool QueueDebugReplayLocked(
    const fm2::native_renderer::DirectDrawDebugReplayPlan& plan,
    const fm2::native_renderer::DirectDrawReplaySourceBytes& sources) {
  if (!plan.ready || plan.stream_count != 2u || !sources.stream0 ||
      !sources.stream1 || !sources.index) {
    return false;
  }
  OwnedDirectReplaySubmission pending;
  pending.plan = plan;
  if (!CopyReplaySourceBytes(sources.stream0, plan.streams[0].upload_bytes,
                             pending.stream0) ||
      !CopyReplaySourceBytes(sources.stream1, plan.streams[1].upload_bytes,
                             pending.stream1) ||
      !CopyReplaySourceBytes(sources.index, plan.index.upload_bytes,
                             pending.index)) {
    return false;
  }
  g_plume.debug_replay_pending.push_back(std::move(pending));
  return true;
}

bool FlushDebugReplayBatchLocked(const char* reason) {
  if (g_plume.debug_replay_pending.empty()) {
    return true;
  }
  std::vector<fm2::native_renderer::DirectDrawReplaySubmission> submissions;
  submissions.reserve(g_plume.debug_replay_pending.size());
  for (const OwnedDirectReplaySubmission& owned : g_plume.debug_replay_pending) {
    submissions.push_back({owned.plan,
                           {owned.stream0.data(), owned.stream1.data(),
                            owned.index.data()}});
  }
  const uint32_t queued_count =
      static_cast<uint32_t>(g_plume.debug_replay_pending.size());
  const bool submitted = RenderDirectDebugReplayBatchLocked(
      submissions.data(), queued_count,
      "FM2_PLUME_DEBUG_REPLAY_LIVE_BATCH_SUBMIT");
  REXLOG_INFO(
      "FM2_PLUME_DEBUG_REPLAY_LIVE_BATCH reason={} queued={} submitted={}",
      reason ? reason : "unknown", queued_count, submitted ? 1u : 0u);
  g_plume.debug_replay_pending.clear();
  return submitted;
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
  return REXCVAR_GET(fm2_plume_debug_replay) ||
         REXCVAR_GET(fm2_plume_compare_window);
}

bool WantsNativeDirectDraw() {
  if (!REXCVAR_GET(fm2_plume_native_direct_draw)) {
    return false;
  }
  const uint32_t limit = REXCVAR_GET(fm2_plume_native_direct_draw_limit);
  return limit == 0 ||
         g_native_direct_draw_attempts.load(std::memory_order_relaxed) < limit;
}

bool WantsNativeDirectDrawLiveBatch() {
  return REXCVAR_GET(fm2_plume_native_direct_draw) &&
         REXCVAR_GET(fm2_plume_native_direct_draw_live_batch) &&
         REXCVAR_GET(fm2_plume_native_direct_draw_limit) == 0;
}

bool WantsCompareWindow() {
  return REXCVAR_GET(fm2_plume_compare_window);
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

  const bool wants_direct_plume_submission =
      WantsDirectDebugReplay() || WantsNativeDirectDraw();
  const bool wants_debug_replay_swapchain =
      mode == Mode::kShadow && wants_direct_plume_submission &&
      REXCVAR_GET(fm2_plume_debug_replay_shadow_swapchain);
  const bool wants_debug_replay_window =
      mode == Mode::kShadow && wants_direct_plume_submission &&
      (REXCVAR_GET(fm2_plume_debug_replay_window) || WantsCompareWindow());
  if (mode == Mode::kPlumeClear) {
    if (!CreateSwapchainLocked(window)) {
      REXLOG_WARN("FM2 Plume clear mode could not create swapchain");
      return false;
    }
  } else if (wants_debug_replay_window) {
    if (!CreateDebugReplayWindowSwapchainLocked(window)) {
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
    if (!g_plume.native_direct_draw_pending.empty()) {
      FlushNativeDirectDrawBatchLocked("shutdown");
    }
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
        "direct_indexed_draw={} instance_hybrid_draw={} "
        "debug_replay_attempts={} "
        "debug_replay_submitted={} debug_replay_failed={} "
        "native_direct_draw_attempts={} native_direct_draw_submitted={} "
        "native_direct_draw_failed={} last_hook={:08X}",
        g_build_object_pass_entries.load(std::memory_order_relaxed),
        g_direct_indexed_draw_entries.load(std::memory_order_relaxed),
        g_instance_hybrid_draw_entries.load(std::memory_order_relaxed),
        g_debug_replay_attempts.load(std::memory_order_relaxed),
        g_debug_replay_submitted.load(std::memory_order_relaxed),
        g_debug_replay_failed.load(std::memory_order_relaxed),
        g_native_direct_draw_attempts.load(std::memory_order_relaxed),
        g_native_direct_draw_submitted.load(std::memory_order_relaxed),
        g_native_direct_draw_failed.load(std::memory_order_relaxed),
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
  StoreLastArgs(kFM2RenderBuildObjectPassCommandBufferAddress, args);
  MaybeLogPacket("FM2_PLUME_BUILD_OBJECT_PASS",
                 kFM2RenderBuildObjectPassCommandBufferAddress, count, args);
}

void RecordDirectIndexedDrawEntry(const GuestArgs& args) {
  const uint64_t count = g_direct_indexed_draw_entries.fetch_add(
                             1, std::memory_order_relaxed) +
                         1;
  StoreLastArgs(kFM2RenderBuildDirectIndexedDrawBuffersAddress, args);
  MaybeLogPacket("FM2_PLUME_DIRECT_INDEXED_DRAW_BUILD",
                 kFM2RenderBuildDirectIndexedDrawBuffersAddress, count, args);
}

void RecordInstanceHybridDrawEntry(const GuestArgs& args) {
  const uint64_t count = g_instance_hybrid_draw_entries.fetch_add(
                             1, std::memory_order_relaxed) +
                         1;
  StoreLastArgs(kFM2RenderInstanceHybridDrawPathAddress, args);
  MaybeLogPacket("FM2_PLUME_INSTANCE_HYBRID_DRAW",
                 kFM2RenderInstanceHybridDrawPathAddress, count, args);
}

void RecordPresentEntry(uint32_t present_chain_object) {
  const uint64_t count =
      g_present_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  GuestArgs args{.r3 = present_chain_object};
  StoreLastArgs(kFM2D3DTryPresentAndUpdateStatusAddress, args);
  MaybeLogPacket("FM2_PLUME_PRESENT", kFM2D3DTryPresentAndUpdateStatusAddress,
                 count, args);
}

bool FlushNativeDirectDrawOnPresent() {
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  if (!WantsNativeDirectDrawLiveBatch()) {
    return true;
  }
  std::scoped_lock lock(g_plume_mutex);
  return FlushNativeDirectDrawBatchLocked("present");
#else
  return true;
#endif
}

bool FlushDebugReplayOnPresent() {
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  if (!REXCVAR_GET(fm2_plume_debug_replay_live_batch) ||
      !WantsDirectDebugReplay()) {
    return true;
  }
  std::scoped_lock lock(g_plume_mutex);
  return FlushDebugReplayBatchLocked("present");
#else
  return true;
#endif
}

bool SubmitDirectDebugReplay(const DirectDrawDebugReplayPlan& plan,
                             const DirectDrawReplaySourceBytes& sources) {
  if (!WantsDirectDebugReplay()) {
    return false;
  }

  const uint64_t attempt =
      g_debug_replay_attempts.fetch_add(1, std::memory_order_relaxed) + 1;
  const uint32_t limit = REXCVAR_GET(fm2_plume_debug_replay_limit);
  if (DirectDebugReplaySubmitLimitReached(attempt, limit,
                                          WantsCompareWindow())) {
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
  if (REXCVAR_GET(fm2_plume_debug_replay_live_batch)) {
    bool queued = false;
    {
      std::scoped_lock lock(g_plume_mutex);
      queued = QueueDebugReplayLocked(plan, sources);
    }
    if (queued) {
      g_debug_replay_submitted.fetch_add(1, std::memory_order_relaxed);
    } else {
      g_debug_replay_failed.fetch_add(1, std::memory_order_relaxed);
    }
    return queued;
  }
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

bool SubmitNativeDirectDraw(const DirectDrawDebugReplayPlan& plan,
                            const DirectDrawReplaySourceBytes& sources) {
  if (!REXCVAR_GET(fm2_plume_native_direct_draw)) {
    return false;
  }

  const uint64_t attempt =
      g_native_direct_draw_attempts.fetch_add(1, std::memory_order_relaxed) +
      1;
  const uint32_t limit = REXCVAR_GET(fm2_plume_native_direct_draw_limit);
  if (DirectDebugReplaySubmitLimitReached(attempt, limit, false)) {
    return false;
  }

  const Mode mode = GetMode();
  if (mode != Mode::kPlumeClear && mode != Mode::kShadow) {
    g_native_direct_draw_failed.fetch_add(1, std::memory_order_relaxed);
    REXLOG_WARN(
        "FM2 Plume native direct draw requires fm2_plume_mode=shadow or "
        "plume_clear; current mode={}",
        GetModeName(mode));
    return false;
  }
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  const NativeDrawPacket native_packet = BuildNativeDrawPacket(plan);
  if (!native_packet.ready) {
    g_native_direct_draw_failed.fetch_add(1, std::memory_order_relaxed);
    REXLOG_WARN(
        "FM2_PLUME_NATIVE_DIRECT_DRAW_REJECT attempt={} reason={} "
        "native_state={} pass={} pipeline={} resources={}",
        attempt, NativeDrawPacketRejectReasonName(native_packet.reject_reason),
        plan.native_state.valid ? 1u : 0u,
        native_packet.pass.valid ? 1u : 0u,
        native_packet.pipeline.valid ? 1u : 0u,
        native_packet.resources.valid ? 1u : 0u);
    return false;
  }

  bool accepted = false;
  bool queued = false;
  bool flushed = false;
  uint32_t pending_count = 0;
  {
    std::scoped_lock lock(g_plume_mutex);
    if (WantsNativeDirectDrawLiveBatch()) {
      if (!g_plume.native_direct_draw_pending.empty() &&
          !NativeDrawPassKeysEqual(
              g_plume.native_direct_draw_pending.front().native_packet.pass,
              native_packet.pass)) {
        flushed = true;
        accepted = FlushNativeDirectDrawBatchLocked("pass_change");
        pending_count =
            static_cast<uint32_t>(g_plume.native_direct_draw_pending.size());
      } else {
        accepted = true;
      }

      if (accepted) {
        queued = QueueNativeDirectDrawLocked(plan, sources, native_packet);
      }
      if (queued) {
        pending_count =
            static_cast<uint32_t>(g_plume.native_direct_draw_pending.size());
        if (ShouldFlushNativeDirectDrawLiveBatch(
                pending_count,
                REXCVAR_GET(fm2_plume_native_direct_draw_live_batch_size)) ||
            pending_count >= kNativePassCommandMaxDraws) {
          flushed = true;
          accepted = FlushNativeDirectDrawBatchLocked(
              pending_count >= kNativePassCommandMaxDraws ? "pass_draw_limit"
                                                          : "batch_size");
          pending_count =
              static_cast<uint32_t>(g_plume.native_direct_draw_pending.size());
        } else {
          accepted = true;
        }
      }
    } else {
      const NativePassCommand native_pass =
          BuildNativePassCommand(&native_packet, 1u);
      if (native_pass.ready) {
        OwnedDirectReplaySubmission submission;
        if (BuildOwnedDirectReplaySubmission(plan, sources, native_packet,
                                             submission)) {
          accepted = RenderNativePassCommandLocked(
              native_pass, &submission, 1u,
              "FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT_PASS");
        }
      } else {
        REXLOG_WARN(
            "FM2_PLUME_NATIVE_DIRECT_DRAW_PASS_REJECT attempt={} reason={} "
            "packet_reject={}",
            attempt,
            NativePassCommandRejectReasonName(native_pass.reject_reason),
            NativeDrawPacketRejectReasonName(
                native_pass.first_packet_reject_reason));
      }
    }
  }
  if (accepted) {
    g_native_direct_draw_submitted.fetch_add(1, std::memory_order_relaxed);
  } else {
    g_native_direct_draw_failed.fetch_add(1, std::memory_order_relaxed);
  }
  REXLOG_INFO(
      "FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT attempt={} submitted={} "
      "queued={} flushed={} pending={} live_batch={} "
      "mode={} topology={} layout={} index_count={} native_state={} "
      "viewport={} texture_fetch={} clear={} pass={} pass_flags={:08X} "
      "native_packet={} native_reject={} native_layout={} transform_valid={}",
      attempt, accepted ? 1u : 0u, queued ? 1u : 0u, flushed ? 1u : 0u,
      pending_count, WantsNativeDirectDrawLiveBatch() ? 1u : 0u,
      GetModeName(mode),
      static_cast<unsigned>(plan.topology),
      DirectDrawReplayPipelineLayoutName(
          DirectDrawReplayPipelineLayoutForPlan(plan)),
      plan.draw.index_count, plan.native_state.valid ? 1u : 0u,
      plan.native_state.viewport.valid ? 1u : 0u,
      plan.native_state.texture_fetch.valid ? 1u : 0u,
      plan.native_state.clear.valid ? 1u : 0u,
      plan.native_state.pass.valid ? 1u : 0u,
      plan.native_state.pass.pass_flags,
      native_packet.ready ? 1u : 0u,
      NativeDrawPacketRejectReasonName(native_packet.reject_reason),
      DirectDrawReplayPipelineLayoutName(native_packet.pipeline.native_layout),
      plan.transform.valid ? 1u : 0u);
  return accepted;
#else
  (void)sources;
  g_native_direct_draw_failed.fetch_add(1, std::memory_order_relaxed);
  REXLOG_WARN("FM2 Plume native direct draw requested without Plume support");
  return false;
#endif
}

bool SubmitDirectDebugReplayBatch(const DirectDrawReplaySubmission* submissions,
                                  uint32_t submission_count) {
  if (!WantsCompareWindow() || !submissions || submission_count == 0) {
    return false;
  }

  g_debug_replay_attempts.fetch_add(submission_count,
                                    std::memory_order_relaxed);

  const Mode mode = GetMode();
  if (mode != Mode::kShadow) {
    g_debug_replay_failed.fetch_add(submission_count,
                                    std::memory_order_relaxed);
    REXLOG_WARN("FM2 Plume compare replay requires fm2_plume_mode=shadow; "
                "current mode={}",
                GetModeName(mode));
    return false;
  }

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  bool submitted = false;
  {
    std::scoped_lock lock(g_plume_mutex);
    submitted = RenderDirectDebugReplayBatchLocked(
        submissions, submission_count, "FM2_PLUME_COMPARE_REPLAY_SUBMIT");
  }
  if (submitted) {
    g_debug_replay_submitted.fetch_add(submission_count,
                                       std::memory_order_relaxed);
  } else {
    g_debug_replay_failed.fetch_add(submission_count,
                                    std::memory_order_relaxed);
  }
  return submitted;
#else
  g_debug_replay_failed.fetch_add(submission_count, std::memory_order_relaxed);
  REXLOG_WARN("FM2 Plume compare replay requested without Plume support");
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
  out.instance_hybrid_draw_entries =
      g_instance_hybrid_draw_entries.load(std::memory_order_relaxed);
  out.present_entries = g_present_entries.load(std::memory_order_relaxed);
  out.debug_replay_attempts =
      g_debug_replay_attempts.load(std::memory_order_relaxed);
  out.debug_replay_submitted =
      g_debug_replay_submitted.load(std::memory_order_relaxed);
  out.debug_replay_failed =
      g_debug_replay_failed.load(std::memory_order_relaxed);
  out.native_direct_draw_attempts =
      g_native_direct_draw_attempts.load(std::memory_order_relaxed);
  out.native_direct_draw_submitted =
      g_native_direct_draw_submitted.load(std::memory_order_relaxed);
  out.native_direct_draw_failed =
      g_native_direct_draw_failed.load(std::memory_order_relaxed);
  out.last_hook_address = g_last_hook_address.load(std::memory_order_relaxed);
  {
    std::scoped_lock lock(g_last_args_mutex);
    out.last_args = g_last_args;
  }
  return out;
}

}  // namespace fm2::native_renderer
