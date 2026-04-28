#include <rex/graphics/metal/command_processor.h>

#include <rex/graphics/graphics_system.h>
#include <rex/graphics/metal/shared_memory.h>
#include <rex/graphics/metal/texture_cache.h>
#include <rex/graphics/metal/render_target_cache.h>
#include <rex/graphics/metal/primitive_processor.h>
#include <rex/graphics/util/draw.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/presenter.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/system/kernel_state.h>
#include <rex/logging/macros.h>
#include <rex/assert.h>
#include <xxhash.h>

#include <algorithm>
#include <cstring>

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
  fprintf(stderr, "[metal] SetupContext: starting\n"); fflush(stderr);

  if (!graphics_system_ || !graphics_system_->provider()) {
    fprintf(stderr, "[metal] SetupContext: no graphics system or provider\n"); fflush(stderr);
    return false;
  }

  auto& provider = *static_cast<rex::ui::metal::MetalProvider*>(graphics_system_->provider());
  device_ = provider.GetDevice();
  command_queue_ = provider.GetCommandQueue();
  if (!device_ || !command_queue_) {
    fprintf(stderr, "[metal] SetupContext: No Metal device=%p or command_queue=%p\n", device_, command_queue_); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] SetupContext: device=%s\n", device_->name()->utf8String()); fflush(stderr);

  mesh_shader_supported_ = device_->supportsFamily(MTL::GPUFamilyApple6);

  wait_shared_event_ = device_->newSharedEvent();
  if (!wait_shared_event_) {
    fprintf(stderr, "[metal] SetupContext: failed to create shared event\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] SetupContext: creating buffers\n"); fflush(stderr);

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

  shared_memory_ = std::make_unique<MetalSharedMemory>(*this,
      *graphics_system_->kernel_state()->memory());
  if (!shared_memory_->Initialize()) {
    fprintf(stderr, "[metal] SetupContext: shared_memory init failed\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] SetupContext: shared_memory OK, creating primitive_processor\n"); fflush(stderr);

  primitive_processor_ = std::make_unique<MetalPrimitiveProcessor>(
      *this, *register_file_,
      *graphics_system_->kernel_state()->memory(),
      trace_writer_, *shared_memory_);
  if (!primitive_processor_->Initialize()) {
    fprintf(stderr, "[metal] SetupContext: primitive_processor init failed\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] SetupContext: primitive_processor OK, creating caches\n"); fflush(stderr);

  texture_cache_ = std::make_unique<MetalTextureCache>(
      *register_file_, *shared_memory_, 1, 1, *this);

  render_target_cache_ = std::make_unique<MetalRenderTargetCache>(
      *register_file_,
      *graphics_system_->kernel_state()->memory(),
      trace_writer_, 1, 1, *this);
  if (!render_target_cache_->Initialize()) {
    fprintf(stderr, "[metal] SetupContext: render target cache init failed\n"); fflush(stderr);
    return false;
  }

  if (!InitializeShaderTranslation()) {
    fprintf(stderr, "[metal] SetupContext: shader translation init failed\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] SetupContext: complete (mesh_shaders=%d)\n", mesh_shader_supported_); fflush(stderr);
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
  fprintf(stderr, "[metal] InitializeShaderTranslation: creating DXBC converter\n"); fflush(stderr);
  dxbc_to_dxil_converter_ = std::make_unique<DxbcToDxilConverter>();
  if (!dxbc_to_dxil_converter_->Initialize()) {
    fprintf(stderr, "[metal] InitializeShaderTranslation: DXBC converter init failed\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] InitializeShaderTranslation: creating Metal shader converter\n"); fflush(stderr);
  metal_shader_converter_ = std::make_unique<MetalShaderConverter>();
  if (!metal_shader_converter_->Initialize()) {
    fprintf(stderr, "[metal] InitializeShaderTranslation: Metal converter init failed\n"); fflush(stderr);
    return false;
  }

  fprintf(stderr, "[metal] InitializeShaderTranslation: creating DXBC translator\n"); fflush(stderr);
  shader_translator_ = std::make_unique<DxbcShaderTranslator>(
      ui::GraphicsProvider::GpuVendorID::kApple, true, false);
  fprintf(stderr, "[metal] InitializeShaderTranslation: complete\n"); fflush(stderr);
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
  fprintf(stderr, "[metal] EndCommandBuffer: encoding signal\n"); fflush(stderr);

  uint64_t signal_value = ++submission_current_;
  current_command_buffer_->encodeSignalEvent(wait_shared_event_, signal_value);

  current_command_buffer_->addCompletedHandler(
      ^(MTL::CommandBuffer* buffer) {
        completed_command_buffers_.store(signal_value);
      });

  fprintf(stderr, "[metal] EndCommandBuffer: committing\n"); fflush(stderr);
  current_command_buffer_->commit();
  fprintf(stderr, "[metal] EndCommandBuffer: committed, clearing refs\n"); fflush(stderr);
  current_command_buffer_ = nullptr;
  uniforms_ring_offset_ = 0;
  draw_ring_offset_ = 0;

  if (command_buffer_autorelease_pool_) {
    command_buffer_autorelease_pool_ = nullptr;
  }
  fprintf(stderr, "[metal] EndCommandBuffer: done\n"); fflush(stderr);
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
  DxbcShaderTranslator::Modification mod;
  mod.value = 0;
  mod.vertex.host_vertex_shader_type = host_vertex_shader_type;
  mod.vertex.interpolator_mask = interpolator_mask;
  return mod;
}

DxbcShaderTranslator::Modification
MetalCommandProcessor::GetCurrentPixelShaderModification(
    const Shader& shader, uint32_t interpolator_mask, uint32_t param_gen_pos,
    reg::RB_DEPTHCONTROL normalized_depth_control,
    uint32_t normalized_color_mask) const {
  DxbcShaderTranslator::Modification mod;
  mod.value = 0;
  mod.pixel.interpolator_mask = interpolator_mask;
  return mod;
}

bool MetalCommandProcessor::IssueDraw(xenos::PrimitiveType primitive_type,
                                       uint32_t index_count,
                                       IndexBufferInfo* index_buffer_info,
                                       bool major_mode_explicit) {
  const RegisterFile& regs = *register_file_;
  uint32_t normalized_color_mask = 0;

  static std::atomic<int> entry_count{0};
  int ec = entry_count.fetch_add(1);

  xenos::EdramMode edram_mode = regs.Get<reg::RB_MODECONTROL>().edram_mode;
  if (edram_mode != xenos::EdramMode::kCopy && copy_resolve_writes_pending_) {
    EndCommandBuffer();
  }
  if (edram_mode == xenos::EdramMode::kCopy) {
    return IssueCopy();
  }

  Shader* vertex_shader = active_vertex_shader();
  if (!vertex_shader) {
    REXLOG_WARN("IssueDraw: No vertex shader");
    return false;
  }
  if (!vertex_shader->is_ucode_analyzed()) {
    vertex_shader->AnalyzeUcode(ucode_disasm_buffer_);
  }
  bool memexport_used_vertex = vertex_shader->memexport_eM_written() != 0;

  bool primitive_polygonal = draw_util::IsPrimitivePolygonal(regs);
  bool is_rasterization_done =
      draw_util::IsRasterizationPotentiallyDone(regs, primitive_polygonal);
  static std::atomic<int> raster_count{0};
  if (is_rasterization_done) {
    int rc = raster_count.fetch_add(1);
    if (rc < 5) {
      fprintf(stderr, "[metal] IssueDraw RASTER #%d: prim=%d count=%d edram=%d\n",
              rc, (int)primitive_type, index_count, (int)edram_mode); fflush(stderr);
    }
  }
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
    if (!memexport_used_vertex) return true;
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
  if (!primitive_processing_result.host_draw_vertex_count) return true;
  if (primitive_processing_result.host_vertex_shader_type ==
      Shader::HostVertexShaderType::kMemExportCompute) {
    primitive_processing_result.host_vertex_shader_type =
        Shader::HostVertexShaderType::kVertex;
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

  static std::atomic<int> id_count{0};
  int dc = id_count.fetch_add(1);
  if (dc < 5) {
    fprintf(stderr, "[metal] IssueDraw #%d: prim=%d index=%d cmd=%p enc=%p\n",
            dc, (int)primitive_type, index_count, current_command_buffer_, current_render_encoder_); fflush(stderr);
  }

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
    REXLOG_ERROR("Failed to create pipeline state");
    return false;
  }

  uint32_t used_texture_mask = metal_vertex_shader->GetUsedTextureMaskAfterTranslation();
  if (metal_pixel_shader) {
    used_texture_mask |= metal_pixel_shader->GetUsedTextureMaskAfterTranslation();
  }
  if (texture_cache_ && used_texture_mask &&
      texture_cache_->AnyUsedTextureRequestWorkPending(used_texture_mask)) {
    texture_cache_->RequestTextures(used_texture_mask);
  }

  if (shared_memory_) {
    const auto& vb_bindings = vertex_shader->vertex_bindings();
    for (const auto& binding : vb_bindings) {
      xenos::xe_gpu_vertex_fetch_t vfetch = regs.GetVertexFetch(binding.fetch_constant);
      uint32_t buffer_offset = vfetch.address << 2;
      uint32_t buffer_length = vfetch.size << 2;
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

  bool shared_memory_is_uav = memexport_used;
  BindResources(regs, shared_memory_is_uav);

  if (index_buffer_info) {
    if (index_buffer_info->guest_base) {
      uint32_t index_buffer_offset = index_buffer_info->guest_base;
      MTL::Buffer* index_buffer = shared_memory_->GetBuffer();
      if (index_buffer) {
        current_render_encoder_->drawIndexedPrimitives(
            MTL::PrimitiveTypeTriangle,
            index_count,
            index_buffer_info->format == xenos::IndexFormat::kInt16
                ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32,
            index_buffer, index_buffer_offset);
      }
    }
  } else {
    current_render_encoder_->drawPrimitives(
        MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(index_count));
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
  fprintf(stderr, "[metal] IssueSwap: ptr=0x%08X w=%u h=%u\n", frontbuffer_ptr, frontbuffer_width, frontbuffer_height); fflush(stderr);
  last_swap_ptr_ = frontbuffer_ptr;
  last_swap_width_ = frontbuffer_width;
  last_swap_height_ = frontbuffer_height;
  saw_swap_ = true;
  copy_resolve_writes_pending_ = false;

  fprintf(stderr, "[metal] IssueSwap: getting color0\n"); fflush(stderr);
  if (render_target_cache_) {
    MTL::Texture* color0 = render_target_cache_->GetColorTarget(0);
    if (color0) {
      auto& provider = GetMetalProvider();
      provider.SetFrontbufferTexture(color0);
    }
  }

  fprintf(stderr, "[metal] IssueSwap: EndCommandBuffer\n"); fflush(stderr);
  EndCommandBuffer();
  fprintf(stderr, "[metal] IssueSwap: EndCommandBuffer done\n"); fflush(stderr);

  if (!graphics_system_) {
    fprintf(stderr, "[metal] IssueSwap: no graphics_system_\n"); fflush(stderr);
    return;
  }
  ui::Presenter* presenter = graphics_system_->presenter();
  if (!presenter) {
    fprintf(stderr, "[metal] IssueSwap: no presenter\n"); fflush(stderr);
    return;
  }

  uint32_t guest_width = frontbuffer_width ? frontbuffer_width : 1280;
  uint32_t guest_height = frontbuffer_height ? frontbuffer_height : 720;

  system::X_VIDEO_MODE video_mode;
  kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
  uint32_t display_width = std::max(uint32_t(1), uint32_t(video_mode.display_width));
  uint32_t display_height = std::max(uint32_t(1), uint32_t(video_mode.display_height));
  fprintf(stderr, "[metal] IssueSwap: RefreshGuestOutput %ux%u display=%ux%u\n",
          guest_width, guest_height, display_width, display_height); fflush(stderr);

  bool ok = presenter->RefreshGuestOutput(
      guest_width, guest_height, display_width, display_height,
      [this](ui::Presenter::GuestOutputRefreshContext& context) -> bool {
        return true;
      });
  fprintf(stderr, "[metal] IssueSwap: RefreshGuestOutput returned %d\n", ok); fflush(stderr);
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
    }
  }
  if (request.depth_format != MTL::PixelFormatInvalid) {
    desc->setDepthAttachmentPixelFormat(request.depth_format);
  }
  if (request.stencil_format != MTL::PixelFormatInvalid) {
    desc->setStencilAttachmentPixelFormat(request.stencil_format);
  }

  NS::Error* error = nullptr;
  MTL::RenderPipelineState* pipeline =
      device_->newRenderPipelineState(desc, &error);
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

  uint64_t key = vertex_translation->modification();
  if (pixel_translation) {
    key ^= pixel_translation->modification() * 0x9E3779B97F4A7C15ULL;
  }
  auto it = pipeline_state_cache_.find(key);
  if (it != pipeline_state_cache_.end()) {
    if (compile_status_out) *compile_status_out = MslPipelineCompileStatus::kReady;
    return it->second;
  }

  MslPipelineCompileRequest request = {};
  request.pipeline_key = key;
  request.vertex_function = vertex_translation->metal_function();
  request.fragment_function = pixel_translation ? pixel_translation->metal_function() : nullptr;

  if (render_target_cache_) {
    auto* rt = render_target_cache_->GetOrCreateRenderTarget(regs);
    if (rt) {
      render_target_width_ = rt->key().GetWidth();
    }
  }

  if (render_target_cache_) {
    for (uint32_t i = 0; i < 4; ++i) {
      request.color_formats[i] = render_target_cache_->GetColorFormat(i);
    }
    request.depth_format = render_target_cache_->GetDepthFormat();
    request.stencil_format = render_target_cache_->GetStencilFormat();
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
  if (!current_render_encoder_ || !shared_memory_) return;

  MTL::Buffer* shared_mem_buffer = shared_memory_->GetBuffer();
  if (shared_mem_buffer) {
    MTL::ResourceUsage usage = shared_memory_is_uav
        ? (MTL::ResourceUsageRead | MTL::ResourceUsageWrite)
        : MTL::ResourceUsageRead;
    current_render_encoder_->setVertexBuffer(shared_mem_buffer, 0, 0);
    current_render_encoder_->setFragmentBuffer(shared_mem_buffer, 0, 0);
    UseRenderEncoderResource(shared_mem_buffer, usage);
  }

  if (texture_cache_) {
    uint32_t texture_count = texture_cache_->GetBoundTextureCount();
    for (uint32_t i = 0; i < texture_count && i < 16; ++i) {
      MTL::Texture* tex = texture_cache_->GetBoundTexture(i);
      if (tex) {
        current_render_encoder_->setVertexTexture(tex, i);
        current_render_encoder_->setFragmentTexture(tex, i);
        UseRenderEncoderResource(tex, MTL::ResourceUsageRead);
      }
    }
  }

  if (null_sampler_) {
    for (uint32_t i = 0; i < 16; ++i) {
      current_render_encoder_->setVertexSamplerState(null_sampler_, i);
      current_render_encoder_->setFragmentSamplerState(null_sampler_, i);
    }
  }
}

void MetalCommandProcessor::WriteSystemConstants(
    const ::rex::graphics::RegisterFile& regs) {
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
