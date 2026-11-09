/**
 *******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project
 *******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.
 * Released under the BSD license - see LICENSE in the root for more details.
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>

#include <rex/audio/flags.h>
#include <rex/audio/sdl/sdl_audio_driver.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/logging.h>

namespace rex::audio::sdl {

std::unique_ptr<AudioSystem> SDLAudioSystem::Create(
    runtime::FunctionDispatcher* function_dispatcher) {
  return std::make_unique<SDLAudioSystem>(function_dispatcher);
}

SDLAudioSystem::SDLAudioSystem(runtime::FunctionDispatcher* function_dispatcher)
    : AudioSystem(function_dispatcher) {}

SDLAudioSystem::~SDLAudioSystem() {
  if (music_stream_) {
    SDL_DestroyAudioStream(music_stream_);
    music_stream_ = nullptr;
  }

  if (music_sdl_initialized_) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    music_sdl_initialized_ = false;
  }
}

void SDLAudioSystem::Initialize() {
  AudioSystem::Initialize();
}

void SDLAudioSystem::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(music_mutex_);
    if (music_stream_) {
      SDL_DestroyAudioStream(music_stream_);
      music_stream_ = nullptr;
    }
  }

  if (music_sdl_initialized_) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    music_sdl_initialized_ = false;
  }

  AudioSystem::Shutdown();
}

bool SDLAudioSystem::EnsureMusicStream(int sample_rate, int channels) {
  if (!music_sdl_initialized_) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
      REXAPU_ERROR("SDL_InitSubSystem for music failed: {}", SDL_GetError());
      return false;
    }
    music_sdl_initialized_ = true;
  }

  if (!music_stream_) {
    SDL_AudioSpec output_spec = {};
    output_spec.freq = 48000;
    output_spec.format = SDL_AUDIO_F32LE;
    output_spec.channels = 2;

    music_stream_ = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &output_spec, nullptr, nullptr);

    if (!music_stream_) {
      REXAPU_ERROR("SDL_OpenAudioDeviceStream for music failed: {}", SDL_GetError());
      return false;
    }

    auto device = SDL_GetAudioStreamDevice(music_stream_);
    if (!device) {
      REXAPU_ERROR("SDL_GetAudioStreamDevice for music failed: {}", SDL_GetError());
      SDL_DestroyAudioStream(music_stream_);
      music_stream_ = nullptr;
      return false;
    }

    SDL_AudioSpec input_spec = {};
    input_spec.freq = sample_rate;
    input_spec.format = SDL_AUDIO_F32LE;
    input_spec.channels = static_cast<Uint8>(std::clamp(channels, 1, 32));

    if (!SDL_SetAudioStreamFormat(music_stream_, &input_spec, &output_spec)) {
      REXAPU_ERROR("SDL_SetAudioStreamFormat for music failed: {}", SDL_GetError());
      SDL_DestroyAudioStream(music_stream_);
      music_stream_ = nullptr;
      return false;
    }

    if (!SDL_SetAudioStreamGain(music_stream_, music_volume_)) {
      REXAPU_ERROR("SDL_SetAudioStreamGain for music failed: {}", SDL_GetError());
      SDL_DestroyAudioStream(music_stream_);
      music_stream_ = nullptr;
      return false;
    }

    if (!SDL_ResumeAudioDevice(device)) {
      REXAPU_ERROR("SDL_ResumeAudioDevice for music failed: {}", SDL_GetError());
      SDL_DestroyAudioStream(music_stream_);
      music_stream_ = nullptr;
      return false;
    }
  } else {
    SDL_AudioSpec input_spec = {};
    input_spec.freq = sample_rate;
    input_spec.format = SDL_AUDIO_F32LE;
    input_spec.channels = static_cast<Uint8>(std::clamp(channels, 1, 32));

    SDL_AudioSpec output_spec = {};
    output_spec.freq = 48000;
    output_spec.format = SDL_AUDIO_F32LE;
    output_spec.channels = 2;

    if (!SDL_SetAudioStreamFormat(music_stream_, &input_spec, &output_spec)) {
      REXAPU_ERROR("SDL_SetAudioStreamFormat for music failed: {}", SDL_GetError());
      return false;
    }

    if (!SDL_SetAudioStreamGain(music_stream_, music_volume_)) {
      REXAPU_ERROR("SDL_SetAudioStreamGain for music failed: {}", SDL_GetError());
      return false;
    }
  }

  return true;
}

bool SDLAudioSystem::PlayMusicPcm(std::span<const float> samples, int sample_rate,
                                  int channels) {
  if (samples.empty() || sample_rate <= 0 || channels <= 0) {
    return false;
  }

  std::lock_guard<std::mutex> lock(music_mutex_);

  if (!EnsureMusicStream(sample_rate, channels)) {
    return false;
  }

  SDL_ClearAudioStream(music_stream_);

  const auto byte_count = static_cast<int>(samples.size_bytes());
  if (!SDL_PutAudioStreamData(music_stream_, samples.data(), byte_count)) {
    REXAPU_ERROR("SDL_PutAudioStreamData for music failed: {}", SDL_GetError());
    return false;
  }

  REXAPU_INFO("Queued {} PCM samples for music ({} Hz, {} channels)",
              samples.size(), sample_rate, channels);
  return true;
}

void SDLAudioSystem::StopMusic() {
  std::lock_guard<std::mutex> lock(music_mutex_);

  if (music_stream_) {
    SDL_ClearAudioStream(music_stream_);
    REXAPU_INFO("Stopped music playback");
  }
}
void SDLAudioSystem::PauseMusic() {
  std::lock_guard<std::mutex> lock(music_mutex_);

  if (music_stream_) {
    auto device = SDL_GetAudioStreamDevice(music_stream_);
    if (device) {
      SDL_PauseAudioDevice(device);
      REXAPU_INFO("Paused music playback");
    }
  }
}

void SDLAudioSystem::ResumeMusic() {
  std::lock_guard<std::mutex> lock(music_mutex_);

  if (music_stream_) {
    auto device = SDL_GetAudioStreamDevice(music_stream_);
    if (device) {
      SDL_ResumeAudioDevice(device);
      REXAPU_INFO("Resumed music playback");
    }
  }
}
int64_t SDLAudioSystem::MusicQueuedBytes() {
  std::lock_guard<std::mutex> lock(music_mutex_);
  return music_stream_ ? SDL_GetAudioStreamQueued(music_stream_) : 0;
}

void SDLAudioSystem::SetMusicVolume(float volume) {
  std::lock_guard<std::mutex> lock(music_mutex_);

  music_volume_ = std::clamp(volume, 0.0f, 1.0f);

  if (music_stream_) {
    if (!SDL_SetAudioStreamGain(music_stream_, music_volume_)) {
      REXAPU_ERROR("SDL_SetAudioStreamGain for music failed: {}", SDL_GetError());
    }
  }

  REXAPU_INFO("Music volume set to {}", music_volume_);
}
X_STATUS SDLAudioSystem::CreateDriver([[maybe_unused]] size_t index,
                                      rex::thread::Semaphore* semaphore,
                                      AudioDriver** out_driver) {
  assert_not_null(out_driver);
  auto driver = new SDLAudioDriver(memory_, semaphore);
  if (!driver->Initialize()) {
    driver->Shutdown();
    delete driver;
    return X_STATUS_UNSUCCESSFUL;
  }

  *out_driver = driver;
  return X_STATUS_SUCCESS;
}

void SDLAudioSystem::DestroyDriver(AudioDriver* driver) {
  assert_not_null(driver);
  auto sdldriver = dynamic_cast<SDLAudioDriver*>(driver);
  assert_not_null(sdldriver);
  sdldriver->Shutdown();
  delete sdldriver;
}

}  // namespace rex::audio::sdl

