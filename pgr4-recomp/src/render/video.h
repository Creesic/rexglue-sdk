// render/video.h

#pragma once

#include <cstdint>

namespace pgr4::render {
struct GuestBaseTexture;
}

struct Video {
  // Host viewport dimensions (backbuffer size).
  static inline uint32_t s_viewportWidth = 1280;
  static inline uint32_t s_viewportHeight = 720;

  // Create the Plume interface/device/queue/swapchain for the given native
  // window. Safe to call once; returns false if no backend could be created.
  static bool Init(void* nativeWindowHandle, uint32_t width, uint32_t height);

  // Has Init() succeeded?
  static bool IsInitialized();

  // Acquire the present gate, publish this frame's frontbuffer, and present.
  // Returns false when the frame was dropped by the existing busy coalescer.
  static bool Present(pgr4::render::GuestBaseTexture* frontBuffer);

  // Record the guest's most recent D3DDevice_ClearF color, used as the
  // present-time clear color until real render-target tracking (resource
  // hooks) lands.
  static void SetFallbackClearColor(float r, float g, float b, float a);

  // Block until the GPU has finished all submitted work.
  static void WaitForGPU();

  // Release all host GPU resources.
  static void Shutdown();
};
