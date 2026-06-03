#include "dxcapi.h"

#ifdef __EMULATE_UUID
size_t UuidStrHash(const char* k) {
    size_t hash = 0;
    while (*k) {
        hash = hash * 131 + static_cast<size_t>(*k);
        ++k;
    }
    return hash;
}
#endif

DEFINE_CROSS_PLATFORM_UUIDOF(IUnknown)
