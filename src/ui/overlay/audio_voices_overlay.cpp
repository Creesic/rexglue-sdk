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

bool IsPlayingContext(const AudioXmaContextInfo& ctx) {
  if (!ctx.allocated) {
    return false;
  }
  return ctx.rms_level > 0.0005f || ctx.peak_level > 0.001f || ctx.enabled || ctx.input0_valid ||
         ctx.input1_valid || ctx.output_valid;
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
  ignore_persisted_controls_ = loaded->ignore_persisted_controls;
  show_playing_contexts_only_ = loaded->show_playing_contexts_only;
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
  if (ignore_persisted_controls_) {
    pending_apply_mute_layout_ = false;
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
  state.ignore_persisted_controls = ignore_persisted_controls_;
  state.show_playing_contexts_only = show_playing_contexts_only_;
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
  ImGui::SameLine();
  if (ImGui::Button(ignore_persisted_controls_ ? "Persisted Controls: OFF" : "Persisted Controls: ON")) {
    ignore_persisted_controls_ = !ignore_persisted_controls_;
    pending_apply_mute_layout_ = !ignore_persisted_controls_;
    layout_dirty_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button(show_playing_contexts_only_ ? "Squares: Playing Contexts" : "Squares: Hex Voices")) {
    show_playing_contexts_only_ = !show_playing_contexts_only_;
    layout_dirty_ = true;
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
      if (!IsPlayingContext(ctx)) {
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
  ImGui::Text("Right-click mute/unmute (signature-persistent), right-drag paint, left-click pin details");
  ImGui::Text("Names/tracked voices auto-follow by XMA buffer signature");
  ImGui::Text("Green=enabled, red=muted, brightness=linear peak");
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
      display_level = std::max(ctx_ptr->peak_level, decayed);
      display_levels_[ctx_ptr->index] = display_level;
      const float observed_peak = std::clamp(ctx_ptr->peak_level, 0.0f, 1.0f);
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
        ImGui::SetTooltip("ctx=0x%03X\nout=0x%08X\nin0=0x%08X\nin1=0x%08X", ctx_ptr->index,
                          ctx_ptr->output_buffer_ptr, ctx_ptr->input_buffer_0_ptr,
                          ctx_ptr->input_buffer_1_ptr);
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
      const float detail_effective_rms = std::clamp(detail->muted ? 0.0f : detail->rms_level * detail->volume,
                                                    0.0f, 1.0f);
      const float detail_effective_rms_dbfs =
          std::max(kMeterFloorDbfs, LinearToDbfs(detail_effective_rms));
      ImGui::Text("Muted: %s  Decode RMS: %.3f (%.1f dBFS)  Effective RMS: %.3f (%.1f dBFS)",
                  detail->muted ? "yes" : "no", detail->rms_level, detail_rms_dbfs,
                  detail_effective_rms, detail_effective_rms_dbfs);
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
  ImGui::Text("Volume Indicators (Effective RMS dBFS)");
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
  ImGui::TextUnformatted("Note: effective=decode RMS * context volume (still pre-mix/send matrix).");
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
      const float raw_rms = std::clamp(ctx.rms_level, 0.0f, 1.0f);
      const float effective_rms =
          std::clamp(ctx.muted ? 0.0f : raw_rms * std::clamp(voice_volume, 0.0f, 1.0f), 0.0f, 1.0f);
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
      const float filled_width = std::max(0.0f, fill_end_x - bar_min.x);
      const float limited_end_x = std::clamp(bar_min.x + filled_width * voice_volume, bar_min.x, bar_max.x);
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
