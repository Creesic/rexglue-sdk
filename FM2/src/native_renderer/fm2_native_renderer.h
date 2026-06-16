#pragma once

#include <cstdint>

namespace rex::ui {
class Window;
}

namespace fm2::native_renderer {

enum class Mode : uint8_t {
  kXenos,
  kShadow,
  kPlumeClear,
};

struct GuestArgs {
  uint32_t r3 = 0;
  uint32_t r4 = 0;
  uint32_t r5 = 0;
  uint32_t r6 = 0;
  uint32_t r7 = 0;
  uint32_t r8 = 0;
  uint32_t r9 = 0;
  uint32_t r10 = 0;
};

struct Stats {
  bool initialized = false;
  bool plume_available = false;
  bool plume_device_ready = false;
  bool swapchain_ready = false;
  uint64_t build_object_pass_entries = 0;
  uint64_t direct_indexed_draw_entries = 0;
  uint32_t last_hook_address = 0;
  GuestArgs last_args;
};

Mode GetMode();
const char* GetModeName(Mode mode);
bool WantsReXGraphics();

bool Initialize(rex::ui::Window* window);
void Shutdown();
bool RenderClearOnce();

void RecordBuildObjectPassEntry(const GuestArgs& args);
void RecordDirectIndexedDrawEntry(const GuestArgs& args);
Stats GetStatsSnapshot();

}  // namespace fm2::native_renderer
