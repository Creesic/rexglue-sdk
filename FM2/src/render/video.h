// render/video.h

#pragma once

#include <cstdint>

struct Video {
  // Host viewport dimensions (backbuffer size).
  static inline uint32_t s_viewportWidth = 1280;
  static inline uint32_t s_viewportHeight = 720;

  // Create the Plume interface/device/queue/swapchain for the given native
  // window. Safe to call once; returns false if no backend could be created.
  static bool Init(void* nativeWindowHandle, uint32_t width, uint32_t height);

  // Has Init() succeeded?
  static bool IsInitialized();

  // Acquire the next backbuffer, clear it, and present.
  static void Present();

  // Record the guest's most recent D3DDevice_ClearF color, used as the
  // present-time clear color until real render-target tracking (resource
  // hooks) lands.
  static void SetFallbackClearColor(float r, float g, float b, float a);

  // Latch the calling thread as the sole present owner (first caller wins).
  // FM2 drives present from a many-thread job-system pool, all racing on the
  // single global command list; once an owner is claimed, Present() from any
  // other thread is dropped. Call from the real GPU-submit path so the owner
  // is the authoritative render thread.
  static void ClaimPresentOwner();

  // Block until the GPU has finished all submitted work.
  static void WaitForGPU();

  // Release all host GPU resources.
  static void Shutdown();
};
