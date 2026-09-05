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

// Immutable payloads shared by repeated draws. Entries persist across frames;
// the owner retires once per frame after its consumers finish (render queue,
// GPU frame slot, recorded command batch), which is when versions superseded
// during that frame are released for reuse.
class ByteSnapshotCache {
  struct Entry;

 public:
  struct Stats {
    uint64_t suffixHits = 0, hintHits = 0, contentHits = 0, creates = 0, createdBytes = 0;
    uint64_t releases = 0, entries = 0, suffixes = 0;
  };
  Stats stats() const {
    Stats s = stats_;
    s.entries = usedEntries_ - freeEntries_.size();
    s.suffixes = suffixes_.size();
    return s;
  }

  // Swap complete 2- or 4-byte elements; retain any trailing bytes unchanged.
  // Optional identity names these immutable bytes, not their allocation address.
  uint8_t* Copy(const void* source, uint32_t size, uint32_t swapElementBytes = 0,
                uint64_t* identity = nullptr, uint64_t revision = 0) {
    if (identity != nullptr) *identity = 0;
    if (source == nullptr || size == 0)
      return nullptr;
    const auto* bytes = static_cast<const uint8_t*>(source);
    // Ranges ending at the same address are suffixes of one guest buffer. A
    // watched sub-range is served from the longest cached suffix while no page
    // of it was written since that copy: no hash, no copy.
    if (revision != 0) {
      const auto suffix = suffixes_.find(bytes + size);
      if (suffix != suffixes_.end() && Live(suffix->second.entryIndex, suffix->second.generation) &&
          suffix->second.start <= bytes && revision <= suffix->second.revision) {
        const size_t offset = size_t(bytes - suffix->second.start);
        if (swapElementBytes == 0 || offset % swapElementBytes == 0) {
          ++stats_.suffixHits;
          return View(entries_[suffix->second.entryIndex], offset, swapElementBytes, identity);
        }
      }
    }
    Entry* entry = nullptr;
    // Without a write-watch revision, the address only hints at a content hash;
    // guest buffers and retired staging addresses still require byte validation.
    const auto previous = sourceHints_.find(source);
    const bool hinted = previous != sourceHints_.end();
    const bool hintLive =
        hinted && Live(previous->second.entryIndex, previous->second.generation);
    // A nonzero revision is supplied only by physical-memory write tracking.
    // Other sources (including recorded payloads) still validate every byte.
    if (revision != 0 && hintLive && previous->second.revision == revision &&
        entries_[previous->second.entryIndex].bytes.size() == size) {
      entry = &entries_[previous->second.entryIndex];
      ++stats_.hintHits;
    }
    if (entry == nullptr && hinted && previous->second.contentFirst) {
      entry = Find(source, size, previous->second.hash);
      if (entry != nullptr) ++stats_.contentHits;
    }
    if (entry == nullptr) {
      const uint64_t hash = XXH3_64bits(source, size);
      entry = Find(source, size, hash);
      if (entry != nullptr)
        ++stats_.contentHits;
      else
        entry = &Create(bytes, size, hash);
      const size_t index = size_t(entry - entries_.data());
      // The version this source produced before is superseded: release it once
      // the frame retires. Versions other sources matched by content stay.
      if (hintLive && previous->second.entryIndex != index &&
          entries_[previous->second.entryIndex].source == bytes)
        stale_.push_back(previous->second.entryIndex);
      // A changing source should hash first on its next use, avoiding a
      // redundant full comparison against the preceding version every draw.
      const bool contentFirst = !hinted || previous->second.hash == hash;
      sourceHints_[source] = {hash, contentFirst, revision, index, entry->generation};
    } else {
      previous->second.revision = revision;
      previous->second.entryIndex = size_t(entry - entries_.data());
      previous->second.generation = entry->generation;
    }
    if (revision != 0) {
      // The longest validated suffix serves the shorter ones; a stale record
      // never serves stale bytes because every hit re-checks the revision.
      auto& suffix = suffixes_[bytes + size];
      if (suffix.start == nullptr || bytes <= suffix.start ||
          !Live(suffix.entryIndex, suffix.generation))
        suffix = {size_t(entry - entries_.data()), entry->generation, bytes, revision};
    }
    return View(*entry, 0, swapElementBytes, identity);
  }

  // Once per frame from the owner, after that frame's consumers finished:
  // releases the versions superseded during the frame (their storage is reused
  // by later versions) and, past the byte budget, clears everything.
  // ponytail: whole-cache clear past the budget; LRU eviction if the hitch on
  // that frame ever matters.
  bool Retire() {
    for (const size_t index : stale_) Release(index);
    stale_.clear();
    if (retainedBytes_ <= kRetainBudget) return false;
    Clear();
    return true;
  }

  size_t RetainedBytes() const { return retainedBytes_; }

  void Clear() {
    sourceHints_.clear();
    suffixes_.clear();
    lookup_.clear();
    freeEntries_.clear();
    stale_.clear();
    usedEntries_ = 0;
    retainedBytes_ = 0;
  }

 private:
  struct Entry {
    std::vector<uint8_t> bytes, words, dwords;
    std::array<uint64_t, 3> identities{};
    std::array<bool, 2> swappedValid{};
    const uint8_t* source = nullptr;  // The source this version was copied from.
    uint64_t hash = 0;
    uint32_t generation = 0;  // Bumped on release so old references miss.
  };
  struct SuffixRecord {
    size_t entryIndex = 0;
    uint32_t generation = 0;
    const uint8_t* start = nullptr;
    uint64_t revision = 0;
  };
  struct SourceHint {
    uint64_t hash;
    bool contentFirst;
    uint64_t revision;
    size_t entryIndex;
    uint32_t generation;
  };

  bool Live(size_t index, uint32_t generation) const {
    return index < usedEntries_ && entries_[index].generation == generation &&
           !entries_[index].bytes.empty();
  }

  Entry* Find(const void* source, uint32_t size, uint64_t hash) {
    const auto [first, last] = lookup_.equal_range(hash);
    for (auto it = first; it != last; ++it) {
      Entry& entry = entries_[it->second];
      if (entry.bytes.size() == size && std::memcmp(entry.bytes.data(), source, size) == 0)
        return &entry;
    }
    return nullptr;
  }

  Entry& Create(const uint8_t* bytes, uint32_t size, uint64_t hash) {
    ++stats_.creates;
    stats_.createdBytes += size;
    size_t index;
    if (!freeEntries_.empty()) {
      index = freeEntries_.back();
      freeEntries_.pop_back();
    } else {
      if (usedEntries_ == entries_.size()) entries_.emplace_back();
      index = usedEntries_++;
    }
    Entry& entry = entries_[index];
    entry.bytes.assign(bytes, bytes + size);
    retainedBytes_ += size;
    entry.swappedValid = {};
    entry.identities = {};
    entry.source = bytes;
    entry.hash = hash;
    lookup_.emplace(hash, index);
    return entry;
  }

  // Storage stays allocated for the next version; every reference carries the
  // generation, so hints and suffix records that still name this slot miss.
  void Release(size_t index) {
    Entry& entry = entries_[index];
    if (entry.bytes.empty()) return;
    const auto [first, last] = lookup_.equal_range(entry.hash);
    for (auto it = first; it != last; ++it) {
      if (it->second == index) {
        lookup_.erase(it);
        break;
      }
    }
    const auto suffix = suffixes_.find(entry.source + entry.bytes.size());
    if (suffix != suffixes_.end() && suffix->second.entryIndex == index) suffixes_.erase(suffix);
    retainedBytes_ -= entry.bytes.size() + entry.words.size() + entry.dwords.size();
    entry.bytes.clear();
    entry.words.clear();
    entry.dwords.clear();
    entry.swappedValid = {};
    ++entry.generation;
    ++stats_.releases;
    freeEntries_.push_back(index);
  }

  // Bytes of `entry` from `offset` in the requested endian view. A sub-range
  // identity combines the entry identity with the offset; plain identities are
  // a counter far below 2^32, so the two never collide.
  uint8_t* View(Entry& entry, size_t offset, uint32_t swapElementBytes, uint64_t* identity) {
    if (identity != nullptr) {
      auto& id = entry.identities[swapElementBytes == 2 ? 1 : swapElementBytes == 4 ? 2 : 0];
      if (id == 0) id = nextIdentity_.fetch_add(1, std::memory_order_relaxed);
      *identity = offset == 0 ? id : (id << 32) | uint64_t(offset);
    }
    if (swapElementBytes != 2 && swapElementBytes != 4)
      return entry.bytes.data() + offset;
    auto& swapped = swapElementBytes == 2 ? entry.words : entry.dwords;
    auto& valid = entry.swappedValid[swapElementBytes == 2 ? 0 : 1];
    if (!valid) {
      const uint32_t size = uint32_t(entry.bytes.size());
      swapped.resize(size);
      retainedBytes_ += size;
      // Convert directly into retained storage; no preliminary payload copy.
      if (swapElementBytes == 2)
        rex::memory::copy_and_swap_16_unaligned(swapped.data(), entry.bytes.data(), size / 2);
      else
        rex::memory::copy_and_swap_32_unaligned(swapped.data(), entry.bytes.data(), size / 4);
      const uint32_t wholeBytes = size - size % swapElementBytes;
      if (wholeBytes != size)
        std::memcpy(swapped.data() + wholeBytes, entry.bytes.data() + wholeBytes, size - wholeBytes);
      valid = true;
    }
    return swapped.data() + offset;
  }

  // Reuse retired payload allocations like the oracles' upload arenas. Vector
  // growth moves Entry owners, not their immutable byte allocations; lookup
  // indices also remain valid in a deep copy of this cache.
  // ponytail: retain per-entry peak capacity; use size classes if changing
  // workloads inflate retained memory.
  static constexpr size_t kRetainBudget = size_t(512) << 20;
  std::vector<Entry> entries_;
  size_t usedEntries_ = 0;
  size_t retainedBytes_ = 0;
  Stats stats_;
  std::vector<size_t> freeEntries_, stale_;
  std::unordered_multimap<uint64_t, size_t> lookup_;
  std::unordered_map<const void*, SourceHint> sourceHints_;
  // Keyed by range end: the longest suffix of one guest buffer seen so far.
  std::unordered_map<const uint8_t*, SuffixRecord> suffixes_;
  inline static std::atomic<uint64_t> nextIdentity_{1};
};

}  // namespace pgr4::render
