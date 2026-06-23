#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

#include "generated/shader_cache.h"

TEST_CASE("FM2 generated shader cache contains valid offline entries",
          "[fm2][plume][shader-cache]") {
  REQUIRE(g_shaderCacheEntryCount > 0);
  CHECK(g_dxilCacheCompressedSize > 0);
  CHECK(g_dxilCacheDecompressedSize > 0);
  CHECK(g_spirvCacheCompressedSize > 0);
  CHECK(g_spirvCacheDecompressedSize > 0);

  uint64_t previous_hash = 0;
  for (size_t i = 0; i < g_shaderCacheEntryCount; ++i) {
    const ShaderCacheEntry& entry = g_shaderCacheEntries[i];
    CHECK(entry.hash > previous_hash);
    previous_hash = entry.hash;

    CHECK(entry.dxil_size > 0);
    CHECK(entry.spirv_size > 0);
    CHECK(entry.dxil_offset + entry.dxil_size <= g_dxilCacheDecompressedSize);
    CHECK(entry.spirv_offset + entry.spirv_size <=
          g_spirvCacheDecompressedSize);
    REQUIRE(entry.filename != nullptr);

    const std::string_view filename(entry.filename);
    CHECK(!filename.empty());
    CHECK(filename.starts_with("FM2/assets/"));
    CHECK(filename.find('\\') == std::string_view::npos);
  }
}
