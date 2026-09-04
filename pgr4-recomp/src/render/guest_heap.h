#pragma once

#include <cstdint>
#include <new>
#include <utility>

#include <rex/memory.h>
#include <rex/system/kernel_state.h>

namespace pgr4::ghp {

inline auto* GuestMemory() { return rex::system::kernel_state()->memory(); }
inline uint8_t* GuestBase() { return GuestMemory()->virtual_membase(); }

// Raw guest-memory allocation; returns a guest virtual address (0 on failure).
inline uint32_t GuestAllocRaw(uint32_t size, uint32_t alignment = 0x10) {
  return GuestMemory()->SystemHeapAlloc(size, alignment);
}

inline void GuestFreeRaw(uint32_t guestAddress) {
  if (guestAddress) GuestMemory()->SystemHeapFree(guestAddress);
}

// host pointer -> guest virtual address.
inline uint32_t ToGuest(const void* host) {
  if (!host) return 0;
  return GuestMemory()->HostToGuestVirtual(host);
}

// Base address out of a D3D texture / buffer header -> guest physical. The
// XDK stores the CPU virtual alias (0xA/0xC/0xE ranges) there and converts
// when it builds GPU fetch constants; the 0xE0000000 range carries a 4 KB
// offset (SDK Memory::GetPhysicalAddress), so a plain & 0x1FFFFFFF mask lands
// one page early (PGR4's Bink planes came out rotated by 4096 mod pitch).
inline uint32_t HeaderBaseToPhysical(uint32_t guestAddress) {
  if (guestAddress < 0xA0000000u)
    return guestAddress & 0x1FFFFFFFu;  // already a GPU physical address
  const uint32_t physical = GuestMemory()->GetPhysicalAddress(guestAddress & ~0xFFFu);
  if (physical == UINT32_MAX)
    return guestAddress & 0x1FFFFFFFu;
  return physical | (guestAddress & 0xFFFu);
}

constexpr bool ContainsNonZero(const uint8_t* bytes, uint32_t size) {
  for (uint32_t i = 0; i < size; ++i) {
    if (bytes[i] != 0)
      return true;
  }
  return false;
}

constexpr uint32_t SelectHeaderReadAddress(uint32_t mapped, uint32_t direct,
                                           bool mappedHasData, bool directHasData) {
  return !mappedHasData && directHasData ? direct : mapped;
}

static_assert([] {
  constexpr uint8_t empty[4]{};
  constexpr uint8_t populated[4]{0, 0, 1, 0};
  return SelectHeaderReadAddress(0x2000, 0x1000, ContainsNonZero(empty, 4),
                                 ContainsNonZero(populated, 4)) == 0x1000;
}());

inline bool PhysicalRangeStartsWithData(uint32_t address, uint32_t size) {
  if (size == 0)
    return false;
  const uint32_t sampleSize = size < 0x1000u ? size : 0x1000u;
  if (uint64_t(address) + sampleSize > 0x20000000ull)
    return false;
  auto* memory = GuestMemory();
  if (memory->GetPhysicalHeap()->QueryRangeAccess(address, address + sampleSize - 1u) ==
      rex::memory::PageAccess::kNoAccess) {
    return false;
  }
  return ContainsNonZero(memory->TranslatePhysical<const uint8_t*>(address), sampleSize);
}

// PGR4 has raw resources populated through both CPU aliases: Bink writes the
// XDK-mapped E/F page, while placed asset textures can be written to the direct
// physical page. Prefer the XDK mapping, falling back only when its first page
// is empty and the direct page contains data.
inline uint32_t HeaderBaseToPhysicalForRead(uint32_t guestAddress, uint32_t size) {
  const uint32_t mapped = HeaderBaseToPhysical(guestAddress);
  const uint32_t direct = guestAddress & 0x1FFFFFFFu;
  if (mapped == direct)
    return mapped;
  return SelectHeaderReadAddress(mapped, direct, PhysicalRangeStartsWithData(mapped, size),
                                 PhysicalRangeStartsWithData(direct, size));
}

// guest virtual address -> host pointer.
template <typename T>
inline T* ToHost(uint32_t guestAddress) {
  return guestAddress ? rex::memory::GuestPtr<T*>(GuestBase(), guestAddress) : nullptr;
}

// Construct a T inside guest memory; returns the host pointer (its guest address
// is ToGuest(result)). Returns nullptr if the allocation failed.
template <typename T, typename... Args>
inline T* GuestNew(Args&&... args) {
  constexpr uint32_t align = alignof(T) < 0x10 ? 0x10 : alignof(T);
  uint32_t addr = GuestAllocRaw(sizeof(T), align);
  if (!addr) return nullptr;
  T* host = ToHost<T>(addr);
  return new (host) T(std::forward<Args>(args)...);
}

// Destroy a guest-allocated T and free its memory.
template <typename T>
inline void GuestDelete(T* host) {
  if (!host) return;
  uint32_t addr = ToGuest(host);
  host->~T();
  GuestFreeRaw(addr);
}

}  // namespace pgr4::ghp
