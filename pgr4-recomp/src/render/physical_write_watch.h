#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <rex/system/xmemory.h>

namespace pgr4::render {

// The SDK protects all guest virtual aliases of physical memory. Track its
// invalidations, including decommit/reallocation, without taking renderer locks
// inside the memory system's global critical region.
class PhysicalWriteWatch {
 public:
  void Initialize(rex::memory::Memory* memory) {
    Shutdown();
    if (memory == nullptr) return;
    memory_ = memory;
    handle_ = memory_->RegisterPhysicalMemoryInvalidationCallback(Invalidated, this);
    active_.store(handle_ != nullptr, std::memory_order_release);
  }

  void Shutdown() {
    active_.store(false, std::memory_order_release);
    if (handle_ != nullptr) memory_->UnregisterPhysicalMemoryInvalidationCallback(handle_);
    handle_ = nullptr;
    memory_ = nullptr;
    // A new memory instance must never match an earlier snapshot revision.
    const uint64_t revision = serial_.fetch_add(1, std::memory_order_relaxed) + 1;
    for (auto& page : pages_) page.store(revision, std::memory_order_relaxed);
  }

  uint64_t BeginSnapshot(rex::memory::Memory* memory, uint32_t address, uint32_t size) {
    if (!active_.load(std::memory_order_acquire) || memory != memory_ || size < kPageSize ||
        uint64_t(address) + size > kPhysicalSize)
      return 0;
    const uint64_t revision = Revision(address, size);
    // A write racing with arming must force validation on this draw too.
    // Arm outside the upload allocator mutex to preserve global -> local order.
    if (!memory_->EnablePhysicalMemoryAccessCallbacks(address, size, true, false)) return 0;
    return revision == Revision(address, size) ? revision : 0;
  }

  uint64_t Revision(uint32_t address, uint32_t size) const {
    uint64_t revision = 1;
    for (uint32_t i = address / kPageSize; i <= (address + size - 1) / kPageSize; ++i)
      revision = std::max(revision, pages_[i].load(std::memory_order_acquire));
    return revision;
  }

  // Native command-processor stores bypass the protected virtual aliases.
  // Call after the store so even a snapshot racing the write becomes stale.
  void Written(uint32_t address, uint32_t size) {
    if (!active_.load(std::memory_order_acquire) || size == 0) return;
    address &= kPhysicalSize - 1;
    const uint32_t last = uint32_t(std::min(uint64_t(address) + size, uint64_t(kPhysicalSize)) - 1);
    const uint64_t revision = serial_.fetch_add(1, std::memory_order_relaxed) + 1;
    for (uint32_t i = address / kPageSize; i <= last / kPageSize; ++i) {
      auto previous = pages_[i].load(std::memory_order_relaxed);
      while (previous < revision && !pages_[i].compare_exchange_weak(
          previous, revision, std::memory_order_release, std::memory_order_relaxed)) {}
    }
  }

 private:
  static std::pair<uint32_t, uint32_t> Invalidated(void* context, uint32_t address,
                                                  uint32_t size, bool exact) {
    if (!exact) {
      // Unprotect a block for sequential writes instead of faulting per page.
      const uint64_t end = std::min((uint64_t(address) + size + 0xFFFFu) & ~uint64_t{0xFFFF},
                                    uint64_t(kPhysicalSize));
      address &= ~0xFFFFu;
      size = uint32_t(end - address);
    }
    static_cast<PhysicalWriteWatch*>(context)->Written(address, size);
    return {address, size};
  }

  static constexpr uint32_t kPageSize = 4096, kPhysicalSize = 0x20000000;
  std::array<std::atomic<uint64_t>, kPhysicalSize / kPageSize> pages_{};
  std::atomic<uint64_t> serial_{1};
  std::atomic<bool> active_{false};
  rex::memory::Memory* memory_ = nullptr;
  void* handle_ = nullptr;
};

inline PhysicalWriteWatch g_physicalWriteWatch;

}  // namespace pgr4::render
