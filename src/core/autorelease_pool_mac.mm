#include <rex/autorelease_pool_mac.h>

#if REX_PLATFORM_MAC
#import <Foundation/Foundation.h>

namespace rex {

ScopedAutoreleasePool::ScopedAutoreleasePool(const char*) {
  pool_ = [[NSAutoreleasePool alloc] init];
}

ScopedAutoreleasePool::~ScopedAutoreleasePool() {
  if (pool_) {
    [(NSAutoreleasePool*)pool_ drain];
    pool_ = nullptr;
  }
}

}  // namespace rex
#endif
