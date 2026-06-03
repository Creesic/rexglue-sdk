#pragma once

#include <cstddef>
#include <cstdint>

#include <Metal/Metal.hpp>
#include <rex/ui/graphics_upload_buffer_pool.h>

namespace rex::graphics::metal {

class MetalUploadBufferPool : public ui::GraphicsUploadBufferPool {
 public:
  MetalUploadBufferPool(MTL::Device* device,
                        size_t page_size = kDefaultPageSize);
  ~MetalUploadBufferPool() override;

  uint8_t* Request(uint64_t submission_index, size_t size, size_t alignment,
                   MTL::Buffer** buffer_out, size_t& offset_out,
                   uint64_t& gpu_address_out);

  uint8_t* RequestPartial(uint64_t submission_index, size_t size,
                          size_t alignment, MTL::Buffer** buffer_out,
                          size_t& offset_out, uint64_t& gpu_address_out,
                          size_t& size_out);

 protected:
  struct MetalPage : public Page {
    MetalPage(MTL::Buffer* buffer, void* mapping);
    ~MetalPage() override;

    MTL::Buffer* buffer_;
    void* mapping_;
    uint64_t gpu_address_;
  };

  Page* CreatePageImplementation() override;

 private:
  MTL::Device* device_;
};

}  // namespace rex::graphics::metal
