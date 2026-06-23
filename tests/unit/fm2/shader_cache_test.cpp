#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

#include "generated/shader_cache.h"
#include "native_renderer/fm2_shader_cache_status.h"

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
    CHECK(entry.spirv_offset + entry.spirv_size <= g_spirvCacheDecompressedSize);
    REQUIRE(entry.filename != nullptr);

    const std::string_view filename(entry.filename);
    CHECK(!filename.empty());
    CHECK(filename.starts_with("FM2/assets/"));
    CHECK(filename.find('\\') == std::string_view::npos);
  }
}

TEST_CASE("FM2 shader cache coverage probe checks payload and structural keys",
          "[fm2][plume][shader-cache]") {
  ShaderCacheEntry entries[] = {
      {0x10u, 0, 0, 0, 0, 0, 0, 0, nullptr, nullptr},
      {0x20u, 0, 0, 0, 0, 0, 0, 0, nullptr, nullptr},
      {0x30u, 0, 0, 0, 0, 0, 0, 0, nullptr, nullptr},
  };

  CHECK(fm2::native_renderer::FindShaderCacheEntryByHash(entries, 3, 0x20u) == &entries[1]);
  CHECK(fm2::native_renderer::FindShaderCacheEntryByHash(entries, 3, 0x21u) == nullptr);
  CHECK(fm2::native_renderer::FindShaderCacheEntryByHash(entries, 0, 0x10u) == nullptr);

  const auto payload_coverage =
      fm2::native_renderer::ProbeNativeShaderCacheCoverage(entries, 3, 0x20u, false, 0u);
  CHECK(payload_coverage.payload_hit);
  CHECK_FALSE(payload_coverage.structural_ucode_hit);
  CHECK(payload_coverage.covered());

  const auto structural_coverage =
      fm2::native_renderer::ProbeNativeShaderCacheCoverage(entries, 3, 0x99u, true, 0x30u);
  CHECK_FALSE(structural_coverage.payload_hit);
  CHECK(structural_coverage.structural_ucode_hit);
  CHECK(structural_coverage.covered());

  const auto miss =
      fm2::native_renderer::ProbeNativeShaderCacheCoverage(entries, 3, 0x99u, true, 0x98u);
  CHECK_FALSE(miss.payload_hit);
  CHECK_FALSE(miss.structural_ucode_hit);
  CHECK_FALSE(miss.covered());
}
