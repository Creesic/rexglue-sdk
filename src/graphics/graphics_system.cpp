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
#include <cstdio>

#include <rex/cvar.h>
#include <rex/graphics/command_processor.h>
#include <rex/graphics/flags.h>
#include <rex/kernel/xboxkrnl/threading.h>
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

void SyncGuestRingBufferState(rex::memory::Memory* memory, uint32_t user_data) {
  if (!memory || !user_data) {
    return;
  }

  auto rb_ctrl_host = memory->TranslateVirtual(user_data + 10384);
  if (!rb_ctrl_host) {
    return;
  }
  uint32_t rb_ctrl = rex::memory::load_and_swap<uint32_t>(rb_ctrl_host);
  if (!rb_ctrl) {
    return;
  }

  auto sub_ctr_host = memory->TranslateVirtual(user_data + 10396);
  auto shadow0_host = memory->TranslateVirtual(rb_ctrl);
  auto counter_host = memory->TranslateVirtual(user_data + 13988);
  auto wptr_host = memory->TranslateVirtual(user_data);
  auto shadow4_host = memory->TranslateVirtual(rb_ctrl + 4);
  auto flags_host = memory->TranslateVirtual(user_data + 10433);
  if (!sub_ctr_host || !shadow0_host || !counter_host || !wptr_host || !shadow4_host ||
      !flags_host) {
    return;
  }

  uint32_t sub_ctr = rex::memory::load_and_swap<uint32_t>(sub_ctr_host);
  uint32_t shadow0 = sub_ctr >= 2 ? sub_ctr - 2 : 0;
  uint32_t counter = rex::memory::load_and_swap<uint32_t>(counter_host);
  uint32_t wptr = rex::memory::load_and_swap<uint32_t>(wptr_host);
  uint32_t wptr_shadow = (counter & 0x3) | wptr;

  std::atomic_thread_fence(std::memory_order_release);
  rex::memory::store_and_swap<uint32_t>(shadow0_host, shadow0);
  rex::memory::store_and_swap<uint32_t>(shadow4_host, wptr_shadow);

  uint8_t flags = rex::memory::load_and_swap<uint8_t>(flags_host);
  if (flags & 0x2) {
    rex::memory::store_and_swap<uint8_t>(flags_host, flags & ~uint8_t(0x2));
  }

  auto cmd_buf_host = memory->TranslateVirtual(user_data + 10388);
  if (!cmd_buf_host) {
    return;
  }
  uint32_t cmd_buf = rex::memory::load_and_swap<uint32_t>(cmd_buf_host);
  if (!cmd_buf) {
    return;
  }
  auto marker_host = memory->TranslateVirtual(cmd_buf + 16);
  if (!marker_host) {
    return;
  }
  uint32_t marker = rex::memory::load_and_swap<uint32_t>(marker_host);
  if (marker == 0x0BADF00D) {
    std::atomic_thread_fence(std::memory_order_release);
    rex::memory::store_and_swap<uint32_t>(marker_host, 0);
    std::atomic_thread_fence(std::memory_order_release);
  }
}

struct InterruptControlSnapshot {
  bool valid = false;
  uint32_t rb_ctrl = 0;
  uint32_t shadow0 = 0;
  uint32_t shadow4 = 0;
  uint32_t sub_ctr = 0;
  uint32_t counter = 0;
  uint32_t wptr = 0;
  uint8_t flags = 0;
  uint32_t cmd_buf = 0;
  uint32_t marker = 0;
};

InterruptControlSnapshot ReadInterruptControlSnapshot(rex::memory::Memory* memory,
                                                     uint32_t user_data) {
  InterruptControlSnapshot snapshot;
  static uint32_t snapshot_fail_probe_count = 0;
  if (!memory || !user_data) {
    if (snapshot_fail_probe_count < 16) {
      ++snapshot_fail_probe_count;
      std::fprintf(stderr,
                   "[probe] ReadInterruptControlSnapshot invalid memory=%p user=%08X\n",
                   static_cast<void*>(memory), user_data);
      std::fflush(stderr);
    }
    return snapshot;
  }

  auto rb_ctrl_host = memory->TranslateVirtual(user_data + 10384);
  auto sub_ctr_host = memory->TranslateVirtual(user_data + 10396);
  auto counter_host = memory->TranslateVirtual(user_data + 13988);
  auto wptr_host = memory->TranslateVirtual(user_data);
  auto flags_host = memory->TranslateVirtual(user_data + 10433);
  auto cmd_buf_host = memory->TranslateVirtual(user_data + 10388);
  if (!rb_ctrl_host || !sub_ctr_host || !counter_host || !wptr_host || !flags_host ||
      !cmd_buf_host) {
    if (snapshot_fail_probe_count < 16) {
      ++snapshot_fail_probe_count;
      std::fprintf(stderr,
                   "[probe] ReadInterruptControlSnapshot translate fail user=%08X rb_ctrl=%p "
                   "sub=%p ctr=%p wptr=%p flags=%p cmd=%p\n",
                   user_data, rb_ctrl_host, sub_ctr_host, counter_host, wptr_host, flags_host,
                   cmd_buf_host);
      std::fflush(stderr);
    }
    return snapshot;
  }

  snapshot.rb_ctrl = rex::memory::load_and_swap<uint32_t>(rb_ctrl_host);
  snapshot.sub_ctr = rex::memory::load_and_swap<uint32_t>(sub_ctr_host);
  snapshot.counter = rex::memory::load_and_swap<uint32_t>(counter_host);
  snapshot.wptr = rex::memory::load_and_swap<uint32_t>(wptr_host);
  snapshot.flags = rex::memory::load_and_swap<uint8_t>(flags_host);
  snapshot.cmd_buf = rex::memory::load_and_swap<uint32_t>(cmd_buf_host);
  if (snapshot.rb_ctrl) {
    auto shadow0_host = memory->TranslateVirtual(snapshot.rb_ctrl);
    auto shadow4_host = memory->TranslateVirtual(snapshot.rb_ctrl + 4);
    if (shadow0_host && shadow4_host) {
      snapshot.shadow0 = rex::memory::load_and_swap<uint32_t>(shadow0_host);
      snapshot.shadow4 = rex::memory::load_and_swap<uint32_t>(shadow4_host);
    }
  }
  if (snapshot.cmd_buf) {
    auto marker_host = memory->TranslateVirtual(snapshot.cmd_buf + 16);
    if (marker_host) {
      snapshot.marker = rex::memory::load_and_swap<uint32_t>(marker_host);
    }
  }
  snapshot.valid = true;
  return snapshot;
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
  std::fprintf(stderr, "[probe] GraphicsSystem::SetupPresentation presenter=%p provider=%p app_context=%p\n",
               static_cast<void*>(presenter_.get()), static_cast<void*>(provider_.get()),
               static_cast<void*>(app_context));
  std::fflush(stderr);
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
  std::fprintf(stderr,
               "[probe] GraphicsSystem::SetupGuestGpu provider=%p presenter=%p function_dispatcher=%p kernel_state=%p\n",
               static_cast<void*>(provider_.get()), static_cast<void*>(presenter_.get()),
               static_cast<void*>(function_dispatcher), static_cast<void*>(kernel_state));
  std::fflush(stderr);
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
  std::fprintf(stderr, "[probe] GraphicsSystem::SetupGuestGpu command_processor=%p\n",
               static_cast<void*>(command_processor_.get()));
  std::fflush(stderr);
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

  // Guest vblank timer based on the configured guest video mode.
  vsync_worker_running_ = true;
  vsync_worker_thread_ = system::object_ref<system::XHostThread>(
      new system::XHostThread(kernel_state_, 128 * 1024, 0, [this]() {
        system::X_VIDEO_MODE video_mode;
        kernel::xboxkrnl::VdQueryVideoMode(&video_mode);
        double refresh_rate_hz = std::max(1.0, double(float(video_mode.refresh_rate)));
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
  vsync_worker_thread_->Create();

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

  switch (r) {
    case 0x01C5:  // CP_RB_WPTR
      if (REXCVAR_QUERY(bool, metal_present_probe)) {
        static uint32_t rb_wptr_probe_count = 0;
        if (rb_wptr_probe_count < 32) {
          ++rb_wptr_probe_count;
          REXLOG_WARN("GraphicsSystem::WriteRegister CP_RB_WPTR={:08X}", value);
        }
      }
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
  if (REXCVAR_QUERY(bool, metal_present_probe)) {
    static uint32_t gs_ring_probe_count = 0;
    if (gs_ring_probe_count < 8) {
      ++gs_ring_probe_count;
      REXLOG_WARN("GraphicsSystem::InitializeRingBuffer ptr={:08X} size_log2={}", ptr, size_log2);
    }
  }
  command_processor_->InitializeRingBuffer(ptr, size_log2);
}

void GraphicsSystem::EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) {
  command_processor_->EnableReadPointerWriteBack(ptr, block_size_log2);
}

void GraphicsSystem::SetInterruptCallback(uint32_t callback, uint32_t user_data) {
  interrupt_callback_ = callback;
  interrupt_callback_data_ = user_data;
  std::fprintf(stderr, "[probe] GraphicsSystem::SetInterruptCallback callback=%08X user=%08X\n",
               callback, user_data);
  std::fflush(stderr);
  REXGPU_INFO("SetInterruptCallback({:08X}, {:08X})", callback, user_data);
}

void GraphicsSystem::DispatchInterruptCallback(uint32_t source, uint32_t cpu) {
  static uint32_t dispatch_skipped_probe_count = 0;
  static uint32_t dispatch_callback_probe_count = 0;
  if (!interrupt_callback_) {
    if (source != 0) {
      interrupt_callback_source1_pending_ = true;
    }
    if (dispatch_skipped_probe_count < 32) {
      ++dispatch_skipped_probe_count;
      std::fprintf(
          stderr,
          "[probe] GraphicsSystem::DispatchInterruptCallback skipped source=%u cpu=%u no_callback\n",
          source, cpu);
      std::fflush(stderr);
    }
    return;
  }

  auto thread = system::XThread::GetCurrentThread();
  assert_not_null(thread);

  // Pick a CPU, if needed. We're going to guess 2. Because.
  if (cpu == 0xFFFFFFFF) {
    cpu = 2;
  }
  thread->SetActiveCpu(cpu);

  // REXGPU_INFO("Dispatching GPU interrupt at {:08X} w/ mode {} on cpu {}",
  //          interrupt_callback_, source, cpu);

  uint64_t args[] = {source, interrupt_callback_data_};
  if (dispatch_callback_probe_count < 64 || source != 0) {
    ++dispatch_callback_probe_count;
    std::fprintf(
        stderr,
        "[probe] GraphicsSystem::DispatchInterruptCallback source=%u cpu=%u callback=%08X user=%08X\n",
        source, cpu, interrupt_callback_, interrupt_callback_data_);
    std::fflush(stderr);
  }

  if (source == 0) {
    auto ud_host = memory_->TranslateVirtual(interrupt_callback_data_);
    if (ud_host) {
      uint32_t notify_fn = rex::memory::load_and_swap<uint32_t>(ud_host + 15580);
      if (notify_fn == 0) {
        rex::kernel::xboxkrnl::xeSignalLikelyVblankWaitObject();
      }
    }
  }

  InterruptControlSnapshot before_snapshot =
      ReadInterruptControlSnapshot(memory_, interrupt_callback_data_);
  {
    static uint32_t dispatch_state_probe_count = 0;
    if ((dispatch_state_probe_count < 24 || source != 0) && before_snapshot.valid) {
      ++dispatch_state_probe_count;
      std::fprintf(stderr,
                   "[probe] DispatchInterruptCallback pre source=%u rb_ctrl=%08X sh0=%08X "
                   "sh4=%08X sub=%08X ctr=%08X wptr=%08X flags=%02X cmd=%08X marker=%08X\n",
                   source, before_snapshot.rb_ctrl, before_snapshot.shadow0,
                   before_snapshot.shadow4, before_snapshot.sub_ctr, before_snapshot.counter,
                   before_snapshot.wptr, before_snapshot.flags, before_snapshot.cmd_buf,
                   before_snapshot.marker);
      std::fflush(stderr);
    }
  }

  if (source == 0 && interrupt_callback_source1_pending_ && before_snapshot.rb_ctrl != 0) {
    interrupt_callback_source1_pending_ = false;
    std::fprintf(stderr,
                 "[probe] DispatchInterruptCallback replaying pending source=1 rb_ctrl=%08X\n",
                 before_snapshot.rb_ctrl);
    std::fflush(stderr);
    DispatchInterruptCallback(1, cpu);
  }

  SyncGuestRingBufferState(memory_, interrupt_callback_data_);
  __sync_synchronize();
  function_dispatcher_->ExecuteInterrupt(thread->thread_state(), interrupt_callback_, args,
                                         rex::countof(args));
  SyncGuestRingBufferState(memory_, interrupt_callback_data_);

  {
    InterruptControlSnapshot after_snapshot =
        ReadInterruptControlSnapshot(memory_, interrupt_callback_data_);
    static uint32_t dispatch_state_after_probe_count = 0;
    if ((dispatch_state_after_probe_count < 24 || source != 0) && after_snapshot.valid) {
      ++dispatch_state_after_probe_count;
      std::fprintf(stderr,
                   "[probe] DispatchInterruptCallback post source=%u rb_ctrl=%08X sh0=%08X "
                   "sh4=%08X sub=%08X ctr=%08X wptr=%08X flags=%02X cmd=%08X marker=%08X\n",
                   source, after_snapshot.rb_ctrl, after_snapshot.shadow0, after_snapshot.shadow4,
                   after_snapshot.sub_ctr, after_snapshot.counter, after_snapshot.wptr,
                   after_snapshot.flags, after_snapshot.cmd_buf, after_snapshot.marker);
      std::fflush(stderr);
    }
  }
}

void GraphicsSystem::MarkVblank() {
  // TODO: Enable profiling once ported
  // SCOPE_profile_cpu_f("gpu");

  // Increment vblank counter (so the game sees us making progress).
  if (command_processor_) {
    command_processor_->increment_counter();
    command_processor_->RequestDeferredInterrupt();
  }

  // TODO(benvanik): we shouldn't need to do the dispatch here, but there's
  //     something wrong and the CP will block waiting for code that
  //     needs to be run in the interrupt.
  DispatchInterruptCallback(0, 2);
  if (command_processor_) {
    bool gpu_busy = command_processor_->gpu_busy();
    bool deferred_interrupt_pending = false;
    if (!gpu_busy) {
      deferred_interrupt_pending = command_processor_->TakeDeferredInterruptPending();
    }
    if (REXCVAR_QUERY(bool, metal_present_probe)) {
      static uint32_t vblank_deferred_probe_count = 0;
      if (vblank_deferred_probe_count < 32) {
        ++vblank_deferred_probe_count;
        std::fprintf(stderr,
                     "[probe] GraphicsSystem::MarkVblank gpu_busy=%d deferred_interrupt_pending=%d callback=%08X\n",
                     gpu_busy, deferred_interrupt_pending, interrupt_callback_);
        std::fflush(stderr);
      }
    }
    if (!gpu_busy && deferred_interrupt_pending) {
      DispatchInterruptCallback(1, 2);
    }
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
