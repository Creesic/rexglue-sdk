#include <rex/graphics/metal/shader.h>

#include <dispatch/dispatch.h>
#include <inttypes.h>
#include <atomic>
#include <cstring>

#ifndef DISPATCH_DATA_DESTRUCTOR_NONE
#define DISPATCH_DATA_DESTRUCTOR_NONE DISPATCH_DATA_DESTRUCTOR_DEFAULT
#endif

#include <rex/graphics/metal/dxbc_to_dxil_converter.h>
#include <rex/graphics/metal/shader_cache.h>
#include <rex/graphics/metal/shader_converter.h>
#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

MetalShader::MetalShader(xenos::ShaderType shader_type,
                         uint64_t ucode_data_hash,
                         const uint32_t* ucode_dwords,
                         size_t ucode_dword_count,
                         std::endian ucode_source_endian)
    : DxbcShader(shader_type, ucode_data_hash, ucode_dwords, ucode_dword_count,
                 ucode_source_endian) {}

MetalShader::MetalTranslation::~MetalTranslation() {
  if (metal_function_) {
    metal_function_->release();
    metal_function_ = nullptr;
  }
  if (metal_library_) {
    metal_library_->release();
    metal_library_ = nullptr;
  }
}

bool MetalShader::MetalTranslation::TranslateToMetal(
    MTL::Device* device, DxbcToDxilConverter& dxbc_converter,
    MetalShaderConverter& metal_converter) {
  if (!device) {
    REXLOG_ERROR("MetalShader: No Metal device provided");
    return false;
  }

  const std::vector<uint8_t>& dxbc_data = translated_binary();
  if (dxbc_data.empty()) {
    REXLOG_ERROR("MetalShader: No translated DXBC data available");
    return false;
  }

  const uint64_t shader_cache_key =
      MetalShaderCache::GetCacheKey(shader().ucode_data_hash(), modification(),
                                    static_cast<uint32_t>(shader().type()));

  if (false && g_metal_shader_cache && g_metal_shader_cache->IsInitialized()) {
    MetalShaderCache::CachedMetallib cached;
    if (g_metal_shader_cache->Load(shader_cache_key, &cached)) {
      NS::Error* error = nullptr;
      dispatch_data_t cached_data = dispatch_data_create(
          cached.metallib_data.data(), cached.metallib_data.size(), nullptr,
          DISPATCH_DATA_DESTRUCTOR_NONE);
      metal_library_ = device->newLibrary(cached_data, &error);
      dispatch_release(cached_data);
      if (metal_library_) {
        function_name_ = cached.function_name;
        metallib_data_ = std::move(cached.metallib_data);
        NS::String* fn =
            NS::String::string(function_name_.c_str(), NS::UTF8StringEncoding);
  metal_function_ = metal_library_->newFunction(fn);
  if (metal_function_) {
    static int func_count = 0;
    if (func_count++ < 3) {
      fprintf(stderr, "[metal] newFunction '%s' OK, argCount=%u\n",
              function_name_.c_str(),
              (unsigned)metal_function_->vertexAttributes()->count());
      auto* args = metal_function_->functionConstantsDictionary();
      if (args) {
        fprintf(stderr, "[metal]   functionConstants keys=%u\n",
                (unsigned)args->count());
      }
    }
  }
        if (metal_function_) {
          return true;
        }
        metal_library_->release();
        metal_library_ = nullptr;
      }
    }
  }

  std::string dxbc_error;
  if (!dxbc_converter.Convert(dxbc_data, dxil_data_, &dxbc_error)) {
    REXLOG_ERROR("MetalShader: DXBC to DXIL conversion failed: {}", dxbc_error);
    return false;
  }
  REXLOG_DEBUG("MetalShader: Converted {} bytes DXBC to {} bytes DXIL",
               dxbc_data.size(), dxil_data_.size());

  MetalShaderConversionResult msc_result;
  if (!metal_converter.Convert(shader().type(), dxil_data_, msc_result)) {
    REXLOG_ERROR("MetalShader: DXIL to Metal conversion failed: {}",
                 msc_result.error_message);
    return false;
  }
  function_name_ = msc_result.function_name;
  metallib_data_ = std::move(msc_result.metallib_data);
  REXLOG_DEBUG("MetalShader: Converted {} bytes DXIL to {} bytes MetalLib",
               dxil_data_.size(), metallib_data_.size());

  static std::atomic<int> dump_count{0};
  int dc = dump_count.fetch_add(1);
  fprintf(stderr, "[metal] SHADER DUMP #%d: metallib=%zu bytes fn=%s\n",
          dc, metallib_data_.size(), function_name_.c_str());
  fflush(stderr);
  if (dc < 10) {
    char dump_path[256];
    snprintf(dump_path, sizeof(dump_path), "/tmp/pgr3_shader_%d.metallib", dc);
    FILE* f = fopen(dump_path, "wb");
    if (f) {
      fwrite(metallib_data_.data(), 1, metallib_data_.size(), f);
      fclose(f);
    }
    snprintf(dump_path, sizeof(dump_path), "/tmp/pgr3_shader_%d.dxil", dc);
    f = fopen(dump_path, "wb");
    if (f) {
      fwrite(dxil_data_.data(), 1, dxil_data_.size(), f);
      fclose(f);
    }
    auto& dxbc_bin = translated_binary();
    snprintf(dump_path, sizeof(dump_path), "/tmp/pgr3_shader_%d.dxbc", dc);
    f = fopen(dump_path, "wb");
    if (f) {
      fwrite(dxbc_bin.data(), 1, dxbc_bin.size(), f);
      fclose(f);
    }
    fprintf(stderr, "[metal] Dumped shader #%d: dxbc=%zu dxil=%zu metallib=%zu fn=%s\n",
            dc, dxbc_bin.size(), dxil_data_.size(), metallib_data_.size(),
            function_name_.c_str());
    fflush(stderr);
  }

  NS::Error* error = nullptr;
  dispatch_data_t data =
      dispatch_data_create(metallib_data_.data(), metallib_data_.size(),
                           nullptr, DISPATCH_DATA_DESTRUCTOR_NONE);
  metal_library_ = device->newLibrary(data, &error);
  dispatch_release(data);

  if (!metal_library_) {
    if (error) {
      REXLOG_ERROR("MetalShader: Failed to create Metal library: {}",
                   error->localizedDescription()->utf8String());
      error->release();
    } else {
      REXLOG_ERROR("MetalShader: Failed to create Metal library (unknown error)");
    }
    return false;
  }

  NS::String* fn = NS::String::string(function_name_.c_str(),
                                       NS::UTF8StringEncoding);
  metal_function_ = metal_library_->newFunction(fn);

  if (!metal_function_) {
    const char* alt_names[] = {"main0", "main", "vertexMain", "fragmentMain"};
    for (const char* alt_name : alt_names) {
      NS::String* alt_fn =
          NS::String::string(alt_name, NS::UTF8StringEncoding);
      metal_function_ = metal_library_->newFunction(alt_fn);
      if (metal_function_) {
        REXLOG_DEBUG("MetalShader: Found function with alternative name: {}",
                     alt_name);
        break;
      }
    }
  }

  if (!metal_function_) {
    NS::Array* function_names = metal_library_->functionNames();
    REXLOG_ERROR("MetalShader: Could not find shader function. Available:");
    for (NS::UInteger i = 0; i < function_names->count(); i++) {
      NS::String* name = static_cast<NS::String*>(function_names->object(i));
      REXLOG_ERROR("  - {}", name->utf8String());
    }
    return false;
  }

  if (g_metal_shader_cache && g_metal_shader_cache->IsInitialized()) {
    g_metal_shader_cache->Store(shader_cache_key, function_name_,
                                metallib_data_.data(), metallib_data_.size());
  }

  return true;
}

Shader::Translation* MetalShader::CreateTranslationInstance(
    uint64_t modification) {
  return new MetalTranslation(*this, modification);
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
