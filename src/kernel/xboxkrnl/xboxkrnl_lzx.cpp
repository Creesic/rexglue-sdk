/**
 * @file        xboxkrnl_lzx.cpp
 * @brief       Xbox 360 LDI (LZX Decompression Interface) kernel exports.
 *
 * @copyright   Copyright (c) 2026 Tom Clay. All rights reserved.
 * @license     BSD 3-Clause License - see LICENSE in the project root.
 *
 * Implements the xboxkrnl LDI* family used by titles that stream LZX-compressed
 * archive assets (e.g. Forza Horizon's media `.zip` files driven by
 * zipmanifest.xml). These were previously no-op stubs in xboxkrnl_modules.cpp,
 * which made every archive block decompress to nothing -> blank assets.
 *
 * Model (confirmed against Forza Horizon default.xex, FUN_82b0a850 /
 * FUN_82b0a6a0):
 *   ctx = LDICreateDecompression(&windowSize, &blockSize, 0, 0, window, &scratch, &ctxOut)
 *   per 32 KB block:  LDIDecompress(ctx, src, (u16)cbSrc, dst, &cbDst)   // 0 = success
 *   between files:    LDIResetDecompression(ctx)
 *   teardown:         LDIDestroyDecompression(ctx)
 *
 * Each block is an independent LZX stream whose dictionary is the previously
 * decompressed output (the "window"). That is exactly how the XEX loader's
 * per-block decompression works, so we reuse lzx_decompress() (mspack lzxd) and
 * carry a rolling history buffer as the seed window across LDIDecompress calls.
 */

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/system/kernel_state.h>
#include <rex/system/lzx.h>
#include <rex/types.h>

// Optional override for the LZX window size when titles disagree with our
// derivation (different asset families use different windows: FH1 media zips
// pass 0x8000, the map decompressor reference uses wbits=17 / 0x20000). 0 =
// derive from the size the title passes to LDICreateDecompression. Hot-reload
// so it can be tuned at runtime without a rebuild.
REXCVAR_DEFINE_UINT32(ldi_window_bits, 0, "Kernel",
                      "Override LZX window bits for LDI decompression (0 = use title value)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

namespace rex {
namespace kernel {
namespace xboxkrnl {

namespace {

struct LdiContext {
  uint32_t window_size = 0x8000;  // LZX window in bytes (power of two)
  // Rolling decompressed history used to seed the window for the next block.
  // Holds at most window_size bytes (the most recent output).
  std::vector<uint8_t> history;
};

std::mutex g_ldi_mutex;
std::unordered_map<uint32_t, std::unique_ptr<LdiContext>> g_ldi_contexts;
uint32_t g_ldi_next_handle = 1;

uint32_t ResolveWindowSize(uint32_t title_value) {
  uint32_t override_bits = REXCVAR_GET(ldi_window_bits);
  if (override_bits >= 15 && override_bits <= 21) {
    return 1u << override_bits;
  }
  // Round the title-provided value up to a power-of-two LZX window in the valid
  // range [2^15, 2^21]. lzx_decompress derives window_bits from this via
  // bit_scan_forward, so it must be an exact power of two.
  uint32_t window_size = 0x8000;  // 2^15, the LZX minimum
  while (window_size < title_value && window_size < 0x200000) {
    window_size <<= 1;
  }
  return window_size;
}

}  // namespace

// int LDICreateDecompression(uint32_t* pWindowSize, uint32_t* pBlockSize,
//                            uint32_t unk3, uint32_t unk4, void* window,
//                            uint32_t* pScratchOut, void** ppContext);
u32 LDICreateDecompression_entry(mapped_u32 window_size_ptr, mapped_u32 block_size_ptr,
                                 u32 unk3, u32 unk4, u32 window_addr, mapped_u32 scratch_out_ptr,
                                 mapped_u32 context_out_ptr) {
  (void)unk3;
  (void)unk4;
  (void)window_addr;

  uint32_t title_window = window_size_ptr ? static_cast<uint32_t>(*window_size_ptr) : 0;
  uint32_t block_size = block_size_ptr ? static_cast<uint32_t>(*block_size_ptr) : 0;
  uint32_t window_size = ResolveWindowSize(title_window);

  uint32_t handle;
  {
    std::lock_guard<std::mutex> lock(g_ldi_mutex);
    handle = g_ldi_next_handle++;
    auto ctx = std::make_unique<LdiContext>();
    ctx->window_size = window_size;
    ctx->history.reserve(window_size);
    g_ldi_contexts.emplace(handle, std::move(ctx));
  }

  if (scratch_out_ptr) {
    *scratch_out_ptr = 0;
  }
  if (context_out_ptr) {
    *context_out_ptr = handle;
  }

  REXKRNL_INFO("LDICreateDecompression: title_window={:#x} block={:#x} -> window={:#x} handle={}",
               title_window, block_size, window_size, handle);
  return 0;  // success
}

// int LDIDecompress(void* context, void* src, uint16_t cbSrc, void* dst, uint32_t* pcbDst);
u32 LDIDecompress_entry(u32 context_handle, u32 src_addr, u32 cb_src, u32 dst_addr,
                        mapped_u32 cb_dst_ptr) {
  const uint16_t comp_len = static_cast<uint16_t>(cb_src & 0xFFFF);
  const uint32_t out_len = cb_dst_ptr ? static_cast<uint32_t>(*cb_dst_ptr) : 0;

  LdiContext* ctx = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_ldi_mutex);
    auto it = g_ldi_contexts.find(context_handle);
    if (it != g_ldi_contexts.end()) {
      ctx = it->second.get();
    }
  }
  if (!ctx) {
    REXKRNL_WARN("LDIDecompress: unknown context handle {}", context_handle);
    return 1;  // failure
  }

  auto* memory = REX_KERNEL_MEMORY();
  const auto* src = memory->TranslateVirtual<const uint8_t*>(src_addr);
  auto* dst = memory->TranslateVirtual<uint8_t*>(dst_addr);
  if (!src || !dst || comp_len == 0 || out_len == 0) {
    REXKRNL_WARN("LDIDecompress: bad args ctx={} src={:#x} cbSrc={} dst={:#x} outLen={}",
                 context_handle, src_addr, comp_len, dst_addr, out_len);
    return 1;
  }

  // Decompress one block, seeding the window with the prior output dictionary.
  void* window_data = ctx->history.empty() ? nullptr : ctx->history.data();
  size_t window_data_len = ctx->history.size();
  int rc = lzx_decompress(src, comp_len, dst, out_len, ctx->window_size, window_data,
                          window_data_len);
  if (rc != 0) {
    REXKRNL_WARN("LDIDecompress: lzx_decompress failed rc={} ctx={} cbSrc={} outLen={} window={:#x}",
                 rc, context_handle, comp_len, out_len, ctx->window_size);
    return 1;
  }

  // Roll the freshly produced bytes into the history window for the next block.
  auto& hist = ctx->history;
  if (out_len >= ctx->window_size) {
    hist.assign(dst + (out_len - ctx->window_size), dst + out_len);
  } else {
    if (hist.size() + out_len > ctx->window_size) {
      hist.erase(hist.begin(),
                 hist.begin() + (hist.size() + out_len - ctx->window_size));
    }
    hist.insert(hist.end(), dst, dst + out_len);
  }

  return 0;  // success
}

// void LDIResetDecompression(void* context);
u32 LDIResetDecompression_entry(u32 context_handle) {
  std::lock_guard<std::mutex> lock(g_ldi_mutex);
  auto it = g_ldi_contexts.find(context_handle);
  if (it != g_ldi_contexts.end()) {
    it->second->history.clear();
  }
  return 0;
}

// void LDIDestroyDecompression(void* context);
u32 LDIDestroyDecompression_entry(u32 context_handle) {
  std::lock_guard<std::mutex> lock(g_ldi_mutex);
  g_ldi_contexts.erase(context_handle);
  return 0;
}

}  // namespace xboxkrnl
}  // namespace kernel
}  // namespace rex

REX_EXPORT(__imp__LDICreateDecompression, rex::kernel::xboxkrnl::LDICreateDecompression_entry)
REX_EXPORT(__imp__LDIDecompress, rex::kernel::xboxkrnl::LDIDecompress_entry)
REX_EXPORT(__imp__LDIResetDecompression, rex::kernel::xboxkrnl::LDIResetDecompression_entry)
REX_EXPORT(__imp__LDIDestroyDecompression, rex::kernel::xboxkrnl::LDIDestroyDecompression_entry)
