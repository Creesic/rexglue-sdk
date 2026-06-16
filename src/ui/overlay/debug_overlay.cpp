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
#include <rex/cvar.h>
#include <rex/version.h>
#include <rex/perf/counter.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
#include <cinttypes>
#endif

namespace rex::ui {
namespace {

float ExpSmoothingAlpha(double dt_seconds, double time_constant_seconds) {
  if (time_constant_seconds <= 0.0) {
    return 1.0f;
  }
  const double dt = std::max(0.0, dt_seconds);
  return static_cast<float>(1.0 - std::exp(-dt / time_constant_seconds));
}

const char* LoadTraceStatusText() {
  if (!rex::cvar::Query<bool>("fm2_load_trace")) {
    return "LT OFF";
  }
  switch (rex::cvar::Query<uint32_t>("fm2_load_trace_overlay_state")) {
    case 2:
      return "LT REC";
    case 1:
    default:
      return "LT ARMED";
  }
}

ImVec4 LoadTraceStatusColor() {
  if (!rex::cvar::Query<bool>("fm2_load_trace")) {
    return ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
  }
  return rex::cvar::Query<uint32_t>("fm2_load_trace_overlay_state") == 2u
             ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
             : ImVec4(1.0f, 0.85f, 0.35f, 1.0f);
}

}  // namespace

DebugOverlayDialog::DebugOverlayDialog(ImGuiDrawer* imgui_drawer, FrameStatsProvider stats_provider,
                                       bool compact_only)
    : ImGuiDialog(imgui_drawer),
      stats_provider_(std::move(stats_provider)),
      compact_only_(compact_only) {}

DebugOverlayDialog::~DebugOverlayDialog() {}

void DebugOverlayDialog::OnDraw(ImGuiIO& io) {
  FrameStats stats{};
  bool has_stats = false;
  if (stats_provider_) {
    stats = stats_provider_();
    has_stats = stats.frame_count > 0 || stats.frame_time_ms > 0.0 || stats.fps > 0.0;
  }
  if (!has_stats) {
    const int64_t ft_us = rex::perf::GetSnapshotCounter(rex::perf::CounterId::kFrameTimeUs);
    if (ft_us > 0) {
      stats.frame_time_ms = static_cast<double>(ft_us) / 1000.0;
      stats.fps = 1000000.0 / static_cast<double>(ft_us);
      stats.frame_count = 1;
      has_stats = true;
    }
  }
  if (has_stats) {
    const double now = ImGui::GetTime();
    const double dt_seconds = (last_stats_time_ > 0.0) ? (now - last_stats_time_) : 0.0;
    const float alpha = ExpSmoothingAlpha(dt_seconds, 0.35);
    if (!smoothed_stats_initialized_) {
      smoothed_fps_ = stats.fps;
      smoothed_frame_time_ms_ = stats.frame_time_ms;
      smoothed_stats_initialized_ = true;
    } else {
      smoothed_fps_ += (stats.fps - smoothed_fps_) * alpha;
      smoothed_frame_time_ms_ += (stats.frame_time_ms - smoothed_frame_time_ms_) * alpha;
    }
    last_stats_time_ = now;
  } else {
    smoothed_stats_initialized_ = false;
  }

  if (compact_only_) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("FPS##overlay_compact", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings)) {
      if (has_stats) {
        ImGui::Text("FPS: %.1f (%.2f ms)", smoothed_fps_, smoothed_frame_time_ms_);
      } else {
        ImGui::TextUnformatted("FPS: n/a");
      }
      ImGui::TextColored(LoadTraceStatusColor(), "%s", LoadTraceStatusText());
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
    if (has_stats) {
      ImGui::Text("Guest: %.1f FPS (%.2f ms)", smoothed_fps_, smoothed_frame_time_ms_);
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
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
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
