#pragma once

#include <cstdint>

struct PPCContext;

/// Allocate the 712-byte track-loader object via sub_8240AC00 (native null retry).
uint32_t Fh1AllocateTrackLoaderObject(PPCContext& ctx, uint8_t* base);
