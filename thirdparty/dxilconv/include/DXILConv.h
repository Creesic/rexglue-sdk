#pragma once
// Stub header for DXIL Converter
// The actual dxilconv library needs to be installed separately
#include <cstdint>
#include <string>

class IDxbcConverter {
public:
  virtual ~IDxbcConverter() = default;
  virtual int QueryInterface(const void* iid, void** ppv) = 0;
  virtual int Convert(const void* dxbc, uint32_t dxbc_size, void** dxil, uint32_t* dxil_size) = 0;
};

extern "C" {
  IDxbcConverter* CreateDxbcConverter();
  void DestroyDxbcConverter(IDxbcConverter* conv);
}
