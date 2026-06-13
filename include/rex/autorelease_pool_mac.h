#pragma once

#include <rex/platform.h>

#if REX_PLATFORM_MAC
namespace rex {

class ScopedAutoreleasePool {
 public:
  explicit ScopedAutoreleasePool(const char* name = nullptr);
  ~ScopedAutoreleasePool();

  ScopedAutoreleasePool(const ScopedAutoreleasePool&) = delete;
  ScopedAutoreleasePool& operator=(const ScopedAutoreleasePool&) = delete;

 private:
  void* pool_ = nullptr;
};

}  // namespace rex

#define REX_SCOPED_AUTORELEASE_POOL(name) \
  ::rex::ScopedAutoreleasePool rex_autorelease_pool_##__LINE__(name)
#define XE_SCOPED_AUTORELEASE_POOL(name) REX_SCOPED_AUTORELEASE_POOL(name)
#else
#define REX_SCOPED_AUTORELEASE_POOL(name) ((void)0)
#define XE_SCOPED_AUTORELEASE_POOL(name) ((void)0)
#endif
