/**
 ******************************************************************************
 * ReXGlue FM2 Native Diagnostics                                             *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

#include <rex/audio/fm2_native/codec.h>

namespace rex::audio::fm2_native {

struct RuntimeMetrics;

void LogBackendDecision(uint32_t codec_instance_ptr, const BackendDecision& decision);
void LogPerSecond(const RuntimeMetrics& metrics);

}  // namespace rex::audio::fm2_native

