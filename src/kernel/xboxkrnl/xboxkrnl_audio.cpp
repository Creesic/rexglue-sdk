/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

// Disable warnings about unused parameters for kernel functions
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include <rex/audio/audio_system.h>
#include <rex/chrono/clock.h>
#include <rex/kernel/xboxkrnl/private.h>
#include <rex/logging.h>
#include <rex/hook.h>
#include <rex/types.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xtypes.h>

namespace {

constexpr uint32_t kDefaultSpeakerConfig = 0x00010001;
constexpr uint32_t kMaxTrackedVoiceCategories = 32;

std::array<float, kMaxTrackedVoiceCategories> g_voice_category_volumes = []() {
  std::array<float, kMaxTrackedVoiceCategories> values{};
  values.fill(1.0f);
  return values;
}();
uint32_t g_voice_category_change_mask = 0;

uint32_t g_speaker_config = kDefaultSpeakerConfig;
bool g_speaker_config_overridden = false;
uint32_t g_override_speaker_config = kDefaultSpeakerConfig;

uint32_t GetAudioCallerAddress() {
  uint32_t caller = 0;
  auto* thread = rex::system::XThread::GetCurrentThread();
  if (thread && thread->thread_state() && thread->thread_state()->context()) {
    caller = static_cast<uint32_t>(thread->thread_state()->context()->lr);
  }
  return caller;
}

float DecodeGuestFloat(uint32_t bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

}  // namespace

namespace rex::kernel::xboxkrnl {
using namespace rex::system;

u32 XAudioGetSpeakerConfig_entry(mapped_u32 config_ptr) {
  const uint32_t effective_config =
      g_speaker_config_overridden ? g_override_speaker_config : g_speaker_config;
  *config_ptr = effective_config;

  static uint32_t get_speaker_log_count = 0;
  if (get_speaker_log_count < 80 || (get_speaker_log_count % 200) == 0) {
    REXKRNL_ERROR(
        "XAUDIO_DIAG GetSpeakerConfig caller={:08X} config={:08X} base={:08X} override={} "
        "override_cfg={:08X}",
        GetAudioCallerAddress(), effective_config, g_speaker_config, g_speaker_config_overridden ? 1 : 0,
        g_override_speaker_config);
  }
  ++get_speaker_log_count;
  return X_ERROR_SUCCESS;
}

u32 XAudioGetVoiceCategoryVolumeChangeMask_entry(mapped_void driver_ptr, mapped_u32 out_ptr) {
  assert_true((driver_ptr.guest_address() & 0xFFFF0000) == 0x41550000);

  rex::thread::Sleep(std::chrono::microseconds(1));

  // Checking these bits to see if any voice volume changed.
  // I think.
  const uint32_t mask_value = g_voice_category_change_mask;
  *out_ptr = mask_value;
  g_voice_category_change_mask = 0;

  static uint32_t get_mask_log_count = 0;
  if (mask_value != 0 || get_mask_log_count < 80 || (get_mask_log_count % 200) == 0) {
    REXKRNL_ERROR("XAUDIO_DIAG GetVoiceCategoryVolumeChangeMask caller={:08x} driver={:08x} "
                  "mask={:08x}",
                  GetAudioCallerAddress(), driver_ptr.guest_address(), mask_value);
  }
  ++get_mask_log_count;
  return X_ERROR_SUCCESS;
}

u32 XAudioGetVoiceCategoryVolume_entry(u32 category, mapped_f32 out_ptr) {
  // Expects a floating point single. Volume %?
  const float value =
      category < g_voice_category_volumes.size() ? g_voice_category_volumes[category] : 1.0f;
  *out_ptr = value;

  static uint32_t get_vol_log_count = 0;
  if (get_vol_log_count < 120 || std::fabs(value - 1.0f) > 0.001f ||
      (get_vol_log_count % 200) == 0) {
    REXKRNL_ERROR("XAUDIO_DIAG GetVoiceCategoryVolume caller={:08X} category={} value={:.4f}",
                  GetAudioCallerAddress(), category, value);
  }
  ++get_vol_log_count;

  return X_ERROR_SUCCESS;
}

u32 XAudioEnableDucker_entry(u32 unk) {
  static uint32_t enable_ducker_log_count = 0;
  if (enable_ducker_log_count < 40 || (enable_ducker_log_count % 200) == 0) {
    REXKRNL_ERROR("XAUDIO_DIAG EnableDucker caller={:08X} arg={:08X}", GetAudioCallerAddress(),
                  unk);
  }
  ++enable_ducker_log_count;
  return X_ERROR_SUCCESS;
}

u32 XAudioRegisterRenderDriverClient_entry(mapped_u32 callback_ptr, mapped_u32 driver_ptr) {
  if (!callback_ptr) {
    return X_E_INVALIDARG;
  }

  uint32_t callback = callback_ptr[0];

  if (!callback) {
    return X_E_INVALIDARG;
  }
  uint32_t callback_arg = callback_ptr[1];

  REXKRNL_ERROR("XAudioRegisterRenderDriverClient: callback={:08X} arg={:08X}",
                callback, callback_arg);

  auto* audio_system =
      static_cast<audio::AudioSystem*>(REX_KERNEL_STATE()->emulator()->audio_system());

  size_t index;
  auto result = audio_system->RegisterClient(callback, callback_arg, &index);
  if (XFAILED(result)) {
    REXKRNL_ERROR("XAudioRegisterRenderDriverClient: RegisterClient failed -> {:#x}", result);
    return result;
  }

  assert_true(!(index & ~0x0000FFFF));
  const uint32_t driver_handle = 0x41550000 | (static_cast<uint32_t>(index) & 0x0000FFFF);
  *driver_ptr = driver_handle;
  REXKRNL_ERROR("XAudioRegisterRenderDriverClient: success driver={:#x} index={}", driver_handle,
                index);
  return X_ERROR_SUCCESS;
}

u32 XAudioUnregisterRenderDriverClient_entry(mapped_void driver_ptr) {
  assert_true((driver_ptr.guest_address() & 0xFFFF0000) == 0x41550000);

  auto* audio_system =
      static_cast<audio::AudioSystem*>(REX_KERNEL_STATE()->emulator()->audio_system());
  audio_system->UnregisterClient(driver_ptr.guest_address() & 0x0000FFFF);
  return X_ERROR_SUCCESS;
}

u32 XAudioSubmitRenderDriverFrame_entry(mapped_void driver_ptr, mapped_void samples_ptr) {
  assert_true((driver_ptr.guest_address() & 0xFFFF0000) == 0x41550000);

  static uint32_t submit_krnl_count = 0;
  if (submit_krnl_count < 30 || (submit_krnl_count % 500) == 0) {
    REXKRNL_ERROR("XAUDIO_TRACE SubmitRenderDriverFrame #{}: driver={:08X} samples={:08X} caller={:08X}",
                  submit_krnl_count, driver_ptr.guest_address(), samples_ptr.guest_address(),
                  GetAudioCallerAddress());
  }
  ++submit_krnl_count;

  auto* audio_system =
      static_cast<audio::AudioSystem*>(REX_KERNEL_STATE()->emulator()->audio_system());
  audio_system->SubmitFrame(driver_ptr.guest_address() & 0x0000FFFF, samples_ptr.guest_address());

  return X_ERROR_SUCCESS;
}

}  // namespace rex::kernel::xboxkrnl

REX_EXPORT(__imp__XAudioGetSpeakerConfig, rex::kernel::xboxkrnl::XAudioGetSpeakerConfig_entry)
REX_EXPORT(__imp__XAudioGetVoiceCategoryVolumeChangeMask,
           rex::kernel::xboxkrnl::XAudioGetVoiceCategoryVolumeChangeMask_entry)
REX_EXPORT(__imp__XAudioGetVoiceCategoryVolume,
           rex::kernel::xboxkrnl::XAudioGetVoiceCategoryVolume_entry)
REX_EXPORT(__imp__XAudioEnableDucker, rex::kernel::xboxkrnl::XAudioEnableDucker_entry)
REX_EXPORT(__imp__XAudioRegisterRenderDriverClient,
           rex::kernel::xboxkrnl::XAudioRegisterRenderDriverClient_entry)
REX_EXPORT(__imp__XAudioUnregisterRenderDriverClient,
           rex::kernel::xboxkrnl::XAudioUnregisterRenderDriverClient_entry)
REX_EXPORT(__imp__XAudioSubmitRenderDriverFrame,
           rex::kernel::xboxkrnl::XAudioSubmitRenderDriverFrame_entry)

REX_HOOK_RAW(__imp__XAudioRenderDriverInitialize) {
  static uint32_t call_count = 0;
  if (call_count < 8) {
    REXKRNL_WARN("XAUDIO_TRACE RenderDriverInitialize #{} caller={:08X} r3={:08X} r4={:08X} r5={:08X}",
                 call_count, static_cast<uint32_t>(ctx.lr), ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
  }
  ++call_count;
  ctx.r3.u64 = 0;
}
static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioRenderDriverInitialize(
    "__imp__XAudioRenderDriverInitialize", &__imp__XAudioRenderDriverInitialize);

REX_HOOK_RAW(__imp__XAudioRenderDriverLock) {
  static uint32_t call_count = 0;
  if (call_count < 8) {
    REXKRNL_WARN("XAUDIO_TRACE RenderDriverLock #{} caller={:08X} r3={:08X} r4={:08X}",
                 call_count, static_cast<uint32_t>(ctx.lr), ctx.r3.u32, ctx.r4.u32);
  }
  ++call_count;
  ctx.r3.u64 = 0;
}
static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioRenderDriverLock(
    "__imp__XAudioRenderDriverLock", &__imp__XAudioRenderDriverLock);
REX_EXPORT_STUB(__imp__XAudioBeginDigitalBypassMode);
REX_EXPORT_STUB(__imp__XAudioEndDigitalBypassMode);
REX_EXPORT_STUB(__imp__XAudioSubmitDigitalPacket);
REX_EXPORT_STUB(__imp__XAudioQueryDriverPerformance);
REX_EXPORT_STUB(__imp__XAudioGetRenderDriverThread);
REX_EXPORT_STUB(__imp__XAudioSuspendRenderDriverClients);
REX_EXPORT_STUB(__imp__XAudioRegisterRenderDriverMECClient);
REX_EXPORT_STUB(__imp__XAudioUnregisterRenderDriverMECClient);
REX_EXPORT_STUB(__imp__XAudioCaptureRenderDriverFrame);
// XAudioGetRenderDriverTic - Returns the current audio driver tick count.
// On Xbox 360, this returns a 64-bit value based on the system timebase.
// Used by XAudio2 for internal timing (buffer position tracking, etc.).
// Previously stubbed (returned 0), which may cause audio timing issues.
REX_HOOK_RAW(__imp__XAudioGetRenderDriverTic) {
  // TEMP_DIAG: Log tic values for clock rate analysis
  {
    static uint32_t tic_call_count = 0;
    static uint64_t tic_prev_value = 0;
    static auto tic_prev_wall = std::chrono::steady_clock::now();
    tic_call_count++;
    uint64_t tic_value = rex::chrono::Clock::QueryGuestTickCount();
    ctx.r3.u64 = tic_value;
    bool should_log = (tic_call_count <= 200) || (tic_call_count % 500 == 0);
    if (should_log) {
      auto now = std::chrono::steady_clock::now();
      double wall_delta = std::chrono::duration<double>(now - tic_prev_wall).count();
      uint64_t tic_delta = (tic_value >= tic_prev_value) ? (tic_value - tic_prev_value) : 0;
      double tic_rate = (wall_delta > 0.0001) ? (double)tic_delta / wall_delta : 0;
      REXAPU_ERROR("TIC_DIAG[{}] tick={:016X} delta_tick={} wall_dt={:.6f}s "
                   "rate={:.0f}/s tid={}",
                   tic_call_count, tic_value, tic_delta, wall_delta, tic_rate,
                   rex::thread::current_thread_id());
      tic_prev_wall = now;
      tic_prev_value = tic_value;
    } else {
      // Still update tracking even when not logging
      auto now = std::chrono::steady_clock::now();
      tic_prev_wall = now;
      tic_prev_value = tic_value;
    }
  }
  // END TEMP_DIAG
}

static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioGetRenderDriverTic(
    "__imp__XAudioGetRenderDriverTic", &__imp__XAudioGetRenderDriverTic);

REX_HOOK_RAW(__imp__XAudioSetVoiceCategoryVolume) {
  const uint32_t category = ctx.r3.u32;
  const uint32_t volume_bits = ctx.r4.u32;
  float volume = DecodeGuestFloat(volume_bits);
  if (!std::isfinite(volume)) {
    volume = 1.0f;
  }
  const float clamped_volume = std::clamp(volume, 0.0f, 8.0f);

  float old_volume = 1.0f;
  if (category < g_voice_category_volumes.size()) {
    old_volume = g_voice_category_volumes[category];
    g_voice_category_volumes[category] = clamped_volume;
    if (category < 32 && std::fabs(old_volume - clamped_volume) > 0.0001f) {
      g_voice_category_change_mask |= (1u << category);
    }
  }

  static uint32_t set_vol_log_count = 0;
  if (set_vol_log_count < 180 || std::fabs(old_volume - clamped_volume) > 0.0001f ||
      (set_vol_log_count % 200) == 0) {
    REXKRNL_ERROR("XAUDIO_DIAG SetVoiceCategoryVolume caller={:08X} category={} value={:.4f} "
                  "old={:.4f} raw={:08X} changed_mask={:08X}",
                  static_cast<uint32_t>(ctx.lr), category, clamped_volume, old_volume, volume_bits,
                  g_voice_category_change_mask);
  }
  ++set_vol_log_count;

  ctx.r3.u64 = 0;
}

static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioSetVoiceCategoryVolume(
    "__imp__XAudioSetVoiceCategoryVolume", &__imp__XAudioSetVoiceCategoryVolume);

REX_HOOK_RAW(__imp__XAudioSetSpeakerConfig) {
  const uint32_t new_config = ctx.r3.u32;
  const uint32_t old_config = g_speaker_config;
  g_speaker_config = new_config;

  static uint32_t set_speaker_log_count = 0;
  if (set_speaker_log_count < 100 || old_config != new_config || (set_speaker_log_count % 200) == 0) {
    REXKRNL_ERROR(
        "XAUDIO_DIAG SetSpeakerConfig caller={:08X} config={:08X} old={:08X} override={} "
        "override_cfg={:08X}",
        static_cast<uint32_t>(ctx.lr), new_config, old_config, g_speaker_config_overridden ? 1 : 0,
        g_override_speaker_config);
  }
  ++set_speaker_log_count;

  ctx.r3.u64 = 0;
}

static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioSetSpeakerConfig(
    "__imp__XAudioSetSpeakerConfig", &__imp__XAudioSetSpeakerConfig);

REX_HOOK_RAW(__imp__XAudioOverrideSpeakerConfig) {
  const uint32_t arg0 = ctx.r3.u32;
  const uint32_t arg1 = ctx.r4.u32;
  const uint32_t old_override_cfg = g_override_speaker_config;
  const bool old_override_enabled = g_speaker_config_overridden;

  if (arg0 == 0 && arg1 == 0) {
    g_speaker_config_overridden = false;
  } else {
    g_speaker_config_overridden = true;
    g_override_speaker_config = arg1 ? arg1 : arg0;
  }

  static uint32_t override_speaker_log_count = 0;
  if (override_speaker_log_count < 100 ||
      old_override_cfg != g_override_speaker_config ||
      old_override_enabled != g_speaker_config_overridden ||
      (override_speaker_log_count % 200) == 0) {
    REXKRNL_ERROR(
        "XAUDIO_DIAG OverrideSpeakerConfig caller={:08X} arg0={:08X} arg1={:08X} "
        "enabled={} cfg={:08X} old_enabled={} old_cfg={:08X}",
        static_cast<uint32_t>(ctx.lr), arg0, arg1, g_speaker_config_overridden ? 1 : 0,
        g_override_speaker_config, old_override_enabled ? 1 : 0, old_override_cfg);
  }
  ++override_speaker_log_count;

  ctx.r3.u64 = 0;
}

static rex::ppc::detail::PPCFuncRegistrar _ppc_reg___imp__XAudioOverrideSpeakerConfig(
    "__imp__XAudioOverrideSpeakerConfig", &__imp__XAudioOverrideSpeakerConfig);

REX_EXPORT_STUB(__imp__XAudioSetDuckerLevel);
REX_EXPORT_STUB(__imp__XAudioIsDuckerEnabled);
REX_EXPORT_STUB(__imp__XAudioGetDuckerLevel);
REX_EXPORT_STUB(__imp__XAudioGetDuckerThreshold);
REX_EXPORT_STUB(__imp__XAudioSetDuckerThreshold);
REX_EXPORT_STUB(__imp__XAudioGetDuckerAttackTime);
REX_EXPORT_STUB(__imp__XAudioSetDuckerAttackTime);
REX_EXPORT_STUB(__imp__XAudioGetDuckerReleaseTime);
REX_EXPORT_STUB(__imp__XAudioSetDuckerReleaseTime);
REX_EXPORT_STUB(__imp__XAudioGetDuckerHoldTime);
REX_EXPORT_STUB(__imp__XAudioSetDuckerHoldTime);
REX_EXPORT_STUB(__imp__XAudioGetUnderrunCount);
REX_EXPORT_STUB(__imp__XAudioSetProcessFrameCallback);
