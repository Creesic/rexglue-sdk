/**
 * @file        ui/overlay/audio_voices_overlay.cpp
 *
 * @brief       Audio voices overlay implementation. See audio_voices_overlay.h for details.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#include <rex/ui/overlay/audio_voices_overlay.h>
#include <imgui.h>
#include <fmt/format.h>
#include <algorithm>
#include <cstdio>
#include <cfloat>
#include <cmath>

namespace rex::ui {

namespace {

constexpr float kMeterFloorDbfs = -60.0f;
constexpr float kMeterReleaseDbPerSecond = 18.0f;
constexpr float kVoiceVolumeMin = 0.0f;
constexpr float kVoiceVolumeMax = 1.0f;
constexpr uint32_t kXmaPacketBytes = 2048;
constexpr uint32_t kXmaPcmBitsPerSample = 16;
constexpr uint32_t kXmaSamplesPerFramePerChannel = 512;

float LinearToDbfs(float linear) {
  const float safe_linear = std::max(linear, 1.0e-6f);
  return 20.0f * std::log10(safe_linear);
}

float DbfsToNormalized(float dbfs) {
  return std::clamp((dbfs - kMeterFloorDbfs) / -kMeterFloorDbfs, 0.0f, 1.0f);
}

float ExpSmoothingAlpha(double dt_seconds, double time_constant_seconds) {
  if (time_constant_seconds <= 0.0) {
    return 1.0f;
  }
  const double dt = std::max(0.0, dt_seconds);
  return static_cast<float>(1.0 - std::exp(-dt / time_constant_seconds));
}

uint32_t SampleRateIdToHz(uint8_t sample_rate_id) {
  static constexpr uint32_t kSampleRateTable[4] = {24000, 32000, 44100, 48000};
  if (sample_rate_id >= 4) {
    return 0;
  }
  return kSampleRateTable[sample_rate_id];
}

bool ContainsContext(const std::vector<uint32_t>& contexts, uint32_t context_id) {
  return std::find(contexts.begin(), contexts.end(), context_id) != contexts.end();
}

void EraseContext(std::vector<uint32_t>& contexts, uint32_t context_id) {
  contexts.erase(std::remove(contexts.begin(), contexts.end(), context_id), contexts.end());
}

uint64_t HashCombine64(uint64_t hash, uint64_t value) {
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

uint64_t BuildContextSignature(const AudioXmaContextInfo& ctx) {
  if (!ctx.allocated) {
    return 0;
  }
  if (ctx.output_buffer_ptr == 0 && ctx.input_buffer_0_ptr == 0 && ctx.input_buffer_1_ptr == 0) {
    return 0;
  }
  uint64_t hash = 0xcbf29ce484222325ULL;
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.output_buffer_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.input_buffer_0_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.input_buffer_1_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.sample_rate_id));
  hash = HashCombine64(hash, ctx.stereo ? 1ULL : 0ULL);
  return hash;
}

float GetContextPeakForMode(const AudioXmaContextInfo& ctx, bool use_audible_meter) {
  return use_audible_meter ? ctx.audible_peak_level : ctx.peak_level;
}

float GetContextRmsForMode(const AudioXmaContextInfo& ctx, bool use_audible_meter) {
  return use_audible_meter ? ctx.audible_rms_level : ctx.rms_level;
}

bool IsPlayingContext(const AudioXmaContextInfo& ctx, bool use_audible_meter) {
  if (!ctx.allocated) {
    return false;
  }
  const float rms = GetContextRmsForMode(ctx, use_audible_meter);
  const float peak = GetContextPeakForMode(ctx, use_audible_meter);
  return rms > 0.0005f || peak > 0.001f || ctx.enabled || ctx.input0_valid || ctx.input1_valid ||
         ctx.output_valid;
}

void DrawLineGraphNoTooltip(const char* id, const std::vector<float>& values, float scale_min,
                            float scale_max, ImVec2 graph_size, ImVec2* out_min = nullptr,
                            ImVec2* out_max = nullptr) {
  ImGui::InvisibleButton(id, graph_size);
  const ImVec2 plot_min = ImGui::GetItemRectMin();
  const ImVec2 plot_max = ImGui::GetItemRectMax();
  if (out_min) {
    *out_min = plot_min;
  }
  if (out_max) {
    *out_max = plot_max;
  }

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImU32 bg_col = ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 0.55f));
  const ImU32 border_col = ImGui::GetColorU32(ImVec4(0.55f, 0.55f, 0.55f, 0.45f));
  const ImU32 line_col = ImGui::GetColorU32(ImGuiCol_PlotLines);
  draw_list->AddRectFilled(plot_min, plot_max, bg_col, 0.0f);
  draw_list->AddRect(plot_min, plot_max, border_col, 0.0f);

  if (values.size() < 2 || !(scale_max > scale_min)) {
    return;
  }

  const float inner_left = plot_min.x + 1.0f;
  const float inner_right = plot_max.x - 1.0f;
  const float inner_top = plot_min.y + 1.0f;
  const float inner_bottom = plot_max.y - 1.0f;
  const float inner_width = std::max(1.0f, inner_right - inner_left);
  const float inner_height = std::max(1.0f, inner_bottom - inner_top);
  const size_t point_count = values.size();
  for (size_t i = 1; i < point_count; ++i) {
    const float t0 = static_cast<float>(i - 1) / static_cast<float>(point_count - 1);
    const float t1 = static_cast<float>(i) / static_cast<float>(point_count - 1);
    const float v0 = std::clamp((values[i - 1] - scale_min) / (scale_max - scale_min), 0.0f, 1.0f);
    const float v1 = std::clamp((values[i] - scale_min) / (scale_max - scale_min), 0.0f, 1.0f);
    const ImVec2 p0(inner_left + t0 * inner_width, inner_bottom - v0 * inner_height);
    const ImVec2 p1(inner_left + t1 * inner_width, inner_bottom - v1 * inner_height);
    draw_list->AddLine(p0, p1, line_col, 1.0f);
  }
}

}  // namespace

AudioVoicesDialog::AudioVoicesDialog(ImGuiDrawer* imgui_drawer, SnapshotProvider snapshot_provider,
                                     SetContextMuteCallback set_context_mute,
                                     SetContextVolumeCallback set_context_volume,
                                     LoadLayoutStateCallback load_layout_state,
                                     SaveLayoutStateCallback save_layout_state)
    : ImGuiDialog(imgui_drawer),
      snapshot_provider_(std::move(snapshot_provider)),
      set_context_mute_(std::move(set_context_mute)),
      set_context_volume_(std::move(set_context_volume)),
      load_layout_state_(std::move(load_layout_state)),
      save_layout_state_(std::move(save_layout_state)) {}

AudioVoicesDialog::~AudioVoicesDialog() {
  SaveLayoutState(true);
}

void AudioVoicesDialog::OnClose() {
  ClearHoverSoloOverride();
  SaveLayoutState(true);
}

void AudioVoicesDialog::ClearHoverSoloOverride() {
  if (!hover_solo_active_ || !set_context_mute_) {
    hover_solo_active_ = false;
    hover_solo_context_.reset();
    hover_solo_saved_mute_states_.clear();
    return;
  }

  for (const auto& [context_id, was_muted] : hover_solo_saved_mute_states_) {
    set_context_mute_(context_id, was_muted);
  }
  hover_solo_saved_mute_states_.clear();
  hover_solo_context_.reset();
  hover_solo_active_ = false;
}

void AudioVoicesDialog::ApplyPersistedMuteStateToSnapshot(const AudioVoicesSnapshot& snapshot) {
  if (!set_context_mute_) {
    return;
  }
  for (const auto& ctx : snapshot.xma_contexts) {
    const uint64_t signature = BuildContextSignature(ctx);
    const bool should_be_muted =
        signature ? (persisted_muted_signatures_.find(signature) != persisted_muted_signatures_.end())
                  : (persisted_muted_contexts_.find(ctx.index) != persisted_muted_contexts_.end());
    if (ctx.muted != should_be_muted) {
      set_context_mute_(ctx.index, should_be_muted);
    }
  }
}

void AudioVoicesDialog::UpdateHoverSoloOverride(const AudioVoicesSnapshot& snapshot,
                                                const std::optional<uint32_t>& hovered_context) {
  if (!hover_solo_enabled_ || !set_context_mute_ || !hovered_context ||
      ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    ClearHoverSoloOverride();
    ApplyPersistedMuteStateToSnapshot(snapshot);
    return;
  }

  if (hover_solo_active_ && hover_solo_context_ && *hover_solo_context_ == *hovered_context) {
    // Keep solo behavior strict even as contexts appear/disappear while hovering.
    for (const auto& ctx : snapshot.xma_contexts) {
      if (!ctx.allocated && !ctx.enabled && !ctx.input0_valid && !ctx.input1_valid &&
          !ctx.output_valid) {
        continue;
      }
      const bool should_mute = ctx.index != *hovered_context;
      if (ctx.muted != should_mute) {
        set_context_mute_(ctx.index, should_mute);
      }
    }
    return;
  }

  ClearHoverSoloOverride();
  hover_solo_saved_mute_states_.reserve(snapshot.xma_contexts.size());
  for (const auto& ctx : snapshot.xma_contexts) {
    if (!ctx.allocated && !ctx.enabled && !ctx.input0_valid && !ctx.input1_valid && !ctx.output_valid) {
      continue;
    }
    hover_solo_saved_mute_states_[ctx.index] = ctx.muted;
  }
  if (hover_solo_saved_mute_states_.find(*hovered_context) == hover_solo_saved_mute_states_.end()) {
    hover_solo_saved_mute_states_[*hovered_context] = false;
  }

  for (const auto& [context_id, _] : hover_solo_saved_mute_states_) {
    const bool should_mute = context_id != *hovered_context;
    set_context_mute_(context_id, should_mute);
  }
  hover_solo_context_ = hovered_context;
  hover_solo_active_ = true;
}

void AudioVoicesDialog::LoadLayoutStateOnce() {
  if (layout_state_loaded_) {
    return;
  }
  layout_state_loaded_ = true;
  if (!load_layout_state_) {
    return;
  }
  auto loaded = load_layout_state_();
  if (!loaded) {
    return;
  }

  if (loaded->has_window_size) {
    pending_window_width_ = std::max(loaded->window_width, 320.0f);
    pending_window_height_ = std::max(loaded->window_height, 240.0f);
    last_saved_window_width_ = pending_window_width_;
    last_saved_window_height_ = pending_window_height_;
  }

  normalize_volume_panel_ = loaded->normalize_volume_panel;
  // Persisted controls are always enabled; ignore legacy toggle state.
  ignore_persisted_controls_ = false;
  show_playing_contexts_only_ = loaded->show_playing_contexts_only;
  meter_use_audible_ = loaded->meter_use_audible;
  persisted_muted_contexts_ = std::move(loaded->muted_contexts);
  persisted_muted_signatures_ = std::move(loaded->muted_signatures);
  tracked_volume_contexts_ = std::move(loaded->tracked_volume_contexts);
  context_names_ = std::move(loaded->context_names);
  persisted_context_volumes_ = std::move(loaded->context_volumes);
  for (uint32_t context_id : tracked_volume_contexts_) {
    known_contexts_.insert(context_id);
  }
  for (const auto& [context_id, _] : context_names_) {
    known_contexts_.insert(context_id);
  }
  for (const auto& [context_id, _] : persisted_context_volumes_) {
    known_contexts_.insert(context_id);
  }
  for (uint32_t context_id : persisted_muted_contexts_) {
    known_contexts_.insert(context_id);
  }
  pending_apply_mute_layout_ = true;
  layout_dirty_ = false;
}

void AudioVoicesDialog::ApplyMutedLayoutState(const AudioVoicesSnapshot& snapshot) {
  if (!pending_apply_mute_layout_ || !snapshot.valid) {
    return;
  }
  if (!set_context_mute_) {
    return;
  }
  for (uint32_t i = 0; i < snapshot.xma_contexts.size(); ++i) {
    const auto& ctx = snapshot.xma_contexts[i];
    const uint64_t signature = BuildContextSignature(ctx);
    if (signature && persisted_muted_contexts_.find(i) != persisted_muted_contexts_.end()) {
      persisted_muted_signatures_.insert(signature);
      persisted_muted_contexts_.erase(i);
      layout_dirty_ = true;
    }
    bool should_be_muted = false;
    if (signature) {
      should_be_muted = persisted_muted_signatures_.find(signature) != persisted_muted_signatures_.end();
    } else {
      should_be_muted = persisted_muted_contexts_.find(i) != persisted_muted_contexts_.end();
    }
    if (snapshot.xma_contexts[i].muted != should_be_muted) {
      set_context_mute_(i, should_be_muted);
    }
    if (set_context_volume_) {
      const auto vol_it = persisted_context_volumes_.find(i);
      if (vol_it != persisted_context_volumes_.end()) {
        const float clamped_volume = std::clamp(vol_it->second, kVoiceVolumeMin, kVoiceVolumeMax);
        if (std::fabs(snapshot.xma_contexts[i].volume - clamped_volume) > 0.001f) {
          set_context_volume_(i, clamped_volume);
        }
      }
    }
  }
  pending_apply_mute_layout_ = false;
}

AudioVoicesLayoutState AudioVoicesDialog::BuildLayoutState() const {
  AudioVoicesLayoutState state;
  state.has_window_size = true;
  state.window_width = pending_window_width_;
  state.window_height = pending_window_height_;
  state.normalize_volume_panel = normalize_volume_panel_;
  state.ignore_persisted_controls = false;
  state.show_playing_contexts_only = show_playing_contexts_only_;
  state.meter_use_audible = meter_use_audible_;
  state.muted_contexts = persisted_muted_contexts_;
  state.muted_signatures = persisted_muted_signatures_;
  state.tracked_volume_contexts = tracked_volume_contexts_;
  state.context_names = context_names_;
  state.context_volumes = persisted_context_volumes_;
  return state;
}

void AudioVoicesDialog::SaveLayoutState(bool force) {
  if (!save_layout_state_ || (!layout_dirty_ && !force)) {
    return;
  }
  const double now = ImGui::GetTime();
  if (!force && (now - last_layout_save_time_) < 0.35) {
    return;
  }
  save_layout_state_(BuildLayoutState());
  layout_dirty_ = false;
  last_layout_save_time_ = now;
  last_saved_window_width_ = pending_window_width_;
  last_saved_window_height_ = pending_window_height_;
}

void AudioVoicesDialog::OnDraw([[maybe_unused]] ImGuiIO& io) {
  LoadLayoutStateOnce();

  AudioVoicesSnapshot snapshot;
  if (snapshot_provider_) {
    snapshot = snapshot_provider_();
  }
  ApplyMutedLayoutState(snapshot);

  ImGui::SetNextWindowPos(ImVec2(20, 80), ImGuiCond_FirstUseEver);
  const ImGuiStyle& style = ImGui::GetStyle();
  constexpr float kGridColumns = 16.0f;
  constexpr float kGridCellWidth = 30.0f;
  constexpr float kGridItemSpacing = 4.0f;
  constexpr float kRightPanelMinWidth = 320.0f;
  constexpr float kMinWindowHeight = 320.0f;
  const float grid_inner_width =
      kGridColumns * kGridCellWidth + (kGridColumns - 1.0f) * kGridItemSpacing;
  const float grid_width_required =
      grid_inner_width + style.WindowPadding.x * 2.0f + style.ChildBorderSize * 2.0f;
  const float window_min_width = grid_width_required + style.ItemSpacing.x + kRightPanelMinWidth +
                                 style.WindowPadding.x * 2.0f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(window_min_width, kMinWindowHeight),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGui::SetNextWindowSize(ImVec2(pending_window_width_, pending_window_height_),
                           ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.80f);
  if (!ImGui::Begin("Audio Voices##overlay", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  if (!snapshot.valid) {
    ClearHoverSoloOverride();
    ImGui::TextUnformatted("Audio voice debug data unavailable.");
    ImGui::End();
    return;
  }

  size_t active_xma_contexts = 0;
  for (const auto& ctx : snapshot.xma_contexts) {
    if (ctx.allocated && (ctx.input0_valid || ctx.input1_valid || ctx.output_valid || ctx.enabled)) {
      active_xma_contexts++;
    }
  }

  ImGui::Text("Audio Paused: %s", snapshot.paused ? "yes" : "no");
  ImGui::SameLine();
  ImGui::Text("Queued Frames: %u", snapshot.queued_frames);
  ImGui::SameLine();
  ImGui::Text("Render Voices: %zu", snapshot.voices.size());
  ImGui::SameLine();
  ImGui::Text("Active XMA Contexts: %zu", active_xma_contexts);

  uint64_t submitted_frames_total = 0;
  for (const auto& voice : snapshot.voices) {
    submitted_frames_total += voice.submitted_frames;
  }
  const uint64_t rendered_callbacks_total = snapshot.render_callbacks_total;
  const uint64_t queue_depth_estimated =
      submitted_frames_total > rendered_callbacks_total
          ? (submitted_frames_total - rendered_callbacks_total)
          : 0;
  const uint64_t queue_capacity_estimated =
      std::max<uint64_t>(1, static_cast<uint64_t>(std::max<size_t>(1, snapshot.voices.size())) *
                                static_cast<uint64_t>(std::max<uint32_t>(1, snapshot.queued_frames)));
  const float utilization_percent = std::clamp(
      (100.0f * static_cast<float>(queue_depth_estimated) /
       static_cast<float>(queue_capacity_estimated)),
      0.0f, 100.0f);

  const double now_time = ImGui::GetTime();
  auto push_history_value = [](std::vector<float>& history, float value, size_t limit) {
    history.push_back(value);
    if (history.size() > limit) {
      history.erase(history.begin(), history.begin() + (history.size() - limit));
    }
  };
  if (last_render_callbacks_total_ == 0) {
    last_render_callbacks_total_ = rendered_callbacks_total;
    last_render_callback_time_ = now_time;
  } else if (rendered_callbacks_total > last_render_callbacks_total_) {
    const uint64_t callback_delta = rendered_callbacks_total - last_render_callbacks_total_;
    const double time_delta = now_time - last_render_callback_time_;
    if (time_delta > 0.0) {
      const float interval_ms =
          static_cast<float>((time_delta * 1000.0) / static_cast<double>(callback_delta));
      push_history_value(callback_interval_history_ms_, interval_ms, 240);
      if (render_interval_ema_ms_ <= 0.0f) {
        render_interval_ema_ms_ = interval_ms;
      } else {
        constexpr float kIntervalEmaAlpha = 0.12f;
        render_interval_ema_ms_ += (interval_ms - render_interval_ema_ms_) * kIntervalEmaAlpha;
      }
      render_deviation_ms_ = std::fabs(interval_ms - render_interval_ema_ms_);
    }
    last_render_callbacks_total_ = rendered_callbacks_total;
    last_render_callback_time_ = now_time;
  }
  if (last_metrics_sample_time_ == 0.0 || (now_time - last_metrics_sample_time_) >= (1.0 / 30.0)) {
    push_history_value(queue_depth_history_, static_cast<float>(queue_depth_estimated), 240);
    last_metrics_sample_time_ = now_time;
  }
  const float pacing_interval_ms = callback_interval_history_ms_.empty()
                                       ? render_interval_ema_ms_
                                       : callback_interval_history_ms_.back();
  const float latency_ms_estimated =
      (render_interval_ema_ms_ > 0.0f)
          ? (static_cast<float>(queue_depth_estimated) * render_interval_ema_ms_)
          : 0.0f;
  const double metrics_dt_seconds =
      (last_metrics_smoothing_time_ > 0.0) ? (now_time - last_metrics_smoothing_time_) : 0.0;
  const float metrics_alpha = ExpSmoothingAlpha(metrics_dt_seconds, 0.45);
  if (!smoothed_metrics_initialized_) {
    smoothed_utilization_percent_ = utilization_percent;
    smoothed_deviation_ms_ = render_deviation_ms_;
    smoothed_latency_ms_ = latency_ms_estimated;
    smoothed_pacing_interval_ms_ = pacing_interval_ms;
    smoothed_vp_total_us_ = static_cast<float>(snapshot.vp.total_worker_time_us);
    smoothed_vp_sweeps_per_second_ = static_cast<float>(snapshot.vp.sweeps_per_second);
    smoothed_vp_decode_iters_per_second_ =
        static_cast<float>(snapshot.vp.decode_iterations_per_second);
    smoothed_gp_cycles_ = static_cast<float>(snapshot.gp.cycles);
    smoothed_ep_cycles_ = static_cast<float>(snapshot.ep.cycles);
    smoothed_metrics_initialized_ = true;
  } else {
    smoothed_utilization_percent_ +=
        (utilization_percent - smoothed_utilization_percent_) * metrics_alpha;
    smoothed_deviation_ms_ += (render_deviation_ms_ - smoothed_deviation_ms_) * metrics_alpha;
    smoothed_latency_ms_ += (latency_ms_estimated - smoothed_latency_ms_) * metrics_alpha;
    smoothed_pacing_interval_ms_ +=
        (pacing_interval_ms - smoothed_pacing_interval_ms_) * metrics_alpha;
    smoothed_vp_total_us_ +=
        (static_cast<float>(snapshot.vp.total_worker_time_us) - smoothed_vp_total_us_) *
        metrics_alpha;
    smoothed_vp_sweeps_per_second_ +=
        (static_cast<float>(snapshot.vp.sweeps_per_second) - smoothed_vp_sweeps_per_second_) *
        metrics_alpha;
    smoothed_vp_decode_iters_per_second_ +=
        (static_cast<float>(snapshot.vp.decode_iterations_per_second) -
         smoothed_vp_decode_iters_per_second_) *
        metrics_alpha;
    smoothed_gp_cycles_ += (static_cast<float>(snapshot.gp.cycles) - smoothed_gp_cycles_) *
                           metrics_alpha;
    smoothed_ep_cycles_ += (static_cast<float>(snapshot.ep.cycles) - smoothed_ep_cycles_) *
                           metrics_alpha;
  }
  last_metrics_smoothing_time_ = now_time;

  ImGui::Text(
      "Utilization: %.1f%%   Deviation: %.3f ms   Latency: %.2f ms   Queue: %llu / %llu   Pace: %.2f ms",
      smoothed_utilization_percent_, smoothed_deviation_ms_, smoothed_latency_ms_,
      static_cast<unsigned long long>(queue_depth_estimated),
      static_cast<unsigned long long>(queue_capacity_estimated), smoothed_pacing_interval_ms_);
  ImGui::Text("VP: %.0f us", smoothed_vp_total_us_);
  if (ImGui::TreeNode("VP Workers")) {
    ImGui::Text("W: #  us");
    ImGui::Text("0: %2u %3u", snapshot.vp.workers[0].num_voices, snapshot.vp.workers[0].time_us);
    ImGui::Text("Sweeps/s: %.0f   Decode Iters/s: %.0f", smoothed_vp_sweeps_per_second_,
                smoothed_vp_decode_iters_per_second_);
    ImGui::TreePop();
  }
  ImGui::Text("GP Cycles (est): %.0f", smoothed_gp_cycles_);
  ImGui::Text("EP Cycles (est): %.0f", smoothed_ep_cycles_);
  ImGui::TextDisabled("Queue utilization/latency are queue-depth based and may stay near zero when audio keeps up.");
  ImGui::TextDisabled("VP/GP/EP values are host-side estimates in the current XMA pipeline.");
  ImGui::Separator();

  const float queue_plot_max = std::max(1.0f, static_cast<float>(queue_capacity_estimated));
  if (!queue_depth_history_.empty()) {
    ImGui::Text("Queue Depth Y-axis: 0 to %.0f frames (current %.0f)", queue_plot_max,
                static_cast<float>(queue_depth_estimated));
    DrawLineGraphNoTooltip("##queue_depth_graph", queue_depth_history_, 0.0f, queue_plot_max,
                           ImVec2(ImGui::GetContentRegionAvail().x, 56.0f));
  }
  if (!callback_interval_history_ms_.empty()) {
    float interval_max = 0.0f;
    double interval_sum = 0.0;
    for (float v : callback_interval_history_ms_) {
      interval_max = std::max(interval_max, v);
      interval_sum += static_cast<double>(v);
    }
    interval_max = std::max(4.0f, interval_max * 1.2f);
    const float interval_mean_ms = static_cast<float>(
        interval_sum / static_cast<double>(callback_interval_history_ms_.size()));
    ImGui::Text("Callback Interval Y-axis: 0.00 to %.2f ms (mean %.2f ms, current %.2f ms)",
                interval_max, interval_mean_ms, smoothed_pacing_interval_ms_);
    ImVec2 plot_min, plot_max;
    DrawLineGraphNoTooltip("##callback_interval_graph", callback_interval_history_ms_, 0.0f,
                           interval_max, ImVec2(ImGui::GetContentRegionAvail().x, 56.0f),
                           &plot_min, &plot_max);
    if (interval_max > 1.0e-5f) {
      const float plot_height = std::max(1.0f, plot_max.y - plot_min.y);
      const float mean_ratio = std::clamp(interval_mean_ms / interval_max, 0.0f, 1.0f);
      const float mean_y = plot_max.y - (mean_ratio * plot_height);
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      draw_list->AddLine(ImVec2(plot_min.x, mean_y), ImVec2(plot_max.x, mean_y),
                         ImGui::GetColorU32(ImVec4(1.0f, 0.9f, 0.1f, 0.9f)), 1.5f);
      char mean_text[48] = {};
      std::snprintf(mean_text, sizeof(mean_text), "mean %.2f ms", interval_mean_ms);
      const ImVec2 mean_text_size = ImGui::CalcTextSize(mean_text);
      const ImVec2 mean_text_pos(plot_min.x + 6.0f, mean_y - ImGui::GetTextLineHeight() - 1.0f);
      const ImVec2 mean_bg_min(mean_text_pos.x - 3.0f, mean_text_pos.y - 1.0f);
      const ImVec2 mean_bg_max(mean_text_pos.x + mean_text_size.x + 3.0f,
                               mean_text_pos.y + mean_text_size.y + 1.0f);
      draw_list->AddRectFilled(mean_bg_min, mean_bg_max,
                               ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.42f)), 2.0f);
      draw_list->AddText(mean_text_pos, ImGui::GetColorU32(ImVec4(1.0f, 0.93f, 0.2f, 0.95f)),
                         mean_text);
    }
  }

  ImGui::Separator();

  std::unordered_map<uint32_t, uint64_t> frame_context_signatures;
  frame_context_signatures.reserve(snapshot.xma_contexts.size());
  std::unordered_map<uint64_t, uint32_t> frame_signature_to_context;
  frame_signature_to_context.reserve(snapshot.xma_contexts.size());
  for (const auto& ctx : snapshot.xma_contexts) {
    const uint64_t signature = BuildContextSignature(ctx);
    if (!signature) {
      continue;
    }
    frame_context_signatures.emplace(ctx.index, signature);
    context_last_signatures_[ctx.index] = signature;
    frame_signature_to_context[signature] = ctx.index;
    const auto name_it = context_names_.find(ctx.index);
    if (name_it != context_names_.end() && !name_it->second.empty()) {
      signature_names_[signature] = name_it->second;
    }
  }
  auto resolve_context_name = [&](uint32_t context_index) -> std::string {
    const auto sig_it = frame_context_signatures.find(context_index);
    if (sig_it != frame_context_signatures.end()) {
      const auto name_by_sig_it = signature_names_.find(sig_it->second);
      if (name_by_sig_it != signature_names_.end() && !name_by_sig_it->second.empty()) {
        return name_by_sig_it->second;
      }
    }
    const auto name_it = context_names_.find(context_index);
    if (name_it != context_names_.end()) {
      return name_it->second;
    }
    return {};
  };

  std::optional<uint32_t> hovered_context;
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    drag_mute_active_ = false;
    drag_mute_applied_contexts_.clear();
  }
  struct VisibleSquare {
    const AudioXmaContextInfo* ctx = nullptr;
    std::string label;
    uint64_t signature = 0;
    bool active = false;
  };
  std::vector<VisibleSquare> visible_squares;
  visible_squares.reserve(snapshot.xma_contexts.size());
  if (show_playing_contexts_only_) {
    struct PlayingInstance {
      const AudioXmaContextInfo* ctx = nullptr;
      uint64_t signature = 0;
      uint32_t slot = 0;
    };
    std::unordered_map<uint64_t, const AudioXmaContextInfo*> playing_by_signature;
    playing_by_signature.reserve(snapshot.xma_contexts.size());
    for (const auto& ctx : snapshot.xma_contexts) {
      if (!IsPlayingContext(ctx, meter_use_audible_)) {
        continue;
      }
      const uint64_t signature = BuildContextSignature(ctx);
      if (!signature) {
        visible_squares.push_back({&ctx, {}, 0, true});
        continue;
      }
      const auto existing_it = playing_by_signature.find(signature);
      if (existing_it == playing_by_signature.end()) {
        playing_by_signature.emplace(signature, &ctx);
        continue;
      }
      const auto* existing_ctx = existing_it->second;
      if (!existing_ctx || ctx.peak_level > existing_ctx->peak_level ||
          (ctx.peak_level == existing_ctx->peak_level && ctx.rms_level > existing_ctx->rms_level)) {
        existing_it->second = &ctx;
      }
    }

    const double now = ImGui::GetTime();
    auto allocate_signature_slot = [&](uint64_t signature) -> uint32_t {
      const auto existing_slot_it = signature_display_slots_.find(signature);
      if (existing_slot_it != signature_display_slots_.end()) {
        return existing_slot_it->second;
      }
      uint32_t slot = 0;
      slot = static_cast<uint32_t>(signature_slot_order_.size());
      signature_slot_order_.push_back(signature);
      signature_display_slots_[signature] = slot;
      return slot;
    };

    std::vector<PlayingInstance> playing_instances;
    playing_instances.reserve(playing_by_signature.size());
    for (const auto& [signature, ctx] : playing_by_signature) {
      const uint32_t slot = allocate_signature_slot(signature);
      signature_last_seen_times_[signature] = now;
      playing_instances.push_back({ctx, signature, slot});
    }

    std::unordered_map<uint32_t, const PlayingInstance*> playing_by_slot;
    playing_by_slot.reserve(playing_instances.size());
    for (const auto& instance : playing_instances) {
      playing_by_slot.emplace(instance.slot, &instance);
    }

    visible_squares.reserve(visible_squares.size() + signature_slot_order_.size());
    for (uint32_t slot = 0; slot < signature_slot_order_.size(); ++slot) {
      const auto active_it = playing_by_slot.find(slot);
      if (active_it != playing_by_slot.end()) {
        const auto* instance = active_it->second;
        visible_squares.push_back(
            {instance->ctx, fmt::format("C{:02X}", slot & 0xFFu), instance->signature, true});
      } else {
        const uint64_t signature = signature_slot_order_[slot];
        visible_squares.push_back(
            {nullptr, fmt::format("C{:02X}", slot & 0xFFu), signature, false});
      }
    }
  } else {
    for (const auto& ctx : snapshot.xma_contexts) {
      if (ctx.allocated) {
        known_contexts_.insert(ctx.index);
      }
      const bool known_context = known_contexts_.find(ctx.index) != known_contexts_.end();
      if (!ctx.allocated && !known_context) {
        continue;
      }
      visible_squares.push_back({&ctx, fmt::format("{:03X}", ctx.index), 0, true});
    }
  }
  tracked_volume_contexts_.erase(
      std::remove_if(tracked_volume_contexts_.begin(), tracked_volume_contexts_.end(),
                     [&](uint32_t context_index) {
                       if (context_index >= snapshot.xma_contexts.size()) {
                          meter_db_levels_.erase(context_index);
                          meter_max_db_levels_.erase(context_index);
                          meter_last_volume_levels_.erase(context_index);
                          persisted_context_volumes_.erase(context_index);
                          tracked_signature_hints_.erase(context_index);
                          return true;
                        }
                        return false;
                     }),
      tracked_volume_contexts_.end());

  const float row_height = ImGui::GetTextLineHeightWithSpacing();
  const float grid_width = std::min(
      grid_width_required,
      std::max(220.0f,
               ImGui::GetContentRegionAvail().x - kRightPanelMinWidth - style.ItemSpacing.x));
  ImGui::BeginChild("xma_grid", ImVec2(grid_width, 0), true);
  ImGui::Text("XMA Contexts (square view)");
  const float mode_button_width = 78.0f;
  const float hover_button_width = 112.0f;
  const float button_spacing = style.ItemInnerSpacing.x;
  const float mode_button_x =
      ImGui::GetContentRegionAvail().x - (mode_button_width + button_spacing + hover_button_width);
  if (mode_button_x > 8.0f) {
    ImGui::SameLine(mode_button_x);
  } else {
    ImGui::SameLine();
  }
  if (ImGui::Button(show_playing_contexts_only_ ? "Context" : "Voice",
                    ImVec2(mode_button_width, 0.0f))) {
    show_playing_contexts_only_ = !show_playing_contexts_only_;
    layout_dirty_ = true;
  }
  ImGui::SameLine(0.0f, button_spacing);
  if (ImGui::Button(hover_solo_enabled_ ? "Solo: ON" : "Solo: OFF",
                    ImVec2(hover_button_width, 0.0f))) {
    hover_solo_enabled_ = !hover_solo_enabled_;
    if (!hover_solo_enabled_) {
      ClearHoverSoloOverride();
      ApplyPersistedMuteStateToSnapshot(snapshot);
    }
  }
  ImGui::Text("Right-click mute/unmute (signature-persistent), right-drag paint, left-click pin details");
  ImGui::Text("Names/tracked voices auto-follow by XMA buffer signature");
  ImGui::Text("Green=enabled, red=muted, brightness=linear %s peak",
              meter_use_audible_ ? "audible" : "decode");
  ImGui::Text("Mode: %s", show_playing_contexts_only_
                             ? "playing-context instances (stable slots by buffer signature)"
                             : "hex voice slots (allocated/known)");
  ImGui::Separator();
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
  const float decay_per_second = 1.6f;
  const float decay_step = decay_per_second * std::max(io.DeltaTime, 0.0f);
  if (visible_squares.empty()) {
    ImGui::TextUnformatted(show_playing_contexts_only_ ? "No contexts are currently playing."
                                                       : "No allocated XMA contexts.");
  }
  for (size_t i = 0; i < visible_squares.size(); ++i) {
    const auto& square = visible_squares[i];
    const AudioXmaContextInfo* ctx_ptr = square.ctx;
    if (i % 16 != 0) {
      ImGui::SameLine();
    }

    float base = 0.14f;
    float hue = 0.58f;
    float sat = 0.0f;
    if (!ctx_ptr) {
      hue = 0.0f;
      sat = 0.0f;
      base = 0.07f;
    } else if (ctx_ptr->muted) {
      hue = 0.0f;
      sat = 0.80f;
      base = 0.24f;
    } else if (ctx_ptr->enabled) {
      hue = 0.33f;
      sat = 0.80f;
      base = 0.26f;
    } else if (ctx_ptr->input0_valid || ctx_ptr->input1_valid || ctx_ptr->output_valid) {
      hue = 0.10f;
      sat = 0.70f;
      base = 0.22f;
    } else {
      hue = 0.58f;
      sat = 0.50f;
      base = 0.18f;
    }

    float display_level = 0.0f;
    float color_activity = 1.0f;
    if (ctx_ptr) {
      const float prev_level = display_levels_[ctx_ptr->index];
      const float decayed = std::max(0.0f, prev_level - decay_step);
      const float peak_level = std::clamp(GetContextPeakForMode(*ctx_ptr, meter_use_audible_), 0.0f, 1.0f);
      display_level = std::max(peak_level, decayed);
      display_levels_[ctx_ptr->index] = display_level;
      const float observed_peak = peak_level;
      const float previous_observed_peak = last_observed_peak_levels_[ctx_ptr->index];
      const double now_time = ImGui::GetTime();
      double& last_peak_change_time = last_peak_change_times_[ctx_ptr->index];
      if (last_peak_change_time == 0.0) {
        last_peak_change_time = now_time;
      }
      if (std::fabs(observed_peak - previous_observed_peak) > 0.01f) {
        last_peak_change_time = now_time;
        last_observed_peak_levels_[ctx_ptr->index] = observed_peak;
      }
      const float inactivity_seconds = static_cast<float>(now_time - last_peak_change_time);
      color_activity =
          inactivity_seconds <= 1.0f
              ? 1.0f
              : std::clamp(1.0f - ((inactivity_seconds - 1.0f) / 1.5f), 0.0f, 1.0f);
    }
    sat *= color_activity;

    const float activity_boost = display_level * 0.72f * color_activity;
    const float val = std::clamp(base + activity_boost, 0.0f, 1.0f);

    ImGui::PushID(static_cast<int>(i));
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, sat, val));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, sat, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, sat, 1.0f));

    ImGui::Button(square.label.c_str(), ImVec2(30.0f, row_height));
    if (ctx_ptr && selected_context_ && *selected_context_ == ctx_ptr->index) {
      const float pulse_wave =
          0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 2.0 * 3.141592653589793 * 0.6));
      const float alpha = 0.30f + 0.60f * pulse_wave;
      const float thickness = 1.5f + 1.0f * pulse_wave;
      ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                          ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha)),
                                          0.0f, 0, thickness);
    }
    if (ImGui::IsItemHovered()) {
      if (ctx_ptr) {
        hovered_context = ctx_ptr->index;
        const uint32_t sample_rate_hz = SampleRateIdToHz(ctx_ptr->sample_rate_id);
        const uint32_t channels = ctx_ptr->stereo ? 2u : 1u;
        const uint32_t sample_size_bytes = (kXmaPcmBitsPerSample / 8u) * channels;
        const uint32_t in0_size = ctx_ptr->input_buffer_0_packet_count * kXmaPacketBytes;
        const uint32_t in1_size = ctx_ptr->input_buffer_1_packet_count * kXmaPacketBytes;
        ImGui::SetTooltip(
            "ctx=0x%03X\nrate=%u Hz (id=%u)\ncontainer: in0=%u pkts (%u bytes), in1=%u pkts (%u "
            "bytes)\nsample size=%u bytes (%u-bit %s), frame=%u samples/ch\nmultipass bin=%u  "
            "mix=%s\nout=0x%08X\nin0=0x%08X\nin1=0x%08X",
            ctx_ptr->index, sample_rate_hz, static_cast<uint32_t>(ctx_ptr->sample_rate_id),
            static_cast<uint32_t>(ctx_ptr->input_buffer_0_packet_count), in0_size,
            static_cast<uint32_t>(ctx_ptr->input_buffer_1_packet_count), in1_size, sample_size_bytes,
            kXmaPcmBitsPerSample, ctx_ptr->stereo ? "stereo" : "mono", kXmaSamplesPerFramePerChannel,
            static_cast<uint32_t>(ctx_ptr->packet_metadata), ctx_ptr->consume_only ? "consume-only" : "decode",
            ctx_ptr->output_buffer_ptr, ctx_ptr->input_buffer_0_ptr, ctx_ptr->input_buffer_1_ptr);
      } else if (show_playing_contexts_only_) {
        ImGui::SetTooltip("Slot inactive");
      }
    }
    if (ctx_ptr && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      if (selected_context_ && *selected_context_ == ctx_ptr->index) {
        selected_context_.reset();
      } else {
        selected_context_ = ctx_ptr->index;
      }
    }
    if (ctx_ptr && set_context_mute_) {
      uint64_t mute_signature = 0;
      const auto sig_it = frame_context_signatures.find(ctx_ptr->index);
      if (sig_it != frame_context_signatures.end()) {
        mute_signature = sig_it->second;
      }
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        drag_mute_active_ = true;
        drag_mute_target_ = !ctx_ptr->muted;
        drag_mute_applied_contexts_.clear();
        set_context_mute_(ctx_ptr->index, drag_mute_target_);
        if (drag_mute_target_) {
          if (mute_signature) {
            persisted_muted_signatures_.insert(mute_signature);
            persisted_muted_contexts_.erase(ctx_ptr->index);
          } else {
            persisted_muted_contexts_.insert(ctx_ptr->index);
          }
        } else {
          persisted_muted_contexts_.erase(ctx_ptr->index);
          if (mute_signature) {
            persisted_muted_signatures_.erase(mute_signature);
          }
        }
        layout_dirty_ = true;
        drag_mute_applied_contexts_.insert(ctx_ptr->index);
      } else if (drag_mute_active_ && ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
                 ImGui::IsItemHovered()) {
        if (drag_mute_applied_contexts_.find(ctx_ptr->index) == drag_mute_applied_contexts_.end()) {
          set_context_mute_(ctx_ptr->index, drag_mute_target_);
          if (drag_mute_target_) {
            if (mute_signature) {
              persisted_muted_signatures_.insert(mute_signature);
              persisted_muted_contexts_.erase(ctx_ptr->index);
            } else {
              persisted_muted_contexts_.insert(ctx_ptr->index);
            }
          } else {
            persisted_muted_contexts_.erase(ctx_ptr->index);
            if (mute_signature) {
              persisted_muted_signatures_.erase(mute_signature);
            }
          }
          layout_dirty_ = true;
          drag_mute_applied_contexts_.insert(ctx_ptr->index);
        }
      }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopID();
  }
  ImGui::PopStyleVar(3);

  const AudioXmaContextInfo* detail = nullptr;
  if (hovered_context) {
    for (const auto& ctx : snapshot.xma_contexts) {
      if (ctx.index == *hovered_context) {
        detail = &ctx;
        break;
      }
    }
  }
  if (!detail && selected_context_) {
    for (const auto& ctx : snapshot.xma_contexts) {
      if (ctx.index == *selected_context_) {
        detail = &ctx;
        break;
      }
    }
  }

  UpdateHoverSoloOverride(snapshot, hovered_context);

  ImGui::Separator();
  ImGui::Text("Context Details");
  ImGui::Separator();
  if (!detail) {
    ImGui::TextUnformatted("Hover or click a context square.");
  } else {
    if (ImGui::BeginTable("context_detail_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("##ctx_left", ImGuiTableColumnFlags_WidthStretch, 3.2f);
      ImGui::TableSetupColumn("##ctx_right", ImGuiTableColumnFlags_WidthStretch, 1.2f);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("Context: 0x%03X (%u)", detail->index, detail->index);
      ImGui::Text("Allocated: %s  Enabled: %s", detail->allocated ? "yes" : "no",
                  detail->enabled ? "yes" : "no");
      const float detail_decode_rms = std::clamp(detail->rms_level, 0.0f, 1.0f);
      const float detail_decode_rms_dbfs = std::max(kMeterFloorDbfs, LinearToDbfs(detail_decode_rms));
      const float detail_audible_rms = std::clamp(detail->audible_rms_level, 0.0f, 1.0f);
      const float detail_audible_rms_dbfs =
          std::max(kMeterFloorDbfs, LinearToDbfs(detail_audible_rms));
      ImGui::Text("Muted: %s  Decode RMS: %.3f (%.1f dBFS)", detail->muted ? "yes" : "no",
                  detail_decode_rms, detail_decode_rms_dbfs);
      ImGui::Text("Audible RMS: %.3f (%.1f dBFS)", detail_audible_rms, detail_audible_rms_dbfs);
      ImGui::Text("In0: %s  In1: %s  Out: %s", detail->input0_valid ? "yes" : "no",
                  detail->input1_valid ? "yes" : "no", detail->output_valid ? "yes" : "no");
      ImGui::Text("Stereo: %s  ConsumeOnly: %s", detail->stereo ? "yes" : "no",
                  detail->consume_only ? "yes" : "no");
      ImGui::Text("StopWhenDone: %s  IRQWhenDone: %s", detail->stop_when_done ? "yes" : "no",
                  detail->interrupt_when_done ? "yes" : "no");
      ImGui::Separator();
      ImGui::Text("Guest Ptr: 0x%08X", detail->guest_ptr);
      ImGui::Text("Input0 Ptr: 0x%08X", detail->input_buffer_0_ptr);
      ImGui::Text("Input1 Ptr: 0x%08X", detail->input_buffer_1_ptr);
      ImGui::Text("Output Ptr: 0x%08X", detail->output_buffer_ptr);
      ImGui::Separator();
      ImGui::Text("Current Buffer: %u", detail->current_buffer);
      ImGui::Text("Input Read Offset: %u", detail->input_buffer_read_offset);
      ImGui::Text("Subframe Decode Count: %u", detail->subframe_decode_count);
      ImGui::Text("Output Blocks: %u  Read: %u  Write: %u", detail->output_buffer_block_count,
                  detail->output_buffer_read_offset, detail->output_buffer_write_offset);
      const uint32_t detail_rate_hz = SampleRateIdToHz(detail->sample_rate_id);
      const uint32_t detail_channels = detail->stereo ? 2u : 1u;
      const uint32_t detail_sample_size_bytes = (kXmaPcmBitsPerSample / 8u) * detail_channels;
      const uint32_t detail_in0_size =
          static_cast<uint32_t>(detail->input_buffer_0_packet_count) * kXmaPacketBytes;
      const uint32_t detail_in1_size =
          static_cast<uint32_t>(detail->input_buffer_1_packet_count) * kXmaPacketBytes;
      ImGui::Text("Rate: %u Hz (id=%u)  Loop Count: %u", detail_rate_hz,
                  static_cast<uint32_t>(detail->sample_rate_id),
                  static_cast<uint32_t>(detail->loop_count));
      ImGui::Text("Container: in0=%u pkts (%u bytes)  in1=%u pkts (%u bytes)",
                  static_cast<uint32_t>(detail->input_buffer_0_packet_count), detail_in0_size,
                  static_cast<uint32_t>(detail->input_buffer_1_packet_count), detail_in1_size);
      ImGui::Text("Sample Size: %u bytes (%u-bit %s)  Frame: %u/ch", detail_sample_size_bytes,
                  kXmaPcmBitsPerSample, detail->stereo ? "stereo" : "mono",
                  kXmaSamplesPerFramePerChannel);
      ImGui::Text("Multipass Bin: %u  Mix: %s", static_cast<uint32_t>(detail->packet_metadata),
                  detail->consume_only ? "consume-only" : "decode");
      ImGui::Text("Loop SF start/end/skip: %u / %u / %u  Output Padding: %u",
                  static_cast<uint32_t>(detail->loop_subframe_start),
                  static_cast<uint32_t>(detail->loop_subframe_end),
                  static_cast<uint32_t>(detail->loop_subframe_skip),
                  static_cast<uint32_t>(detail->output_buffer_padding));

      ImGui::TableSetColumnIndex(1);
      const auto detail_sig_it = frame_context_signatures.find(detail->index);
      const uint64_t detail_signature =
          detail_sig_it != frame_context_signatures.end() ? detail_sig_it->second : 0ULL;
      std::string context_name = resolve_context_name(detail->index);
      char name_buf[64] = {};
      std::snprintf(name_buf, sizeof(name_buf), "%s", context_name.c_str());
      uint32_t tracked_source_index = detail->index;
      bool tracked_in_volume = ContainsContext(tracked_volume_contexts_, detail->index);
      if (!tracked_in_volume && detail_signature) {
        for (uint32_t source_index : tracked_volume_contexts_) {
          const auto hint_it = tracked_signature_hints_.find(source_index);
          if (hint_it != tracked_signature_hints_.end() && hint_it->second == detail_signature) {
            tracked_in_volume = true;
            tracked_source_index = source_index;
            break;
          }
        }
      }
      const ImGuiStyle& ui_style = ImGui::GetStyle();
      const float arrow_button_width = ImGui::GetFrameHeight();
      ImGui::PushItemWidth(-arrow_button_width - ui_style.ItemInnerSpacing.x);
      if (ImGui::InputTextWithHint("##voice_name", "Name", name_buf, sizeof(name_buf))) {
        context_names_[detail->index] = name_buf;
        if (detail_signature) {
          if (name_buf[0] == '\0') {
            signature_names_.erase(detail_signature);
          } else {
            signature_names_[detail_signature] = name_buf;
          }
        }
        layout_dirty_ = true;
      }
      ImGui::PopItemWidth();
      ImGui::SameLine(0.0f, ui_style.ItemInnerSpacing.x);
      if (!tracked_in_volume) {
        if (ImGui::Button("->", ImVec2(arrow_button_width, 0.0f))) {
          tracked_volume_contexts_.push_back(detail->index);
          if (detail_signature) {
            tracked_signature_hints_[detail->index] = detail_signature;
          }
          layout_dirty_ = true;
        }
      } else if (ImGui::Button("x", ImVec2(arrow_button_width, 0.0f))) {
        EraseContext(tracked_volume_contexts_, tracked_source_index);
        tracked_signature_hints_.erase(tracked_source_index);
        layout_dirty_ = true;
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginChild("xma_levels", ImVec2(0, 0), true);
  ImGui::Text("Volume Indicators (%s RMS dBFS)", meter_use_audible_ ? "Audible" : "Decode");
  ImGui::SameLine();
  if (ImGui::Button(normalize_volume_panel_ ? "Normalized" : "Absolute dBFS")) {
    normalize_volume_panel_ = !normalize_volume_panel_;
    meter_max_db_levels_.clear();
    layout_dirty_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button(meter_use_audible_ ? "Meters: Audible" : "Meters: Decode")) {
    meter_use_audible_ = !meter_use_audible_;
    display_levels_.clear();
    meter_db_levels_.clear();
    meter_max_db_levels_.clear();
    meter_last_volume_levels_.clear();
    last_observed_peak_levels_.clear();
    last_peak_change_times_.clear();
    layout_dirty_ = true;
  }
  if (normalize_volume_panel_) {
    ImGui::Text("Scale: %.0f dBFS to per-voice observed max", kMeterFloorDbfs);
  } else {
    ImGui::Text("Scale: %.0f dBFS to 0 dBFS", kMeterFloorDbfs);
  }
  ImGui::TextUnformatted(
      meter_use_audible_
          ? "Note: audible meter is post mute/voice-volume at XMA output (still pre-send/pre-final mix)."
          : "Note: decode meter is raw decoded context level before mute/voice-volume.");
  ImGui::Separator();
  if (tracked_volume_contexts_.empty()) {
    ImGui::TextUnformatted("No tracked voices. Add one from Context Details.");
  } else {
    struct VolumeRow {
      uint32_t source_index = 0;
      uint32_t index = 0;
      uint64_t signature = 0;
      const AudioXmaContextInfo* ctx = nullptr;
      std::string label;
    };
    std::vector<VolumeRow> rows;
    rows.reserve(tracked_volume_contexts_.size());
    for (uint32_t source_context_index : tracked_volume_contexts_) {
      if (source_context_index >= snapshot.xma_contexts.size()) {
        continue;
      }
      uint32_t resolved_context_index = source_context_index;
      uint64_t signature_hint = 0;
      const auto tracked_sig_hint_it = tracked_signature_hints_.find(source_context_index);
      if (tracked_sig_hint_it != tracked_signature_hints_.end()) {
        signature_hint = tracked_sig_hint_it->second;
      } else {
        const auto last_sig_it = context_last_signatures_.find(source_context_index);
        if (last_sig_it != context_last_signatures_.end()) {
          signature_hint = last_sig_it->second;
        }
      }
      if (signature_hint) {
        const auto mapped_it = frame_signature_to_context.find(signature_hint);
        if (mapped_it != frame_signature_to_context.end()) {
          resolved_context_index = mapped_it->second;
          tracked_signature_hints_[source_context_index] = signature_hint;
        }
      }
      const auto& ctx = snapshot.xma_contexts[resolved_context_index];
      const uint64_t context_signature = BuildContextSignature(ctx);
      if (context_signature) {
        tracked_signature_hints_[source_context_index] = context_signature;
      }
      const std::string resolved_name = resolve_context_name(ctx.index);
      VolumeRow row;
      row.source_index = source_context_index;
      row.index = ctx.index;
      row.signature = context_signature;
      row.ctx = &ctx;
      if (!resolved_name.empty()) {
        row.label = resolved_name + " (0x" + fmt::format("{:03X}", ctx.index) + ")";
      } else {
        row.label = "0x" + fmt::format("{:03X}", ctx.index);
      }
      if (!ctx.allocated) {
        row.label += " [inactive]";
      }
      if (!ignore_persisted_controls_) {
        auto persisted_volume_it = persisted_context_volumes_.find(ctx.index);
        if (persisted_volume_it == persisted_context_volumes_.end()) {
          persisted_context_volumes_.emplace(ctx.index, std::clamp(ctx.volume, 0.0f, 1.0f));
        }
      }
      rows.push_back(std::move(row));
    }

    const ImGuiStyle& ui_style = ImGui::GetStyle();
    const float meter_right_x = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    float max_meter_scale_width = 1.0f;
    for (const auto& row : rows) {
      const float label_width = ImGui::CalcTextSize(row.label.c_str()).x;
      const float meter_start_x = ImGui::GetCursorScreenPos().x + label_width + ui_style.ItemSpacing.x;
      max_meter_scale_width = std::max(max_meter_scale_width, meter_right_x - meter_start_x);
    }

    for (const auto& row : rows) {
      const auto& ctx = *row.ctx;
      ImGui::PushID(static_cast<int>(row.index));
      ImGui::TextUnformatted(row.label.c_str());
      ImGui::SameLine();

      float voice_volume = std::clamp(ctx.volume, kVoiceVolumeMin, kVoiceVolumeMax);
      if (!ignore_persisted_controls_) {
        const auto persisted_volume_it = persisted_context_volumes_.find(ctx.index);
        if (persisted_volume_it != persisted_context_volumes_.end()) {
          voice_volume = std::clamp(persisted_volume_it->second, kVoiceVolumeMin, kVoiceVolumeMax);
        }
      }
      const float decode_rms = std::clamp(ctx.rms_level, 0.0f, 1.0f);
      const float audible_rms = std::clamp(ctx.audible_rms_level, 0.0f, 1.0f);
      const float meter_rms = meter_use_audible_ ? audible_rms : decode_rms;
      const float current_db = std::max(kMeterFloorDbfs, LinearToDbfs(meter_rms));
      const auto prev_it = meter_db_levels_.find(ctx.index);
      float shown_db = current_db;
      const float prev_volume = meter_last_volume_levels_[ctx.index];
      const bool volume_reduced_now = voice_volume + 0.005f < prev_volume;
      if (!volume_reduced_now && prev_it != meter_db_levels_.end() && current_db < prev_it->second) {
        const float db_step = kMeterReleaseDbPerSecond * std::max(0.0f, io.DeltaTime);
        shown_db = std::max(current_db, prev_it->second - db_step);
      }
      meter_db_levels_[ctx.index] = shown_db;
      meter_last_volume_levels_[ctx.index] = voice_volume;
      float level = DbfsToNormalized(shown_db);
      float limit_level = DbfsToNormalized(
          std::max(kMeterFloorDbfs,
                   LinearToDbfs(std::clamp(ctx.muted ? 0.0f : decode_rms * voice_volume, 0.0f, 1.0f))));
      if (normalize_volume_panel_) {
        auto max_it = meter_max_db_levels_.find(ctx.index);
        if (max_it == meter_max_db_levels_.end()) {
          max_it = meter_max_db_levels_.emplace(ctx.index, shown_db).first;
        } else if (max_it->second < shown_db) {
          max_it->second = shown_db;
        }
        const float normalized_range = std::max(max_it->second - kMeterFloorDbfs, 1.0e-3f);
        level = std::clamp((shown_db - kMeterFloorDbfs) / normalized_range, 0.0f, 1.0f);
        const float limit_db = std::max(
            kMeterFloorDbfs,
            LinearToDbfs(std::clamp(ctx.muted ? 0.0f : decode_rms * voice_volume, 0.0f, 1.0f)));
        limit_level = std::clamp((limit_db - kMeterFloorDbfs) / normalized_range, 0.0f, 1.0f);
      }

      ImGui::ProgressBar(0.0f, ImVec2(-1.0f, 0.0f), "");
      const ImVec2 bar_min = ImGui::GetItemRectMin();
      const ImVec2 bar_max = ImGui::GetItemRectMax();
      const ImVec2 bar_size(bar_max.x - bar_min.x, bar_max.y - bar_min.y);
      ImGui::SetCursorScreenPos(bar_min);
      ImGui::InvisibleButton("##volume_bar_hit", bar_size);
      const bool bar_hovered = ImGui::IsItemHovered();
      const bool bar_active = ImGui::IsItemActive();
      const float fill_end_target_x = bar_max.x - (1.0f - level) * max_meter_scale_width;
      const float fill_end_x = std::clamp(fill_end_target_x, bar_min.x, bar_max.x);
      if (fill_end_x > bar_min.x) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            bar_min, ImVec2(fill_end_x, bar_max.y), ImGui::GetColorU32(ImGuiCol_PlotHistogram),
            ui_style.FrameRounding);
      }
      const float limited_end_target_x = bar_max.x - (1.0f - limit_level) * max_meter_scale_width;
      const float limited_end_x = std::clamp(limited_end_target_x, bar_min.x, bar_max.x);
      ImGui::GetWindowDrawList()->AddLine(
          ImVec2(limited_end_x, bar_min.y + 1.0f), ImVec2(limited_end_x, bar_max.y - 1.0f),
          ImGui::GetColorU32(ImVec4(1.0f, 0.92f, 0.35f, 0.95f)), 2.0f);
      const float slider_x = bar_min.x + (bar_max.x - bar_min.x) * voice_volume;
      const float slider_x_clamped = std::clamp(slider_x, bar_min.x, bar_max.x);
      ImGui::GetWindowDrawList()->AddLine(
          ImVec2(slider_x_clamped, bar_min.y), ImVec2(slider_x_clamped, bar_max.y),
          ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.85f)), 1.8f);
      if (set_context_volume_ && (bar_active || (bar_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)))) {
        const float width = std::max(1.0f, bar_max.x - bar_min.x);
        const float mouse_norm =
            std::clamp((ImGui::GetIO().MousePos.x - bar_min.x) / width, 0.0f, 1.0f);
        const float target_volume =
            kVoiceVolumeMin + (kVoiceVolumeMax - kVoiceVolumeMin) * mouse_norm;
        set_context_volume_(ctx.index, target_volume);
        if (!ignore_persisted_controls_) {
          persisted_context_volumes_[ctx.index] = target_volume;
        }
        layout_dirty_ = true;
      }

      char overlay_text[32];
      std::snprintf(overlay_text, sizeof(overlay_text), "%.1f dBFS", shown_db);
      const ImVec2 text_size = ImGui::CalcTextSize(overlay_text);
      const float text_pad_x = 6.0f;
      const float text_x = std::max(bar_min.x + text_pad_x, bar_max.x - text_size.x - text_pad_x);
      const float text_y = bar_min.y + (bar_max.y - bar_min.y - text_size.y) * 0.5f;
      ImGui::GetWindowDrawList()->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text),
                                          overlay_text);

      ImGui::PopID();
    }
  }
  ImGui::EndChild();

  const ImVec2 window_size = ImGui::GetWindowSize();
  if (std::fabs(window_size.x - pending_window_width_) > 0.5f ||
      std::fabs(window_size.y - pending_window_height_) > 0.5f) {
    pending_window_width_ = window_size.x;
    pending_window_height_ = window_size.y;
    layout_dirty_ = true;
  }

  SaveLayoutState(false);
  ImGui::End();
}

}  // namespace rex::ui
