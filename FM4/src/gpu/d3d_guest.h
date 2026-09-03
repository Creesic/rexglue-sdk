// Pure helpers shared by the trace and the native hooks, plus the TraceOnSwap
// hand-off. The constexpr helpers are self-checked with static_assert.
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

// Self-checks.
static_assert(ShaderContainerBytes(0x100, 0x40) == 0x140);
constexpr uint8_t kFnvProbe[3] = {'a', 'b', 'c'};
static_assert(Fnv1a64(kFnvProbe, 3) == 0xE71FA2190541574Bull);  // FNV-1a("abc")

// Implemented in fm4_d3d_trace.cpp: per-frame trace bookkeeping, no-op unless
// fm4_d3d_trace is set. Called by the D3DDevice_Swap hook in fm4_d3d_hooks.cpp.
void TraceOnSwap();

// Same hand-off for the three creation entry points the native renderer now
// owns outright (render/d3d_hooks.cpp): the trace cannot define its own hook
// for a guest function another translation unit already replaces.
void TraceOnCreateTexture();
void TraceOnCreateShader(uint32_t function_va, bool pixel);

}  // namespace fm4::gpu
