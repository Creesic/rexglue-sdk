#include "generated/fm2_init.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

#include <rex/memory/utils.h>
#include <rex/ppc.h>
#include <rex/graphics/graphics_system.h>
#include <rex/graphics/registers.h>
#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/thread.h>

#include "native_renderer/fm2_native_renderer.h"
#include "native_renderer/fm2_native_state.h"

namespace {


REXCVAR_DEFINE_BOOL(
    fm2_break_before_load_637f8, false, "FM2",
    "Break once in FM2HelperEntry637F8 (set true while debugging load transitions)");

REXCVAR_DEFINE_UINT32(
    fm2_break_before_load_lr, 0, "FM2",
    "Break once in FM2HelperEntry63768 when LR (r12) equals this guest address; 0 disables");

REXCVAR_DEFINE_BOOL(
    fm2_prod_guard_stats, false, "FM2",
    "Emit per-second counters for FM2_ProducerProgressGuard_82369340 outcomes");

REXCVAR_DEFINE_UINT32(
    fm2_prod_guard_wait_pause_count, 0, "FM2",
    "On FM2_ProducerProgressGuard wait-return path, execute this many delay_execution() calls per hit (0 disables)");

REXCVAR_DEFINE_UINT32(
    fm2_prod_guard_wait_yield_interval, 0, "FM2",
    "On FM2_ProducerProgressGuard wait-return path, call MaybeYield every N hits (0 disables)");

REXCVAR_DEFINE_BOOL(
    fm2_prod_guard_trace, false, "FM2",
    "Emit deeper per-second decision counters for FM2_ProducerProgressGuard_82369340");

REXCVAR_DEFINE_UINT32(
    fm2_prod_guard_trace_sample_interval, 0, "FM2",
    "When fm2_prod_guard_trace is enabled, emit one detailed wait-path sample every N hits (0 disables)");

REXCVAR_DEFINE_UINT32(
    fm2_prod_waitloop_spin_min_gap, 0, "FM2",
    "In sub_823729E0, continue producer wait-loop only if (avail - need) is greater than this gap (0 keeps original behavior)");

REXCVAR_DEFINE_UINT32(
    fm2_prod_waitloop_yield_interval, 0, "FM2",
    "In sub_823729E0 wait-loop, call MaybeYield every N spin iterations (0 disables)");

REXCVAR_DEFINE_BOOL(
    fm2_apu_mix_stats, false, "FM2",
    "Emit per-second call/timing counters for FM2_ApuMixRenderCore_82697F08");

REXCVAR_DEFINE_BOOL(
    fm2_load_trace, false, "FM2",
    "Enable toggleable load-trace capture for loading-screen investigation");

REXCVAR_DEFINE_UINT32(
    fm2_load_trace_toggle_vk, '1', "FM2",
    "Virtual-key code used to toggle load-trace capture (default key 1, 0 disables)");

REXCVAR_DEFINE_UINT32(
    fm2_load_trace_sample_limit, 16, "FM2",
    "Maximum number of bounded load-trace sample lines per session");

REXCVAR_DEFINE_UINT32(
    fm2_load_trace_autostart_ms, 0, "FM2",
    "Automatically start a load-trace session this many milliseconds after boot (0 disables)");

REXCVAR_DEFINE_UINT32(
    fm2_load_trace_overlay_state, 0, "FM2",
    "Load-trace overlay state: 0=off, 1=armed, 2=recording");

REXCVAR_DEFINE_BOOL(
    fm2_plume_midasm_trace, false, "FM2",
    "Emit bounded diagnostics when FM2 Plume midasm trace hooks fire.");

REXCVAR_DEFINE_UINT32(
    fm2_plume_midasm_trace_limit, 128, "FM2",
    "Maximum FM2 Plume midasm trace lines to emit per hook in one run; 0 is unlimited.");

uint8_t* GuestBase() {
  auto* kernel_state = rex::system::kernel_state();
  if (!kernel_state || !kernel_state->memory()) {
    return nullptr;
  }
  return kernel_state->memory()->virtual_membase();
}

rex::graphics::RegisterFile* GraphicsRegisterFile() {
  auto* runtime = rex::Runtime::instance();
  if (!runtime || !runtime->graphics_system()) {
    return nullptr;
  }
  auto* graphics_system =
      static_cast<rex::graphics::GraphicsSystem*>(runtime->graphics_system());
  return graphics_system ? graphics_system->register_file() : nullptr;
}

const uint32_t* DirectDrawVSFloatConstantRegisters() {
  rex::graphics::RegisterFile* register_file = GraphicsRegisterFile();
  if (!register_file) {
    return nullptr;
  }
  return &register_file->values[rex::graphics::XE_GPU_REG_SHADER_CONSTANT_000_X];
}

bool GuestReadableByte(uint8_t* base, uint32_t guest_address) {
  (void)base;
  constexpr uint32_t kPageMask = ~uint32_t(0xFFFu);
  constexpr uint32_t kPageCacheSlots = 32u;
  struct PageCacheEntry {
    uint32_t page = 0;
    uint8_t readable = 0;
    uint8_t valid = 0;
  };
  thread_local PageCacheEntry cache[kPageCacheSlots];

  const uint32_t page = guest_address & kPageMask;
  const uint32_t slot = (page >> 12) & (kPageCacheSlots - 1);
  PageCacheEntry& e = cache[slot];
  if (e.valid && e.page == page) {
    return e.readable != 0;
  }

  size_t length = 1;
  rex::memory::PageAccess access = rex::memory::PageAccess::kNoAccess;
  if (!rex::memory::QueryProtect(REX_RAW_ADDR(page), length, access)) {
    e.page = page;
    e.readable = 0;
    e.valid = 1;
    return false;
  }
  const bool readable = access != rex::memory::PageAccess::kNoAccess;
  e.page = page;
  e.readable = readable ? 1 : 0;
  e.valid = 1;
  return readable;
}

bool GuestReadableRange(uint8_t* base, uint32_t guest_address, uint32_t byte_count) {
  if (byte_count == 0) {
    return true;
  }

  const uint64_t last = uint64_t(guest_address) + byte_count - 1;
  if (last > std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  return GuestReadableByte(base, guest_address) &&
         GuestReadableByte(base, static_cast<uint32_t>(last));
}

bool HashGuestReadableRange(uint8_t* base, uint32_t guest_address, uint32_t byte_count,
                            uint64_t& hash_out) {
  hash_out = 0;
  if (!base || guest_address == 0 || byte_count == 0 ||
      !GuestReadableRange(base, guest_address, byte_count)) {
    return false;
  }

  hash_out = XXH3_64bits(REX_RAW_ADDR(guest_address), byte_count);
  return true;
}

void SnapshotGuestCString(uint32_t guest_address, char (&out)[65]) {
  std::fill(std::begin(out), std::end(out), '\0');
  uint8_t* base = GuestBase();
  if (!base || guest_address == 0) {
    return;
  }
  for (size_t i = 0; i < 64; ++i) {
    const uint32_t p = guest_address + static_cast<uint32_t>(i);
    if (!GuestReadableByte(base, p)) {
      break;
    }
    const uint8_t c = REX_LOAD_U8(p);
    out[i] = (c >= 32 && c < 127) ? static_cast<char>(c) : '.';
    if (c == 0) {
      out[i] = '\0';
      break;
    }
  }
  out[64] = '\0';
}

bool HasCallableVtableSlot(uint8_t* base, uint32_t object, uint32_t slot_offset) {
  if (object == 0 || (object & 3) != 0 || !GuestReadableRange(base, object, 4)) {
    return false;
  }

  const uint32_t vtable = REX_LOAD_U32(object);
  const uint64_t slot = uint64_t(vtable) + slot_offset;
  if ((vtable & 3) != 0 || slot > std::numeric_limits<uint32_t>::max() ||
      !GuestReadableRange(base, static_cast<uint32_t>(slot), 4)) {
    return false;
  }

  const uint32_t target = REX_LOAD_U32(static_cast<uint32_t>(slot));
  constexpr uint64_t code_end = uint64_t(REX_CODE_BASE) + REX_CODE_SIZE;
  return (target & 3) == 0 && target >= REX_CODE_BASE && target < code_end;
}

bool MaybeBreakOnLoadPoint637F8() {
  static std::atomic<uint32_t> last_enabled{0};
  static std::atomic<uint32_t> fired{0};
  const uint32_t enabled = REXCVAR_GET(fm2_break_before_load_637f8) ? 1u : 0u;
  const uint32_t previous = last_enabled.exchange(enabled, std::memory_order_relaxed);
  if (previous != enabled) {
    fired.store(0, std::memory_order_relaxed);
  }
  if (!enabled || !IsDebuggerPresent()) {
    return false;
  }
  const uint32_t prior = fired.fetch_add(1, std::memory_order_relaxed);
  if (prior == 0) {
    __debugbreak();
    return true;
  }
  return false;
}

bool MaybeBreakOnLoadPointLR(uint32_t lr) {
  static std::atomic<uint32_t> last_lr_cfg{0};
  static std::atomic<uint32_t> fired{0};
  const uint32_t cfg_lr = REXCVAR_GET(fm2_break_before_load_lr);
  const uint32_t previous_cfg = last_lr_cfg.exchange(cfg_lr, std::memory_order_relaxed);
  if (previous_cfg != cfg_lr) {
    fired.store(0, std::memory_order_relaxed);
  }
  if (cfg_lr == 0 || lr != cfg_lr || !IsDebuggerPresent()) {
    return false;
  }
  const uint32_t prior = fired.fetch_add(1, std::memory_order_relaxed);
  if (prior == 0) {
    __debugbreak();
    return true;
  }
  return false;
}

uint64_t NowSec() {
  using clock = std::chrono::steady_clock;
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch())
                      .count();
  return static_cast<uint64_t>(ns / 1000000000LL);
}

uint64_t NowNs() {
  using clock = std::chrono::steady_clock;
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch())
                      .count();
  return static_cast<uint64_t>(ns);
}

struct ProdGuardDiagState {
  std::atomic<uint64_t> entries{0};
  std::atomic<uint64_t> lr_82217e28{0};
  std::atomic<uint64_t> lr_82266490{0};
  std::atomic<uint64_t> lr_82289640{0};
  std::atomic<uint64_t> lr_8245d048{0};
  std::atomic<uint64_t> lr_other{0};
  std::atomic<uint32_t> last_lr{0};
  std::atomic<uint64_t> flag_blocked{0};
  std::atomic<uint64_t> cursor_eq{0};
  std::atomic<uint64_t> cursor_ne{0};
  std::atomic<uint64_t> wait_delta_lt64{0};
  std::atomic<uint64_t> wait_delta_64_255{0};
  std::atomic<uint64_t> wait_delta_256_1023{0};
  std::atomic<uint64_t> wait_delta_1024_4095{0};
  std::atomic<uint64_t> wait_delta_4096_4999{0};
  std::atomic<uint64_t> timeout_delta_ge5000{0};
  std::atomic<uint64_t> wait_delta_sum{0};
  std::atomic<uint64_t> timeout_delta_sum{0};
  std::atomic<uint64_t> wait_delta_max{0};
  std::atomic<uint64_t> timeout_delta_max{0};
  std::atomic<uint32_t> last_wait_delta{0};
  std::atomic<uint32_t> last_timeout_delta{0};
  std::atomic<uint64_t> sample_wait_logs{0};
  std::atomic<uint64_t> loop72_hits{0};
  std::atomic<uint64_t> loop72_guard_ret1{0};
  std::atomic<uint64_t> loop72_guard_ret0{0};
  std::atomic<uint64_t> loop72_obj_zero{0};
  std::atomic<uint64_t> loop72_limit_zero{0};
  std::atomic<uint64_t> loop72_cursor_zero{0};
  std::atomic<uint64_t> loop72_target_zero{0};
  std::atomic<uint64_t> loop72_need_lt_avail{0};
  std::atomic<uint64_t> loop72_need_ge_avail{0};
  std::atomic<uint32_t> loop72_last_obj{0};
  std::atomic<uint32_t> loop72_last_target{0};
  std::atomic<uint32_t> loop72_last_limit{0};
  std::atomic<uint32_t> loop72_last_cursor{0};
  std::atomic<uint32_t> loop72_last_need{0};
  std::atomic<uint32_t> loop72_last_avail{0};
  std::atomic<uint32_t> loop72_last_flag12944{0};
  std::atomic<uint64_t> loop72_small_gap_breaks{0};
  std::atomic<uint32_t> loop72_last_gap{0};
  std::atomic<uint32_t> loop72_last_break_threshold{0};
  std::atomic<uint64_t> loop72_yields{0};
  std::atomic<uint32_t> loop72_last_yield_interval{0};
  std::atomic<uint64_t> call73078_pre{0};
  std::atomic<uint64_t> call73078_post{0};
  std::atomic<uint64_t> call73078_changed{0};
  std::atomic<uint32_t> call73078_last_lim_before{0};
  std::atomic<uint32_t> call73078_last_cur_before{0};
  std::atomic<uint32_t> call73078_last_lim_after{0};
  std::atomic<uint32_t> call73078_last_cur_after{0};
  std::atomic<uint64_t> wait_ret1{0};
  std::atomic<uint64_t> timeout_call{0};
  std::atomic<uint64_t> ret0{0};
  std::atomic<uint64_t> last_emit_sec{0};
  std::mutex log_mutex;
  FILE* log_file = nullptr;
};

ProdGuardDiagState& ProdGuardDiag() {
  static ProdGuardDiagState state;
  return state;
}

void LogLineProdGuard(const char* fmt, ...) {
  auto& d = ProdGuardDiag();
  char line[2048];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);

  // Mirror through the main krnl logger so trace lines survive even if the
  // ad-hoc stdio handle path collides with the app's file sink.
  REXKRNL_ERROR("{}", line);

  std::lock_guard<std::mutex> lock(d.log_mutex);
  if (!d.log_file) {
    d.log_file = std::fopen("C:\\temp\\fm2-clean.log", "a");
    if (!d.log_file) {
      return;
    }
  }
  std::fputs(line, d.log_file);
  std::fputc('\n', d.log_file);
  std::fflush(d.log_file);
}

void MaybeEmitProdGuardPerSec() {
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  const uint64_t now_sec = NowSec();
  uint64_t last_sec = d.last_emit_sec.load(std::memory_order_relaxed);
  if (last_sec == 0) {
    d.last_emit_sec.store(now_sec, std::memory_order_relaxed);
    return;
  }
  if (now_sec == last_sec) {
    return;
  }
  if (!d.last_emit_sec.compare_exchange_strong(last_sec, now_sec, std::memory_order_relaxed)) {
    return;
  }

  const uint64_t wait_ret1 = d.wait_ret1.exchange(0, std::memory_order_relaxed);
  const uint64_t timeout_call = d.timeout_call.exchange(0, std::memory_order_relaxed);
  const uint64_t ret0 = d.ret0.exchange(0, std::memory_order_relaxed);
  const uint64_t total = wait_ret1 + ret0;
  const uint64_t ret0_non_timeout = ret0 > timeout_call ? (ret0 - timeout_call) : 0;
  const uint32_t wait_pct = total ? static_cast<uint32_t>((wait_ret1 * 100u) / total) : 0u;

  LogLineProdGuard(
      "FM2_PROD_GUARD_PERSEC sec=%llu total=%llu wait_ret1=%llu ret0=%llu "
      "timeout_call=%llu ret0_non_timeout=%llu wait_pct=%u",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(total),
      static_cast<unsigned long long>(wait_ret1), static_cast<unsigned long long>(ret0),
      static_cast<unsigned long long>(timeout_call),
      static_cast<unsigned long long>(ret0_non_timeout), wait_pct);

  if (REXCVAR_GET(fm2_prod_guard_trace)) {
    const uint64_t entries = d.entries.exchange(0, std::memory_order_relaxed);
    const uint64_t lr_82217e28 = d.lr_82217e28.exchange(0, std::memory_order_relaxed);
    const uint64_t lr_82266490 = d.lr_82266490.exchange(0, std::memory_order_relaxed);
    const uint64_t lr_82289640 = d.lr_82289640.exchange(0, std::memory_order_relaxed);
    const uint64_t lr_8245d048 = d.lr_8245d048.exchange(0, std::memory_order_relaxed);
    const uint64_t lr_other = d.lr_other.exchange(0, std::memory_order_relaxed);
    const uint64_t flag_blocked = d.flag_blocked.exchange(0, std::memory_order_relaxed);
    const uint64_t cursor_eq = d.cursor_eq.exchange(0, std::memory_order_relaxed);
    const uint64_t cursor_ne = d.cursor_ne.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_lt64 = d.wait_delta_lt64.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_64_255 = d.wait_delta_64_255.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_256_1023 = d.wait_delta_256_1023.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_1024_4095 =
        d.wait_delta_1024_4095.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_4096_4999 =
        d.wait_delta_4096_4999.exchange(0, std::memory_order_relaxed);
    const uint64_t timeout_delta_ge5000 =
        d.timeout_delta_ge5000.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_sum = d.wait_delta_sum.exchange(0, std::memory_order_relaxed);
    const uint64_t timeout_delta_sum = d.timeout_delta_sum.exchange(0, std::memory_order_relaxed);
    const uint64_t wait_delta_max = d.wait_delta_max.exchange(0, std::memory_order_relaxed);
    const uint64_t timeout_delta_max = d.timeout_delta_max.exchange(0, std::memory_order_relaxed);
    const uint32_t last_wait_delta = d.last_wait_delta.load(std::memory_order_relaxed);
    const uint32_t last_timeout_delta = d.last_timeout_delta.load(std::memory_order_relaxed);
    const uint32_t last_lr = d.last_lr.load(std::memory_order_relaxed);
    const uint64_t sample_wait_logs = d.sample_wait_logs.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_hits = d.loop72_hits.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_guard_ret1 = d.loop72_guard_ret1.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_guard_ret0 = d.loop72_guard_ret0.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_obj_zero = d.loop72_obj_zero.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_limit_zero = d.loop72_limit_zero.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_cursor_zero = d.loop72_cursor_zero.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_target_zero = d.loop72_target_zero.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_need_lt_avail =
        d.loop72_need_lt_avail.exchange(0, std::memory_order_relaxed);
    const uint64_t loop72_need_ge_avail =
        d.loop72_need_ge_avail.exchange(0, std::memory_order_relaxed);
    const uint32_t loop72_last_obj = d.loop72_last_obj.load(std::memory_order_relaxed);
    const uint32_t loop72_last_target = d.loop72_last_target.load(std::memory_order_relaxed);
    const uint32_t loop72_last_limit = d.loop72_last_limit.load(std::memory_order_relaxed);
    const uint32_t loop72_last_cursor = d.loop72_last_cursor.load(std::memory_order_relaxed);
    const uint32_t loop72_last_need = d.loop72_last_need.load(std::memory_order_relaxed);
    const uint32_t loop72_last_avail = d.loop72_last_avail.load(std::memory_order_relaxed);
    const uint32_t loop72_last_flag12944 = d.loop72_last_flag12944.load(std::memory_order_relaxed);
    const uint64_t loop72_small_gap_breaks =
        d.loop72_small_gap_breaks.exchange(0, std::memory_order_relaxed);
    const uint32_t loop72_last_gap = d.loop72_last_gap.load(std::memory_order_relaxed);
    const uint32_t loop72_last_break_threshold =
        d.loop72_last_break_threshold.load(std::memory_order_relaxed);
    const uint64_t loop72_yields = d.loop72_yields.exchange(0, std::memory_order_relaxed);
    const uint32_t loop72_last_yield_interval =
        d.loop72_last_yield_interval.load(std::memory_order_relaxed);
    const uint64_t call73078_pre = d.call73078_pre.exchange(0, std::memory_order_relaxed);
    const uint64_t call73078_post = d.call73078_post.exchange(0, std::memory_order_relaxed);
    const uint64_t call73078_changed = d.call73078_changed.exchange(0, std::memory_order_relaxed);
    const uint32_t call73078_last_lim_before =
        d.call73078_last_lim_before.load(std::memory_order_relaxed);
    const uint32_t call73078_last_cur_before =
        d.call73078_last_cur_before.load(std::memory_order_relaxed);
    const uint32_t call73078_last_lim_after =
        d.call73078_last_lim_after.load(std::memory_order_relaxed);
    const uint32_t call73078_last_cur_after =
        d.call73078_last_cur_after.load(std::memory_order_relaxed);
    const uint64_t avg_wait_delta = wait_ret1 ? (wait_delta_sum / wait_ret1) : 0u;
    const uint64_t avg_timeout_delta = timeout_call ? (timeout_delta_sum / timeout_call) : 0u;

    LogLineProdGuard(
        "FM2_PROD_GUARD_TRACE_PERSEC sec=%llu entries=%llu lr82217e28=%llu lr82266490=%llu "
        "lr82289640=%llu lr8245d048=%llu lr_other=%llu flag_blocked=%llu cursor_eq=%llu "
        "cursor_ne=%llu wait_d_lt64=%llu wait_d_64_255=%llu wait_d_256_1023=%llu "
        "wait_d_1024_4095=%llu wait_d_4096_4999=%llu timeout_d_ge5000=%llu avg_wait_d=%llu "
        "max_wait_d=%llu avg_timeout_d=%llu max_timeout_d=%llu last_wait_d=%u "
        "last_timeout_d=%u last_lr=%08X sample_wait_logs=%llu",
        static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(entries),
        static_cast<unsigned long long>(lr_82217e28), static_cast<unsigned long long>(lr_82266490),
        static_cast<unsigned long long>(lr_82289640), static_cast<unsigned long long>(lr_8245d048),
        static_cast<unsigned long long>(lr_other), static_cast<unsigned long long>(flag_blocked),
        static_cast<unsigned long long>(cursor_eq), static_cast<unsigned long long>(cursor_ne),
        static_cast<unsigned long long>(wait_delta_lt64),
        static_cast<unsigned long long>(wait_delta_64_255),
        static_cast<unsigned long long>(wait_delta_256_1023),
        static_cast<unsigned long long>(wait_delta_1024_4095),
        static_cast<unsigned long long>(wait_delta_4096_4999),
        static_cast<unsigned long long>(timeout_delta_ge5000),
        static_cast<unsigned long long>(avg_wait_delta),
        static_cast<unsigned long long>(wait_delta_max),
        static_cast<unsigned long long>(avg_timeout_delta),
        static_cast<unsigned long long>(timeout_delta_max), last_wait_delta, last_timeout_delta,
        static_cast<unsigned int>(last_lr), static_cast<unsigned long long>(sample_wait_logs));

    LogLineProdGuard(
        "FM2_PROD_WAITLOOP_72A70 sec=%llu hits=%llu ret1=%llu ret0=%llu obj0=%llu lim0=%llu "
        "cur0=%llu tgt0=%llu need_lt_avail=%llu need_ge_avail=%llu small_break=%llu yields=%llu "
        "last(obj=%08X tgt=%u lim=%u cur=%u need=%u avail=%u gap=%u break_th=%u yield_int=%u f12944=%u) "
        "call73078(pre=%llu post=%llu changed=%llu lim:%u->%u cur:%u->%u)",
        static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(loop72_hits),
        static_cast<unsigned long long>(loop72_guard_ret1),
        static_cast<unsigned long long>(loop72_guard_ret0),
        static_cast<unsigned long long>(loop72_obj_zero),
        static_cast<unsigned long long>(loop72_limit_zero),
        static_cast<unsigned long long>(loop72_cursor_zero),
        static_cast<unsigned long long>(loop72_target_zero),
        static_cast<unsigned long long>(loop72_need_lt_avail),
        static_cast<unsigned long long>(loop72_need_ge_avail),
        static_cast<unsigned long long>(loop72_small_gap_breaks),
        static_cast<unsigned long long>(loop72_yields),
        static_cast<unsigned int>(loop72_last_obj), loop72_last_target, loop72_last_limit,
        loop72_last_cursor, loop72_last_need, loop72_last_avail, loop72_last_gap,
        loop72_last_break_threshold, loop72_last_yield_interval, loop72_last_flag12944,
        static_cast<unsigned long long>(call73078_pre),
        static_cast<unsigned long long>(call73078_post),
        static_cast<unsigned long long>(call73078_changed), call73078_last_lim_before,
        call73078_last_lim_after, call73078_last_cur_before, call73078_last_cur_after);
  }
}

void ProdGuardHitCaller(uint32_t lr) {
  auto& d = ProdGuardDiag();
  d.entries.fetch_add(1, std::memory_order_relaxed);
  d.last_lr.store(lr, std::memory_order_relaxed);
  switch (lr) {
    case 0x82217E28u:
      d.lr_82217e28.fetch_add(1, std::memory_order_relaxed);
      break;
    case 0x82266490u:
      d.lr_82266490.fetch_add(1, std::memory_order_relaxed);
      break;
    case 0x82289640u:
      d.lr_82289640.fetch_add(1, std::memory_order_relaxed);
      break;
    case 0x8245D048u:
      d.lr_8245d048.fetch_add(1, std::memory_order_relaxed);
      break;
    default:
      d.lr_other.fetch_add(1, std::memory_order_relaxed);
      break;
  }
}

void AtomicMaxU64(std::atomic<uint64_t>& slot, uint64_t value) {
  uint64_t current = slot.load(std::memory_order_relaxed);
  while (current < value &&
         !slot.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
  }
}

struct ApuMixDiagState {
  std::atomic<uint64_t> calls{0};
  std::atomic<uint64_t> exits_a{0};
  std::atomic<uint64_t> exits_b{0};
  std::atomic<uint64_t> total_ns{0};
  std::atomic<uint64_t> max_ns{0};
  std::atomic<uint64_t> unmatched_exit{0};
  std::atomic<uint64_t> stack_overflow{0};
  std::atomic<uint64_t> last_emit_sec{0};
};

ApuMixDiagState& ApuMixDiag() {
  static ApuMixDiagState state;
  return state;
}

constexpr uint32_t kApuMixMaxDepth = 64u;
thread_local uint64_t g_apu_mix_enter_ns[kApuMixMaxDepth] = {};
thread_local uint32_t g_apu_mix_depth = 0u;
thread_local uint32_t g_apu_mix_dropped = 0u;

void MaybeEmitApuMixPerSec() {
  if (!REXCVAR_GET(fm2_apu_mix_stats)) {
    return;
  }
  auto& d = ApuMixDiag();
  const uint64_t now_sec = NowSec();
  uint64_t last_sec = d.last_emit_sec.load(std::memory_order_relaxed);
  if (last_sec == 0) {
    d.last_emit_sec.store(now_sec, std::memory_order_relaxed);
    return;
  }
  if (now_sec == last_sec) {
    return;
  }
  if (!d.last_emit_sec.compare_exchange_strong(last_sec, now_sec, std::memory_order_relaxed)) {
    return;
  }

  const uint64_t calls = d.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t exits_a = d.exits_a.exchange(0, std::memory_order_relaxed);
  const uint64_t exits_b = d.exits_b.exchange(0, std::memory_order_relaxed);
  const uint64_t exits = exits_a + exits_b;
  const uint64_t total_ns = d.total_ns.exchange(0, std::memory_order_relaxed);
  const uint64_t max_ns = d.max_ns.exchange(0, std::memory_order_relaxed);
  const uint64_t unmatched_exit = d.unmatched_exit.exchange(0, std::memory_order_relaxed);
  const uint64_t stack_overflow = d.stack_overflow.exchange(0, std::memory_order_relaxed);
  const uint64_t total_us = total_ns / 1000u;
  const uint64_t avg_us = exits ? (total_ns / exits) / 1000u : 0u;
  const uint64_t max_us = max_ns / 1000u;
  const long long inflight_delta =
      static_cast<long long>(calls) - static_cast<long long>(exits);

  LogLineProdGuard(
      "FM2_APU_MIX_PERSEC sec=%llu calls=%llu exits=%llu exit_a=%llu exit_b=%llu "
      "total_us=%llu avg_us=%llu max_us=%llu inflight_delta=%lld unmatched_exit=%llu "
      "stack_ovf=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(calls),
      static_cast<unsigned long long>(exits), static_cast<unsigned long long>(exits_a),
      static_cast<unsigned long long>(exits_b), static_cast<unsigned long long>(total_us),
      static_cast<unsigned long long>(avg_us), static_cast<unsigned long long>(max_us),
      inflight_delta, static_cast<unsigned long long>(unmatched_exit),
      static_cast<unsigned long long>(stack_overflow));
}

struct LoadTraceState {
  std::atomic<uint32_t> active{0};
  std::atomic<uint32_t> hotkey_down{0};
  std::atomic<uint32_t> current_session{0};
  std::atomic<uint32_t> next_session{0};
  std::atomic<uint32_t> autostart_fired{0};
  std::atomic<uint32_t> poll_init_logged{0};
  std::atomic<uint64_t> boot_ns{0};
  std::atomic<uint64_t> start_ns{0};
  std::atomic<uint64_t> helper_637f8{0};
  std::atomic<uint64_t> helper_63768{0};
  std::atomic<uint64_t> helper_67f60{0};
  std::atomic<uint64_t> helper_63538{0};
  std::atomic<uint64_t> alloc_pool_req_bytes{0};
  std::atomic<uint64_t> alloc_pool_req_max{0};
  std::atomic<uint64_t> alloc_pool_req_le32{0};
  std::atomic<uint64_t> alloc_pool_req_33_64{0};
  std::atomic<uint64_t> alloc_pool_req_65_128{0};
  std::atomic<uint64_t> alloc_pool_req_129_512{0};
  std::atomic<uint64_t> alloc_pool_req_gt512{0};
  std::atomic<uint64_t> alloc_pool_fast_hit{0};
  std::atomic<uint64_t> alloc_pool_fast_miss{0};
  std::atomic<uint64_t> alloc_pool_fallback_calls{0};
  std::atomic<uint64_t> alloc_pool_fallback_hit{0};
  std::atomic<uint64_t> alloc_pool_fallback_fail{0};
  std::atomic<uint64_t> alloc_1d03e8{0};
  std::atomic<uint64_t> alloc_1d0e10{0};
  std::atomic<uint64_t> alloc_1d1568{0};
  std::atomic<uint64_t> str_24d8{0};
  std::atomic<uint64_t> str_25c0{0};
  std::atomic<uint64_t> str_30c10{0};
  std::atomic<uint64_t> path_5cf298{0};
  std::atomic<uint64_t> gate_344c0_entry{0};
  std::atomic<uint64_t> gate_344c0_match{0};
  std::atomic<uint64_t> buffered_async{0};
  std::atomic<uint64_t> buffered_sync{0};
  std::atomic<uint64_t> prod_guard_wait{0};
  std::atomic<uint64_t> prod_guard_timeout{0};
  std::atomic<uint64_t> prod_guard_ret0{0};
  std::atomic<uint64_t> path_samples{0};
  std::atomic<uint64_t> read_samples{0};
};

LoadTraceState& LoadTrace() {
  static LoadTraceState state;
  return state;
}

void SetLoadTraceOverlayState(uint32_t state) {
  char value[16];
  std::snprintf(value, sizeof(value), "%u", state);
  rex::cvar::SetFlagByName("fm2_load_trace_overlay_state", value);
}

void SyncLoadTraceOverlayState() {
  const uint32_t state =
      !REXCVAR_GET(fm2_load_trace) ? 0u : (LoadTrace().active.load(std::memory_order_relaxed) != 0u ? 2u : 1u);
  SetLoadTraceOverlayState(state);
}

void ResetLoadTraceCounters() {
  auto& d = LoadTrace();
  d.helper_637f8.store(0, std::memory_order_relaxed);
  d.helper_63768.store(0, std::memory_order_relaxed);
  d.helper_67f60.store(0, std::memory_order_relaxed);
  d.helper_63538.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_bytes.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_max.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_le32.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_33_64.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_65_128.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_129_512.store(0, std::memory_order_relaxed);
  d.alloc_pool_req_gt512.store(0, std::memory_order_relaxed);
  d.alloc_pool_fast_hit.store(0, std::memory_order_relaxed);
  d.alloc_pool_fast_miss.store(0, std::memory_order_relaxed);
  d.alloc_pool_fallback_calls.store(0, std::memory_order_relaxed);
  d.alloc_pool_fallback_hit.store(0, std::memory_order_relaxed);
  d.alloc_pool_fallback_fail.store(0, std::memory_order_relaxed);
  d.alloc_1d03e8.store(0, std::memory_order_relaxed);
  d.alloc_1d0e10.store(0, std::memory_order_relaxed);
  d.alloc_1d1568.store(0, std::memory_order_relaxed);
  d.str_24d8.store(0, std::memory_order_relaxed);
  d.str_25c0.store(0, std::memory_order_relaxed);
  d.str_30c10.store(0, std::memory_order_relaxed);
  d.path_5cf298.store(0, std::memory_order_relaxed);
  d.gate_344c0_entry.store(0, std::memory_order_relaxed);
  d.gate_344c0_match.store(0, std::memory_order_relaxed);
  d.buffered_async.store(0, std::memory_order_relaxed);
  d.buffered_sync.store(0, std::memory_order_relaxed);
  d.prod_guard_wait.store(0, std::memory_order_relaxed);
  d.prod_guard_timeout.store(0, std::memory_order_relaxed);
  d.prod_guard_ret0.store(0, std::memory_order_relaxed);
  d.path_samples.store(0, std::memory_order_relaxed);
  d.read_samples.store(0, std::memory_order_relaxed);
}

bool IsLoadTraceActive() {
  return LoadTrace().active.load(std::memory_order_relaxed) != 0u;
}

void HitLoadTraceAllocPoolRequest(uint32_t size_bytes) {
  auto& d = LoadTrace();
  d.alloc_pool_req_bytes.fetch_add(size_bytes, std::memory_order_relaxed);
  AtomicMaxU64(d.alloc_pool_req_max, size_bytes);
  if (size_bytes <= 32u) {
    d.alloc_pool_req_le32.fetch_add(1, std::memory_order_relaxed);
  } else if (size_bytes <= 64u) {
    d.alloc_pool_req_33_64.fetch_add(1, std::memory_order_relaxed);
  } else if (size_bytes <= 128u) {
    d.alloc_pool_req_65_128.fetch_add(1, std::memory_order_relaxed);
  } else if (size_bytes <= 512u) {
    d.alloc_pool_req_129_512.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.alloc_pool_req_gt512.fetch_add(1, std::memory_order_relaxed);
  }
}

void EmitLoadTraceSummary(uint32_t session, const char* reason) {
  auto& d = LoadTrace();
  const uint64_t start_ns = d.start_ns.load(std::memory_order_relaxed);
  const uint64_t duration_ms = start_ns ? ((NowNs() - start_ns) / 1000000u) : 0u;
  LogLineProdGuard(
      "FM2_LOAD_TRACE_SUMMARY session=%u reason=%s dur_ms=%llu helper637f8=%llu helper63768=%llu "
      "helper67f60=%llu helper63538=%llu pool_req_bytes=%llu pool_req_max=%llu "
      "pool_sz_le32=%llu pool_sz_33_64=%llu pool_sz_65_128=%llu pool_sz_129_512=%llu "
      "pool_sz_gt512=%llu pool_fast_hit=%llu pool_fast_miss=%llu pool_fallback=%llu "
      "pool_fallback_hit=%llu pool_fallback_fail=%llu alloc03e8=%llu alloc0e10=%llu alloc1568=%llu "
      "str24d8=%llu str25c0=%llu str30c10=%llu path5cf298=%llu gate344c0=%llu "
      "gate344c0_match=%llu read_async=%llu read_sync=%llu prod_wait=%llu prod_timeout=%llu "
      "prod_ret0=%llu path_samples=%llu read_samples=%llu",
      session, reason ? reason : "stop", static_cast<unsigned long long>(duration_ms),
      static_cast<unsigned long long>(d.helper_637f8.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.helper_63768.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.helper_67f60.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.helper_63538.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_bytes.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_max.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_le32.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_33_64.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_65_128.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_129_512.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_req_gt512.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_fast_hit.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_fast_miss.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_fallback_calls.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_fallback_hit.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_pool_fallback_fail.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_1d03e8.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_1d0e10.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.alloc_1d1568.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.str_24d8.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.str_25c0.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.str_30c10.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.path_5cf298.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.gate_344c0_entry.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.gate_344c0_match.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.buffered_async.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.buffered_sync.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.prod_guard_wait.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.prod_guard_timeout.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.prod_guard_ret0.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.path_samples.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(d.read_samples.load(std::memory_order_relaxed)));
}

void StopLoadTraceSession(const char* reason) {
  auto& d = LoadTrace();
  if (d.active.exchange(0, std::memory_order_relaxed) == 0u) {
    return;
  }
  SyncLoadTraceOverlayState();
  const uint32_t session = d.current_session.load(std::memory_order_relaxed);
  EmitLoadTraceSummary(session, reason);
  LogLineProdGuard("FM2_LOAD_TRACE_STOP session=%u reason=%s", session,
                   reason ? reason : "stop");
}

void StartLoadTraceSession(const char* reason) {
  auto& d = LoadTrace();
  ResetLoadTraceCounters();
  const uint32_t session = d.next_session.fetch_add(1, std::memory_order_relaxed) + 1u;
  d.current_session.store(session, std::memory_order_relaxed);
  d.start_ns.store(NowNs(), std::memory_order_relaxed);
  d.active.store(1, std::memory_order_relaxed);
  SyncLoadTraceOverlayState();
  LogLineProdGuard("FM2_LOAD_TRACE_START session=%u reason=%s vk=%u autostart_ms=%u", session,
                   reason ? reason : "start", REXCVAR_GET(fm2_load_trace_toggle_vk),
                   REXCVAR_GET(fm2_load_trace_autostart_ms));
}

void MaybePollLoadTraceToggle() {
  auto& d = LoadTrace();
  const uint64_t now_ns = NowNs();
  uint64_t expected_boot_ns = 0;
  d.boot_ns.compare_exchange_strong(expected_boot_ns, now_ns, std::memory_order_relaxed);
  if (d.poll_init_logged.exchange(1u, std::memory_order_relaxed) == 0u) {
    LogLineProdGuard(
        "FM2_LOAD_TRACE_POLL_INIT enabled=%u autostart_ms=%u vk=%u sample_limit=%u",
        REXCVAR_GET(fm2_load_trace) ? 1u : 0u, REXCVAR_GET(fm2_load_trace_autostart_ms),
        REXCVAR_GET(fm2_load_trace_toggle_vk), REXCVAR_GET(fm2_load_trace_sample_limit));
  }
  if (!REXCVAR_GET(fm2_load_trace)) {
    SetLoadTraceOverlayState(0u);
    d.hotkey_down.store(0, std::memory_order_relaxed);
    d.autostart_fired.store(0, std::memory_order_relaxed);
    if (d.active.load(std::memory_order_relaxed) != 0u) {
      StopLoadTraceSession("disabled");
    }
    return;
  }

  if (d.active.load(std::memory_order_relaxed) == 0u) {
    SetLoadTraceOverlayState(1u);
  }

  const uint32_t autostart_ms = REXCVAR_GET(fm2_load_trace_autostart_ms);
  if (autostart_ms != 0u && d.active.load(std::memory_order_relaxed) == 0u &&
      d.autostart_fired.load(std::memory_order_relaxed) == 0u) {
    const uint64_t boot_ns = d.boot_ns.load(std::memory_order_relaxed);
    const uint64_t elapsed_ms = boot_ns ? ((now_ns - boot_ns) / 1000000u) : 0u;
    if (elapsed_ms >= autostart_ms) {
      d.autostart_fired.store(1, std::memory_order_relaxed);
      StartLoadTraceSession("autostart");
      return;
    }
  }

  const uint32_t vk = REXCVAR_GET(fm2_load_trace_toggle_vk);
  if (vk == 0u) {
    d.hotkey_down.store(0, std::memory_order_relaxed);
    return;
  }

  const bool is_down = (GetAsyncKeyState(static_cast<int>(vk)) & 0x8000) != 0;
  const uint32_t was_down = d.hotkey_down.exchange(is_down ? 1u : 0u, std::memory_order_relaxed);
  if (!is_down || was_down != 0u) {
    return;
  }

  if (d.active.load(std::memory_order_relaxed) != 0u) {
    StopLoadTraceSession("toggle");
  } else {
    StartLoadTraceSession("toggle");
  }
}

void HitLoadTraceCounter(std::atomic<uint64_t>& counter) {
  MaybePollLoadTraceToggle();
  if (!IsLoadTraceActive()) {
    return;
  }
  counter.fetch_add(1, std::memory_order_relaxed);
}

void MaybeLogLoadTracePathSample(uint32_t lr, uint32_t flag, uint32_t path_ptr) {
  if (!IsLoadTraceActive()) {
    return;
  }
  auto& d = LoadTrace();
  const uint64_t sample_ix = d.path_samples.fetch_add(1, std::memory_order_relaxed);
  if (sample_ix >= REXCVAR_GET(fm2_load_trace_sample_limit)) {
    return;
  }
  char path[65];
  SnapshotGuestCString(path_ptr, path);
  LogLineProdGuard("FM2_LOAD_TRACE_PATH session=%u n=%llu lr=%08X flag=%u path=%s",
                   d.current_session.load(std::memory_order_relaxed),
                   static_cast<unsigned long long>(sample_ix + 1), lr, flag, path);
}

void MaybeLogLoadTraceReadSample(const char* site, uint32_t r3, uint32_t r4, uint32_t r5) {
  if (!IsLoadTraceActive()) {
    return;
  }
  auto& d = LoadTrace();
  const uint64_t sample_ix = d.read_samples.fetch_add(1, std::memory_order_relaxed);
  if (sample_ix >= REXCVAR_GET(fm2_load_trace_sample_limit)) {
    return;
  }
  LogLineProdGuard("FM2_LOAD_TRACE_READ session=%u n=%llu site=%s r3=%08X r4=%08X r5=%08X",
                   d.current_session.load(std::memory_order_relaxed),
                   static_cast<unsigned long long>(sample_ix + 1), site, r3, r4, r5);
}

struct SigSiteDiagState {
  bool enabled = true;
  bool force_a4e8_setevent_every_call = false;
  bool force_sched_mode2 = false;
  bool force_submit_mode3 = false;
  bool force_pump_wait_override = true;
  int32_t force_pump_wait_ms = 8;
  bool force_read_size_4096 = false;
  std::atomic<uint64_t> a56c_count{0};
  std::atomic<uint64_t> s9968_count{0};
  std::atomic<uint64_t> s9d10_count{0};
  std::atomic<uint64_t> s9ffc_count{0};
  std::atomic<uint64_t> s86a40_count{0};
  std::atomic<uint64_t> p898f8_count{0};
  std::atomic<uint64_t> p89be0_count{0};
  std::atomic<uint64_t> p89e88_count{0};
  std::atomic<uint64_t> p86988_count{0};
  std::atomic<uint64_t> h87678_count{0};
  std::atomic<uint64_t> h637f8_count{0};
  std::atomic<uint64_t> h53d718_count{0};
  std::atomic<uint64_t> c82214cf0_count{0};
  std::atomic<uint64_t> c82599a88_count{0};
  std::atomic<uint64_t> c821e38c8_count{0};
  std::atomic<uint64_t> c8229e368_count{0};
  std::atomic<uint64_t> c8235f3d8_count{0};
  std::atomic<uint64_t> h63768_count{0};
  std::atomic<uint64_t> h63768_total_count{0};
  std::atomic<uint64_t> h67f60_count{0};
  std::atomic<uint64_t> h63538_count{0};
  std::atomic<uint64_t> lr821d0448_count{0};
  std::atomic<uint64_t> lr8259f3a0_count{0};
  std::atomic<uint64_t> lr825345a8_count{0};
  std::atomic<uint64_t> lr822097c8_count{0};
  std::atomic<uint64_t> lr_other_count{0};
  std::atomic<uint64_t> b_workitem_missing{0};
  std::atomic<uint64_t> b_alloc_fail{0};
  std::atomic<uint64_t> b821d_r31_zero{0};
  std::atomic<uint64_t> b821d_r31_nonzero{0};
  std::atomic<uint64_t> b821d_div_ge_1{0};
  std::atomic<uint64_t> b821d_div_lt_1{0};
  std::atomic<uint64_t> b8259f_r31_zero{0};
  std::atomic<uint64_t> b8259f_r31_nonzero{0};
  std::atomic<uint64_t> b8259f_div_ge_8{0};
  std::atomic<uint64_t> b8259f_div_lt_8{0};
  std::atomic<uint64_t> b825345_r31_zero{0};
  std::atomic<uint64_t> b825345_r31_nonzero{0};
  std::atomic<uint64_t> b825345_div_ge_52{0};
  std::atomic<uint64_t> b825345_div_lt_52{0};
  std::atomic<uint64_t> b82209038_flag_zero{0};
  std::atomic<uint64_t> b82209038_flag_nonzero{0};
  std::atomic<uint64_t> b82209038_cmp_eq_m1{0};
  std::atomic<uint64_t> b82209038_cmp_ne_m1{0};
  std::atomic<uint64_t> b82209038_path_97c4{0};
  std::atomic<uint64_t> b82209038_path_9840{0};
  std::atomic<uint64_t> d_a528_entry{0};
  std::atomic<uint64_t> d_a528_gate_true{0};
  std::atomic<uint64_t> d_a528_gate_false{0};
  std::atomic<uint64_t> d_a528_mode5{0};
  std::atomic<uint64_t> d_a528_mode6{0};
  std::atomic<uint64_t> d_a528_mode7{0};
  std::atomic<uint64_t> d_a528_mode8{0};
  std::atomic<uint64_t> d_a528_mode9{0};
  std::atomic<uint64_t> d_a528_mode_other{0};
  std::atomic<uint64_t> d_a528_m5_match{0};
  std::atomic<uint64_t> d_a528_m5_miss{0};
  std::atomic<uint64_t> d_a528_m6_match{0};
  std::atomic<uint64_t> d_a528_m6_miss{0};
  std::atomic<uint64_t> d_a528_m7_match{0};
  std::atomic<uint64_t> d_a528_m7_miss{0};
  std::atomic<uint64_t> d_a528_m8_match{0};
  std::atomic<uint64_t> d_a528_m8_miss{0};
  std::atomic<uint64_t> d_a528_m9_match{0};
  std::atomic<uint64_t> d_a528_m9_miss{0};
  std::atomic<uint64_t> d_a528_route_882e0{0};
  std::atomic<uint64_t> d_a528_route_89e88{0};
  std::atomic<uint64_t> d_a528_route_883e0{0};
  std::atomic<uint64_t> d_a528_route_89990{0};
  std::atomic<uint64_t> d_a528_route_89978{0};
  std::atomic<uint64_t> d_slot136_total{0};
  std::atomic<uint64_t> d_slot136_to_a528{0};
  std::atomic<uint64_t> d_slot136_to_other{0};
  std::atomic<uint32_t> d_slot136_last_target{0};
  std::atomic<uint64_t> d_7310_entry{0};
  std::atomic<uint64_t> d_7310_pass{0};
  std::atomic<uint64_t> d_7310_fail1{0};
  std::atomic<uint64_t> d_7310_fail2{0};
  std::atomic<uint64_t> d_7310_fail3{0};
  std::atomic<uint64_t> d_7310_r148_eq1{0};
  std::atomic<uint64_t> d_7310_r148_ne1{0};
  std::atomic<uint32_t> d_7310_r148_last_target{0};
  std::atomic<uint64_t> d_344c0_entry{0};
  std::atomic<uint64_t> d_344c0_loop_end{0};
  std::atomic<uint64_t> d_344c0_name_eq{0};
  std::atomic<uint64_t> d_344c0_name_ne{0};
  std::atomic<uint64_t> d_344c0_match{0};
  std::atomic<uint64_t> d_344c0_sample_count{0};
  std::atomic<uint64_t> d_alloc_cs_0e70_count{0};
  std::atomic<uint64_t> d_alloc_cs_7578_count{0};
  std::atomic<uint64_t> d_alloc_cs_8c98_count{0};
  std::atomic<uint64_t> d_alloc_cs_0e70_lt32{0};
  std::atomic<uint64_t> d_alloc_cs_0e70_32_255{0};
  std::atomic<uint64_t> d_alloc_cs_0e70_256_4095{0};
  std::atomic<uint64_t> d_alloc_cs_0e70_ge4096{0};
  std::atomic<uint64_t> d_alloc_cs_7578_lt32{0};
  std::atomic<uint64_t> d_alloc_cs_7578_32_255{0};
  std::atomic<uint64_t> d_alloc_cs_7578_256_4095{0};
  std::atomic<uint64_t> d_alloc_cs_7578_ge4096{0};
  std::atomic<uint64_t> d_alloc_cs_8c98_lt32{0};
  std::atomic<uint64_t> d_alloc_cs_8c98_32_255{0};
  std::atomic<uint64_t> d_alloc_cs_8c98_256_4095{0};
  std::atomic<uint64_t> d_alloc_cs_8c98_ge4096{0};
  std::atomic<uint32_t> d_alloc_cs_0e70_last_size{0};
  std::atomic<uint32_t> d_alloc_cs_7578_last_size{0};
  std::atomic<uint32_t> d_alloc_cs_8c98_last_size{0};
  std::atomic<uint64_t> d_1d03e8_entry{0};
  std::atomic<uint64_t> d_1d03e8_lr_0e74{0};
  std::atomic<uint64_t> d_1d03e8_lr_757c{0};
  std::atomic<uint64_t> d_1d03e8_lr_8c9c{0};
  std::atomic<uint64_t> d_1d03e8_lr_other{0};
  std::atomic<uint64_t> d_1d03e8_other_samples{0};
  std::atomic<uint32_t> d_1d03e8_last_lr{0};
  std::atomic<uint32_t> d_1d03e8_last_size{0};
  std::atomic<uint64_t> d_1d0e10_entry{0};
  std::atomic<uint64_t> d_1d0e10_lr_other{0};
  std::atomic<uint64_t> d_1d0e10_lr_sample_count{0};
  std::atomic<uint32_t> d_1d0e10_last_lr{0};
  std::atomic<uint32_t> d_1d0e10_last_req{0};
  std::atomic<uint32_t> d_1d0e10_last_copy{0};
  std::atomic<uint64_t> d_1d1568_entry{0};
  std::atomic<uint64_t> d_1d1568_lr_other{0};
  std::atomic<uint64_t> d_1d1568_lr_sample_count{0};
  std::atomic<uint32_t> d_1d1568_last_lr{0};
  std::atomic<uint32_t> d_1d1568_last_req{0};
  std::atomic<uint32_t> d_1d1568_last_flag{0};
  std::atomic<uint64_t> d_1d25c0_entry{0};
  std::atomic<uint64_t> d_1d25c0_samples{0};
  std::atomic<uint32_t> d_1d25c0_last_lr{0};
  std::atomic<uint64_t> d_1d24d8_entry{0};
  std::atomic<uint64_t> d_1d24d8_samples{0};
  std::atomic<uint32_t> d_1d24d8_last_lr{0};
  std::atomic<uint64_t> d_430c10_entry{0};
  std::atomic<uint64_t> d_430c10_samples{0};
  std::atomic<uint32_t> d_430c10_last_lr{0};
  std::atomic<uint64_t> d_5cf298_entry{0};
  std::atomic<uint64_t> d_5cf298_samples{0};
  std::atomic<uint32_t> d_5cf298_last_lr{0};
  std::atomic<uint32_t> d_5cf298_last_flag{0};
  std::atomic<uint64_t> reg_snapshot_count{0};
  std::atomic<uint32_t> fmod_worker_tid{0};
  std::atomic<uint64_t> fmod_worker_bind_events{0};
  std::atomic<uint64_t> fmod_auto_bind_events{0};
  std::atomic<uint64_t> fmod_hook_loop_hits{0};
  std::atomic<uint64_t> fmod_hook_gate_hits{0};
  std::atomic<uint64_t> fmod_hook_wait_hits{0};
  std::atomic<uint64_t> fmod_hook_work_hits{0};
  std::atomic<uint64_t> fmod_hook_decode_hits{0};
  std::atomic<uint64_t> fmod_render_cb_hits{0};
  std::atomic<uint64_t> fmod_render_dispatch_hits{0};
  std::atomic<uint64_t> fmod_decode_thunk_hits{0};
  std::atomic<uint64_t> fmod_fill_hits{0};
  std::atomic<uint64_t> fmod_sched_eb998_hits{0};
  std::atomic<uint64_t> fmod_sched_eb9d0_hits{0};
  std::atomic<uint64_t> fmod_irq_6c380_hits{0};
  std::atomic<uint64_t> fmod_irq_6c380_eq_hits{0};
  std::atomic<uint64_t> fmod_irq_sched_6c4f0_hits{0};
  std::atomic<uint64_t> fmod_irq_sched_over_thr_hits{0};
  std::atomic<uint64_t> fmod_irq_sched_path_immediate_hits{0};
  std::atomic<uint64_t> fmod_irq_sched_path_queued_hits{0};
  std::atomic<uint64_t> fmod_irq_sched_mode_hist[16]{};
  std::atomic<uint64_t> fmod_irq_sched_thr_hist[16]{};
  std::atomic<uint64_t> fmod_irq_submit_6c688_hits{0};
  std::atomic<uint64_t> fmod_irq_submit_a2_20_hits{0};
  std::atomic<uint64_t> fmod_irq_submit_a2_other_hits{0};
  std::atomic<uint32_t> fmod_irq_submit_last_a2{0};
  std::atomic<uint64_t> fmod_irq_submit_lr_a_hits{0};
  std::atomic<uint64_t> fmod_irq_submit_lr_b_hits{0};
  std::atomic<uint64_t> fmod_irq_submit_lr_other_hits{0};
  std::atomic<uint32_t> fmod_irq_submit_last_lr{0};
  std::atomic<uint64_t> fmod_wrap_6c8c8_hits{0};
  std::atomic<uint64_t> fmod_wrap_6c948_hits{0};
  std::atomic<uint64_t> fmod_wrap_6cb20_hits{0};
  std::atomic<uint64_t> fmod_read_calls{0};
  std::atomic<uint64_t> fmod_read_req_bytes{0};
  std::atomic<uint64_t> fmod_read_req_forced_4096{0};
  std::atomic<uint64_t> fmod_read_copy1_calls{0};
  std::atomic<uint64_t> fmod_read_copy1_bytes{0};
  std::atomic<uint64_t> fmod_read_copy2_calls{0};
  std::atomic<uint64_t> fmod_read_copy2_bytes{0};
  std::atomic<uint64_t> fmod_read_ctx_ffca8300_hits{0};
  std::atomic<uint32_t> fmod_read_last_req{0};
  std::atomic<uint32_t> fmod_read_last_ctx{0};
  std::atomic<uint32_t> fmod_read_last_cursor{0};
  std::atomic<uint64_t> fmod_pump_thread_81d60_hits{0};
  std::atomic<uint64_t> fmod_pump_waitprep_81de4_hits{0};
  std::atomic<uint64_t> fmod_pump_waitprep_ptr_nonzero{0};
  std::atomic<uint64_t> fmod_pump_waitprep_ptr_zero{0};
  std::atomic<uint64_t> fmod_pump_wait_override_hits{0};
  std::atomic<uint32_t> fmod_pump_wait_last_ms{0};
  std::atomic<uint64_t> fmod_pump_waitresult_81dfc_hits{0};
  std::atomic<uint64_t> fmod_pump_waitresult_timeout258{0};
  std::atomic<uint64_t> fmod_pump_waitresult_other{0};
  std::atomic<uint32_t> fmod_pump_waitresult_last{0};
  std::atomic<uint64_t> fmod_gate_entry{0};
  std::atomic<uint64_t> fmod_setevent{0};
  std::atomic<uint64_t> fmod_wait_call{0};
  std::atomic<uint64_t> fmod_work_tick{0};
  std::atomic<uint64_t> fmod_decode_read{0};
  std::atomic<uint64_t> fmod_gate_entry_any{0};
  std::atomic<uint64_t> fmod_setevent_any{0};
  std::atomic<uint64_t> fmod_wait_call_any{0};
  std::atomic<uint64_t> fmod_work_tick_any{0};
  std::atomic<uint64_t> fmod_decode_read_any{0};
  std::atomic<uint64_t> fmod_wait_timeout32{0};
  std::atomic<uint64_t> fmod_wait_alertable1{0};
  std::atomic<uint32_t> fmod_wait_last_handle{0};
  std::atomic<uint32_t> fmod_loop_last_tid{0};
  std::atomic<uint32_t> fmod_gate_last_tid{0};
  std::atomic<uint32_t> fmod_setevent_last_tid{0};
  std::atomic<uint32_t> fmod_wait_last_tid{0};
  std::atomic<uint32_t> fmod_work_last_tid{0};
  std::atomic<uint32_t> fmod_decode_last_tid{0};
  std::atomic<uint32_t> fmod_render_cb_last_tid{0};
  std::atomic<uint32_t> fmod_render_dispatch_last_tid{0};
  std::atomic<uint32_t> fmod_decode_thunk_last_tid{0};
  std::atomic<uint32_t> fmod_fill_last_tid{0};
  std::atomic<uint32_t> fmod_sched_eb998_last_tid{0};
  std::atomic<uint32_t> fmod_sched_eb9d0_last_tid{0};
  std::atomic<uint32_t> fmod_irq_6c380_last_tid{0};
  std::atomic<uint32_t> fmod_irq_last_obj{0};
  std::atomic<uint32_t> fmod_irq_last_cur{0};
  std::atomic<uint32_t> fmod_irq_last_target{0};
  std::atomic<uint32_t> fmod_irq_last_pending{0};
  std::atomic<uint32_t> fmod_irq_last_ticket{0};
  std::atomic<uint32_t> fmod_irq_sched_last_arg{0};
  std::atomic<uint32_t> fmod_irq_sched_last_mode{0};
  std::atomic<uint32_t> fmod_irq_sched_last_thr{0};

  std::atomic<uint32_t> a56c_handle{0};
  std::atomic<uint32_t> s9968_handle{0};
  std::atomic<uint32_t> s9d10_handle{0};
  std::atomic<uint32_t> s9ffc_handle{0};
  std::atomic<uint32_t> s86a40_handle{0};
  std::atomic<uint64_t> p898f8_samples{0};
  std::atomic<uint64_t> p89be0_samples{0};
  std::atomic<uint64_t> p89e88_samples{0};
  std::atomic<uint64_t> p86988_samples{0};
  std::atomic<bool> reg_logged_once{false};

  std::atomic<uint64_t> last_emit_sec{0};
  std::mutex log_mutex;
  FILE* log_file = nullptr;
};

SigSiteDiagState& SigDiag() {
  static SigSiteDiagState state;
  static std::once_flag init_once;
  std::call_once(init_once, [] {
    const char* env = std::getenv("REX_FM2_SIGSITE_DIAG");
    if (env && *env == '0') {
      state.enabled = false;
    }
    const char* force_env = std::getenv("REX_FM2_A4E8_FORCE_SET");
    if (force_env && *force_env == '1') {
      state.force_a4e8_setevent_every_call = true;
    }
    const char* sched_env = std::getenv("REX_FM2_SCHED_MODE2");
    if (sched_env && *sched_env == '1') {
      state.force_sched_mode2 = true;
    }
    const char* submit_env = std::getenv("REX_FM2_SUBMIT_MODE3");
    if (submit_env && *submit_env == '1') {
      state.force_submit_mode3 = true;
    }
    const char* wait_env = std::getenv("REX_FM2_PUMP_WAIT_MS");
    if (wait_env && *wait_env) {
      long v = std::strtol(wait_env, nullptr, 10);
      if (v <= 0) {
        state.force_pump_wait_override = false;
      } else if (v <= 32) {
        state.force_pump_wait_override = true;
        state.force_pump_wait_ms = static_cast<int32_t>(v);
      }
    }
    const char* wait8_env = std::getenv("REX_FM2_PUMP_WAIT_8MS");
    if (wait8_env && *wait8_env == '1') {
      state.force_pump_wait_override = true;
      state.force_pump_wait_ms = 8;
    }
    const char* read4096_env = std::getenv("REX_FM2_READ4096");
    if (read4096_env && *read4096_env == '1') {
      state.force_read_size_4096 = true;
    }
  });
  return state;
}

void LogLine(const char* fmt, ...) {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  std::lock_guard<std::mutex> lock(d.log_mutex);
  if (!d.log_file) {
    d.log_file = std::fopen("C:\\temp\\fm2-clean.log", "a");
    if (!d.log_file) {
      return;
    }
  }
  va_list args;
  va_start(args, fmt);
  std::vfprintf(d.log_file, fmt, args);
  va_end(args);
  std::fputc('\n', d.log_file);
  std::fflush(d.log_file);
}



void MaybeEmitSigPerSec() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }

  const uint64_t now_sec = NowSec();
  uint64_t last_sec = d.last_emit_sec.load(std::memory_order_relaxed);
  if (last_sec == 0) {
    d.last_emit_sec.store(now_sec, std::memory_order_relaxed);
    return;
  }
  if (now_sec == last_sec) {
    return;
  }
  if (!d.last_emit_sec.compare_exchange_strong(last_sec, now_sec, std::memory_order_relaxed)) {
    return;
  }

  const uint64_t a56c = d.a56c_count.exchange(0, std::memory_order_relaxed);
  const uint64_t s9968 = d.s9968_count.exchange(0, std::memory_order_relaxed);
  const uint64_t s9d10 = d.s9d10_count.exchange(0, std::memory_order_relaxed);
  const uint64_t s9ffc = d.s9ffc_count.exchange(0, std::memory_order_relaxed);
  const uint64_t s86a40 = d.s86a40_count.exchange(0, std::memory_order_relaxed);
  const uint64_t p898f8 = d.p898f8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t p89be0 = d.p89be0_count.exchange(0, std::memory_order_relaxed);
  const uint64_t p89e88 = d.p89e88_count.exchange(0, std::memory_order_relaxed);
  const uint64_t p86988 = d.p86988_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h87678 = d.h87678_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h637f8 = d.h637f8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h53d718 = d.h53d718_count.exchange(0, std::memory_order_relaxed);
  const uint64_t c82214cf0 = d.c82214cf0_count.exchange(0, std::memory_order_relaxed);
  const uint64_t c82599a88 = d.c82599a88_count.exchange(0, std::memory_order_relaxed);
  const uint64_t c821e38c8 = d.c821e38c8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t c8229e368 = d.c8229e368_count.exchange(0, std::memory_order_relaxed);
  const uint64_t c8235f3d8 = d.c8235f3d8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h63768 = d.h63768_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h67f60 = d.h67f60_count.exchange(0, std::memory_order_relaxed);
  const uint64_t h63538 = d.h63538_count.exchange(0, std::memory_order_relaxed);
  const uint64_t lr821d0448 = d.lr821d0448_count.exchange(0, std::memory_order_relaxed);
  const uint64_t lr8259f3a0 = d.lr8259f3a0_count.exchange(0, std::memory_order_relaxed);
  const uint64_t lr825345a8 = d.lr825345a8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t lr822097c8 = d.lr822097c8_count.exchange(0, std::memory_order_relaxed);
  const uint64_t lr_other = d.lr_other_count.exchange(0, std::memory_order_relaxed);
  const uint64_t b_missing = d.b_workitem_missing.exchange(0, std::memory_order_relaxed);
  const uint64_t b_allocfail = d.b_alloc_fail.exchange(0, std::memory_order_relaxed);
  const uint64_t b821d_r31_zero = d.b821d_r31_zero.exchange(0, std::memory_order_relaxed);
  const uint64_t b821d_r31_nonzero = d.b821d_r31_nonzero.exchange(0, std::memory_order_relaxed);
  const uint64_t b821d_div_ge_1 = d.b821d_div_ge_1.exchange(0, std::memory_order_relaxed);
  const uint64_t b821d_div_lt_1 = d.b821d_div_lt_1.exchange(0, std::memory_order_relaxed);
  const uint64_t b8259f_r31_zero = d.b8259f_r31_zero.exchange(0, std::memory_order_relaxed);
  const uint64_t b8259f_r31_nonzero = d.b8259f_r31_nonzero.exchange(0, std::memory_order_relaxed);
  const uint64_t b8259f_div_ge_8 = d.b8259f_div_ge_8.exchange(0, std::memory_order_relaxed);
  const uint64_t b8259f_div_lt_8 = d.b8259f_div_lt_8.exchange(0, std::memory_order_relaxed);
  const uint64_t b825345_r31_zero = d.b825345_r31_zero.exchange(0, std::memory_order_relaxed);
  const uint64_t b825345_r31_nonzero =
      d.b825345_r31_nonzero.exchange(0, std::memory_order_relaxed);
  const uint64_t b825345_div_ge_52 = d.b825345_div_ge_52.exchange(0, std::memory_order_relaxed);
  const uint64_t b825345_div_lt_52 = d.b825345_div_lt_52.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_flag_zero =
      d.b82209038_flag_zero.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_flag_nonzero =
      d.b82209038_flag_nonzero.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_cmp_eq_m1 =
      d.b82209038_cmp_eq_m1.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_cmp_ne_m1 =
      d.b82209038_cmp_ne_m1.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_path_97c4 =
      d.b82209038_path_97c4.exchange(0, std::memory_order_relaxed);
  const uint64_t b82209038_path_9840 =
      d.b82209038_path_9840.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_entry = d.d_a528_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_gate_true = d.d_a528_gate_true.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_gate_false = d.d_a528_gate_false.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode5 = d.d_a528_mode5.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode6 = d.d_a528_mode6.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode7 = d.d_a528_mode7.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode8 = d.d_a528_mode8.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode9 = d.d_a528_mode9.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_mode_other =
      d.d_a528_mode_other.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m5_match = d.d_a528_m5_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m5_miss = d.d_a528_m5_miss.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m6_match = d.d_a528_m6_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m6_miss = d.d_a528_m6_miss.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m7_match = d.d_a528_m7_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m7_miss = d.d_a528_m7_miss.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m8_match = d.d_a528_m8_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m8_miss = d.d_a528_m8_miss.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m9_match = d.d_a528_m9_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_m9_miss = d.d_a528_m9_miss.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_route_882e0 =
      d.d_a528_route_882e0.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_route_89e88 =
      d.d_a528_route_89e88.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_route_883e0 =
      d.d_a528_route_883e0.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_route_89990 =
      d.d_a528_route_89990.exchange(0, std::memory_order_relaxed);
  const uint64_t d_a528_route_89978 =
      d.d_a528_route_89978.exchange(0, std::memory_order_relaxed);
  const uint64_t d_slot136_total = d.d_slot136_total.exchange(0, std::memory_order_relaxed);
  const uint64_t d_slot136_to_a528 =
      d.d_slot136_to_a528.exchange(0, std::memory_order_relaxed);
  const uint64_t d_slot136_to_other =
      d.d_slot136_to_other.exchange(0, std::memory_order_relaxed);
  const uint32_t d_slot136_last_target = d.d_slot136_last_target.load(std::memory_order_relaxed);
  const uint64_t d_7310_entry = d.d_7310_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_pass = d.d_7310_pass.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_fail1 = d.d_7310_fail1.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_fail2 = d.d_7310_fail2.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_fail3 = d.d_7310_fail3.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_r148_eq1 = d.d_7310_r148_eq1.exchange(0, std::memory_order_relaxed);
  const uint64_t d_7310_r148_ne1 = d.d_7310_r148_ne1.exchange(0, std::memory_order_relaxed);
  const uint32_t d_7310_r148_last_target =
      d.d_7310_r148_last_target.load(std::memory_order_relaxed);
  const uint64_t d_344c0_entry = d.d_344c0_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_344c0_loop_end = d.d_344c0_loop_end.exchange(0, std::memory_order_relaxed);
  const uint64_t d_344c0_name_eq = d.d_344c0_name_eq.exchange(0, std::memory_order_relaxed);
  const uint64_t d_344c0_name_ne = d.d_344c0_name_ne.exchange(0, std::memory_order_relaxed);
  const uint64_t d_344c0_match = d.d_344c0_match.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_0e70_count =
      d.d_alloc_cs_0e70_count.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_7578_count =
      d.d_alloc_cs_7578_count.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_8c98_count =
      d.d_alloc_cs_8c98_count.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_0e70_lt32 =
      d.d_alloc_cs_0e70_lt32.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_0e70_32_255 =
      d.d_alloc_cs_0e70_32_255.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_0e70_256_4095 =
      d.d_alloc_cs_0e70_256_4095.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_0e70_ge4096 =
      d.d_alloc_cs_0e70_ge4096.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_7578_lt32 =
      d.d_alloc_cs_7578_lt32.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_7578_32_255 =
      d.d_alloc_cs_7578_32_255.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_7578_256_4095 =
      d.d_alloc_cs_7578_256_4095.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_7578_ge4096 =
      d.d_alloc_cs_7578_ge4096.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_8c98_lt32 =
      d.d_alloc_cs_8c98_lt32.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_8c98_32_255 =
      d.d_alloc_cs_8c98_32_255.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_8c98_256_4095 =
      d.d_alloc_cs_8c98_256_4095.exchange(0, std::memory_order_relaxed);
  const uint64_t d_alloc_cs_8c98_ge4096 =
      d.d_alloc_cs_8c98_ge4096.exchange(0, std::memory_order_relaxed);
  const uint32_t d_alloc_cs_0e70_last = d.d_alloc_cs_0e70_last_size.load(std::memory_order_relaxed);
  const uint32_t d_alloc_cs_7578_last = d.d_alloc_cs_7578_last_size.load(std::memory_order_relaxed);
  const uint32_t d_alloc_cs_8c98_last = d.d_alloc_cs_8c98_last_size.load(std::memory_order_relaxed);
  const uint64_t d_1d03e8_entry = d.d_1d03e8_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d03e8_lr_0e74 = d.d_1d03e8_lr_0e74.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d03e8_lr_757c = d.d_1d03e8_lr_757c.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d03e8_lr_8c9c = d.d_1d03e8_lr_8c9c.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d03e8_lr_other = d.d_1d03e8_lr_other.exchange(0, std::memory_order_relaxed);
  const uint32_t d_1d03e8_last_lr = d.d_1d03e8_last_lr.load(std::memory_order_relaxed);
  const uint32_t d_1d03e8_last_size = d.d_1d03e8_last_size.load(std::memory_order_relaxed);
  const uint64_t d_1d0e10_entry = d.d_1d0e10_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d0e10_lr_other = d.d_1d0e10_lr_other.exchange(0, std::memory_order_relaxed);
  const uint32_t d_1d0e10_last_lr = d.d_1d0e10_last_lr.load(std::memory_order_relaxed);
  const uint32_t d_1d0e10_last_req = d.d_1d0e10_last_req.load(std::memory_order_relaxed);
  const uint32_t d_1d0e10_last_copy = d.d_1d0e10_last_copy.load(std::memory_order_relaxed);
  const uint64_t d_1d1568_entry = d.d_1d1568_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t d_1d1568_lr_other = d.d_1d1568_lr_other.exchange(0, std::memory_order_relaxed);
  const uint32_t d_1d1568_last_lr = d.d_1d1568_last_lr.load(std::memory_order_relaxed);
  const uint32_t d_1d1568_last_req = d.d_1d1568_last_req.load(std::memory_order_relaxed);
  const uint32_t d_1d1568_last_flag = d.d_1d1568_last_flag.load(std::memory_order_relaxed);
  const uint64_t d_1d25c0_entry = d.d_1d25c0_entry.exchange(0, std::memory_order_relaxed);
  const uint32_t d_1d25c0_last_lr = d.d_1d25c0_last_lr.load(std::memory_order_relaxed);
  const uint64_t d_1d24d8_entry = d.d_1d24d8_entry.exchange(0, std::memory_order_relaxed);
  const uint32_t d_1d24d8_last_lr = d.d_1d24d8_last_lr.load(std::memory_order_relaxed);
  const uint64_t d_430c10_entry = d.d_430c10_entry.exchange(0, std::memory_order_relaxed);
  const uint32_t d_430c10_last_lr = d.d_430c10_last_lr.load(std::memory_order_relaxed);
  const uint64_t d_5cf298_entry = d.d_5cf298_entry.exchange(0, std::memory_order_relaxed);
  const uint32_t d_5cf298_last_lr = d.d_5cf298_last_lr.load(std::memory_order_relaxed);
  const uint32_t d_5cf298_last_flag = d.d_5cf298_last_flag.load(std::memory_order_relaxed);
  const uint64_t reg_snapshots = d.reg_snapshot_count.exchange(0, std::memory_order_relaxed);
  const uint32_t fmod_tid = d.fmod_worker_tid.load(std::memory_order_relaxed);
  const uint64_t fmod_bind = d.fmod_worker_bind_events.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_autobind = d.fmod_auto_bind_events.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_loop_hits = d.fmod_hook_loop_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_gate_hits = d.fmod_hook_gate_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wait_hits = d.fmod_hook_wait_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_work_hits = d.fmod_hook_work_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_decode_hits = d.fmod_hook_decode_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_render_cb = d.fmod_render_cb_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_render_dispatch =
      d.fmod_render_dispatch_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_decode_thunk = d.fmod_decode_thunk_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_fill = d.fmod_fill_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_eb998 = d.fmod_sched_eb998_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_eb9d0 = d.fmod_sched_eb9d0_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_6c380 = d.fmod_irq_6c380_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_6c380_eq =
      d.fmod_irq_6c380_eq_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_sched_6c4f0 =
      d.fmod_irq_sched_6c4f0_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_sched_over_thr =
      d.fmod_irq_sched_over_thr_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_sched_path_immediate =
      d.fmod_irq_sched_path_immediate_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_sched_path_queued =
      d.fmod_irq_sched_path_queued_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_6c688 =
      d.fmod_irq_submit_6c688_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_a2_20 =
      d.fmod_irq_submit_a2_20_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_a2_other =
      d.fmod_irq_submit_a2_other_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_lr_a =
      d.fmod_irq_submit_lr_a_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_lr_b =
      d.fmod_irq_submit_lr_b_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_irq_submit_lr_other =
      d.fmod_irq_submit_lr_other_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wrap_6c8c8 = d.fmod_wrap_6c8c8_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wrap_6c948 = d.fmod_wrap_6c948_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wrap_6cb20 = d.fmod_wrap_6cb20_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_calls = d.fmod_read_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_req_bytes =
      d.fmod_read_req_bytes.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_req_forced_4096 =
      d.fmod_read_req_forced_4096.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_copy1_calls =
      d.fmod_read_copy1_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_copy1_bytes =
      d.fmod_read_copy1_bytes.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_copy2_calls =
      d.fmod_read_copy2_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_copy2_bytes =
      d.fmod_read_copy2_bytes.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_read_ctx_ffca8300 =
      d.fmod_read_ctx_ffca8300_hits.exchange(0, std::memory_order_relaxed);
  const uint32_t fmod_read_last_req = d.fmod_read_last_req.load(std::memory_order_relaxed);
  const uint32_t fmod_read_last_ctx = d.fmod_read_last_ctx.load(std::memory_order_relaxed);
  const uint32_t fmod_read_last_cursor = d.fmod_read_last_cursor.load(std::memory_order_relaxed);
  const uint64_t fmod_pump_81d60 =
      d.fmod_pump_thread_81d60_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitprep_81de4 =
      d.fmod_pump_waitprep_81de4_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitprep_ptr_nonzero =
      d.fmod_pump_waitprep_ptr_nonzero.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitprep_ptr_zero =
      d.fmod_pump_waitprep_ptr_zero.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_wait_override =
      d.fmod_pump_wait_override_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitresult_81dfc =
      d.fmod_pump_waitresult_81dfc_hits.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitresult_timeout258 =
      d.fmod_pump_waitresult_timeout258.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_pump_waitresult_other =
      d.fmod_pump_waitresult_other.exchange(0, std::memory_order_relaxed);
  const uint32_t fmod_pump_wait_last_ms =
      d.fmod_pump_wait_last_ms.load(std::memory_order_relaxed);
  const uint32_t fmod_pump_waitresult_last =
      d.fmod_pump_waitresult_last.load(std::memory_order_relaxed);
  const uint64_t fmod_gate = d.fmod_gate_entry.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_setevent = d.fmod_setevent.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wait = d.fmod_wait_call.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_work = d.fmod_work_tick.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_decode = d.fmod_decode_read.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_gate_any = d.fmod_gate_entry_any.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_setevent_any = d.fmod_setevent_any.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wait_any = d.fmod_wait_call_any.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_work_any = d.fmod_work_tick_any.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_decode_any = d.fmod_decode_read_any.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_wait32 = d.fmod_wait_timeout32.exchange(0, std::memory_order_relaxed);
  const uint64_t fmod_alertable1 =
      d.fmod_wait_alertable1.exchange(0, std::memory_order_relaxed);
  const uint32_t fmod_wait_last_h = d.fmod_wait_last_handle.load(std::memory_order_relaxed);
  const uint32_t fmod_loop_tid = d.fmod_loop_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_gate_tid = d.fmod_gate_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_setevent_tid = d.fmod_setevent_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_wait_tid = d.fmod_wait_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_work_tid = d.fmod_work_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_decode_tid = d.fmod_decode_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_render_cb_tid = d.fmod_render_cb_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_render_dispatch_tid =
      d.fmod_render_dispatch_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_decode_thunk_tid = d.fmod_decode_thunk_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_fill_tid = d.fmod_fill_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_eb998_tid = d.fmod_sched_eb998_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_eb9d0_tid = d.fmod_sched_eb9d0_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_tid = d.fmod_irq_6c380_last_tid.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_obj = d.fmod_irq_last_obj.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_cur = d.fmod_irq_last_cur.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_tgt = d.fmod_irq_last_target.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_pending = d.fmod_irq_last_pending.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_ticket = d.fmod_irq_last_ticket.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_sched_arg = d.fmod_irq_sched_last_arg.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_sched_mode = d.fmod_irq_sched_last_mode.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_sched_thr = d.fmod_irq_sched_last_thr.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_submit_last_a2 =
      d.fmod_irq_submit_last_a2.load(std::memory_order_relaxed);
  const uint32_t fmod_irq_submit_last_lr =
      d.fmod_irq_submit_last_lr.load(std::memory_order_relaxed);
  uint64_t fmod_irq_mode_hist[16]{};
  uint64_t fmod_irq_thr_hist[16]{};
  for (int i = 0; i < 16; ++i) {
    fmod_irq_mode_hist[i] = d.fmod_irq_sched_mode_hist[i].exchange(0, std::memory_order_relaxed);
    fmod_irq_thr_hist[i] = d.fmod_irq_sched_thr_hist[i].exchange(0, std::memory_order_relaxed);
  }

  const uint32_t h_a56c = d.a56c_handle.load(std::memory_order_relaxed);
  const uint32_t h_9968 = d.s9968_handle.load(std::memory_order_relaxed);
  const uint32_t h_9d10 = d.s9d10_handle.load(std::memory_order_relaxed);
  const uint32_t h_9ffc = d.s9ffc_handle.load(std::memory_order_relaxed);
  const uint32_t h_86a40 = d.s86a40_handle.load(std::memory_order_relaxed);

  LogLine(
      "FM2_SIGSITE_PERSEC sec=%llu a56c=%llu h_a56c=%08X s9968=%llu h_9968=%08X s9d10=%llu "
      "h_9d10=%08X s9ffc=%llu h_9ffc=%08X s86a40=%llu h_86a40=%08X",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(a56c), h_a56c,
      static_cast<unsigned long long>(s9968), h_9968, static_cast<unsigned long long>(s9d10), h_9d10,
      static_cast<unsigned long long>(s9ffc), h_9ffc, static_cast<unsigned long long>(s86a40),
      h_86a40);
  LogLine(
      "FM2_PROD_PERSEC sec=%llu p898f8=%llu p89be0=%llu p89e88=%llu p86988=%llu h87678=%llu "
      "h637f8=%llu h53d718=%llu bail_missing=%llu bail_allocfail=%llu reg_oneshot=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(p898f8),
      static_cast<unsigned long long>(p89be0), static_cast<unsigned long long>(p89e88),
      static_cast<unsigned long long>(p86988), static_cast<unsigned long long>(h87678),
      static_cast<unsigned long long>(h637f8), static_cast<unsigned long long>(h53d718),
      static_cast<unsigned long long>(b_missing), static_cast<unsigned long long>(b_allocfail),
      static_cast<unsigned long long>(reg_snapshots));
  LogLine(
      "FM2_637F8_CALLER_PERSEC sec=%llu c82214cf0=%llu c82599a88=%llu c821e38c8=%llu "
      "c8229e368=%llu c8235f3d8=%llu h63768=%llu h67f60=%llu h63538=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(c82214cf0),
      static_cast<unsigned long long>(c82599a88), static_cast<unsigned long long>(c821e38c8),
      static_cast<unsigned long long>(c8229e368), static_cast<unsigned long long>(c8235f3d8),
      static_cast<unsigned long long>(h63768), static_cast<unsigned long long>(h67f60),
      static_cast<unsigned long long>(h63538));
  LogLine(
      "FM2_63768_LR_PERSEC sec=%llu lr821d0448=%llu lr8259f3a0=%llu lr825345a8=%llu "
      "lr822097c8=%llu lr_other=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(lr821d0448),
      static_cast<unsigned long long>(lr8259f3a0), static_cast<unsigned long long>(lr825345a8),
      static_cast<unsigned long long>(lr822097c8), static_cast<unsigned long long>(lr_other));
  LogLine(
      "FM2_ALLOC_BRANCH_PERSEC sec=%llu 821d_zero=%llu 821d_nz=%llu 821d_ge1=%llu 821d_lt1=%llu "
      "8259f_zero=%llu 8259f_nz=%llu 8259f_ge8=%llu 8259f_lt8=%llu "
      "825345_zero=%llu 825345_nz=%llu 825345_ge52=%llu 825345_lt52=%llu "
      "9038_flag0=%llu 9038_flagnz=%llu 9038_cmpm1=%llu 9038_cmpne=%llu "
      "9038_p97c4=%llu 9038_p9840=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(b821d_r31_zero),
      static_cast<unsigned long long>(b821d_r31_nonzero),
      static_cast<unsigned long long>(b821d_div_ge_1),
      static_cast<unsigned long long>(b821d_div_lt_1),
      static_cast<unsigned long long>(b8259f_r31_zero),
      static_cast<unsigned long long>(b8259f_r31_nonzero),
      static_cast<unsigned long long>(b8259f_div_ge_8),
      static_cast<unsigned long long>(b8259f_div_lt_8),
      static_cast<unsigned long long>(b825345_r31_zero),
      static_cast<unsigned long long>(b825345_r31_nonzero),
      static_cast<unsigned long long>(b825345_div_ge_52),
      static_cast<unsigned long long>(b825345_div_lt_52),
      static_cast<unsigned long long>(b82209038_flag_zero),
      static_cast<unsigned long long>(b82209038_flag_nonzero),
      static_cast<unsigned long long>(b82209038_cmp_eq_m1),
      static_cast<unsigned long long>(b82209038_cmp_ne_m1),
      static_cast<unsigned long long>(b82209038_path_97c4),
      static_cast<unsigned long long>(b82209038_path_9840));
  LogLine(
      "FM2_A528_PERSEC sec=%llu entry=%llu gate_t=%llu gate_f=%llu "
      "m5=%llu m6=%llu m7=%llu m8=%llu m9=%llu moth=%llu "
      "m5ok=%llu m5miss=%llu m6ok=%llu m6miss=%llu m7ok=%llu m7miss=%llu "
      "m8ok=%llu m8miss=%llu m9ok=%llu m9miss=%llu "
      "r882e0=%llu r89e88=%llu r883e0=%llu r89990=%llu r89978=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_a528_entry),
      static_cast<unsigned long long>(d_a528_gate_true),
      static_cast<unsigned long long>(d_a528_gate_false),
      static_cast<unsigned long long>(d_a528_mode5), static_cast<unsigned long long>(d_a528_mode6),
      static_cast<unsigned long long>(d_a528_mode7), static_cast<unsigned long long>(d_a528_mode8),
      static_cast<unsigned long long>(d_a528_mode9),
      static_cast<unsigned long long>(d_a528_mode_other),
      static_cast<unsigned long long>(d_a528_m5_match),
      static_cast<unsigned long long>(d_a528_m5_miss),
      static_cast<unsigned long long>(d_a528_m6_match),
      static_cast<unsigned long long>(d_a528_m6_miss),
      static_cast<unsigned long long>(d_a528_m7_match),
      static_cast<unsigned long long>(d_a528_m7_miss),
      static_cast<unsigned long long>(d_a528_m8_match),
      static_cast<unsigned long long>(d_a528_m8_miss),
      static_cast<unsigned long long>(d_a528_m9_match),
      static_cast<unsigned long long>(d_a528_m9_miss),
      static_cast<unsigned long long>(d_a528_route_882e0),
      static_cast<unsigned long long>(d_a528_route_89e88),
      static_cast<unsigned long long>(d_a528_route_883e0),
      static_cast<unsigned long long>(d_a528_route_89990),
      static_cast<unsigned long long>(d_a528_route_89978));
  LogLine("FM2_SLOT136_PERSEC sec=%llu total=%llu to_a528=%llu to_other=%llu last_target=%08X",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_slot136_total),
          static_cast<unsigned long long>(d_slot136_to_a528),
          static_cast<unsigned long long>(d_slot136_to_other), d_slot136_last_target);
  LogLine("FM2_7310_PERSEC sec=%llu entry=%llu pass=%llu fail1=%llu fail2=%llu fail3=%llu",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_7310_entry),
          static_cast<unsigned long long>(d_7310_pass), static_cast<unsigned long long>(d_7310_fail1),
          static_cast<unsigned long long>(d_7310_fail2),
          static_cast<unsigned long long>(d_7310_fail3));
  LogLine("FM2_7310_R148_PERSEC sec=%llu eq1=%llu ne1=%llu last_target=%08X",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_7310_r148_eq1),
          static_cast<unsigned long long>(d_7310_r148_ne1), d_7310_r148_last_target);
  LogLine("FM2_344C0_PERSEC sec=%llu entry=%llu loop_end=%llu name_eq=%llu name_ne=%llu match=%llu",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_344c0_entry),
          static_cast<unsigned long long>(d_344c0_loop_end),
          static_cast<unsigned long long>(d_344c0_name_eq),
          static_cast<unsigned long long>(d_344c0_name_ne),
          static_cast<unsigned long long>(d_344c0_match));
  LogLine(
      "FM2_ALLOC_CALLSITE_PERSEC sec=%llu cs0e70=%llu lt32=%llu b32_255=%llu b256_4095=%llu ge4096=%llu last=%u "
      "cs7578=%llu lt32=%llu b32_255=%llu b256_4095=%llu ge4096=%llu last=%u "
      "cs8c98=%llu lt32=%llu b32_255=%llu b256_4095=%llu ge4096=%llu last=%u",
      static_cast<unsigned long long>(now_sec),
      static_cast<unsigned long long>(d_alloc_cs_0e70_count),
      static_cast<unsigned long long>(d_alloc_cs_0e70_lt32),
      static_cast<unsigned long long>(d_alloc_cs_0e70_32_255),
      static_cast<unsigned long long>(d_alloc_cs_0e70_256_4095),
      static_cast<unsigned long long>(d_alloc_cs_0e70_ge4096), d_alloc_cs_0e70_last,
      static_cast<unsigned long long>(d_alloc_cs_7578_count),
      static_cast<unsigned long long>(d_alloc_cs_7578_lt32),
      static_cast<unsigned long long>(d_alloc_cs_7578_32_255),
      static_cast<unsigned long long>(d_alloc_cs_7578_256_4095),
      static_cast<unsigned long long>(d_alloc_cs_7578_ge4096), d_alloc_cs_7578_last,
      static_cast<unsigned long long>(d_alloc_cs_8c98_count),
      static_cast<unsigned long long>(d_alloc_cs_8c98_lt32),
      static_cast<unsigned long long>(d_alloc_cs_8c98_32_255),
      static_cast<unsigned long long>(d_alloc_cs_8c98_256_4095),
      static_cast<unsigned long long>(d_alloc_cs_8c98_ge4096), d_alloc_cs_8c98_last);
  LogLine(
      "FM2_1D03E8_CALLERS sec=%llu entry=%llu lr0e74=%llu lr757c=%llu lr8c9c=%llu other=%llu last_lr=%08X last_size=%u",
      static_cast<unsigned long long>(now_sec),
      static_cast<unsigned long long>(d_1d03e8_entry),
      static_cast<unsigned long long>(d_1d03e8_lr_0e74),
      static_cast<unsigned long long>(d_1d03e8_lr_757c),
      static_cast<unsigned long long>(d_1d03e8_lr_8c9c),
      static_cast<unsigned long long>(d_1d03e8_lr_other), d_1d03e8_last_lr, d_1d03e8_last_size);
  LogLine("FM2_1D0E10_CALLERS sec=%llu entry=%llu other=%llu last_lr=%08X last_req=%u last_copy=%u",
          static_cast<unsigned long long>(now_sec),
          static_cast<unsigned long long>(d_1d0e10_entry),
          static_cast<unsigned long long>(d_1d0e10_lr_other), d_1d0e10_last_lr, d_1d0e10_last_req,
          d_1d0e10_last_copy);
  LogLine("FM2_1D1568_CALLERS sec=%llu entry=%llu other=%llu last_lr=%08X last_req=%u last_flag=%u",
          static_cast<unsigned long long>(now_sec),
          static_cast<unsigned long long>(d_1d1568_entry),
          static_cast<unsigned long long>(d_1d1568_lr_other), d_1d1568_last_lr, d_1d1568_last_req,
          d_1d1568_last_flag);
  LogLine("FM2_STRCORE_PERSEC sec=%llu f25c0=%llu lr25c0=%08X f24d8=%llu lr24d8=%08X f30c10=%llu lr30c10=%08X",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_1d25c0_entry),
          d_1d25c0_last_lr, static_cast<unsigned long long>(d_1d24d8_entry), d_1d24d8_last_lr,
          static_cast<unsigned long long>(d_430c10_entry), d_430c10_last_lr);
  LogLine("FM2_5CF298_PERSEC sec=%llu entry=%llu last_lr=%08X last_flag=%u",
          static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(d_5cf298_entry),
          d_5cf298_last_lr, d_5cf298_last_flag);
  LogLine(
      "FM2_FMOD_THREAD_PERSEC sec=%llu tid=%u bind=%llu autobind=%llu hooks(loop=%llu gate=%llu "
      "wait=%llu work=%llu decode=%llu) stage_tids(loop=%u gate=%u setevent=%u wait=%u work=%u decode=%u) "
      "gate=%llu setevent=%llu wait=%llu work=%llu decode=%llu "
      "all(gate=%llu setevent=%llu wait=%llu work=%llu decode=%llu) "
      "wait32=%llu alert1=%llu wait_h=%08X",
      static_cast<unsigned long long>(now_sec), fmod_tid, static_cast<unsigned long long>(fmod_bind),
      static_cast<unsigned long long>(fmod_autobind), static_cast<unsigned long long>(fmod_loop_hits),
      static_cast<unsigned long long>(fmod_gate_hits), static_cast<unsigned long long>(fmod_wait_hits),
      static_cast<unsigned long long>(fmod_work_hits),
      static_cast<unsigned long long>(fmod_decode_hits),
      fmod_loop_tid, fmod_gate_tid, fmod_setevent_tid, fmod_wait_tid, fmod_work_tid, fmod_decode_tid,
      static_cast<unsigned long long>(fmod_gate), static_cast<unsigned long long>(fmod_setevent),
      static_cast<unsigned long long>(fmod_wait), static_cast<unsigned long long>(fmod_work),
      static_cast<unsigned long long>(fmod_decode), static_cast<unsigned long long>(fmod_gate_any),
      static_cast<unsigned long long>(fmod_setevent_any), static_cast<unsigned long long>(fmod_wait_any),
      static_cast<unsigned long long>(fmod_work_any), static_cast<unsigned long long>(fmod_decode_any),
      static_cast<unsigned long long>(fmod_wait32),
      static_cast<unsigned long long>(fmod_alertable1), fmod_wait_last_h);
  LogLine(
      "FM2_FMOD_UP_PERSEC sec=%llu cb=%llu disp=%llu dthunk=%llu fill=%llu b998=%llu b9d0=%llu "
      "t_cb=%u t_disp=%u t_dthunk=%u t_fill=%u t_b998=%u t_b9d0=%u",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_render_cb),
      static_cast<unsigned long long>(fmod_render_dispatch),
      static_cast<unsigned long long>(fmod_decode_thunk), static_cast<unsigned long long>(fmod_fill),
      static_cast<unsigned long long>(fmod_eb998), static_cast<unsigned long long>(fmod_eb9d0),
      fmod_render_cb_tid, fmod_render_dispatch_tid, fmod_decode_thunk_tid, fmod_fill_tid,
      fmod_eb998_tid, fmod_eb9d0_tid);
  LogLine(
      "FM2_FMOD_IRQ_PERSEC sec=%llu enter=%llu eq=%llu t_irq=%u obj=%08X cur=%u tgt=%u pend=%u tic=%u",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_irq_6c380),
      static_cast<unsigned long long>(fmod_irq_6c380_eq), fmod_irq_tid, fmod_irq_obj, fmod_irq_cur,
      fmod_irq_tgt, fmod_irq_pending, fmod_irq_ticket);
  LogLine(
      "FM2_FMOD_SCHED_PERSEC sec=%llu hit=%llu last_arg=%08X mode=%u thr=%u over_thr=%llu "
      "imm=%llu queued=%llu "
      "m0=%llu m1=%llu m2=%llu m3=%llu m4=%llu m5=%llu m6=%llu m7=%llu "
      "m8=%llu m9=%llu ma=%llu mb=%llu mc=%llu md=%llu me=%llu mf=%llu "
      "t0=%llu t1=%llu t2=%llu t3=%llu t4=%llu t5=%llu t6=%llu t7=%llu "
      "t8=%llu t9=%llu ta=%llu tb=%llu tc=%llu td=%llu te=%llu tf=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_irq_sched_6c4f0),
      fmod_irq_sched_arg, fmod_irq_sched_mode, fmod_irq_sched_thr,
      static_cast<unsigned long long>(fmod_irq_sched_over_thr),
      static_cast<unsigned long long>(fmod_irq_sched_path_immediate),
      static_cast<unsigned long long>(fmod_irq_sched_path_queued),
      static_cast<unsigned long long>(fmod_irq_mode_hist[0]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[1]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[2]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[3]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[4]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[5]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[6]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[7]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[8]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[9]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[10]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[11]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[12]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[13]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[14]),
      static_cast<unsigned long long>(fmod_irq_mode_hist[15]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[0]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[1]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[2]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[3]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[4]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[5]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[6]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[7]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[8]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[9]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[10]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[11]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[12]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[13]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[14]),
      static_cast<unsigned long long>(fmod_irq_thr_hist[15]));
  LogLine(
      "FM2_FMOD_SUBMIT_PERSEC sec=%llu hit=%llu a2_20=%llu a2_other=%llu "
      "lr_a=%llu lr_b=%llu lr_other=%llu last_a2=%u last_lr=%08X",
      static_cast<unsigned long long>(now_sec),
      static_cast<unsigned long long>(fmod_irq_submit_6c688),
      static_cast<unsigned long long>(fmod_irq_submit_a2_20),
      static_cast<unsigned long long>(fmod_irq_submit_a2_other),
      static_cast<unsigned long long>(fmod_irq_submit_lr_a),
      static_cast<unsigned long long>(fmod_irq_submit_lr_b),
      static_cast<unsigned long long>(fmod_irq_submit_lr_other),
      fmod_irq_submit_last_a2, fmod_irq_submit_last_lr);
  LogLine(
      "FM2_FMOD_WRAP_PERSEC sec=%llu c8c8=%llu c948=%llu cb20=%llu",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_wrap_6c8c8),
      static_cast<unsigned long long>(fmod_wrap_6c948),
      static_cast<unsigned long long>(fmod_wrap_6cb20));
  LogLine(
      "FM2_FMOD_READ_PERSEC sec=%llu calls=%llu req_b=%llu f4096=%llu c1=%llu/%llu c2=%llu/%llu "
      "ctx_ffca=%llu last(req=%u ctx=%08X cursor=%u cfg4096=%u)",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_read_calls),
      static_cast<unsigned long long>(fmod_read_req_bytes),
      static_cast<unsigned long long>(fmod_read_req_forced_4096),
      static_cast<unsigned long long>(fmod_read_copy1_calls),
      static_cast<unsigned long long>(fmod_read_copy1_bytes),
      static_cast<unsigned long long>(fmod_read_copy2_calls),
      static_cast<unsigned long long>(fmod_read_copy2_bytes),
      static_cast<unsigned long long>(fmod_read_ctx_ffca8300), fmod_read_last_req, fmod_read_last_ctx,
      fmod_read_last_cursor, d.force_read_size_4096 ? 1u : 0u);
  LogLine(
      "FM2_FMOD_PUMP_PERSEC sec=%llu t81d60=%llu waitprep=%llu ptr_nz=%llu ptr_z=%llu ovr=%llu "
      "waitres=%llu to258=%llu other=%llu last_res=%u cfg(wait=%dms,on=%u) last_wait=%ums",
      static_cast<unsigned long long>(now_sec), static_cast<unsigned long long>(fmod_pump_81d60),
      static_cast<unsigned long long>(fmod_pump_waitprep_81de4),
      static_cast<unsigned long long>(fmod_pump_waitprep_ptr_nonzero),
      static_cast<unsigned long long>(fmod_pump_waitprep_ptr_zero),
      static_cast<unsigned long long>(fmod_pump_wait_override),
      static_cast<unsigned long long>(fmod_pump_waitresult_81dfc),
      static_cast<unsigned long long>(fmod_pump_waitresult_timeout258),
      static_cast<unsigned long long>(fmod_pump_waitresult_other), fmod_pump_waitresult_last,
      d.force_pump_wait_ms, d.force_pump_wait_override ? 1u : 0u, fmod_pump_wait_last_ms);
}

void SigSiteHit(std::atomic<uint64_t>& counter, std::atomic<uint32_t>& handle_slot,
                uint32_t handle) {
  counter.fetch_add(1, std::memory_order_relaxed);
  handle_slot.store(handle, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void HitSimple(std::atomic<uint64_t>& counter) {
  counter.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

bool IsFmodWorkerThread() {
  auto& d = SigDiag();
  const uint32_t tid = d.fmod_worker_tid.load(std::memory_order_relaxed);
  return tid != 0 && tid == static_cast<uint32_t>(GetCurrentThreadId());
}

void BindFmodWorkerThread(const char* site) {
  auto& d = SigDiag();
  const uint32_t tid = static_cast<uint32_t>(GetCurrentThreadId());
  const uint32_t prev = d.fmod_worker_tid.exchange(tid, std::memory_order_relaxed);
  d.fmod_worker_bind_events.fetch_add(1, std::memory_order_relaxed);
  if (prev != tid) {
    LogLine("FM2_FMOD_THREAD_BIND site=%s prev_tid=%u tid=%u", site, prev, tid);
  }
  MaybeEmitSigPerSec();
}

void AutoBindFmodThreadIfUnset(const char* site) {
  auto& d = SigDiag();
  const uint32_t tid = static_cast<uint32_t>(GetCurrentThreadId());
  uint32_t expected = 0;
  if (d.fmod_worker_tid.compare_exchange_strong(expected, tid, std::memory_order_relaxed)) {
    d.fmod_auto_bind_events.fetch_add(1, std::memory_order_relaxed);
    LogLine("FM2_FMOD_THREAD_AUTOBIND site=%s tid=%u", site, tid);
  }
}

void HitAllocBuckets(uint32_t size, std::atomic<uint64_t>& lt32, std::atomic<uint64_t>& b32_255,
                     std::atomic<uint64_t>& b256_4095, std::atomic<uint64_t>& ge4096) {
  if (size < 32u) {
    HitSimple(lt32);
  } else if (size < 256u) {
    HitSimple(b32_255);
  } else if (size < 4096u) {
    HitSimple(b256_4095);
  } else {
    HitSimple(ge4096);
  }
}

uint32_t TryLoadU32(uint8_t* base, uint32_t addr) {
  if (!base || !GuestReadableRange(base, addr, 4)) {
    return 0;
  }
  return REX_LOAD_U32(addr);
}

uint64_t TryLoadU64(uint8_t* base, uint32_t addr) {
  if (!base || !GuestReadableRange(base, addr, 8)) {
    return 0;
  }
  return REX_LOAD_U64(addr);
}

uint16_t TryLoadU16(uint8_t* base, uint32_t addr) {
  if (!base || !GuestReadableRange(base, addr, 2)) {
    return 0;
  }
  return REX_LOAD_U16(addr);
}

uint8_t TryLoadU8(uint8_t* base, uint32_t addr) {
  if (!base || !GuestReadableRange(base, addr, 1)) {
    return 0;
  }
  return REX_LOAD_U8(addr);
}

uint32_t AddGuestOffsetOrZero(uint32_t address, uint32_t offset) {
  const uint64_t target = uint64_t(address) + offset;
  if (address == 0 || target > std::numeric_limits<uint32_t>::max()) {
    return 0;
  }
  return static_cast<uint32_t>(target);
}

uint32_t TryLoadU32AtOffset(uint8_t* base, uint32_t address, uint32_t offset) {
  const uint32_t target = AddGuestOffsetOrZero(address, offset);
  if (target == 0) {
    return 0;
  }
  return TryLoadU32(base, target);
}

uint64_t TryLoadU64AtOffset(uint8_t* base, uint32_t address, uint32_t offset) {
  const uint32_t target = AddGuestOffsetOrZero(address, offset);
  if (target == 0) {
    return 0;
  }
  return TryLoadU64(base, target);
}



std::string SnapshotGuestBytes(uint8_t* base, uint32_t guest_address, uint32_t byte_count) {
  std::string out;
  if (!base || byte_count == 0 || !GuestReadableRange(base, guest_address, byte_count)) {
    return out;
  }

  static constexpr char kHex[] = "0123456789ABCDEF";
  out.reserve(byte_count * 3u);
  for (uint32_t i = 0; i < byte_count; ++i) {
    if (i != 0) {
      out.push_back(' ');
    }
    const uint8_t byte = REX_LOAD_U8(guest_address + i);
    out.push_back(kHex[byte >> 4]);
    out.push_back(kHex[byte & 0x0Fu]);
  }
  return out;
}

std::string FormatByteDump(const uint8_t* bytes, uint32_t byte_count) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  if (!bytes || byte_count == 0) {
    return out;
  }

  out.reserve(byte_count * 3u);
  for (uint32_t i = 0; i < byte_count; ++i) {
    if (i != 0) {
      out.push_back(' ');
    }
    const uint8_t byte = bytes[i];
    out.push_back(kHex[byte >> 4]);
    out.push_back(kHex[byte & 0x0Fu]);
  }
  return out;
}






void MaybeLogProducerSample(const char* site, std::atomic<uint64_t>& samples, uint32_t obj) {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  const uint64_t sample_ix = samples.fetch_add(1, std::memory_order_relaxed);
  if (sample_ix >= 3) {
    return;
  }
  uint8_t* base = GuestBase();
  const uint32_t h4c = TryLoadU32(base, obj + 0x4C);
  const uint32_t q88 = TryLoadU32(base, obj + 0x88);
  const uint32_t qa8 = TryLoadU32(base, obj + 0xA8);
  const uint8_t f44 = TryLoadU8(base, obj + 0x44);
  const uint8_t f50 = TryLoadU8(base, obj + 0x50);
  LogLine("FM2_PROD_SAMPLE site=%s n=%llu obj=%08X h4c=%08X f44=%u f50=%u q88=%08X qa8=%08X",
          site, static_cast<unsigned long long>(sample_ix + 1), obj, h4c, static_cast<unsigned>(f44),
          static_cast<unsigned>(f50), q88, qa8);
}

void MaybeLogRegistrationSnapshot(uint32_t r31_obj, uint32_t r26_src) {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  bool expected = false;
  if (!d.reg_logged_once.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
    return;
  }
  uint8_t* base = GuestBase();
  const uint32_t dst_9bc = TryLoadU32(base, r31_obj + 0x9BC);
  const uint32_t dst_9c0 = TryLoadU32(base, r31_obj + 0x9C0);
  const uint32_t src_b68 = TryLoadU32(base, r26_src + 0xB68);
  const uint32_t src_b6c = TryLoadU32(base, r26_src + 0xB6C);
  d.reg_snapshot_count.fetch_add(1, std::memory_order_relaxed);
  LogLine(
      "FM2_REG_ONESHOT obj=%08X src=%08X dst9bc=%08X dst9c0=%08X srcb68=%08X srcb6c=%08X cb=%08X",
      r31_obj, r26_src, dst_9bc, dst_9c0, src_b68, src_b6c, 0x8220A4E8u);
}

void MaybeLog344C0Sample(uint32_t a1_obj, uint32_t a2_item) {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  const uint64_t sample_ix = d.d_344c0_sample_count.fetch_add(1, std::memory_order_relaxed);
  if (sample_ix >= 8) {
    return;
  }

  uint8_t* base = GuestBase();
  const uint32_t a1_begin = TryLoadU32(base, a1_obj + 0x10);
  const uint32_t a1_end = TryLoadU32(base, a1_obj + 0x14);
  const uint32_t a2_key = TryLoadU32(base, a2_item + 0x4);
  const uint32_t a2_name_ptr = TryLoadU32(base, a2_item + 0xC);
  const uint32_t a2_name_len = TryLoadU32(base, a2_item + 0x20);

  LogLine("FM2_344C0_SAMPLE n=%llu a1=%08X a2=%08X begin=%08X end=%08X key=%08X name_ptr=%08X name_len=%u",
          static_cast<unsigned long long>(sample_ix + 1), a1_obj, a2_item, a1_begin, a1_end,
          a2_key, a2_name_ptr, static_cast<unsigned>(a2_name_len));
}

}  // namespace

bool FM2SkipBadChildSlot(PPCRegister& r11, PPCRegister& r31) {
  uint8_t* base = GuestBase();
  if (!base || HasCallableVtableSlot(base, r11.u32, 12)) {
    return false;
  }
  REX_STORE_U32(r31.u32, 0);
  return true;
}

bool FM2ReturnZeroOnBadNestedVcall(PPCRegister& r3) {
  uint8_t* base = GuestBase();
  return base && !HasCallableVtableSlot(base, r3.u32, 44);
}

bool FM2ReturnOnBadListHead(PPCRegister& r10) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r10.u32, 4);
}

bool FM2ReturnOnBadD5A8ListHead(PPCRegister& r11) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r11.u32, 4);
}

bool FM2ReturnOnBadD4F8ListHead(PPCRegister& r10) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r10.u32, 4);
}

bool FM2ReturnOnBad75A40Object(PPCRegister& r30) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r30.u32, 172);
}

bool FM2ReturnZeroOnBad76A58Object(PPCRegister& r30, PPCRegister& r29) {
  uint8_t* base = GuestBase();
  if (!base || GuestReadableRange(base, r30.u32, 156)) {
    return false;
  }

  r29.u64 = 0;
  return true;
}

bool FM2ReturnOnBad75ED0Mask(PPCRegister& r25) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r25.u32, 124);
}

bool FM2ReturnOnBad40160PrimaryResult(PPCRegister& r25) {
  uint8_t* base = GuestBase();
  return base && (!HasCallableVtableSlot(base, r25.u32, 20) ||
                  !HasCallableVtableSlot(base, r25.u32, 8));
}

bool FM2SpinWaitYield(PPCRegister& f0, PPCRegister& f31) {
  (void)f0;
  (void)f31;
  return false;
}

bool FM2SkipStartupIntroWait(PPCRegister& r3, PPCRegister& r11, PPCRegister& r30) {
  (void)r3;
  return r11.u32 == r30.u32;
}

bool FM2FastForwardSplashTiming(PPCRegister& f1, PPCRegister& f2, PPCRegister& r7,
                                PPCRegister& r31) {
  (void)r7;
  (void)r31;

  constexpr double kFastForwardDurationSec = 0.05;
  const double in_f1 = f1.f64;
  const double in_f2 = f2.f64;

  if (std::isfinite(in_f1) && std::isfinite(in_f2) && in_f2 >= 0.0 &&
      in_f2 > (in_f1 + kFastForwardDurationSec)) {
    f2.f64 = in_f1 + kFastForwardDurationSec;
  }

  return false;
}













// Intentionally not manifest-hooked: BuildDirectIndexedDrawBuffers has a one-shot
// guard at direct_render_ctx+0x48. Use FM2PlumeTraceInstanceHybridDrawEntry.

namespace {

fm2::native_renderer::GuestArgs PlumeGuestArgs(
    PPCRegister& r3, PPCRegister& r4, PPCRegister& r5, PPCRegister& r6,
    PPCRegister& r7, PPCRegister& r8, PPCRegister& r9, PPCRegister& r10) {
  return {.r3 = r3.u32,
          .r4 = r4.u32,
          .r5 = r5.u32,
          .r6 = r6.u32,
          .r7 = r7.u32,
          .r8 = r8.u32,
          .r9 = r9.u32,
          .r10 = r10.u32};
}

uint32_t PlumeDirectRenderContextFromDrawIface(uint32_t draw_iface) {
  uint8_t* base = GuestBase();
  return TryLoadU32(base, draw_iface + 0x14u);
}

uint32_t NextPlumeMidasmTraceIndex(std::atomic<uint32_t>& counter) {
  if (!REXCVAR_GET(fm2_plume_midasm_trace)) {
    return 0;
  }
  const uint32_t index = counter.fetch_add(1, std::memory_order_relaxed) + 1;
  const uint32_t limit = REXCVAR_GET(fm2_plume_midasm_trace_limit);
  if (limit != 0 && index > limit) {
    return 0;
  }
  return index;
}

}  // namespace

void FM2PlumeTracePixelShaderState(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativePixelShaderStateArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceVertexShaderState(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeVertexShaderStateArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceTextureFetchLow(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeTextureFetchLowArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceTextureFetchMid(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeTextureFetchMidArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceClearColor(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeClearColorArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceClearFlags(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeClearFlagsArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceVertexStreamBinding(PPCRegister& r3, PPCRegister& r4,
                                      PPCRegister& r5, PPCRegister& r6,
                                      PPCRegister& r7, PPCRegister& r8) {
  fm2::native_renderer::RecordNativeVertexStreamBindingArgs(
      r3.u32, r4.u32, r5.u32, r6.u32, r7.u32, r8.u32);
}

void FM2PlumeTraceIndexBufferBinding(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeIndexBufferBindingArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceViewportMode(PPCRegister& r3, PPCRegister& r4) {
  fm2::native_renderer::RecordNativeViewportModeArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceBoundSurface(PPCRegister& r3, PPCRegister& r4,
                               PPCRegister& r5) {
  fm2::native_renderer::RecordNativeBoundSurfaceArgs(r3.u32, r4.u32, r5.u32);
}

void FM2PlumeTraceD3DDirtyStateEntry(PPCRegister& r3, PPCRegister& r4,
                                     PPCRegister& r5) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=D3DDirtyState n=%u r3=%08X r4=%08X r5=%08X",
            n, r3.u32, r4.u32, r5.u32);
  }
  (void)r3;
  (void)r4;
  (void)r5;
}

void FM2PlumeTracePresent(PPCRegister& r3) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=Present n=%u r3=%08X", n, r3.u32);
  }
  fm2::native_renderer::RecordPresentEntry(r3.u32);
  fm2::native_renderer::FlushNativeDirectDrawOnPresent();
  fm2::native_renderer::FlushDebugReplayOnPresent();
}

void FM2PlumeTracePassDrawWork(PPCRegister& r3, PPCRegister& r4,
                               PPCRegister& r5, PPCRegister& r6,
                               PPCRegister& r7, PPCRegister& r8,
                               PPCRegister& r9, PPCRegister& r10) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=PassDrawWork n=%u r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X",
            n, r3.u32, r4.u32, r5.u32, r6.u32, r7.u32, r8.u32, r9.u32,
            r10.u32);
  }
  fm2::native_renderer::RecordNativePassDrawBoundaryArgs(
      r3.u32, r4.u32, r5.u32, r6.u32, r7.u32, r8.u32, r9.u32, r10.u32);
}

void FM2PlumeTraceBuildObjectPassEntry(PPCRegister& r3, PPCRegister& r4,
                                       PPCRegister& r5, PPCRegister& r6,
                                       PPCRegister& r7, PPCRegister& r8,
                                       PPCRegister& r9, PPCRegister& r10) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=BuildObjectPass n=%u r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X",
            n, r3.u32, r4.u32, r5.u32, r6.u32, r7.u32, r8.u32, r9.u32,
            r10.u32);
  }
  fm2::native_renderer::RecordBuildObjectPassEntry(
      PlumeGuestArgs(r3, r4, r5, r6, r7, r8, r9, r10));
}

void FM2PlumeTraceInstanceHybridDrawEntry(PPCRegister& r3, PPCRegister& r4,
                                          PPCRegister& r5, PPCRegister& r6,
                                          PPCRegister& r7, PPCRegister& r8,
                                          PPCRegister& r9, PPCRegister& r10) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=InstanceHybridDraw n=%u r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X",
            n, r3.u32, r4.u32, r5.u32, r6.u32, r7.u32, r8.u32, r9.u32,
            r10.u32);
  }
  fm2::native_renderer::RecordInstanceHybridDrawEntry(
      PlumeGuestArgs(r3, r4, r5, r6, r7, r8, r9, r10));
}

void FM2PlumeTraceDirectIfaceIndexedDraw(PPCRegister& r3, PPCRegister& r4,
                                         PPCRegister& r5, PPCRegister& r6,
                                         PPCRegister& r7) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=DirectIfaceIndexedDraw n=%u r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X",
            n, r3.u32, r4.u32, r5.u32, r6.u32, r7.u32);
  }
  fm2::native_renderer::RecordDirectIndexedDrawEntry(
      {.r3 = r3.u32, .r4 = r4.u32, .r5 = r5.u32, .r6 = r6.u32, .r7 = r7.u32});
  const uint32_t direct_render_context =
      PlumeDirectRenderContextFromDrawIface(r3.u32);
  fm2::native_renderer::RecordNativeDirectDrawEntry(
      {.direct_render_context = direct_render_context, .draw_iface = r3.u32});
}

void FM2PlumeTraceExecuteBoundDrawPass(PPCRegister& r3, PPCRegister& r4,
                                       PPCRegister& r5) {
  static std::atomic<uint32_t> s_trace_count{0};
  if (const uint32_t n = NextPlumeMidasmTraceIndex(s_trace_count)) {
    LogLine("FM2_PLUME_MIDASM_TRACE hook=ExecuteBoundDrawPass n=%u r3=%08X r4=%08X r5=%08X",
            n, r3.u32, r4.u32, r5.u32);
  }
  fm2::native_renderer::RecordDirectIndexedDrawEntry(
      {.r3 = r3.u32, .r4 = r4.u32, .r5 = r5.u32});
}





void FM2SigSiteA56C(PPCRegister& r3) {
  auto& d = SigDiag();
  SigSiteHit(d.a56c_count, d.a56c_handle, r3.u32);
  d.fmod_setevent_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                 std::memory_order_relaxed);
  d.fmod_setevent_any.fetch_add(1, std::memory_order_relaxed);
  AutoBindFmodThreadIfUnset("8220A56C");
  if (IsFmodWorkerThread()) {
    d.fmod_setevent.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2SigSite9968(PPCRegister& r3) {
  auto& d = SigDiag();
  SigSiteHit(d.s9968_count, d.s9968_handle, r3.u32);
}

void FM2SigSite9D10(PPCRegister& r3) {
  auto& d = SigDiag();
  SigSiteHit(d.s9d10_count, d.s9d10_handle, r3.u32);
}

void FM2SigSite9FFC(PPCRegister& r3) {
  auto& d = SigDiag();
  SigSiteHit(d.s9ffc_count, d.s9ffc_handle, r3.u32);
}

void FM2SigSite86A40(PPCRegister& r3) {
  auto& d = SigDiag();
  SigSiteHit(d.s86a40_count, d.s86a40_handle, r3.u32);
}

void FM2ProdEntry898F8(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.p898f8_count);
  MaybeLogProducerSample("898f8", d.p898f8_samples, r3.u32);
}

void FM2ProdEntry89BE0(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.p89be0_count);
  MaybeLogProducerSample("89be0", d.p89be0_samples, r3.u32);
}

void FM2ProdEntry89E88(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.p89e88_count);
  MaybeLogProducerSample("89e88", d.p89e88_samples, r3.u32);
}

void FM2ProdEntry86988(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.p86988_count);
  MaybeLogProducerSample("86988", d.p86988_samples, r3.u32);
}

void FM2HelperEntry87678(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.h87678_count);
}

void FM2HelperEntry637F8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.h637f8_count);
  HitLoadTraceCounter(LoadTrace().helper_637f8);
  MaybeBreakOnLoadPoint637F8();
}

void FM2HelperEntry53D718(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.h53d718_count);
}

void FM2BailCheck9C2C(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 == 0) {
    HitSimple(d.b_workitem_missing);
  }
}

void FM2BailCheck9C60(PPCRegister& r3) {
  auto& d = SigDiag();
  if (r3.u32 == 0) {
    HitSimple(d.b_alloc_fail);
  }
}

void FM2Branch821D03E8NullCheck(PPCRegister& r31) {
  auto& d = SigDiag();
  if (r31.u32 == 0) {
    HitSimple(d.b821d_r31_zero);
  } else {
    HitSimple(d.b821d_r31_nonzero);
  }
}

void FM2Branch821D03E8DivCheck(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 >= 1u) {
    HitSimple(d.b821d_div_ge_1);
  } else {
    HitSimple(d.b821d_div_lt_1);
  }
}

void FM2Branch8259F340NullCheck(PPCRegister& r31) {
  auto& d = SigDiag();
  if (r31.u32 == 0) {
    HitSimple(d.b8259f_r31_zero);
  } else {
    HitSimple(d.b8259f_r31_nonzero);
  }
}

void FM2Branch8259F340DivCheck(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 >= 8u) {
    HitSimple(d.b8259f_div_ge_8);
  } else {
    HitSimple(d.b8259f_div_lt_8);
  }
}

void FM2Branch82534548NullCheck(PPCRegister& r31) {
  auto& d = SigDiag();
  if (r31.u32 == 0) {
    HitSimple(d.b825345_r31_zero);
  } else {
    HitSimple(d.b825345_r31_nonzero);
  }
}

void FM2Branch82534548DivCheck(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 >= 52u) {
    HitSimple(d.b825345_div_ge_52);
  } else {
    HitSimple(d.b825345_div_lt_52);
  }
}

void FM2Branch82209038EntryFlag(PPCRegister& r31) {
  auto& d = SigDiag();
  if ((r31.u32 & 0xFFu) == 0) {
    HitSimple(d.b82209038_flag_zero);
  } else {
    HitSimple(d.b82209038_flag_nonzero);
  }
}

void FM2Branch82209038CmpNeg1(PPCRegister& r3) {
  auto& d = SigDiag();
  if (r3.s32 == -1) {
    HitSimple(d.b82209038_cmp_eq_m1);
  } else {
    HitSimple(d.b82209038_cmp_ne_m1);
  }
}

void FM2Branch82209038Path97C4(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.b82209038_path_97c4);
}

void FM2Branch82209038Path9840(PPCRegister& r11) {
  (void)r11;
  auto& d = SigDiag();
  HitSimple(d.b82209038_path_9840);
}

void FM2DispatchA528PostGate(PPCRegister& r3, PPCRegister& r28, PPCRegister& r29, PPCRegister& r31) {
  auto& d = SigDiag();
  HitSimple(d.d_a528_entry);
  const bool gate_true = (r3.u32 & 0xFFu) != 0;
  if (gate_true) {
    HitSimple(d.d_a528_gate_true);
  } else {
    HitSimple(d.d_a528_gate_false);
  }

  uint8_t* base = GuestBase();
  const uint32_t mode = r28.u32;
  switch (mode) {
    case 5: {
      HitSimple(d.d_a528_mode5);
      const uint32_t lhs = r29.u32;
      const uint32_t rhs = TryLoadU32(base, r31.u32 + 16) + 21u;
      if (lhs == rhs) {
        HitSimple(d.d_a528_m5_match);
      } else {
        HitSimple(d.d_a528_m5_miss);
      }
      break;
    }
    case 6: {
      HitSimple(d.d_a528_mode6);
      const uint32_t lhs = r29.u32;
      const uint32_t rhs = TryLoadU32(base, r31.u32 + 8) + 12u;
      if (lhs == rhs) {
        HitSimple(d.d_a528_m6_match);
      } else {
        HitSimple(d.d_a528_m6_miss);
      }
      break;
    }
    case 7: {
      HitSimple(d.d_a528_mode7);
      const uint32_t lhs = r29.u32;
      const uint32_t rhs = TryLoadU32(base, r31.u32 + 4) + 8u;
      if (lhs == rhs) {
        HitSimple(d.d_a528_m7_match);
      } else {
        HitSimple(d.d_a528_m7_miss);
      }
      break;
    }
    case 8: {
      HitSimple(d.d_a528_mode8);
      if (r29.u32 == 5u) {
        HitSimple(d.d_a528_m8_match);
      } else {
        HitSimple(d.d_a528_m8_miss);
      }
      break;
    }
    case 9: {
      HitSimple(d.d_a528_mode9);
      if (r29.u32 == 5u) {
        HitSimple(d.d_a528_m9_match);
      } else {
        HitSimple(d.d_a528_m9_miss);
      }
      break;
    }
    default:
      HitSimple(d.d_a528_mode_other);
      break;
  }
}

void FM2DispatchA528Route882E0(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_a528_route_882e0);
}

void FM2DispatchA528Route89E88(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_a528_route_89e88);
}

void FM2DispatchA528Route883E0(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_a528_route_883e0);
}

void FM2DispatchA528Route89990(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_a528_route_89990);
}

void FM2DispatchA528Route89978(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_a528_route_89978);
}

void FM2Trace82209038Slot136Target(PPCRegister& r11) {
  auto& d = SigDiag();
  const uint32_t target = r11.u32;
  HitSimple(d.d_slot136_total);
  d.d_slot136_last_target.store(target, std::memory_order_relaxed);
  if (target == 0x8258A528u) {
    HitSimple(d.d_slot136_to_a528);
  } else {
    HitSimple(d.d_slot136_to_other);
  }

  static std::atomic<uint32_t> prev_target{0};
  static std::atomic<uint32_t> change_logs{0};
  const uint32_t prev = prev_target.exchange(target, std::memory_order_relaxed);
  if (prev != target) {
    const uint32_t n = change_logs.fetch_add(1, std::memory_order_relaxed);
    if (n < 24) {
      LogLine("FM2_SLOT136_TARGET_CHANGE n=%u target=%08X", n + 1, target);
    }
  }
}

void FM2Gate82437310Entry(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.d_7310_entry);
}

void FM2Gate82437310Fail1(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 != 0) {
    HitSimple(d.d_7310_fail1);
  }
}

void FM2Gate82437310Fail2(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 == 0) {
    HitSimple(d.d_7310_fail2);
  }
}

void FM2Gate82437310Fail3(PPCRegister& r3) {
  auto& d = SigDiag();
  if (r3.u32 != 0) {
    HitSimple(d.d_7310_fail3);
  }
}

void FM2Gate82437310Pass(PPCRegister& r29) {
  (void)r29;
  auto& d = SigDiag();
  HitSimple(d.d_7310_pass);
}

void FM2Gate82437310Slot148Target(PPCRegister& r11) {
  auto& d = SigDiag();
  const uint32_t target = r11.u32;
  d.d_7310_r148_last_target.store(target, std::memory_order_relaxed);
}

void FM2Gate82437310R148Result(PPCRegister& r11) {
  auto& d = SigDiag();
  if (r11.u32 == 1u) {
    HitSimple(d.d_7310_r148_eq1);
  } else {
    HitSimple(d.d_7310_r148_ne1);
  }
}

void FM2Gate824344C0Entry(PPCRegister& r3, PPCRegister& r4) {
  auto& d = SigDiag();
  HitSimple(d.d_344c0_entry);
  HitLoadTraceCounter(LoadTrace().gate_344c0_entry);
  MaybeLog344C0Sample(r3.u32, r4.u32);
}

void FM2Gate824344C0LoopEnd(PPCRegister& r31) {
  (void)r31;
  auto& d = SigDiag();
  HitSimple(d.d_344c0_loop_end);
}

void FM2Gate824344C0NameCmpResult(PPCRegister& r3) {
  auto& d = SigDiag();
  if (r3.s32 == 0) {
    HitSimple(d.d_344c0_name_eq);
  } else {
    HitSimple(d.d_344c0_name_ne);
  }
}

void FM2Gate824344C0Match(PPCRegister& r26) {
  (void)r26;
  auto& d = SigDiag();
  HitSimple(d.d_344c0_match);
  HitLoadTraceCounter(LoadTrace().gate_344c0_match);
}

void FM2AllocCallsite0E70(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.d_alloc_cs_0e70_count);
  d.d_alloc_cs_0e70_last_size.store(r3.u32, std::memory_order_relaxed);
  HitAllocBuckets(r3.u32, d.d_alloc_cs_0e70_lt32, d.d_alloc_cs_0e70_32_255,
                  d.d_alloc_cs_0e70_256_4095, d.d_alloc_cs_0e70_ge4096);
}

void FM2AllocCallsite7578(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.d_alloc_cs_7578_count);
  d.d_alloc_cs_7578_last_size.store(r3.u32, std::memory_order_relaxed);
  HitAllocBuckets(r3.u32, d.d_alloc_cs_7578_lt32, d.d_alloc_cs_7578_32_255,
                  d.d_alloc_cs_7578_256_4095, d.d_alloc_cs_7578_ge4096);
}

void FM2AllocCallsite8C98(PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.d_alloc_cs_8c98_count);
  d.d_alloc_cs_8c98_last_size.store(r3.u32, std::memory_order_relaxed);
  HitAllocBuckets(r3.u32, d.d_alloc_cs_8c98_lt32, d.d_alloc_cs_8c98_32_255,
                  d.d_alloc_cs_8c98_256_4095, d.d_alloc_cs_8c98_ge4096);
}

void FM2AllocPath1D03E8Entry(PPCRegister& r12, PPCRegister& r3) {
  auto& d = SigDiag();
  HitSimple(d.d_1d03e8_entry);
  HitLoadTraceCounter(LoadTrace().alloc_1d03e8);
  d.d_1d03e8_last_lr.store(r12.u32, std::memory_order_relaxed);
  d.d_1d03e8_last_size.store(r3.u32, std::memory_order_relaxed);
  switch (r12.u32) {
    case 0x821D0E74u:
      HitSimple(d.d_1d03e8_lr_0e74);
      break;
    case 0x8261757Cu:
      HitSimple(d.d_1d03e8_lr_757c);
      break;
    case 0x82618C9Cu:
      HitSimple(d.d_1d03e8_lr_8c9c);
      break;
    default: {
      HitSimple(d.d_1d03e8_lr_other);
      const uint64_t n = d.d_1d03e8_other_samples.fetch_add(1, std::memory_order_relaxed) + 1;
      if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
        LogLine("FM2_1D03E8_OTHER_LR n=%llu lr=%08X size=%u", static_cast<unsigned long long>(n),
                r12.u32, r3.u32);
      }
      break;
    }
  }
}

void FM2AllocGrowPath1D0E10Entry(PPCRegister& r12, PPCRegister& r4, PPCRegister& r5) {
  auto& d = SigDiag();
  HitSimple(d.d_1d0e10_entry);
  HitLoadTraceCounter(LoadTrace().alloc_1d0e10);
  d.d_1d0e10_last_lr.store(r12.u32, std::memory_order_relaxed);
  d.d_1d0e10_last_req.store(r4.u32, std::memory_order_relaxed);
  d.d_1d0e10_last_copy.store(r5.u32, std::memory_order_relaxed);

  HitSimple(d.d_1d0e10_lr_other);
  const uint64_t n = d.d_1d0e10_lr_sample_count.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
    LogLine("FM2_1D0E10_LR_SAMPLE n=%llu tid=%u lr=%08X req=%u copy=%u",
            static_cast<unsigned long long>(n), static_cast<unsigned>(GetCurrentThreadId()),
            r12.u32, r4.u32, r5.u32);
  }
}

void FM2AllocEnsurePath1D1568Entry(PPCRegister& r12, PPCRegister& r4, PPCRegister& r5) {
  auto& d = SigDiag();
  HitSimple(d.d_1d1568_entry);
  HitLoadTraceCounter(LoadTrace().alloc_1d1568);
  d.d_1d1568_last_lr.store(r12.u32, std::memory_order_relaxed);
  d.d_1d1568_last_req.store(r4.u32, std::memory_order_relaxed);
  d.d_1d1568_last_flag.store(r5.u32 & 0xFFu, std::memory_order_relaxed);

  HitSimple(d.d_1d1568_lr_other);
  const uint64_t n = d.d_1d1568_lr_sample_count.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
    LogLine("FM2_1D1568_LR_SAMPLE n=%llu tid=%u lr=%08X req=%u flag=%u",
            static_cast<unsigned long long>(n), static_cast<unsigned>(GetCurrentThreadId()), r12.u32,
            r4.u32, static_cast<unsigned>(r5.u32 & 0xFFu));
  }
}

void FM2StrCore25C0Entry(PPCRegister& r12) {
  auto& d = SigDiag();
  HitSimple(d.d_1d25c0_entry);
  HitLoadTraceCounter(LoadTrace().str_25c0);
  d.d_1d25c0_last_lr.store(r12.u32, std::memory_order_relaxed);
  const uint64_t n = d.d_1d25c0_samples.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
    LogLine("FM2_STR25C0_LR_SAMPLE n=%llu tid=%u lr=%08X", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()), r12.u32);
  }
}

void FM2StrCore24D8Entry(PPCRegister& r12) {
  auto& d = SigDiag();
  HitSimple(d.d_1d24d8_entry);
  HitLoadTraceCounter(LoadTrace().str_24d8);
  d.d_1d24d8_last_lr.store(r12.u32, std::memory_order_relaxed);
  const uint64_t n = d.d_1d24d8_samples.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
    LogLine("FM2_STR24D8_LR_SAMPLE n=%llu tid=%u lr=%08X", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()), r12.u32);
  }
}

void FM2StrCore30C10Entry(PPCRegister& r12) {
  auto& d = SigDiag();
  HitSimple(d.d_430c10_entry);
  HitLoadTraceCounter(LoadTrace().str_30c10);
  d.d_430c10_last_lr.store(r12.u32, std::memory_order_relaxed);
  const uint64_t n = d.d_430c10_samples.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 64 && (n <= 16 || (n % 8) == 0)) {
    LogLine("FM2_STR30C10_LR_SAMPLE n=%llu tid=%u lr=%08X", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()), r12.u32);
  }
}

void FM2PathBuilder5CF298Entry(PPCRegister& r12, PPCRegister& r4, PPCRegister& r5) {
  auto& d = SigDiag();
  HitSimple(d.d_5cf298_entry);
  HitLoadTraceCounter(LoadTrace().path_5cf298);
  d.d_5cf298_last_lr.store(r12.u32, std::memory_order_relaxed);
  d.d_5cf298_last_flag.store(r5.u32 & 0xFFu, std::memory_order_relaxed);
  MaybeLogLoadTracePathSample(r12.u32, r5.u32 & 0xFFu, r4.u32);
  const uint64_t n = d.d_5cf298_samples.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 40 && (n <= 16 || (n % 8) == 0)) {
    char path[65];
    SnapshotGuestCString(r4.u32, path);
    LogLine("FM2_5CF298_SAMPLE n=%llu tid=%u lr=%08X flag=%u path=%s",
            static_cast<unsigned long long>(n), static_cast<unsigned>(GetCurrentThreadId()), r12.u32,
            static_cast<unsigned>(r5.u32 & 0xFFu), path);
  }
}

void FM2BufferedFileReadAsyncAwareEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  HitLoadTraceCounter(LoadTrace().buffered_async);
  MaybeLogLoadTraceReadSample("BufferedFileReadAsyncAware", r3.u32, r4.u32, r5.u32);
}

void FM2BufferedFileReadEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  HitLoadTraceCounter(LoadTrace().buffered_sync);
  MaybeLogLoadTraceReadSample("BufferedFileRead", r3.u32, r4.u32, r5.u32);
}

void FM2FmodWorkerLoop825890F8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_loop_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                             std::memory_order_relaxed);
  d.fmod_hook_loop_hits.fetch_add(1, std::memory_order_relaxed);
  BindFmodWorkerThread("825890F8");
}

void FM2FmodGateEntry8220A4E8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_gate_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                             std::memory_order_relaxed);
  d.fmod_hook_gate_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_gate_entry_any.fetch_add(1, std::memory_order_relaxed);
  if (d.force_a4e8_setevent_every_call) {
    uint8_t* base = GuestBase();
    if (base && GuestReadableRange(base, 0x829C24C8u, 4u)) {
      REX_STORE_U32(0x829C24C8u, 2u);
    }
  }
  AutoBindFmodThreadIfUnset("8220A4E8");
  if (IsFmodWorkerThread()) {
    d.fmod_gate_entry.fetch_add(1, std::memory_order_relaxed);
  }
  MaybeEmitSigPerSec();
}

void FM2FmodWaitCall8258916C(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  auto& d = SigDiag();
  d.fmod_wait_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                             std::memory_order_relaxed);
  d.fmod_hook_wait_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_wait_call_any.fetch_add(1, std::memory_order_relaxed);
  AutoBindFmodThreadIfUnset("8258916C");
  if (IsFmodWorkerThread()) {
    d.fmod_wait_call.fetch_add(1, std::memory_order_relaxed);
    d.fmod_wait_last_handle.store(r3.u32, std::memory_order_relaxed);
    if (r4.u32 == 32u) {
      d.fmod_wait_timeout32.fetch_add(1, std::memory_order_relaxed);
    }
    if (r5.u32 == 1u) {
      d.fmod_wait_alertable1.fetch_add(1, std::memory_order_relaxed);
    }
  }
  MaybeEmitSigPerSec();
}

void FM2FmodWorkTick82588C10(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_work_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                             std::memory_order_relaxed);
  d.fmod_hook_work_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_work_tick_any.fetch_add(1, std::memory_order_relaxed);
  AutoBindFmodThreadIfUnset("82588C10");
  if (IsFmodWorkerThread()) {
    d.fmod_work_tick.fetch_add(1, std::memory_order_relaxed);
  }
  MaybeEmitSigPerSec();
}

void FM2FmodDecodeRead826938E8(PPCRegister& r3, PPCRegister& r4, PPCRegister& r6) {
  (void)r4;
  auto& d = SigDiag();
  d.fmod_decode_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                               std::memory_order_relaxed);
  d.fmod_hook_decode_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_decode_read_any.fetch_add(1, std::memory_order_relaxed);
  d.fmod_read_calls.fetch_add(1, std::memory_order_relaxed);

  uint8_t* base = GuestBase();
  if (base && GuestReadableRange(base, r3.u32 + 220u, 4u)) {
    const uint32_t slot = REX_LOAD_U32(r3.u32 + 220u);
    if (GuestReadableRange(base, slot, 4u)) {
      const uint32_t ctx = REX_LOAD_U32(slot);
      d.fmod_read_last_ctx.store(ctx, std::memory_order_relaxed);
      if (ctx == 0xFFCA8300u) {
        d.fmod_read_ctx_ffca8300_hits.fetch_add(1, std::memory_order_relaxed);
      }
    }
  }
  if (base && r6.u32 != 0 && GuestReadableRange(base, r6.u32, 4u)) {
    d.fmod_read_last_cursor.store(REX_LOAD_U32(r6.u32), std::memory_order_relaxed);
  }

  AutoBindFmodThreadIfUnset("826938E8");
  if (IsFmodWorkerThread()) {
    d.fmod_decode_read.fetch_add(1, std::memory_order_relaxed);
  }
  MaybeEmitSigPerSec();
}

void FM2FmodReadCopy1At82693970(PPCRegister& r5) {
  auto& d = SigDiag();
  d.fmod_read_copy1_calls.fetch_add(1, std::memory_order_relaxed);
  d.fmod_read_copy1_bytes.fetch_add(r5.u32, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodReadSizeAt82693954(PPCRegister& r5, PPCRegister& r29) {
  auto& d = SigDiag();
  (void)r29;
  uint32_t req = r5.u32;
  if (d.force_read_size_4096 && req == 2048u) {
    req = 4096u;
    r5.u32 = req;
    d.fmod_read_req_forced_4096.fetch_add(1, std::memory_order_relaxed);
  }
  d.fmod_read_req_bytes.fetch_add(req, std::memory_order_relaxed);
  d.fmod_read_last_req.store(req, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodReadCopy2At82693988(PPCRegister& r5) {
  auto& d = SigDiag();
  d.fmod_read_copy2_calls.fetch_add(1, std::memory_order_relaxed);
  d.fmod_read_copy2_bytes.fetch_add(r5.u32, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodRenderCallback823EBF20(PPCRegister& r3) {
  (void)r3;
  MaybePollLoadTraceToggle();
  auto& d = SigDiag();
  d.fmod_render_cb_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_render_cb_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                  std::memory_order_relaxed);
  static std::atomic<uint64_t> s{0};
  const uint64_t n = s.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 12) {
    LogLine("FM2_FMOD_RENDER_CB_SAMPLE n=%llu tid=%u", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()));
  }
  MaybeEmitSigPerSec();
}

void FM2FmodRenderDispatch823E83C8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_render_dispatch_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_render_dispatch_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                        std::memory_order_relaxed);
  static std::atomic<uint64_t> s{0};
  const uint64_t n = s.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 12) {
    LogLine("FM2_FMOD_RENDER_DISP_SAMPLE n=%llu tid=%u", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()));
  }
  MaybeEmitSigPerSec();
}

void FM2FmodDecodeThunk82693B18(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_decode_thunk_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_decode_thunk_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                     std::memory_order_relaxed);
  static std::atomic<uint64_t> s{0};
  const uint64_t n = s.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 12) {
    LogLine("FM2_FMOD_DECODE_THUNK_SAMPLE n=%llu tid=%u", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()));
  }
  MaybeEmitSigPerSec();
}

void FM2FmodFill82692CC8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_fill_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_fill_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                             std::memory_order_relaxed);
  static std::atomic<uint64_t> s{0};
  const uint64_t n = s.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 12) {
    LogLine("FM2_FMOD_FILL_SAMPLE n=%llu tid=%u", static_cast<unsigned long long>(n),
            static_cast<unsigned>(GetCurrentThreadId()));
  }
  MaybeEmitSigPerSec();
}

void FM2FmodSchedEB998(PPCRegister& r3, PPCRegister& r4) {
  (void)r3;
  (void)r4;
  auto& d = SigDiag();
  d.fmod_sched_eb998_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_sched_eb998_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                    std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodSchedEB9D0(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  d.fmod_sched_eb9d0_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_sched_eb9d0_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                    std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodIrqDispatch8236C380(PPCRegister& r3) {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_irq_6c380_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_irq_6c380_last_tid.store(static_cast<uint32_t>(GetCurrentThreadId()),
                                  std::memory_order_relaxed);
  d.fmod_irq_last_obj.store(r3.u32, std::memory_order_relaxed);

  uint8_t* base = GuestBase();
  if (base && GuestReadableRange(base, r3.u32 + 16328u, 4u)) {
    const uint32_t cur = REX_LOAD_U32(r3.u32 + 16324u);
    const uint32_t tgt = REX_LOAD_U32(r3.u32 + 16328u);
    const uint32_t pending = REX_LOAD_U32(r3.u32 + 16192u);
    const uint32_t ticket = REX_LOAD_U32(r3.u32 + 16172u);
    d.fmod_irq_last_cur.store(cur, std::memory_order_relaxed);
    d.fmod_irq_last_target.store(tgt, std::memory_order_relaxed);
    d.fmod_irq_last_pending.store(pending, std::memory_order_relaxed);
    d.fmod_irq_last_ticket.store(ticket, std::memory_order_relaxed);
    if (cur == tgt) {
      d.fmod_irq_6c380_eq_hits.fetch_add(1, std::memory_order_relaxed);
    }
  }
  MaybeEmitSigPerSec();
}

void FM2FmodIrqSchedule8236C4F0(PPCRegister& r3) {
  auto& d = SigDiag();
  uint32_t arg = r3.u32;
  if (d.force_sched_mode2 && arg == 0x114u) {
    arg = 0x214u;
    r3.u32 = arg;
  }
  if (!d.enabled) {
    return;
  }
  const uint32_t mode = (arg >> 8) & 0xFu;
  const uint32_t thr = arg & 0xFFu;
  const uint32_t thr_bucket = arg & 0xFu;
  d.fmod_irq_sched_6c4f0_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_irq_sched_last_arg.store(arg, std::memory_order_relaxed);
  d.fmod_irq_sched_last_mode.store(mode, std::memory_order_relaxed);
  d.fmod_irq_sched_last_thr.store(thr, std::memory_order_relaxed);
  d.fmod_irq_sched_mode_hist[mode].fetch_add(1, std::memory_order_relaxed);
  d.fmod_irq_sched_thr_hist[thr_bucket].fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodIrqSchedOverThr8236C5AC() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_irq_sched_over_thr_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodIrqSchedPathQueued8236C618() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_irq_sched_path_queued_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodIrqSchedPathImmediate8236C640() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_irq_sched_path_immediate_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodIrqSubmit8236C688(PPCRegister& r4, PPCRegister& r12) {
  auto& d = SigDiag();
  if (d.force_submit_mode3) {
    r4.u32 = 0x200u;
  }
  if (!d.enabled) {
    return;
  }
  const uint32_t a2 = r4.u32;
  const uint32_t lr = r12.u32;
  d.fmod_irq_submit_6c688_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_irq_submit_last_a2.store(a2, std::memory_order_relaxed);
  d.fmod_irq_submit_last_lr.store(lr, std::memory_order_relaxed);
  if ((a2 & 0xFFu) == 20u) {
    d.fmod_irq_submit_a2_20_hits.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.fmod_irq_submit_a2_other_hits.fetch_add(1, std::memory_order_relaxed);
  }
  switch (lr) {
    case 0x8236C7B0u:
      d.fmod_irq_submit_lr_a_hits.fetch_add(1, std::memory_order_relaxed);
      break;
    case 0x8236D134u:
      d.fmod_irq_submit_lr_b_hits.fetch_add(1, std::memory_order_relaxed);
      break;
    default:
      d.fmod_irq_submit_lr_other_hits.fetch_add(1, std::memory_order_relaxed);
      break;
  }
  MaybeEmitSigPerSec();
}

void FM2FmodWrap8236C8C8() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_wrap_6c8c8_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodWrap8236C948() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_wrap_6c948_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodWrap8236CB20() {
  auto& d = SigDiag();
  if (!d.enabled) {
    return;
  }
  d.fmod_wrap_6cb20_hits.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitSigPerSec();
}

void FM2FmodPumpThread82381D60(PPCRegister& r26) {
  (void)r26;
  auto& d = SigDiag();
  d.fmod_pump_thread_81d60_hits.fetch_add(1, std::memory_order_relaxed);
  AutoBindFmodThreadIfUnset("82381D60");
  MaybeEmitSigPerSec();
}

void FM2FmodPumpWaitPrep82381DE4(PPCRegister& r29) {
  auto& d = SigDiag();
  d.fmod_pump_waitprep_81de4_hits.fetch_add(1, std::memory_order_relaxed);

  if (r29.u32 != 0) {
    d.fmod_pump_waitprep_ptr_nonzero.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.fmod_pump_waitprep_ptr_zero.fetch_add(1, std::memory_order_relaxed);
  }

  if (d.force_pump_wait_override && r29.u32 != 0) {
    int32_t ms = d.force_pump_wait_ms;
    if (ms < 1) {
      ms = 1;
    } else if (ms > 32) {
      ms = 32;
    }
    const int64_t wait_100ns = -static_cast<int64_t>(ms) * 10000ll;
    uint8_t* base = GuestBase();
    if (base && GuestReadableRange(base, r29.u32, 8u)) {
      REX_STORE_U64(r29.u32, static_cast<uint64_t>(wait_100ns));
      d.fmod_pump_wait_override_hits.fetch_add(1, std::memory_order_relaxed);
      d.fmod_pump_wait_last_ms.store(static_cast<uint32_t>(ms), std::memory_order_relaxed);
    }
  }

  MaybeEmitSigPerSec();
}

void FM2FmodPumpWaitResult82381DFC(PPCRegister& r3) {
  auto& d = SigDiag();
  d.fmod_pump_waitresult_81dfc_hits.fetch_add(1, std::memory_order_relaxed);
  d.fmod_pump_waitresult_last.store(r3.u32, std::memory_order_relaxed);
  if (r3.u32 == 258u) {
    d.fmod_pump_waitresult_timeout258.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.fmod_pump_waitresult_other.fetch_add(1, std::memory_order_relaxed);
  }
  MaybeEmitSigPerSec();
}

void FM2RegSnapshot864F0(PPCRegister& r31, PPCRegister& r26) {
  MaybeLogRegistrationSnapshot(r31.u32, r26.u32);
}

void FM2Caller82214CF0(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.c82214cf0_count);
}

void FM2Caller82599A88(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.c82599a88_count);
}

void FM2Caller821E38C8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.c821e38c8_count);
}

void FM2Caller8229E368(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.c8229e368_count);
}

void FM2Caller8235F3D8(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.c8235f3d8_count);
}

void FM2ProducerProgressGuardWait823693F8() {
  HitLoadTraceCounter(LoadTrace().prod_guard_wait);
  if (REXCVAR_GET(fm2_prod_guard_stats)) {
    auto& d = ProdGuardDiag();
    d.wait_ret1.fetch_add(1, std::memory_order_relaxed);
    MaybeEmitProdGuardPerSec();
  }

  uint32_t pause_count = REXCVAR_GET(fm2_prod_guard_wait_pause_count);
  if (pause_count > 64u) {
    pause_count = 64u;
  }
  for (uint32_t i = 0; i < pause_count; ++i) {
    rex::ppc::delay_execution();
  }

  const uint32_t yield_interval = REXCVAR_GET(fm2_prod_guard_wait_yield_interval);
  if (yield_interval != 0u) {
    thread_local uint32_t hit_counter = 0;
    ++hit_counter;
    if (hit_counter >= yield_interval) {
      hit_counter = 0;
      rex::thread::MaybeYield();
    }
  }
}

void FM2ProducerProgressGuardEntry82369340(PPCRegister& lr) {
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  ProdGuardHitCaller(lr.u32);
}

void FM2ProducerProgressGuardFlagBlocked82369390(PPCRegister& lr) {
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  ProdGuardHitCaller(lr.u32);
  d.flag_blocked.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitProdGuardPerSec();
}

void FM2ProducerProgressGuardCursorCmp823693B8(PPCRegister& r9, PPCRegister& r10) {
  if (!REXCVAR_GET(fm2_prod_guard_stats) || !REXCVAR_GET(fm2_prod_guard_trace)) {
    return;
  }
  auto& d = ProdGuardDiag();
  if (r9.u32 == r10.u32) {
    d.cursor_eq.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.cursor_ne.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2ProducerProgressGuardWaitDetail823693F8(PPCRegister& lr, PPCRegister& delta,
                                                PPCRegister& r30, PPCRegister& r31) {
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  ProdGuardHitCaller(lr.u32);
  const uint32_t dlt = delta.u32;
  d.wait_delta_sum.fetch_add(dlt, std::memory_order_relaxed);
  d.last_wait_delta.store(dlt, std::memory_order_relaxed);
  AtomicMaxU64(d.wait_delta_max, dlt);
  if (dlt < 64u) {
    d.wait_delta_lt64.fetch_add(1, std::memory_order_relaxed);
  } else if (dlt < 256u) {
    d.wait_delta_64_255.fetch_add(1, std::memory_order_relaxed);
  } else if (dlt < 1024u) {
    d.wait_delta_256_1023.fetch_add(1, std::memory_order_relaxed);
  } else if (dlt < 4096u) {
    d.wait_delta_1024_4095.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.wait_delta_4096_4999.fetch_add(1, std::memory_order_relaxed);
  }

  if (REXCVAR_GET(fm2_prod_guard_trace)) {
    const uint32_t sample_interval = REXCVAR_GET(fm2_prod_guard_trace_sample_interval);
    if (sample_interval != 0u) {
      const uint64_t prior = d.wait_ret1.load(std::memory_order_relaxed);
      if ((prior % sample_interval) == 0u) {
        uint32_t last_producer_tick = 0u;
        uint32_t ticket = 0u;
        uint8_t* base = GuestBase();
        if (base && GuestReadableRange(base, r31.u32 + 8u, 8u)) {
          last_producer_tick = REX_LOAD_U32(r31.u32 + 12u);
          ticket = REX_LOAD_U32(r31.u32 + 8u);
        }
        LogLineProdGuard(
            "FM2_PROD_GUARD_WAIT_SAMPLE lr=%08X delta=%u cur=%u last=%u ticket=%u state_ptr=%08X",
            static_cast<unsigned int>(lr.u32), dlt, static_cast<unsigned int>(r30.u32),
            static_cast<unsigned int>(last_producer_tick), static_cast<unsigned int>(ticket),
            static_cast<unsigned int>(r31.u32));
        d.sample_wait_logs.fetch_add(1, std::memory_order_relaxed);
      }
    }
  }
}

void FM2ProducerWaitLoopSample82372A68(PPCRegister& obj, PPCRegister& target) {
  if (!REXCVAR_GET(fm2_prod_guard_stats) || !REXCVAR_GET(fm2_prod_guard_trace)) {
    return;
  }
  auto& d = ProdGuardDiag();
  d.loop72_hits.fetch_add(1, std::memory_order_relaxed);
  d.loop72_last_obj.store(obj.u32, std::memory_order_relaxed);
  d.loop72_last_target.store(target.u32, std::memory_order_relaxed);
  if (obj.u32 == 0u) {
    d.loop72_obj_zero.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  uint32_t limit = 0u;
  uint32_t cursor = 0u;
  uint32_t flag12944 = 0u;
  uint8_t* base = GuestBase();
  if (base && GuestReadableRange(base, obj.u32 + 10768u, 16u) &&
      GuestReadableRange(base, obj.u32 + 12944u, 4u)) {
    const uint32_t cursor_ptr = REX_LOAD_U32(obj.u32 + 10768u);
    limit = REX_LOAD_U32(obj.u32 + 10780u);
    flag12944 = REX_LOAD_U32(obj.u32 + 12944u);
    if (cursor_ptr != 0u && GuestReadableRange(base, cursor_ptr, 4u)) {
      cursor = REX_LOAD_U32(cursor_ptr);
    }
  }

  const uint32_t need = limit - target.u32;
  const uint32_t avail = limit - cursor;
  const uint32_t gap = avail > need ? (avail - need) : 0u;
  d.loop72_last_limit.store(limit, std::memory_order_relaxed);
  d.loop72_last_cursor.store(cursor, std::memory_order_relaxed);
  d.loop72_last_need.store(need, std::memory_order_relaxed);
  d.loop72_last_avail.store(avail, std::memory_order_relaxed);
  d.loop72_last_gap.store(gap, std::memory_order_relaxed);
  d.loop72_last_flag12944.store(flag12944, std::memory_order_relaxed);

  if (limit == 0u) {
    d.loop72_limit_zero.fetch_add(1, std::memory_order_relaxed);
  }
  if (cursor == 0u) {
    d.loop72_cursor_zero.fetch_add(1, std::memory_order_relaxed);
  }
  if (target.u32 == 0u) {
    d.loop72_target_zero.fetch_add(1, std::memory_order_relaxed);
  }
  if (need < avail) {
    d.loop72_need_lt_avail.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.loop72_need_ge_avail.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2ProducerWaitLoopGuardResult82372A70(PPCRegister& result) {
  if (!REXCVAR_GET(fm2_prod_guard_stats) || !REXCVAR_GET(fm2_prod_guard_trace)) {
    return;
  }
  auto& d = ProdGuardDiag();
  if (result.u32 == 0u) {
    d.loop72_guard_ret0.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.loop72_guard_ret1.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2ProducerWaitLoopCall73078Before82372A38(PPCRegister& obj) {
  if (!REXCVAR_GET(fm2_prod_guard_stats) || !REXCVAR_GET(fm2_prod_guard_trace)) {
    return;
  }
  auto& d = ProdGuardDiag();
  d.call73078_pre.fetch_add(1, std::memory_order_relaxed);
  if (obj.u32 == 0u) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base || !GuestReadableRange(base, obj.u32 + 10768u, 16u)) {
    return;
  }
  const uint32_t cursor_ptr = REX_LOAD_U32(obj.u32 + 10768u);
  const uint32_t limit = REX_LOAD_U32(obj.u32 + 10780u);
  uint32_t cursor = 0u;
  if (cursor_ptr != 0u && GuestReadableRange(base, cursor_ptr, 4u)) {
    cursor = REX_LOAD_U32(cursor_ptr);
  }
  d.call73078_last_lim_before.store(limit, std::memory_order_relaxed);
  d.call73078_last_cur_before.store(cursor, std::memory_order_relaxed);
}

void FM2ProducerWaitLoopCall73078After82372A38(PPCRegister& obj) {
  if (!REXCVAR_GET(fm2_prod_guard_stats) || !REXCVAR_GET(fm2_prod_guard_trace)) {
    return;
  }
  auto& d = ProdGuardDiag();
  d.call73078_post.fetch_add(1, std::memory_order_relaxed);
  if (obj.u32 == 0u) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base || !GuestReadableRange(base, obj.u32 + 10768u, 16u)) {
    return;
  }
  const uint32_t cursor_ptr = REX_LOAD_U32(obj.u32 + 10768u);
  const uint32_t limit = REX_LOAD_U32(obj.u32 + 10780u);
  uint32_t cursor = 0u;
  if (cursor_ptr != 0u && GuestReadableRange(base, cursor_ptr, 4u)) {
    cursor = REX_LOAD_U32(cursor_ptr);
  }
  d.call73078_last_lim_after.store(limit, std::memory_order_relaxed);
  d.call73078_last_cur_after.store(cursor, std::memory_order_relaxed);
  const uint32_t lim_before = d.call73078_last_lim_before.load(std::memory_order_relaxed);
  const uint32_t cur_before = d.call73078_last_cur_before.load(std::memory_order_relaxed);
  if (lim_before != limit || cur_before != cursor) {
    d.call73078_changed.fetch_add(1, std::memory_order_relaxed);
  }
}

bool FM2ProducerWaitLoopShouldSpin82372A78(PPCRegister& need, PPCRegister& avail) {
  if (need.u32 >= avail.u32) {
    return false;
  }
  const uint32_t gap = avail.u32 - need.u32;
  const uint32_t break_threshold = REXCVAR_GET(fm2_prod_waitloop_spin_min_gap);
  if (break_threshold != 0u && gap <= break_threshold) {
    if (REXCVAR_GET(fm2_prod_guard_stats) && REXCVAR_GET(fm2_prod_guard_trace)) {
      auto& d = ProdGuardDiag();
      d.loop72_small_gap_breaks.fetch_add(1, std::memory_order_relaxed);
      d.loop72_last_gap.store(gap, std::memory_order_relaxed);
      d.loop72_last_break_threshold.store(break_threshold, std::memory_order_relaxed);
    }
    return false;
  }

  const uint32_t yield_interval = REXCVAR_GET(fm2_prod_waitloop_yield_interval);
  if (yield_interval != 0u) {
    thread_local uint32_t spin_hits = 0u;
    ++spin_hits;
    if (spin_hits >= yield_interval) {
      spin_hits = 0u;
      rex::thread::MaybeYield();
      if (REXCVAR_GET(fm2_prod_guard_stats) && REXCVAR_GET(fm2_prod_guard_trace)) {
        auto& d = ProdGuardDiag();
        d.loop72_yields.fetch_add(1, std::memory_order_relaxed);
        d.loop72_last_yield_interval.store(yield_interval, std::memory_order_relaxed);
      }
    }
  }
  return true;
}

void FM2ProducerProgressGuardTimeout82369400() {
  HitLoadTraceCounter(LoadTrace().prod_guard_timeout);
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  d.timeout_call.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitProdGuardPerSec();
}

void FM2ProducerProgressGuardTimeoutDetail82369400(PPCRegister& lr, PPCRegister& delta) {
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  ProdGuardHitCaller(lr.u32);
  const uint32_t dlt = delta.u32;
  d.timeout_delta_ge5000.fetch_add(1, std::memory_order_relaxed);
  d.timeout_delta_sum.fetch_add(dlt, std::memory_order_relaxed);
  d.last_timeout_delta.store(dlt, std::memory_order_relaxed);
  AtomicMaxU64(d.timeout_delta_max, dlt);
}

void FM2ProducerProgressGuardReturnZero82369408() {
  HitLoadTraceCounter(LoadTrace().prod_guard_ret0);
  if (!REXCVAR_GET(fm2_prod_guard_stats)) {
    return;
  }
  auto& d = ProdGuardDiag();
  d.ret0.fetch_add(1, std::memory_order_relaxed);
  MaybeEmitProdGuardPerSec();
}

void FM2ApuMixRenderEnter82697F08() {
  MaybePollLoadTraceToggle();
  if (!REXCVAR_GET(fm2_apu_mix_stats)) {
    return;
  }

  auto& d = ApuMixDiag();
  d.calls.fetch_add(1, std::memory_order_relaxed);
  if (g_apu_mix_depth < kApuMixMaxDepth) {
    g_apu_mix_enter_ns[g_apu_mix_depth++] = NowNs();
  } else {
    d.stack_overflow.fetch_add(1, std::memory_order_relaxed);
    ++g_apu_mix_dropped;
  }
  MaybeEmitApuMixPerSec();
}

void FM2ApuMixRenderExitA826983A0() {
  if (!REXCVAR_GET(fm2_apu_mix_stats)) {
    return;
  }

  auto& d = ApuMixDiag();
  d.exits_a.fetch_add(1, std::memory_order_relaxed);
  if (g_apu_mix_depth == 0) {
    if (g_apu_mix_dropped != 0u) {
      --g_apu_mix_dropped;
      MaybeEmitApuMixPerSec();
      return;
    }
    d.unmatched_exit.fetch_add(1, std::memory_order_relaxed);
    MaybeEmitApuMixPerSec();
    return;
  }

  const uint64_t start_ns = g_apu_mix_enter_ns[--g_apu_mix_depth];
  const uint64_t elapsed_ns = NowNs() - start_ns;
  d.total_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
  AtomicMaxU64(d.max_ns, elapsed_ns);
  MaybeEmitApuMixPerSec();
}

void FM2ApuMixRenderExitB826983C0() {
  if (!REXCVAR_GET(fm2_apu_mix_stats)) {
    return;
  }

  auto& d = ApuMixDiag();
  d.exits_b.fetch_add(1, std::memory_order_relaxed);
  if (g_apu_mix_depth == 0) {
    if (g_apu_mix_dropped != 0u) {
      --g_apu_mix_dropped;
      MaybeEmitApuMixPerSec();
      return;
    }
    d.unmatched_exit.fetch_add(1, std::memory_order_relaxed);
    MaybeEmitApuMixPerSec();
    return;
  }

  const uint64_t start_ns = g_apu_mix_enter_ns[--g_apu_mix_depth];
  const uint64_t elapsed_ns = NowNs() - start_ns;
  d.total_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
  AtomicMaxU64(d.max_ns, elapsed_ns);
  MaybeEmitApuMixPerSec();
}

void FM2HelperEntry63768(PPCRegister& r12) {
  auto& d = SigDiag();
  HitSimple(d.h63768_count);
  HitLoadTraceCounter(LoadTrace().helper_63768);
  MaybeBreakOnLoadPointLR(r12.u32);
  switch (r12.u32) {
    case 0x821D0448u:
      HitSimple(d.lr821d0448_count);
      break;
    case 0x8259F3A0u:
      HitSimple(d.lr8259f3a0_count);
      break;
    case 0x825345A8u:
      HitSimple(d.lr825345a8_count);
      break;
    case 0x822097C8u:
      HitSimple(d.lr822097c8_count);
      break;
    default:
      HitSimple(d.lr_other_count);
      break;
  }
  const uint64_t n = d.h63768_total_count.fetch_add(1, std::memory_order_relaxed) + 1;
  if (n <= 10000 && (n % 250) == 0) {
    LogLine("FM2_63768_LR_SAMPLE n=%llu lr=%08X", static_cast<unsigned long long>(n), r12.u32);
  }
}

void FM2HelperEntry67F60(PPCRegister& r4) {
  auto& d = SigDiag();
  HitSimple(d.h67f60_count);
  HitLoadTraceCounter(LoadTrace().helper_67f60);
  if (IsLoadTraceActive()) {
    HitLoadTraceAllocPoolRequest(r4.u32);
  }
}

void FM2AllocPoolTryAcquirePoolResult82367FA8(PPCRegister& r30, PPCRegister& r31) {
  (void)r30;
  if (!IsLoadTraceActive()) {
    return;
  }
  auto& d = LoadTrace();
  if (r31.u32 != 0u) {
    d.alloc_pool_fast_hit.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.alloc_pool_fast_miss.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2AllocPoolTryAcquireFallbackResult82368004(PPCRegister& r30, PPCRegister& r31) {
  (void)r30;
  if (!IsLoadTraceActive()) {
    return;
  }
  auto& d = LoadTrace();
  d.alloc_pool_fallback_calls.fetch_add(1, std::memory_order_relaxed);
  if (r31.u32 != 0u) {
    d.alloc_pool_fallback_hit.fetch_add(1, std::memory_order_relaxed);
  } else {
    d.alloc_pool_fallback_fail.fetch_add(1, std::memory_order_relaxed);
  }
}

void FM2HelperEntry63538(PPCRegister& r3) {
  (void)r3;
  auto& d = SigDiag();
  HitSimple(d.h63538_count);
  HitLoadTraceCounter(LoadTrace().helper_63538);
}
