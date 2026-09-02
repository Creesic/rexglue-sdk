// Native-path guest hooks, milestone 1. The real D3D library still builds and
// owns the guest D3DDevice and its ring; these hooks only make every GPU wait
// return immediately and route Swap to Plume. Every hook forwards to the
// original body when the native path is off, so the xenos plugin is
// unaffected.
#include "generated/default/fm4_init.h"

#include <rex/logging.h>

#include "gpu/d3d_guest.h"
#include "gpu/native_video.h"

namespace {

bool Native() { return fm4::gpu::NativeRequested(); }

uint32_t GuestLoad32(uint8_t* base, uint32_t va) {
  return __builtin_bswap32(*reinterpret_cast<volatile uint32_t*>(base + va));
}

}  // namespace

// Let the library build the device; nothing else to do under native.
extern "C" REX_FUNC(Direct3D_CreateDevice) {
  const uint32_t out_device_va = ctx.r8.u32;  // read before the call clobbers r8
  __imp__Direct3D_CreateDevice(ctx, base);
  if (!Native() || ctx.r3.u32 != 0) {
    return;
  }
  const uint32_t device = GuestLoad32(base, out_device_va);
  REXLOG_INFO("native gpu: guest D3DDevice at 0x{:08X}", device);
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
  if (!Native()) {
    __imp__D3DDevice_Swap(ctx, base);
    return;
  }
  fm4::gpu::Video::Present();
  ctx.r3.u64 = 0;
}
