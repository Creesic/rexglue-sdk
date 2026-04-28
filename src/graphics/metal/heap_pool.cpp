#include <rex/graphics/metal/heap_pool.h>

#include <algorithm>

#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

namespace {
constexpr size_t kDefaultMaxHeapBytes = 512ull * 1024ull * 1024ull;
constexpr size_t kMinMaxHeapBytes = 256ull * 1024ull * 1024ull;
constexpr size_t kMaxMaxHeapBytes = 1024ull * 1024ull * 1024ull;

uint64_t next_pow2(uint64_t v) {
  if (v == 0) return 1;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  return v + 1;
}

uint64_t round_up(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

size_t GetMaxHeapBytes(MTL::Device* device) {
  if (!device) return kDefaultMaxHeapBytes;
  uint64_t recommended = device->recommendedMaxWorkingSetSize();
  if (!recommended) return kDefaultMaxHeapBytes;
  uint64_t budget = recommended / 4;
  budget = std::max<uint64_t>(budget, kMinMaxHeapBytes);
  budget = std::min<uint64_t>(budget, kMaxMaxHeapBytes);
  return static_cast<size_t>(budget);
}
}  // namespace

HeapPool::HeapPool(MTL::Device* device, MTL::StorageMode storage_mode,
                   size_t min_heap_size, const char* label_prefix)
    : device_(device),
      storage_mode_(storage_mode),
      min_heap_size_(min_heap_size),
      max_heap_bytes_(GetMaxHeapBytes(device)),
      label_prefix_(label_prefix ? label_prefix : "") {}

HeapPool::~HeapPool() { Shutdown(); }

void HeapPool::Shutdown() {
  for (auto& entry : heaps_) {
    if (entry.heap) {
      entry.heap->release();
      entry.heap = nullptr;
    }
  }
  heaps_.clear();
  total_heap_bytes_ = 0;
}

MTL::Texture* HeapPool::CreateTexture(MTL::TextureDescriptor* descriptor) {
  if (!device_ || !descriptor) return nullptr;
  MTL::SizeAndAlign size_align = device_->heapTextureSizeAndAlign(descriptor);
  if (!size_align.size || !size_align.align) return nullptr;
  MTL::Heap* heap =
      GetHeapForSize(size_t(size_align.size), size_t(size_align.align));
  if (!heap) return nullptr;
  return heap->newTexture(descriptor);
}

MTL::Heap* HeapPool::GetHeapForSize(size_t size, size_t alignment) {
  for (auto& entry : heaps_) {
    if (!entry.heap) continue;
    size_t available = size_t(
        entry.heap->maxAvailableSize(static_cast<NS::UInteger>(alignment)));
    if (available >= size) return entry.heap;
  }

  size_t heap_size = std::max(size, min_heap_size_);
  heap_size = static_cast<size_t>(next_pow2(heap_size));
  heap_size = static_cast<size_t>(round_up(heap_size, alignment));
  if (max_heap_bytes_ && heap_size > max_heap_bytes_) return nullptr;
  if (max_heap_bytes_ && total_heap_bytes_ > max_heap_bytes_ - heap_size)
    return nullptr;

  MTL::HeapDescriptor* desc = MTL::HeapDescriptor::alloc()->init();
  desc->setStorageMode(storage_mode_);
  desc->setHazardTrackingMode(MTL::HazardTrackingModeTracked);
  desc->setSize(heap_size);

  MTL::Heap* heap = device_->newHeap(desc);
  desc->release();
  if (!heap) {
    REXLOG_ERROR("HeapPool: failed to create heap ({} bytes)", heap_size);
    return nullptr;
  }
  if (!label_prefix_.empty()) {
    std::string label =
        label_prefix_ + "_heap_" + std::to_string(heaps_.size());
    heap->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
  }

  heaps_.push_back({heap, heap_size});
  total_heap_bytes_ += heap_size;
  return heap;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
