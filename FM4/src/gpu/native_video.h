// Milestone 1 native video: a Plume D3D12 device that clears and presents.
// Draw emulation arrives in later milestones; this file must stay small.
#pragma once

#include <cstdint>

namespace rex::ui {
class Window;
}

namespace fm4::gpu {

// Set from Fm4App::OnPreSetup when fm4.toml selects gpu_plugin = "native".
void SetNativeRequested(bool on);
bool NativeRequested();

class Video {
 public:
  // Creates the Plume interface, device, queue and per-frame objects. The
  // swapchain is created here if the window already has a native handle and
  // otherwise on the first Present. Returns false if the device cannot be made.
  static bool Init(rex::ui::Window* window);
  static void Shutdown();

  // The next Present clears the back buffer to this D3DCOLOR (ARGB8).
  static void RequestClear(uint32_t argb);

  // Acquire a swapchain image, clear it, present. Safe to call from any guest
  // thread; no-op until Init succeeded.
  static void Present();

  static uint64_t PresentedFrames();
};

}  // namespace fm4::gpu
