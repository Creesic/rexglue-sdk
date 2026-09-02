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

  // The guest registered D3D::InterruptCallback for this device (hooked at
  // D3D_InitializeEngines). Without a GPU plugin nothing raises graphics
  // interrupts, and the game loop gates on the vblank they deliver, so this maps
  // a stub GPU MMIO window (the callback reads interrupt status there) and starts
  // a host vsync thread that calls the callback with source 0 at 60 Hz.
  static void OnGraphicsInterruptRegistered(uint32_t device_va);

  // The next Present clears the back buffer to this D3DCOLOR (ARGB8).
  static void RequestClear(uint32_t argb);

  // Present one frame through the ported renderer (render/video.cpp). Safe to
  // call from any guest thread; a Present already in flight is coalesced away
  // by ::Video::Present's own busy latch.
  static void Present();

  static uint64_t PresentedFrames();
};

}  // namespace fm4::gpu
