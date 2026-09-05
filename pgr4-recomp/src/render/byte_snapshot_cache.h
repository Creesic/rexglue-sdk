#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <rex/hash.h>
#include <rex/memory/utils.h>

namespace pgr4::render {

// Immutable payloads shared by repeated draws. The owner clears only after its
// consumers retire (render queue, GPU frame slot, or recorded command batch).
class ByteSnapshotCache {
 public:
  // Swap complete 2- or 4-byte elements; retain any trailing bytes unchanged.
  // Optional identity names these immutable bytes, not their allocation address.
  uint8_t* Copy(const void* source, uint32_t size, uint32_t swapElementBytes = 0,
                uint64_t* identity = nullptr, uint64_t revision = 0) {
    if (identity != nullptr) *identity = 0;
    if (source == nullptr || size == 0)
      return nullptr;
    Entry* entry = nullptr;
    // Without a write-watch revision, the address only hints at a content hash;
    // guest buffers and retired staging addresses still require byte validation.
    const auto previous = sourceHints_.find(source);
    // A nonzero revision is supplied only by physical-memory write tracking.
    // Other sources (including recorded payloads) still validate every byte.
    if (revision != 0 && previous != sourceHints_.end() && previous->second.revision == revision &&
        entries_[previous->second.entryIndex].bytes.size() == size)
      entry = &entries_[previous->second.entryIndex];
    if (entry == nullptr && previous != sourceHints_.end() && previous->second.contentFirst)
      entry = Find(source, size, previous->second.hash);
    if (entry == nullptr) {
      const uint64_t hash = XXH3_64bits(source, size);
      entry = Find(source, size, hash);
      if (entry == nullptr) {
        if (usedEntries_ == entries_.size()) entries_.emplace_back();
        entry = &entries_[usedEntries_];
        const auto* bytes = static_cast<const uint8_t*>(source);
        entry->bytes.assign(bytes, bytes + size);
        entry->swappedValid = {};
        entry->identities = {};
        lookup_.emplace(hash, usedEntries_++);
      }
      // A changing source should hash first on its next use, avoiding a
      // redundant full comparison against the preceding version every draw.
      const bool contentFirst = previous == sourceHints_.end() || previous->second.hash == hash;
      sourceHints_[source] = {hash, contentFirst, revision, size_t(entry - entries_.data())};
    } else {
      previous->second.revision = revision;
      previous->second.entryIndex = size_t(entry - entries_.data());
    }
    if (identity != nullptr) {
      auto& id = entry->identities[swapElementBytes == 2 ? 1 : swapElementBytes == 4 ? 2 : 0];
      if (id == 0) id = nextIdentity_.fetch_add(1, std::memory_order_relaxed);
      *identity = id;
    }
    if (swapElementBytes != 2 && swapElementBytes != 4)
      return entry->bytes.data();
    auto& swapped = swapElementBytes == 2 ? entry->words : entry->dwords;
    auto& valid = entry->swappedValid[swapElementBytes == 2 ? 0 : 1];
    if (!valid) {
      swapped.resize(size);
      // Convert directly into retained storage; no preliminary payload copy.
      if (swapElementBytes == 2)
        rex::memory::copy_and_swap_16_unaligned(swapped.data(), entry->bytes.data(), size / 2);
      else
        rex::memory::copy_and_swap_32_unaligned(swapped.data(), entry->bytes.data(), size / 4);
      const uint32_t wholeBytes = size - size % swapElementBytes;
      if (wholeBytes != size)
        std::memcpy(swapped.data() + wholeBytes, entry->bytes.data() + wholeBytes, size - wholeBytes);
      valid = true;
    }
    return swapped.data();
  }

  void Clear() {
    sourceHints_.clear();
    lookup_.clear();
    usedEntries_ = 0;
  }

 private:
  struct Entry {
    std::vector<uint8_t> bytes, words, dwords;
    std::array<uint64_t, 3> identities{};
    std::array<bool, 2> swappedValid{};
  };
  struct SourceHint {
    uint64_t hash;
    bool contentFirst;
    uint64_t revision;
    size_t entryIndex;
  };

  Entry* Find(const void* source, uint32_t size, uint64_t hash) {
    const auto [first, last] = lookup_.equal_range(hash);
    for (auto it = first; it != last; ++it) {
      Entry& entry = entries_[it->second];
      if (entry.bytes.size() == size && std::memcmp(entry.bytes.data(), source, size) == 0)
        return &entry;
    }
    return nullptr;
  }

  // Reuse retired payload allocations like the oracles' upload arenas. Vector
  // growth moves Entry owners, not their immutable byte allocations; lookup
  // indices also remain valid in a deep copy of this cache.
  // ponytail: retain per-entry peak capacity; use size classes if changing
  // workloads inflate retained memory.
  std::vector<Entry> entries_;
  size_t usedEntries_ = 0;
  std::unordered_multimap<uint64_t, size_t> lookup_;
  std::unordered_map<const void*, SourceHint> sourceHints_;
  inline static std::atomic<uint64_t> nextIdentity_{1};
};

}  // namespace pgr4::render
