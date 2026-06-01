/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include <rex/audio/audio_driver.h>
#include <rex/platform.h>
#include <rex/thread.h>

#if REX_PLATFORM_WIN32
#include <xaudio2.h>
#endif

namespace rex::audio::xaudio2 {

class XAudio2AudioDriver : public AudioDriver {
 public:
  XAudio2AudioDriver(rex::memory::Memory* memory, rex::thread::Semaphore* semaphore,
                     uint32_t frequency = 48000, uint32_t channels = 6,
                     bool need_format_conversion = true);
  ~XAudio2AudioDriver() override;

  bool Initialize();
  void SubmitFrame(uint32_t frame_ptr) override;
  void Pause();
  void Resume();
  void SetVolume(float volume);
  void Shutdown();

 private:
  static constexpr uint32_t kFrameSamplesMax = 6 * 256;
  static constexpr uint32_t kFrameCount = 64;
#if REX_PLATFORM_WIN32
  static constexpr XAUDIO2_PROCESSOR kProcessor = 0x00000001;

  class VoiceCallback;
  using XAudio2CreateFn = HRESULT(WINAPI*)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

  bool InitializeObjects();
  void ShutdownObjects();
  void MTAThread();

  HMODULE xaudio2_module_ = nullptr;
  XAudio2CreateFn xaudio2_create_ = nullptr;

  bool mta_thread_initialization_completion_result_ = false;
  std::mutex mta_thread_initialization_completion_mutex_;
  std::condition_variable mta_thread_initialization_completion_cond_;
  bool mta_thread_initialization_attempt_completed_ = false;

  std::mutex mta_thread_shutdown_request_mutex_;
  std::condition_variable mta_thread_shutdown_request_cond_;
  bool mta_thread_shutdown_requested_ = false;

  std::thread mta_thread_;

  IXAudio2* audio_ = nullptr;
  IXAudio2MasteringVoice* mastering_voice_ = nullptr;
  IXAudio2SourceVoice* pcm_voice_ = nullptr;
  VoiceCallback* voice_callback_ = nullptr;

#endif

  rex::thread::Semaphore* semaphore_ = nullptr;

  uint32_t frame_frequency_ = 48000;
  uint32_t frame_channels_ = 6;
  uint32_t output_channels_ = 6;
  uint32_t channel_samples_ = 256;
  uint32_t frame_size_ = sizeof(float) * 6 * 256;
  bool need_format_conversion_ = true;

  float frames_[kFrameCount][kFrameSamplesMax] = {};
  uint32_t current_frame_ = 0;
};

}  // namespace rex::audio::xaudio2
