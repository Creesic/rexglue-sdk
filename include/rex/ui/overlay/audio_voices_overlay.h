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
  uint8_t current_buffer = 0;
  uint8_t subframe_decode_count = 0;
  uint8_t output_buffer_block_count = 0;
  uint8_t output_buffer_write_offset = 0;
  uint8_t output_buffer_read_offset = 0;
  uint8_t sample_rate_id = 0;
  uint8_t loop_count = 0;
  uint32_t guest_ptr = 0;
  uint32_t input_buffer_read_offset = 0;
  uint32_t input_buffer_0_ptr = 0;
  uint32_t input_buffer_1_ptr = 0;
  uint32_t output_buffer_ptr = 0;
};

struct AudioVoicesSnapshot {
  bool valid = false;
  bool paused = false;
  uint32_t queued_frames = 0;
  bool xma_paused = false;
  std::vector<AudioVoiceInfo> voices;
  std::vector<AudioXmaContextInfo> xma_contexts;
};

struct AudioVoicesLayoutState {
  bool has_window_size = false;
  float window_width = 0.0f;
  float window_height = 0.0f;
  bool normalize_volume_panel = false;
  std::unordered_set<uint32_t> muted_contexts;
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
  std::unordered_map<uint32_t, std::string> context_names_;
  std::unordered_map<uint32_t, float> persisted_context_volumes_;
  std::vector<uint32_t> tracked_volume_contexts_;
  std::unordered_set<uint32_t> known_contexts_;
  std::unordered_set<uint32_t> drag_mute_applied_contexts_;
  std::unordered_set<uint32_t> persisted_muted_contexts_;
  bool drag_mute_active_ = false;
  bool drag_mute_target_ = false;
  bool normalize_volume_panel_ = false;
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
