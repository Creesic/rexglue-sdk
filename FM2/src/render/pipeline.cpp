// render/pipeline.cpp
// shader loading

#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>

#include <plume_render_interface.h>
#include <zstd.h>

#include <rex/logging.h>

#include "generated/shader_cache.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"

#ifdef _WIN32
#include <windows.h>
#include <objidl.h>
#include <unknwn.h>
#include <dxcapi.h>
#endif

using namespace plume;

namespace fm2::render {

namespace {

std::unique_ptr<uint8_t[]> g_dxilCache;
std::once_flag g_dxilCacheOnce;

std::unique_ptr<uint8_t[]> g_spirvCache;
std::once_flag g_spirvCacheOnce;

#ifdef _WIN32
// SEH-guarded DXC linker call: some translated shaders (FM2 menu, spec_mask!=0)
// crash dxcompiler.dll's IDxcLinker::Link with a null deref (0xCC). Catch it so the
// single bad shader is skipped (draw falls back to no host VS) instead of taking
// down the whole process. Must be a plain function with no C++ unwinding objects.
static HRESULT SafeDxcLink(IDxcLinker* linker, const wchar_t* profile, const wchar_t* const* libs,
                           uint32_t libCount, IDxcOperationResult** out) {
  __try {
    return linker->Link(L"shaderMain", profile, libs, libCount, nullptr, 0, out);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *out = nullptr;
    return E_UNEXPECTED;
  }
}

// Some regenerated spec-mask shaders produce a DXC linker-output blob whose
// GetBufferPointer()/GetBufferSize() vtable calls null-deref inside dxcompiler.dll
// (fault 0xCC). Read the bytes + build the plume shader behind an SEH guard so the
// one bad shader is skipped. The unwinding unique_ptr temporary lives in the *Raw
// callee, keeping the __try frame free of objects requiring unwinding.
static RenderShader* CreateShaderFromDxcBlobRaw(IDxcBlob* blob) {
  return Device()
      ->createShader(blob->GetBufferPointer(), blob->GetBufferSize(), "main", RenderShaderFormat::DXIL)
      .release();
}
static RenderShader* SafeCreateShaderFromDxcBlob(IDxcBlob* blob) {
  RenderShader* out = nullptr;
  __try {
    out = CreateShaderFromDxcBlobRaw(blob);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out = nullptr;
  }
  return out;
}

class DxcRuntime {
 public:
  DxcRuntime() {
    library_ = LoadLibraryW(L"dxcompiler.dll");
    if (library_ == nullptr) {
      REXLOG_ERROR("DXC: failed to load dxcompiler.dll");
      return;
    }

    createInstance_ =
        reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(library_, "DxcCreateInstance"));
    if (createInstance_ == nullptr) {
      REXLOG_ERROR("DXC: failed to get DxcCreateInstance");
      return;
    }

    if (FAILED(createInstance_(CLSID_DxcCompiler, __uuidof(IDxcCompiler3),
                               reinterpret_cast<void**>(&compiler_))) ||
        compiler_ == nullptr) {
      REXLOG_ERROR("DXC: failed to create IDxcCompiler3");
      return;
    }
    if (FAILED(createInstance_(CLSID_DxcUtils, __uuidof(IDxcUtils),
                               reinterpret_cast<void**>(&utils_))) ||
        utils_ == nullptr) {
      REXLOG_ERROR("DXC: failed to create IDxcUtils");
      return;
    }
  }

  ~DxcRuntime() {
    for (auto& [_, blob] : specConstantLibraries_) {
      if (blob != nullptr) {
        blob->Release();
      }
    }
    if (utils_ != nullptr) {
      utils_->Release();
    }
    if (compiler_ != nullptr) {
      compiler_->Release();
    }
    if (library_ != nullptr) {
      FreeLibrary(library_);
    }
  }

  DxcRuntime(const DxcRuntime&) = delete;
  DxcRuntime& operator=(const DxcRuntime&) = delete;

  bool ready() const { return compiler_ != nullptr && utils_ != nullptr; }

  IDxcBlob* GetSpecConstantLibrary(uint32_t specConstants) {
    std::lock_guard lock(mutex_);

    auto cached = specConstantLibraries_.find(specConstants);
    if (cached != specConstantLibraries_.end()) {
      cached->second->AddRef();
      return cached->second;
    }

    char source[128];
    int sourceSize = std::snprintf(source, sizeof(source),
                                   "export uint g_SpecConstants() { return %u; }", specConstants);

    DxcBuffer sourceBuffer{};
    sourceBuffer.Ptr = source;
    sourceBuffer.Size = static_cast<size_t>(sourceSize);
    sourceBuffer.Encoding = DXC_CP_ACP;

    LPCWSTR args[] = {L"-T lib_6_3"};

    IDxcResult* result = nullptr;
    HRESULT hr = compiler_->Compile(&sourceBuffer, args, std::size(args), nullptr,
                                    __uuidof(IDxcResult), reinterpret_cast<void**>(&result));
    if (FAILED(hr) || result == nullptr) {
      REXLOG_ERROR("DXC: spec constant library compile failed hr=0x{:08X}", static_cast<unsigned>(hr));
      return nullptr;
    }

    IDxcBlob* object = GetResultObject(result, "DXC: spec constant library");
    result->Release();
    if (object != nullptr) {
      object->AddRef();
      specConstantLibraries_.emplace(specConstants, object);
    }
    return object;
  }

  IDxcBlobEncoding* CreatePinnedLibraryBlob(const void* dxilData, uint32_t dxilSize) {
    if (!ready()) {
      REXLOG_ERROR("PIPELINE-TRACE: CreatePinnedLibraryBlob: DxcRuntime not ready (compiler={} utils={})",
                   compiler_ != nullptr, utils_ != nullptr);
      return nullptr;
    }

    IDxcBlobEncoding* shaderBlob = nullptr;
    HRESULT hr = utils_->CreateBlobFromPinned(dxilData, dxilSize, DXC_CP_ACP, &shaderBlob);
    if (FAILED(hr) || shaderBlob == nullptr) {
      REXLOG_ERROR("DXC: failed to create shader library blob hr=0x{:08X}", static_cast<unsigned>(hr));
      return nullptr;
    }
    return shaderBlob;
  }

  // Plain function (not called from inside the __try frame) so it's safe to
  // call from the __except handler despite using C++ objects internally.
  static void LogDxcLinkCrashOnce(uint32_t dxilOffset, uint32_t specConstants) {
    static bool logged = false;
    if (!logged) {
      logged = true;
      REXLOG_ERROR(
          "DXC: caught a structured exception linking shader dxilOffset={} specConstants={} -- "
          "skipping this shader (known dxcompiler.dll crash on some regenerated permutations)",
          dxilOffset, specConstants);
    }
  }

  // Public entry: SEH-guard the ENTIRE DXC link path. Some regenerated FM2 menu
  // shaders crash dxcompiler.dll with a null deref (fault 0xCC) -- not always in
  // Link() itself but in RegisterLibrary/Compile on a malformed blob. Catch it
  // here (whole path) and skip the one bad shader instead of killing the process.
  // No C++ unwinding objects live in this frame, so __try is legal here.
  IDxcBlob* LinkShaderLibrary(IDxcBlob* shaderBlob, uint32_t dxilOffset, ResourceType shaderType,
                              uint32_t specConstants) {
    IDxcBlob* result = nullptr;
    __try {
      result = LinkShaderLibraryInner(shaderBlob, dxilOffset, shaderType, specConstants);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      result = nullptr;
      LogDxcLinkCrashOnce(dxilOffset, specConstants);
    }
    return result;
  }

  IDxcBlob* LinkShaderLibraryInner(IDxcBlob* shaderBlob, uint32_t dxilOffset, ResourceType shaderType,
                                   uint32_t specConstants) {
    REXLOG_ERROR("PIPELINE-TRACE: LinkShaderLibraryInner entered");
    if (!ready()) {
      return nullptr;
    }

    IDxcBlob* specBlob = GetSpecConstantLibrary(specConstants);
    REXLOG_ERROR("PIPELINE-TRACE: GetSpecConstantLibrary returned {}", specBlob != nullptr ? "non-null" : "NULL");
    if (specBlob == nullptr) {
      return nullptr;
    }

    IDxcLinker* linker = nullptr;
    HRESULT hr =
        createInstance_(CLSID_DxcLinker, __uuidof(IDxcLinker), reinterpret_cast<void**>(&linker));
    if (FAILED(hr) || linker == nullptr) {
      REXLOG_ERROR("DXC: failed to create IDxcLinker hr=0x{:08X}", static_cast<unsigned>(hr));
      specBlob->Release();
      return nullptr;
    }

    wchar_t specConstantsLibName[64];
    swprintf_s(specConstantsLibName, L"SpecConstants_%u", specConstants);
    wchar_t shaderLibName[64];
    swprintf_s(shaderLibName, L"Shader_%u", dxilOffset);

    bool registered = SUCCEEDED(linker->RegisterLibrary(specConstantsLibName, specBlob)) &&
                      SUCCEEDED(linker->RegisterLibrary(shaderLibName, shaderBlob));
    specBlob->Release();
    if (!registered) {
      REXLOG_ERROR("DXC: failed to register shader libraries");
      linker->Release();
      return nullptr;
    }

    const wchar_t* libraries[] = {specConstantsLibName, shaderLibName};
    const wchar_t* profile = shaderType == ResourceType::VertexShader ? L"vs_6_0" : L"ps_6_0";
    IDxcOperationResult* linkResult = nullptr;
    hr = SafeDxcLink(linker, profile, libraries, std::size(libraries), &linkResult);
    linker->Release();
    if (FAILED(hr) || linkResult == nullptr) {
      REXLOG_ERROR("DXC: shader link failed hr=0x{:08X}", static_cast<unsigned>(hr));
      return nullptr;
    }

    IDxcBlob* object = GetOperationResultObject(linkResult, "DXC: shader link");
    linkResult->Release();
    return object;
  }

 private:
  static IDxcBlob* GetResultObject(IDxcResult* result, const char* label) {
    HRESULT status = E_FAIL;
    HRESULT hr = result->GetStatus(&status);
    if (FAILED(hr) || FAILED(status)) {
      LogResultErrors(result, label);
      return nullptr;
    }

    IDxcBlob* object = nullptr;
    hr = result->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), reinterpret_cast<void**>(&object),
                           nullptr);
    if (FAILED(hr) || object == nullptr) {
      REXLOG_ERROR("{}: failed to get object hr=0x{:08X}", label, static_cast<unsigned>(hr));
      return nullptr;
    }
    return object;
  }

  static IDxcBlob* GetOperationResultObject(IDxcOperationResult* result, const char* label) {
    HRESULT status = E_FAIL;
    HRESULT hr = result->GetStatus(&status);
    if (FAILED(hr) || FAILED(status)) {
      IDxcBlobEncoding* errors = nullptr;
      if (SUCCEEDED(result->GetErrorBuffer(&errors)) && errors != nullptr) {
        REXLOG_ERROR("{}: {}", label,
                     std::string_view(static_cast<const char*>(errors->GetBufferPointer()),
                                      errors->GetBufferSize()));
        errors->Release();
      } else {
        REXLOG_ERROR("{}: failed status hr=0x{:08X}", label, static_cast<unsigned>(status));
      }
      return nullptr;
    }

    IDxcBlob* object = nullptr;
    hr = result->GetResult(&object);
    if (FAILED(hr) || object == nullptr) {
      REXLOG_ERROR("{}: failed to get linked object hr=0x{:08X}", label, static_cast<unsigned>(hr));
      return nullptr;
    }
    return object;
  }

  static void LogResultErrors(IDxcResult* result, const char* label) {
    IDxcBlobUtf8* errors = nullptr;
    if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, __uuidof(IDxcBlobUtf8),
                                    reinterpret_cast<void**>(&errors), nullptr)) &&
        errors != nullptr) {
      REXLOG_ERROR("{}: {}", label, errors->GetStringPointer());
      errors->Release();
    } else {
      REXLOG_ERROR("{}: failed", label);
    }
  }

  HMODULE library_ = nullptr;
  DxcCreateInstanceProc createInstance_ = nullptr;
  IDxcCompiler3* compiler_ = nullptr;
  IDxcUtils* utils_ = nullptr;
  std::mutex mutex_;
  std::unordered_map<uint32_t, IDxcBlob*> specConstantLibraries_;
};

DxcRuntime& GetDxcRuntime() {
  static DxcRuntime runtime;
  return runtime;
}
#endif

void EnsureDxilCache() {
  std::call_once(g_dxilCacheOnce, [] {
    g_dxilCache = std::make_unique<uint8_t[]>(g_dxilCacheDecompressedSize);
    size_t result = ZSTD_decompress(g_dxilCache.get(), g_dxilCacheDecompressedSize, g_compressedDxilCache,
                                    g_dxilCacheCompressedSize);
    if (ZSTD_isError(result)) {
      REXLOG_ERROR("EnsureDxilCache: ZSTD_decompress failed: {}", ZSTD_getErrorName(result));
    } else if (result != g_dxilCacheDecompressedSize) {
      REXLOG_ERROR("EnsureDxilCache: decompressed {} bytes, expected {}", result, g_dxilCacheDecompressedSize);
    }
  });
}

void EnsureSpirvCache() {
  std::call_once(g_spirvCacheOnce, [] {
    g_spirvCache = std::make_unique<uint8_t[]>(g_spirvCacheDecompressedSize);
    ZSTD_decompress(g_spirvCache.get(), g_spirvCacheDecompressedSize, g_compressedSpirvCache,
                    g_spirvCacheCompressedSize);
  });
}

}  // namespace

#ifdef _WIN32
GuestShader::~GuestShader() {
  if (dxilLibraryBlob != nullptr) {
    dxilLibraryBlob->Release();
  }
}
#else
GuestShader::~GuestShader() = default;
#endif

RenderShader* LoadShader(GuestShader* guestShader, uint32_t specConstants) {
  if (guestShader == nullptr) {
    return nullptr;
  }

  if (guestShader->shaderCacheEntry == nullptr) {
    return nullptr;
  }

  const ShaderCacheEntry* entry = guestShader->shaderCacheEntry;
  RenderShaderFormat fmt = Interface()->getCapabilities().shaderFormat;
  if (fmt == RenderShaderFormat::SPIRV) {
    if (guestShader->shader != nullptr) {
      return guestShader->shader.get();
    }
    EnsureSpirvCache();
    guestShader->shader = Device()->createShader(g_spirvCache.get() + entry->spirv_offset,
                                                  entry->spirv_size, "main", RenderShaderFormat::SPIRV);
    return guestShader->shader.get();
  }

#ifdef _WIN32
  EnsureDxilCache();
  if (entry->spec_constants_mask == 0) {
    if (guestShader->shader != nullptr) {
      return guestShader->shader.get();
    }
    if (entry->dxil_offset + entry->dxil_size > g_dxilCacheDecompressedSize) {
      static bool loggedOob = false;
      if (!loggedOob) {
        loggedOob = true;
        REXLOG_ERROR(
            "LoadShader: entry offset+size out of bounds (offset={} size={} cacheSize={})",
            entry->dxil_offset, entry->dxil_size, g_dxilCacheDecompressedSize);
      }
      return nullptr;
    }
    guestShader->shader = Device()->createShader(g_dxilCache.get() + entry->dxil_offset,
                                                 entry->dxil_size, "main", RenderShaderFormat::DXIL);
    if (guestShader->shader == nullptr) {
      static bool loggedFail = false;
      if (!loggedFail) {
        loggedFail = true;
        REXLOG_ERROR("LoadShader: Device()->createShader failed (offset={} size={} hash=0x{:016X})",
                     entry->dxil_offset, entry->dxil_size, entry->hash);
      }
    }
    return guestShader->shader.get();
  }

  uint32_t specializedValue = specConstants & entry->spec_constants_mask;
  {
    std::lock_guard lock(guestShader->mutex);
    auto cached = guestShader->specializedShaders.find(specializedValue);
    if (cached != guestShader->specializedShaders.end()) {
      return cached->second.get();
    }
    if (guestShader->dxilLibraryBlob == nullptr) {
      REXLOG_ERROR("PIPELINE-TRACE: about to call CreatePinnedLibraryBlob (offset={} size={})",
                   entry->dxil_offset, entry->dxil_size);
      guestShader->dxilLibraryBlob = GetDxcRuntime().CreatePinnedLibraryBlob(
          g_dxilCache.get() + entry->dxil_offset, entry->dxil_size);
      REXLOG_ERROR("PIPELINE-TRACE: CreatePinnedLibraryBlob returned {}",
                   guestShader->dxilLibraryBlob != nullptr ? "non-null" : "NULL");
    }
  }

  IDxcBlobEncoding* libraryBlob = nullptr;
  {
    std::lock_guard lock(guestShader->mutex);
    libraryBlob = guestShader->dxilLibraryBlob;
    if (libraryBlob != nullptr) {
      libraryBlob->AddRef();
    }
  }
  if (libraryBlob == nullptr) {
    REXLOG_ERROR("PIPELINE-TRACE: libraryBlob is null, returning from LoadShader (path #1)");
    return nullptr;
  }

  REXLOG_ERROR("PIPELINE-TRACE: calling LinkShaderLibrary");
  IDxcBlob* linkedBlob = GetDxcRuntime().LinkShaderLibrary(libraryBlob, entry->dxil_offset,
                                                           guestShader->type, specializedValue);
  REXLOG_ERROR("PIPELINE-TRACE: LinkShaderLibrary returned {}", linkedBlob != nullptr ? "non-null" : "NULL");
  libraryBlob->Release();
  if (linkedBlob == nullptr) {
    return nullptr;
  }

  // SEH-guarded: a malformed linker-output blob (some regenerated spec-mask
  // shaders) crashes dxcompiler.dll when its bytes are read. On crash, skip this
  // shader (the draw falls back to no host shader) instead of killing the process.
  std::unique_ptr<RenderShader> shader(SafeCreateShaderFromDxcBlob(linkedBlob));
  linkedBlob->Release();
  if (shader == nullptr) {
    REXLOG_ERROR("PIPELINE-TRACE: SafeCreateShaderFromDxcBlob returned NULL (path #3)");
    return nullptr;
  }

  RenderShader* shaderPtr = shader.get();
  {
    std::lock_guard lock(guestShader->mutex);
    auto [it, inserted] = guestShader->specializedShaders.emplace(specializedValue, std::move(shader));
    shaderPtr = it->second.get();
  }
  return shaderPtr;
#else
  return nullptr;
#endif
}

}  // namespace fm2::render
