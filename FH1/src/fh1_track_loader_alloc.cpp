// FH1 track-loader heap alloc (sub_8240AC00) with native null retry.
//
// The guest allocator returns pointers in the 0x4010xxxx pool (e.g. 0x401011C0).
// Work-queue 0x40101238 sits at object+0x78 — that is a field inside the same
// object, not a separate poison pointer.

#include "fh1_track_loader_alloc.h"

#include "generated/fh1_init.h"

#include <atomic>
#include <cstring>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>

namespace {

constexpr uint32_t kAllocatorSingleton =
    static_cast<uint32_t>(static_cast<int32_t>(-2094137344) - 4032);

bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size) {
  if (size == 0 || guest_address == 0) {
    return false;
  }
  if (!memory->IsGuestVirtualCommitted(guest_address)) {
    return false;
  }
  const uint32_t end = guest_address + size - 1;
  if (end < guest_address) {
    return false;
  }
  return memory->IsGuestVirtualCommitted(end);
}

uint32_t CallGuestAlloc(PPCContext& ctx, uint8_t* base, uint32_t size) {
  ctx.r3.u64 = size;
  sub_8240AC00(ctx, base);
  return ctx.r3.u32;
}

void LogAllocResult(rex::memory::Memory* memory, uint32_t object, uint32_t size,
                    const char* phase) {
  static std::atomic<uint32_t> log_count{0};
  if (log_count.fetch_add(1, std::memory_order_relaxed) >= 8) {
    return;
  }

  uint32_t vtable = 0;
  uint32_t alloc_fn = 0;
  if (GuestRangeReadable(memory, kAllocatorSingleton, 4)) {
    std::memcpy(&vtable, memory->TranslateVirtual(kAllocatorSingleton),
                sizeof(vtable));
    vtable = __builtin_bswap32(vtable);
    if (vtable != 0 && GuestRangeReadable(memory, vtable + 8, 4)) {
      std::memcpy(&alloc_fn, memory->TranslateVirtual(vtable + 8),
                  sizeof(alloc_fn));
      alloc_fn = __builtin_bswap32(alloc_fn);
    }
  }

  REXSYS_WARN(
      "FH1 sub_8240AC00({}): {} ptr=0x{:08X} readable={} allocator=0x{:08X} "
      "vtable=0x{:08X} alloc_fn=0x{:08X}",
      size, phase, object, GuestRangeReadable(memory, object, size),
      kAllocatorSingleton, vtable, alloc_fn);
}

}  // namespace

uint32_t Fh1AllocateTrackLoaderObject(PPCContext& ctx, uint8_t* base) {
  constexpr uint32_t kSize = 712;
  auto* memory = rex::Runtime::instance()->memory();
  if (!memory) {
    return 0;
  }

  uint32_t object = CallGuestAlloc(ctx, base, kSize);
  if (object != 0 && GuestRangeReadable(memory, object, kSize)) {
    LogAllocResult(memory, object, kSize, "ok");
    return object;
  }

  LogAllocResult(memory, object, kSize, "null/unreadable, heap grow");

  // Native retry when virtual alloc returns null.
  ctx.lr = 0x8255AE28;
  sub_82CBE598(ctx, base);

  object = CallGuestAlloc(ctx, base, kSize);
  if (object != 0 && GuestRangeReadable(memory, object, kSize)) {
    LogAllocResult(memory, object, kSize, "ok after grow");
    return object;
  }

  LogAllocResult(memory, object, kSize, "failed after grow");
  return 0;
}
