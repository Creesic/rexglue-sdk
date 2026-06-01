// FH1 TReference / smart-pointer helpers.
//
// sub_82DE7330 — release: load *slot, call vtable+12, clear slot.
// sub_824E81A8 — assign: addref new (vtable+8), store, release old (vtable+12).
//
// During GameManager init / work-queue fiber jobs held objects sometimes resolve
// to unreadable memory or unregistered vtable targets → AV / InvalidFunctionTrap.

#include "generated/fh1_init.h"

#include <atomic>
#include <cstring>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>

namespace {

bool IsKnownPoisonGuestPointer(uint32_t addr) {
  switch (addr) {
    case 0:
    case 0x00BEBEBEu:
    case 0xBEBEBEBEu:
    case 0xCDCDCDCDu:
    case 0xDDDDDDDDu:
    case 0xFEEEFEEEu:
      return true;
    default:
      return false;
  }
}

// Reject null+offset slots (e.g. r28=0 → r28+52=0x34) and other non-pointer EAs.
// IsGuestVirtualCommitted alone is not enough for low absolute addresses.
bool IsPlausibleGuestDataPointer(uint32_t addr) {
  if (IsKnownPoisonGuestPointer(addr)) {
    return false;
  }
  if (addr < 0x10000u) {
    return false;
  }
  if (addr >= REX_IMAGE_BASE &&
      (addr - REX_IMAGE_BASE) < REX_IMAGE_SIZE) {
    return true;
  }
  if (addr >= 0x30000000u && addr < 0x80000000u) {
    return true;
  }
  return false;
}

bool GuestRangeReadable(rex::memory::Memory* memory, uint32_t guest_address,
                        uint32_t size) {
  if (size == 0 || !IsPlausibleGuestDataPointer(guest_address)) {
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

uint32_t LoadGuestU32(rex::memory::Memory* memory, uint32_t guest_address) {
  if (!GuestRangeReadable(memory, guest_address, 4)) {
    return 0;
  }
  uint32_t be = 0;
  std::memcpy(&be, memory->TranslateVirtual(guest_address), sizeof(be));
  return __builtin_bswap32(be);
}

void StoreGuestU32(rex::memory::Memory* memory, uint32_t guest_address,
                   uint32_t value) {
  if (!GuestRangeReadable(memory, guest_address, 4)) {
    return;
  }
  const uint32_t be = __builtin_bswap32(value);
  std::memcpy(memory->TranslateVirtual(guest_address), &be, sizeof(be));
}

bool TryCallVtableMember(rex::memory::Memory* memory, PPCContext& ctx,
                         unsigned char* base, uint32_t object,
                         uint32_t vtable_offset, uint32_t return_lr,
                         const char* tag) {
  if (!GuestRangeReadable(memory, object, 4)) {
    static std::atomic<uint32_t> bad_obj_log{0};
    if (bad_obj_log.fetch_add(1, std::memory_order_relaxed) < 12) {
      REXSYS_WARN("FH1 {}: unreadable object 0x{:08X}", tag, object);
    }
    return false;
  }

  const uint32_t vtable = LoadGuestU32(memory, object);
  const uint32_t fn =
      vtable != 0 && GuestRangeReadable(memory, vtable + vtable_offset, 4)
          ? LoadGuestU32(memory, vtable + vtable_offset)
          : 0u;

  if (fn != 0 && rex::runtime::ResolveIndirectFunction(fn) != nullptr) {
    ctx.r3.u64 = object;
    ctx.lr = return_lr;
    REX_CALL_INDIRECT_FUNC(fn);
    return true;
  }

  static std::atomic<uint32_t> skip_log{0};
  if (skip_log.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXSYS_WARN(
        "FH1 {}: skip fn=0x{:08X} vtable=0x{:08X} object=0x{:08X} off={}",
        tag, fn, vtable, object, vtable_offset);
  }
  return false;
}

}  // namespace

extern "C" REX_FUNC(sub_82DE7330) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  if (!memory) {
    return;
  }

  const uint32_t slot = ctx.r3.u32;
  if (!GuestRangeReadable(memory, slot, 4)) {
    return;
  }

  const uint32_t object = LoadGuestU32(memory, slot);
  if (object != 0) {
    TryCallVtableMember(memory, ctx, base, object, 12, 0x82DE7360u,
                        "sub_82DE7330 release");
  }

  StoreGuestU32(memory, slot, 0);
}

extern "C" REX_FUNC(sub_824E81A8) {
  REX_FUNC_PROLOGUE();

  auto* memory = rex::Runtime::instance()->memory();
  const uint32_t slot = ctx.r3.u32;
  const uint32_t new_object = ctx.r4.u32;

  if (!memory) {
    ctx.r3.u64 = slot;
    return;
  }

  if (new_object != 0) {
    TryCallVtableMember(memory, ctx, base, new_object, 8, 0x824E81E0u,
                        "sub_824E81A8 addref");
  }

  if (!GuestRangeReadable(memory, slot, 4)) {
    static std::atomic<uint32_t> bad_slot_log{0};
    if (bad_slot_log.fetch_add(1, std::memory_order_relaxed) < 12) {
      REXSYS_WARN(
          "FH1 sub_824E81A8: unreadable slot 0x{:08X} new=0x{:08X}; skipping",
          slot, new_object);
    }
    ctx.r3.u64 = slot;
    return;
  }

  const uint32_t old_object = LoadGuestU32(memory, slot);
  StoreGuestU32(memory, slot, new_object);

  if (old_object != 0) {
    TryCallVtableMember(memory, ctx, base, old_object, 12, 0x824E8200u,
                        "sub_824E81A8 release");
  }

  ctx.r3.u64 = slot;
}
