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
// The guest registers a graphics interrupt callback and relies on it to advance
// its frame loop. With the callback dropped on the floor the loop never turns,
// so D3DDevice_Swap is never reached and the present hook never fires.
//
// RuntimeConfig::graphics is the SDK's extension point for exactly this: ReXApp
// only loads a GPU plugin when that field is still null, so supplying our own
// keeps the Vd* surface alive without bringing xenos back.
//
// This deliberately does NOT emulate a GPU. It owns two things:
//
//   1. The vblank. A worker fires the guest's interrupt callback at display
//      refresh so its frame loop advances.
//
//   2. Ring-buffer *consumption*, without execution. Until every ring-writing
//      D3D entry point is hooked, the guest's own D3D library still emits PM4
//      into the ring and blocks in its allocator when the ring is full. Nothing
//      here executes those packets; we simply report the ring as fully consumed
//      by mirroring the write pointer back as the read pointer, exactly where
//      the real command processor would write it after executing. Without this
//      the guest stalls in BeginRingBig long before it can reach a swap.

#pragma once

#include <atomic>
#include <cstdint>

#include <rex/system/interfaces/graphics.h>
#include <rex/system/xthread.h>

namespace rex::memory {
class Memory;
}

namespace pgr4::render {

class Pgr4GraphicsSystem final : public rex::system::IGraphicsSystem {
 public:
  Pgr4GraphicsSystem() = default;
  ~Pgr4GraphicsSystem() override;

  // Video::Init owns the real swapchain, so there is no provider or presenter
  // to build here. Reports presentation as available so ReXApp does not try to
  // stand up a headless path instead.
  rex::X_STATUS SetupPresentation(rex::ui::WindowedAppContext* app_context) override;

  // Maps the GPU register window and starts the vblank worker. Needs the
  // dispatcher and kernel state because the guest callback has to run on a
  // guest thread.
  rex::X_STATUS SetupGuestGpu(rex::runtime::FunctionDispatcher* function_dispatcher,
                              rex::system::KernelState* kernel_state) override;

  bool has_presentation() const override { return has_presentation_; }

  void SetInterruptCallback(uint32_t callback, uint32_t user_data) override;
  void InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) override;
  void EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) override;

  void Shutdown() override;

 private:
  // GPU register window (0x7FC80000). The guest's D3D publishes its ring write
  // pointer here (CP_RB_WPTR) and polls status registers during init; both must
  // be answered or it never finishes bringing the device up.
  static uint32_t ReadRegisterThunk(void* ppc_context, void* self, uint32_t addr);
  static void WriteRegisterThunk(void* ppc_context, void* self, uint32_t addr, uint32_t value);
  uint32_t ReadRegister(uint32_t addr);
  void WriteRegister(uint32_t addr, uint32_t value);

  // Reports the ring as consumed up to the current write pointer.
  void PublishReadPointer();

  void WorkerMain();

  rex::runtime::FunctionDispatcher* function_dispatcher_ = nullptr;
  rex::memory::Memory* memory_ = nullptr;
  rex::system::object_ref<rex::system::XHostThread> worker_thread_;

  // Written by the guest thread that calls the Vd* export and read by the
  // worker, so atomic rather than plain uint32_t.
  std::atomic<uint32_t> interrupt_callback_{0};
  std::atomic<uint32_t> interrupt_user_data_{0};

  // Ring state. write_ptr_index_ is whatever the guest last stored to
  // CP_RB_WPTR; read_ptr_writeback_ is the physical address VdEnableRingBuffer-
  // RPtrWriteBack asked us to publish the read pointer to (0 until then).
  std::atomic<uint32_t> write_ptr_index_{0};
  std::atomic<uint32_t> read_ptr_writeback_{0};

  std::atomic<bool> worker_running_{false};
  bool has_presentation_ = false;
  bool mmio_mapped_ = false;
};

}  // namespace pgr4::render
