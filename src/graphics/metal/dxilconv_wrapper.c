#include <stddef.h>
#include <stdint.h>

typedef int32_t HRESULT;
typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;
typedef const GUID* REFCLSID;
typedef const GUID* REFIID;

extern HRESULT DxcCreateInstance(REFCLSID rclsid, REFIID riid, void** ppv);

static const GUID CLSID_DxbcConverter = {
    0x4900391e, 0xb752, 0x4edd,
    {0xa8, 0x85, 0x6f, 0xb7, 0x6e, 0x25, 0xad, 0xdb}};

static const GUID IID_IDxbcConverter = {
    0x5f956ed5, 0x78d1, 0x4b15,
    {0x82, 0x47, 0xf7, 0x18, 0x76, 0x14, 0xa0, 0x41}};

HRESULT DxilConvCreateInstance(void** ppv) {
    return DxcCreateInstance(&CLSID_DxbcConverter, &IID_IDxbcConverter, ppv);
}
