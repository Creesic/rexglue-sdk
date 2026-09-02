// Pure helpers shared by the trace and the native hooks. Everything here is
// constexpr and self-checked with static_assert; no runtime test target needed.
#pragma once

#include <cstddef>
#include <cstdint>

#include <rex/types.h>

namespace fm4::gpu {

// Xenos shader container header: dword[1] = virtualSize, dword[2] =
// physicalSize. XenosRecomp hashes exactly virtualSize + physicalSize bytes
// from the container start, so a dump of that many bytes is what it expects.
constexpr uint32_t ShaderContainerBytes(uint32_t virtual_size, uint32_t physical_size) {
  return virtual_size + physical_size;
}
inline uint32_t ShaderContainerBytes(const rex::be<uint32_t>* words) {
  return ShaderContainerBytes(static_cast<uint32_t>(words[1]), static_cast<uint32_t>(words[2]));
}

// File-name hash only; XenosRecomp computes its own content hash.
constexpr uint64_t Fnv1a64(const uint8_t* p, size_t n) {
  uint64_t h = 14695981039346656037ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

// Public D3DDevice ring fields (ida40 type_inspect D3DDevice, 2026-09-02).
constexpr uint32_t kDevRing = 0x30;           // m_pRing: next write
constexpr uint32_t kDevRingLimit = 0x34;      // m_pRingLimit
constexpr uint32_t kDevRingGuarantee = 0x38;  // m_pRingGuarantee: "make space" when m_pRing exceeds this

// PM4 sink: a host-allocated guest buffer the D3D library writes packets into
// and nobody reads. D3D_RingMakeSpace resets m_pRing to the base, so the
// guarantee window must cover the largest burst the library writes between two
// guarantee checks. 64 KiB is 16x the largest burst seen in the XDK library.
constexpr uint32_t kRingSinkBytes = 1u << 20;
constexpr uint32_t kRingSinkGuaranteeBytes = 64u << 10;
constexpr uint32_t RingSinkLimit(uint32_t base) { return base + kRingSinkBytes; }
constexpr uint32_t RingSinkGuarantee(uint32_t base) {
  return base + kRingSinkBytes - kRingSinkGuaranteeBytes;
}

// Self-checks.
static_assert(ShaderContainerBytes(0x100, 0x40) == 0x140);
static_assert(RingSinkGuarantee(0x1000) < RingSinkLimit(0x1000));
static_assert(RingSinkGuarantee(0x1000) > 0x1000);
constexpr uint8_t kFnvProbe[3] = {'a', 'b', 'c'};
static_assert(Fnv1a64(kFnvProbe, 3) == 0xE71FA2190541574Bull);  // FNV-1a("abc")

}  // namespace fm4::gpu
