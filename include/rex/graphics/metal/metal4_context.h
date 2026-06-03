#pragma once

#include <cstdint>
#include <functional>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

namespace rex::graphics::metal {

class Metal4Context {
 public:
  static constexpr uint32_t kMaxBufferBindings = 128;
  static constexpr uint32_t kMaxTextureBindings = 128;
  static constexpr uint32_t kMaxSamplerBindings = 32;
  static constexpr size_t kInlineConstantsSize = 1024 * 1024;

  Metal4Context() = default;
  ~Metal4Context();

  bool Initialize(MTL::Device* device);
  void Shutdown();

  MTL::Device* device() const { return device_; }
  MTL4::CommandQueue* queue() const { return queue_; }
  MTL4::CommandAllocator* allocator() const { return allocator_; }
  MTL::ResidencySet* GetResidencySet() const { return residency_set_; }

  MTL4::CommandBuffer* BeginCommandBuffer();
  void EndCommandBuffer(MTL4::CommandBuffer* cmd);
  void Commit(MTL4::CommandBuffer* cmd);
  void Commit(MTL4::CommandBuffer* cmd, MTL4::CommitOptions* options);

  MTL4::CommandBuffer* BeginStandaloneCommandBuffer();
  void CommitStandaloneAsync(MTL4::CommandBuffer* cmd);
  void CommitStandaloneAsyncWithCallback(
      MTL4::CommandBuffer* cmd,
      const std::function<void(MTL4::CommandBuffer*)>& callback);
  void CommitStandaloneAndWait(MTL4::CommandBuffer* cmd);

  void SignalDrawable(CA::MetalDrawable* drawable);
  void SignalEvent(MTL::Event* event, uint64_t value);
  void WaitEvent(MTL::Event* event, uint64_t value);

  void SetVertexAddress(MTL::GPUAddress addr, uint32_t idx);
  void SetVertexAddressWithStride(MTL::GPUAddress addr, uint32_t stride,
                                  uint32_t idx);
  void SetFragmentAddress(MTL::GPUAddress addr, uint32_t idx);
  void SetFragmentTexture(MTL::ResourceID resource_id, uint32_t idx);
  void SetFragmentSampler(MTL::ResourceID resource_id, uint32_t idx);
  void FlushRenderBindings(MTL4::RenderCommandEncoder* enc,
                           bool include_mesh_stages = false);

  void SetComputeAddress(MTL::GPUAddress addr, uint32_t idx);
  void SetComputeTexture(MTL::ResourceID resource_id, uint32_t idx);
  void SetComputeSampler(MTL::ResourceID resource_id, uint32_t idx);
  void FlushComputeBindings(MTL4::ComputeCommandEncoder* enc);

  MTL::GPUAddress AllocInlineConstant(const void* data, size_t len);

  MTL4::RenderPassDescriptor* CreateRenderPassDescriptor(
      MTL::RenderPassDescriptor* mtl3_desc);

  void ResetFrameState();

  void AddResidentAllocation(MTL::Allocation* alloc);
  void CommitResidency();

 private:
  MTL::Device* device_ = nullptr;
  MTL4::CommandQueue* queue_ = nullptr;
  MTL4::CommandAllocator* allocator_ = nullptr;
  MTL4::CommandAllocator* standalone_allocator_ = nullptr;
  MTL4::ArgumentTable* vertex_arg_table_ = nullptr;
  MTL4::ArgumentTable* fragment_arg_table_ = nullptr;
  MTL4::ArgumentTable* compute_arg_table_ = nullptr;
  MTL::Buffer* inline_constants_buffer_ = nullptr;
  size_t inline_constants_offset_ = 0;
  MTL::ResidencySet* residency_set_ = nullptr;
  bool residency_dirty_ = false;
  bool vertex_bindings_dirty_ = false;
  bool fragment_bindings_dirty_ = false;
  bool compute_bindings_dirty_ = false;
};

}  // namespace rex::graphics::metal
