// Native-path guest hooks, milestone 1. The real D3D library still builds and
// owns the guest D3DDevice and its ring; these hooks only make every GPU wait
// return immediately and route Swap to Plume. Every hook forwards to the
// original body when the native path is off, so the xenos plugin is
// unaffected.
#include "generated/default/fm4_init.h"

#include <cstdint>

#include <rex/logging.h>

#include "gpu/d3d_guest.h"
#include "gpu/native_video.h"

namespace {

bool Native() { return fm4::gpu::NativeRequested(); }

}  // namespace

// Direct3D_CreateDevice is owned by render/d3d_hooks.cpp, which latches the
// device for the ported renderer.

namespace fm4::render {
void OnResourceTraceFrame();
}

// GPU waits: nothing to wait for.
extern "C" REX_FUNC(D3DDevice_BlockUntilIdle) {
  if (!Native()) __imp__D3DDevice_BlockUntilIdle(ctx, base);
}

extern "C" REX_FUNC(D3D_WaitUntilIdleOrFlushCaches) {
  if (!Native()) __imp__D3D_WaitUntilIdleOrFlushCaches(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_BlockOnFence) {
  if (!Native()) __imp__D3DDevice_BlockOnFence(ctx, base);
}

extern "C" REX_FUNC(D3DResource_BlockUntilNotBusy) {
  if (!Native()) __imp__D3DResource_BlockUntilNotBusy(ctx, base);
}

extern "C" REX_FUNC(D3D_CDevice_BlockOnSecondaryPosition) {
  if (!Native()) __imp__D3D_CDevice_BlockOnSecondaryPosition(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_IsFencePending) {
  if (!Native()) {
    __imp__D3DDevice_IsFencePending(ctx, base);
    return;
  }
  ctx.r3.u64 = 0;
}

// D3DDevice_Swap(pDevice, pFrontBuffer, pParameters): present through Plume.
// The front buffer is ignored in milestone 1; the swapchain shows the clear.
extern "C" REX_FUNC(D3DDevice_Swap) {
  fm4::gpu::TraceOnSwap();
  // The library's own Swap advances the frame token and swap counters the game
  // polls; with the ring library-owned and every GPU wait neutralised it is safe
  // to run (VdSwap is ignored by the kernel without a GPU plugin). Present after.
  __imp__D3DDevice_Swap(ctx, base);
  if (Native()) {
    fm4::render::OnResourceTraceFrame();
    fm4::gpu::Video::Present();
  }
}

// D3D_RingMakeSpace(pDevice) @0x822FEF08 -- the counterpart to the GPU waits
// above, and required for the same reason. The library writes PM4 straight into
// the guest ring and only recycles a segment once the GPU has consumed it. With
// no GPU plugin nothing ever consumes it, so the refill
// (D3D::RingBufferDeviceAllocate via sub_82379388) eventually returns null; the
// library then parks m_pRing on an emergency scratch base and, when that is
// null too, every subsequent ring emitter stores to guest address 4. That is
// exactly the "write of guest 0x00000004" seen from D3D::SetSurfaceClip and
// D3DDevice_SetShaderGPRAllocation, on the *main* device.
//
// Under the native path we therefore never let make-space fail: rewind the ring
// to its own base every time. Overwriting unconsumed PM4 is harmless precisely
// because nothing consumes it. The bookkeeping below is copied from
// D3D_InitializeRingBuffer @0x826E4E28, which after allocating the ring does:
//     *(dev+15292 / 0x3BBC) = base          ring base
//     *(dev+15300 / 0x3BC4) = segmentSize
//     v16 = (segmentSize & ~3) + base
//     *(dev+48   / 0x30)   = base - 4       m_pRing
//     *(dev+52   / 0x34)   = v16            segment end
//     *(dev+56   / 0x38)   = v16 - 160      m_pRingLimit (160-byte guarantee)
//     *(dev+15304 / 0x3BC8) = 0
//     *(dev+15316 / 0x3BD4) = base          current segment base
// plus sub_82379388's success path, which also clears *(dev+15320 / 0x3BD8).
// The sticky "segment allocation failed" bit 0x20 at 0x2B3D is cleared so the
// library stops preferring its scratch buffer. sub_822FEF08 returns
// *(dev+0x30), so the hook returns the same value.
//
// ponytail: rewinds to the ring base unconditionally -- correct only while no
// GPU consumes the ring. Whoever lands real PM4 consumption must delete this.
extern "C" REX_FUNC(D3D_RingMakeSpace) {
  if (!Native()) {
    __imp__D3D_RingMakeSpace(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  if (device == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  const auto word = [&](uint32_t off) -> volatile uint32_t* {
    return reinterpret_cast<volatile uint32_t*>(base + device + off);
  };
  const auto load = [&](uint32_t off) { return __builtin_bswap32(*word(off)); };
  const auto store = [&](uint32_t off, uint32_t value) { *word(off) = __builtin_bswap32(value); };

  const uint32_t ringBase = load(0x3BBC);
  const uint32_t segmentSize = load(0x3BC4);
  if (ringBase == 0 || segmentSize == 0) {
    // Ring not initialised yet: leave the library's own state alone.
    ctx.r3.u64 = load(0x30);
    return;
  }
  const uint32_t segmentEnd = ringBase + (segmentSize & ~3u);
  store(0x3BD4, ringBase);
  store(0x3BC8, 0);
  store(0x3BD8, 0);
  store(0x30, ringBase - 4);
  store(0x34, segmentEnd);
  store(0x38, segmentEnd - 160);
  auto* flags = reinterpret_cast<volatile uint8_t*>(base + device + 0x2B3D);
  *flags = static_cast<uint8_t>(*flags & ~0x20);
  ctx.r3.u64 = ringBase - 4;
}

// D3D_InitializeEngines(pDevice) (ida40 sub_826F46E0) calls VdInitializeEngines and
// VdSetGraphicsInterruptCallback(D3D::InterruptCallback, pDevice). The kernel drops
// that registration when no GPU plugin is loaded, so the native path takes the
// device here and raises vblank interrupts itself.
extern "C" REX_FUNC(D3D_InitializeEngines) {
  const uint32_t device = ctx.r3.u32;
  __imp__D3D_InitializeEngines(ctx, base);
  if (Native()) {
    fm4::gpu::Video::OnGraphicsInterruptRegistered(device);
  }
}

// D3DDevice_Clear(pDevice, Count, pRects, Flags, Color, Z, Stencil, EDRAMClear):
// r6 = Flags, r7 = Color (Z rides in f1 with a reserved GPR slot after it).
// The library's own body still runs so its pending state stays consistent.
extern "C" REX_FUNC(D3DDevice_Clear) {
  if (Native() && (ctx.r6.u32 & 0x1u) != 0) {
    fm4::gpu::Video::RequestClear(ctx.r7.u32);
  }
  __imp__D3DDevice_Clear(ctx, base);
}
