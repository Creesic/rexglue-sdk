// render/d3d_hooks.cpp
//
// Guest D3D entry points replaced by the native Plume renderer.
//
// Phase 1 hooks exactly one function: D3DDevice_Swap (0x82695BE0). IDA
// identifies it as the sole caller of the VdSwap import, and it is the same
// XDK library routine FM2 presents from -- 0x594 bytes here against 0x598
// there. Hooking Swap rather than VdSwap itself puts the present trigger at
// the API boundary instead of inside the ring-buffer plumbing.
//
// REX_HOOK is a link-time full replacement with no fall-through to the
// recompiled body, so once this TU is compiled the guest's own swap path is
// gone. That is why the renderer sits behind the PGR4_ENABLE_PLUME build
// option rather than a runtime cvar: with the option OFF this file is not
// compiled at all and the xenos plugin path is untouched.

#include <rex/hook.h>

#include "video.h"

namespace {

// D3DDevice_Swap(CDevice* device, void* frontBuffer,
//                const D3DVIDEO_SCALER_PARAMETERS* scalerParams)
//
// Phase 1 ignores all three: no guest resource translation exists yet, so
// there is nothing to present but a cleared image. The parameters are named
// for the real signature so Phase 2 has the shape to work from.
void SwapHook(u32 device_ptr, u32 front_buffer_ptr, u32 scaler_params_ptr) {
  (void)device_ptr;
  (void)front_buffer_ptr;
  (void)scaler_params_ptr;

  // Init failed, or the renderer was never brought up. Returning leaves the
  // guest running with nothing on screen, which beats taking the game down.
  if (!Video::IsInitialized()) {
    return;
  }

  Video::Present();
}

}  // namespace

REX_HOOK(D3DDevice_Swap, SwapHook)
