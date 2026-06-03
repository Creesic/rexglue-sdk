#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <Metal/Metal.hpp>

namespace rex::graphics::metal {

class MetalHeapPool {
 public:
  MetalHeapPool(MTL::Device* device, MTL::StorageMode storage_mode,
                size_t min_heap_size, const char* label_prefix);
  ~MetalHeapPool();

  void Shutdown();

  MTL::Texture* CreateTexture(MTL::TextureDescriptor* descriptor);

 private:
  MTL::Heap* GetHeapForSize(size_t size, size_t alignment);

  MTL::Device* device_;
  MTL::StorageMode storage_mode_;
  size_t min_heap_size_;
  size_t max_heap_bytes_;
  std::string label_prefix_;

  struct HeapEntry {
    MTL::Heap* heap = nullptr;
    size_t size = 0;
  };
  std::vector<HeapEntry> heaps_;
  size_t total_heap_bytes_ = 0;
};

}  // namespace rex::graphics::metal
