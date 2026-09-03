#pragma once

#include <cstddef>
#include <cstdint>

namespace fm2::render {
struct GuestShader;
}

struct ShaderCacheEntry {
  uint64_t hash;
  uint32_t dxil_offset;
  uint32_t dxil_size;
  uint32_t spirv_offset;
  uint32_t spirv_size;
  uint32_t air_offset;
  uint32_t air_size;
  uint32_t spec_constants_mask;
  const char* filename;
  fm2::render::GuestShader* guest_shader = nullptr;
};

extern ShaderCacheEntry g_shaderCacheEntries[];
extern const size_t g_shaderCacheEntryCount;

extern const uint8_t g_compressedDxilCache[];
extern const size_t g_dxilCacheCompressedSize;
extern const size_t g_dxilCacheDecompressedSize;

extern const uint8_t g_compressedSpirvCache[];
extern const size_t g_spirvCacheCompressedSize;
extern const size_t g_spirvCacheDecompressedSize;
