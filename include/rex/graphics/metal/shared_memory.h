#pragma once

#include <cstdint>
#include <vector>

#include <Metal/Metal.hpp>
#include <rex/graphics/shared_memory.h>

namespace rex::graphics::metal {

class MetalCommandProcessor;

class MetalSharedMemory : public SharedMemory {
 public:
  MetalSharedMemory(MetalCommandProcessor& command_processor,
                    memory::Memory& memory);
  ~MetalSharedMemory() override;

  bool Initialize();
  void Shutdown();

  MTL::Buffer* GetBuffer() const { return buffer_; }

 protected:
  bool UploadRanges(
      const std::vector<std::pair<uint32_t, uint32_t>>& upload_page_ranges) override;

 private:
  MetalCommandProcessor& command_processor_;
  MTL::Buffer* buffer_ = nullptr;
  bool use_zero_copy_ = false;
};

}  // namespace rex::graphics::metal
