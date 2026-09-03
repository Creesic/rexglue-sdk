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
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host) -
                               reinterpret_cast<uintptr_t>(GuestBase()));
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
