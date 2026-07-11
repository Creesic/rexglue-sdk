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

#include <rex/audio/xaudio2/xaudio2_audio_driver.h>
#include <rex/audio/xaudio2/xaudio2_audio_system.h>

namespace rex::audio::xaudio2 {

std::unique_ptr<AudioSystem> XAudio2AudioSystem::Create(
    runtime::FunctionDispatcher* function_dispatcher) {
  return std::make_unique<XAudio2AudioSystem>(function_dispatcher);
}

XAudio2AudioSystem::XAudio2AudioSystem(runtime::FunctionDispatcher* function_dispatcher)
    : AudioSystem(function_dispatcher) {}

XAudio2AudioSystem::~XAudio2AudioSystem() = default;

void XAudio2AudioSystem::Initialize() { AudioSystem::Initialize(); }

X_STATUS XAudio2AudioSystem::CreateDriver([[maybe_unused]] size_t index,
                                          rex::thread::Semaphore* semaphore,
                                          AudioDriver** out_driver) {
  assert_not_null(out_driver);
  auto driver = new XAudio2AudioDriver(memory_, semaphore);
  if (!driver->Initialize()) {
    driver->Shutdown();
    delete driver;
    return X_STATUS_UNSUCCESSFUL;
  }

  *out_driver = driver;
  return X_STATUS_SUCCESS;
}

void XAudio2AudioSystem::DestroyDriver(AudioDriver* driver) {
  assert_not_null(driver);
  auto xdriver = dynamic_cast<XAudio2AudioDriver*>(driver);
  assert_not_null(xdriver);
  xdriver->Shutdown();
  delete xdriver;
}

}  // namespace rex::audio::xaudio2
