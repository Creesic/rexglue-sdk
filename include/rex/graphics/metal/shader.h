#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <Metal/Metal.hpp>
#include <rex/graphics/pipeline/shader/dxbc.h>

namespace rex::graphics::metal {

class DxbcToDxilConverter;
class MetalShaderCache;
class MetalShaderConverter;

class MetalShader : public DxbcShader {
 public:
  MetalShader(xenos::ShaderType shader_type, uint64_t ucode_data_hash,
              const uint32_t* ucode_dwords, size_t ucode_dword_count,
              std::endian ucode_source_endian = std::endian::big);

  class MetalTranslation : public Translation {
   public:
    MetalTranslation(Shader& shader, uint64_t modification)
        : Translation(shader, modification) {}
    ~MetalTranslation() override;

    bool TranslateToMetal(MTL::Device* device,
                          DxbcToDxilConverter& dxbc_converter,
                          MetalShaderConverter& metal_converter);

    MTL::Function* metal_function() const { return metal_function_; }
    MTL::Library* metal_library() const { return metal_library_; }
    const std::string& function_name() const { return function_name_; }
    const std::vector<uint8_t>& metallib_data() const { return metallib_data_; }
    const std::vector<uint8_t>& dxil_data() const { return dxil_data_; }

   private:
    MTL::Function* metal_function_ = nullptr;
    MTL::Library* metal_library_ = nullptr;
    std::string function_name_;
    std::vector<uint8_t> metallib_data_;
    std::vector<uint8_t> dxil_data_;
  };

  Translation* CreateTranslationInstance(uint64_t modification) override;

  size_t GetTextureBindingLayoutUserUID() const { return texture_binding_layout_user_uid_; }
  size_t GetSamplerBindingLayoutUserUID() const { return sampler_binding_layout_user_uid_; }
  bool EnterBindingLayoutUserUIDSetup() { return !binding_layout_user_uids_set_up_.test_and_set(); }
  void SetTextureBindingLayoutUserUID(size_t uid) { texture_binding_layout_user_uid_ = uid; }
  void SetSamplerBindingLayoutUserUID(size_t uid) { sampler_binding_layout_user_uid_ = uid; }

 private:
  std::atomic_flag binding_layout_user_uids_set_up_ = ATOMIC_FLAG_INIT;
  size_t texture_binding_layout_user_uid_ = 0;
  size_t sampler_binding_layout_user_uid_ = 0;
};

}  // namespace rex::graphics::metal
