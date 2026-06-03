#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rex::graphics::metal {

class MetalShaderCache {
 public:
  static constexpr uint32_t kStorageVersion = 1;

  struct CachedMetallib {
    std::string function_name;
    std::vector<uint8_t> metallib_data;
  };

  struct CacheStats {
    size_t entry_count = 0;
    size_t memory_entry_count = 0;
    size_t memory_total_bytes = 0;
    size_t total_bytes = 0;
  };

  MetalShaderCache() = default;
  ~MetalShaderCache() = default;

  void Initialize(const std::filesystem::path& cache_dir);
  void Shutdown();

  bool IsInitialized() const { return initialized_; }

  static uint64_t GetCacheKey(uint64_t ucode_hash, uint64_t modification,
                              uint32_t stage);

  CacheStats GetStats() const;
  bool Load(uint64_t cache_key, CachedMetallib* out);
  void Store(uint64_t cache_key, std::string_view function_name,
             const uint8_t* metallib_data, size_t metallib_size);

 private:
  std::filesystem::path GetDiskPath(uint64_t cache_key) const;
  bool LoadFromDisk(uint64_t cache_key, CachedMetallib* out);
  bool StoreToDisk(uint64_t cache_key, const CachedMetallib& in);

  struct MemoryEntry {
    std::string function_name;
    std::vector<uint8_t> metallib_data;
  };

  mutable std::mutex mutex_;
  std::unordered_map<uint64_t, MemoryEntry> cache_;
  std::filesystem::path cache_dir_;
  bool initialized_ = false;
};

extern std::unique_ptr<MetalShaderCache> g_metal_shader_cache;

}  // namespace rex::graphics::metal
