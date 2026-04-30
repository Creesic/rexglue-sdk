#include <rex/graphics/metal/command_processor.h>

#include <rex/graphics/graphics_system.h>
#include <rex/graphics/metal/bindings.h>
#include <rex/graphics/metal/shared_memory.h>
#include <rex/graphics/metal/texture_cache.h>
#include <rex/graphics/metal/render_target_cache.h>
#include <rex/graphics/metal/primitive_processor.h>
#include <rex/graphics/pipeline/shader/dxbc_translator.h>
#include <rex/graphics/util/draw.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/presenter.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/system/kernel_state.h>
#include <rex/logging/macros.h>
#include <rex/assert.h>
#include <rex/math.h>
#include <xxhash.h>

#include <algorithm>
#include <cstring>

#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime.h>

namespace {

constexpr bool kMetalVerboseDiagnostics = true;
constexpr bool kMetalDebugForceSolidFragment = true;
constexpr bool kMetalDebugForceSolidPipeline = false;
constexpr bool kMetalDebugForceDepthAlways = true;
constexpr bool kMetalDebugFillBeforeCopy = false;

void SetDescriptorBuffer(::IRDescriptorTableEntry* entry, uint64_t gpu_va, uint64_t size) {
  entry->gpuVA = gpu_va;
  entry->textureViewID = 0;
  entry->metadata = size;
}

void SetDescriptorTexture(::IRDescriptorTableEntry* entry, MTL::Texture* tex) {
  entry->gpuVA = 0;
  entry->textureViewID = tex->gpuResourceID()._impl;
  entry->metadata = 0;
}

void SetDescriptorSampler(::IRDescriptorTableEntry* entry, MTL::SamplerState* sampler) {
  entry->gpuVA = sampler->gpuResourceID()._impl;
  entry->textureViewID = 0;
  entry->metadata = 0;
}

bool GetMetalPrimitiveType(rex::graphics::xenos::PrimitiveType primitive_type,
                           MTL::PrimitiveType& metal_primitive_type_out) {
  switch (primitive_type) {
    case rex::graphics::xenos::PrimitiveType::kPointList:
      metal_primitive_type_out = MTL::PrimitiveTypePoint;
      return true;
    case rex::graphics::xenos::PrimitiveType::kLineList:
      metal_primitive_type_out = MTL::PrimitiveTypeLine;
      return true;
    case rex::graphics::xenos::PrimitiveType::kLineStrip:
      metal_primitive_type_out = MTL::PrimitiveTypeLineStrip;
      return true;
    case rex::graphics::xenos::PrimitiveType::kTriangleList:
    case rex::graphics::xenos::PrimitiveType::kRectangleList:
      metal_primitive_type_out = MTL::PrimitiveTypeTriangle;
      return true;
    case rex::graphics::xenos::PrimitiveType::kTriangleStrip:
      metal_primitive_type_out = MTL::PrimitiveTypeTriangleStrip;
      return true;
    default:
      return false;
  }
}

MTL::RenderPipelineState* GetOrCreateDebugFillPipeline(
    MTL::Device* device, MTL::PixelFormat color_format,
    MTL::PixelFormat depth_format, MTL::PixelFormat stencil_format) {
  struct Key {
    uint32_t color;
    uint32_t depth;
    uint32_t stencil;
    bool operator==(const Key& other) const {
      return color == other.color && depth == other.depth &&
             stencil == other.stencil;
    }
  };
  struct KeyHasher {
    size_t operator()(const Key& key) const {
      size_t hash = key.color;
      hash ^= size_t(key.depth) << 8;
      hash ^= size_t(key.stencil) << 16;
      return hash;
    }
  };
  static std::unordered_map<Key, MTL::RenderPipelineState*, KeyHasher> cache;
  Key key = {uint32_t(color_format), uint32_t(depth_format),
             uint32_t(stencil_format)};
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  constexpr const char* kSource = R"(
    #include <metal_stdlib>
    using namespace metal;
    struct VSOut {
      float4 position [[position]];
    };
    vertex VSOut rex_debug_fill_vs(uint vertex_id [[vertex_id]]) {
      float2 pos[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
      };
      VSOut out;
      out.position = float4(pos[vertex_id], 0.0, 1.0);
      return out;
    }
    fragment float4 rex_debug_fill_fs() {
      return float4(1.0, 0.0, 1.0, 1.0);
    }
  )";

  NS::Error* error = nullptr;
  MTL::Library* library = device->newLibrary(
      NS::String::string(kSource, NS::UTF8StringEncoding), nullptr, &error);
  if (!library) {
    fprintf(stderr, "[metal] DEBUG FILL library failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
    if (error) error->release();
    return nullptr;
  }
  MTL::Function* vertex_fn = library->newFunction(
      NS::String::string("rex_debug_fill_vs", NS::UTF8StringEncoding));
  MTL::Function* fragment_fn = library->newFunction(
      NS::String::string("rex_debug_fill_fs", NS::UTF8StringEncoding));
  MTL::RenderPipelineDescriptor* desc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vertex_fn);
  desc->setFragmentFunction(fragment_fn);
  desc->colorAttachments()->object(0)->setPixelFormat(color_format);
  if (depth_format != MTL::PixelFormatInvalid) {
    desc->setDepthAttachmentPixelFormat(depth_format);
  }
  if (stencil_format != MTL::PixelFormatInvalid) {
    desc->setStencilAttachmentPixelFormat(stencil_format);
  }
  MTL::RenderPipelineState* pipeline =
      device->newRenderPipelineState(desc, &error);
  if (!pipeline) {
    fprintf(stderr, "[metal] DEBUG FILL pipeline failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
  }
  desc->release();
  if (fragment_fn) fragment_fn->release();
  if (vertex_fn) vertex_fn->release();
  library->release();
  if (error) error->release();
  if (pipeline) {
    cache[key] = pipeline;
  }
  return pipeline;
}

MTL::Function* GetDebugSolidFragmentFunction(MTL::Device* device) {
  static MTL::Library* library = nullptr;
  static MTL::Function* function = nullptr;
  if (function) {
    return function;
  }

  constexpr const char* kSource = R"(
    #include <metal_stdlib>
    using namespace metal;
    fragment float4 rex_debug_solid_fs() {
      return float4(1.0, 0.0, 1.0, 1.0);
    }
  )";
  NS::Error* error = nullptr;
  library = device->newLibrary(
      NS::String::string(kSource, NS::UTF8StringEncoding), nullptr, &error);
  if (!library) {
    fprintf(stderr, "[metal] DEBUG SOLID FS library failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
    if (error) error->release();
    return nullptr;
  }
  function = library->newFunction(
      NS::String::string("rex_debug_solid_fs", NS::UTF8StringEncoding));
  if (!function) {
    fprintf(stderr, "[metal] DEBUG SOLID FS function failed\n");
  }
  if (error) error->release();
  return function;
}

MTL::RenderPipelineState* GetOrCreateDebugSolidFragmentPipeline(
    MTL::Device* device, MTL::Function* vertex_function,
    MTL::PixelFormat color_format, MTL::PixelFormat depth_format,
    MTL::PixelFormat stencil_format) {
  struct Key {
    uintptr_t vertex;
    uint32_t color;
    uint32_t depth;
    uint32_t stencil;
    bool operator==(const Key& other) const {
      return vertex == other.vertex && color == other.color &&
             depth == other.depth && stencil == other.stencil;
    }
  };
  struct KeyHasher {
    size_t operator()(const Key& key) const {
      size_t hash = key.vertex;
      hash ^= size_t(key.color) << 3;
      hash ^= size_t(key.depth) << 11;
      hash ^= size_t(key.stencil) << 19;
      return hash;
    }
  };
  static std::unordered_map<Key, MTL::RenderPipelineState*, KeyHasher> cache;
  Key key = {reinterpret_cast<uintptr_t>(vertex_function), uint32_t(color_format),
             uint32_t(depth_format), uint32_t(stencil_format)};
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  MTL::Function* fragment_function = GetDebugSolidFragmentFunction(device);
  if (!vertex_function || !fragment_function) {
    return nullptr;
  }
  MTL::RenderPipelineDescriptor* desc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vertex_function);
  desc->setFragmentFunction(fragment_function);
  desc->colorAttachments()->object(0)->setPixelFormat(color_format);
  if (depth_format != MTL::PixelFormatInvalid) {
    desc->setDepthAttachmentPixelFormat(depth_format);
  }
  if (stencil_format != MTL::PixelFormatInvalid) {
    desc->setStencilAttachmentPixelFormat(stencil_format);
  }

  NS::Error* error = nullptr;
  MTL::RenderPipelineState* pipeline =
      device->newRenderPipelineState(desc, &error);
  if (!pipeline) {
    fprintf(stderr, "[metal] DEBUG SOLID FS pipeline failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
  }
  desc->release();
  if (error) error->release();
  if (pipeline) {
    cache[key] = pipeline;
  }
  return pipeline;
}

MTL::RenderPipelineState* GetOrCreateDebugSolidPipeline(
    MTL::Device* device, MTL::PixelFormat color_format,
    MTL::PixelFormat depth_format, MTL::PixelFormat stencil_format) {
  struct Key {
    uint32_t color;
    uint32_t depth;
    uint32_t stencil;
    bool operator==(const Key& other) const {
      return color == other.color && depth == other.depth &&
             stencil == other.stencil;
    }
  };
  struct KeyHasher {
    size_t operator()(const Key& key) const {
      size_t hash = key.color;
      hash ^= size_t(key.depth) << 8;
      hash ^= size_t(key.stencil) << 16;
      return hash;
    }
  };
  static std::unordered_map<Key, MTL::RenderPipelineState*, KeyHasher> cache;
  Key key = {uint32_t(color_format), uint32_t(depth_format),
             uint32_t(stencil_format)};
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  constexpr const char* kSource = R"(
    #include <metal_stdlib>
    using namespace metal;
    struct VSOut {
      float4 position [[position]];
    };
    vertex VSOut rex_debug_draw_vs(uint vertex_id [[vertex_id]]) {
      float2 pos[4] = {
        float2(-1.0, -1.0),
        float2(-1.0,  1.0),
        float2( 1.0, -1.0),
        float2( 1.0,  1.0)
      };
      VSOut out;
      out.position = float4(pos[vertex_id & 3u], 0.0, 1.0);
      return out;
    }
    fragment float4 rex_debug_draw_fs() {
      return float4(1.0, 0.0, 1.0, 1.0);
    }
  )";
  NS::Error* error = nullptr;
  MTL::Library* library = device->newLibrary(
      NS::String::string(kSource, NS::UTF8StringEncoding), nullptr, &error);
  if (!library) {
    fprintf(stderr, "[metal] DEBUG DRAW library failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
    if (error) error->release();
    return nullptr;
  }
  MTL::Function* vertex_fn = library->newFunction(
      NS::String::string("rex_debug_draw_vs", NS::UTF8StringEncoding));
  MTL::Function* fragment_fn = library->newFunction(
      NS::String::string("rex_debug_draw_fs", NS::UTF8StringEncoding));
  MTL::RenderPipelineDescriptor* desc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vertex_fn);
  desc->setFragmentFunction(fragment_fn);
  desc->colorAttachments()->object(0)->setPixelFormat(color_format);
  if (depth_format != MTL::PixelFormatInvalid) {
    desc->setDepthAttachmentPixelFormat(depth_format);
  }
  if (stencil_format != MTL::PixelFormatInvalid) {
    desc->setStencilAttachmentPixelFormat(stencil_format);
  }
  MTL::RenderPipelineState* pipeline =
      device->newRenderPipelineState(desc, &error);
  if (!pipeline) {
    fprintf(stderr, "[metal] DEBUG DRAW pipeline failed: %s\n",
            error ? error->localizedDescription()->utf8String() : "<unknown>");
  }
  desc->release();
  if (fragment_fn) fragment_fn->release();
  if (vertex_fn) vertex_fn->release();
  library->release();
  if (error) error->release();
  if (pipeline) {
    cache[key] = pipeline;
  }
  return pipeline;
}

}

namespace rex {
namespace graphics {
namespace metal {

MetalCommandProcessor::MetalCommandProcessor(
    GraphicsSystem* graphics_system, system::KernelState* kernel_state)
    : CommandProcessor(graphics_system, kernel_state) {}

MetalCommandProcessor::~MetalCommandProcessor() { ShutdownContext(); }

rex::ui::metal::MetalProvider& MetalCommandProcessor::GetMetalProvider() const {
  return *static_cast<rex::ui::metal::MetalProvider*>(graphics_system_->provider());
}

uint64_t MetalCommandProcessor::GetCurrentSubmission() const {
  return submission_current_;
}

uint64_t MetalCommandProcessor::GetCompletedSubmission() const {
  return completed_command_buffers_.load();
}

bool MetalCommandProcessor::SetupContext() {
  if (!graphics_system_ || !graphics_system_->provider()) {
    return false;
  }

  auto& provider = *static_cast<rex::ui::metal::MetalProvider*>(graphics_system_->provider());
  device_ = provider.GetDevice();
  command_queue_ = provider.GetCommandQueue();
  if (!device_ || !command_queue_) {
    return false;
  }

  mesh_shader_supported_ = device_->supportsFamily(MTL::GPUFamilyApple6);

  wait_shared_event_ = device_->newSharedEvent();
  if (!wait_shared_event_) {
    return false;
  }

  null_buffer_ = device_->newBuffer(kNullBufferSize, MTL::ResourceStorageModeShared);
  {
    MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA8Unorm, 1, 1, MTL::TextureUsageShaderRead);
    null_texture_ = device_->newTexture(desc);
    desc->release();
  }
  {
    MTL::SamplerDescriptor* desc = MTL::SamplerDescriptor::alloc()->init();
    null_sampler_ = device_->newSamplerState(desc);
    desc->release();
  }

  uniforms_ring_buffer_ = device_->newBuffer(kUniformsRingSize, MTL::ResourceStorageModeShared);
  if (uniforms_ring_buffer_) {
    uniforms_ring_data_ = static_cast<uint8_t*>(uniforms_ring_buffer_->contents());
  }

  draw_ring_pool_ = device_->newBuffer(kDrawRingPoolSize, MTL::ResourceStorageModeShared);
  if (draw_ring_pool_) {
    draw_ring_data_ = static_cast<uint8_t*>(draw_ring_pool_->contents());
  }

  {
    using namespace MscHeapLayout;
    const size_t kResourceHeapBytes =
        (kResourceHeapSlotsPerTable + kResourceHeapSlotsPerTable) *
        sizeof(IRDescriptorTableEntry);
    res_heap_ab_ = device_->newBuffer(kResourceHeapBytes, MTL::ResourceStorageModeShared);
    if (!res_heap_ab_) return false;

    auto* res_entries = reinterpret_cast<IRDescriptorTableEntry*>(res_heap_ab_->contents());
    auto* uav_entries = res_entries + kResourceHeapSlotsPerTable;

    SetDescriptorBuffer(&res_entries[0], null_buffer_->gpuAddress(), kNullBufferSize);
    for (size_t i = 1; i < kResourceHeapSlotsPerTable; ++i) {
      SetDescriptorTexture(&res_entries[i], null_texture_);
    }
    for (size_t i = 0; i < kResourceHeapSlotsPerTable; ++i) {
      SetDescriptorBuffer(&uav_entries[i], null_buffer_->gpuAddress(), kNullBufferSize);
    }

    const size_t kSamplerHeapBytes = kSamplerHeapSlotsPerTable * sizeof(IRDescriptorTableEntry);
    smp_heap_ab_ = device_->newBuffer(kSamplerHeapBytes, MTL::ResourceStorageModeShared);
    if (!smp_heap_ab_) return false;
    auto* smp_entries = reinterpret_cast<IRDescriptorTableEntry*>(smp_heap_ab_->contents());
    for (size_t i = 0; i < kSamplerHeapSlotsPerTable; ++i) {
      SetDescriptorSampler(&smp_entries[i], null_sampler_);
    }

    const size_t kCBVHeapBytes = kCbvHeapSlotsPerTable * sizeof(IRDescriptorTableEntry);
    vs_cbv_heap_ab_ = device_->newBuffer(kCBVHeapBytes, MTL::ResourceStorageModeShared);
    if (!vs_cbv_heap_ab_) return false;
    std::memset(vs_cbv_heap_ab_->contents(), 0, kCBVHeapBytes);

    ps_cbv_heap_ab_ = device_->newBuffer(kCBVHeapBytes, MTL::ResourceStorageModeShared);
    if (!ps_cbv_heap_ab_) return false;
    std::memset(ps_cbv_heap_ab_->contents(), 0, kCBVHeapBytes);

    const size_t kTopLevelABBytes = kTopLevelABSlots * sizeof(uint64_t);
    vs_top_level_ab_ = device_->newBuffer(kTopLevelABBytes, MTL::ResourceStorageModeShared);
    if (!vs_top_level_ab_) return false;
    ps_top_level_ab_ = device_->newBuffer(kTopLevelABBytes, MTL::ResourceStorageModeShared);
    if (!ps_top_level_ab_) return false;

    auto* vs_top_ptrs = reinterpret_cast<uint64_t*>(vs_top_level_ab_->contents());
    auto* ps_top_ptrs = reinterpret_cast<uint64_t*>(ps_top_level_ab_->contents());
    std::memset(vs_top_ptrs, 0, kTopLevelABBytes);
    std::memset(ps_top_ptrs, 0, kTopLevelABBytes);

    uint64_t srv_base = res_heap_ab_->gpuAddress();
    uint64_t srv_texture_base =
        srv_base + sizeof(IRDescriptorTableEntry);
    uint64_t uav_base = res_heap_ab_->gpuAddress() +
                        kResourceHeapSlotsPerTable * sizeof(IRDescriptorTableEntry);
    uint64_t smp_base = smp_heap_ab_->gpuAddress();
    uint64_t vs_cbv_base = vs_cbv_heap_ab_->gpuAddress();
    uint64_t ps_cbv_base = ps_cbv_heap_ab_->gpuAddress();

    vs_top_ptrs[0] = srv_base;
    ps_top_ptrs[0] = srv_base;
    for (int i = 1; i < 4; ++i) {
      vs_top_ptrs[i] = srv_texture_base;
      ps_top_ptrs[i] = srv_texture_base;
    }
    vs_top_ptrs[4] = srv_base;
    ps_top_ptrs[4] = srv_base;
    for (int i = 5; i < 9; ++i) {
      vs_top_ptrs[i] = uav_base;
      ps_top_ptrs[i] = uav_base;
    }
    vs_top_ptrs[9] = smp_base;
    ps_top_ptrs[9] = smp_base;
    for (int i = 10; i < 14; ++i) {
      vs_top_ptrs[i] = vs_cbv_base;
      ps_top_ptrs[i] = ps_cbv_base;
    }
    vs_top_ptrs[14] = vs_cbv_base;
    ps_top_ptrs[14] = ps_cbv_base;
  }

  shared_memory_ = std::make_unique<MetalSharedMemory>(*this,
      *graphics_system_->kernel_state()->memory());
  if (!shared_memory_->Initialize()) {
    return false;
  }

  primitive_processor_ = std::make_unique<MetalPrimitiveProcessor>(
      *this, *register_file_,
      *graphics_system_->kernel_state()->memory(),
      trace_writer_, *shared_memory_);
  if (!primitive_processor_->Initialize()) {
    return false;
  }

  texture_cache_ = std::make_unique<MetalTextureCache>(
      *register_file_, *shared_memory_, 1, 1, *this);

  render_target_cache_ = std::make_unique<MetalRenderTargetCache>(
      *register_file_,
      *graphics_system_->kernel_state()->memory(),
      trace_writer_, 1, 1, *this);
  if (!render_target_cache_->Initialize()) {
    return false;
  }

  if (!InitializeShaderTranslation()) {
    return false;
  }

  return true;
}

void MetalCommandProcessor::ShutdownContext() {
  EndRenderEncoder();
  EndCommandBuffer();

  render_target_cache_.reset();
  texture_cache_.reset();
  primitive_processor_.reset();
  shared_memory_.reset();

  for (auto& [key, state] : depth_stencil_state_cache_) {
    if (state) state->release();
  }
  depth_stencil_state_cache_.clear();
  pipeline_state_cache_.clear();

  if (vs_top_level_ab_) { vs_top_level_ab_->release(); vs_top_level_ab_ = nullptr; }
  if (ps_top_level_ab_) { ps_top_level_ab_->release(); ps_top_level_ab_ = nullptr; }
  if (vs_cbv_heap_ab_) { vs_cbv_heap_ab_->release(); vs_cbv_heap_ab_ = nullptr; }
  if (ps_cbv_heap_ab_) { ps_cbv_heap_ab_->release(); ps_cbv_heap_ab_ = nullptr; }
  if (smp_heap_ab_) { smp_heap_ab_->release(); smp_heap_ab_ = nullptr; }
  if (res_heap_ab_) { res_heap_ab_->release(); res_heap_ab_ = nullptr; }
  if (draw_ring_pool_) { draw_ring_pool_->release(); draw_ring_pool_ = nullptr; }
  if (uniforms_ring_buffer_) { uniforms_ring_buffer_->release(); uniforms_ring_buffer_ = nullptr; }
  if (resolved_frontbuffer_texture_) {
    resolved_frontbuffer_texture_->release();
    resolved_frontbuffer_texture_ = nullptr;
  }
  if (present_texture_) { present_texture_->release(); present_texture_ = nullptr; }
  if (null_sampler_) { null_sampler_->release(); null_sampler_ = nullptr; }
  if (null_texture_) { null_texture_->release(); null_texture_ = nullptr; }
  if (null_buffer_) { null_buffer_->release(); null_buffer_ = nullptr; }
  if (wait_shared_event_) { wait_shared_event_->release(); wait_shared_event_ = nullptr; }
  command_queue_ = nullptr;
  device_ = nullptr;
}

bool MetalCommandProcessor::InitializeShaderTranslation() {
  dxbc_to_dxil_converter_ = std::make_unique<DxbcToDxilConverter>();
  if (!dxbc_to_dxil_converter_->Initialize()) {
    return false;
  }

  metal_shader_converter_ = std::make_unique<MetalShaderConverter>();
  if (!metal_shader_converter_->Initialize()) {
    return false;
  }

  shader_translator_ = std::make_unique<DxbcShaderTranslator>(
      ui::GraphicsProvider::GpuVendorID::kApple,
      false,  // bindless_resources_used
      false,  // edram_rov_used
      true,   // gamma_render_target_as_unorm8
      false,  // msaa_2x_supported
      1,      // draw_resolution_scale_x
      1,      // draw_resolution_scale_y
      false); // force_emit_source_map
  return true;
}

MTL::CommandBuffer* MetalCommandProcessor::EnsureCommandBuffer() {
  if (!current_command_buffer_) {
    BeginCommandBuffer();
  }
  return current_command_buffer_;
}

void MetalCommandProcessor::BeginCommandBuffer() {
  if (current_command_buffer_) return;
  command_buffer_autorelease_pool_ = NS::AutoreleasePool::alloc()->init();
  current_command_buffer_ = command_queue_->commandBuffer();
  if (!current_command_buffer_) {
    REXLOG_ERROR("MetalCommandProcessor: Failed to create command buffer");
    return;
  }
  current_command_buffer_->setLabel(
      NS::String::string("ReX Command Buffer", NS::UTF8StringEncoding));
}

void MetalCommandProcessor::EndCommandBuffer() {
  if (!current_command_buffer_) return;
  EndRenderEncoder();

  uint64_t signal_value = ++submission_current_;
  current_command_buffer_->encodeSignalEvent(wait_shared_event_, signal_value);

  current_command_buffer_->addCompletedHandler(
      ^(MTL::CommandBuffer* buffer) {
        completed_command_buffers_.store(signal_value);
      });

  current_command_buffer_->commit();
  current_command_buffer_ = nullptr;
  uniforms_ring_offset_ = 0;
  draw_ring_offset_ = 0;

  if (command_buffer_autorelease_pool_) {
    command_buffer_autorelease_pool_ = nullptr;
  }
}

void MetalCommandProcessor::EndRenderEncoder() {
  if (current_render_encoder_) {
    current_render_encoder_->endEncoding();
    current_render_encoder_->release();
    current_render_encoder_ = nullptr;
  }
  if (current_render_pass_descriptor_) {
    current_render_pass_descriptor_->release();
    current_render_pass_descriptor_ = nullptr;
  }
  render_encoder_resource_usage_.clear();
  render_encoder_heap_usage_.clear();
}

void MetalCommandProcessor::ResetRenderEncoderStateCache() {
  bound_pipeline_state_ = nullptr;
}

void MetalCommandProcessor::PrepareForWait() {
  EndCommandBuffer();
}

void MetalCommandProcessor::OnPrimaryBufferEnd() {}

void MetalCommandProcessor::OnGammaRamp256EntryTableValueWritten() {}
void MetalCommandProcessor::OnGammaRampPWLValueWritten() {}

Shader* MetalCommandProcessor::LoadShader(xenos::ShaderType shader_type,
                                          uint32_t guest_address,
                                          const uint32_t* host_address,
                                          uint32_t dword_count) {
  uint64_t hash = XXH3_64bits(host_address, dword_count * sizeof(uint32_t));
  auto it = shader_cache_.find(hash);
  if (it != shader_cache_.end()) return it->second.get();

  auto shader = std::make_unique<MetalShader>(
      shader_type, hash, host_address, dword_count);
  MetalShader* ptr = shader.get();
  shader_cache_[hash] = std::move(shader);
  return ptr;
}

DxbcShaderTranslator::Modification
MetalCommandProcessor::GetCurrentVertexShaderModification(
    const Shader& shader, Shader::HostVertexShaderType host_vertex_shader_type,
    uint32_t interpolator_mask) const {
  assert_true(shader.type() == xenos::ShaderType::kVertex);
  assert_true(shader.is_ucode_analyzed());
  const RegisterFile& regs = *register_file_;

  DxbcShaderTranslator::Modification mod(
      shader_translator_->GetDefaultVertexShaderModification(
          shader.GetDynamicAddressableRegisterCount(
              regs.Get<reg::SQ_PROGRAM_CNTL>().vs_num_reg),
          host_vertex_shader_type));
  mod.vertex.host_vertex_shader_type = host_vertex_shader_type;
  mod.vertex.interpolator_mask = interpolator_mask;

  auto pa_cl_clip_cntl = regs.Get<reg::PA_CL_CLIP_CNTL>();
  uint32_t user_clip_planes =
      pa_cl_clip_cntl.clip_disable ? 0 : pa_cl_clip_cntl.ucp_ena;
  mod.vertex.user_clip_plane_count = rex::bit_count(user_clip_planes);
  mod.vertex.user_clip_plane_cull =
      uint32_t(user_clip_planes && pa_cl_clip_cntl.ucp_cull_only_ena);
  mod.vertex.point_ps_ucp_mode = pa_cl_clip_cntl.ps_ucp_mode;
  mod.vertex.vertex_kill_and =
      uint32_t((shader.writes_point_size_edge_flag_kill_vertex() & 0b100) &&
               !pa_cl_clip_cntl.vtx_kill_or);
  mod.vertex.output_point_size =
      uint32_t((shader.writes_point_size_edge_flag_kill_vertex() & 0b001) &&
               regs.Get<reg::VGT_DRAW_INITIATOR>().prim_type ==
                   xenos::PrimitiveType::kPointList);
  return mod;
}

DxbcShaderTranslator::Modification
MetalCommandProcessor::GetCurrentPixelShaderModification(
    const Shader& shader, uint32_t interpolator_mask, uint32_t param_gen_pos,
    reg::RB_DEPTHCONTROL normalized_depth_control,
    uint32_t normalized_color_mask) const {
  assert_true(shader.type() == xenos::ShaderType::kPixel);
  assert_true(shader.is_ucode_analyzed());
  const RegisterFile& regs = *register_file_;

  DxbcShaderTranslator::Modification mod(
      shader_translator_->GetDefaultPixelShaderModification(
          shader.GetDynamicAddressableRegisterCount(
              regs.Get<reg::SQ_PROGRAM_CNTL>().ps_num_reg)));
  mod.pixel.interpolator_mask = interpolator_mask;
  mod.pixel.interpolators_centroid =
      interpolator_mask &
      ~xenos::GetInterpolatorSamplingPattern(
          regs.Get<reg::RB_SURFACE_INFO>().msaa_samples,
          regs.Get<reg::SQ_CONTEXT_MISC>().sc_sample_cntl,
          regs.Get<reg::SQ_INTERPOLATOR_CNTL>().sampling_pattern);

  if (param_gen_pos < xenos::kMaxInterpolators) {
    mod.pixel.param_gen_enable = 1;
    mod.pixel.param_gen_interpolator = param_gen_pos;
    mod.pixel.param_gen_point =
        uint32_t(regs.Get<reg::VGT_DRAW_INITIATOR>().prim_type ==
                 xenos::PrimitiveType::kPointList);
  } else {
    mod.pixel.param_gen_enable = 0;
    mod.pixel.param_gen_interpolator = 0;
    mod.pixel.param_gen_point = 0;
  }

  using DepthStencilMode = DxbcShaderTranslator::Modification::DepthStencilMode;
  if (shader.implicit_early_z_write_allowed() &&
      (!shader.writes_color_target(0) ||
       !draw_util::DoesCoverageDependOnAlpha(
           regs.Get<reg::RB_COLORCONTROL>()))) {
    mod.pixel.depth_stencil_mode = DepthStencilMode::kEarlyHint;
  } else {
    mod.pixel.depth_stencil_mode = DepthStencilMode::kNoModifiers;
  }
  return mod;
}

bool MetalCommandProcessor::IssueDraw(xenos::PrimitiveType primitive_type,
                                       uint32_t index_count,
                                       IndexBufferInfo* index_buffer_info,
                                       bool major_mode_explicit) {
  static std::atomic<int> draw_count{0};
  int dc = draw_count.fetch_add(1);

  const RegisterFile& regs = *register_file_;
  uint32_t normalized_color_mask = 0;

  xenos::EdramMode edram_mode = regs.Get<reg::RB_MODECONTROL>().edram_mode;
  if (edram_mode != xenos::EdramMode::kCopy && copy_resolve_writes_pending_) {
    EndCommandBuffer();
  }
  if (edram_mode == xenos::EdramMode::kCopy) {
    return IssueCopy();
  }

  static std::atomic<int> non_copy_count{0};
  static std::atomic<int> no_vs_count{0};
  static std::atomic<int> skip_path_count{0};
  static std::atomic<int> metal_draw_count{0};
  static std::atomic<int> draw_diag_count{0};
  static std::atomic<int> fail_reason{0};

  Shader* vertex_shader = active_vertex_shader();
  if (!vertex_shader) {
    int nv = no_vs_count.fetch_add(1);
    if constexpr (kMetalVerboseDiagnostics) {
    if (nv < 3) {
      fprintf(stderr, "[metal] DIAG: no vertex shader #%d\n", nv); fflush(stderr);
    }
    }
    return true;
  }
  if (!vertex_shader->is_ucode_analyzed()) {
    vertex_shader->AnalyzeUcode(ucode_disasm_buffer_);
  }
  bool memexport_used_vertex = vertex_shader->memexport_eM_written() != 0;

  bool primitive_polygonal = draw_util::IsPrimitivePolygonal(regs);
  bool is_rasterization_done =
      draw_util::IsRasterizationPotentiallyDone(regs, primitive_polygonal);
  Shader* pixel_shader = nullptr;
  if (is_rasterization_done) {
    if (edram_mode == xenos::EdramMode::kColorDepth) {
      pixel_shader = active_pixel_shader();
      if (pixel_shader) {
        if (!pixel_shader->is_ucode_analyzed()) {
          pixel_shader->AnalyzeUcode(ucode_disasm_buffer_);
        }
        if (!draw_util::IsPixelShaderNeededWithRasterization(*pixel_shader, regs)) {
          pixel_shader = nullptr;
        }
      }
    }
  } else {
    if (!memexport_used_vertex) {
      int sp = skip_path_count.fetch_add(1);
      if constexpr (kMetalVerboseDiagnostics) {
      if (sp < 3) {
        fprintf(stderr, "[metal] DIAG: skip no_raster+no_memexport #%d\n", sp); fflush(stderr);
      }
      }
      return true;
    }
  }

  bool memexport_used_pixel =
      pixel_shader && (pixel_shader->memexport_eM_written() != 0);
  bool memexport_used = memexport_used_vertex || memexport_used_pixel;
  memexport_ranges_.clear();
  if (memexport_used_vertex) {
    draw_util::AddMemExportRanges(regs, *vertex_shader, memexport_ranges_);
  }
  if (memexport_used_pixel) {
    draw_util::AddMemExportRanges(regs, *pixel_shader, memexport_ranges_);
  }
  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> memexport_diag{0};
    int med = memexport_diag.fetch_add(1);
    if ((memexport_used || med < 8) && med < 80) {
      fprintf(stderr,
              "[metal] MEMEXPORT DIAG #%d: used_v=%d used_p=%d ranges=%zu raster=%d\n",
              med, int(memexport_used_vertex), int(memexport_used_pixel),
              memexport_ranges_.size(), int(is_rasterization_done));
      for (size_t i = 0; i < memexport_ranges_.size() && i < 4; ++i) {
        fprintf(stderr, "[metal]   range[%zu]=0x%08X bytes=%u\n", i,
                memexport_ranges_[i].base_address_dwords << 2,
                memexport_ranges_[i].size_bytes);
      }
      fflush(stderr);
    }
  }

  PrimitiveProcessor::ProcessingResult primitive_processing_result;
  if (!primitive_processor_) {
    REXLOG_ERROR("IssueDraw: primitive processor not initialized");
    return false;
  }
  if (!primitive_processor_->Process(primitive_processing_result)) {
    REXLOG_ERROR("IssueDraw: primitive processing failed");
    return false;
  }
  if constexpr (kMetalVerboseDiagnostics) {
  if (dc < 60) {
    fprintf(stderr, "[metal] IssueDraw #%d: prim=%d vtx=%u vs=%p ps=%p host_vs_type=%d\n",
            dc, (int)primitive_type,
            primitive_processing_result.host_draw_vertex_count,
            vertex_shader, pixel_shader,
            (int)primitive_processing_result.host_vertex_shader_type);
    fflush(stderr);
  }
  }
  if (!primitive_processing_result.host_draw_vertex_count) {
    int sp = skip_path_count.fetch_add(1);
    if constexpr (kMetalVerboseDiagnostics) {
    if (sp < 3) {
      fprintf(stderr, "[metal] DIAG: skip 0 vertex count #%d\n", sp); fflush(stderr);
    }
    }
    return true;
  }

  if (render_target_cache_) {
    auto normalized_depth_control = draw_util::GetNormalizedDepthControl(regs);
    uint32_t ps_writes_color_targets =
        pixel_shader ? pixel_shader->writes_color_targets() : 0;
    normalized_color_mask = pixel_shader
        ? draw_util::GetNormalizedColorMask(regs, ps_writes_color_targets) : 0;
    if (!render_target_cache_->Update(is_rasterization_done,
                                      normalized_depth_control,
                                      normalized_color_mask, *vertex_shader)) {
      REXLOG_ERROR("IssueDraw: RenderTargetCache::Update failed");
      return false;
    }
  }

  BeginCommandBuffer();

  if (!current_command_buffer_ || !current_render_encoder_) {
    REXLOG_ERROR("IssueDraw: no command buffer or render encoder");
    return false;
  }

  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> post_rt_diag{0};
    int prd = post_rt_diag.fetch_add(1);
    if (prd < 10) {
      fprintf(stderr, "[metal] DIAG: post-RT #%d prim=%d host_vs_type=%d cb=%p enc=%p\n",
              prd, (int)primitive_type,
              (int)primitive_processing_result.host_vertex_shader_type,
              current_command_buffer_, current_render_encoder_);
      fflush(stderr);
    }
  }

  auto* metal_vertex_shader = static_cast<MetalShader*>(vertex_shader);
  auto* metal_pixel_shader = static_cast<MetalShader*>(pixel_shader);

  uint32_t ps_param_gen_pos = UINT32_MAX;
  uint32_t interpolator_mask = 0;
  if (pixel_shader) {
    interpolator_mask = vertex_shader->writes_interpolators() &
        pixel_shader->GetInterpolatorInputMask(
            regs.Get<reg::SQ_PROGRAM_CNTL>(),
            regs.Get<reg::SQ_CONTEXT_MISC>(), ps_param_gen_pos);
  }

  auto normalized_depth_control = draw_util::GetNormalizedDepthControl(regs);
  Shader::HostVertexShaderType host_vs_type =
      primitive_processing_result.host_vertex_shader_type;

  DxbcShaderTranslator::Modification vertex_mod =
      GetCurrentVertexShaderModification(*vertex_shader, host_vs_type, interpolator_mask);
   DxbcShaderTranslator::Modification pixel_mod =
       pixel_shader ? GetCurrentPixelShaderModification(
                         *pixel_shader, interpolator_mask, ps_param_gen_pos,
                         normalized_depth_control, normalized_color_mask)
                   : DxbcShaderTranslator::Modification(0);

  auto vertex_translation = static_cast<MetalShader::MetalTranslation*>(
      vertex_shader->GetOrCreateTranslation(vertex_mod.value));
  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> vs_trans_diag{0};
    int vtd = vs_trans_diag.fetch_add(1);
    if (vtd < 10) {
      fprintf(stderr, "[metal] DIAG: vs_trans #%d host_vs_type=%d mod=0x%016llX translated=%d valid=%d\n",
              vtd, (int)host_vs_type, (unsigned long long)vertex_mod.value,
              (int)vertex_translation->is_translated(),
              (int)vertex_translation->is_valid());
      fflush(stderr);
    }
  }
  if (!vertex_translation->is_translated()) {
    if (!shader_translator_->TranslateAnalyzedShader(*vertex_translation)) {
      fprintf(stderr, "[metal] DIAG: DXBC translation FAILED for vs host_vs_type=%d\n", (int)host_vs_type);
      fflush(stderr);
      REXLOG_ERROR("Failed to translate vertex shader to DXBC");
      return false;
    }
    if constexpr (kMetalVerboseDiagnostics) {
      static std::atomic<int> dxbc_ok_diag{0};
      int dod = dxbc_ok_diag.fetch_add(1);
      if (dod < 10) {
        fprintf(stderr, "[metal] DIAG: DXBC translation OK #%d host_vs_type=%d\n", dod, (int)host_vs_type);
        fflush(stderr);
      }
    }
  }
  if (!vertex_translation->is_valid()) {
    if (!vertex_translation->TranslateToMetal(device_, *dxbc_to_dxil_converter_,
                                               *metal_shader_converter_)) {
      fprintf(stderr, "[metal] DIAG: Metal translation FAILED for vs host_vs_type=%d\n", (int)host_vs_type);
      fflush(stderr);
      REXLOG_ERROR("Failed to translate vertex shader to Metal");
      return false;
    }
    if constexpr (kMetalVerboseDiagnostics) {
      static std::atomic<int> metal_ok_diag{0};
      int mod2 = metal_ok_diag.fetch_add(1);
      if (mod2 < 10) {
        fprintf(stderr, "[metal] DIAG: Metal translation OK #%d host_vs_type=%d\n", mod2, (int)host_vs_type);
        fflush(stderr);
      }
    }
  }

  MetalShader::MetalTranslation* pixel_translation = nullptr;
  if (pixel_shader) {
    pixel_translation = static_cast<MetalShader::MetalTranslation*>(
        pixel_shader->GetOrCreateTranslation(pixel_mod.value));
    if constexpr (kMetalVerboseDiagnostics) {
      static std::atomic<int> ps_trans_diag{0};
      int ptd = ps_trans_diag.fetch_add(1);
      if (ptd < 10) {
        fprintf(stderr, "[metal] DIAG: ps_trans #%d translated=%d valid=%d\n",
                ptd, (int)pixel_translation->is_translated(),
                (int)pixel_translation->is_valid());
        fflush(stderr);
      }
    }
    if (!pixel_translation->is_translated()) {
      if (!shader_translator_->TranslateAnalyzedShader(*pixel_translation)) {
        fprintf(stderr, "[metal] DIAG: PS DXBC translation FAILED\n"); fflush(stderr);
        return false;
      }
    }
    if (!pixel_translation->is_valid()) {
      if (!pixel_translation->TranslateToMetal(
              device_, *dxbc_to_dxil_converter_, *metal_shader_converter_)) {
        fprintf(stderr, "[metal] DIAG: PS Metal translation FAILED\n"); fflush(stderr);
        return false;
      }
    }
  }

  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> pre_pipe_diag{0};
    int ppd = pre_pipe_diag.fetch_add(1);
    if (ppd < 10) {
      fprintf(stderr, "[metal] DIAG: pre-pipeline #%d host_vs_type=%d vs_valid=%d ps_valid=%d\n",
              ppd, (int)host_vs_type,
              (int)vertex_translation->is_valid(),
              pixel_translation ? (int)pixel_translation->is_valid() : -1);
      fflush(stderr);
    }
  }

  MslPipelineCompileStatus pipeline_status = MslPipelineCompileStatus::kFailed;
  MTL::RenderPipelineState* pipeline = GetOrCreatePipelineState(
      vertex_translation, pixel_translation, regs, &pipeline_status);
  if (!pipeline) {
    if constexpr (kMetalVerboseDiagnostics) {
    int pd = draw_diag_count.fetch_add(1);
    if (pd < 10) {
      fprintf(stderr, "[metal] DIAG: pipeline FAILED #%d prim=%d host_vs_type=%d vs_mod=0x%016llX status=%d\n",
              pd, (int)primitive_type,
              (int)primitive_processing_result.host_vertex_shader_type,
              (unsigned long long)vertex_mod.value,
              (int)pipeline_status);
      fflush(stderr);
    }
    }
    return false;
  }

  uint32_t used_texture_mask = metal_vertex_shader->GetUsedTextureMaskAfterTranslation();
  if (metal_pixel_shader) {
    used_texture_mask |= metal_pixel_shader->GetUsedTextureMaskAfterTranslation();
  }
  if (texture_cache_ && used_texture_mask) {
    if (texture_cache_->AnyUsedTextureRequestWorkPending(used_texture_mask)) {
    }
    texture_cache_->RequestTextures(used_texture_mask);
  }

  draw_util::ViewportInfo viewport_info;
  draw_util::GetHostViewportInfo(
      regs, 1, 1, true, 16384, 16384, false, normalized_depth_control,
      false, true, pixel_shader && pixel_shader->writes_depth(), viewport_info);

  if (shared_memory_) {
    const auto& constant_map_vertex = vertex_shader->constant_register_map();
    for (uint32_t i = 0;
         i < rex::countof(constant_map_vertex.vertex_fetch_bitmap); ++i) {
      uint32_t vfetch_bits_remaining =
          constant_map_vertex.vertex_fetch_bitmap[i];
      uint32_t j;
      while (rex::bit_scan_forward(vfetch_bits_remaining, &j)) {
        vfetch_bits_remaining &= ~(uint32_t(1) << j);
        uint32_t vfetch_index = i * 32 + j;
        xenos::xe_gpu_vertex_fetch_t vfetch = regs.GetVertexFetch(vfetch_index);
        if (vfetch.type != xenos::FetchConstantType::kVertex &&
            vfetch.type != xenos::FetchConstantType::kInvalidVertex) {
          REXLOG_WARN("Vertex fetch constant {} is invalid ({:08X} {:08X})",
                      vfetch_index, vfetch.dword_0, vfetch.dword_1);
          return false;
        }
        uint32_t buffer_offset = vfetch.address << 2;
        uint32_t buffer_length = vfetch.size << 2;
        if constexpr (kMetalVerboseDiagnostics) {
          static std::atomic<int> vfetch_diag{0};
          int vfd = vfetch_diag.fetch_add(1);
          if (vfd < 24) {
            const uint32_t* phys = reinterpret_cast<const uint32_t*>(
                memory_->TranslatePhysical(buffer_offset));
            const uint32_t* gpu = reinterpret_cast<const uint32_t*>(
                static_cast<const uint8_t*>(shared_memory_->GetGuestRamPtr(0)) +
                buffer_offset);
            fprintf(stderr,
                    "[metal] VFETCH DIAG #%d: fc=%u raw=%08X,%08X addr=0x%08X len=%u "
                    "phys=%08X %08X %08X %08X gpu=%08X %08X %08X %08X\n",
                    vfd, vfetch_index, vfetch.dword_0, vfetch.dword_1,
                    buffer_offset, buffer_length,
                    buffer_length >= 4 ? phys[0] : 0,
                    buffer_length >= 8 ? phys[1] : 0,
                    buffer_length >= 12 ? phys[2] : 0,
                    buffer_length >= 16 ? phys[3] : 0,
                    buffer_length >= 4 ? gpu[0] : 0,
                    buffer_length >= 8 ? gpu[1] : 0,
                    buffer_length >= 12 ? gpu[2] : 0,
                    buffer_length >= 16 ? gpu[3] : 0);
            if (vfetch_index == 95 && buffer_length >= 96) {
              for (uint32_t vi = 0; vi < 4; ++vi) {
                const float* vf = reinterpret_cast<const float*>(phys + vi * 6);
                fprintf(stderr,
                        "[metal]   fc95 v%u pos=(%f,%f,%f,%f) raw=%08X %08X %08X %08X\n",
                        vi, vf[0], vf[1], vf[2], vf[3],
                        phys[vi * 6 + 0], phys[vi * 6 + 1],
                        phys[vi * 6 + 2], phys[vi * 6 + 3]);
              }
            }
            fflush(stderr);
          }
        }
        if (!shared_memory_->RequestRange(buffer_offset, buffer_length)) {
          REXLOG_ERROR("Failed to request vertex buffer at 0x{:08X}",
                       buffer_offset);
          return false;
        }
      }
    }
    for (const auto& memexport_range : memexport_ranges_) {
      uint32_t base_bytes = memexport_range.base_address_dwords << 2;
      if (!shared_memory_->RequestRange(base_bytes, memexport_range.size_bytes)) {
        REXLOG_ERROR("Failed to request memexport stream at 0x{:08X}", base_bytes);
        return false;
      }
    }
  }

  bool debug_solid_fragment = false;
  if constexpr (kMetalDebugForceSolidPipeline) {
    if (render_target_cache_) {
      MTL::RenderPipelineState* debug_pipeline =
          GetOrCreateDebugSolidPipeline(
              device_,
              render_target_cache_->GetColorFormat(0),
              render_target_cache_->GetDepthFormat(),
              render_target_cache_->GetStencilFormat());
      if (debug_pipeline) {
        pipeline = debug_pipeline;
        debug_solid_fragment = true;
      }
    }
  } else if constexpr (kMetalDebugForceSolidFragment) {
    if (render_target_cache_) {
      MTL::RenderPipelineState* debug_pipeline =
          GetOrCreateDebugSolidFragmentPipeline(
              device_, vertex_translation->metal_function(),
              render_target_cache_->GetColorFormat(0),
              render_target_cache_->GetDepthFormat(),
              render_target_cache_->GetStencilFormat());
      if (debug_pipeline) {
        pipeline = debug_pipeline;
        debug_solid_fragment = true;
      }
    }
  }

  current_render_encoder_->setRenderPipelineState(pipeline);
  ApplyRasterizerState(primitive_polygonal);
  ApplyDepthStencilState(primitive_polygonal, normalized_depth_control);

  int md = metal_draw_count.fetch_add(1);
  if constexpr (kMetalVerboseDiagnostics) {
  if (md < 20) {
    fprintf(stderr,
            "[metal] METAL DRAW #%d: verts=%u prim=%d host_vs_type=%d "
            "debug_solid=%d pipeline=%p enc=%p\n",
            md, primitive_processing_result.host_draw_vertex_count,
            (int)primitive_type,
            (int)primitive_processing_result.host_vertex_shader_type,
            int(debug_solid_fragment), pipeline, current_render_encoder_);
    fflush(stderr);
  }
  }

  bool shared_memory_is_uav = memexport_used;
  BindResources(regs, shared_memory_is_uav, primitive_polygonal,
                primitive_processing_result, viewport_info, used_texture_mask,
                normalized_depth_control, normalized_color_mask,
                metal_vertex_shader, metal_pixel_shader);

  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> desc_diag{0};
    int dd = desc_diag.fetch_add(1);
    if (dd < 3) {
      auto* top_ptrs = reinterpret_cast<const uint64_t*>(vs_top_level_ab_->contents());
      fprintf(stderr, "[metal] DESC DIAG #%d: VS AB[0]=%p AB[5]=%p AB[9]=%p AB[10]=%p\n",
              dd, (void*)top_ptrs[0], (void*)top_ptrs[5], (void*)top_ptrs[9], (void*)top_ptrs[10]);
      auto* ps_top_ptrs = reinterpret_cast<const uint64_t*>(ps_top_level_ab_->contents());
      fprintf(stderr, "[metal] DESC DIAG #%d: PS AB[10]=%p\n",
              dd, (void*)ps_top_ptrs[10]);
      auto* srv_entries = reinterpret_cast<const IRDescriptorTableEntry*>(res_heap_ab_->contents());
      fprintf(stderr, "[metal] DESC DIAG: SRV[0] gpuVA=%p size=%llu\n",
              (void*)srv_entries[0].gpuVA, (unsigned long long)srv_entries[0].metadata);
      for (int si = 1; si <= 4; si++) {
        fprintf(stderr, "[metal] DESC DIAG: SRV[%d] gpuVA=%p texViewID=%llu meta=%llu\n",
                si, (void*)srv_entries[si].gpuVA,
                (unsigned long long)srv_entries[si].textureViewID,
                (unsigned long long)srv_entries[si].metadata);
      }
      auto* smp_entries = reinterpret_cast<const IRDescriptorTableEntry*>(smp_heap_ab_->contents());
      for (int si = 0; si < 4; si++) {
        fprintf(stderr, "[metal] DESC DIAG: SMP[%d] gpuVA=%p texViewID=%llu\n",
                si, (void*)smp_entries[si].gpuVA,
                (unsigned long long)smp_entries[si].textureViewID);
      }
      auto* cbv_entries = reinterpret_cast<const IRDescriptorTableEntry*>(vs_cbv_heap_ab_->contents());
      for (int i = 0; i < 5; i++) {
        fprintf(stderr, "[metal] DESC DIAG: VS CBV[%d] gpuVA=%p size=%llu\n",
                i, (void*)cbv_entries[i].gpuVA, (unsigned long long)cbv_entries[i].metadata);
      }
      auto* ps_cbv_entries = reinterpret_cast<const IRDescriptorTableEntry*>(ps_cbv_heap_ab_->contents());
      for (int i = 0; i < 5; i++) {
        fprintf(stderr, "[metal] DESC DIAG: PS CBV[%d] gpuVA=%p size=%llu\n",
                i, (void*)ps_cbv_entries[i].gpuVA, (unsigned long long)ps_cbv_entries[i].metadata);
      }
      if (shared_memory_ && shared_memory_->GetBuffer()) {
        fprintf(stderr, "[metal] DESC DIAG: shmem gpuVA=%p actual=%p\n",
                (void*)shared_memory_->GetBuffer()->gpuAddress(),
                (void*)srv_entries[0].gpuVA);
      }
      fprintf(stderr, "[metal] DESC DIAG: uniforms gpuVA=%p\n",
              (void*)uniforms_ring_buffer_->gpuAddress());
      fflush(stderr);
    }
  }

  uint32_t draw_vertex_count = primitive_processing_result.host_draw_vertex_count;
  MTL::PrimitiveType metal_primitive_type;
  if (!GetMetalPrimitiveType(primitive_processing_result.host_primitive_type,
                             metal_primitive_type)) {
    REXLOG_ERROR("IssueDraw: unsupported Metal host primitive type {}",
                 uint32_t(primitive_processing_result.host_primitive_type));
    return false;
  }

  if constexpr (kMetalVerboseDiagnostics) {
    static std::atomic<int> draw_diag{0};
    int dd = draw_diag.fetch_add(1);
    if (dd < 10) {
      fprintf(stderr, "[metal] DRAW #%d: prim=%d->%d vtx=%u ib_type=%d ib_fmt=%d\n",
              dd, (int)primitive_processing_result.host_primitive_type,
              (int)metal_primitive_type, draw_vertex_count,
              (int)primitive_processing_result.index_buffer_type,
              (int)primitive_processing_result.host_index_format);
      fflush(stderr);
    }
  }

  using PIBT = PrimitiveProcessor::ProcessedIndexBufferType;
  bool draw_issued = false;
  auto do_draw_indexed = [&](MTL::IndexType index_type, MTL::Buffer* index_buffer,
                              uint64_t index_offset) {
    IRRuntimeDrawIndexedArgument da = {
        draw_vertex_count, 1, static_cast<uint32_t>(index_offset), 0, 0};
    IRRuntimeDrawParams dp = { .drawIndexed = da };
    uint16_t ir_index_type = static_cast<uint16_t>(index_type + 1);
    struct {
      uint16_t index_type;
      uint16_t pad0;
      uint32_t pad1;
    } frag_uniforms = { ir_index_type, 0, 0 };
    current_render_encoder_->setVertexBytes(&dp, sizeof(dp), MscBufferIndex::kDrawArguments);
    current_render_encoder_->setVertexBytes(&ir_index_type, sizeof(ir_index_type), MscBufferIndex::kUniforms);
    current_render_encoder_->setFragmentBytes(&frag_uniforms, sizeof(frag_uniforms), MscBufferIndex::kUniforms);
    current_render_encoder_->drawIndexedPrimitives(
        metal_primitive_type, NS::UInteger(draw_vertex_count),
        index_type, index_buffer, index_offset, NS::UInteger(1), 0, 0);
    draw_issued = true;
  };

  if (primitive_processing_result.index_buffer_type == PIBT::kGuestDMA) {
    MTL::Buffer* index_buffer = shared_memory_->GetBuffer();
    if (index_buffer) {
      MTL::IndexType index_type =
          primitive_processing_result.host_index_format == xenos::IndexFormat::kInt16
              ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
      uint64_t index_offset = primitive_processing_result.guest_index_base;
      do_draw_indexed(index_type, index_buffer, index_offset);
    }
  } else if (primitive_processing_result.index_buffer_type == PIBT::kHostConverted) {
    uint64_t converted_offset = 0;
    MTL::Buffer* converted_buffer = primitive_processor_->GetConvertedIndexBuffer(
        primitive_processing_result.host_index_buffer_handle, converted_offset);
    if (converted_buffer) {
      MTL::IndexType index_type =
          primitive_processing_result.host_index_format == xenos::IndexFormat::kInt16
              ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
      UseRenderEncoderResource(converted_buffer, MTL::ResourceUsageRead);
      do_draw_indexed(index_type, converted_buffer, converted_offset);
    }
  } else if (primitive_processing_result.index_buffer_type == PIBT::kHostBuiltinForAuto ||
             primitive_processing_result.index_buffer_type == PIBT::kHostBuiltinForDMA) {
    uint64_t builtin_offset = 0;
    MTL::Buffer* builtin_buffer = primitive_processor_->GetBuiltinIndexBuffer(
        primitive_processing_result.host_index_buffer_handle, builtin_offset);
    if (builtin_buffer) {
      MTL::IndexType index_type =
          primitive_processing_result.host_index_format == xenos::IndexFormat::kInt16
              ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
      UseRenderEncoderResource(builtin_buffer, MTL::ResourceUsageRead);
      do_draw_indexed(index_type, builtin_buffer, builtin_offset);
    }
  } else {
    IRRuntimeDrawArgument da = { draw_vertex_count, 1, 0, 0 };
    IRRuntimeDrawParams dp = { .draw = da };
    uint16_t ir_index_type = 0;
    struct {
      uint16_t index_type;
      uint16_t pad0;
      uint32_t pad1;
    } frag_uniforms = { ir_index_type, 0, 0 };
    current_render_encoder_->setVertexBytes(&dp, sizeof(dp), MscBufferIndex::kDrawArguments);
    current_render_encoder_->setVertexBytes(&ir_index_type, sizeof(ir_index_type), MscBufferIndex::kUniforms);
    current_render_encoder_->setFragmentBytes(&frag_uniforms, sizeof(frag_uniforms), MscBufferIndex::kUniforms);
    current_render_encoder_->drawPrimitives(
        metal_primitive_type, NS::UInteger(0), NS::UInteger(draw_vertex_count));
    draw_issued = true;
  }

  if (draw_issued && memexport_used && shared_memory_) {
    current_render_encoder_->memoryBarrier(
        MTL::BarrierScopeBuffers,
        MTL::RenderStageVertex | MTL::RenderStageFragment,
        MTL::RenderStageVertex | MTL::RenderStageFragment);
    if (!memexport_ranges_.empty()) {
      for (const auto& memexport_range : memexport_ranges_) {
        shared_memory_->RangeWrittenByGpu(
            memexport_range.base_address_dwords << 2,
            memexport_range.size_bytes);
      }
    } else {
      shared_memory_->RangeWrittenByGpu(0, SharedMemory::kBufferSize);
    }
  }

  ++current_draw_index_;




  return true;
}

bool MetalCommandProcessor::IssueCopy() {
  const RegisterFile& regs = *register_file_;
  if (!render_target_cache_) return false;
  static std::atomic<int> copy_diag_count{0};

  copy_resolve_writes_pending_ = true;

  BeginCommandBuffer();
  if (!current_command_buffer_) return false;

  draw_util::ResolveInfo resolve_info;
  if (!draw_util::GetResolveInfo(regs, *memory_, trace_writer_, 1, 1,
                                 false, false, resolve_info)) {
    return false;
  }
  {
    int cdc = copy_diag_count.load();
    if (cdc < 32 &&
        (resolve_info.IsClearingDepth() || resolve_info.IsClearingColor())) {
      fprintf(stderr,
              "[metal] COPY CLEAR #%d: copy_depth=%d clear_depth=%d "
              "clear_color=%d src=%u depth=0x%08X color=0x%08X%08X "
              "rect=%ux%u off=%u,%u dest=0x%08X\n",
              cdc, int(resolve_info.IsCopyingDepth()),
              int(resolve_info.IsClearingDepth()),
              int(resolve_info.IsClearingColor()),
              uint32_t(resolve_info.rb_copy_control.copy_src_select),
              resolve_info.rb_depth_clear, resolve_info.rb_color_clear_lo,
              resolve_info.rb_color_clear,
              uint32_t(resolve_info.coordinate_info.width_div_8) << 3,
              uint32_t(resolve_info.height_div_8) << 3,
              uint32_t(resolve_info.coordinate_info.edram_offset_x_div_8) << 3,
              uint32_t(resolve_info.coordinate_info.edram_offset_y_div_8) << 3,
              resolve_info.copy_dest_base);
      fflush(stderr);
    }
  }

  if (!resolve_info.IsCopyingDepth()) {
    uint32_t color_index = uint32_t(resolve_info.rb_copy_control.copy_src_select);
    MTL::Texture* source = render_target_cache_->GetColorTarget(color_index);
    int cdc = copy_diag_count.fetch_add(1);
    if (cdc < 8) {
      fprintf(stderr,
              "[metal] COPY #%d: color=%u dest_raw=0x%08X dest_adj=0x%08X "
              "extent=0x%08X+0x%X size8=%ux%u draws=%u src=%p %lux%lu fmt=%lu\n",
              cdc, color_index, regs[XE_GPU_REG_RB_COPY_DEST_BASE],
              resolve_info.copy_dest_base, resolve_info.copy_dest_extent_start,
              resolve_info.copy_dest_extent_length,
              resolve_info.coordinate_info.width_div_8, resolve_info.height_div_8,
              current_draw_index_, source, source ? source->width() : 0, source ? source->height() : 0,
              source ? source->pixelFormat() : 0);
      fflush(stderr);
    }
    if (source) {
      EndRenderEncoder();

      if constexpr (kMetalDebugFillBeforeCopy) {
        MTL::CommandBuffer* cmd = EnsureCommandBuffer();
        MTL::RenderPipelineState* debug_pipeline =
            GetOrCreateDebugFillPipeline(
                device_, source->pixelFormat(),
                render_target_cache_->GetDepthFormat(),
                render_target_cache_->GetStencilFormat());
        if (cmd && debug_pipeline) {
          MTL::RenderPassDescriptor* desc =
              MTL::RenderPassDescriptor::alloc()->init();
          auto* color = desc->colorAttachments()->object(0);
          color->setTexture(source);
          color->setLoadAction(MTL::LoadActionLoad);
          color->setStoreAction(MTL::StoreActionStore);
          MTL::RenderCommandEncoder* encoder =
              cmd->renderCommandEncoder(desc);
          if (encoder) {
            encoder->setRenderPipelineState(debug_pipeline);
            encoder->drawPrimitives(MTL::PrimitiveTypeTriangle,
                                    NS::UInteger(0), NS::UInteger(3));
            encoder->endEncoding();
            encoder->release();
          }
          desc->release();
        }
      }

      uint32_t width = std::max(
          uint32_t(1), uint32_t(resolve_info.coordinate_info.width_div_8) << 3);
      uint32_t height =
          std::max(uint32_t(1), uint32_t(resolve_info.height_div_8) << 3);
      width = std::min(width, uint32_t(source->width()));
      height = std::min(height, uint32_t(source->height()));

      bool recreate =
          !resolved_frontbuffer_texture_ ||
          resolved_frontbuffer_texture_->width() != width ||
          resolved_frontbuffer_texture_->height() != height ||
          resolved_frontbuffer_texture_->pixelFormat() != source->pixelFormat();
      if (recreate) {
        if (resolved_frontbuffer_texture_) {
          resolved_frontbuffer_texture_->release();
          resolved_frontbuffer_texture_ = nullptr;
        }
        MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
            source->pixelFormat(), width, height,
            MTL::TextureUsageShaderRead);
        desc->setStorageMode(MTL::StorageModePrivate);
        resolved_frontbuffer_texture_ = device_->newTexture(desc);
        desc->release();
      }

      if (resolved_frontbuffer_texture_) {
        MTL::BlitCommandEncoder* blit = current_command_buffer_->blitCommandEncoder();
        if (blit) {
          blit->copyFromTexture(source, 0, 0, MTL::Origin(0, 0, 0),
                                MTL::Size(width, height, 1),
                                resolved_frontbuffer_texture_, 0, 0,
                                MTL::Origin(0, 0, 0));
          blit->endEncoding();
          blit->release();
          resolved_frontbuffer_ptr_ = resolve_info.copy_dest_base;
          resolved_frontbuffer_width_ = width;
          resolved_frontbuffer_height_ = height;
          if (cdc < 8) {
            fprintf(stderr,
                    "[metal] COPY #%d: snap=%p copied=%ux%u tex=%lux%lu ptr=0x%08X\n",
                    cdc, resolved_frontbuffer_texture_, width, height,
                    resolved_frontbuffer_texture_->width(),
                    resolved_frontbuffer_texture_->height(),
                    resolved_frontbuffer_ptr_);
            fflush(stderr);
          }
        }
      }
    }
  }

  if (resolve_info.IsClearingDepth() || resolve_info.IsClearingColor()) {
    if (!render_target_cache_->ResolveClear(resolve_info)) {
      return false;
    }
  }

  return true;
}

void MetalCommandProcessor::IssueSwap(uint32_t frontbuffer_ptr,
                                      uint32_t frontbuffer_width,
                                      uint32_t frontbuffer_height) {
  static std::atomic<int> swap_count{0};
  int sc = swap_count.fetch_add(1);
  if constexpr (kMetalVerboseDiagnostics) {
  if (sc < 5 || sc % 1000 == 0) {
    fprintf(stderr, "[metal] IssueSwap #%d: ptr=0x%08X w=%u h=%u draws=%u\n",
            sc, frontbuffer_ptr, frontbuffer_width, frontbuffer_height,
            current_draw_index_);
    fflush(stderr);
  }
  }

  last_swap_ptr_ = frontbuffer_ptr;
  last_swap_width_ = frontbuffer_width;
  last_swap_height_ = frontbuffer_height;
  saw_swap_ = true;
  copy_resolve_writes_pending_ = false;

  MTL::Texture* swap_texture = nullptr;
  uint32_t swap_width_scaled = 0;
  uint32_t swap_height_scaled = 0;
  uint32_t swap_width_unscaled = 0;
  uint32_t swap_height_unscaled = 0;
  xenos::TextureFormat swap_format = xenos::TextureFormat::k_8_8_8_8;
  if (resolved_frontbuffer_texture_ &&
      (!frontbuffer_ptr || frontbuffer_ptr == resolved_frontbuffer_ptr_)) {
    swap_texture = resolved_frontbuffer_texture_;
    swap_width_scaled = resolved_frontbuffer_width_;
    swap_height_scaled = resolved_frontbuffer_height_;
    swap_width_unscaled = resolved_frontbuffer_width_;
    swap_height_unscaled = resolved_frontbuffer_height_;
  } else if (render_target_cache_) {
    swap_texture = render_target_cache_->GetColorTarget(0);
    swap_width_scaled = swap_texture ? uint32_t(swap_texture->width()) : 0;
    swap_height_scaled = swap_texture ? uint32_t(swap_texture->height()) : 0;
    swap_width_unscaled = frontbuffer_width;
    swap_height_unscaled = frontbuffer_height;
  }
  if (sc < 8) {
    fprintf(stderr,
            "[metal] SWAP #%d: fb=0x%08X resolved_ptr=0x%08X resolved=%p "
            "selected=%p selected_size=%ux%u unscaled=%ux%u\n",
            sc, frontbuffer_ptr, resolved_frontbuffer_ptr_,
            resolved_frontbuffer_texture_, swap_texture, swap_width_scaled,
            swap_height_scaled, swap_width_unscaled, swap_height_unscaled);
    fflush(stderr);
  }
  if (swap_texture) {
    if constexpr (kMetalVerboseDiagnostics) {
    if (sc < 3) {
      fprintf(stderr,
              "[metal] SWAP texture: fmt=%d %ux%u ptr=0x%08X tex=%p\n",
              (int)swap_texture->pixelFormat(), (unsigned)swap_width_scaled,
              (unsigned)swap_height_scaled, frontbuffer_ptr, swap_texture);
      fflush(stderr);
    }
    }
  }

  if (swap_texture) {
    if (!present_texture_ || present_texture_->width() != swap_texture->width() ||
        present_texture_->pixelFormat() != swap_texture->pixelFormat()) {
      if (present_texture_) present_texture_->release();
      MTL::TextureDescriptor* td = MTL::TextureDescriptor::texture2DDescriptor(
          swap_texture->pixelFormat(), swap_texture->width(), 720,
          MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget);
      td->setStorageMode(MTL::StorageModePrivate);
      present_texture_ = device_->newTexture(td);
      td->release();
      static bool diag_printed = false;
      if constexpr (kMetalVerboseDiagnostics) {
      if (!diag_printed) {
        fprintf(stderr, "[metal] Created present texture: %ux%u fmt=%d\n",
                (unsigned)present_texture_->width(), 720, (int)present_texture_->pixelFormat());
        fflush(stderr);
        diag_printed = true;
      }
      }
    }
  }

  if (current_render_encoder_) {
    current_render_encoder_->endEncoding();
    current_render_encoder_ = nullptr;
  }

  EndCommandBuffer();

  {
    if (swap_texture) {
      auto& provider = GetMetalProvider();
      provider.SetFrontbufferTexture(swap_texture);
      static std::atomic<int> fb_set_count{0};
      int fbs = fb_set_count.fetch_add(1);
      if constexpr (kMetalVerboseDiagnostics) {
      if (fbs < 5) {
        fprintf(stderr, "[metal] SET FRONTBUFFER DIRECT #%d: tex=%p fmt=%d %ux%u\n",
                fbs, swap_texture, (int)swap_texture->pixelFormat(),
                (unsigned)swap_texture->width(), (unsigned)swap_texture->height());
        fflush(stderr);
      }
      }
    }
  }

  if (!graphics_system_) {
    return;
  }
  ui::Presenter* presenter = graphics_system_->presenter();
  if (!presenter) {
    return;
  }

  auto get_active_swap_dimension = [](uint32_t packet_unscaled,
                                      uint32_t source_unscaled,
                                      uint32_t source_scaled) -> uint32_t {
    if (!source_scaled) {
      return 0;
    }
    uint32_t active_unscaled = packet_unscaled ? packet_unscaled : source_unscaled;
    if (!active_unscaled) {
      return source_scaled;
    }
    if (source_unscaled) {
      active_unscaled = std::min(active_unscaled, source_unscaled);
      uint64_t active_scaled =
          (uint64_t(active_unscaled) * source_scaled + (source_unscaled >> 1)) /
          source_unscaled;
      return uint32_t(std::clamp<uint64_t>(active_scaled, 1, source_scaled));
    }
    return std::min(active_unscaled, source_scaled);
  };

  uint32_t guest_width = get_active_swap_dimension(
      frontbuffer_width, swap_width_unscaled, swap_width_scaled);
  uint32_t guest_height = get_active_swap_dimension(
      frontbuffer_height, swap_height_unscaled, swap_height_scaled);
  if (!guest_width) {
    guest_width = swap_width_scaled ? swap_width_scaled
                                    : (frontbuffer_width ? frontbuffer_width : 1280);
  }
  if (!guest_height) {
    guest_height = swap_height_scaled ? swap_height_scaled
                                      : (frontbuffer_height ? frontbuffer_height : 720);
  }

  system::X_VIDEO_MODE video_mode;
  kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
  uint32_t display_width = std::max(uint32_t(1), uint32_t(video_mode.display_width));
  uint32_t display_height = std::max(uint32_t(1), uint32_t(video_mode.display_height));

  presenter->RefreshGuestOutput(
      guest_width, guest_height, display_width, display_height,
      [this](ui::Presenter::GuestOutputRefreshContext& context) -> bool {
        return true;
      });
}

void MetalCommandProcessor::WriteRegister(uint32_t index, uint32_t value) {
  CommandProcessor::WriteRegister(index, value);
}

void MetalCommandProcessor::TracePlaybackWroteMemory(uint32_t base_ptr,
                                                     uint32_t length) {
  if (shared_memory_) {
    shared_memory_->RequestRange(base_ptr, length);
  }
}

void MetalCommandProcessor::RestoreEdramSnapshot(const void* snapshot) {}

void MetalCommandProcessor::ClearCaches() {
  if (shared_memory_) shared_memory_->ClearCache();
  if (texture_cache_) texture_cache_->ClearCache();
}

void MetalCommandProcessor::InitializeShaderStorage(
    const std::filesystem::path& cache_root, uint32_t title_id, bool blocking) {
  if (g_metal_shader_cache) {
    std::filesystem::path cache_dir = cache_root / "metal_shaders";
    g_metal_shader_cache->Initialize(cache_dir);
  }
}

MTL::RenderPipelineState*
MetalCommandProcessor::CreatePipelineState(
    const MslPipelineCompileRequest& request, std::string* error_out) {
  MTL::RenderPipelineDescriptor* desc =
      MTL::RenderPipelineDescriptor::alloc()->init();

  if (request.vertex_function) {
    desc->setVertexFunction(request.vertex_function);
  }
  if (request.fragment_function) {
    desc->setFragmentFunction(request.fragment_function);
  }

  for (uint32_t i = 0; i < 4; ++i) {
    if (request.color_formats[i] != MTL::PixelFormatInvalid) {
      auto* ca = desc->colorAttachments()->object(i);
      ca->setPixelFormat(request.color_formats[i]);
      uint32_t bc = request.blendcontrol[i];
      if constexpr (kMetalVerboseDiagnostics) {
      static std::atomic<int> bc_diag{0};
      int bd = bc_diag.fetch_add(1);
      if (bd < 5) {
        fprintf(stderr, "[metal] BLEND DIAG: attachment=%u bc=0x%08X blend=%s\n",
                i, bc, bc ? "ENABLED" : "disabled");
        fflush(stderr);
      }
      }
      if (bc) {
        ca->setBlendingEnabled(true);
      }
      ca->setWriteMask(MTL::ColorWriteMaskAll);
    }
  }
  if (request.depth_format != MTL::PixelFormatInvalid) {
    desc->setDepthAttachmentPixelFormat(request.depth_format);
  }
  if (request.stencil_format != MTL::PixelFormatInvalid) {
    desc->setStencilAttachmentPixelFormat(request.stencil_format);
  }

  NS::Error* error = nullptr;
  MTL::RenderPipelineReflection* reflection = nullptr;
  MTL::RenderPipelineState* pipeline =
      device_->newRenderPipelineState(desc, MTL::PipelineOptionBufferTypeInfo,
                                      &reflection, &error);

  static std::atomic<int> pipeline_count{0};
  int pc = pipeline_count.fetch_add(1);
  if constexpr (kMetalVerboseDiagnostics) {
  if (pc < 10) {
    fprintf(stderr, "[metal] DIAG CreatePipeline #%d: vs=%p fs=%p depth=%d c0=%d ok=%d\n",
            pc, request.vertex_function, request.fragment_function,
            request.depth_format, request.color_formats[0],
            pipeline ? 1 : 0);
    if (pipeline && reflection) {
      auto dump_bindings = [&](const char* stage, NS::Array* args) {
        for (NS::UInteger i = 0; i < args->count(); i++) {
          auto* arg = static_cast<MTL::Argument*>(args->object(i));
          if (arg->type() == MTL::ArgumentTypeBuffer) {
            fprintf(stderr, "[metal]   %s buf[%u]: name='%s' bind=%u size=%zu\n",
                    stage, (unsigned)i, arg->name()->utf8String(),
                    (unsigned)arg->index(), arg->bufferDataSize());
          } else if (arg->type() == MTL::ArgumentTypeTexture) {
            fprintf(stderr, "[metal]   %s tex[%u]: name='%s' bind=%u\n",
                    stage, (unsigned)i, arg->name()->utf8String(),
                    (unsigned)arg->index());
          } else if (arg->type() == MTL::ArgumentTypeSampler) {
            fprintf(stderr, "[metal]   %s smp[%u]: name='%s' bind=%u\n",
                    stage, (unsigned)i, arg->name()->utf8String(),
                    (unsigned)arg->index());
          }
        }
      };
      dump_bindings("VS", reflection->vertexArguments());
      dump_bindings("FS", reflection->fragmentArguments());
    }
    fflush(stderr);
  }
  }
  if (reflection) reflection->release();

  desc->release();

  if (!pipeline) {
    if (error_out && error) {
      *error_out = error->localizedDescription()->utf8String();
    }
    if (error) error->release();
    return nullptr;
  }
  return pipeline;
}

MTL::RenderPipelineState*
MetalCommandProcessor::GetOrCreatePipelineState(
    MetalShader::MetalTranslation* vertex_translation,
    MetalShader::MetalTranslation* pixel_translation,
    const ::rex::graphics::RegisterFile& regs,
    MslPipelineCompileStatus* compile_status_out) {
  if (!vertex_translation || !vertex_translation->is_valid()) {
    if (compile_status_out) *compile_status_out = MslPipelineCompileStatus::kFailed;
    return nullptr;
  }

  MslPipelineCompileRequest request = {};
  request.vertex_function = vertex_translation->metal_function();
  request.fragment_function = pixel_translation ? pixel_translation->metal_function() : nullptr;

  if (render_target_cache_) {
    auto* rt = render_target_cache_->GetOrCreateRenderTarget(regs);
    if (rt) {
      auto rt_key = rt->key();
      render_target_width_ = rt_key.GetWidth();
    }
  }

  if (render_target_cache_) {
    for (uint32_t i = 0; i < 4; ++i) {
      request.color_formats[i] = render_target_cache_->GetColorFormat(i);
    }
    request.depth_format = render_target_cache_->GetDepthFormat();
    request.stencil_format = render_target_cache_->GetStencilFormat();
  }

  struct PipelineKeyData {
    uint64_t vertex_shader_hash;
    uint64_t vertex_shader_modification;
    uint64_t pixel_shader_hash;
    uint64_t pixel_shader_modification;
    uint32_t color_formats[4];
    uint32_t depth_format;
    uint32_t stencil_format;
    uint32_t sample_count;
    uint32_t normalized_color_mask;
    uint32_t blendcontrol[4];
  };
  PipelineKeyData key_data = {};
  key_data.vertex_shader_hash =
      vertex_translation->shader().ucode_data_hash();
  key_data.vertex_shader_modification = vertex_translation->modification();
  if (pixel_translation) {
    key_data.pixel_shader_hash =
        pixel_translation->shader().ucode_data_hash();
    key_data.pixel_shader_modification = pixel_translation->modification();
  }
  for (uint32_t i = 0; i < 4; ++i) {
    key_data.color_formats[i] = uint32_t(request.color_formats[i]);
    key_data.blendcontrol[i] = request.blendcontrol[i];
  }
  key_data.depth_format = uint32_t(request.depth_format);
  key_data.stencil_format = uint32_t(request.stencil_format);
  key_data.sample_count = request.sample_count;
  key_data.normalized_color_mask = request.normalized_color_mask;

  uint64_t key = XXH3_64bits(&key_data, sizeof(key_data));
  request.pipeline_key = key;

  auto it = pipeline_state_cache_.find(key);
  if (it != pipeline_state_cache_.end()) {
    if (compile_status_out) *compile_status_out = MslPipelineCompileStatus::kReady;
    return it->second;
  }

  std::string error;
  MTL::RenderPipelineState* pipeline = CreatePipelineState(request, &error);
  if (!pipeline) {
    REXLOG_ERROR("MetalCommandProcessor: Pipeline creation failed: {}", error);
    if (compile_status_out) *compile_status_out = MslPipelineCompileStatus::kFailed;
    return nullptr;
  }

  pipeline_state_cache_[key] = pipeline;
  if (compile_status_out) *compile_status_out = MslPipelineCompileStatus::kReady;
  return pipeline;
}

void MetalCommandProcessor::ApplyRasterizerState(bool primitive_polygonal) {
  if (!current_render_encoder_) return;

  const RegisterFile& regs = *register_file_;
  auto pa_su_sc_mode_cntl = regs.Get<reg::PA_SU_SC_MODE_CNTL>();
  auto pa_cl_clip_cntl = regs.Get<reg::PA_CL_CLIP_CNTL>();

  MTL::CullMode cull_mode = MTL::CullModeNone;
  if (primitive_polygonal) {
    bool cull_front = pa_su_sc_mode_cntl.cull_front;
    bool cull_back = pa_su_sc_mode_cntl.cull_back;
    if (cull_front && !cull_back) {
      cull_mode = MTL::CullModeFront;
    } else if (cull_back && !cull_front) {
      cull_mode = MTL::CullModeBack;
    }
  }
  current_render_encoder_->setCullMode(cull_mode);

  current_render_encoder_->setFrontFacingWinding(
      pa_su_sc_mode_cntl.face ? MTL::WindingClockwise
                              : MTL::WindingCounterClockwise);

  MTL::TriangleFillMode fill_mode = MTL::TriangleFillModeFill;
  if (primitive_polygonal &&
      pa_su_sc_mode_cntl.poly_mode == xenos::PolygonModeEnable::kDualMode) {
    xenos::PolygonType polygon_type = xenos::PolygonType::kTriangles;
    if (!pa_su_sc_mode_cntl.cull_front) {
      polygon_type = std::min(polygon_type, pa_su_sc_mode_cntl.polymode_front_ptype);
    }
    if (!pa_su_sc_mode_cntl.cull_back) {
      polygon_type = std::min(polygon_type, pa_su_sc_mode_cntl.polymode_back_ptype);
    }
    if (polygon_type != xenos::PolygonType::kTriangles) {
      fill_mode = MTL::TriangleFillModeLines;
    }
  }
  current_render_encoder_->setTriangleFillMode(fill_mode);

  float polygon_offset_scale = 0.0f;
  float polygon_offset = 0.0f;
  draw_util::GetPreferredFacePolygonOffset(
      regs, primitive_polygonal, polygon_offset_scale, polygon_offset);
  float depth_bias_factor = regs.Get<reg::RB_DEPTH_INFO>().depth_format ==
                                    xenos::DepthRenderTargetFormat::kD24S8
                                ? draw_util::kD3D10PolygonOffsetFactorUnorm24
                                : draw_util::kD3D10PolygonOffsetFactorFloat24;
  float depth_bias_constant = polygon_offset * depth_bias_factor;
  float depth_bias_slope = polygon_offset_scale * xenos::kPolygonOffsetScaleSubpixelUnit;
  current_render_encoder_->setDepthBias(depth_bias_constant, depth_bias_slope, 0.0f);

  current_render_encoder_->setDepthClipMode(pa_cl_clip_cntl.clip_disable
                                                 ? MTL::DepthClipModeClamp
                                                 : MTL::DepthClipModeClip);
}

void MetalCommandProcessor::ApplyDepthStencilState(
    bool primitive_polygonal,
    reg::RB_DEPTHCONTROL normalized_depth_control) {
  DepthStencilStateKey key = {};
  key.depth_control = normalized_depth_control.value;
  key.polygonal_and_backface = primitive_polygonal ? 1 : 0;

  auto it = depth_stencil_state_cache_.find(key);
  if (it != depth_stencil_state_cache_.end()) {
    current_render_encoder_->setDepthStencilState(it->second);
    return;
  }

  MTL::DepthStencilDescriptor* desc = MTL::DepthStencilDescriptor::alloc()->init();
  if constexpr (kMetalDebugForceDepthAlways) {
    desc->setDepthCompareFunction(MTL::CompareFunctionAlways);
    desc->setDepthWriteEnabled(false);
  } else if (normalized_depth_control.z_enable) {
    desc->setDepthCompareFunction(static_cast<MTL::CompareFunction>(
        uint32_t(normalized_depth_control.zfunc)));
    desc->setDepthWriteEnabled(normalized_depth_control.z_write_enable);
  } else {
    desc->setDepthCompareFunction(MTL::CompareFunctionAlways);
    desc->setDepthWriteEnabled(false);
  }

  MTL::DepthStencilState* state = device_->newDepthStencilState(desc);
  desc->release();

  if (state) {
    depth_stencil_state_cache_[key] = state;
    current_render_encoder_->setDepthStencilState(state);
  }
}

void MetalCommandProcessor::BindResources(
    const ::rex::graphics::RegisterFile& regs, bool shared_memory_is_uav,
    bool primitive_polygonal,
    const PrimitiveProcessor::ProcessingResult& primitive_processing_result,
    const draw_util::ViewportInfo& viewport_info, uint32_t used_texture_mask,
    reg::RB_DEPTHCONTROL normalized_depth_control, uint32_t normalized_color_mask,
    MetalShader* vertex_shader, MetalShader* pixel_shader) {
  if (!current_render_encoder_) return;

  using namespace MscHeapLayout;
  using namespace MscBufferIndex;

  WriteSystemConstants(regs, shared_memory_is_uav, primitive_polygonal,
                       primitive_processing_result, viewport_info,
                       used_texture_mask, normalized_depth_control,
                       normalized_color_mask, vertex_shader, pixel_shader);

  auto* res_entries = reinterpret_cast<IRDescriptorTableEntry*>(res_heap_ab_->contents());
  auto* uav_entries = res_entries + kResourceHeapSlotsPerTable;

  MTL::Buffer* shared_mem_buffer = shared_memory_ ? shared_memory_->GetBuffer() : nullptr;
  if (shared_mem_buffer) {
    SetDescriptorBuffer(&res_entries[0],
                               shared_mem_buffer->gpuAddress(),
                               shared_mem_buffer->length());
    SetDescriptorBuffer(&uav_entries[0],
                               shared_mem_buffer->gpuAddress(),
                               shared_mem_buffer->length());
    MTL::ResourceUsage usage = shared_memory_is_uav
        ? (MTL::ResourceUsageRead | MTL::ResourceUsageWrite)
        : MTL::ResourceUsageRead;
    UseRenderEncoderResource(shared_mem_buffer, usage);
  }

  if (texture_cache_) {
    if constexpr (kMetalVerboseDiagnostics) {
      static std::atomic<int> tex_diag{0};
      int td = tex_diag.fetch_add(1);
      if (td < 3) {
        uint32_t texture_count = texture_cache_->GetBoundTextureCount();
        fprintf(stderr, "[metal] TEX DIAG #%d: bound_texture_count=%u\n", td, texture_count);
        for (uint32_t i = 0; i < texture_count && i < 4; ++i) {
          MTL::Texture* t = texture_cache_->GetBoundTexture(i);
          fprintf(stderr, "[metal] TEX DIAG: tex[%u]=%p %ux%u fmt=%d\n",
                  i, t, t ? (unsigned)t->width() : 0, t ? (unsigned)t->height() : 0,
                  t ? (int)t->pixelFormat() : -1);
        }
        fflush(stderr);
      }
    }
    auto bind_shader_textures = [&](MetalShader* shader) {
      if (!shader) return;
      const auto& tex_bindings = shader->GetTextureBindingsAfterTranslation();
      for (size_t binding_index = 0; binding_index < tex_bindings.size(); ++binding_index) {
        uint32_t slot = 1 + static_cast<uint32_t>(binding_index);
        if (slot >= kResourceHeapSlotsPerTable) break;
        const auto& binding = tex_bindings[binding_index];
        MTL::Texture* tex = texture_cache_->GetBoundTexture(
            binding.fetch_constant, binding.is_signed);
        if (!tex) continue;
        SetDescriptorTexture(&res_entries[slot], tex);
        UseRenderEncoderResource(tex, MTL::ResourceUsageSample);
        if constexpr (kMetalVerboseDiagnostics) {
          static std::atomic<int> rid_diag{0};
          int rd = rid_diag.fetch_add(1);
          if (rd < 8) {
            fprintf(stderr, "[metal] TEX RID: slot=%u bi=%zu fc=%u signed=%d gpuRID=%llu ptr=%p\n",
                    slot, binding_index, binding.fetch_constant, int(binding.is_signed),
                    (unsigned long long)tex->gpuResourceID()._impl, tex);
            fflush(stderr);
          }
        }
      }
    };
    bind_shader_textures(vertex_shader);
    bind_shader_textures(pixel_shader);

    auto* smp_entries = reinterpret_cast<IRDescriptorTableEntry*>(smp_heap_ab_->contents());
    auto bind_shader_samplers = [&](MetalShader* shader) {
      if (!shader) return;
      const auto& sampler_bindings = shader->GetSamplerBindingsAfterTranslation();
      for (size_t sampler_index = 0; sampler_index < sampler_bindings.size(); ++sampler_index) {
        if (sampler_index >= kSamplerHeapSlotsPerTable) break;
        const auto& binding = sampler_bindings[sampler_index];
        MTL::SamplerState* sampler = texture_cache_->GetBoundSamplerState(binding.fetch_constant);
        if (sampler) {
          SetDescriptorSampler(&smp_entries[sampler_index], sampler);
        }
      }
    };
    bind_shader_samplers(vertex_shader);
    bind_shader_samplers(pixel_shader);
  }

  {
    uint64_t uniforms_base = uniforms_ring_buffer_->gpuAddress();
    size_t base_offset = (uniforms_ring_offset_ > kUniformsBytesPerTable)
        ? uniforms_ring_offset_ - kUniformsBytesPerTable : 0;

    auto* vs_cbv = reinterpret_cast<IRDescriptorTableEntry*>(vs_cbv_heap_ab_->contents());
    SetDescriptorBuffer(&vs_cbv[0],
                                uniforms_base + base_offset,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[1],
                                uniforms_base + base_offset + 1 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[2],
                                uniforms_base + base_offset + 2 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[3],
                                uniforms_base + base_offset + 3 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[4],
                                uniforms_base + base_offset + 4 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[5],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);
    SetDescriptorBuffer(&vs_cbv[6],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);

    auto* ps_cbv = reinterpret_cast<IRDescriptorTableEntry*>(ps_cbv_heap_ab_->contents());
    SetDescriptorBuffer(&ps_cbv[0],
                                uniforms_base + base_offset,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[1],
                                uniforms_base + base_offset + 5 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[2],
                                uniforms_base + base_offset + 2 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[3],
                                uniforms_base + base_offset + 3 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[4],
                                uniforms_base + base_offset + 6 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[5],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);
    SetDescriptorBuffer(&ps_cbv[6],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);
  }

  if (uniforms_ring_buffer_) {
    UseRenderEncoderResource(uniforms_ring_buffer_,
                             MTL::ResourceUsageRead);
  }
  if (null_buffer_) {
    UseRenderEncoderResource(null_buffer_, MTL::ResourceUsageRead);
  }
  if (vs_cbv_heap_ab_) {
    UseRenderEncoderResource(vs_cbv_heap_ab_, MTL::ResourceUsageRead);
  }
  if (ps_cbv_heap_ab_) {
    UseRenderEncoderResource(ps_cbv_heap_ab_, MTL::ResourceUsageRead);
  }
  if (res_heap_ab_) {
    UseRenderEncoderResource(res_heap_ab_, MTL::ResourceUsageRead);
  }
  if (smp_heap_ab_) {
    UseRenderEncoderResource(smp_heap_ab_, MTL::ResourceUsageRead);
  }
  if (vs_top_level_ab_) {
    UseRenderEncoderResource(vs_top_level_ab_, MTL::ResourceUsageRead);
  }
  if (ps_top_level_ab_) {
    UseRenderEncoderResource(ps_top_level_ab_, MTL::ResourceUsageRead);
  }

  current_render_encoder_->setVertexBuffer(res_heap_ab_, 0, kDescriptorHeap);
  current_render_encoder_->setFragmentBuffer(res_heap_ab_, 0, kDescriptorHeap);

  current_render_encoder_->setVertexBuffer(smp_heap_ab_, 0, kSamplerHeap);
  current_render_encoder_->setFragmentBuffer(smp_heap_ab_, 0, kSamplerHeap);

  current_render_encoder_->setVertexBuffer(vs_top_level_ab_, 0, kArgumentBuffer);
  current_render_encoder_->setFragmentBuffer(ps_top_level_ab_, 0, kArgumentBuffer);
}

void MetalCommandProcessor::WriteSystemConstants(
    const ::rex::graphics::RegisterFile& regs, bool shared_memory_is_uav,
    bool primitive_polygonal,
    const PrimitiveProcessor::ProcessingResult& primitive_processing_result,
    const draw_util::ViewportInfo& viewport_info, uint32_t used_texture_mask,
    reg::RB_DEPTHCONTROL normalized_depth_control,
    uint32_t normalized_color_mask, MetalShader* vertex_shader,
    MetalShader* pixel_shader) {
  if (!current_render_encoder_ || !uniforms_ring_data_) return;

  using DxbcTranslator = DxbcShaderTranslator;
  using namespace MscHeapLayout;
  (void)normalized_color_mask;

  size_t total_size = kUniformsBytesPerTable;
  size_t base_offset = uniforms_ring_offset_;
  if (base_offset + total_size > kUniformsRingSize) {
    base_offset = 0;
  }

  uint8_t* base_ptr = uniforms_ring_data_ + base_offset;

  auto* sys_consts = reinterpret_cast<DxbcTranslator::SystemConstants*>(base_ptr);
  std::memset(sys_consts, 0, kCbvSizeBytes);

  MTL::Texture* color0 = render_target_cache_ ? render_target_cache_->GetColorTarget(0) : nullptr;
  uint32_t rt_w = color0 ? color0->width() : 1280;
  uint32_t rt_h = color0 ? color0->height() : 720;

  auto pa_cl_clip_cntl = regs.Get<reg::PA_CL_CLIP_CNTL>();
  auto pa_cl_vte_cntl = regs.Get<reg::PA_CL_VTE_CNTL>();
  auto rb_alpha_ref = regs.Get<float>(XE_GPU_REG_RB_ALPHA_REF);
  auto rb_colorcontrol = regs.Get<reg::RB_COLORCONTROL>();
  auto rb_depth_info = regs.Get<reg::RB_DEPTH_INFO>();
  auto rb_surface_info = regs.Get<reg::RB_SURFACE_INFO>();
  auto vgt_draw_initiator = regs.Get<reg::VGT_DRAW_INITIATOR>();
  uint32_t vgt_indx_offset = regs.Get<reg::VGT_INDX_OFFSET>().indx_offset;
  uint32_t vgt_max_vtx_indx = regs.Get<reg::VGT_MAX_VTX_INDX>().max_indx;
  uint32_t vgt_min_vtx_indx = regs.Get<reg::VGT_MIN_VTX_INDX>().min_indx;

  {
    uint32_t flags = 0;
    if (shared_memory_is_uav) {
      flags |= DxbcTranslator::kSysFlag_SharedMemoryIsUAV;
    }
    if (pa_cl_vte_cntl.vtx_xy_fmt) {
      flags |= DxbcTranslator::kSysFlag_XYDividedByW;
    }
    if (pa_cl_vte_cntl.vtx_z_fmt) {
      flags |= DxbcTranslator::kSysFlag_ZDividedByW;
    }
    if (pa_cl_vte_cntl.vtx_w0_fmt) {
      flags |= DxbcTranslator::kSysFlag_WNotReciprocal;
    }
    if (primitive_polygonal) {
      flags |= DxbcTranslator::kSysFlag_PrimitivePolygonal;
    }
    if (draw_util::IsPrimitiveLine(regs)) {
      flags |= DxbcTranslator::kSysFlag_PrimitiveLine;
    }
    if (rb_depth_info.depth_format == xenos::DepthRenderTargetFormat::kD24FS8) {
      flags |= DxbcTranslator::kSysFlag_DepthFloat24;
    }
    xenos::CompareFunction alpha_test_function =
        rb_colorcontrol.alpha_test_enable
            ? rb_colorcontrol.alpha_func
            : xenos::CompareFunction::kAlways;
    flags |= uint32_t(alpha_test_function)
             << DxbcTranslator::kSysFlag_AlphaPassIfLess_Shift;
    sys_consts->flags = flags;
  }

  sys_consts->tessellation_factor_range_min =
      regs.Get<float>(XE_GPU_REG_VGT_HOS_MIN_TESS_LEVEL) + 1.0f;
  sys_consts->tessellation_factor_range_max =
      regs.Get<float>(XE_GPU_REG_VGT_HOS_MAX_TESS_LEVEL) + 1.0f;
  sys_consts->line_loop_closing_index =
      primitive_processing_result.line_loop_closing_index;
  sys_consts->vertex_index_endian =
      primitive_processing_result.host_shader_index_endian;
  sys_consts->vertex_index_offset = vgt_indx_offset;
  sys_consts->vertex_index_min = vgt_min_vtx_indx;
  sys_consts->vertex_index_max = vgt_max_vtx_indx;

  if (!pa_cl_clip_cntl.clip_disable) {
    float* user_clip_plane_write_ptr = sys_consts->user_clip_planes[0];
    uint32_t user_clip_planes_remaining = pa_cl_clip_cntl.ucp_ena;
    uint32_t user_clip_plane_index;
    while (rex::bit_scan_forward(user_clip_planes_remaining,
                                 &user_clip_plane_index)) {
      user_clip_planes_remaining &= ~(UINT32_C(1) << user_clip_plane_index);
      const void* user_clip_plane_regs =
          &regs[XE_GPU_REG_PA_CL_UCP_0_X + user_clip_plane_index * 4];
      std::memcpy(user_clip_plane_write_ptr, user_clip_plane_regs,
                  4 * sizeof(float));
      user_clip_plane_write_ptr += 4;
    }
  }

  for (uint32_t i = 0; i < 3; ++i) {
    sys_consts->ndc_scale[i] = viewport_info.ndc_scale[i];
    sys_consts->ndc_offset[i] = viewport_info.ndc_offset[i];
  }

  {
    if (vgt_draw_initiator.prim_type == xenos::PrimitiveType::kPointList) {
      auto pa_su_point_minmax = regs.Get<reg::PA_SU_POINT_MINMAX>();
      auto pa_su_point_size = regs.Get<reg::PA_SU_POINT_SIZE>();
      sys_consts->point_vertex_diameter_min =
          float(pa_su_point_minmax.min_size) * (2.0f / 16.0f);
      sys_consts->point_vertex_diameter_max =
          float(pa_su_point_minmax.max_size) * (2.0f / 16.0f);
      sys_consts->point_constant_diameter[0] =
          float(pa_su_point_size.width) * (2.0f / 16.0f);
      sys_consts->point_constant_diameter[1] =
          float(pa_su_point_size.height) * (2.0f / 16.0f);
      sys_consts->point_screen_diameter_to_ndc_radius[0] =
          1.0f / std::max(viewport_info.xy_extent[0], uint32_t(1));
      sys_consts->point_screen_diameter_to_ndc_radius[1] =
          1.0f / std::max(viewport_info.xy_extent[1], uint32_t(1));
    }
  }

  if (texture_cache_ && used_texture_mask) {
    uint32_t textures_resolution_scaled = 0;
    uint32_t textures_remaining = used_texture_mask;
    uint32_t texture_index;
    while (rex::bit_scan_forward(textures_remaining, &texture_index)) {
      textures_remaining &= ~(uint32_t(1) << texture_index);
      uint32_t& texture_signs_uint =
          sys_consts->texture_swizzled_signs[texture_index >> 2];
      uint32_t texture_signs_shift = (texture_index & 3) * 8;
      uint8_t texture_signs =
          texture_cache_->GetActiveTextureSwizzledSigns(texture_index);
      uint32_t texture_signs_shifted = uint32_t(texture_signs) << texture_signs_shift;
      uint32_t texture_signs_mask = uint32_t(0b11111111) << texture_signs_shift;
      texture_signs_uint =
          (texture_signs_uint & ~texture_signs_mask) | texture_signs_shifted;
      textures_resolution_scaled |=
          uint32_t(texture_cache_->IsActiveTextureResolutionScaled(texture_index))
          << texture_index;
    }
    sys_consts->textures_resolution_scaled = textures_resolution_scaled;
  }

  sys_consts->sample_count_log2[0] =
      rb_surface_info.msaa_samples >= xenos::MsaaSamples::k4X ? 1 : 0;
  sys_consts->sample_count_log2[1] =
      rb_surface_info.msaa_samples >= xenos::MsaaSamples::k2X ? 1 : 0;
  sys_consts->alpha_test_reference = rb_alpha_ref;
  sys_consts->alpha_to_mask =
      rb_colorcontrol.alpha_to_mask_enable ? (rb_colorcontrol.value >> 24) | (1 << 8) : 0;

  {
    for (uint32_t i = 0; i < 4; ++i) {
      auto color_info = regs.Get<reg::RB_COLOR_INFO>(
          reg::RB_COLOR_INFO::rt_register_indices[i]);
      int32_t color_exp_bias = color_info.color_exp_bias;
      auto color_exp_bias_scale =
          rex::memory::Reinterpret<float>(
              int32_t(0x3F800000 + (color_exp_bias << 23)));
      sys_consts->color_exp_bias[i] = color_exp_bias_scale;
    }
  }

  uint8_t* float_ptr = base_ptr + 1 * kCbvSizeBytes;
  std::memset(float_ptr, 0, kCbvSizeBytes);
  {
    if (vertex_shader) {
      auto& crm = vertex_shader->constant_register_map();
      auto* dst = float_ptr;
      if (crm.float_dynamic_addressing) {
        size_t copy_size = std::min(size_t(256) * 4 * sizeof(float), kCbvSizeBytes);
        std::memcpy(dst, &regs.values[0x4000], copy_size);
      } else {
        for (uint32_t i = 0; i < 4; ++i) {
          uint64_t entry = crm.float_bitmap[i];
          uint32_t idx;
          while (rex::bit_scan_forward(entry, &idx)) {
            entry &= ~(1ull << idx);
            std::memcpy(dst,
                        &regs.values[0x4000 + (i << 8) + (idx << 2)],
                        4 * sizeof(float));
            dst += 4 * sizeof(float);
          }
        }
      }
    }
  }

  uint8_t* pixel_float_ptr = base_ptr + 5 * kCbvSizeBytes;
  std::memset(pixel_float_ptr, 0, kCbvSizeBytes);
  {
    if (pixel_shader) {
      auto& crm = pixel_shader->constant_register_map();
      auto* dst = pixel_float_ptr;
      if (crm.float_dynamic_addressing) {
        size_t copy_size = std::min(size_t(256) * 4 * sizeof(float), kCbvSizeBytes);
        std::memcpy(dst, &regs.values[0x4400], copy_size);
      } else {
        for (uint32_t i = 0; i < 4; ++i) {
          uint64_t entry = crm.float_bitmap[i];
          uint32_t idx;
          while (rex::bit_scan_forward(entry, &idx)) {
            entry &= ~(1ull << idx);
            std::memcpy(dst,
                        &regs.values[0x4400 + (i << 8) + (idx << 2)],
                        4 * sizeof(float));
            dst += 4 * sizeof(float);
          }
        }
      }
    }
  }

  uint8_t* bool_loop_ptr = base_ptr + 2 * kCbvSizeBytes;
  std::memset(bool_loop_ptr, 0, kCbvSizeBytes);
  {
    constexpr uint32_t kBoolLoopConstantsSize = (8 + 32) * sizeof(uint32_t);
    std::memcpy(bool_loop_ptr, &regs.values[0x4900], kBoolLoopConstantsSize);
  }

  {
    static std::atomic<int> diag_count{0};
    int dc = diag_count.fetch_add(1);
    if constexpr (kMetalVerboseDiagnostics) {
    if (dc < 5) {
      auto* bl_raw = reinterpret_cast<const uint32_t*>(bool_loop_ptr);
      fprintf(stderr, "[metal] BOOL/LOOP DIAG #%d: bool[0]=0x%08X bool[1]=0x%08X bool[2]=0x%08X bool[3]=0x%08X\n",
              dc, bl_raw[0], bl_raw[1], bl_raw[2], bl_raw[3]);
      fprintf(stderr, "[metal] BOOL/LOOP DIAG: loop[0]=0x%08X loop[1]=0x%08X loop[2]=0x%08X loop[3]=0x%08X\n",
              bl_raw[8], bl_raw[9], bl_raw[10], bl_raw[11]);
      fprintf(stderr, "[metal] SYS DIAG: flags=0x%08X alpha_ref=0x%08X exp_bias=(%f,%f,%f,%f)\n",
              sys_consts->flags, sys_consts->alpha_test_reference,
              sys_consts->color_exp_bias[0], sys_consts->color_exp_bias[1],
              sys_consts->color_exp_bias[2], sys_consts->color_exp_bias[3]);
      fprintf(stderr, "[metal] SYS DIAG: tex_swizzled_signs=[%08X,%08X,%08X,%08X,%08X,%08X,%08X,%08X]\n",
              sys_consts->texture_swizzled_signs[0], sys_consts->texture_swizzled_signs[1],
              sys_consts->texture_swizzled_signs[2], sys_consts->texture_swizzled_signs[3],
              sys_consts->texture_swizzled_signs[4], sys_consts->texture_swizzled_signs[5],
              sys_consts->texture_swizzled_signs[6], sys_consts->texture_swizzled_signs[7]);
      auto* pfc_raw = reinterpret_cast<const float*>(pixel_float_ptr);
      fprintf(stderr, "[metal] PSFC DIAG #%d: packed[0]=(%.6f,%.6f,%.6f,%.6f)\n",
              dc, pfc_raw[0], pfc_raw[1], pfc_raw[2], pfc_raw[3]);
      if (dc == 2) {
        for (int pi = 0; pi < 7 && pfc_raw[pi*4] != 0.0f; pi++) {
          fprintf(stderr, "[metal] PSFC ALL: packed[%d]=(%f,%f,%f,%f) hex=(%08X,%08X,%08X,%08X)\n",
                  pi, pfc_raw[pi*4+0], pfc_raw[pi*4+1], pfc_raw[pi*4+2], pfc_raw[pi*4+3],
                  ((uint32_t*)pfc_raw)[pi*4+0], ((uint32_t*)pfc_raw)[pi*4+1],
                  ((uint32_t*)pfc_raw)[pi*4+2], ((uint32_t*)pfc_raw)[pi*4+3]);
        }
      }
      fprintf(stderr, "[metal] FETCH DIAG: fc3_dword0=0x%08X fc3_dword1=0x%08X fc3_dword2=0x%08X fc3_dword5=0x%08X\n",
              regs.values[0x4800 + 6*3 + 0], regs.values[0x4800 + 6*3 + 1],
              regs.values[0x4800 + 6*3 + 2], regs.values[0x4800 + 6*3 + 5]);
      fflush(stderr);
    }
    }
  }

  uint8_t* fetch_ptr = base_ptr + 3 * kCbvSizeBytes;
  std::memset(fetch_ptr, 0, kCbvSizeBytes);
  {
    auto* fetch_dwords = reinterpret_cast<uint32_t*>(fetch_ptr);
    for (uint32_t i = 0; i < 96; i++) {
      auto vf = regs.GetVertexFetch(i);
      fetch_dwords[i * 2 + 0] = vf.dword_0;
      fetch_dwords[i * 2 + 1] = vf.dword_1;
    }
  }

  uint8_t* vertex_desc_idx_ptr = base_ptr + 4 * kCbvSizeBytes;
  uint8_t* pixel_desc_idx_ptr = base_ptr + 6 * kCbvSizeBytes;
  std::memset(vertex_desc_idx_ptr, 0, kCbvSizeBytes);
  std::memset(pixel_desc_idx_ptr, 0, kCbvSizeBytes);
  auto write_descriptor_indices = [&](MetalShader* shader, uint8_t* desc_idx_ptr) {
    if (!shader) {
      return;
    }
    auto* desc_indices = reinterpret_cast<uint32_t*>(desc_idx_ptr);
    auto& tex_bindings = shader->GetTextureBindingsAfterTranslation();
    for (size_t i = 0; i < tex_bindings.size(); ++i) {
      uint32_t srv_slot = tex_bindings[i].bindless_descriptor_index;
      desc_indices[tex_bindings[i].bindless_descriptor_index] = srv_slot;
    }
    auto& smp_bindings = shader->GetSamplerBindingsAfterTranslation();
    for (size_t i = 0; i < smp_bindings.size(); ++i) {
      desc_indices[smp_bindings[i].bindless_descriptor_index] =
          smp_bindings[i].fetch_constant;
    }
    {
      static std::atomic<int> di_diag{0};
      int did = di_diag.fetch_add(1);
      if constexpr (kMetalVerboseDiagnostics) {
      if (did < 5) {
        fprintf(stderr, "[metal] DESC IDX DIAG #%d: %zu tex_bindings, %zu smp_bindings\n",
                did, tex_bindings.size(), smp_bindings.size());
        for (size_t i = 0; i < tex_bindings.size() && i < 8; ++i) {
          fprintf(stderr, "[metal]   tex[%zu] di=%u fc=%u → srv=%u\n",
                  i, tex_bindings[i].bindless_descriptor_index,
                  tex_bindings[i].fetch_constant,
                  desc_indices[tex_bindings[i].bindless_descriptor_index]);
        }
        for (size_t i = 0; i < smp_bindings.size() && i < 8; ++i) {
          fprintf(stderr, "[metal]   smp[%zu] di=%u fc=%u → smp=%u\n",
                  i, smp_bindings[i].bindless_descriptor_index,
                  smp_bindings[i].fetch_constant,
                  desc_indices[smp_bindings[i].bindless_descriptor_index]);
        }
        fflush(stderr);
      }
      }
    }
  };
  write_descriptor_indices(vertex_shader, vertex_desc_idx_ptr);
  write_descriptor_indices(pixel_shader, pixel_desc_idx_ptr);

  uniforms_ring_offset_ = base_offset + total_size;
  uniforms_ring_offset_ = (uniforms_ring_offset_ + 255) & ~255;

  MTL::Viewport mtl_vp;
  mtl_vp.originX = double(viewport_info.xy_offset[0]);
  mtl_vp.originY = double(viewport_info.xy_offset[1]);
  mtl_vp.width = double(std::min(viewport_info.xy_extent[0], rt_w));
  mtl_vp.height = double(std::min(viewport_info.xy_extent[1], rt_h));
  mtl_vp.znear = double(viewport_info.z_min);
  mtl_vp.zfar = double(viewport_info.z_max);

  if (mtl_vp.width > 0 && mtl_vp.height > 0) {
    current_render_encoder_->setViewport(mtl_vp);
  }
  {
    static std::atomic<int> viewport_probe{0};
    int vp = viewport_probe.fetch_add(1);
    if (vp < 12) {
      fprintf(stderr,
              "[metal] VP PROBE #%d: viewport=%.1fx%.1f+%.1f+%.1f "
              "z=%.6f..%.6f rt=%ux%u ndc=(%.6f,%.6f,%.6f)+(%.6f,%.6f,%.6f)\n",
              vp, mtl_vp.width, mtl_vp.height, mtl_vp.originX,
              mtl_vp.originY, mtl_vp.znear, mtl_vp.zfar, rt_w, rt_h,
              sys_consts->ndc_scale[0], sys_consts->ndc_scale[1],
              sys_consts->ndc_scale[2], sys_consts->ndc_offset[0],
              sys_consts->ndc_offset[1], sys_consts->ndc_offset[2]);
      fflush(stderr);
    }
  }

  static std::atomic<int> vp_count{0};
  int vpc = vp_count.fetch_add(1);
  if constexpr (kMetalVerboseDiagnostics) {
  if (vpc < 10) {
    fprintf(stderr, "[metal] DIAG viewport #%d: %.0fx%.0f+%.0f+%.0f rt=%ux%u ndc_scale=(%.3f,%.3f,%.3f) ndc_off=(%.3f,%.3f,%.3f)\n",
            vpc, mtl_vp.width, mtl_vp.height, mtl_vp.originX, mtl_vp.originY,
            rt_w, rt_h,
            sys_consts->ndc_scale[0], sys_consts->ndc_scale[1], sys_consts->ndc_scale[2],
            sys_consts->ndc_offset[0], sys_consts->ndc_offset[1], sys_consts->ndc_offset[2]);
    auto* fc = reinterpret_cast<float*>(float_ptr);
    fprintf(stderr, "[metal] DIAG VS float_consts[0-3]=(%.3f,%.3f,%.3f,%.3f) [4-7]=(%.3f,%.3f,%.3f,%.3f)\n",
            fc[0], fc[1], fc[2], fc[3], fc[4], fc[5], fc[6], fc[7]);
    auto* pfc = reinterpret_cast<float*>(pixel_float_ptr);
    fprintf(stderr, "[metal] DIAG PS float_consts[0-3]=(%.3f,%.3f,%.3f,%.3f) [4-7]=(%.3f,%.3f,%.3f,%.3f)\n",
            pfc[0], pfc[1], pfc[2], pfc[3], pfc[4], pfc[5], pfc[6], pfc[7]);
    if (pixel_shader) {
      auto& crm = pixel_shader->constant_register_map();
      fprintf(stderr, "[metal] DIAG PS float_dynamic=%d float_count=%u bitmap=[%llX,%llX,%llX,%llX]\n",
              crm.float_dynamic_addressing, crm.float_count,
              (unsigned long long)crm.float_bitmap[0], (unsigned long long)crm.float_bitmap[1],
              (unsigned long long)crm.float_bitmap[2], (unsigned long long)crm.float_bitmap[3]);
    }
    auto* sc = reinterpret_cast<const uint32_t*>(base_ptr);
    fprintf(stderr, "[metal] DIAG sys_consts raw: [0-3]=%08X %08X %08X %08X\n",
            sc[0], sc[1], sc[2], sc[3]);
    fprintf(stderr, "[metal] DIAG sys_consts raw: [4-7]=%08X %08X %08X %08X\n",
            sc[4], sc[5], sc[6], sc[7]);
    fprintf(stderr, "[metal] DIAG sys_consts NDC: [32-35]=%08X %08X %08X %08X\n",
            sc[32], sc[33], sc[34], sc[35]);
    fprintf(stderr, "[metal] DIAG sys_consts NDC: [36-39]=%08X %08X %08X %08X\n",
            sc[36], sc[37], sc[38], sc[39]);
    auto* ndc_f = reinterpret_cast<const float*>(sc);
    fprintf(stderr, "[metal] DIAG sys_consts NDC: scale=(%.6f,%.6f,%.6f) off=(%.6f,%.6f,%.6f)\n",
            ndc_f[32], ndc_f[33], ndc_f[34], ndc_f[36], ndc_f[37], ndc_f[38]);
    auto* fetch0 = reinterpret_cast<uint32_t*>(fetch_ptr);
    fprintf(stderr, "[metal] DIAG fetch_const[0]: raw0=0x%08X raw1=0x%08X\n",
            fetch0[0], fetch0[1]);
    uint32_t fc0_addr = (fetch0[0] >> 2) & 0x3FFFFFFF;
    uint32_t fc0_size = (fetch0[1] >> 2) & 0xFFFFFF;
    fprintf(stderr, "[metal] DIAG fetch_const[0]: byte_addr=0x%08X size_bytes=%u\n",
            fc0_addr * 4, fc0_size * 4);
    if (shared_memory_) {
      MTL::Buffer* smb = shared_memory_->GetBuffer();
      uint32_t byte_addr = fc0_addr * 4;
      if (smb && byte_addr + 32 <= smb->length()) {
        auto* vdata = reinterpret_cast<const float*>(
            static_cast<const uint8_t*>(smb->contents()) + byte_addr);
        fprintf(stderr, "[metal] DIAG metal_buf @0x%X: (%.3f,%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f,%.3f)\n",
                byte_addr, vdata[0], vdata[1], vdata[2], vdata[3],
                vdata[4], vdata[5], vdata[6], vdata[7]);
      }
      auto* gram = reinterpret_cast<const float*>(
          shared_memory_->GetGuestRamPtr(byte_addr));
      if (gram) {
        fprintf(stderr, "[metal] DIAG guest_ram @0x%X: (%.3f,%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f,%.3f)\n",
                byte_addr, gram[0], gram[1], gram[2], gram[3],
                gram[4], gram[5], gram[6], gram[7]);
      }
    }
    auto* fetch1 = reinterpret_cast<uint32_t*>(fetch_ptr + 8);
    fprintf(stderr, "[metal] DIAG fetch_const[1]: raw0=0x%08X raw1=0x%08X\n",
            fetch1[0], fetch1[1]);
    if (vpc == 0) {
      auto* base = reinterpret_cast<const uint32_t*>(
          shared_memory_->GetGuestRamPtr(0));
      int nonzero_count = 0;
      for (uint32_t i = 0; i < 0x20000000 / 4; i += 0x40000) {
        if (base[i] != 0) {
          fprintf(stderr, "[metal] DIAG nonzero ram[0x%X]=%08X\n", i*4, base[i]);
          nonzero_count++;
          if (nonzero_count >= 30) break;
        }
      }
      fprintf(stderr, "[metal] DIAG scan done, found %d non-zero 1MB pages\n", nonzero_count);
    }
    fflush(stderr);
  }
  }

  MTL::ScissorRect scissor;
  draw_util::Scissor sc;
  draw_util::GetScissor(regs, sc);
  scissor.x = sc.offset[0];
  scissor.y = sc.offset[1];
  scissor.width = sc.extent[0];
  scissor.height = sc.extent[1];
  {
    static std::atomic<int> scissor_probe{0};
    int sp = scissor_probe.fetch_add(1);
    if (sp < 12) {
      fprintf(stderr,
              "[metal] SC PROBE #%d: scissor=%lux%lu+%lu+%lu raw=%ux%u+%u+%u rt=%ux%u\n",
              sp, scissor.width, scissor.height, scissor.x, scissor.y,
              sc.extent[0], sc.extent[1], sc.offset[0], sc.offset[1],
              color0 ? uint32_t(color0->width()) : 0,
              color0 ? uint32_t(color0->height()) : 0);
      fflush(stderr);
    }
  }
  {
    static std::atomic<int> sc_diag{0};
    int sd = sc_diag.fetch_add(1);
    if constexpr (kMetalVerboseDiagnostics) {
    if (sd < 5) {
      fprintf(stderr, "[metal] SCISSOR #%d: %ux%u+%u+%u rt=%ux%u\n",
              sd, (unsigned)scissor.width, (unsigned)scissor.height,
              (unsigned)scissor.x, (unsigned)scissor.y,
              color0 ? (unsigned)color0->width() : 0,
              color0 ? (unsigned)color0->height() : 0);
      fflush(stderr);
    }
    }
  }
  if (scissor.width > 0 && scissor.height > 0) {
    if (scissor.x + scissor.width > color0->width())
      scissor.width = color0->width() > scissor.x ? color0->width() - scissor.x : 0;
    if (scissor.y + scissor.height > color0->height())
      scissor.height = color0->height() > scissor.y ? color0->height() - scissor.y : 0;
    if (scissor.width > 0 && scissor.height > 0) {
      current_render_encoder_->setScissorRect(scissor);
    }
  }

}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
