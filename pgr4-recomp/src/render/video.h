// render/video.h
//
// Phase 1 of the PGR4 native Plume renderer: device, swapchain and present
// only. No guest geometry is translated yet -- Present() clears the acquired
// swapchain image and flips it, which is enough to prove the present loop is
// driven correctly from the guest's own D3DDevice_Swap.
//
// Compiled only when PGR4_ENABLE_PLUME is ON. With it OFF the xenos GPU plugin
// path is untouched, because REX_HOOK replaces a recompiled function at link
// time and offers no fall-through to the original body -- so the choice of
// renderer has to be made at build time, not from a cvar.

#pragma once

#include <cstdint>

struct Video {
  // Backbuffer dimensions. These track the host window and are deliberately
  // unrelated to the guest's own 1280x720 frame.
  static inline uint32_t s_viewportWidth = 1280;
  static inline uint32_t s_viewportHeight = 720;

  // Create the Plume interface, device, queue and swapchain for a native
  // window handle. Returns false if no backend could be brought up, in which
  // case every other entry point below is inert and the game still runs (with
  // nothing presented).
  static bool Init(void* nativeWindowHandle, uint32_t width, uint32_t height);

  static bool IsInitialized();

  // Acquire, clear, submit and flip one frame. Driven from the guest's
  // D3DDevice_Swap (0x82695BE0). Returns false if the frame was dropped,
  // which is not an error -- a resize or a lost swapchain both land here.
  static bool Present();

  // Records the colour Present() clears to. Phase 2 replaces this with real
  // render-target tracking once resource hooks exist.
  static void SetClearColor(float r, float g, float b, float a);

  // Block until the GPU has drained all submitted work.
  static void WaitForGPU();

  static void Shutdown();
};
