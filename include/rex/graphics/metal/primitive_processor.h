#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <Metal/Metal.hpp>
#include <rex/graphics/primitive_processor.h>

namespace rex::graphics::metal {

class MetalCommandProcessor;

class MetalPrimitiveProcessor : public PrimitiveProcessor {
 public:
  MetalPrimitiveProcessor(MetalCommandProcessor& command_processor,
                          const RegisterFile& register_file,
                          memory::Memory& memory, TraceWriter& trace_writer,
                          SharedMemory& shared_memory);
  ~MetalPrimitiveProcessor() override;

  bool Initialize();
  void Shutdown(bool from_destructor = false);

  void BeginFrame();
  void EndFrame();

  MTL::Buffer* GetConvertedIndexBuffer(size_t handle,
                                       uint64_t& offset_bytes_out) const;

  MTL::Buffer* GetBuiltinIndexBuffer() const {
    return builtin_index_buffer_;
  }

 protected:
  bool InitializeBuiltinIndexBuffer(
      size_t size_bytes,
      std::function<void(void*)> fill_callback) override;

  void* RequestHostConvertedIndexBufferForCurrentFrame(
      xenos::IndexFormat format, uint32_t index_count, bool coalign_for_simd,
      uint32_t coalignment_original_address,
      size_t& backend_handle_out) override;

 private:
  MetalCommandProcessor& command_processor_;

  struct FrameIndexBuffer {
    MTL::Buffer* buffer = nullptr;
    size_t size = 0;
    uint64_t last_frame_used = 0;
  };
  std::vector<FrameIndexBuffer> frame_index_buffers_;
  uint64_t current_frame_ = 0;

  MTL::Buffer* builtin_index_buffer_ = nullptr;
  size_t builtin_index_buffer_size_ = 0;

  struct ConvertedIndexBufferBinding {
    MTL::Buffer* buffer = nullptr;
    uint64_t offset_bytes = 0;
  };
  std::vector<ConvertedIndexBufferBinding> converted_index_buffers_;
};

}  // namespace rex::graphics::metal
