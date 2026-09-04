#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <rex/hash.h>

namespace pgr4::render {

// Immutable payloads shared by repeated draws. The owner clears only after its
// consumers retire (render queue, GPU frame slot, or recorded command batch).
class ByteSnapshotCache {
 public:
  uint8_t* Copy(const void* source, uint32_t size, bool swapDwords = false) {
    if (source == nullptr || size == 0)
      return nullptr;
    const uint64_t hash = XXH3_64bits(source, size);
    const auto [first, last] = entries_.equal_range(hash);
    Entry* entry = nullptr;
    for (auto it = first; it != last; ++it) {
      if (it->second.bytes.size() == size &&
          std::memcmp(it->second.bytes.data(), source, size) == 0) {
        entry = &it->second;
        break;
      }
    }
    if (entry == nullptr) {
      entry = &entries_.emplace(hash, Entry{})->second;
      const auto* bytes = static_cast<const uint8_t*>(source);
      entry->bytes.assign(bytes, bytes + size);
    }
    if (!swapDwords)
      return entry->bytes.data();
    auto& swapped = entry->dwords;
    if (swapped.empty()) {
      swapped = entry->bytes;
      for (uint32_t offset = 0; size - offset >= sizeof(uint32_t); offset += sizeof(uint32_t)) {
        uint32_t word;
        std::memcpy(&word, swapped.data() + offset, sizeof(word));
        word = std::byteswap(word);
        std::memcpy(swapped.data() + offset, &word, sizeof(word));
      }
    }
    return swapped.data();
  }

  void Clear() { entries_.clear(); }

 private:
  struct Entry {
    std::vector<uint8_t> bytes, dwords;
  };
  std::unordered_multimap<uint64_t, Entry> entries_;
};

}  // namespace pgr4::render
