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
  // kPlumeNative uses the same code path as kPlumeClear (disable ReX Xenos
  // graphics, init Plume via shared Video::Init, enable mirror hooks) but
  // signals production native-rendering intent. See
  // docs/FM2-native-renderer-gap-analysis.md for the mode-transition plan.
  kPlumeNative,
};

enum class DebugReplayFillMode : uint8_t {
  kSolid,
  kWireframe,
};

inline constexpr uint32_t kFM2RenderBuildObjectPassCommandBufferAddress =
    0x82531370u;
// One-shot build guard at direct_render_ctx+0x48 — not manifest-hooked; use
// kFM2RenderInstanceHybridDrawPathAddress for per-frame interception.
inline constexpr uint32_t kFM2RenderBuildDirectIndexedDrawBuffersAddress =
    0x825380B8u;
inline constexpr uint32_t kFM2RenderInstanceHybridDrawPathAddress =
    0x82539650u;
inline constexpr uint32_t kFM2RenderEmitPassDrawWorkAddress = 0x8250F7C0u;
inline constexpr uint32_t kFM2RenderContextSetViewportModeAndApplyAddress =
    0x823715B0u;
inline constexpr uint32_t kFM2RenderContextSetTextureFetchBitsLowAddress =
    0x8236EA60u;
inline constexpr uint32_t kFM2RenderContextSetTextureFetchBitsMidAddress =
    0x8236EA90u;
inline constexpr uint32_t kFM2RenderSetClearColorByteAndDirtyFlagAddress =
    0x8236EF20u;
inline constexpr uint32_t kFM2RenderSetClearFlagsAndDirtyBitAddress =
    0x8236EF88u;
inline constexpr uint32_t kFM2D3DTryPresentAndUpdateStatusAddress =
    0x824F83D8u;
inline constexpr uint32_t kFM2PlumeLiveDirectDrawHookAddress =
    kFM2RenderInstanceHybridDrawPathAddress;

constexpr bool IsFM2PlumeLiveDirectDrawHookAddress(uint32_t address) {
  return address == kFM2PlumeLiveDirectDrawHookAddress;
}

struct NativeWindowRect {
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;
};

constexpr bool IsValidNativeWindowRect(const NativeWindowRect& rect) {
  return rect.width > 0 && rect.height > 0;
}

constexpr NativeWindowRect PlaceCompanionWindowBesideHost(
    const NativeWindowRect& host, int32_t companion_width,
    int32_t companion_height, int32_t gap) {
  if (!IsValidNativeWindowRect(host)) {
    return {.x = gap,
            .y = gap,
            .width = companion_width,
            .height = companion_height};
  }
  return {.x = host.x + host.width + gap,
          .y = host.y,
          .width = companion_width,
          .height = companion_height};
}

constexpr DebugReplayFillMode DebugReplayFillModeForWireframe(
    bool wireframe_enabled) {
  return wireframe_enabled ? DebugReplayFillMode::kWireframe
                           : DebugReplayFillMode::kSolid;
}

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
  uint64_t instance_hybrid_draw_entries = 0;
  uint64_t present_entries = 0;
  uint64_t debug_replay_attempts = 0;
  uint64_t debug_replay_submitted = 0;
  uint64_t debug_replay_failed = 0;
  uint64_t native_direct_draw_attempts = 0;
  uint64_t native_direct_draw_submitted = 0;
  uint64_t native_direct_draw_failed = 0;
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
bool WantsNativeDirectDraw();
bool WantsNativeDirectDrawLiveBatch();
bool WantsCompareWindow();

bool Initialize(rex::ui::Window* window);
void Shutdown();
bool RenderClearOnce();

void RecordBuildObjectPassEntry(const GuestArgs& args);
void RecordDirectIndexedDrawEntry(const GuestArgs& args);
void RecordInstanceHybridDrawEntry(const GuestArgs& args);
void RecordPresentEntry(uint32_t present_chain_object);
bool FlushNativeDirectDrawOnPresent();
bool FlushDebugReplayOnPresent();
bool SubmitDirectDebugReplay(const DirectDrawDebugReplayPlan& plan,
                             const DirectDrawReplaySourceBytes& sources);
bool QueueDirectDebugReplay(const DirectDrawDebugReplayPlan& plan,
                            const DirectDrawReplaySourceBytes& sources);
bool SubmitNativeDirectDraw(const DirectDrawDebugReplayPlan& plan,
                            const DirectDrawReplaySourceBytes& sources);
struct DirectDrawReplaySubmission {
  DirectDrawDebugReplayPlan plan;
  DirectDrawReplaySourceBytes sources;
};
bool SubmitDirectDebugReplayBatch(const DirectDrawReplaySubmission* submissions,
                                  uint32_t submission_count);
bool SubmitDirectDebugReplayBatchForReplayWindow(
    const DirectDrawReplaySubmission* submissions, uint32_t submission_count);
Stats GetStatsSnapshot();

}  // namespace fm2::native_renderer
