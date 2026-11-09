/**
 * @file        system/interfaces/audio.h
 * @brief       Abstract audio system interface for dependency injection
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <rex/system/xtypes.h>
#include <span>

namespace rex::system {
class KernelState;
}

namespace rex::system {

class IAudioSystem {
 public:
  virtual ~IAudioSystem() = default;
  virtual X_STATUS Setup(KernelState* kernel_state) = 0;
  virtual void Shutdown() = 0;

  virtual bool PlayMusicPcm(std::span<const float> samples,
                            int sample_rate,
                            int channels) = 0;
  virtual void StopMusic() = 0;
  virtual void PauseMusic() = 0;
  virtual void ResumeMusic() = 0;
  virtual void SetMusicVolume(float volume) = 0;
  // Bytes of music still queued for output, or -1 if the backend can't tell.
  // (Near) 0 while a track was playing means it has finished.
  virtual int64_t MusicQueuedBytes() { return -1; }
};

}  // namespace rex::system

