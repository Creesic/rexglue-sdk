#include <rex/graphics/metal/metal4_context.h>

#include <rex/logging/macros.h>

#include <cstring>

namespace rex {
namespace graphics {
namespace metal {

Metal4Context::~Metal4Context() { Shutdown(); }

bool Metal4Context::Initialize(MTL::Device* device) {
  device_ = device;
  if (!device_) { fprintf(stderr, "[mtl4] Init: no device\n"); return false; }

  queue_ = device_->newMTL4CommandQueue();
  if (!queue_) {
    fprintf(stderr, "[mtl4] Init: FAILED to create MTL4 CommandQueue\n");
    return false;
  }
  fprintf(stderr, "[mtl4] Init: MTL4 queue=%p\n", (void*)queue_);

  allocator_ = device_->newCommandAllocator();
  if (!allocator_) {
    fprintf(stderr, "[mtl4] Init: FAILED to create CommandAllocator\n");
    return false;
  }

  standalone_allocator_ = device_->newCommandAllocator();
  if (!standalone_allocator_) {
    fprintf(stderr, "[mtl4] Init: FAILED to create standalone CommandAllocator\n");
    return false;
  }
  fprintf(stderr, "[mtl4] Init: allocators ok, creating arg tables\n");

  MTL4::ArgumentTableDescriptor* arg_desc =
      MTL4::ArgumentTableDescriptor::alloc()->init();
  arg_desc->setMaxBufferBindCount(kMaxBufferBindings);
  arg_desc->setMaxTextureBindCount(kMaxTextureBindings);
  arg_desc->setMaxSamplerStateBindCount(kMaxSamplerBindings);
  arg_desc->setInitializeBindings(true);

  NS::Error* err = nullptr;
  vertex_arg_table_ = device_->newArgumentTable(arg_desc, &err);
  if (!vertex_arg_table_) {
    REXLOG_ERROR("Metal4Context: Failed to create vertex ArgumentTable: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
    arg_desc->release();
    return false;
  }

  err = nullptr;
  fragment_arg_table_ = device_->newArgumentTable(arg_desc, &err);
  if (!fragment_arg_table_) {
    REXLOG_ERROR("Metal4Context: Failed to create fragment ArgumentTable: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
    arg_desc->release();
    return false;
  }

  err = nullptr;
  compute_arg_table_ = device_->newArgumentTable(arg_desc, &err);
  if (!compute_arg_table_) {
    REXLOG_ERROR("Metal4Context: Failed to create compute ArgumentTable: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
    arg_desc->release();
    return false;
  }

  arg_desc->release();

  inline_constants_buffer_ = device_->newBuffer(
      kInlineConstantsSize, MTL::ResourceStorageModeShared);
  if (!inline_constants_buffer_) {
    fprintf(stderr, "[mtl4] Init: FAILED to create inline constants buffer\n");
    return false;
  }

  MTL::ResidencySetDescriptor* rs_desc =
      MTL::ResidencySetDescriptor::alloc()->init();
  rs_desc->setInitialCapacity(8192);
  err = nullptr;
  residency_set_ = device_->newResidencySet(rs_desc, &err);
  if (!residency_set_) {
    fprintf(stderr, "[mtl4] Init: FAILED to create residency set: %s\n",
            err ? err->localizedDescription()->utf8String() : "unknown");
    rs_desc->release();
    return false;
  }
  residency_set_->addAllocation(inline_constants_buffer_);
  residency_set_->commit();
  rs_desc->release();

  queue_->addResidencySet(residency_set_);

  fprintf(stderr, "[mtl4] Init: success - queue=%p alloc=%p standalone_alloc=%p inline_buf=%p residency=%p\n",
          (void*)queue_, (void*)allocator_, (void*)standalone_allocator_,
          (void*)inline_constants_buffer_, (void*)residency_set_);
  return true;
}

void Metal4Context::Shutdown() {
  if (residency_set_) {
    residency_set_->release();
    residency_set_ = nullptr;
  }
  if (inline_constants_buffer_) {
    inline_constants_buffer_->release();
    inline_constants_buffer_ = nullptr;
  }
  if (compute_arg_table_) {
    compute_arg_table_->release();
    compute_arg_table_ = nullptr;
  }
  if (fragment_arg_table_) {
    fragment_arg_table_->release();
    fragment_arg_table_ = nullptr;
  }
  if (vertex_arg_table_) {
    vertex_arg_table_->release();
    vertex_arg_table_ = nullptr;
  }
  if (allocator_) {
    allocator_->release();
    allocator_ = nullptr;
  }
  if (standalone_allocator_) {
    standalone_allocator_->release();
    standalone_allocator_ = nullptr;
  }
  if (queue_) {
    queue_->release();
    queue_ = nullptr;
  }
  device_ = nullptr;
}

MTL4::CommandBuffer* Metal4Context::BeginCommandBuffer() {
  MTL4::CommandBuffer* cmd = device_->newCommandBuffer();
  if (!cmd) {
    REXLOG_ERROR("Metal4Context: Failed to create CommandBuffer");
    return nullptr;
  }
  cmd->beginCommandBuffer(allocator_);
  if (residency_set_) {
    cmd->useResidencySet(residency_set_);
  }
  return cmd;
}

void Metal4Context::EndCommandBuffer(MTL4::CommandBuffer* cmd) {
  if (cmd) cmd->endCommandBuffer();
}

void Metal4Context::Commit(MTL4::CommandBuffer* cmd) {
  if (cmd) {
    cmd->endCommandBuffer();
    const MTL4::CommandBuffer* bufs[] = {cmd};
    queue_->commit(bufs, 1);
    cmd->release();
  }
}

void Metal4Context::Commit(MTL4::CommandBuffer* cmd,
                           MTL4::CommitOptions* options) {
  if (cmd) {
    cmd->endCommandBuffer();
    const MTL4::CommandBuffer* bufs[] = {cmd};
    queue_->commit(bufs, 1, options);
    cmd->release();
  }
}

MTL4::CommandBuffer* Metal4Context::BeginStandaloneCommandBuffer() {
  if (!device_ || !standalone_allocator_) return nullptr;
  MTL4::CommandBuffer* cmd = device_->newCommandBuffer();
  if (!cmd) return nullptr;
  cmd->beginCommandBuffer(standalone_allocator_);
  if (residency_set_) {
    cmd->useResidencySet(residency_set_);
  }
  return cmd;
}

void Metal4Context::CommitStandaloneAsync(MTL4::CommandBuffer* cmd) {
  if (!cmd) return;
  cmd->endCommandBuffer();
  MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
  MTL4::CommandBuffer* cmd_ref = cmd;
  options->addFeedbackHandler([cmd_ref](MTL4::CommitFeedback*) {
    cmd_ref->release();
  });
  const MTL4::CommandBuffer* bufs[] = {cmd};
  queue_->commit(bufs, 1, options);
  options->release();
}

void Metal4Context::CommitStandaloneAsyncWithCallback(
    MTL4::CommandBuffer* cmd,
    const std::function<void(MTL4::CommandBuffer*)>& callback) {
  if (!cmd) return;
  cmd->endCommandBuffer();
  MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
  MTL4::CommandBuffer* cmd_ref = cmd;
  std::function<void(MTL4::CommandBuffer*)> cb = callback;
  options->addFeedbackHandler([cmd_ref, cb](MTL4::CommitFeedback*) {
    if (cb) cb(cmd_ref);
    cmd_ref->release();
  });
  const MTL4::CommandBuffer* bufs[] = {cmd};
  queue_->commit(bufs, 1, options);
  options->release();
}

void Metal4Context::CommitStandaloneAndWait(MTL4::CommandBuffer* cmd) {
  if (!cmd) return;
  cmd->endCommandBuffer();
  auto* done = new std::atomic<bool>(false);
  MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
  MTL4::CommandBuffer* cmd_ref = cmd;
  std::atomic<bool>* done_ref = done;
  options->addFeedbackHandler([cmd_ref, done_ref](MTL4::CommitFeedback*) {
    cmd_ref->release();
    done_ref->store(true);
    done_ref->notify_one();
  });
  const MTL4::CommandBuffer* bufs[] = {cmd};
  queue_->commit(bufs, 1, options);
  options->release();
  done->wait(false);
  delete done;
}

void Metal4Context::SignalDrawable(CA::MetalDrawable* drawable) {
  if (queue_ && drawable) {
    queue_->signalDrawable(
        reinterpret_cast<MTL::Drawable*>(drawable));
  }
}

void Metal4Context::WaitDrawable(CA::MetalDrawable* drawable) {
  if (queue_ && drawable) {
    queue_->wait(reinterpret_cast<MTL::Drawable*>(drawable));
  }
}

void Metal4Context::SignalEvent(MTL::Event* event, uint64_t value) {
  if (queue_ && event) {
    queue_->signalEvent(event, value);
  }
}

void Metal4Context::WaitEvent(MTL::Event* event, uint64_t value) {
  if (queue_ && event) {
    queue_->wait(event, value);
  }
}

void Metal4Context::SetVertexAddress(MTL::GPUAddress addr, uint32_t idx) {
  if (vertex_arg_table_) {
    vertex_arg_table_->setAddress(addr, idx);
    vertex_bindings_dirty_ = true;
  }
}

void Metal4Context::SetVertexAddressWithStride(MTL::GPUAddress addr,
                                               uint32_t stride,
                                               uint32_t idx) {
  if (vertex_arg_table_) {
    vertex_arg_table_->setAddress(addr, stride, idx);
    vertex_bindings_dirty_ = true;
  }
}

void Metal4Context::SetFragmentAddress(MTL::GPUAddress addr, uint32_t idx) {
  if (fragment_arg_table_) {
    fragment_arg_table_->setAddress(addr, idx);
    fragment_bindings_dirty_ = true;
  }
}

void Metal4Context::SetFragmentTexture(MTL::ResourceID resource_id,
                                       uint32_t idx) {
  if (fragment_arg_table_) {
    fragment_arg_table_->setTexture(resource_id, idx);
    fragment_bindings_dirty_ = true;
  }
}

void Metal4Context::SetFragmentSampler(MTL::ResourceID resource_id,
                                       uint32_t idx) {
  if (fragment_arg_table_) {
    fragment_arg_table_->setSamplerState(resource_id, idx);
    fragment_bindings_dirty_ = true;
  }
}

void Metal4Context::FlushRenderBindings(MTL4::RenderCommandEncoder* enc,
                                       bool include_mesh_stages) {
  if (!enc) return;
  if (vertex_bindings_dirty_) {
    MTL::RenderStages stages = MTL::RenderStageVertex;
    if (include_mesh_stages) {
      stages |= MTL::RenderStageObject | MTL::RenderStageMesh;
    }
    enc->setArgumentTable(vertex_arg_table_, stages);
    vertex_bindings_dirty_ = false;
    static int vf = 0;
    if (vf < 5) {
      fprintf(stderr, "[mtl4] FlushRender vertex: table=%p stages=%d\n",
              (void*)vertex_arg_table_, (int)stages);
      fflush(stderr);
      vf++;
    }
  }
  if (fragment_bindings_dirty_) {
    enc->setArgumentTable(fragment_arg_table_, MTL::RenderStageFragment);
    fragment_bindings_dirty_ = false;
    static int ff = 0;
    if (ff < 5) {
      fprintf(stderr, "[mtl4] FlushRender fragment: table=%p\n",
              (void*)fragment_arg_table_);
      fflush(stderr);
      ff++;
    }
  }
  CommitResidency();
}

void Metal4Context::SetComputeAddress(MTL::GPUAddress addr, uint32_t idx) {
  if (compute_arg_table_) {
    compute_arg_table_->setAddress(addr, idx);
    compute_bindings_dirty_ = true;
  }
}

void Metal4Context::SetComputeTexture(MTL::ResourceID resource_id,
                                      uint32_t idx) {
  if (compute_arg_table_) {
    compute_arg_table_->setTexture(resource_id, idx);
    compute_bindings_dirty_ = true;
  }
}

void Metal4Context::SetComputeSampler(MTL::ResourceID resource_id,
                                      uint32_t idx) {
  if (compute_arg_table_) {
    compute_arg_table_->setSamplerState(resource_id, idx);
    compute_bindings_dirty_ = true;
  }
}

void Metal4Context::FlushComputeBindings(MTL4::ComputeCommandEncoder* enc) {
  if (!enc) return;
  if (compute_bindings_dirty_) {
    enc->setArgumentTable(compute_arg_table_);
    compute_bindings_dirty_ = false;
  }
  CommitResidency();
}

MTL::GPUAddress Metal4Context::AllocInlineConstant(const void* data,
                                                    size_t len) {
  if (!inline_constants_buffer_ || !data) return 0;

  size_t aligned_len = (len + 255) & ~size_t(255);
  if (inline_constants_offset_ + aligned_len > kInlineConstantsSize) {
    inline_constants_offset_ = 0;
  }

  uint8_t* base =
      reinterpret_cast<uint8_t*>(inline_constants_buffer_->contents());
  std::memcpy(base + inline_constants_offset_, data, len);

  MTL::GPUAddress addr =
      inline_constants_buffer_->gpuAddress() + inline_constants_offset_;
  inline_constants_offset_ += aligned_len;
  return addr;
}

MTL4::RenderPassDescriptor* Metal4Context::CreateRenderPassDescriptor(
    MTL::RenderPassDescriptor* mtl3_desc) {
  if (!mtl3_desc) return nullptr;

  MTL4::RenderPassDescriptor* desc = MTL4::RenderPassDescriptor::alloc()->init();

  auto* src_colors = mtl3_desc->colorAttachments();
  auto* dst_colors = desc->colorAttachments();
  for (uint32_t i = 0; i < 4; ++i) {
    auto* src = src_colors ? src_colors->object(i) : nullptr;
    if (!src) continue;
    auto* dst = dst_colors->object(i);
    if (src->texture()) dst->setTexture(src->texture());
    if (src->resolveTexture()) dst->setResolveTexture(src->resolveTexture());
    dst->setLoadAction(src->loadAction());
    dst->setStoreAction(src->storeAction());
    dst->setClearColor(src->clearColor());
  }

  if (auto* src_depth = mtl3_desc->depthAttachment()) {
    auto* dst_depth = MTL::RenderPassDepthAttachmentDescriptor::alloc()->init();
    dst_depth->setTexture(src_depth->texture());
    dst_depth->setLoadAction(src_depth->loadAction());
    dst_depth->setStoreAction(src_depth->storeAction());
    dst_depth->setClearDepth(src_depth->clearDepth());
    desc->setDepthAttachment(dst_depth);
    dst_depth->release();
  }

  if (auto* src_stencil = mtl3_desc->stencilAttachment()) {
    auto* dst_stencil =
        MTL::RenderPassStencilAttachmentDescriptor::alloc()->init();
    dst_stencil->setTexture(src_stencil->texture());
    dst_stencil->setLoadAction(src_stencil->loadAction());
    dst_stencil->setStoreAction(src_stencil->storeAction());
    dst_stencil->setClearStencil(src_stencil->clearStencil());
    desc->setStencilAttachment(dst_stencil);
    dst_stencil->release();
  }

  return desc;
}

void Metal4Context::ResetFrameState() {
  inline_constants_offset_ = 0;
  allocator_->reset();
}

void Metal4Context::AddResidentAllocation(MTL::Allocation* alloc) {
  if (residency_set_ && alloc) {
    residency_set_->addAllocation(alloc);
    residency_dirty_ = true;
  }
}

void Metal4Context::CommitResidency() {
  if (residency_set_ && residency_dirty_) {
    residency_set_->commit();
    residency_dirty_ = false;
  }
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
