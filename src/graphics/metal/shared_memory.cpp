#include <rex/graphics/metal/shared_memory.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/ui/metal/provider.h>
#include <rex/logging/macros.h>
#include <rex/memory.h>

#include <Metal/Metal.hpp>

namespace rex {
namespace graphics {
namespace metal {

MetalSharedMemory::MetalSharedMemory(MetalCommandProcessor& command_processor,
                                     memory::Memory& memory)
    : SharedMemory(memory), command_processor_(command_processor) {}

MetalSharedMemory::~MetalSharedMemory() { Shutdown(); }

bool MetalSharedMemory::Initialize() {
  InitializeCommon();

  MTL::Device* device = command_processor_.GetMetalDevice();
  if (!device) {
    REXLOG_ERROR("MetalSharedMemory: No Metal device");
    return false;
  }

  void* xbox_ram = memory().TranslatePhysical(0);
  if (!xbox_ram) {
    REXLOG_ERROR("MetalSharedMemory: Xbox RAM is null");
    return false;
  }

  if (device->hasUnifiedMemory()) {
    buffer_ = device->newBuffer(xbox_ram, kBufferSize,
                                MTL::ResourceStorageModeShared, nullptr);
    if (buffer_) {
      use_zero_copy_ = true;
      REXLOG_INFO("MetalSharedMemory: using zero-copy buffer");
    }
  }

  if (!buffer_) {
    buffer_ = device->newBuffer(kBufferSize, MTL::ResourceStorageModeShared);
    if (buffer_ && xbox_ram) {
      memcpy(buffer_->contents(), xbox_ram, kBufferSize);
    }
  }

  if (!buffer_) {
    REXLOG_ERROR("MetalSharedMemory: Failed to create buffer");
    return false;
  }

  REXLOG_INFO("MetalSharedMemory: Initialized ({} bytes, zero_copy={})",
              kBufferSize, use_zero_copy_);
  return true;
}

void MetalSharedMemory::ClearCache() { SharedMemory::ClearCache(); }

bool MetalSharedMemory::UploadRanges(
    const std::vector<std::pair<uint32_t, uint32_t>>& upload_page_ranges) {
  if (!buffer_ || upload_page_ranges.empty()) return true;

  if (!use_zero_copy_) {
    uint8_t* buffer_data = static_cast<uint8_t*>(buffer_->contents());
    uint8_t* xbox_data =
        static_cast<uint8_t*>(memory().TranslatePhysical(0));
    if (!xbox_data) return false;

    const uint32_t page_size = 1u << page_size_log2();
    for (const auto& range : upload_page_ranges) {
      uint32_t start = range.first * page_size;
      uint32_t length = range.second * page_size;
      if (start >= kBufferSize) continue;
      if (start + length > kBufferSize) length = kBufferSize - start;
      MakeRangeValid(start, length, false);
      memcpy(buffer_data + start, xbox_data + start, length);
    }
  }
  return true;
}

void MetalSharedMemory::Shutdown() {
  if (buffer_) {
    buffer_->release();
    buffer_ = nullptr;
  }
  use_zero_copy_ = false;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
