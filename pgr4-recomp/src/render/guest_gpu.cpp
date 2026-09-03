// render/guest_gpu.cpp

#include "guest_gpu.h"

#include <array>
#include <chrono>
#include <string>

#include <fmt/format.h>
#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/graphics/xenos.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/thread_state.h>
#include <rex/system/util/object_table.h>
#include <rex/system/xmemory.h>
#include <rex/thread.h>

// This models a hardware vblank, so it must stay near display refresh. It is
// NOT the frame-rate cap.
//
// Free-running this was measured at ~4,000,000 interrupts/sec, which starves
// the guest's own threads and stalls it before it ever reaches a swap. Removing
// an FPS limit is a separate concern -- that belongs to swapchain vsync
// (RenderSwapChain::setVsyncEnabled) and to not throttling the guest's own
// frame work, neither of which is served by spinning this signal.
//
// <= 0 falls back to kDefaultVsyncHz rather than free-running, so a stray 0
// cannot reintroduce that stall.
REXCVAR_DEFINE_INT32(pgr4_vsync_hz, 60, "GPU",
                     "Guest graphics-interrupt (vblank) rate in Hz. Models display refresh; "
                     "not a frame-rate cap. <= 0 falls back to 60.");

namespace pgr4::render {

// X_STATUS_SUCCESS is a macro that casts to an unqualified X_STATUS, so the
// type has to be visible here for it to expand.
using rex::X_STATUS;

namespace {

// The guest's interrupt callback takes two arguments:
//   r3 = source, 0 for a normal vsync interrupt
//   r4 = the user_data handed to VdSetGraphicsInterruptCallback
constexpr uint64_t kInterruptSourceVsync = 0;

// Fallback when the cvar is unset or nonsensical. 60 matches the rate the
// title was written for.
constexpr int32_t kDefaultVsyncHz = 60;

// GPU register window. Same mapping the xenos plugin registers: 64 KB at
// 0x7FC80000, addressed as dword indices.
constexpr uint32_t kGpuRegisterBase = 0x7FC80000;
constexpr uint32_t kGpuRegisterMask = 0xFFFF0000;
constexpr uint32_t kGpuRegisterSize = 0x0000FFFF;
constexpr uint32_t kGpuRegisterCount = 0x4000;

// Register indices the guest's D3D touches during bring-up.
constexpr uint32_t kRegCpRbRptr = 0x01C4;         // CP_RB_RPTR
constexpr uint32_t kRegCpRbWptr = 0x01C5;         // CP_RB_WPTR
constexpr uint32_t kRegCoherStatusHost = 0x0A31;  // COHER_STATUS_HOST
constexpr uint32_t kRegScratchUmsk = 0x01DC;      // SCRATCH_UMSK: per-reg writeback enable bits
constexpr uint32_t kRegScratchAddr = 0x01DD;      // SCRATCH_ADDR: physical base for writeback
constexpr uint32_t kRegScratchReg0 = 0x0578;      // SCRATCH_REG0 ("interrupt sync")
constexpr uint32_t kRegScratchReg7 = 0x057F;      // SCRATCH_REG7

// Every CP_RB_WPTR store, not just the first: "how many times did the guest
// kick the GPU" is the difference between a guest that submitted once and
// stalled and one that is submitting steadily but never sees progress.
std::atomic<uint64_t> g_wptr_stores{0};
constexpr uint32_t kRegRbEdramTiming = 0x0F00;    // RB_EDRAM_TIMING
constexpr uint32_t kRegRbBcControl = 0x0F01;      // RB_BC_CONTROL
constexpr uint32_t kRegD1ModeVCounter = 0x194C;   // R500_D1MODE_V_COUNTER
constexpr uint32_t kRegInterruptStatus = 0x1951;  // interrupt status
constexpr uint32_t kRegD1ModeViewport = 0x1961;   // AVIVO_D1MODE_VIEWPORT_SIZE

// The guest frame PGR4 renders. Reported for the display-mode registers so the
// same values the xenos plugin derives from VdQueryVideoMode come back here,
// without pulling that dependency in for two constants.
constexpr uint32_t kGuestWidth = 1280;
constexpr uint32_t kGuestHeight = 720;

// Last-written values, so reads of registers we do not special-case return
// what the guest stored -- the same fallback the real register file provides.
std::array<uint32_t, kGpuRegisterCount> g_registers{};

}  // namespace

Pgr4GraphicsSystem::~Pgr4GraphicsSystem() { Shutdown(); }

X_STATUS Pgr4GraphicsSystem::SetupPresentation(rex::ui::WindowedAppContext* app_context) {
  (void)app_context;
  has_presentation_ = true;
  return X_STATUS_SUCCESS;
}

// ---- Vd* surface -----------------------------------------------------------

void Pgr4GraphicsSystem::SetInterruptCallback(uint32_t callback, uint32_t user_data) {
  interrupt_user_data_.store(user_data, std::memory_order_relaxed);
  // Publish the callback last: the worker gates on it, so it must never observe
  // a live callback paired with stale user data.
  interrupt_callback_.store(callback, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: interrupt callback {:08X} (user_data {:08X})", callback, user_data);
}

void Pgr4GraphicsSystem::InitializeRingBuffer(uint32_t ptr, uint32_t size_log2) {
  // Nothing here executes PM4; the ring is only ever *consumed* (see
  // PublishReadPointer). Its location is not needed for that, so it is logged
  // and otherwise ignored. Size follows the real command processor's decode.
  write_ptr_index_.store(0, std::memory_order_relaxed);
  read_ptr_index_ = 0;
  ring_size_dwords_ = (uint32_t(1) << (size_log2 + 3)) / 4;
  ring_base_.store(ptr, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: ring buffer at {:08X} ({} bytes); consumed without execution",
              ptr, uint32_t(1) << (size_log2 + 3));
}

void Pgr4GraphicsSystem::EnableReadPointerWriteBack(uint32_t ptr, uint32_t block_size_log2) {
  (void)block_size_log2;
  // CP_RB_RPTR_ADDR: the physical address the guest polls for how far the GPU
  // has consumed the ring. From here on, every write-pointer update and every
  // vblank republishes "everything consumed" to it.
  read_ptr_writeback_.store(ptr, std::memory_order_release);
  REXLOG_INFO("PGR4 guest GPU: read-pointer writeback at {:08X}", ptr);
  PublishReadPointer();
}

void Pgr4GraphicsSystem::PublishReadPointer() {
  const uint32_t writeback = read_ptr_writeback_.load(std::memory_order_acquire);
  if (writeback == 0 || memory_ == nullptr) {
    return;
  }
  // Mirror the write pointer back as the read pointer. To the guest this is a
  // GPU that has always finished everything it was handed, which is exactly
  // what a native renderer intercepting at the API level looks like from the
  // guest's side. The real command processor does this same store after
  // executing; we do it without executing.
  const uint32_t write_index = write_ptr_index_.load(std::memory_order_acquire);
  rex::memory::store_and_swap<uint32_t>(memory_->TranslatePhysical<uint8_t*>(writeback),
                                        write_index);
}

// ---- GPU register window ---------------------------------------------------

uint32_t Pgr4GraphicsSystem::ReadRegisterThunk(void* ppc_context, void* self, uint32_t addr) {
  (void)ppc_context;
  return static_cast<Pgr4GraphicsSystem*>(self)->ReadRegister(addr);
}

void Pgr4GraphicsSystem::WriteRegisterThunk(void* ppc_context, void* self, uint32_t addr,
                                            uint32_t value) {
  (void)ppc_context;
  static_cast<Pgr4GraphicsSystem*>(self)->WriteRegister(addr, value);
}

uint32_t Pgr4GraphicsSystem::ReadRegister(uint32_t addr) {
  const uint32_t r = (addr & 0xFFFF) / 4;
  switch (r) {
    // The guest polls these during device init. Values match what the xenos
    // plugin answers, so bring-up sees the same hardware it would there.
    case kRegRbEdramTiming:
      return 0x08100748;
    case kRegRbBcControl:
      return 0x0000200E;
    case kRegD1ModeVCounter:
      return kGuestHeight;
    case kRegInterruptStatus:
      return 1;  // vblank
    case kRegD1ModeViewport:
      return (kGuestWidth << 16) | kGuestHeight;
    // Consumption is instantaneous, so the read pointer is always the write
    // pointer -- same answer the writeback location gives.
    case kRegCpRbRptr:
      return write_ptr_index_.load(std::memory_order_acquire);
    default:
      break;
  }

  // Bring-up instrumentation: a register the guest polls that we answer from
  // the plain shadow is the likeliest place a stall hides. Once per register.
  static std::array<std::atomic<bool>, kGpuRegisterCount> logged{};
  if (r < kGpuRegisterCount && !logged[r].exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("PGR4 guest GPU: unmodelled register read {:04X} -> {:08X}", r, g_registers[r]);
  }
  return r < kGpuRegisterCount ? g_registers[r] : 0;
}

void Pgr4GraphicsSystem::WriteRegister(uint32_t addr, uint32_t value) {
  const uint32_t r = (addr & 0xFFFF) / 4;
  if (r < kGpuRegisterCount) {
    g_registers[r] = value;
  }
  if (r == kRegCpRbWptr) {
    // The guest just advanced its write pointer. Only publish it here: the
    // packets are executed on the worker, like the real command processor,
    // which trails the CPU. Executing synchronously inside this store delivered
    // PM4_INTERRUPT before the guest had installed its callback and it trapped
    // on the 0x0BADF00D sentinel.
    write_ptr_index_.store(value, std::memory_order_release);

    // Bring-up instrumentation: whether the guest ever submits ring work is the
    // dividing line between "stalled on a full ring" and "stalled before it
    // ever built a frame". Without this the two are indistinguishable.
    if (g_wptr_stores.fetch_add(1, std::memory_order_relaxed) == 0) {
      REXLOG_INFO("PGR4 guest GPU: first CP_RB_WPTR write = {:08X}", value);

      // Dump the first submission verbatim. The guest submits this one batch
      // and then waits on whatever it was supposed to produce; decoding it is
      // how we learn which side effects to synthesise.
      const uint32_t base = ring_base_.load(std::memory_order_acquire);
      if (base != 0 && memory_ != nullptr && value <= 256) {
        const uint8_t* ring = memory_->TranslatePhysical<const uint8_t*>(base);
        for (uint32_t i = 0; i < value; i += 8) {
          std::string line;
          for (uint32_t j = i; j < value && j < i + 8; ++j) {
            line += fmt::format(" {:08X}", rex::memory::load_and_swap<uint32_t>(ring + j * 4));
          }
          REXLOG_INFO("PGR4 guest GPU: ring[{:02X}]:{}", i, line);
        }

        // Follow PM4_INDIRECT_BUFFER (type 3, opcode 0x3F): the guest's init
        // batch is ME_INIT + one indirect buffer, and the polled side effects
        // live in the latter. Same decode the command processor uses: address
        // masked to a dword-aligned physical pointer, length in dwords.
        for (uint32_t i = 0; i + 2 < value;) {
          const uint32_t header = rex::memory::load_and_swap<uint32_t>(ring + i * 4);
          const uint32_t type = header >> 30;
          const uint32_t count = ((header >> 16) & 0x3FFF) + 1;
          if (type == 3 && ((header >> 8) & 0x7F) == 0x3F) {
            const uint32_t ib_ptr =
                rex::memory::load_and_swap<uint32_t>(ring + (i + 1) * 4) & 0x1FFFFFFC;
            const uint32_t ib_len =
                rex::memory::load_and_swap<uint32_t>(ring + (i + 2) * 4) & 0xFFFFF;
            REXLOG_INFO("PGR4 guest GPU: indirect buffer at {:08X}, {} dwords", ib_ptr, ib_len);
            if (ib_len <= 256) {
              const uint8_t* ib = memory_->TranslatePhysical<const uint8_t*>(ib_ptr);
              for (uint32_t k = 0; k < ib_len; k += 8) {
                std::string line;
                for (uint32_t j = k; j < ib_len && j < k + 8; ++j) {
                  line += fmt::format(" {:08X}",
                                      rex::memory::load_and_swap<uint32_t>(ib + j * 4));
                }
                REXLOG_INFO("PGR4 guest GPU:   ib[{:02X}]:{}", k, line);
              }
            }
          }
          i += (type == 3 || type == 0) ? count + 1 : 1;
        }
      }
    }
    return;
  }

  // Anything else the guest pokes during bring-up that we are not modelling.
  // Logged once per register so a missing behaviour shows up by name instead
  // of as silence.
  static std::array<std::atomic<bool>, kGpuRegisterCount> logged{};
  if (r < kGpuRegisterCount && !logged[r].exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("PGR4 guest GPU: unmodelled register write {:04X} = {:08X}", r, value);
  }
}

// ---- minimal PM4 executor ----------------------------------------------------

void Pgr4GraphicsSystem::ApplyRegisterWrite(uint32_t reg, uint32_t value) {
  if (reg >= kGpuRegisterCount) {
    return;
  }
  g_registers[reg] = value;
  if (reg == kRegCoherStatusHost) {
    // Hardware (and the real command processor) marks the coherency request
    // busy on write; the guest's WAIT_REG_MEM then spins until that bit
    // clears, which happens when the flush executes below.
    g_registers[reg] |= 0x80000000u;
    return;
  }

  // Scratch-register writeback: the one other guest-memory write the real
  // command processor performs. SCRATCH_REG0 ("interrupt sync") is the GPU
  // progress word the D3D library's CBlocker polls at *(device+10896); if it
  // only ever lands in this shadow and never in memory, that word stays at its
  // initial value and D3D::CBlocker::Check spins forever -- which is exactly
  // the black screen.
  if (reg >= kRegScratchReg0 && reg <= kRegScratchReg7) {
    const uint32_t scratch = reg - kRegScratchReg0;
    if (((1u << scratch) & g_registers[kRegScratchUmsk]) != 0 && memory_ != nullptr) {
      const uint32_t addr = g_registers[kRegScratchAddr] + scratch * 4;
      rex::memory::store_and_swap<uint32_t>(memory_->TranslatePhysical<uint8_t*>(addr), value);
      // Every writeback, not just the first: whether SCRATCH_REG4 is ever
      // rewritten from the 0x0BADF00D sentinel to a real callback is the whole
      // question. Low volume during bring-up.
      REXLOG_INFO("PGR4 guest GPU: scratch REG{} = {:08X} -> {:08X}", scratch, value, addr);
    }
  } else if (reg == kRegScratchAddr || reg == kRegScratchUmsk) {
    REXLOG_INFO("PGR4 guest GPU: {} = {:08X}", reg == kRegScratchAddr ? "SCRATCH_ADDR" : "SCRATCH_UMSK",
                value);
  }
}

void Pgr4GraphicsSystem::ExecutePackets(const uint8_t* buffer, uint32_t dword_count,
                                        uint32_t depth) {
  if (depth > 4) {
    return;  // runaway indirect chains are a corrupt ring, not a real workload
  }
  auto rd = [&](uint32_t i) { return rex::memory::load_and_swap<uint32_t>(buffer + i * 4); };

  uint32_t i = 0;
  while (i < dword_count) {
    const uint32_t header = rd(i);
    const uint32_t type = header >> 30;

    if (type == 0) {
      // Type 0: sequential register writes starting at bits 15:0.
      const uint32_t count = ((header >> 16) & 0x3FFF) + 1;
      const uint32_t base = header & 0x7FFF;
      for (uint32_t k = 0; k < count && i + 1 + k < dword_count; ++k) {
        ApplyRegisterWrite(base + k, rd(i + 1 + k));
      }
      i += 1 + count;
      continue;
    }

    if (type == 2) {
      i += 1;  // filler
      continue;
    }

    if (type != 3) {
      i += 1;  // type 1 is unused on Xenos; step past whatever this is
      continue;
    }

    const uint32_t count = ((header >> 16) & 0x3FFF) + 1;
    const uint32_t opcode = (header >> 8) & 0x7F;

    // Bring-up instrumentation: which type-3 opcodes the guest actually emits.
    // Logged once per opcode so unmodelled side effects show up by name.
    {
      static std::array<std::atomic<bool>, 0x80> seen{};
      if (!seen[opcode].exchange(true, std::memory_order_relaxed)) {
        REXLOG_INFO("PGR4 guest GPU: first PM4 opcode 0x{:02X} (count {}) at depth {}", opcode,
                    count, depth);
      }
    }

    switch (opcode) {
      case 0x54: {  // PM4_INTERRUPT: cpu_mask -> guest callback with source=1 per CPU.
        if (i + 1 < dword_count) {
          pending_cpu_interrupts_.fetch_or(rd(i + 1) & 0xFF, std::memory_order_release);
        }
        break;
      }
      case 0x3D: {  // PM4_MEM_WRITE: addr(+endian in low bits), then count-1 dwords.
        if (i + 1 < dword_count && memory_ != nullptr) {
          uint32_t write_addr = rd(i + 1);
          const auto endian = static_cast<rex::graphics::xenos::Endian>(write_addr & 0x3);
          write_addr &= ~0x3u;
          for (uint32_t k = 0; k + 2 < count + 1 && i + 2 + k < dword_count; ++k) {
            const uint32_t data = rex::graphics::xenos::GpuSwap(rd(i + 2 + k), endian);
            *reinterpret_cast<uint32_t*>(memory_->TranslatePhysical<uint8_t*>(write_addr)) = data;
            write_addr += 4;
          }
        }
        break;
      }
      case 0x58: {  // PM4_EVENT_WRITE_SHD: initiator, address, value -> fence write.
        if (i + 3 < dword_count && memory_ != nullptr) {
          const uint32_t initiator = rd(i + 1);
          uint32_t address = rd(i + 2);
          uint32_t data = rd(i + 3);
          if ((initiator >> 31) & 0x1) {
            data = event_counter_.load(std::memory_order_acquire);
          }
          const auto endian = static_cast<rex::graphics::xenos::Endian>(address & 0x3);
          address &= ~0x3u;
          *reinterpret_cast<uint32_t*>(memory_->TranslatePhysical<uint8_t*>(address)) =
              rex::graphics::xenos::GpuSwap(data, endian);
        }
        break;
      }
      case 0x64: {  // PM4_XE_SWAP: the command processor bumps its counter here.
        event_counter_.fetch_add(1, std::memory_order_acq_rel);
        break;
      }
      case 0x21: {  // PM4_REG_RMW: reg = (reg & and) | or, each operand a reg or immediate.
        // This is how the D3D library installs its CPU-interrupt callback into
        // SCRATCH_REG4 after seeding it with the 0x0BADF00D sentinel; skipping
        // it leaves the sentinel in place and the first PM4_INTERRUPT traps.
        if (i + 3 < dword_count) {
          const uint32_t rmw_info = rd(i + 1);
          const uint32_t and_mask = rd(i + 2);
          const uint32_t or_mask = rd(i + 3);
          const uint32_t reg = rmw_info & 0x1FFF;
          auto shadow = [&](uint32_t r) { return r < kGpuRegisterCount ? g_registers[r] : 0u; };
          uint32_t value = shadow(reg);
          value &= ((rmw_info >> 31) & 1) ? shadow(and_mask & 0x1FFF) : and_mask;
          value |= ((rmw_info >> 30) & 1) ? shadow(or_mask & 0x1FFF) : or_mask;
          ApplyRegisterWrite(reg, value);
        }
        break;
      }
      case 0x3F: {  // PM4_INDIRECT_BUFFER
        if (i + 2 < dword_count) {
          const uint32_t ib_ptr = rd(i + 1) & 0x1FFFFFFC;
          const uint32_t ib_len = rd(i + 2) & 0xFFFFF;
          if (ib_ptr != 0 && ib_len != 0) {
            ExecutePackets(memory_->TranslatePhysical<const uint8_t*>(ib_ptr), ib_len, depth + 1);
          }
        }
        break;
      }
      case 0x3C: {  // PM4_WAIT_REG_MEM
        // wait_info, poll addr, ref, mask, wait. Only the coherency handshake
        // is honoured: a native renderer has no GPU caches, so "flush" is
        // simply "done", which the real command processor expresses by
        // zeroing COHER_STATUS_HOST after MakeCoherent.
        if (i + 4 < dword_count) {
          const uint32_t wait_info = rd(i + 1);
          const uint32_t poll = rd(i + 2);
          const bool is_memory = (wait_info & 0x10) != 0;
          if (!is_memory && poll == kRegCoherStatusHost) {
            g_registers[kRegCoherStatusHost] = 0;
          } else if (is_memory) {
            WaitOnGuestMemory(wait_info, poll, rd(i + 3), rd(i + 4));
          }
        }
        break;
      }
      case 0x22:  // PM4_DRAW_INDX
      case 0x36:  // PM4_DRAW_INDX_2
        g_pm4DrawPackets.fetch_add(1, std::memory_order_relaxed);
        break;
      default:
        // ME_INIT, draws, constants, events, swaps: no side effect the guest
        // polls for at this stage, and everything renderable is intercepted at
        // the D3D API instead. Skipped by count.
        break;
    }
    i += 1 + count;
  }
}

void Pgr4GraphicsSystem::ExecuteRing(uint32_t write_index) {
  const uint32_t base = ring_base_.load(std::memory_order_acquire);
  if (base == 0 || memory_ == nullptr || ring_size_dwords_ == 0) {
    return;
  }
  const uint8_t* ring = memory_->TranslatePhysical<const uint8_t*>(base);
  const uint32_t write = write_index % ring_size_dwords_;

  if (write >= read_ptr_index_) {
    ExecutePackets(ring + read_ptr_index_ * 4, write - read_ptr_index_, 0);
  } else {
    // Wrapped: tail of the ring, then the head.
    ExecutePackets(ring + read_ptr_index_ * 4, ring_size_dwords_ - read_ptr_index_, 0);
    ExecutePackets(ring, write, 0);
  }
  read_ptr_index_ = write;
}

// ---- lifecycle ---------------------------------------------------------------

X_STATUS Pgr4GraphicsSystem::SetupGuestGpu(rex::runtime::FunctionDispatcher* function_dispatcher,
                                           rex::system::KernelState* kernel_state) {
  function_dispatcher_ = function_dispatcher;
  memory_ = kernel_state->memory();

  // Without this mapping the guest's CP_RB_WPTR stores land nowhere and its
  // status polls read zero, and D3D device creation never completes.
  if (!memory_->AddVirtualMappedRange(
          kGpuRegisterBase, kGpuRegisterMask, kGpuRegisterSize, this,
          reinterpret_cast<rex::runtime::MMIOReadCallback>(ReadRegisterThunk),
          reinterpret_cast<rex::runtime::MMIOWriteCallback>(WriteRegisterThunk))) {
    REXLOG_ERROR("PGR4 guest GPU: failed to map GPU register window at {:08X}", kGpuRegisterBase);
    return X_STATUS_UNSUCCESSFUL;
  }
  mmio_mapped_ = true;

  worker_running_.store(true, std::memory_order_release);
  worker_thread_ = rex::system::object_ref<rex::system::XHostThread>(
      new rex::system::XHostThread(kernel_state, 128 * 1024, 0, [this]() {
        WorkerMain();
        return 0;
      }));
  worker_thread_->set_name("PGR4 Vsync Worker");
  worker_thread_->Create();

  const int32_t hz = REXCVAR_GET(pgr4_vsync_hz);
  REXLOG_INFO("PGR4 guest GPU: vsync worker started ({} Hz{})", hz > 0 ? hz : kDefaultVsyncHz,
              hz > 0 ? "" : ", cvar unset/invalid");
  return X_STATUS_SUCCESS;
}

void Pgr4GraphicsSystem::DeliverPendingCpuInterrupts() {
  const uint32_t callback = interrupt_callback_.load(std::memory_order_acquire);
  if (callback == 0 || function_dispatcher_ == nullptr || !worker_thread_) {
    return;
  }
  uint32_t mask = pending_cpu_interrupts_.exchange(0, std::memory_order_acq_rel);

  // The guest's source=1 handler reads its callback from
  // *(*(device+10900)+16) and traps if it still holds the 0x0BADF00D
  // sentinel. On hardware the CP trails the CPU far enough that the slot
  // is always installed first; here, hold delivery until it is, and log
  // where the slot actually lives so a mis-modelled writer shows up.
  if (mask != 0) {
    const uint32_t device = interrupt_user_data_.load(std::memory_order_relaxed);
    auto rd32 = [&](uint32_t addr) {
      return rex::memory::load_and_swap<uint32_t>(
          memory_->TranslateVirtual<const uint8_t*>(addr));
    };
    const uint32_t blk = device ? rd32(device + 10900) : 0;
    const uint32_t cb = blk ? rd32(blk + 16) : 0;
    static std::atomic<bool> probed{false};
    if (!probed.exchange(true, std::memory_order_relaxed)) {
      REXLOG_INFO("PGR4 guest GPU: cpu-interrupt block={:08X} pending={:08X} callback={:08X} arg={:08X}",
                  blk, blk ? rd32(blk) : 0, cb, blk ? rd32(blk + 20) : 0);
    }
    if (cb == 0x0BADF00D || cb == 0) {
      static std::atomic<bool> deferred{false};
      if (!deferred.exchange(true, std::memory_order_relaxed)) {
        REXLOG_INFO("PGR4 guest GPU: deferring CPU interrupt: callback slot {:08X} = {:08X}",
                    blk + 16, cb);
      }
      pending_cpu_interrupts_.fetch_or(mask, std::memory_order_acq_rel);
      mask = 0;
    }
  }

  for (uint32_t cpu = 0; mask != 0; ++cpu, mask >>= 1) {
    if ((mask & 1) == 0) {
      continue;
    }
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true, std::memory_order_relaxed)) {
      REXLOG_INFO("PGR4 guest GPU: delivering first CPU interrupt (source=1) to cpu {}", cpu);
    }
    uint64_t cpu_args[] = {1, interrupt_user_data_.load(std::memory_order_relaxed)};
    worker_thread_->SetActiveCpu(static_cast<uint8_t>(cpu));
    function_dispatcher_->ExecuteInterrupt(worker_thread_->thread_state(), callback, cpu_args,
                                           rex::countof(cpu_args));
  }
}

bool Pgr4GraphicsSystem::WaitOnGuestMemory(uint32_t wait_info, uint32_t poll, uint32_t ref,
                                           uint32_t mask) {
  if (memory_ == nullptr) {
    return false;
  }
  const uint32_t function = wait_info & 0x7;
  const auto endian = static_cast<rex::graphics::xenos::Endian>(poll & 0x3);
  const uint32_t address = poll & ~0x3u & 0x1FFFFFFFu;
  auto satisfied = [&]() {
    const uint32_t raw =
        *reinterpret_cast<const uint32_t*>(memory_->TranslatePhysical<const uint8_t*>(address));
    const uint32_t value = rex::graphics::xenos::GpuSwap(raw, endian) & mask;
    switch (function) {
      case 0: return false;
      case 1: return value < ref;
      case 2: return value <= ref;
      case 3: return value == ref;
      case 4: return value != ref;
      case 5: return value >= ref;
      case 6: return value > ref;
      default: return true;
    }
  };
  const uint64_t freq = rex::chrono::Clock::QueryHostTickFrequency();
  const uint64_t deadline = rex::chrono::Clock::QueryHostTickCount() + (freq ? freq / 20 : 0);
  while (!satisfied()) {
    // The other side of this handshake is the guest's source=1 handler, which
    // only runs when the interrupt the CP raised just before is delivered.
    DeliverPendingCpuInterrupts();
    if (satisfied()) {
      break;
    }
    const uint64_t now = rex::chrono::Clock::QueryHostTickCount();
    if (vblank_period_ != 0 && now >= next_vblank_tick_) {
      DeliverVblank();
      next_vblank_tick_ += vblank_period_;
      continue;
    }
    if (now >= deadline) {
      static std::atomic<bool> logged{false};
      if (!logged.exchange(true, std::memory_order_relaxed)) {
        REXLOG_WARN("PGR4 guest GPU: WAIT_REG_MEM timed out: [{:08X}] fn={} ref={:08X} mask={:08X}",
                    address, function, ref, mask);
      }
      return false;
    }
    rex::thread::Sleep(std::chrono::microseconds(50));
  }
  return true;
}

void Pgr4GraphicsSystem::DeliverVblank() {
  const uint32_t callback = interrupt_callback_.load(std::memory_order_acquire);
  if (callback == 0 || function_dispatcher_ == nullptr || !worker_thread_) {
    return;
  }
  uint64_t args[] = {kInterruptSourceVsync,
                     interrupt_user_data_.load(std::memory_order_relaxed)};

  // Bring-up instrumentation. Without this there is no way to tell a worker
  // that never fires from a guest callback that returns without advancing the
  // frame loop -- the two look identical from outside.
  static bool logged_first = false;
  if (!logged_first) {
    logged_first = true;
    REXLOG_INFO("PGR4 guest GPU: dispatching first interrupt to {:08X}", callback);
  }

  // Same delivery the xenos plugin uses: pin the dispatching thread to the
  // CPU the guest expects (2 for vblank), and run the callback through the
  // interrupt entry rather than a plain call. The source=1 handler takes
  // spinlocks at raised IRQL and assumes interrupt context.
  worker_thread_->SetActiveCpu(2);
  function_dispatcher_->ExecuteInterrupt(worker_thread_->thread_state(), callback, args,
                                         rex::countof(args));
  event_counter_.fetch_add(1, std::memory_order_acq_rel);
}

void Pgr4GraphicsSystem::WorkerMain() {

  while (worker_running_.load(std::memory_order_acquire)) {
    const uint32_t callback = interrupt_callback_.load(std::memory_order_acquire);
    if (callback == 0) {
      // The guest has not registered its interrupt yet, but it may already be
      // kicking the ring (device init submits before the callback exists), so
      // keep consuming; just don't try to deliver interrupts.
      const uint32_t write = write_ptr_index_.load(std::memory_order_acquire);
      if (ring_size_dwords_ != 0 && (write % ring_size_dwords_) != read_ptr_index_) {
        ExecuteRing(write);
        PublishReadPointer();
      }
      rex::thread::Sleep(std::chrono::milliseconds(1));
      continue;
    }

    // Republish consumption every vblank as well as on every write-pointer
    // store, so a guest that polls the writeback rather than the register sees
    // progress even between its own submissions.
    PublishReadPointer();

    DeliverVblank();

    {
      static uint64_t fires = 0;
      static uint64_t window = 0;
      ++fires;
      const uint64_t f = rex::chrono::Clock::QueryHostTickFrequency();
      const uint64_t t = rex::chrono::Clock::QueryHostTickCount();
      if (window == 0) {
        window = t;
      } else if (f != 0 && (t - window) >= f) {
        // user_data is the guest CDevice; these are the fields the vblank path
        // and the frame loop share. If vblanks increments at 60/sec the guest
        // handler is genuinely running; callback tells us whether the engine
        // registered anything to be woken; submitted/retired show whether any
        // frame was ever handed to Swap.
        const uint32_t device = interrupt_user_data_.load(std::memory_order_relaxed);
        auto field = [&](uint32_t off) {
          return rex::memory::load_and_swap<uint32_t>(
              memory_->TranslateVirtual<const uint8_t*>(device + off));
        };
        // Engine globals gating the render path in its main loop
        // (sub_822F6BE0): rendering happens only while byte_82A61024 != 0.
        auto gbyte = [&](uint32_t addr) {
          return *memory_->TranslateVirtual<const uint8_t*>(addr);
        };
        REXLOG_INFO(
            "PGR4 guest GPU: {} guest interrupts/sec | kicks(total)={} | device vblanks={} "
            "callback={:08X} submitted={} retired={} | render_enabled(82A61024)={} 82B17F30={} "
            "82A61280={}",
            fires, g_wptr_stores.load(std::memory_order_relaxed), field(16532), field(16528),
            field(16544), field(16552), gbyte(0x82A61024), gbyte(0x82B17F30), gbyte(0x82A61280));
        fires = 0;
        window = t;

        // Under plume the guest resumes its worker threads and none ever runs
        // a line of guest code, while under xenos ~30 of them chatter. The SDK
        // does not log guest-thread execution, so enumerate them here: which
        // exist, which are guest threads, which are running, and whether any
        // is still suspended.
        static uint32_t dumps = 0;
        if (dumps++ % 3 == 0) {
          auto threads = REX_KERNEL_OBJECTS()->GetObjectsByType<rex::system::XThread>(
              rex::system::XObject::Type::Thread);
          std::string line;
          for (auto& th : threads) {
            // native = host thread id (matches what the OS reports for CPU
            // time), lr = return address of the guest call in progress, which
            // the recompiled code stores before every bl -- enough for IDA to
            // name the function a spinning thread is sitting in.
            const uint32_t native = th->thread() ? th->thread()->system_id() : 0;
            uint64_t lr = 0;
            uint32_t r1 = 0;
            if (th->is_guest_thread() && th->thread_state() && th->thread_state()->context()) {
              lr = th->thread_state()->context()->lr;
              r1 = th->thread_state()->context()->r1.u32;
            }
            line += fmt::format(" [tid={} native={} guest={} lr={:08X} r1={:08X} prio={}]",
                                th->thread_id(), native, th->is_guest_thread() ? 1 : 0,
                                static_cast<uint32_t>(lr), r1, th->priority());
          }
          REXLOG_INFO("PGR4 guest GPU: {} threads:{}", threads.size(), line);

          // The main thread spins in D3D::CBlocker::Check, which returns
          // "keep waiting" while a GPU progress word (*(device+10896) -> word 0)
          // keeps changing, or until 0x1388 ticks of the per-CPU tick counter
          // (*(r13+256) -> +88) pass without change. Sample both so we can tell
          // a frozen clock from a churning progress word.
          for (auto& th : threads) {
            if (!th->is_guest_thread() || !th->thread_state() || !th->thread_state()->context()) {
              continue;
            }
            const uint32_t r13 = th->thread_state()->context()->r13.u32;
            const uint32_t device = interrupt_user_data_.load(std::memory_order_relaxed);
            if (r13 == 0 || device == 0) {
              continue;
            }
            auto rd32 = [&](uint32_t addr) {
              return rex::memory::load_and_swap<uint32_t>(
                  memory_->TranslateVirtual<const uint8_t*>(addr));
            };
            const uint32_t status_block = rd32(device + 10896);
            const uint32_t progress = status_block ? rd32(status_block) : 0xFFFFFFFF;
            const uint32_t kpcr_p = rd32(r13 + 256);
            const uint32_t tick = kpcr_p ? rd32(kpcr_p + 88) : 0xFFFFFFFF;
            const uint8_t flags10941 = *memory_->TranslateVirtual<const uint8_t*>(device + 10941);
            REXLOG_INFO(
                "PGR4 guest GPU: blocker probe tid={} status_block={:08X} progress={:08X} "
                "tick={:08X} dev+10941={:02X} dev+10888={:08X} dev+10996={:08X}",
                th->thread_id(), status_block, progress, tick, flags10941, rd32(device + 10888),
                rd32(device + 10996));
            break;  // one guest thread is enough; r13 tick source is shared
          }
        }
      }
    }

    int32_t hz = REXCVAR_GET(pgr4_vsync_hz);
    if (hz <= 0) {
      hz = kDefaultVsyncHz;
    }

    // Advance an absolute deadline rather than sleeping a fixed interval each
    // pass, so the cost of the callback itself does not compound into
    // ever-growing drift.
    const uint64_t freq = rex::chrono::Clock::QueryHostTickFrequency();
    if (freq == 0) {
      continue;
    }
    const uint64_t now = rex::chrono::Clock::QueryHostTickCount();
    const uint64_t period = freq / static_cast<uint64_t>(hz);
    if (next_vblank_tick_ == 0 || now > next_vblank_tick_ + period) {
      next_vblank_tick_ = now;  // first pass, or we fell far enough behind to resync
    }
    next_vblank_tick_ += period;
    vblank_period_ = period;

    // Sleep towards the next vblank in short slices, delivering any CPU
    // interrupts PM4_INTERRUPT queued in between. On hardware those fire the
    // moment the CP reaches the packet; a full vblank of latency would stall
    // the D3D worker that advances the progress word the main thread spins on.
    while (worker_running_.load(std::memory_order_acquire)) {
      // Command processing, asynchronous to the CPU like the real CP: pick up
      // whatever the guest has kicked since the last slice, execute it (which
      // may queue CPU interrupts), then publish consumption.
      const uint32_t write = write_ptr_index_.load(std::memory_order_acquire);
      if (ring_size_dwords_ != 0 && (write % ring_size_dwords_) != read_ptr_index_) {
        ExecuteRing(write);
        PublishReadPointer();
      }

      DeliverPendingCpuInterrupts();
      const uint64_t t = rex::chrono::Clock::QueryHostTickCount();
      if (t >= next_vblank_tick_) {
        break;
      }
      const uint64_t remaining_us = (next_vblank_tick_ - t) * 1000000ULL / freq;
      rex::thread::Sleep(std::chrono::microseconds(remaining_us < 1000 ? remaining_us : 1000));
    }
  }
}

void Pgr4GraphicsSystem::Shutdown() {
  if (!worker_running_.exchange(false, std::memory_order_acq_rel)) {
    return;  // already shut down, or never started
  }
  if (worker_thread_) {
    worker_thread_->Wait(0, 0, 0, nullptr);
    worker_thread_.reset();
  }
  // The MMIO range is left registered: Memory offers no matching remove, and
  // the guest is gone by the time this runs.
  function_dispatcher_ = nullptr;
  memory_ = nullptr;
}

}  // namespace pgr4::render
