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

constexpr bool kMetalVerboseDiagnostics = false;

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
    cbv_heap_ab_ = device_->newBuffer(kCBVHeapBytes, MTL::ResourceStorageModeShared);
    if (!cbv_heap_ab_) return false;
    std::memset(cbv_heap_ab_->contents(), 0, kCBVHeapBytes);

    const size_t kTopLevelABBytes = kTopLevelABSlots * sizeof(uint64_t);
    top_level_ab_ = device_->newBuffer(kTopLevelABBytes, MTL::ResourceStorageModeShared);
    if (!top_level_ab_) return false;

    auto* top_ptrs = reinterpret_cast<uint64_t*>(top_level_ab_->contents());
    std::memset(top_ptrs, 0, kTopLevelABBytes);

    uint64_t srv_base = res_heap_ab_->gpuAddress();
    uint64_t uav_base = res_heap_ab_->gpuAddress() +
                        kResourceHeapSlotsPerTable * sizeof(IRDescriptorTableEntry);
    uint64_t smp_base = smp_heap_ab_->gpuAddress();
    uint64_t cbv_base = cbv_heap_ab_->gpuAddress();

    for (int i = 0; i < 5; ++i) {
      top_ptrs[i] = srv_base;
    }
    for (int i = 5; i < 9; ++i) {
      top_ptrs[i] = uav_base;
    }
    top_ptrs[9] = smp_base;
    for (int i = 10; i < 14; ++i) {
      top_ptrs[i] = cbv_base;
    }
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

  if (top_level_ab_) { top_level_ab_->release(); top_level_ab_ = nullptr; }
  if (cbv_heap_ab_) { cbv_heap_ab_->release(); cbv_heap_ab_ = nullptr; }
  if (smp_heap_ab_) { smp_heap_ab_->release(); smp_heap_ab_ = nullptr; }
  if (res_heap_ab_) { res_heap_ab_->release(); res_heap_ab_ = nullptr; }
  if (draw_ring_pool_) { draw_ring_pool_->release(); draw_ring_pool_ = nullptr; }
  if (uniforms_ring_buffer_) { uniforms_ring_buffer_->release(); uniforms_ring_buffer_ = nullptr; }
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
      ui::GraphicsProvider::GpuVendorID::kApple, true, false);
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
  if constexpr (kMetalVerboseDiagnostics) {
  if (dc < 5) {
    fprintf(stderr, "[metal] IssueDraw #%d: prim=%d count=%d\n", dc, (int)primitive_type, index_count);
    fflush(stderr);
  }
  }

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

  PrimitiveProcessor::ProcessingResult primitive_processing_result;
  if (!primitive_processor_) {
    REXLOG_ERROR("IssueDraw: primitive processor not initialized");
    return false;
  }
  if (!primitive_processor_->Process(primitive_processing_result)) {
    REXLOG_ERROR("IssueDraw: primitive processing failed");
    return false;
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
  if (!vertex_translation->is_translated()) {
    if (!shader_translator_->TranslateAnalyzedShader(*vertex_translation)) {
      REXLOG_ERROR("Failed to translate vertex shader to DXBC");
      return false;
    }
  }
  if (!vertex_translation->is_valid()) {
    if (!vertex_translation->TranslateToMetal(device_, *dxbc_to_dxil_converter_,
                                               *metal_shader_converter_)) {
      REXLOG_ERROR("Failed to translate vertex shader to Metal");
      return false;
    }
  }

  MetalShader::MetalTranslation* pixel_translation = nullptr;
  if (pixel_shader) {
    pixel_translation = static_cast<MetalShader::MetalTranslation*>(
        pixel_shader->GetOrCreateTranslation(pixel_mod.value));
    if (!pixel_translation->is_translated()) {
      if (!shader_translator_->TranslateAnalyzedShader(*pixel_translation)) {
        REXLOG_ERROR("Failed to translate pixel shader to DXBC");
        return false;
      }
    }
    if (!pixel_translation->is_valid()) {
      if (!pixel_translation->TranslateToMetal(
              device_, *dxbc_to_dxil_converter_, *metal_shader_converter_)) {
        REXLOG_ERROR("Failed to translate pixel shader to Metal");
        return false;
      }
    }
  }

  MslPipelineCompileStatus pipeline_status = MslPipelineCompileStatus::kFailed;
  MTL::RenderPipelineState* pipeline = GetOrCreatePipelineState(
      vertex_translation, pixel_translation, regs, &pipeline_status);
  if (!pipeline) {
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

  if (shared_memory_) {
    const auto& vb_bindings = vertex_shader->vertex_bindings();
    int vdiag = 0;
    for (const auto& binding : vb_bindings) {
      xenos::xe_gpu_vertex_fetch_t vfetch = regs.GetVertexFetch(binding.fetch_constant);
      uint32_t buffer_offset = vfetch.address << 2;
      uint32_t buffer_length = vfetch.size << 2;
      if constexpr (kMetalVerboseDiagnostics) {
      if (metal_draw_count.load() < 10 && vdiag < 4) {
        uint8_t* phys_ptr = (uint8_t*)memory_->TranslatePhysical(buffer_offset);
        const void* shmem_base = shared_memory_->GetGuestRamPtr(0);
        const uint8_t* shmem_ptr = (const uint8_t*)shmem_base;
        uint32_t phys_val = buffer_length >= 4 ? *(uint32_t*)(phys_ptr) : 0;
        uint32_t shmem_val = (shmem_ptr && buffer_offset + 4 <= 0x20000000)
                                 ? *(uint32_t*)(shmem_ptr + buffer_offset) : 0xBADBAD;
        fprintf(stderr, "[metal] VFETCH[%d] fc=%d addr=0x%08X len=%u phys[0]=%08X shmem[0]=%08X\n",
                vdiag, binding.fetch_constant, buffer_offset, buffer_length, phys_val, shmem_val);
        fflush(stderr);
        vdiag++;
      }
      }
      if (!shared_memory_->RequestRange(buffer_offset, buffer_length)) {
        REXLOG_ERROR("Failed to request vertex buffer at 0x{:08X}", buffer_offset);
        return false;
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

  current_render_encoder_->setRenderPipelineState(pipeline);
  ApplyDepthStencilState(primitive_polygonal, normalized_depth_control);

  int md = metal_draw_count.fetch_add(1);
  if constexpr (kMetalVerboseDiagnostics) {
  if (md < 20) {
    fprintf(stderr, "[metal] METAL DRAW #%d: verts=%u prim=%d pipeline=%p enc=%p\n",
            md, primitive_processing_result.host_draw_vertex_count,
            (int)primitive_type, pipeline, current_render_encoder_);
    fflush(stderr);
  }
  }

  bool shared_memory_is_uav = memexport_used;
  BindResources(regs, shared_memory_is_uav);

  uint32_t draw_vertex_count = primitive_processing_result.host_draw_vertex_count;
  MTL::PrimitiveType metal_primitive_type;
  if (!GetMetalPrimitiveType(primitive_processing_result.host_primitive_type,
                             metal_primitive_type)) {
    REXLOG_ERROR("IssueDraw: unsupported Metal host primitive type {}",
                 uint32_t(primitive_processing_result.host_primitive_type));
    return false;
  }

  using PIBT = PrimitiveProcessor::ProcessedIndexBufferType;
  auto do_draw_indexed = [&](MTL::IndexType index_type, MTL::Buffer* index_buffer,
                              uint64_t index_offset) {
    struct DrawIndexedArgs {
      uint32_t index_count_per_instance;
      uint32_t instance_count;
      uint32_t start_index_location;
      int32_t base_vertex_location;
      uint32_t start_instance_location;
    };
    DrawIndexedArgs da = {
      draw_vertex_count, 1,
      static_cast<uint32_t>(index_offset), 0, 0
    };
    uint16_t ir_index_type = static_cast<uint16_t>(index_type + 1);
    struct {
      uint16_t index_type;
      uint16_t pad0;
      uint32_t pad1;
    } frag_uniforms = { ir_index_type, 0, 0 };
    current_render_encoder_->setVertexBytes(&da, sizeof(da), MscBufferIndex::kDrawArguments);
    current_render_encoder_->setVertexBytes(&ir_index_type, sizeof(ir_index_type), MscBufferIndex::kUniforms);
    current_render_encoder_->setFragmentBytes(&frag_uniforms, sizeof(frag_uniforms), MscBufferIndex::kUniforms);
    current_render_encoder_->drawIndexedPrimitives(
        metal_primitive_type, NS::UInteger(draw_vertex_count),
        index_type, index_buffer, index_offset, NS::UInteger(1), 0, 0);
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
    MTL::Buffer* builtin_buffer = primitive_processor_->GetBuiltinIndexBuffer();
    if (builtin_buffer) {
      MTL::IndexType index_type =
          primitive_processing_result.host_index_format == xenos::IndexFormat::kInt16
              ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
      UseRenderEncoderResource(builtin_buffer, MTL::ResourceUsageRead);
      do_draw_indexed(index_type, builtin_buffer, 0);
    }
  } else {
    struct DrawArgs {
      uint32_t vertex_count_per_instance;
      uint32_t instance_count;
      uint32_t start_vertex_location;
      uint32_t start_instance_location;
    };
    DrawArgs da = { draw_vertex_count, 1, 0, 0 };
    uint16_t ir_index_type = 0;
    struct {
      uint16_t index_type;
      uint16_t pad0;
      uint32_t pad1;
    } frag_uniforms = { ir_index_type, 0, 0 };
    current_render_encoder_->setVertexBytes(&da, sizeof(da), MscBufferIndex::kDrawArguments);
    current_render_encoder_->setVertexBytes(&ir_index_type, sizeof(ir_index_type), MscBufferIndex::kUniforms);
    current_render_encoder_->setFragmentBytes(&frag_uniforms, sizeof(frag_uniforms), MscBufferIndex::kUniforms);
    current_render_encoder_->drawPrimitives(
        metal_primitive_type, NS::UInteger(0), NS::UInteger(draw_vertex_count));
  }

  ++current_draw_index_;
  return true;
}

bool MetalCommandProcessor::IssueCopy() {
  const RegisterFile& regs = *register_file_;
  if (!render_target_cache_) return false;

  copy_resolve_writes_pending_ = true;

  BeginCommandBuffer();
  if (!current_command_buffer_) return false;

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
  if (texture_cache_) {
    swap_texture = texture_cache_->RequestSwapTexture(
        swap_width_scaled, swap_height_scaled, swap_format,
        &swap_width_unscaled, &swap_height_unscaled);
  }
  if (!swap_texture && render_target_cache_) {
    swap_texture = render_target_cache_->GetColorTarget(0);
    swap_width_scaled = swap_texture ? swap_texture->width() : 0;
    swap_height_scaled = swap_texture ? swap_texture->height() : 0;
  }
  if (swap_texture) {
    auto& provider = GetMetalProvider();
    provider.SetFrontbufferTexture(swap_texture);

    if constexpr (kMetalVerboseDiagnostics) {
    if (sc < 3) {
      fprintf(stderr,
              "[metal] SWAP texture: fmt=%d %ux%u unscaled=%ux%u packet=%ux%u tex=%p\n",
              (int)swap_texture->pixelFormat(), (unsigned)swap_width_scaled,
              (unsigned)swap_height_scaled, (unsigned)swap_width_unscaled,
              (unsigned)swap_height_unscaled, frontbuffer_width,
              frontbuffer_height, swap_texture);
      fflush(stderr);
    }
    }
  }

  EndCommandBuffer();

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
  if (normalized_depth_control.z_enable) {
    desc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
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
    const ::rex::graphics::RegisterFile& regs, bool shared_memory_is_uav) {
  if (!current_render_encoder_) return;

  using namespace MscHeapLayout;
  using namespace MscBufferIndex;

  WriteSystemConstants(regs);

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
    uint32_t texture_count = texture_cache_->GetBoundTextureCount();
    for (uint32_t i = 0; i < texture_count && i < kResourceHeapSlotsPerTable - 1; ++i) {
      MTL::Texture* tex = texture_cache_->GetBoundTexture(i);
      if (tex) {
        SetDescriptorTexture(&res_entries[1 + i], tex);
        UseRenderEncoderResource(tex, MTL::ResourceUsageRead);
      }
    }

    auto* smp_entries = reinterpret_cast<IRDescriptorTableEntry*>(smp_heap_ab_->contents());
    for (uint32_t i = 0; i < texture_count && i < kSamplerHeapSlotsPerTable; ++i) {
      MTL::SamplerState* sampler = texture_cache_->GetBoundSamplerState(i);
      if (sampler) {
        SetDescriptorSampler(&smp_entries[i], sampler);
      }
    }
  }

  {
    auto* cbv_entries = reinterpret_cast<IRDescriptorTableEntry*>(cbv_heap_ab_->contents());
    uint64_t uniforms_base = uniforms_ring_buffer_->gpuAddress();
    size_t base_offset = (uniforms_ring_offset_ > kUniformsBytesPerTable)
        ? uniforms_ring_offset_ - kUniformsBytesPerTable : 0;

    SetDescriptorBuffer(&cbv_entries[0],
                                uniforms_base + base_offset,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[1],
                                uniforms_base + base_offset + 1 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[2],
                                uniforms_base + base_offset + 2 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[3],
                                uniforms_base + base_offset + 3 * kCbvSizeBytes,
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[4],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[5],
                                null_buffer_->gpuAddress(),
                                kCbvSizeBytes);
    SetDescriptorBuffer(&cbv_entries[6],
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
  if (cbv_heap_ab_) {
    UseRenderEncoderResource(cbv_heap_ab_, MTL::ResourceUsageRead);
  }
  if (res_heap_ab_) {
    UseRenderEncoderResource(res_heap_ab_, MTL::ResourceUsageRead);
  }
  if (smp_heap_ab_) {
    UseRenderEncoderResource(smp_heap_ab_, MTL::ResourceUsageRead);
  }
  if (top_level_ab_) {
    UseRenderEncoderResource(top_level_ab_, MTL::ResourceUsageRead);
  }

  current_render_encoder_->setVertexBuffer(res_heap_ab_, 0, kDescriptorHeap);
  current_render_encoder_->setFragmentBuffer(res_heap_ab_, 0, kDescriptorHeap);

  current_render_encoder_->setVertexBuffer(smp_heap_ab_, 0, kSamplerHeap);
  current_render_encoder_->setFragmentBuffer(smp_heap_ab_, 0, kSamplerHeap);

  current_render_encoder_->setVertexBuffer(top_level_ab_, 0, kArgumentBuffer);
  current_render_encoder_->setFragmentBuffer(top_level_ab_, 0, kArgumentBuffer);
}

void MetalCommandProcessor::WriteSystemConstants(
    const ::rex::graphics::RegisterFile& regs) {
  if (!current_render_encoder_ || !uniforms_ring_data_) return;

  using DxbcTranslator = DxbcShaderTranslator;
  using namespace MscHeapLayout;

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
  draw_util::ViewportInfo viewport_info;
  draw_util::GetHostViewportInfo(regs, 1, 1, false, rt_w, rt_h,
                                 true, draw_util::GetNormalizedDepthControl(regs),
                                 false, false, false, viewport_info);
  sys_consts->ndc_scale[0] = viewport_info.ndc_scale[0];
  sys_consts->ndc_scale[1] = viewport_info.ndc_scale[1];
  sys_consts->ndc_scale[2] = viewport_info.ndc_scale[2];
  sys_consts->ndc_offset[0] = viewport_info.ndc_offset[0];
  sys_consts->ndc_offset[1] = viewport_info.ndc_offset[1];
  sys_consts->ndc_offset[2] = viewport_info.ndc_offset[2];

  uint8_t* float_ptr = base_ptr + 1 * kCbvSizeBytes;
  std::memset(float_ptr, 0, kCbvSizeBytes);
  size_t float_constants_size = 512 * 4 * sizeof(float);
  std::memcpy(float_ptr, &regs.values[0x4000],
              std::min(float_constants_size, kCbvSizeBytes));

  uint8_t* bool_loop_ptr = base_ptr + 2 * kCbvSizeBytes;
  std::memset(bool_loop_ptr, 0, kCbvSizeBytes);
  {
    auto* bool_loop = reinterpret_cast<uint32_t*>(bool_loop_ptr);
    for (uint32_t i = 0; i < 8; i++) {
      bool_loop[i] = regs.values[0x4000 + 4 * (256 + i * 8)];
    }
    auto* loop_consts = bool_loop + 8;
    for (uint32_t i = 0; i < 8; i++) {
      loop_consts[i] = regs.values[0x4000 + 4 * (256 + i * 8) + 1];
    }
  }

  uint8_t* fetch_ptr = base_ptr + 3 * kCbvSizeBytes;
  std::memset(fetch_ptr, 0, kCbvSizeBytes);
  {
    auto* fetch_consts = reinterpret_cast<xenos::xe_gpu_vertex_fetch_t*>(fetch_ptr);
    for (uint32_t i = 0; i < 96; i++) {
      fetch_consts[i] = regs.GetVertexFetch(i);
    }
  }

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
    fprintf(stderr, "[metal] DIAG float_consts[0-3]=(%.3f,%.3f,%.3f,%.3f) [4-7]=(%.3f,%.3f,%.3f,%.3f)\n",
            fc[0], fc[1], fc[2], fc[3], fc[4], fc[5], fc[6], fc[7]);
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
