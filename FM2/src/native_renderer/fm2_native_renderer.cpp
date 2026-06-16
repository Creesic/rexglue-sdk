#include "native_renderer/fm2_native_renderer.h"

#include <atomic>
#include <mutex>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>

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

std::atomic<bool> g_initialized{false};
std::atomic<bool> g_plume_available{false};
std::atomic<bool> g_plume_device_ready{false};
std::atomic<bool> g_swapchain_ready{false};
std::atomic<uint64_t> g_build_object_pass_entries{0};
std::atomic<uint64_t> g_direct_indexed_draw_entries{0};
std::atomic<uint32_t> g_last_hook_address{0};

std::mutex g_last_args_mutex;
fm2::native_renderer::GuestArgs g_last_args;

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
  (void)window;
  const Mode mode = GetMode();
  if (mode == Mode::kXenos) {
    return true;
  }

  g_initialized.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume native renderer skeleton initialized mode={}",
              GetModeName(mode));
  return true;
}

void Shutdown() {
  const bool was_initialized =
      g_initialized.exchange(false, std::memory_order_relaxed);
  g_plume_available.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_swapchain_ready.store(false, std::memory_order_relaxed);
  if (was_initialized) {
    REXLOG_INFO("FM2 Plume native renderer skeleton shut down");
  }
}

bool RenderClearOnce() {
  REXLOG_WARN(
      "FM2 Plume clear requested before Plume device support is implemented");
  return false;
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
