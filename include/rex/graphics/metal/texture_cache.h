#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <Metal/Metal.hpp>
#include <rex/graphics/pipeline/texture/cache.h>
#include <rex/graphics/pipeline/shader/dxbc.h>
#include <rex/graphics/register_file.h>
#include <rex/graphics/xenos.h>
#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime.h>

namespace rex::graphics {
class SharedMemory;
}  // namespace rex::graphics

namespace rex::graphics::metal {

class MetalCommandProcessor;
class MetalHeapPool;
class MetalSharedMemory;

class MetalTextureCache : public TextureCache {
 public:
  union SamplerParameters {
    uint32_t value;
    struct {
      xenos::ClampMode clamp_x : 3;
      xenos::ClampMode clamp_y : 3;
      xenos::ClampMode clamp_z : 3;
      xenos::BorderColor border_color : 2;
      uint32_t mag_linear : 1;
      uint32_t min_linear : 1;
      uint32_t mip_linear : 1;
      xenos::AnisoFilter aniso_filter : 3;
      uint32_t mip_min_level : 4;
      uint32_t mip_base_map : 1;
    };

    SamplerParameters() : value(0) {}
    bool operator==(const SamplerParameters& parameters) const { return value == parameters.value; }
    bool operator!=(const SamplerParameters& parameters) const { return value != parameters.value; }
  };

  struct TextureSRVKey {
    TextureKey key;
    uint32_t host_swizzle;
    uint8_t swizzled_signs;
  };

  MetalTextureCache(MetalCommandProcessor* command_processor,
                    const RegisterFile& register_file,
                    MetalSharedMemory& shared_memory,
                    uint32_t draw_resolution_scale_x,
                    uint32_t draw_resolution_scale_y);
  ~MetalTextureCache();

  bool Initialize();
  void Shutdown();

  uint32_t draw_resolution_scale_x() const { return draw_resolution_scale_x_; }
  uint32_t draw_resolution_scale_y() const { return draw_resolution_scale_y_; }

  MTL::StorageMode GetCacheTextureStorageMode() const;
  bool ShouldUploadViaBlit() const;
  bool CanUseCurrentCommandBufferForTextureUploads() const;

  void BeginUploadCommandBufferBatch();
  void EndUploadCommandBufferBatch();
  void AbortUploadCommandBufferBatch(bool commit_if_has_work = false);

  MTL::Texture* RequestSwapTexture(uint32_t& width_out, uint32_t& height_out,
                                   xenos::TextureFormat& format_out);

  void RequestTextures(uint32_t used_texture_mask);

  bool TrimViewBindlessPressure(uint32_t target_available = 0);

  bool MakeScaledResolveRangeCurrent(uint32_t start_unscaled, uint32_t length_unscaled,
                                     uint32_t length_scaled_alignment_log2 = 4);
  bool GetCurrentScaledResolveBuffer(MTL::Buffer*& buffer_out,
                                     size_t& offset_out, size_t& length_out) const;
  bool IsScaledResolveRangeResident(uint32_t address, uint32_t size,
                                    uint32_t alignment) const;

  SamplerParameters GetSamplerParameters(const DxbcShader::SamplerBinding& binding) const;

  MTL::Texture* GetTextureForBinding(uint32_t fetch_constant,
                                     xenos::FetchOpDimension dimension,
                                     bool is_signed);

  uint32_t GetBindlessSRVIndexForBinding(uint32_t fetch_constant,
                                         xenos::FetchOpDimension dimension,
                                         bool is_signed);
  uint32_t GetBindlessSamplerIndexForBinding(const DxbcShader::SamplerBinding& binding);

  bool AreActiveTextureSRVKeysUpToDate(
      const TextureSRVKey* keys,
      const DxbcShader::TextureBinding* bindings, size_t count) const;
  void WriteActiveTextureSRVKeys(
      TextureSRVKey* keys_out,
      const DxbcShader::TextureBinding* bindings, size_t count) const;

  MTL::Texture* GetNullTexture2D() const { return null_texture_2d_; }
  MTL::Texture* GetNullTexture3D() const { return null_texture_3d_; }
  MTL::Texture* GetNullTextureCube() const { return null_texture_cube_; }

  // TextureCache pure virtual overrides
  void ClearCache() override;
  void CompletedSubmissionUpdated(uint64_t completed_submission_index) override;
  bool EnsureScaledResolveMemoryCommitted(uint32_t start_unscaled,
                                          uint32_t length_unscaled,
                                          uint32_t length_scaled_alignment_log2 = 0) override;
  bool IsSignedVersionSeparateForFormat(TextureKey key) const override;
  bool IsScaledResolveSupportedForFormat(TextureKey key) const override;
  uint32_t GetHostFormatSwizzle(TextureKey key) const override;
  uint32_t GetMaxHostTextureWidthHeight(xenos::DataDimension dimension) const override;
  uint32_t GetMaxHostTextureDepthOrArraySize(xenos::DataDimension dimension) const override;
  std::unique_ptr<Texture> CreateTexture(TextureKey key) override;
  bool LoadTextureDataFromResidentMemoryImpl(Texture& texture, bool load_base,
                                              bool load_mips) override;

 protected:
  class MetalTexture : public TextureCache::Texture {
   public:
    MetalTexture(MetalTextureCache& texture_cache, const TextureKey& key,
                 MTL::Texture* metal_texture, bool track_usage = true,
                 bool is_3d_as_2d_wrapper = false);
    ~MetalTexture() override;

    MTL::Texture* metal_texture() const { return metal_texture_; }
    void SetMetalTexture(MTL::Texture* tex) { metal_texture_ = tex; }

    bool HasBindlessViews() const;
    void LinkBindlessUsage();
    void UnlinkBindlessUsage();
    void MarkBindlessViewsUsed();
    void ReleaseBindlessViews();
    bool ReleaseBindlessViewsIfUnused(uint64_t completed_submission_index);

    uint64_t GetViewKey(uint32_t host_swizzle,
                       xenos::FetchOpDimension dimension, bool is_signed,
                       MTL::PixelFormat view_format) const;
    MTL::PixelFormat GetViewPixelFormat(bool is_signed) const;
    MTL::TextureType GetViewType(xenos::FetchOpDimension dimension) const;
    uint32_t GetOrCreateBindlessSRVIndexForResolvedView(
        uint64_t view_key, MTL::Texture* view);
    MTL::Texture* GetOrCreateView(uint32_t host_swizzle,
                                  xenos::FetchOpDimension dimension,
                                  bool is_signed);
    uint32_t GetOrCreateBindlessSRVIndex(uint32_t host_swizzle,
                                         xenos::FetchOpDimension dimension,
                                         bool is_signed);
    MTL::Texture* GetOrCreate3DAs2DView(uint32_t host_swizzle,
                                        xenos::FetchOpDimension dimension,
                                        bool is_signed);

   private:
    MetalTextureCache& texture_cache_;
    MTL::Texture* metal_texture_ = nullptr;
    bool is_3d_as_2d_wrapper_ = false;

    uint32_t bindless_srv_index_ = UINT32_MAX;
    std::unordered_map<uint64_t, uint32_t> swizzled_view_bindless_srv_indices_;
    MetalTexture* bindless_previous_ = nullptr;
    MetalTexture* bindless_next_ = nullptr;
    bool in_bindless_usage_list_ = false;
    uint64_t bindless_last_usage_submission_index_ = 0;

    std::unordered_map<uint64_t, MTL::Texture*> swizzled_view_cache_;
    std::unique_ptr<MetalTexture> texture_3d_as_2d_;
  };

 private:
  class UploadBufferPool;
  friend class UploadBufferPool;

  struct Norm16Selection {
    bool unsigned_uses_float = false;
    bool signed_uses_float = false;
  };

  bool IsDecompressionNeededForKey(TextureKey key) const;
  TextureCache::LoadShaderIndex GetLoadShaderIndexForKey(TextureKey key) const;
  MTL::PixelFormat GetPixelFormatForKey(TextureKey key) const;
  bool TryGpuLoadTexture(Texture& texture, bool load_base, bool load_mips);

  bool EnsureViewBindlessHeadroom(uint32_t target_free_slots) const;

  void InitializeNorm16Selection(MTL::Device* device);

  MTL::Texture* CreateNullTexture2D();
  MTL::Texture* CreateNullTexture3D();
  MTL::Texture* CreateNullTextureCube();

  bool InitializeLoadPipelines();

  MTL::SamplerState* GetOrCreateSampler(SamplerParameters parameters);
  void ReleaseOrRetireSamplerState(MTL::SamplerState* sampler);
  MTL::Texture* CreateTexture2D(uint32_t width, uint32_t height,
                                uint32_t array_length, MTL::PixelFormat format,
                                MTL::TextureSwizzleChannels swizzle,
                                uint32_t mip_levels);
  MTL::Texture* CreateTexture3D(uint32_t width, uint32_t height,
                                uint32_t depth, MTL::PixelFormat format,
                                MTL::TextureSwizzleChannels swizzle,
                                uint32_t mip_levels);
  MTL::Texture* CreateTextureCube(uint32_t size, MTL::PixelFormat format,
                                  MTL::TextureSwizzleChannels swizzle,
                                  uint32_t mip_levels, uint32_t cube_count);
  xenos::ClampMode NormalizeClampMode(xenos::ClampMode clamp_mode) const;

  struct ScaledResolveBuffer {
    MTL::Buffer* buffer = nullptr;
    uint64_t base_scaled = 0;
    uint64_t length_scaled = 0;
  };
  struct RetiredScaledResolveBuffer {
    MTL::Buffer* buffer = nullptr;
    uint64_t submission_id = 0;
    uint64_t length_scaled = 0;
  };
  bool GetScaledResolveRange(uint32_t start_unscaled, uint32_t length_unscaled,
                             uint32_t length_scaled_alignment_log2,
                             uint64_t& start_scaled_out,
                             uint64_t& length_scaled_out) const;
  bool EnsureScaledResolveBufferRange(uint64_t start_scaled,
                                      uint64_t length_scaled);
  void ClearScaledResolveBuffers();

  MetalCommandProcessor* command_processor_;
  uint32_t draw_resolution_scale_x_;
  uint32_t draw_resolution_scale_y_;

  bool supports_bc_texture_compression_ = false;

  Norm16Selection r16_selection_;
  Norm16Selection rg16_selection_;
  Norm16Selection rgba16_selection_;

  MTL::Texture* null_texture_2d_ = nullptr;
  MTL::Texture* null_texture_3d_ = nullptr;
  MTL::Texture* null_texture_cube_ = nullptr;
  uint32_t null_texture_2d_bindless_index_ = UINT32_MAX;
  uint32_t null_texture_3d_bindless_index_ = UINT32_MAX;
  uint32_t null_texture_cube_bindless_index_ = UINT32_MAX;

  MTL::SamplerState* null_sampler_bindless_ = nullptr;
  uint32_t null_sampler_bindless_index_ = UINT32_MAX;

  struct RetiredSamplerState {
    MTL::SamplerState* sampler = nullptr;
    uint64_t submission_id = 0;
  };
  std::vector<RetiredSamplerState> retired_sampler_states_;

  // Load pipelines
  static constexpr size_t kLoadShaderCount = 64;
  MTL::ComputePipelineState* load_pipelines_[kLoadShaderCount] = {};
  MTL::ComputePipelineState* load_pipelines_scaled_[kLoadShaderCount] = {};

  std::unique_ptr<MetalHeapPool> texture_heap_pool_;

  std::shared_ptr<UploadBufferPool> upload_buffer_pool_;
  std::mutex upload_buffer_pool_mutex_;
  MTL4::CommandBuffer* upload_batch_command_buffer_ = nullptr;
  bool upload_batch_command_buffer_has_work_ = false;
  uint32_t upload_batch_depth_ = 0;

  std::unordered_map<uint32_t, MTL::SamplerState*> sampler_cache_;
  std::unordered_map<uint32_t, uint32_t> sampler_bindless_indices_;

  class MetalTexture* bindless_used_first_ = nullptr;
  class MetalTexture* bindless_used_last_ = nullptr;

  std::vector<ScaledResolveBuffer> scaled_resolve_buffers_;
  std::vector<RetiredScaledResolveBuffer> scaled_resolve_retired_buffers_;
  uint64_t scaled_resolve_retired_bytes_ = 0;
  size_t scaled_resolve_current_buffer_index_ = size_t(-1);
  uint64_t scaled_resolve_current_range_start_scaled_ = 0;
  uint64_t scaled_resolve_current_range_length_scaled_ = 0;
};

}  // namespace rex::graphics::metal
