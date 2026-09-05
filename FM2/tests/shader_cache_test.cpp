#include <cstddef>
#include <cstdio>
#include <string_view>

#include "generated/shader_cache.h"

namespace {

int Fail(const char* what) {
  std::fprintf(stderr, "shader_cache_test: FAILED: %s\n", what);
  return 1;
}

// Entries come from two legitimate XenosRecomp passes: one against known
// FM2/assets/... files, one backfilled from hash-named missed_shaders/*.bin
// dumps that have no discoverable source-asset path. Verify the latter's
// hex actually matches the entry's own hash rather than just trusting the
// prefix, since that's the only way to catch real corruption here.
bool IsWellFormedHashFallbackName(std::string_view filename, uint64_t hash) {
  constexpr std::string_view kPrefix = "shaders/";
  constexpr std::string_view kSuffix = ".bin";
  if (!filename.starts_with(kPrefix) || !filename.ends_with(kSuffix)) return false;
  const std::string_view hex = filename.substr(
      kPrefix.size(), filename.size() - kPrefix.size() - kSuffix.size());
  if (hex.size() != 16) return false;
  char expected[17];
  std::snprintf(expected, sizeof(expected), "%016llX",
                static_cast<unsigned long long>(hash));
  return hex == std::string_view(expected);
}

}  // namespace

int main() {
  // Menu/race shaders, including the preloaded XGRegisterShader track corpus.
  if (g_shaderCacheEntryCount != 371) return Fail("expected all 371 menu and race shaders");
  if (g_dxilCacheCompressedSize == 0) return Fail("g_dxilCacheCompressedSize == 0");
  if (g_dxilCacheDecompressedSize == 0) return Fail("g_dxilCacheDecompressedSize == 0");
  if (g_spirvCacheCompressedSize == 0) return Fail("g_spirvCacheCompressedSize == 0");
  if (g_spirvCacheDecompressedSize == 0) return Fail("g_spirvCacheDecompressedSize == 0");

  uint64_t previous_hash = 0;
  for (size_t i = 0; i < g_shaderCacheEntryCount; ++i) {
    const ShaderCacheEntry& entry = g_shaderCacheEntries[i];
    if (entry.hash <= previous_hash) return Fail("entry hashes not strictly increasing");
    previous_hash = entry.hash;

    if (entry.dxil_size == 0) return Fail("entry.dxil_size == 0");
    if (entry.spirv_size == 0) return Fail("entry.spirv_size == 0");
    if (entry.dxil_offset + entry.dxil_size > g_dxilCacheDecompressedSize)
      return Fail("dxil_offset + dxil_size exceeds decompressed cache size");
    if (entry.spirv_offset + entry.spirv_size > g_spirvCacheDecompressedSize)
      return Fail("spirv_offset + spirv_size exceeds decompressed cache size");
    if (entry.filename == nullptr) return Fail("entry.filename == nullptr");

    const std::string_view filename(entry.filename);
    if (filename.empty()) return Fail("entry.filename is empty");
    if (!filename.starts_with("FM2/assets/") &&
        !IsWellFormedHashFallbackName(filename, entry.hash)) {
      return Fail("entry.filename is neither an FM2/assets/ path nor a valid shaders/<hash>.bin fallback name");
    }
    if (filename.find('\\') != std::string_view::npos) return Fail("entry.filename contains a backslash");
  }

  std::printf("shader_cache_test: OK (%zu entries)\n", g_shaderCacheEntryCount);
  return 0;
}
