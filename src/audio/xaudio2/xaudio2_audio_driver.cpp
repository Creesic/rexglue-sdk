/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cstdint>
#include <cstring>

#include <rex/assert.h>
#include <rex/audio/conversion.h>
#include <rex/audio/flags.h>
#include <rex/audio/xaudio2/xaudio2_audio_driver.h>
#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/logging.h>

#if REX_PLATFORM_WIN32
#include <ks.h>
#include <ksmedia.h>
#include <mmreg.h>
#endif

namespace rex::audio::xaudio2 {

REXCVAR_DEFINE_BOOL(audio_force_stereo_mix, true, "Audio",
                    "Force XAudio2 output to stereo by downmixing 6-channel guest frames");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_fl, 1.0, "Audio",
                      "Stereo downmix coefficient for front-left channel");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_fr, 1.0, "Audio",
                      "Stereo downmix coefficient for front-right channel");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_fc, 0.5, "Audio",
                      "Stereo downmix coefficient for center channel (applied to L and R)");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_bl, 1.0, "Audio",
                      "Stereo downmix coefficient for back-left channel");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_br, 1.0, "Audio",
                      "Stereo downmix coefficient for back-right channel");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_lfe, 0.0, "Audio",
                      "Stereo downmix coefficient for LFE channel (applied to L and R)");
REXCVAR_DEFINE_DOUBLE(audio_stereo_downmix_norm, 2.5, "Audio",
                      "Stereo downmix normalization divisor (set <= 0 to disable normalization)");

namespace {

inline float LoadBigEndianFloat(const float* input, size_t index) {
  const uint32_t raw_be = reinterpret_cast<const uint32_t*>(input)[index];
  const uint32_t raw_le = rex::byte_swap(raw_be);
  float value = 0.0f;
  std::memcpy(&value, &raw_le, sizeof(value));
  return value;
}

}  // namespace

#if REX_PLATFORM_WIN32
class XAudio2AudioDriver::VoiceCallback final : public IXAudio2VoiceCallback {
 public:
  explicit VoiceCallback(rex::thread::Semaphore* semaphore) : semaphore_(semaphore) {}
  ~VoiceCallback() = default;

  void OnStreamEnd() noexcept override {}
  void OnVoiceProcessingPassEnd() noexcept override {}
  void OnVoiceProcessingPassStart([[maybe_unused]] uint32_t samples_required) noexcept override {}
  void OnBufferEnd([[maybe_unused]] void* context) noexcept override {
    auto ret = semaphore_->Release(1, nullptr);
    assert_true(ret);
  }
  void OnBufferStart([[maybe_unused]] void* context) noexcept override {}
  void OnLoopEnd([[maybe_unused]] void* context) noexcept override {}
  void OnVoiceError([[maybe_unused]] void* context, [[maybe_unused]] HRESULT result) noexcept override {}

 private:
  rex::thread::Semaphore* semaphore_ = nullptr;
};
#endif

XAudio2AudioDriver::XAudio2AudioDriver(rex::memory::Memory* memory,
                                       rex::thread::Semaphore* semaphore,
                                       uint32_t frequency, uint32_t channels,
                                       bool need_format_conversion)
    : AudioDriver(memory),
      semaphore_(semaphore),
      frame_frequency_(frequency),
      frame_channels_(channels),
      need_format_conversion_(need_format_conversion) {
#if REX_PLATFORM_WIN32
  output_channels_ = frame_channels_;
  if (need_format_conversion_ && frame_channels_ == 6 && REXCVAR_GET(audio_force_stereo_mix)) {
    output_channels_ = 2;
  }

  switch (frame_channels_) {
    case 6:
      channel_samples_ = 256;
      break;
    case 2:
      channel_samples_ = 768;
      break;
    default:
      assert_unhandled_case(frame_channels_);
      break;
  }
  frame_size_ = sizeof(float) * output_channels_ * channel_samples_;
  assert_true(frame_channels_ == 2 || frame_channels_ == 6);
  assert_true(output_channels_ == 2 || output_channels_ == 6);
  assert_true(frame_size_ <= sizeof(frames_[0]));
  assert_true(!need_format_conversion_ || frame_channels_ == 6);
#endif
}

XAudio2AudioDriver::~XAudio2AudioDriver() = default;

bool XAudio2AudioDriver::Initialize() {
#if !REX_PLATFORM_WIN32
  return false;
#else
  REXAPU_INFO("XAudio2AudioDriver init: {} Hz, in_ch={}, out_ch={}, frame_size={} bytes",
              frame_frequency_, frame_channels_, output_channels_, frame_size_);
  voice_callback_ = new VoiceCallback(semaphore_);

  xaudio2_module_ = LoadLibraryW(L"XAudio2_9.dll");
  if (!xaudio2_module_) {
    xaudio2_module_ = LoadLibraryW(L"XAudio2_8.dll");
  }
  if (!xaudio2_module_) {
    REXAPU_ERROR("Failed to load XAudio2 runtime DLL (tried XAudio2_9.dll, XAudio2_8.dll)");
    return false;
  }

  xaudio2_create_ = reinterpret_cast<XAudio2CreateFn>(
      GetProcAddress(xaudio2_module_, "XAudio2Create"));
  if (!xaudio2_create_) {
    REXAPU_ERROR("XAudio2Create not found in loaded XAudio2 runtime DLL");
    FreeLibrary(xaudio2_module_);
    xaudio2_module_ = nullptr;
    return false;
  }

  assert_false(mta_thread_.joinable());
  mta_thread_initialization_attempt_completed_ = false;
  mta_thread_shutdown_requested_ = false;
  mta_thread_ = std::thread(&XAudio2AudioDriver::MTAThread, this);

  {
    std::unique_lock<std::mutex> lock(mta_thread_initialization_completion_mutex_);
    while (!mta_thread_initialization_attempt_completed_) {
      mta_thread_initialization_completion_cond_.wait(lock);
    }
  }

  if (!mta_thread_initialization_completion_result_) {
    if (mta_thread_.joinable()) {
      mta_thread_.join();
    }
    REXAPU_ERROR("XAudio2AudioDriver init failed during MTA thread setup");
    return false;
  }

  REXAPU_INFO("XAudio2AudioDriver init complete");
  return true;
#endif
}

void XAudio2AudioDriver::SubmitFrame(uint32_t frame_ptr) {
#if !REX_PLATFORM_WIN32
  (void)frame_ptr;
  return;
#else
  if (!pcm_voice_) {
    return;
  }

  XAUDIO2_VOICE_STATE state = {};
  pcm_voice_->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
  if (state.BuffersQueued >= kFrameCount) {
    return;
  }

  auto output_frame = reinterpret_cast<float*>(frames_[current_frame_]);
  const auto input_frame = memory_->TranslateVirtual<float*>(frame_ptr);

  if (need_format_conversion_) {
    if (output_channels_ == 2) {
      const float w_fl = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_fl));
      const float w_fr = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_fr));
      const float w_fc = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_fc));
      const float w_bl = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_bl));
      const float w_br = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_br));
      const float w_lfe = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_lfe));
      const float norm = static_cast<float>(REXCVAR_GET(audio_stereo_downmix_norm));
      const float inv_norm = norm > 0.0001f ? (1.0f / norm) : 1.0f;

      for (uint32_t s = 0; s < channel_samples_; ++s) {
        const float fl = LoadBigEndianFloat(input_frame, 0 * channel_samples_ + s);
        const float fr = LoadBigEndianFloat(input_frame, 1 * channel_samples_ + s);
        const float fc = LoadBigEndianFloat(input_frame, 2 * channel_samples_ + s);
        const float lfe = LoadBigEndianFloat(input_frame, 3 * channel_samples_ + s);
        const float bl = LoadBigEndianFloat(input_frame, 4 * channel_samples_ + s);
        const float br = LoadBigEndianFloat(input_frame, 5 * channel_samples_ + s);

        const float left_raw = (fl * w_fl) + (fc * w_fc) + (bl * w_bl) + (lfe * w_lfe);
        const float right_raw = (fr * w_fr) + (fc * w_fc) + (br * w_br) + (lfe * w_lfe);
        output_frame[s * 2 + 0] = left_raw * inv_norm;
        output_frame[s * 2 + 1] = right_raw * inv_norm;
      }
    } else {
      conversion::sequential_6_BE_to_interleaved_6_LE(output_frame, input_frame, channel_samples_);
    }
  } else {
    std::memcpy(output_frame, input_frame, frame_size_);
  }

  XAUDIO2_BUFFER buffer = {};
  buffer.Flags = 0;
  buffer.pAudioData = reinterpret_cast<const BYTE*>(output_frame);
  buffer.AudioBytes = frame_size_;
  buffer.PlayBegin = 0;
  buffer.PlayLength = channel_samples_;
  buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
  buffer.LoopLength = 0;
  buffer.LoopCount = 0;
  buffer.pContext = nullptr;

  auto hr = pcm_voice_->SubmitSourceBuffer(&buffer);
  if (FAILED(hr)) {
    REXAPU_ERROR("XAudio2 SubmitSourceBuffer failed with 0x{:08X}", static_cast<uint32_t>(hr));
    return;
  }

  current_frame_ = (current_frame_ + 1) % kFrameCount;

  float frequency_ratio = static_cast<float>(rex::chrono::Clock::guest_time_scalar());
  pcm_voice_->SetFrequencyRatio(frequency_ratio);
#endif
}

void XAudio2AudioDriver::Pause() {
#if REX_PLATFORM_WIN32
  if (pcm_voice_) {
    pcm_voice_->Stop();
  }
#endif
}

void XAudio2AudioDriver::Resume() {
#if REX_PLATFORM_WIN32
  if (pcm_voice_) {
    pcm_voice_->Start();
  }
#endif
}

void XAudio2AudioDriver::SetVolume(float volume) {
#if REX_PLATFORM_WIN32
  if (REXCVAR_GET(audio_mute) || !pcm_voice_) {
    return;
  }
  pcm_voice_->SetVolume(volume);
#else
  (void)volume;
#endif
}

void XAudio2AudioDriver::Shutdown() {
#if !REX_PLATFORM_WIN32
  return;
#else
  if (mta_thread_.joinable()) {
    {
      std::unique_lock<std::mutex> lock(mta_thread_shutdown_request_mutex_);
      mta_thread_shutdown_requested_ = true;
    }
    mta_thread_shutdown_request_cond_.notify_all();
    mta_thread_.join();
  }

  xaudio2_create_ = nullptr;
  if (xaudio2_module_) {
    FreeLibrary(xaudio2_module_);
    xaudio2_module_ = nullptr;
  }

  if (voice_callback_) {
    delete voice_callback_;
    voice_callback_ = nullptr;
  }
#endif
}

bool XAudio2AudioDriver::InitializeObjects() {
#if !REX_PLATFORM_WIN32
  return false;
#else
  HRESULT hr;

  XAUDIO2_DEBUG_CONFIGURATION config = {};
  config.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
  config.BreakMask = 0;
  config.LogThreadID = FALSE;
  config.LogTiming = TRUE;
  config.LogFunctionName = TRUE;
  config.LogFileline = TRUE;

  audio_->SetDebugConfiguration(&config);

  hr = audio_->CreateMasteringVoice(&mastering_voice_);
  if (FAILED(hr)) {
    REXAPU_ERROR("IXAudio2::CreateMasteringVoice failed with 0x{:08X}",
                 static_cast<uint32_t>(hr));
    ShutdownObjects();
    return false;
  }

  WAVEFORMATEXTENSIBLE waveformat = {};
  waveformat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  waveformat.Format.nChannels = static_cast<WORD>(output_channels_);
  waveformat.Format.nSamplesPerSec = frame_frequency_;
  waveformat.Format.wBitsPerSample = 32;
  waveformat.Format.nBlockAlign =
      (waveformat.Format.nChannels * waveformat.Format.wBitsPerSample) / 8;
  waveformat.Format.nAvgBytesPerSec =
      waveformat.Format.nSamplesPerSec * waveformat.Format.nBlockAlign;
  waveformat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
  waveformat.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
  waveformat.Samples.wValidBitsPerSample = waveformat.Format.wBitsPerSample;
  static constexpr DWORD kChannelMasks[] = {
      0,
      0,
      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
      0,
      0,
      0,
      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
          SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
      0,
  };
  waveformat.dwChannelMask = kChannelMasks[waveformat.Format.nChannels];

  hr = audio_->CreateSourceVoice(&pcm_voice_, &waveformat.Format, 0, XAUDIO2_MAX_FREQ_RATIO,
                                 voice_callback_);
  if (FAILED(hr)) {
    REXAPU_ERROR("IXAudio2::CreateSourceVoice failed with 0x{:08X}", static_cast<uint32_t>(hr));
    ShutdownObjects();
    return false;
  }

  hr = pcm_voice_->Start();
  if (FAILED(hr)) {
    REXAPU_ERROR("IXAudio2SourceVoice::Start failed with 0x{:08X}", static_cast<uint32_t>(hr));
    ShutdownObjects();
    return false;
  }

  if (REXCVAR_GET(audio_mute)) {
    pcm_voice_->SetVolume(0.0f);
  }

  REXAPU_INFO("XAudio2 source voice ready: {} Hz, in_ch={}, out_ch={}, samples_per_frame={}",
              frame_frequency_, frame_channels_, output_channels_, channel_samples_);
  return true;
#endif
}

void XAudio2AudioDriver::ShutdownObjects() {
#if REX_PLATFORM_WIN32
  if (audio_) {
    audio_->StopEngine();
  }

  if (pcm_voice_) {
    pcm_voice_->DestroyVoice();
    pcm_voice_ = nullptr;
  }

  if (mastering_voice_) {
    mastering_voice_->DestroyVoice();
    mastering_voice_ = nullptr;
  }

  if (audio_) {
    audio_->Release();
    audio_ = nullptr;
  }
#endif
}

void XAudio2AudioDriver::MTAThread() {
#if REX_PLATFORM_WIN32
  bool initialized = false;

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool com_initialized = SUCCEEDED(hr);
  if (!com_initialized) {
    REXAPU_ERROR("XAudio2 MTA thread CoInitializeEx failed with 0x{:08X}",
                 static_cast<uint32_t>(hr));
  } else {
    hr = xaudio2_create_(&audio_, 0, kProcessor);
    if (FAILED(hr)) {
      REXAPU_ERROR("XAudio2Create failed with 0x{:08X}", static_cast<uint32_t>(hr));
    } else {
      initialized = InitializeObjects();
    }
  }

  mta_thread_initialization_completion_result_ = initialized;
  {
    std::unique_lock<std::mutex> lock(mta_thread_initialization_completion_mutex_);
    mta_thread_initialization_attempt_completed_ = true;
  }
  mta_thread_initialization_completion_cond_.notify_all();

  if (initialized) {
    std::unique_lock<std::mutex> lock(mta_thread_shutdown_request_mutex_);
    while (!mta_thread_shutdown_requested_) {
      mta_thread_shutdown_request_cond_.wait(lock);
    }
    lock.unlock();

    ShutdownObjects();
  }

  if (com_initialized) {
    CoUninitialize();
  }
#endif
}

}  // namespace rex::audio::xaudio2
