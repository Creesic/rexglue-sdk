/**
 * @file        rex/ui/overlay/audio_voices_overlay.h
 *
 * @brief       ImGui audio voices overlay dialog.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once

#include <rex/ui/imgui_dialog.h>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <vector>

namespace rex::ui {

struct AudioVoiceInfo {
  uint32_t client_index = 0;
  uint32_t driver_handle = 0;
  uint32_t callback = 0;
  uint32_t callback_arg = 0;
  uint64_t submitted_frames = 0;
};

struct AudioXmaContextInfo {
  uint32_t index = 0;
  bool allocated = false;
  bool enabled = false;
  bool input0_valid = false;
  bool input1_valid = false;
  bool output_valid = false;
  bool stop_when_done = false;
  bool interrupt_when_done = false;
  bool consume_only = false;
  bool stereo = false;
  bool muted = false;
  float volume = 1.0f;
  float peak_level = 0.0f;
  float rms_level = 0.0f;
  float audible_peak_level = 0.0f;
  float audible_rms_level = 0.0f;
  uint8_t current_buffer = 0;
  uint8_t subframe_decode_count = 0;
  uint8_t output_buffer_block_count = 0;
  uint8_t output_buffer_write_offset = 0;
  uint8_t output_buffer_read_offset = 0;
  uint8_t sample_rate_id = 0;
  uint8_t loop_count = 0;
  uint8_t output_buffer_padding = 0;
  uint8_t loop_subframe_start = 0;
  uint8_t loop_subframe_end = 0;
  uint8_t loop_subframe_skip = 0;
  uint8_t packet_metadata = 0;
  uint16_t input_buffer_0_packet_count = 0;
  uint16_t input_buffer_1_packet_count = 0;
  uint32_t guest_ptr = 0;
  uint32_t input_buffer_read_offset = 0;
  uint32_t input_buffer_0_ptr = 0;
  uint32_t input_buffer_1_ptr = 0;
  uint32_t output_buffer_ptr = 0;
};

struct AudioVoicesSnapshot {
  struct VpWorkerInfo {
    uint32_t num_voices = 0;
    uint32_t time_us = 0;
  };

  struct VpInfo {
    uint32_t total_worker_time_us = 0;
    uint32_t sweeps_per_second = 0;
    uint32_t decode_iterations_per_second = 0;
    std::array<VpWorkerInfo, 1> workers = {};
  };

  struct DspInfo {
    uint32_t cycles = 0;
  };

  bool valid = false;
  bool paused = false;
  uint32_t queued_frames = 0;
  uint64_t render_callbacks_total = 0;
  bool xma_paused = false;
  VpInfo vp = {};
  DspInfo gp = {};
  DspInfo ep = {};
  std::vector<AudioVoiceInfo> voices;
  std::vector<AudioXmaContextInfo> xma_contexts;
};

struct AudioVoicesLayoutState {
  bool has_window_size = false;
  float window_width = 0.0f;
  float window_height = 0.0f;
  bool normalize_volume_panel = false;
  bool ignore_persisted_controls = false;
  bool show_playing_contexts_only = false;
  bool meter_use_audible = true;
  std::unordered_set<uint32_t> muted_contexts;
  std::unordered_set<uint64_t> muted_signatures;
  std::vector<uint32_t> tracked_volume_contexts;
  std::unordered_map<uint32_t, std::string> context_names;
  std::unordered_map<uint32_t, float> context_volumes;
};

class AudioVoicesDialog : public ImGuiDialog {
 public:
  using SnapshotProvider = std::function<AudioVoicesSnapshot()>;
  using SetContextMuteCallback = std::function<void(uint32_t, bool)>;
  using SetContextVolumeCallback = std::function<void(uint32_t, float)>;
  using LoadLayoutStateCallback = std::function<std::optional<AudioVoicesLayoutState>()>;
  using SaveLayoutStateCallback = std::function<void(const AudioVoicesLayoutState&)>;

  AudioVoicesDialog(ImGuiDrawer* imgui_drawer, SnapshotProvider snapshot_provider,
                    SetContextMuteCallback set_context_mute,
                    SetContextVolumeCallback set_context_volume,
                    LoadLayoutStateCallback load_layout_state = {},
                    SaveLayoutStateCallback save_layout_state = {});
  ~AudioVoicesDialog();

  void SetSnapshotProvider(SnapshotProvider provider) { snapshot_provider_ = std::move(provider); }
  void SetContextMute(SetContextMuteCallback callback) {
    set_context_mute_ = std::move(callback);
  }
  void SetContextVolume(SetContextVolumeCallback callback) {
    set_context_volume_ = std::move(callback);
  }
  void SetLayoutStateCallbacks(LoadLayoutStateCallback load_callback,
                               SaveLayoutStateCallback save_callback) {
    load_layout_state_ = std::move(load_callback);
    save_layout_state_ = std::move(save_callback);
    layout_state_loaded_ = false;
    pending_apply_mute_layout_ = false;
  }

 protected:
  void OnDraw(ImGuiIO& io) override;
  void OnClose() override;

 private:
  void LoadLayoutStateOnce();
  void ApplyMutedLayoutState(const AudioVoicesSnapshot& snapshot);
  void ClearHoverSoloOverride();
  void ApplyPersistedMuteStateToSnapshot(const AudioVoicesSnapshot& snapshot);
  void UpdateHoverSoloOverride(const AudioVoicesSnapshot& snapshot,
                               const std::optional<uint32_t>& hovered_context);
  void SaveLayoutState(bool force);
  AudioVoicesLayoutState BuildLayoutState() const;

  SnapshotProvider snapshot_provider_;
  SetContextMuteCallback set_context_mute_;
  SetContextVolumeCallback set_context_volume_;
  LoadLayoutStateCallback load_layout_state_;
  SaveLayoutStateCallback save_layout_state_;
  std::optional<uint32_t> selected_context_;
  std::unordered_map<uint32_t, float> display_levels_;
  std::unordered_map<uint32_t, float> meter_db_levels_;
  std::unordered_map<uint32_t, float> meter_max_db_levels_;
  std::unordered_map<uint32_t, float> meter_last_volume_levels_;
  std::unordered_map<uint32_t, float> last_observed_peak_levels_;
  std::unordered_map<uint32_t, double> last_peak_change_times_;
  std::unordered_map<uint32_t, std::string> context_names_;
  std::unordered_map<uint64_t, std::string> signature_names_;
  std::unordered_map<uint64_t, uint32_t> signature_display_slots_;
  std::unordered_map<uint64_t, double> signature_last_seen_times_;
  std::vector<uint64_t> signature_slot_order_;
  std::unordered_map<uint32_t, uint64_t> context_last_signatures_;
  std::unordered_map<uint32_t, uint64_t> tracked_signature_hints_;
  std::unordered_map<uint32_t, float> persisted_context_volumes_;
  std::vector<uint32_t> tracked_volume_contexts_;
  std::unordered_set<uint32_t> known_contexts_;
  std::unordered_set<uint32_t> drag_mute_applied_contexts_;
  std::unordered_set<uint32_t> persisted_muted_contexts_;
  std::unordered_set<uint64_t> persisted_muted_signatures_;
  bool drag_mute_active_ = false;
  bool drag_mute_target_ = false;
  bool normalize_volume_panel_ = false;
  bool ignore_persisted_controls_ = false;
  bool show_playing_contexts_only_ = false;
  bool meter_use_audible_ = true;
  bool hover_solo_enabled_ = true;
  bool hover_solo_active_ = false;
  std::optional<uint32_t> hover_solo_context_;
  std::unordered_map<uint32_t, bool> hover_solo_saved_mute_states_;
  std::vector<float> queue_depth_history_;
  std::vector<float> callback_interval_history_ms_;
  uint64_t last_render_callbacks_total_ = 0;
  double last_render_callback_time_ = 0.0;
  double last_metrics_sample_time_ = 0.0;
  double last_metrics_smoothing_time_ = 0.0;
  float render_interval_ema_ms_ = 0.0f;
  float render_deviation_ms_ = 0.0f;
  float smoothed_utilization_percent_ = 0.0f;
  float smoothed_deviation_ms_ = 0.0f;
  float smoothed_latency_ms_ = 0.0f;
  float smoothed_pacing_interval_ms_ = 0.0f;
  float smoothed_vp_total_us_ = 0.0f;
  float smoothed_vp_sweeps_per_second_ = 0.0f;
  float smoothed_vp_decode_iters_per_second_ = 0.0f;
  float smoothed_gp_cycles_ = 0.0f;
  float smoothed_ep_cycles_ = 0.0f;
  bool smoothed_metrics_initialized_ = false;
  bool layout_state_loaded_ = false;
  bool pending_apply_mute_layout_ = false;
  bool layout_dirty_ = false;
  float last_saved_window_width_ = 720.0f;
  float last_saved_window_height_ = 320.0f;
  float pending_window_width_ = 720.0f;
  float pending_window_height_ = 320.0f;
  double last_layout_save_time_ = 0.0;
};

}  // namespace rex::ui
