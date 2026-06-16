/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <atomic>
#include <array>
#include <mutex>
#include <queue>

#include <rex/audio/xma/context.h>
#include <rex/audio/xma/register_file.h>
#include <rex/bit.h>
#include <rex/kernel.h>
#include <rex/system/xthread.h>

namespace rex::runtime {
class FunctionDispatcher;
}  // namespace rex::runtime

namespace rex::audio {

struct XMA_CONTEXT_DATA;

class XmaDecoder {
 public:
  static constexpr uint32_t kContextCount = 320;

  struct DebugContextInfo {
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
    float rms_ch0_level = 0.0f;
    float rms_ch1_level = 0.0f;
    float audible_peak_level = 0.0f;
    float audible_rms_level = 0.0f;
    float audible_rms_ch0_level = 0.0f;
    float audible_rms_ch1_level = 0.0f;
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

  struct DebugVpWorkerInfo {
    uint32_t num_voices = 0;
    uint32_t time_us = 0;
  };

  struct DebugVpInfo {
    uint32_t total_worker_time_us = 0;
    uint32_t sweeps_per_second = 0;
    uint32_t decode_iterations_per_second = 0;
    std::array<DebugVpWorkerInfo, 1> workers = {};
  };

  struct DebugDspInfo {
    uint32_t cycles = 0;
  };

  struct DebugSnapshot {
    bool paused = false;
    DebugVpInfo vp = {};
    DebugDspInfo gp = {};
    DebugDspInfo ep = {};
    std::array<DebugContextInfo, kContextCount> contexts = {};
  };

  explicit XmaDecoder(runtime::FunctionDispatcher* function_dispatcher);
  ~XmaDecoder();

  memory::Memory* memory() const { return memory_; }
  runtime::FunctionDispatcher* function_dispatcher() const { return function_dispatcher_; }

  X_STATUS Setup(system::KernelState* kernel_state);
  void Shutdown();

  uint32_t context_array_ptr() const { return register_file_[XmaRegister::ContextArrayAddress]; }

  uint32_t AllocateContext();
  void ReleaseContext(uint32_t guest_ptr);
  bool BlockOnContext(uint32_t guest_ptr, bool poll);

  uint32_t ReadRegister(uint32_t addr);
  void WriteRegister(uint32_t addr, uint32_t value);

  bool is_paused() const { return paused_; }
  void Pause();
  void Resume();

  DebugSnapshot GetDebugSnapshot();
  void ToggleContextMute(uint32_t context_id);
  void SetContextMuted(uint32_t context_id, bool muted);
  void SetContextVolume(uint32_t context_id, float volume);

 protected:
  int GetContextId(uint32_t guest_ptr);

 private:
  void WorkerThreadMain();

  static uint32_t MMIOReadRegisterThunk(void* ppc_context, XmaDecoder* as, uint32_t addr) {
    return as->ReadRegister(addr);
  }
  static void MMIOWriteRegisterThunk(void* ppc_context, XmaDecoder* as, uint32_t addr,
                                     uint32_t value) {
    as->WriteRegister(addr, value);
  }

 protected:
  memory::Memory* memory_ = nullptr;
  runtime::FunctionDispatcher* function_dispatcher_ = nullptr;

  std::atomic<bool> worker_running_ = {false};
  system::object_ref<system::XHostThread> worker_thread_;
  std::unique_ptr<rex::thread::Event> work_event_ = nullptr;

  std::atomic<bool> paused_ = false;
  rex::thread::Fence pause_fence_;   // Signaled when worker paused.
  rex::thread::Fence resume_fence_;  // Signaled when resume requested.

  XmaRegisterFile register_file_;

  XmaContext contexts_[kContextCount];
  bit::BitMap context_bitmap_;

  uint32_t context_data_first_ptr_ = 0;
  uint32_t context_data_last_ptr_ = 0;

  std::atomic<uint32_t> debug_vp_total_worker_time_us_ = {0};
  std::atomic<uint32_t> debug_vp_worker_voices_ = {0};
  std::atomic<uint32_t> debug_vp_sweeps_per_second_ = {0};
  std::atomic<uint32_t> debug_vp_decode_iterations_per_second_ = {0};
  std::atomic<uint32_t> debug_gp_cycles_estimate_ = {0};
  std::atomic<uint32_t> debug_ep_cycles_estimate_ = {0};
};

}  // namespace rex::audio
