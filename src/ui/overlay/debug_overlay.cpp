/**
 * @file        ui/overlay/debug_overlay.cpp
 *
 * @brief       Debug overlay implementation. See debug_overlay.h for details.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#include <rex/ui/overlay/debug_overlay.h>
#include <rex/version.h>
#include <imgui.h>
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
#include <rex/perf/counter.h>
#include <cinttypes>
#endif

namespace rex::ui {

namespace {

struct DisplayFrameStats {
  FrameStats stats{};
  bool has_stats = false;
  bool guest_source = false;
};

DisplayFrameStats ResolveFrameStats(const DebugOverlayDialog::FrameStatsProvider& stats_provider,
                                    ImGuiIO& io) {
  DisplayFrameStats result{};
  if (stats_provider) {
    result.stats = stats_provider();
    result.has_stats = result.stats.frame_count > 0 || result.stats.frame_time_ms > 0.0 ||
                       result.stats.fps > 0.0;
    result.guest_source = result.has_stats;
  }

#ifdef REXGLUE_ENABLE_PERF_COUNTERS
  if (!result.has_stats) {
    const int64_t ft_us = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kFrameTimeUs);
    if (ft_us > 0) {
      result.stats.frame_time_ms = static_cast<double>(ft_us) / 1000.0;
      result.stats.fps = 1000000.0 / static_cast<double>(ft_us);
      result.stats.frame_count = 1;
      result.has_stats = true;
      result.guest_source = true;
    }
  }
#endif

  if (!result.has_stats && io.Framerate > 0.0f) {
    result.stats.fps = static_cast<double>(io.Framerate);
    result.stats.frame_time_ms = 1000.0 / result.stats.fps;
    result.stats.frame_count = 1;
    result.has_stats = true;
  }

  return result;
}

}  // namespace

DebugOverlayDialog::DebugOverlayDialog(ImGuiDrawer* imgui_drawer, FrameStatsProvider stats_provider,
                                       bool compact_only)
    : ImGuiDialog(imgui_drawer),
      stats_provider_(std::move(stats_provider)),
      compact_only_(compact_only) {}

DebugOverlayDialog::~DebugOverlayDialog() {}

void DebugOverlayDialog::OnDraw(ImGuiIO& io) {
  const DisplayFrameStats display_stats = ResolveFrameStats(stats_provider_, io);

  if (compact_only_) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("FPS##overlay_compact", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoSavedSettings)) {
      if (display_stats.has_stats) {
        ImGui::Text("FPS: %.1f (%.2f ms)", display_stats.stats.fps,
                    display_stats.stats.frame_time_ms);
      } else {
        ImGui::TextUnformatted("FPS: n/a");
      }
    }
    ImGui::End();
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
  ImGui::SetNextWindowSize(ImVec2(280, 280), ImGuiCond_FirstUseEver);
#else
  ImGui::SetNextWindowSize(ImVec2(220, 60), ImGuiCond_FirstUseEver);
#endif
  ImGui::SetNextWindowBgAlpha(0.5f);
  if (ImGui::Begin("Debug##overlay", nullptr, ImGuiWindowFlags_NoCollapse)) {
    if (display_stats.has_stats) {
      ImGui::Text("%s: %.1f FPS (%.2f ms)",
                  display_stats.guest_source ? "Guest" : "Host", display_stats.stats.fps,
                  display_stats.stats.frame_time_ms);
    }
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
    ImGui::Separator();

    // Frame time graph
    auto ft_us = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kFrameTimeUs);
    float ft_ms = static_cast<float>(ft_us) / 1000.0f;
    frame_time_history_[frame_history_idx_] = ft_ms;
    frame_history_idx_ = (frame_history_idx_ + 1) % kFrameHistorySize;
    ImGui::PlotLines("##ft", frame_time_history_.data(), kFrameHistorySize,
                     static_cast<int>(frame_history_idx_), "Frame (ms)", 0.0f, 50.0f,
                     ImVec2(200, 40));

    // GPU
    ImGui::Text("Draw: %" PRId64 "  Stalls: %" PRId64 "  Verts: %" PRId64,
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kDrawCalls),
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kCommandBufferStalls),
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kVerticesProcessed));

    // Audio
    ImGui::Text("XMA: %" PRId64 "  Lat: %.1fms  BufQ: %" PRId64,
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kXmaFramesDecoded),
                static_cast<float>(
                    rex::perf::GetSnapshotCounter(rex::perf::CounterId::kAudioFrameLatencyUs)) /
                    1000.0f,
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kBufferQueueDepth));

    // Dispatch
    ImGui::Text("Dispatch: %" PRId64 "  IRQ: %" PRId64,
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kFunctionsDispatched),
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kInterruptDispatches));

    // Threading
    ImGui::Text("Threads: %" PRId64 "  APC: %" PRId64 "  Contention: %" PRId64,
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kActiveThreads),
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kApcQueueDepth),
                rex::perf::GetSnapshotCounter(rex::perf::CounterId::kCriticalRegionContentions));

    // Caches
    auto tex_h = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kTextureCacheHits);
    auto tex_m = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kTextureCacheMisses);
    auto pip_h = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kPipelineCacheHits);
    auto pip_m = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kPipelineCacheMisses);
    ImGui::Text("TexCache: %" PRId64 "/%" PRId64 "  PipeCache: %" PRId64 "/%" PRId64, tex_h,
                tex_h + tex_m, pip_h, pip_h + pip_m);
#endif
  }
  ImGui::End();

  // Build stamp watermark -- centered near bottom of screen
  auto text_size = ImGui::CalcTextSize(REXGLUE_BUILD_STAMP);
  float padding = ImGui::GetStyle().WindowPadding.x * 2.0f;
  float bottom_offset = io.DisplaySize.y * 0.03f;
  ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - text_size.x - padding) * 0.5f,
                                 io.DisplaySize.y - text_size.y - bottom_offset));
  ImGui::SetNextWindowSize(ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_Text, imgui_drawer()->style().debug.muted_text);
  if (ImGui::Begin("##watermark", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                       ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted(REXGLUE_BUILD_STAMP);
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

}  // namespace rex::ui
