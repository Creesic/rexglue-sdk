#pragma once

#include "native_renderer/fm2_direct_draw_decode.h"

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
  uint64_t debug_replay_attempts = 0;
  uint64_t debug_replay_submitted = 0;
  uint64_t debug_replay_failed = 0;
  uint32_t last_hook_address = 0;
  GuestArgs last_args;
};

struct DirectDrawReplaySourceBytes {
  const uint8_t* stream0 = nullptr;
  const uint8_t* stream1 = nullptr;
  const uint8_t* index = nullptr;
};

Mode GetMode();
const char* GetModeName(Mode mode);
bool WantsReXGraphics();
bool WantsDirectDebugReplay();
bool WantsCompareWindow();

bool Initialize(rex::ui::Window* window);
void Shutdown();
bool RenderClearOnce();

void RecordBuildObjectPassEntry(const GuestArgs& args);
void RecordDirectIndexedDrawEntry(const GuestArgs& args);
bool SubmitDirectDebugReplay(const DirectDrawDebugReplayPlan& plan,
                             const DirectDrawReplaySourceBytes& sources);
struct DirectDrawReplaySubmission {
  DirectDrawDebugReplayPlan plan;
  DirectDrawReplaySourceBytes sources;
};
bool SubmitDirectDebugReplayBatch(const DirectDrawReplaySubmission* submissions,
                                  uint32_t submission_count);
Stats GetStatsSnapshot();

}  // namespace fm2::native_renderer
