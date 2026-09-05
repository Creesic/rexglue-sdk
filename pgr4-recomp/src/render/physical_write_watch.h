#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <unordered_map>
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
    for (auto& block : blocks_) block.store(revision, std::memory_order_relaxed);
  }

  uint64_t BeginSnapshot(rex::memory::Memory* memory, uint32_t address, uint32_t size) {
    if (!active_.load(std::memory_order_acquire) || memory != memory_ || size < kPageSize ||
        uint64_t(address) + size > kPhysicalSize)
      return 0;
    const uint64_t revision = Revision(address, size);
    // Pages stay protected until a write bumps their revision, so a range this
    // thread armed at the same revision skips the SDK page walk (global lock,
    // three heaps, one iteration per page) that otherwise runs on every draw.
    thread_local std::unordered_map<uint64_t, uint64_t> armed;
    const uint64_t key = uint64_t(address) << 32 | size;
    const auto it = armed.find(key);
    if (it != armed.end() && it->second == revision) return revision;
    // A write racing with arming must force validation on this draw too.
    // Arm outside the upload allocator mutex to preserve global -> local order.
    if (!memory_->EnablePhysicalMemoryAccessCallbacks(address, size, true, false)) return 0;
    if (revision != Revision(address, size)) return 0;
    armed[key] = revision;
    return revision;
  }

  uint64_t Revision(uint32_t address, uint32_t size) const {
    uint64_t revision = 1;
    const uint32_t last = (address + size - 1) / kPageSize;
    for (uint32_t i = address / kPageSize; i <= last;) {
      // A whole block reads one summary instead of kBlockPages pages.
      if ((i & (kBlockPages - 1)) == 0 && i + kBlockPages - 1 <= last) {
        revision = std::max(revision, blocks_[i / kBlockPages].load(std::memory_order_acquire));
        i += kBlockPages;
      } else {
        revision = std::max(revision, pages_[i].load(std::memory_order_acquire));
        ++i;
      }
    }
    return revision;
  }

  // Native command-processor stores bypass the protected virtual aliases.
  // Call after the store so even a snapshot racing the write becomes stale.
  void Written(uint32_t address, uint32_t size) {
    if (!active_.load(std::memory_order_acquire) || size == 0) return;
    address &= kPhysicalSize - 1;
    const uint32_t last = uint32_t(std::min(uint64_t(address) + size, uint64_t(kPhysicalSize)) - 1);
    const uint64_t revision = serial_.fetch_add(1, std::memory_order_relaxed) + 1;
    const auto raise = [revision](std::atomic<uint64_t>& slot) {
      auto previous = slot.load(std::memory_order_relaxed);
      while (previous < revision && !slot.compare_exchange_weak(
          previous, revision, std::memory_order_release, std::memory_order_relaxed)) {}
    };
    for (uint32_t i = address / kPageSize; i <= last / kPageSize; ++i) {
      raise(pages_[i]);
      raise(blocks_[i / kBlockPages]);
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

  static constexpr uint32_t kPageSize = 4096, kPhysicalSize = 0x20000000, kBlockPages = 64;
  std::array<std::atomic<uint64_t>, kPhysicalSize / kPageSize> pages_{};
  // Per-block (256 KB) maximum of pages_, kept by Written for cheap range reads.
  std::array<std::atomic<uint64_t>, kPhysicalSize / kPageSize / kBlockPages> blocks_{};
  std::atomic<uint64_t> serial_{1};
  std::atomic<bool> active_{false};
  rex::memory::Memory* memory_ = nullptr;
  void* handle_ = nullptr;
};

inline PhysicalWriteWatch g_physicalWriteWatch;

}  // namespace pgr4::render
