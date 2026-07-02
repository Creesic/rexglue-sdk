/**
 * TEMP_DIAG: DOAX pool "fall in the water -> full-screen black" swap/resolve sync.
 *
 * Superseded note, 2026-06-30: the stronger RenderDoc comparison is
 * doax_2026.06.30_16.03.47_frame5631.rdc vs doaxgood.rdc. Those captures match
 * through EID 5235 / EID 4832, then the bad capture skips the good capture's
 * depth-only 2xMSAA branch and later indexed scene work, running only a tiny
 * presenter tail. This diagnostic is still useful for checking frontbuffer
 * publish/resolve ordering, but the current lead is a skipped command-stream
 * branch, not a proven resolve-after-present ordering bug.
 *
 * These two greppable lines share a monotonic seq (g_seq = swaps-so-far):
 *   DOAXSWAP seq=N present=PPPPPPPP base=BBBBB000 other=OOOOO000
 *   DOAXRSV  seq=M dst=BBBBB000 len=... depth=0
 * On a black frame, take the swap's present base Bx and scan the DOAXRSV history:
 *   - newest DOAXRSV dst=Bx has seq < N  -> Bx was filled before the swap   (OK)
 *   - a DOAXRSV dst=Bx has seq == N      -> resolve ran AFTER swap N         (swap races resolve)
 *   - no recent DOAXRSV dst=Bx           -> Bx never filled                  (flip desync / dead buffer)
 * That three-way split picks the fix site: swap gating vs flip-index vs fiber wakeup.
 *
 * The two frontbuffer bases are LEARNED from swaps (no hard-coded guest VA), so
 * DOAXRSV is emitted only for resolves that hit a buffer the title actually
 * presents -- it is not swamped by ordinary scene resolves.
 *
 * Single rexgraphics object library => inline state merges to one instance (the
 * gpu_sync_diag DLL/EXE split does not apply here). Grep "DOAXSWAP\|DOAXRSV".
 * Remove this file and both call sites when the swap/resolve sync bug is fixed.
 */
#pragma once

#include <atomic>
#include <cstdint>

#include <rex/logging.h>

namespace doax_swap_diag {

inline std::atomic<uint64_t> g_seq{0};
// The two most-recent distinct page-aligned frontbuffer bases seen at swaps.
inline std::atomic<uint32_t> g_fb_a{0};
inline std::atomic<uint32_t> g_fb_b{0};

inline uint32_t PageBase(uint32_t ptr) { return ptr & ~uint32_t(0xFFF); }

// Called from the CommandProcessor swap dispatch with the guest frontbuffer ptr.
inline void OnSwap(uint32_t frontbuffer_ptr) {
  const uint32_t base = PageBase(frontbuffer_ptr);
  const uint32_t a = g_fb_a.load(std::memory_order_relaxed);
  if (base != a) {
    g_fb_b.store(a, std::memory_order_relaxed);  // remember the partner buffer
    g_fb_a.store(base, std::memory_order_relaxed);
  }
  const uint64_t seq = g_seq.fetch_add(1, std::memory_order_relaxed) + 1;
  REXGPU_WARN("DOAXSWAP seq={} present={:08X} base={:08X} other={:08X}", seq, frontbuffer_ptr, base,
              g_fb_b.load(std::memory_order_relaxed));
}

// True if a page-aligned dest base matches a frontbuffer the title presents.
inline bool IsFrontbuffer(uint32_t dest_base) {
  const uint32_t base = PageBase(dest_base);
  return base != 0 && (base == g_fb_a.load(std::memory_order_relaxed) ||
                       base == g_fb_b.load(std::memory_order_relaxed));
}

// Called from the guest-output resolve with its dest base. Logs only resolves that
// target a known frontbuffer, tagged with the current swap epoch (g_seq).
inline void OnResolve(uint32_t copy_dest_base, uint32_t dest_length, bool is_depth,
                      uint32_t src_fmt, uint32_t src_is_64bpp, uint32_t src_msaa,
                      uint32_t dest_fmt) {
  if (!IsFrontbuffer(copy_dest_base)) {
    return;
  }
  // src_fmt = xenos::ColorRenderTargetFormat, dest_fmt = xenos::ColorFormat. A
  // front-buffer resolve whose src_fmt != dest_fmt (esp. a k_2_10_10_10_FLOAT /
  // 7e3 source) is the suspect for the pool "fall in water" black: the pack to the
  // R10G10B10A2 present buffer produces zero. Absent DOAXRSV => resolve skipped.
  REXGPU_WARN("DOAXRSV seq={} dst={:08X} len={:#x} depth={} srcfmt={} src64={} msaa={} dstfmt={}",
              g_seq.load(std::memory_order_relaxed), copy_dest_base, dest_length, is_depth ? 1 : 0,
              src_fmt, src_is_64bpp, src_msaa, dest_fmt);
}

}  // namespace doax_swap_diag
