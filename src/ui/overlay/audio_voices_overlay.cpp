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

float LinearToDbfs(float linear) {
  const float safe_linear = std::max(linear, 1.0e-6f);
  return 20.0f * std::log10(safe_linear);
}

float DbfsToNormalized(float dbfs) {
  return std::clamp((dbfs - kMeterFloorDbfs) / -kMeterFloorDbfs, 0.0f, 1.0f);
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
  SaveLayoutState(true);
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
  persisted_muted_contexts_ = std::move(loaded->muted_contexts);
  tracked_volume_contexts_ = std::move(loaded->tracked_volume_contexts);
  context_names_ = std::move(loaded->context_names);
  persisted_context_volumes_ = std::move(loaded->context_volumes);
  pending_apply_mute_layout_ = true;
  layout_dirty_ = false;
}

void AudioVoicesDialog::ApplyMutedLayoutState(const AudioVoicesSnapshot& snapshot) {
  if (!pending_apply_mute_layout_ || !set_context_mute_ || !snapshot.valid) {
    return;
  }
  for (uint32_t i = 0; i < snapshot.xma_contexts.size(); ++i) {
    const bool should_be_muted = persisted_muted_contexts_.find(i) != persisted_muted_contexts_.end();
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
  state.muted_contexts = persisted_muted_contexts_;
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
  ImGui::Separator();

  std::optional<uint32_t> hovered_context;
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    drag_mute_active_ = false;
    drag_mute_applied_contexts_.clear();
  }
  std::vector<const AudioXmaContextInfo*> visible_contexts;
  visible_contexts.reserve(snapshot.xma_contexts.size());
  for (const auto& ctx : snapshot.xma_contexts) {
    if (ctx.allocated) {
      visible_contexts.push_back(&ctx);
    }
  }
  for (auto it = tracked_volume_contexts_.begin(); it != tracked_volume_contexts_.end();) {
    if (*it >= snapshot.xma_contexts.size()) {
      meter_db_levels_.erase(*it);
      meter_max_db_levels_.erase(*it);
      it = tracked_volume_contexts_.erase(it);
    } else {
      ++it;
    }
  }

  const float row_height = ImGui::GetTextLineHeightWithSpacing();
  const float grid_width = std::min(
      grid_width_required,
      std::max(220.0f,
               ImGui::GetContentRegionAvail().x - kRightPanelMinWidth - style.ItemSpacing.x));
  ImGui::BeginChild("xma_grid", ImVec2(grid_width, 0), true);
  ImGui::Text("XMA Contexts (square view)");
  ImGui::Text("Right-click mute/unmute, right-drag paint, left-click pin details");
  ImGui::Text("Green=enabled, red=muted, brightness=linear peak");
  ImGui::Separator();
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
  const float decay_per_second = 1.6f;
  const float decay_step = decay_per_second * std::max(io.DeltaTime, 0.0f);
  if (visible_contexts.empty()) {
    ImGui::TextUnformatted("No allocated XMA contexts.");
  }
  for (size_t i = 0; i < visible_contexts.size(); ++i) {
    const auto& ctx = *visible_contexts[i];
    if (i % 16 != 0) {
      ImGui::SameLine();
    }

    float base = 0.14f;
    float hue = 0.58f;
    float sat = 0.0f;
    if (ctx.muted) {
      hue = 0.0f;
      sat = 0.80f;
      base = 0.24f;
    } else if (ctx.enabled) {
      hue = 0.33f;
      sat = 0.80f;
      base = 0.26f;
    } else if (ctx.input0_valid || ctx.input1_valid || ctx.output_valid) {
      hue = 0.10f;
      sat = 0.70f;
      base = 0.22f;
    } else {
      hue = 0.58f;
      sat = 0.50f;
      base = 0.18f;
    }

    const float prev_level = display_levels_[ctx.index];
    const float decayed = std::max(0.0f, prev_level - decay_step);
    const float display_level = std::max(ctx.peak_level, decayed);
    display_levels_[ctx.index] = display_level;

    const float val = std::clamp(base + display_level * 0.72f, 0.0f, 1.0f);

    ImGui::PushID(static_cast<int>(ctx.index));
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, sat, val));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, sat, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, sat, 1.0f));

    char label[8];
    std::snprintf(label, sizeof(label), "%03X", ctx.index);
    ImGui::Button(label, ImVec2(30.0f, row_height));
    if (selected_context_ && *selected_context_ == ctx.index) {
      const float pulse_wave =
          0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 2.0 * 3.141592653589793 * 0.6));
      const float alpha = 0.30f + 0.60f * pulse_wave;
      const float thickness = 1.5f + 1.0f * pulse_wave;
      ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                          ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha)),
                                          0.0f, 0, thickness);
    }
    if (ImGui::IsItemHovered()) {
      hovered_context = ctx.index;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      if (selected_context_ && *selected_context_ == ctx.index) {
        selected_context_.reset();
      } else {
        selected_context_ = ctx.index;
      }
    }
    if (set_context_mute_) {
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        drag_mute_active_ = true;
        drag_mute_target_ = !ctx.muted;
        drag_mute_applied_contexts_.clear();
        set_context_mute_(ctx.index, drag_mute_target_);
        if (drag_mute_target_) {
          persisted_muted_contexts_.insert(ctx.index);
        } else {
          persisted_muted_contexts_.erase(ctx.index);
        }
        layout_dirty_ = true;
        drag_mute_applied_contexts_.insert(ctx.index);
      } else if (drag_mute_active_ && ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
                 ImGui::IsItemHovered()) {
        if (drag_mute_applied_contexts_.find(ctx.index) == drag_mute_applied_contexts_.end()) {
          set_context_mute_(ctx.index, drag_mute_target_);
          if (drag_mute_target_) {
            persisted_muted_contexts_.insert(ctx.index);
          } else {
            persisted_muted_contexts_.erase(ctx.index);
          }
          layout_dirty_ = true;
          drag_mute_applied_contexts_.insert(ctx.index);
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
      const float detail_rms_dbfs =
          std::max(kMeterFloorDbfs, LinearToDbfs(std::clamp(detail->rms_level, 0.0f, 1.0f)));
      ImGui::Text("Muted: %s  Peak: %.3f  RMS: %.3f (%.1f dBFS)", detail->muted ? "yes" : "no",
                  detail->peak_level, detail->rms_level, detail_rms_dbfs);
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
      ImGui::Text("Sample Rate ID: %u  Loop Count: %u", detail->sample_rate_id, detail->loop_count);

      ImGui::TableSetColumnIndex(1);
      std::string& context_name = context_names_[detail->index];
      char name_buf[64] = {};
      std::snprintf(name_buf, sizeof(name_buf), "%s", context_name.c_str());
      const bool tracked_in_volume = tracked_volume_contexts_.find(detail->index) !=
                                     tracked_volume_contexts_.end();
      const ImGuiStyle& ui_style = ImGui::GetStyle();
      const float arrow_button_width = ImGui::GetFrameHeight();
      ImGui::PushItemWidth(-arrow_button_width - ui_style.ItemInnerSpacing.x);
      if (ImGui::InputTextWithHint("##voice_name", "Name", name_buf, sizeof(name_buf))) {
        context_name = name_buf;
        layout_dirty_ = true;
      }
      ImGui::PopItemWidth();
      ImGui::SameLine(0.0f, ui_style.ItemInnerSpacing.x);
      if (!tracked_in_volume) {
        if (ImGui::Button("->", ImVec2(arrow_button_width, 0.0f))) {
          tracked_volume_contexts_.insert(detail->index);
          layout_dirty_ = true;
        }
      } else if (ImGui::Button("x", ImVec2(arrow_button_width, 0.0f))) {
        tracked_volume_contexts_.erase(detail->index);
        layout_dirty_ = true;
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginChild("xma_levels", ImVec2(0, 0), true);
  ImGui::Text("Volume Indicators (RMS dBFS)");
  ImGui::SameLine();
  if (ImGui::Button(normalize_volume_panel_ ? "Normalized" : "Absolute dBFS")) {
    normalize_volume_panel_ = !normalize_volume_panel_;
    meter_max_db_levels_.clear();
    layout_dirty_ = true;
  }
  if (normalize_volume_panel_) {
    ImGui::Text("Scale: %.0f dBFS to per-voice observed max", kMeterFloorDbfs);
  } else {
    ImGui::Text("Scale: %.0f dBFS to 0 dBFS", kMeterFloorDbfs);
  }
  ImGui::Separator();
  if (tracked_volume_contexts_.empty()) {
    ImGui::TextUnformatted("No tracked voices. Add one from Context Details.");
  } else {
    struct VolumeRow {
      uint32_t index = 0;
      const AudioXmaContextInfo* ctx = nullptr;
      std::string label;
    };
    std::vector<VolumeRow> rows;
    rows.reserve(tracked_volume_contexts_.size());
    for (uint32_t context_index : tracked_volume_contexts_) {
      if (context_index >= snapshot.xma_contexts.size()) {
        continue;
      }
      const auto& ctx = snapshot.xma_contexts[context_index];
      if (!ctx.allocated) {
        continue;
      }
      const auto name_it = context_names_.find(ctx.index);
      VolumeRow row;
      row.index = ctx.index;
      row.ctx = &ctx;
      if (name_it != context_names_.end() && !name_it->second.empty()) {
        row.label = name_it->second + " (0x" + fmt::format("{:03X}", ctx.index) + ")";
      } else {
        row.label = "0x" + fmt::format("{:03X}", ctx.index);
      }
      auto persisted_volume_it = persisted_context_volumes_.find(ctx.index);
      if (persisted_volume_it == persisted_context_volumes_.end()) {
        persisted_context_volumes_.emplace(ctx.index, std::clamp(ctx.volume, 0.0f, 1.0f));
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
      const auto persisted_volume_it = persisted_context_volumes_.find(ctx.index);
      if (persisted_volume_it != persisted_context_volumes_.end()) {
        voice_volume = std::clamp(persisted_volume_it->second, kVoiceVolumeMin, kVoiceVolumeMax);
      }
      const float effective_rms = std::clamp(ctx.rms_level * voice_volume, 0.0f, 1.0f);
      const float current_db = std::max(kMeterFloorDbfs, LinearToDbfs(effective_rms));
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
      if (normalize_volume_panel_) {
        auto max_it = meter_max_db_levels_.find(ctx.index);
        if (max_it == meter_max_db_levels_.end()) {
          max_it = meter_max_db_levels_.emplace(ctx.index, shown_db).first;
        } else if (max_it->second < shown_db) {
          max_it->second = shown_db;
        }
        const float normalized_range = std::max(max_it->second - kMeterFloorDbfs, 1.0e-3f);
        level = std::clamp((shown_db - kMeterFloorDbfs) / normalized_range, 0.0f, 1.0f);
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
      const float slider_x = bar_min.x + (bar_max.x - bar_min.x) * voice_volume;
      const float slider_x_clamped = std::clamp(slider_x, bar_min.x, bar_max.x);
      if (voice_volume < 0.999f) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(slider_x_clamped, bar_min.y), bar_max, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.28f)),
            ui_style.FrameRounding);
      }
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
        persisted_context_volumes_[ctx.index] = target_volume;
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
