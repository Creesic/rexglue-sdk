#pragma once

#include "generated/shader_cache.h"

#include <cstddef>
#include <cstdint>

namespace fm2::native_renderer {

struct NativeShaderCacheCoverage {
  bool payload_hit = false;
  bool structural_ucode_hit = false;

  constexpr bool covered() const { return payload_hit || structural_ucode_hit; }
};

constexpr const ShaderCacheEntry* FindShaderCacheEntryByHash(const ShaderCacheEntry* entries,
                                                             size_t entry_count, uint64_t hash) {
  if (entries == nullptr || hash == 0) {
    return nullptr;
  }

  size_t lo = 0;
  size_t hi = entry_count;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (entries[mid].hash < hash) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }

  return lo < entry_count && entries[lo].hash == hash ? &entries[lo] : nullptr;
}

constexpr NativeShaderCacheCoverage ProbeNativeShaderCacheCoverage(const ShaderCacheEntry* entries,
                                                                   size_t entry_count,
                                                                   uint64_t payload_hash,
                                                                   bool has_structural_ucode_hash,
                                                                   uint64_t structural_ucode_hash) {
  NativeShaderCacheCoverage out;
  out.payload_hit = FindShaderCacheEntryByHash(entries, entry_count, payload_hash) != nullptr;
  if (has_structural_ucode_hash) {
    out.structural_ucode_hit =
        FindShaderCacheEntryByHash(entries, entry_count, structural_ucode_hash) != nullptr;
  }
  return out;
}

}  // namespace fm2::native_renderer
