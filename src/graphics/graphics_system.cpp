/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/graphics/graphics_system.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include <rex/cvar.h>
#include <rex/graphics/command_processor.h>
#include <rex/graphics/flags.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/logging.h>
#include <rex/stream.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/ui/graphics_provider.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>

REXCVAR_DEFINE_STRING(trace_gpu_prefix, "", "GPU", "GPU trace file prefix");

REXCVAR_DEFINE_BOOL(trace_gpu_stream, false, "GPU", "Enable GPU trace streaming");

REXCVAR_DEFINE_STRING(swap_post_effect, "none", "GPU", "Swap post effect: none, fxaa, fxaa_extreme")
    .allowed({"none", "fxaa", "fxaa_extreme"})
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(store_shaders, true, "GPU",
                    "Store shaders persistently and load them when loading games to avoid "
                    "runtime spikes and freezes when playing the game not for the first time.");

namespace {

rex::graphics::CommandProcessor::SwapPostEffect ParseSwapPostEffect(
    const std::string& effect_name) {
  std::string lowered = effect_name;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
    c = static_cast<unsigned char>(std::tolower(c));
    return c == '-' ? '_' : char(c);
  });
  if (lowered == "fxaa") {
    return rex::graphics::CommandProcessor::SwapPostEffect::kFxaa;
  }
  if (lowered == "fxaa_extreme" || lowered == "extreme") {
    return rex::graphics::CommandProcessor::SwapPostEffect::kFxaaExtreme;
  }
  return rex::graphics::CommandProcessor::SwapPostEffect::kNone;
}
}  // namespace

namespace rex::graphics {

// Nvidia Optimus/AMD PowerXpress support.
// These exports force the process to trigger the discrete GPU in multi-GPU
// systems.
// https://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
// https://stackoverflow.com/questions/17458803/amd-equivalent-to-nvoptimusenablement
#if REX_PLATFORM_WIN32
extern "C" {
__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
__declspec(dllexport) uint32_t AmdPowerXpressRequestHighPerformance = 1;
}  // extern "C"
#endif  // REX_PLATFORM_WIN32

GraphicsSystem::GraphicsSystem() : vsync_worker_running_(false) {}

GraphicsSystem::~GraphicsSystem() = default;

X_STATUS GraphicsSystem::SetupPresentation(ui::WindowedAppContext* app_context) {
  if (presenter_) {
    return X_STATUS_SUCCESS;
  }

  if (!provider_) {
    CreateProvider(true);
    if (!provider_) {
      REXGPU_ERROR("Unable to create graphics provider");
      return X_STATUS_UNSUCCESSFUL;
    }
    provider_supports_presentation_ = true;
  } else if (!provider_supports_presentation_) {
    // A prior SetupGuestGpu built a headless provider; backends like Vulkan
    // need swapchain support baked in at provider creation time.
    REXGPU_ERROR("SetupPresentation called after headless SetupGuestGpu; call order is reversed");
    return X_STATUS_UNSUCCESSFUL;
  }

  app_context_ = app_context;
  auto loss_cb = [this](bool is_responsible, bool statically_from_ui_thread) {
    OnHostGpuLossFromAnyThread(is_responsible);
  };
  if (app_context_) {
    // Presenter creation must happen on the UI thread.
    app_context_->CallInUIThreadSynchronous(
        [this, loss_cb]() { presenter_ = provider_->CreatePresenter(loss_cb); });
  } else {
    // Offscreen path (e.g. capturing guest output without a window).
    presenter_ = provider_->CreatePresenter(loss_cb);
  }

  if (!presenter_) {
    REXGPU_ERROR("Unable to create presenter");
    return X_STATUS_UNSUCCESSFUL;
  }
  return X_STATUS_SUCCESS;
}

X_STATUS GraphicsSystem::SetupGuestGpu(runtime::FunctionDispatcher* function_dispatcher,
                                       system::KernelState* kernel_state) {
  memory_ = function_dispatcher->memory();
  function_dispatcher_ = function_dispatcher;
  kernel_state_ = kernel_state;

  // Headless path: no one set up presentation, so build a no-presentation
  // provider just for the command processor.
  if (!provider_) {
    CreateProvider(false);
    provider_supports_presentation_ = false;
  }

  // Create command processor. This will spin up a thread to process all
  // incoming ringbuffer packets.
  command_processor_ = CreateCommandProcessor();
  if (!command_processor_->Initialize()) {
    REXGPU_ERROR("Unable to initialize command processor");
    return X_STATUS_UNSUCCESSFUL;
  }
  command_processor_->SetDesiredSwapPostEffect(ParseSwapPostEffect(REXCVAR_GET(swap_post_effect)));

  // Register GPU MMIO handlers
  // GPU registers are at 0x7FC80000-0x7FCFFFFF
  memory_->AddVirtualMappedRange(0x7FC80000,  // base address
                                 0xFFFF0000,  // mask
                                 0x0000FFFF,  // size (64KB)
                                 this,        // context (GraphicsSystem*)
                                 reinterpret_cast<runtime::MMIOReadCallback>(ReadRegisterThunk),
                                 reinterpret_cast<runtime::MMIOWriteCallback>(WriteRegisterThunk));
  REXGPU_INFO("GPU MMIO registered at 0x7FC80000, host=0x{:016X}", (uintptr_t)memory_->TranslateVirtual(0x7FC80000));

  // Guest vblank timer based on the configured guest video mode.
  vsync_worker_running_ = true;
  vsync_worker_thread_ = system::object_ref<system::XHostThread>(
      new system::XHostThread(kernel_state_, 128 * 1024, 0, [this]() {
        REXGPU_INFO("VSync worker thread started");
        system::X_VIDEO_MODE video_mode;
        kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
        double refresh_rate_hz = std::max(1.0, double(float(video_mode.refresh_rate)));
        REXGPU_INFO("VSync: refresh_rate={:.1f} Hz", refresh_rate_hz);
        uint64_t guest_tick_frequency = chrono::Clock::guest_tick_frequency();
        uint64_t vsync_interval_ticks =
            std::max(uint64_t(1), uint64_t(double(guest_tick_frequency) / refresh_rate_hz));
        uint64_t no_vsync_interval_ticks = std::max(uint64_t(1), guest_tick_frequency / 1000);
        uint64_t last_frame_time = chrono::Clock::QueryGuestTickCount();
        while (vsync_worker_running_) {
          uint64_t current_time = chrono::Clock::QueryGuestTickCount();
          uint64_t interval_ticks =
              REXCVAR_GET(vsync) ? vsync_interval_ticks : no_vsync_interval_ticks;
          while (current_time - last_frame_time >= interval_ticks) {
            MarkVblank();
            last_frame_time += interval_ticks;
          }
          rex::thread::Sleep(std::chrono::milliseconds(1));
        }
        return 0;
      }));
  // TODO: set_can_debugger_suspend not yet ported
  // vsync_worker_thread_->set_can_debugger_suspend(true);
  vsync_worker_thread_->set_name("GPU VSync");
  REXGPU_INFO("Creating VSync worker thread...");
  vsync_worker_thread_->Create();
  REXGPU_INFO("VSync worker thread created");

  if (REXCVAR_GET(trace_gpu_stream)) {
    BeginTracing();
  }

  return X_STATUS_SUCCESS;
}

void GraphicsSystem::Shutdown() {
  if (command_processor_) {
    EndTracing();
    command_processor_->Shutdown();
    command_processor_.reset();
  }

  if (vsync_worker_thread_) {
    vsync_worker_running_ = false;
    vsync_worker_thread_->Wait(0, 0, 0, nullptr);
    vsync_worker_thread_.reset();
  }

  if (presenter_) {
    if (app_context_) {
      app_context_->CallInUIThreadSynchronous([this]() { presenter_.reset(); });
    }
    // If there's no app context (thus the presenter is owned by the thread that
    // initialized the GraphicsSystem) or can't be queueing UI thread calls
    // anymore, shutdown anyway.
    presenter_.reset();
  }

  provider_.reset();
}

void GraphicsSystem::OnHostGpuLossFromAnyThread([[maybe_unused]] bool is_responsible) {
  // TODO(Triang3l): Somehow gain exclusive ownership of the Provider (may be
  // used by the command processor, the presenter, and possibly anything else,
  // it's considered free-threaded, except for lifetime management which will be
  // involved in this case) and reset it so a new host GPU API device is
  // created. Then ask the command processor to reset itself in its thread, and
  // ask the UI thread to reset the Presenter (the UI thread manages its
  // lifetime - but if there's no WindowedAppContext, either don't reset it as
  // in this case there's no user who needs uninterrupted gameplay, or somehow
  // protect it with a mutex so any thread can be considered a UI thread and
  // reset).
  if (host_gpu_loss_reported_.test_and_set(std::memory_order_relaxed)) {
    return;
  }
  rex::FatalError("Graphics device lost (probably due to an internal error)");
}

uint32_t GraphicsSystem::ReadRegisterThunk(void* ppc_context, GraphicsSystem* gs, uint32_t addr) {
  return gs->ReadRegister(addr);
}

void GraphicsSystem::WriteRegisterThunk(void* ppc_context, GraphicsSystem* gs, uint32_t addr,
                                        uint32_t value) {
  gs->WriteRegister(addr, value);
}

uint32_t GraphicsSystem::ReadRegister(uint32_t addr) {
  uint32_t r = (addr & 0xFFFF) / 4;

  switch (r) {
    case 0x0F00:  // RB_EDRAM_TIMING
      return 0x08100748;
    case 0x0F01:  // RB_BC_CONTROL
      return 0x0000200E;
    case 0x194C: {  // R500_D1MODE_V_COUNTER
      system::X_VIDEO_MODE video_mode;
      kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
      return std::min(uint32_t(video_mode.display_height), uint32_t(0x0FFF));
    }
    case 0x1951:    // interrupt status
      return 1;     // vblank
    case 0x1961: {  // AVIVO_D1MODE_VIEWPORT_SIZE
      // Maximum [width(0x0FFF), height(0x0FFF)].
      system::X_VIDEO_MODE video_mode;
      kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
      uint32_t viewport_width = std::min(uint32_t(video_mode.display_width), uint32_t(0x0FFF));
      uint32_t viewport_height = std::min(uint32_t(video_mode.display_height), uint32_t(0x0FFF));
      return (viewport_width << 16) | viewport_height;
    }
    default:
      if (!register_file_.GetRegisterInfo(r)) {
        REXGPU_DEBUG("GPU: Read from unknown register ({:04X})", r);
      }
  }

  assert_true(r < RegisterFile::kRegisterCount);
  return register_file_.values[r];
}

void GraphicsSystem::WriteRegister(uint32_t addr, uint32_t value) {
  uint32_t r = (addr & 0xFFFF) / 4;
  REXGPU_DEBUG("WriteRegister: addr=0x{:08X}, r=0x{:04X}, value=0x{:08X}", addr, r, value);

  switch (r) {
    case 0x01C5:  // CP_RB_WPTR
      fprintf(stderr, "[gpu] CP_RB_WPTR write: 0x%08X\n", value); fflush(stderr);
      command_processor_->UpdateWritePointer(value);
      break;
    case 0x1844:  // AVIVO_D1GRPH_PRIMARY_SURFACE_ADDRESS
      break;
    default:
      REXGPU_WARN("Unknown GPU register {:04X} write: {:08X}", r, value);
      break;
  }

  assert_true(r < RegisterFile::kRegisterCount);
  register_file_.values[r] = value;
}

void GraphicsSystem::InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) {
  REXGPU_INFO("InitializeRingBuffer: ptr=0x{:08X}, size_log2={}", ptr, size_log2);
  command_processor_->InitializeRingBuffer(ptr, size_log2);
}

void GraphicsSystem::EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) {
  command_processor_->EnableReadPointerWriteBack(ptr, block_size_log2);
}

void GraphicsSystem::SetInterruptCallback(uint32_t callback, uint32_t user_data) {
  interrupt_callback_ = callback;
  interrupt_callback_data_ = user_data;
  REXGPU_INFO("SetInterruptCallback({:08X}, {:08X})", callback, user_data);
}

void GraphicsSystem::DispatchInterruptCallback(uint32_t source, uint32_t cpu) {
  if (!interrupt_callback_) {
    return;
  }

  auto thread = system::XThread::GetCurrentThread();
  assert_not_null(thread);

  if (cpu == 0xFFFFFFFF) {
    cpu = 2;
  }
  thread->SetActiveCpu(cpu);

  static int dispatch_log_count = 0;
  if (dispatch_log_count < 20) {
    dispatch_log_count++;
    REXGPU_INFO("Dispatching GPU interrupt at {:08X} w/ source {} on cpu {} (dispatch #{})", 
                interrupt_callback_, source, cpu, dispatch_log_count);
  }

  uint64_t args[] = {source, interrupt_callback_data_};

  if (source == 0) {
    uint32_t user_data = static_cast<uint32_t>(interrupt_callback_data_);
    auto ud_host = memory_->TranslateVirtual(user_data);
    uint32_t vblank_ctr = memory::load_and_swap<uint32_t>(ud_host + 15592);
    uint32_t frame_ctr = memory::load_and_swap<uint32_t>(ud_host + 15584);
    uint32_t notify_fn = memory::load_and_swap<uint32_t>(ud_host + 15580);
    static int src0_log_count = 0;
    if (src0_log_count < 20) {
      src0_log_count++;
      REXGPU_INFO("source=0: vblank_ctr={} frame_ctr={} notify_fn=0x{:08X}",
                   vblank_ctr, frame_ctr, notify_fn);
    }
  }

  // Ring buffer control structure at [context+10384] has 3 key fields:
  //   [ctrl+0]  = submit counter shadow (game writes via sub_82377BD8)
  //   [ctrl+4]  = WPTR shadow = (counter & 3) | write_position
  //   [ctrl+60] = hardware RPTR writeback (GPU writes here)
  // On real Xbox 360, the GPU writes RPTR to [ctrl+60] via writeback, and the
  // game updates [ctrl+0] and [ctrl+4] during submit (sub_82377BD8).  The submit
  // function only writes these when bit 2 of [context+10433] is set; otherwise
  // the game relies on the wait-timeout handler (sub_82386130) to set that bit.
  // Since we process GPU commands synchronously, we emulate all three updates
  // here to keep the D3D tracking consistent.
   static int cp_check_count = 0;
   if (cp_check_count < 3) {
     cp_check_count++;
     REXGPU_INFO("  CP check: command_processor_={} data=0x{:08X}",
                 (void*)command_processor_.get(), interrupt_callback_data_);
   }
   if (command_processor_) {
     uint32_t user_data = static_cast<uint32_t>(interrupt_callback_data_);
     uint32_t rb_ctrl_addr = user_data + 10384;
    auto rb_ctrl_host = memory_->TranslateVirtual(rb_ctrl_addr);
    uint32_t rb_ctrl = memory::load_and_swap<uint32_t>(rb_ctrl_host);
    static int rb_diag_count = 0;
    if (rb_diag_count < 5) {
      rb_diag_count++;
      REXGPU_INFO("  rb_ctrl check: user_data=0x{:08X} rb_ctrl=0x{:08X} rptr={}",
                  user_data, rb_ctrl, command_processor_->read_index());
    }
    if (rb_ctrl != 0) {
      auto sub_ctr_host = memory_->TranslateVirtual(user_data + 10396);
      uint32_t sub_ctr = memory::load_and_swap<uint32_t>(sub_ctr_host);
      auto shadow0_host = memory_->TranslateVirtual(rb_ctrl);
      uint32_t shadow0 = sub_ctr >= 2 ? sub_ctr - 2 : 0;
      memory::store_and_swap<uint32_t>(shadow0_host, shadow0);
      auto counter_host = memory_->TranslateVirtual(user_data + 13988);
      uint32_t counter = memory::load_and_swap<uint32_t>(counter_host);
      auto wptr_host = memory_->TranslateVirtual(user_data);
      uint32_t wptr = memory::load_and_swap<uint32_t>(wptr_host);
      uint32_t wptr_shadow = (counter & 0x3) | wptr;
      auto shadow4_host = memory_->TranslateVirtual(rb_ctrl + 4);
      memory::store_and_swap<uint32_t>(shadow4_host, wptr_shadow);

      // Clear CPU_INTERRUPT flag (bit 1) set by RPTR wait timeout.
      // On real Xbox 360, the GPU hardware processes fast enough. In our
      // emulator, the software GPU thread may be too slow, causing the
      // timeout handler to set this flag. Clear it to prevent the error.
      auto flags_host = memory_->TranslateVirtual(user_data + 10433);
      uint8_t flags = memory::load_and_swap<uint8_t>(flags_host);
      if (flags & 0x2) {
        memory::store_and_swap<uint8_t>(flags_host, flags & ~0x2);
        static int flag_clear_count = 0;
        if (flag_clear_count < 20) {
          flag_clear_count++;
          REXGPU_INFO("  Cleared CPU_INTERRUPT flag (was 0x{:02X})", flags);
        }
      }

      static int rb_log_count = 0;
      if (rb_log_count < 20) {
        rb_log_count++;
        uint32_t current_rptr = command_processor_->read_index();
        REXGPU_INFO("  RB update: ctrl+0={}(sc={}) ctrl+4=0x{:08X}(ctr={} wp=0x{:08X}) "
                    "ctrl+60={} rptr={}",
                    shadow0, sub_ctr, wptr_shadow, counter, wptr,
                    current_rptr, current_rptr);
      }
      auto cmd_buf_host = memory_->TranslateVirtual(user_data + 10388);
      uint32_t cmd_buf = memory::load_and_swap<uint32_t>(cmd_buf_host);
      if (cmd_buf != 0) {
        auto marker_host = memory_->TranslateVirtual(cmd_buf + 16);
        uint32_t marker = memory::load_and_swap<uint32_t>(marker_host);
        static int marker_check_count = 0;
        if (marker_check_count < 200) {
          marker_check_count++;
          REXGPU_INFO("  Marker check [0x{:08X}+16] = 0x{:08X}{}", cmd_buf, marker,
                      marker == 0x0BADF00D ? " (CLEARING)" : "");
        }
        if (marker == 0x0BADF00D) {
          memory::store_and_swap<uint32_t>(marker_host, 0);
        }
      }
    }
  }

  function_dispatcher_->ExecuteInterrupt(thread->thread_state(), interrupt_callback_, args,
                                         rex::countof(args));

  if (command_processor_) {
    uint32_t user_data = static_cast<uint32_t>(interrupt_callback_data_);
    auto ud_host = memory_->TranslateVirtual(user_data);
    if (source == 1) {
      auto cmd_buf_host = memory_->TranslateVirtual(user_data + 10388);
      uint32_t cmd_buf = memory::load_and_swap<uint32_t>(cmd_buf_host);
      if (cmd_buf != 0) {
        auto marker_host = memory_->TranslateVirtual(cmd_buf + 16);
        uint32_t marker = memory::load_and_swap<uint32_t>(marker_host);
        if (marker == 0x0BADF00D) {
          memory::store_and_swap<uint32_t>(marker_host, 0);
        }
      }
    }
    auto flags_host = memory_->TranslateVirtual(user_data + 10433);
    uint8_t flags = memory::load_and_swap<uint8_t>(flags_host);
    if (flags & 0x2) {
      memory::store_and_swap<uint8_t>(flags_host, flags & ~uint8_t(0x2));
    }
  }
}

void GraphicsSystem::MarkVblank() {
  if (command_processor_) {
    command_processor_->increment_counter();
    command_processor_->RequestDeferredInterrupt();
  }

  static int vblank_log_count = 0;
  if (vblank_log_count < 50) {
    vblank_log_count++;
    REXGPU_INFO("MarkVblank #{}: interrupt_callback_=0x{:08X}", vblank_log_count, interrupt_callback_);
  }

  DispatchInterruptCallback(0, 2);
}

void GraphicsSystem::SynchronizeRingBuffer() {
  if (!interrupt_callback_data_ || !command_processor_) return;
  uint32_t user_data = interrupt_callback_data_;
  auto rb_ctrl_host = memory_->TranslateVirtual(user_data + 10384);
  uint32_t rb_ctrl = memory::load_and_swap<uint32_t>(rb_ctrl_host);
  if (rb_ctrl == 0) return;
  auto sub_ctr_host = memory_->TranslateVirtual(user_data + 10396);
  uint32_t sub_ctr = memory::load_and_swap<uint32_t>(sub_ctr_host);
  auto shadow0_host = memory_->TranslateVirtual(rb_ctrl);
  uint32_t old_shadow0 = memory::load_and_swap<uint32_t>(shadow0_host);
  uint32_t shadow0 = sub_ctr >= 2 ? sub_ctr - 2 : 0;
  memory::store_and_swap<uint32_t>(shadow0_host, shadow0);
  auto counter_host = memory_->TranslateVirtual(user_data + 13988);
  uint32_t counter = memory::load_and_swap<uint32_t>(counter_host);
  auto wptr_host = memory_->TranslateVirtual(user_data);
  uint32_t wptr = memory::load_and_swap<uint32_t>(wptr_host);
  uint32_t wptr_shadow = (counter & 0x3) | wptr;
  auto shadow4_host = memory_->TranslateVirtual(rb_ctrl + 4);
  memory::store_and_swap<uint32_t>(shadow4_host, wptr_shadow);
  auto flags_host = memory_->TranslateVirtual(user_data + 10433);
  uint8_t flags = memory::load_and_swap<uint8_t>(flags_host);
  if (flags & 0x2) {
    memory::store_and_swap<uint8_t>(flags_host, flags & ~0x2);
  }
  static int sync_log_count = 0;
  if (sync_log_count < 50) {
    sync_log_count++;
    REXGPU_INFO("SyncRB: ctrl+0 {}->{} sc={} wp=0x{:08X} flags=0x{:02X}",
                old_shadow0, shadow0, sub_ctr, wptr, flags);
  }
}

void GraphicsSystem::ClearCaches() {
  command_processor_->CallInThread([&]() { command_processor_->ClearCaches(); });
}

void GraphicsSystem::InvalidateGpuMemory() {
  command_processor_->CallInThread([&]() { command_processor_->InvalidateGpuMemory(); });
}

void GraphicsSystem::InitializeShaderStorage(const std::filesystem::path& cache_root,
                                             uint32_t title_id, bool blocking) {
  if (!REXCVAR_GET(store_shaders)) {
    return;
  }
  if (blocking) {
    if (command_processor_->is_paused()) {
      // Safe to run on any thread while the command processor is paused, no
      // race condition.
      command_processor_->InitializeShaderStorage(cache_root, title_id, true);
    } else {
      rex::thread::Fence fence;
      command_processor_->CallInThread([this, cache_root, title_id, &fence]() {
        command_processor_->InitializeShaderStorage(cache_root, title_id, true);
        fence.Signal();
      });
      fence.Wait();
    }
  } else {
    command_processor_->CallInThread([this, cache_root, title_id]() {
      command_processor_->InitializeShaderStorage(cache_root, title_id, false);
    });
  }
}

void GraphicsSystem::RequestFrameTrace() {
  command_processor_->RequestFrameTrace(REXCVAR_GET(trace_gpu_prefix));
}

void GraphicsSystem::BeginTracing() {
  command_processor_->BeginTracing(REXCVAR_GET(trace_gpu_prefix));
}

void GraphicsSystem::EndTracing() {
  command_processor_->EndTracing();
}

void GraphicsSystem::Pause() {
  paused_ = true;
  command_processor_->Pause();
}

void GraphicsSystem::Resume() {
  paused_ = false;
  command_processor_->Resume();
}

bool GraphicsSystem::Save(::rex::stream::ByteStream* stream) {
  stream->Write<uint32_t>(interrupt_callback_);
  stream->Write<uint32_t>(interrupt_callback_data_);
  return command_processor_->Save(stream);
}

bool GraphicsSystem::Restore(::rex::stream::ByteStream* stream) {
  interrupt_callback_ = stream->Read<uint32_t>();
  interrupt_callback_data_ = stream->Read<uint32_t>();
  return command_processor_->Restore(stream);
}

}  // namespace rex::graphics
