#include <rex/graphics/metal/dxbc_to_dxil_converter.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include <DxbcConverter.h>

#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

namespace {
constexpr wchar_t kDefaultExtraOptions[] = L"-skip-container-parts";

const CLSID kClsidDxbcConverter = {
    0x4900391e, 0xb752, 0x4edd,
    {0xa8, 0x85, 0x6f, 0xb7, 0x6e, 0x25, 0xad, 0xdb}};

size_t ElfHash(const char* k) {
  unsigned long h = 0;
  while (*k) {
    h = (h << 4) + static_cast<unsigned char>(*k++);
    unsigned long g = h & 0xF0000000UL;
    if (g) h ^= g >> 24;
    h &= ~g;
  }
  return static_cast<size_t>(h);
}

std::string HResultHex(HRESULT hr) {
  char buffer[11];
  std::snprintf(buffer, sizeof(buffer), "%08X", static_cast<unsigned>(hr));
  return std::string(buffer);
}

struct ThreadConverter {
  IDxbcConverter* converter = nullptr;
  ~ThreadConverter() {
    if (converter) converter->Release();
  }
};
}  // namespace

DxbcToDxilConverter::DxbcToDxilConverter() = default;
DxbcToDxilConverter::~DxbcToDxilConverter() = default;

bool DxbcToDxilConverter::Initialize() {
  const char* extra_options = std::getenv("REX_DXBC2DXIL_FLAGS");
  if (extra_options) {
    extra_options_ = std::wstring(extra_options, extra_options + strlen(extra_options));
  } else {
    extra_options_ = kDefaultExtraOptions;
  }

  IDxbcConverter* test_converter = nullptr;
  size_t iid_val = ElfHash("IUnknown");
  REFIID iid = reinterpret_cast<REFIID>(iid_val);
  fprintf(stderr, "[metal] DxbcToDxilConverter: calling DxcCreateInstance iid=0x%016zx\n", iid_val); fflush(stderr);
  HRESULT hr = DxcCreateInstance(kClsidDxbcConverter, iid,
                                 reinterpret_cast<void**>(&test_converter));
  fprintf(stderr, "[metal] DxbcToDxilConverter: DxcCreateInstance returned hr=0x%08X conv=%p\n", static_cast<unsigned>(hr), test_converter); fflush(stderr);
  if (hr != S_OK || !test_converter) {
    fprintf(stderr, "[metal] DxbcToDxilConverter: DxcCreateInstance failed hr=0x%08X\n", static_cast<unsigned>(hr)); fflush(stderr);
    is_available_ = false;
    return false;
  }
  test_converter->Release();

  is_available_ = true;
  fprintf(stderr, "[metal] DxbcToDxilConverter: initialized OK\n"); fflush(stderr);
  return true;
}

bool DxbcToDxilConverter::Convert(const std::vector<uint8_t>& dxbc_data,
                                  std::vector<uint8_t>& dxil_data_out,
                                  std::string* error_message) {
  if (!is_available_) {
    if (error_message)
      *error_message = "DxbcToDxilConverter not initialized";
    return false;
  }

  if (dxbc_data.size() < 4 || dxbc_data[0] != 'D' || dxbc_data[1] != 'X' ||
      dxbc_data[2] != 'B' || dxbc_data[3] != 'C') {
    if (error_message)
      *error_message = "Invalid DXBC data - missing DXBC magic header";
    return false;
  }

  IDxbcConverter* converter = GetThreadConverter(error_message);
  if (!converter) return false;

  void* dxil_ptr = nullptr;
  UINT32 dxil_size = 0;
  wchar_t* diag = nullptr;

  HRESULT hr = converter->Convert(
      dxbc_data.data(), static_cast<UINT32>(dxbc_data.size()),
      extra_options_.empty() ? nullptr : extra_options_.c_str(),
      &dxil_ptr, &dxil_size, &diag);

  if (hr != S_OK || dxil_ptr == nullptr || dxil_size == 0) {
    if (error_message) {
      if (diag) {
        std::string diag_utf8;
        for (const wchar_t* p = diag; *p; ++p)
          diag_utf8.push_back(static_cast<char>(*p));
        *error_message = "dxbc2dxil failed: " + diag_utf8;
      } else {
        *error_message = "dxbc2dxil failed with HRESULT 0x" + HResultHex(hr);
      }
    }
    CoTaskMemFree(diag);
    CoTaskMemFree(dxil_ptr);
    return false;
  }

  dxil_data_out.assign(reinterpret_cast<const uint8_t*>(dxil_ptr),
                       reinterpret_cast<const uint8_t*>(dxil_ptr) + dxil_size);

  CoTaskMemFree(diag);
  CoTaskMemFree(dxil_ptr);

  REXLOG_DEBUG("DxbcToDxilConverter: Converted {} bytes DXBC to {} bytes DXIL",
               dxbc_data.size(), dxil_data_out.size());
  return true;
}

IDxbcConverter* DxbcToDxilConverter::GetThreadConverter(
    std::string* error_message) {
  static thread_local ThreadConverter thread_state;
  if (thread_state.converter) return thread_state.converter;

  HRESULT hr =
      DxcCreateInstance(kClsidDxbcConverter,
                        reinterpret_cast<REFIID>(ElfHash("IUnknown")),
                        reinterpret_cast<void**>(&thread_state.converter));
  if (hr != S_OK || !thread_state.converter) {
    if (error_message) {
      *error_message = "Failed to create IDxbcConverter (HRESULT 0x" +
                       HResultHex(hr) + ")";
    }
    return nullptr;
  }
  return thread_state.converter;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
