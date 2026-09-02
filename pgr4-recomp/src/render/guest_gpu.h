// render/guest_gpu.h
//
// Minimal IGraphicsSystem for the native Plume renderer.
//
// Running with gpu_plugin cleared leaves KernelState::graphics_system() null,
// and the xboxkrnl Vd* exports bail out early when it is -- observed live as:
//
//   VdSetGraphicsInterruptCallback: no GPU emulation loaded; call ignored
//   VdInitializeRingBuffer:         no GPU emulation loaded; call ignored
//
// The guest registers a graphics interrupt callback and then relies on it to
// advance its frame loop. With the callback dropped on the floor the loop never
// turns, so D3DDevice_Swap is never reached and the present hook never fires.
//
// RuntimeConfig::graphics is the SDK's extension point for exactly this: ReXApp
// only loads a GPU plugin when that field is still null, so supplying our own
// keeps the Vd* surface alive without bringing xenos back.
//
// This deliberately does NOT emulate a GPU. The ring buffer is ignored -- the
// native renderer intercepts D3D at the API level, so nothing consumes PM4.
// The only real job here is owning the guest's vsync interrupt.

#pragma once

#include <atomic>
#include <cstdint>

#include <rex/system/interfaces/graphics.h>
#include <rex/system/xthread.h>

namespace pgr4::render {

class Pgr4GraphicsSystem final : public rex::system::IGraphicsSystem {
 public:
  Pgr4GraphicsSystem() = default;
  ~Pgr4GraphicsSystem() override;

  // Video::Init owns the real swapchain, so there is no provider or presenter
  // to build here. Reports presentation as available so ReXApp does not try to
  // stand up a headless path instead.
  rex::X_STATUS SetupPresentation(rex::ui::WindowedAppContext* app_context) override;

  // Starts the vsync worker. Needs the dispatcher and kernel state because the
  // guest callback has to run on a guest thread.
  rex::X_STATUS SetupGuestGpu(rex::runtime::FunctionDispatcher* function_dispatcher,
                         rex::system::KernelState* kernel_state) override;

  bool has_presentation() const override { return has_presentation_; }

  void SetInterruptCallback(uint32_t callback, uint32_t user_data) override;

  // The native renderer never reads the ring buffer; accept and ignore these so
  // the guest's D3D init completes instead of warning on every call.
  void InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) override;
  void EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) override;

  void Shutdown() override;

 private:
  void WorkerMain();

  rex::runtime::FunctionDispatcher* function_dispatcher_ = nullptr;
  rex::system::object_ref<rex::system::XHostThread> worker_thread_;

  // Written by the guest thread calling VdSetGraphicsInterruptCallback and read
  // by the worker, so atomic rather than plain uint32_t.
  std::atomic<uint32_t> interrupt_callback_{0};
  std::atomic<uint32_t> interrupt_user_data_{0};

  std::atomic<bool> worker_running_{false};
  bool has_presentation_ = false;
};

}  // namespace pgr4::render
