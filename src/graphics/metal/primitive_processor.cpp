#include <rex/graphics/metal/primitive_processor.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/logging/macros.h>

#include <algorithm>

namespace rex {
namespace graphics {
namespace metal {

MetalPrimitiveProcessor::MetalPrimitiveProcessor(
    MetalCommandProcessor& command_processor, const RegisterFile& register_file,
    memory::Memory& memory, TraceWriter& trace_writer,
    SharedMemory& shared_memory)
    : PrimitiveProcessor(register_file, memory, trace_writer, shared_memory),
      command_processor_(command_processor) {}

MetalPrimitiveProcessor::~MetalPrimitiveProcessor() { Shutdown(true); }

bool MetalPrimitiveProcessor::Initialize() {
  if (!InitializeCommon(true, false, false, false, false, false)) {
    Shutdown();
    return false;
  }

  constexpr uint32_t kMaxExpandedPrimitiveCount = UINT16_MAX;
  constexpr uint32_t kIndicesPerExpandedPrimitive = 6;
  size_t index_count =
      size_t(kMaxExpandedPrimitiveCount) * kIndicesPerExpandedPrimitive;
  size_t buffer_size_bytes = index_count * sizeof(uint32_t);
  MTL::Device* device = command_processor_.GetMetalDevice();
  expansion_triangle_list_index_buffer_ =
      device->newBuffer(buffer_size_bytes, MTL::ResourceStorageModeShared);
  if (!expansion_triangle_list_index_buffer_) {
    REXLOG_ERROR("Failed to create Metal expansion index buffer");
    Shutdown();
    return false;
  }
  expansion_triangle_list_index_buffer_->setLabel(
      NS::String::string("ReX Expansion Triangle List Index Buffer",
                         NS::UTF8StringEncoding));
  uint32_t* indices = reinterpret_cast<uint32_t*>(
      expansion_triangle_list_index_buffer_->contents());
  for (uint32_t i = 0; i < kMaxExpandedPrimitiveCount; ++i) {
    uint32_t base = i << 2;
    size_t write_index = size_t(i) * kIndicesPerExpandedPrimitive;
    indices[write_index + 0] = base + 0;
    indices[write_index + 1] = base + 1;
    indices[write_index + 2] = base + 2;
    indices[write_index + 3] = base + 2;
    indices[write_index + 4] = base + 1;
    indices[write_index + 5] = base + 3;
  }

  REXLOG_INFO("MetalPrimitiveProcessor initialized");
  return true;
}

void MetalPrimitiveProcessor::Shutdown(bool from_destructor) {
  for (auto& fb : frame_index_buffers_) {
    if (fb.buffer) fb.buffer->release();
  }
  frame_index_buffers_.clear();
  if (builtin_index_buffer_) {
    builtin_index_buffer_->release();
    builtin_index_buffer_ = nullptr;
    builtin_index_buffer_gpu_address_ = 0;
    builtin_index_buffer_size_ = 0;
  }
  if (expansion_triangle_list_index_buffer_) {
    expansion_triangle_list_index_buffer_->release();
    expansion_triangle_list_index_buffer_ = nullptr;
  }
  if (!from_destructor) ShutdownCommon();
}

void MetalPrimitiveProcessor::CompletedSubmissionUpdated() {}
void MetalPrimitiveProcessor::BeginSubmission() {}

void MetalPrimitiveProcessor::BeginFrame() {
  converted_index_buffers_.clear();
  ++current_frame_;
  uint64_t frame = current_frame_;
  frame_index_buffers_.erase(
      std::remove_if(frame_index_buffers_.begin(), frame_index_buffers_.end(),
                     [frame](const FrameIndexBuffer& b) {
                       if (frame - b.last_frame_used > 2) {
                         if (b.buffer) b.buffer->release();
                         return true;
                       }
                       return false;
                     }),
      frame_index_buffers_.end());
}

void MetalPrimitiveProcessor::EndFrame() {
  ClearPerFrameCache();
  converted_index_buffers_.clear();
}

MTL::Buffer* MetalPrimitiveProcessor::GetConvertedIndexBuffer(
    size_t handle, uint64_t& offset_bytes_out) const {
  if (handle >= converted_index_buffers_.size()) {
    offset_bytes_out = 0;
    return nullptr;
  }
  const ConvertedIndexBufferBinding& binding = converted_index_buffers_[handle];
  offset_bytes_out = binding.offset_bytes;
  return binding.buffer;
}

bool MetalPrimitiveProcessor::InitializeBuiltinIndexBuffer(
    size_t size_bytes, std::function<void(void*)> fill_callback) {
  assert_not_zero(size_bytes);
  assert_null(builtin_index_buffer_);
  MTL::Device* device = command_processor_.GetMetalDevice();
  builtin_index_buffer_ =
      device->newBuffer(size_bytes, MTL::ResourceStorageModeShared);
  if (!builtin_index_buffer_) return false;
  builtin_index_buffer_size_ = size_bytes;
  builtin_index_buffer_->setLabel(
      NS::String::string("ReX Built-in Index Buffer", NS::UTF8StringEncoding));
  fill_callback(builtin_index_buffer_->contents());
  builtin_index_buffer_gpu_address_ = builtin_index_buffer_->gpuAddress();
  return true;
}

void* MetalPrimitiveProcessor::RequestHostConvertedIndexBufferForCurrentFrame(
    xenos::IndexFormat format, uint32_t index_count, bool coalign_for_simd,
    uint32_t coalignment_original_address, size_t& backend_handle_out) {
  size_t element_size =
      format == xenos::IndexFormat::kInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
  size_t required_size = index_count * element_size;
  if (coalign_for_simd) required_size += XE_GPU_PRIMITIVE_PROCESSOR_SIMD_SIZE;

  FrameIndexBuffer* chosen = nullptr;
  uint64_t frame = current_frame_;
  for (auto& fb : frame_index_buffers_) {
    if (fb.size >= required_size && fb.last_frame_used != frame) {
      chosen = &fb;
      break;
    }
  }
  if (!chosen) {
    MTL::Device* device = command_processor_.GetMetalDevice();
    size_t alloc_size = std::max(required_size, size_t(4096));
    alloc_size = (alloc_size + 4095) & ~size_t(4095);
    MTL::Buffer* buf =
        device->newBuffer(alloc_size, MTL::ResourceStorageModeShared);
    if (!buf) {
      backend_handle_out = 0;
      return nullptr;
    }
    frame_index_buffers_.push_back({buf, alloc_size, 0});
    chosen = &frame_index_buffers_.back();
  }
  chosen->last_frame_used = frame;

  uint64_t gpu_offset = 0;
  void* cpu = chosen->buffer->contents();
  if (coalign_for_simd) {
    ptrdiff_t offset =
        GetSimdCoalignmentOffset(cpu, coalignment_original_address);
    cpu = static_cast<uint8_t*>(cpu) + offset;
    gpu_offset += uint64_t(offset);
  }

  backend_handle_out = converted_index_buffers_.size();
  converted_index_buffers_.push_back({chosen->buffer, gpu_offset});
  return cpu;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
