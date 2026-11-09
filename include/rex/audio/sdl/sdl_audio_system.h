/**
 *******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project
 *******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.
 * Released under the BSD license - see LICENSE in the root for more details.
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <mutex>

#include <rex/audio/audio_system.h>
#include <SDL3/SDL.h>

namespace rex::audio::sdl {

class SDLAudioSystem : public AudioSystem {
 public:
  explicit SDLAudioSystem(runtime::FunctionDispatcher* function_dispatcher);
  ~SDLAudioSystem() override;

  static bool IsAvailable() { return true; }

  static std::unique_ptr<AudioSystem> Create(runtime::FunctionDispatcher* function_dispatcher);

  X_STATUS CreateDriver(size_t index, rex::thread::Semaphore* semaphore,
                        AudioDriver** out_driver) override;
  void DestroyDriver(AudioDriver* driver) override;

  bool PlayMusicPcm(std::span<const float> samples, int sample_rate,
                    int channels) override;
  void StopMusic() override;
  void PauseMusic() override;
  void ResumeMusic() override;
  void SetMusicVolume(float volume) override;
  int64_t MusicQueuedBytes() override;

 protected:
  void Initialize() override;
  void Shutdown() override;

 private:
  bool EnsureMusicStream(int sample_rate, int channels);

  SDL_AudioStream* music_stream_ = nullptr;
  bool music_sdl_initialized_ = false;
  float music_volume_ = 1.0f;
  std::mutex music_mutex_;
};

}  // namespace rex::audio::sdl

