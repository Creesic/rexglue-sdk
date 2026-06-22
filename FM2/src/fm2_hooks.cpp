#include "generated/fm2_init.h"
#include "native_renderer/fm2_direct_draw_decode.h"
#include "native_renderer/fm2_native_renderer.h"
#include "native_renderer/fm2_native_state.h"
#include "native_renderer/fm2_shader_analysis.h"

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

namespace {

namespace fm2nr = fm2::native_renderer;

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
    fm2_plume_trace_direct_decode, false, "FM2",
    "Emit sampled FM2 Plume direct indexed-draw record/segment decode lines");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_decode_limit, 8, "FM2",
    "Maximum FM2 Plume direct indexed-draw decode samples per process");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_decode_skip, 0, "FM2",
    "FM2 Plume direct indexed-draw decode samples to count before logging");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_decode_record_limit, 4, "FM2",
    "Maximum direct-draw records to inspect per decoded Plume sample");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_buffer_bytes, 0, "FM2",
    "When nonzero, dump up to this many bytes from each decoded direct-draw D3D resource buffer");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_state_bytes, 0, "FM2",
    "When nonzero, dump up to this many bytes from each decoded direct-draw shader/state object");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_shader_bytes, 0, "FM2",
    "When nonzero, dump up to this many bytes from each decoded direct-draw shader payload");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_vs_float_constants, 0, "FM2",
    "When nonzero, summarize VS float constants and log up to this many nonzero float4 samples");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_direct_transform_candidates, 0, "FM2",
    "When nonzero, score direct-draw stream0 positions through candidate VS "
    "constant matrices using this many sampled vertices");

REXCVAR_DEFINE_STRING(
    fm2_plume_direct_replay_transform_source, "auto", "FM2",
    "VS constant transform source for Plume direct debug replay: auto, c28, "
    "c0, or c36_mul_c28")
    .allowed({"auto", "c28", "c0", "c36_mul_c28"});

REXCVAR_DEFINE_UINT32(
    fm2_plume_direct_replay_record_index,
    fm2nr::kDirectDrawReplayAnyRecordIndex, "FM2",
    "Direct-draw record index to submit for Plume debug replay; "
    "0xFFFFFFFF submits the first ready record");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_render_context_limit, 0, "FM2",
    "Maximum lower-level D3D render-context dirty-state samples per process");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_render_context_skip, 0, "FM2",
    "Lower-level D3D render-context dirty-state samples to count before logging");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_render_context_after_direct_limit, 0, "FM2",
    "Lower-level D3D render-context dirty-state samples to log after the first decoded direct draw");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_render_context_fetch_group_limit, 0, "FM2",
    "When nonzero, decode up to this many fetch groups from each traced D3D render-context state shadow");

REXCVAR_DEFINE_BOOL(
    fm2_plume_native_state_trace, false, "FM2",
    "Emit sampled FM2 Plume native-state snapshots at direct draw entry");

REXCVAR_DEFINE_UINT32(
    fm2_plume_native_state_trace_limit, 16, "FM2",
    "Maximum FM2 Plume native-state snapshot lines per process");

std::atomic<uint64_t> g_plume_direct_decode_samples{0};
std::atomic<uint64_t> g_plume_render_context_samples{0};
std::atomic<uint64_t> g_plume_native_state_trace_samples{0};
std::atomic<uint8_t> g_plume_render_context_after_direct_armed{0};
std::atomic<uint32_t> g_plume_render_context_after_direct_remaining{0};

constexpr uint32_t kRememberedDirectReplayPlanCount = 64u;
constexpr uint32_t kDirectDrawPositionStatsMaxVertices = 4096u;

struct RememberedDirectReplayPlan {
  bool valid = false;
  uint64_t sample_number = 0;
  uint32_t record_index = 0;
  fm2nr::DirectDrawDebugReplayPlan plan;
};

std::mutex g_plume_direct_replay_plan_mutex;
std::array<RememberedDirectReplayPlan, kRememberedDirectReplayPlanCount>
    g_plume_direct_replay_plans;
uint32_t g_plume_direct_replay_plan_count = 0;
uint32_t g_plume_direct_replay_plan_next = 0;

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

void MaybeLogPlumeNativeStateSnapshot(
    const fm2nr::NativeStateSnapshot& snapshot) {
  if (!REXCVAR_GET(fm2_plume_native_state_trace)) {
    return;
  }
  const uint64_t sample_ix =
      g_plume_native_state_trace_samples.fetch_add(1, std::memory_order_relaxed);
  if (sample_ix >= REXCVAR_GET(fm2_plume_native_state_trace_limit)) {
    return;
  }

  const auto& stream0 = snapshot.streams[0];
  const auto& stream1 = snapshot.streams[1];
  LogLine(
      "FM2_PLUME_NATIVE_STATE n=%llu valid=%u ctx=%08X seq=%llu "
      "vs_valid=%u vs=%08X ps_valid=%u ps=%08X "
      "s0_valid=%u s0_res=%08X s0_off=%u s0_stride=%u "
      "s1_valid=%u s1_res=%08X s1_off=%u s1_stride=%u "
      "ib_valid=%u ib=%08X surf_valid=%u surf=%08X "
      "direct_valid=%u direct_iface=%08X pass_valid=%u pass_flags=%08X",
      static_cast<unsigned long long>(sample_ix + 1u), snapshot.valid ? 1u : 0u,
      snapshot.render_context,
      static_cast<unsigned long long>(snapshot.sequence),
      snapshot.vertex_shader.valid ? 1u : 0u, snapshot.vertex_shader.shader,
      snapshot.pixel_shader.valid ? 1u : 0u, snapshot.pixel_shader.shader,
      stream0.valid ? 1u : 0u, stream0.resource, stream0.byte_offset,
      stream0.stride_bytes, stream1.valid ? 1u : 0u, stream1.resource,
      stream1.byte_offset, stream1.stride_bytes,
      snapshot.index_buffer.valid ? 1u : 0u, snapshot.index_buffer.resource,
      snapshot.bound_surface.valid ? 1u : 0u, snapshot.bound_surface.surface,
      snapshot.last_direct_draw.valid ? 1u : 0u,
      snapshot.last_direct_draw.draw_iface, snapshot.last_pass.valid ? 1u : 0u,
      snapshot.last_pass.pass_flags);
}

void LogDirectDrawVSFloatConstants(uint64_t sample_number, const char* site) {
  const uint32_t sample_limit =
      REXCVAR_GET(fm2_plume_trace_direct_vs_float_constants);
  if (sample_limit == 0) {
    return;
  }

  const uint32_t* constants = DirectDrawVSFloatConstantRegisters();
  if (!constants) {
    LogLine(
        "FM2_PLUME_DIRECT_VS_FLOAT_CONST_STATS n=%llu site=%s "
        "missing_register_file=1",
        static_cast<unsigned long long>(sample_number), site);
    return;
  }

  const fm2nr::DirectDrawFloat4ConstantStats stats =
      fm2nr::AnalyzeDirectDrawFloat4ConstantStats(constants, 256u, sample_limit);
  LogLine(
      "FM2_PLUME_DIRECT_VS_FLOAT_CONST_STATS n=%llu site=%s valid=%u "
      "constants=%u finite=%u nonzero=%u first_nonzero=%u last_nonzero=%u "
      "samples=%u",
      static_cast<unsigned long long>(sample_number), site, stats.valid ? 1u : 0u,
      stats.constant_count, stats.finite_constants, stats.nonzero_constants,
      stats.first_nonzero_constant, stats.last_nonzero_constant, stats.sample_count);

  for (uint32_t i = 0; i < stats.sample_count; ++i) {
    const fm2nr::DirectDrawFloat4ConstantSample& sample = stats.samples[i];
    const uint32_t* raw = constants + sample.constant_index * 4u;
    LogLine(
        "FM2_PLUME_DIRECT_VS_FLOAT_CONST n=%llu site=%s c=%u "
        "raw=(%08X,%08X,%08X,%08X) value=(%.8g,%.8g,%.8g,%.8g)",
        static_cast<unsigned long long>(sample_number), site, sample.constant_index,
        raw[0], raw[1], raw[2], raw[3], sample.values[0], sample.values[1],
        sample.values[2], sample.values[3]);
  }
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

void LogDirectDrawInterfaceSlots(uint8_t* base, uint64_t sample_number, uint32_t draw_iface) {
  if (!draw_iface || !GuestReadableRange(base, draw_iface, 4)) {
    LogLine("FM2_PLUME_DIRECT_IFACE n=%llu iface=%08X unreadable=1",
            static_cast<unsigned long long>(sample_number), draw_iface);
    return;
  }

  const uint32_t vtable = TryLoadU32(base, draw_iface);
  if (!vtable || !GuestReadableRange(base, vtable + 0x80u, 4)) {
    LogLine("FM2_PLUME_DIRECT_IFACE n=%llu iface=%08X vtable=%08X unreadable_slots=1",
            static_cast<unsigned long long>(sample_number), draw_iface, vtable);
    return;
  }

  LogLine(
      "FM2_PLUME_DIRECT_IFACE n=%llu iface=%08X vtable=%08X "
      "slot28=%08X slot30=%08X slot64=%08X slot74=%08X slot80=%08X",
      static_cast<unsigned long long>(sample_number), draw_iface, vtable,
      TryLoadU32(base, vtable + 0x28u), TryLoadU32(base, vtable + 0x30u),
      TryLoadU32(base, vtable + 0x64u), TryLoadU32(base, vtable + 0x74u),
      TryLoadU32(base, vtable + 0x80u));
}

void LogDirectDrawResourceDescriptor(uint8_t* base, uint64_t sample_number,
                                     uint32_t record_index, const char* role,
                                     uint32_t descriptor) {
  if (!descriptor ||
      !GuestReadableRange(base, descriptor, fm2nr::kDirectDrawResourceDescriptorSize)) {
    LogLine(
        "FM2_PLUME_DIRECT_RESOURCE n=%llu rec_i=%08X role=%s desc=%08X unreadable=1",
        static_cast<unsigned long long>(sample_number), record_index, role, descriptor);
    return;
  }

  LogLine(
      "FM2_PLUME_DIRECT_RESOURCE n=%llu rec_i=%08X role=%s desc=%08X "
      "w00=%08X w04=%08X w08=%08X w0c=%08X",
      static_cast<unsigned long long>(sample_number), record_index, role, descriptor,
      TryLoadU32(base, descriptor + 0x00u), TryLoadU32(base, descriptor + 0x04u),
      TryLoadU32(base, descriptor + 0x08u), TryLoadU32(base, descriptor + 0x0Cu));
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

void LogDirectDrawStateBytes(uint8_t* base, uint64_t sample_number, const char* role,
                             const char* kind, uint32_t address) {
  uint32_t byte_count = REXCVAR_GET(fm2_plume_trace_direct_state_bytes);
  if (byte_count == 0 || address == 0) {
    return;
  }
  if (byte_count > fm2nr::kDirectDrawStateByteDumpMax) {
    byte_count = fm2nr::kDirectDrawStateByteDumpMax;
  }

  const std::string bytes = SnapshotGuestBytes(base, address, byte_count);
  if (bytes.empty()) {
    LogLine(
        "FM2_PLUME_DIRECT_STATE_BYTES n=%llu role=%s kind=%s addr=%08X "
        "bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), role, kind, address, byte_count);
    return;
  }

  LogLine(
      "FM2_PLUME_DIRECT_STATE_BYTES n=%llu role=%s kind=%s addr=%08X bytes=%u data=%s",
      static_cast<unsigned long long>(sample_number), role, kind, address, byte_count,
      bytes.c_str());
}

void LogDirectDrawCompiledStateTable(uint8_t* base, uint64_t sample_number,
                                     const char* role, uint32_t table) {
  uint32_t byte_count = REXCVAR_GET(fm2_plume_trace_direct_state_bytes);
  if (byte_count == 0 || table == 0) {
    return;
  }
  if (byte_count > fm2nr::kDirectDrawStateByteDumpMax) {
    byte_count = fm2nr::kDirectDrawStateByteDumpMax;
  }
  if (byte_count < fm2nr::kDirectDrawCompiledStateHeaderSize ||
      !GuestReadableRange(base, table, byte_count)) {
    LogLine(
        "FM2_PLUME_DIRECT_COMPILED_STATE n=%llu role=%s table=%08X "
        "bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), role, table, byte_count);
    return;
  }

  uint8_t table_bytes[fm2nr::kDirectDrawStateByteDumpMax] = {};
  std::memcpy(table_bytes, REX_RAW_ADDR(table), byte_count);

  const std::string bytes = FormatByteDump(table_bytes, byte_count);
  LogLine(
      "FM2_PLUME_DIRECT_STATE_BYTES n=%llu role=%s kind=table addr=%08X bytes=%u data=%s",
      static_cast<unsigned long long>(sample_number), role, table, byte_count,
      bytes.c_str());

  const fm2nr::DirectDrawCompiledStateTableSummary summary =
      fm2nr::AnalyzeDirectDrawCompiledStateTable(table_bytes, byte_count);
  LogLine(
      "FM2_PLUME_DIRECT_COMPILED_STATE n=%llu role=%s table=%08X "
      "parser=4 bytes=%u valid=%u truncated=%u entries_truncated=%u "
      "h00=%08X h04=%08X h08=%08X h0c=%08X declared_payload_bytes=%08X "
      "entry_count=%u",
      static_cast<unsigned long long>(sample_number), role, table, byte_count,
      summary.valid ? 1u : 0u, summary.truncated ? 1u : 0u,
      summary.entries_truncated ? 1u : 0u, summary.header_w00,
      summary.header_w04, summary.header_w08, summary.header_w0c,
      summary.declared_payload_bytes, summary.entry_count);

  for (uint32_t i = 0; i < summary.entry_count; ++i) {
    const fm2nr::DirectDrawCompiledStateEntry& entry = summary.entries[i];
    const uint32_t payload_address = AddGuestOffsetOrZero(table, entry.payload_offset);
    uint64_t payload_hash = 0;
    const bool payload_hash_ok =
        !entry.truncated && payload_address != 0 &&
        HashGuestReadableRange(base, payload_address, entry.payload_bytes, payload_hash);
    const uint32_t sample_bytes =
        entry.payload_bytes > 64u ? 64u : entry.payload_bytes;
    const std::string payload_sample =
        payload_address != 0 && sample_bytes != 0
            ? SnapshotGuestBytes(base, payload_address, sample_bytes)
            : std::string();
    LogLine(
        "FM2_PLUME_DIRECT_COMPILED_STATE_ENTRY n=%llu role=%s table=%08X "
        "entry=%u section=%s table_off=%04X target_off=%04X dwords=%u "
        "payload_off=%04X payload_bytes=%u truncated=%u payload_hash_ok=%u "
        "payload_hash=%016llX sample_bytes=%u data=%s",
        static_cast<unsigned long long>(sample_number), role, table, i,
        fm2nr::DirectDrawCompiledStateSectionName(entry.section), entry.table_offset,
        entry.target_offset, entry.dword_count, entry.payload_offset,
        entry.payload_bytes, entry.truncated ? 1u : 0u,
        payload_hash_ok ? 1u : 0u, static_cast<unsigned long long>(payload_hash),
        sample_bytes, payload_sample.c_str());

    if (entry.section == fm2nr::DirectDrawCompiledStateSection::kMaskValue) {
      constexpr uint32_t kMaskValuePairLogMax = 64u;
      const uint32_t pair_count =
          fm2nr::DirectDrawCompiledStateMaskValuePairCount(entry);
      const uint32_t logged_pair_count =
          pair_count > kMaskValuePairLogMax ? kMaskValuePairLogMax : pair_count;
      for (uint32_t pair_index = 0; pair_index < logged_pair_count; ++pair_index) {
        const fm2nr::DirectDrawCompiledStateMaskValuePair pair =
            fm2nr::DirectDrawReadCompiledStateMaskValuePair(
                table_bytes, byte_count, entry, pair_index);
        if (!pair.valid) {
          break;
        }

        LogLine(
            "FM2_PLUME_DIRECT_COMPILED_STATE_MASK_PAIR n=%llu role=%s "
            "table=%08X entry=%u pair=%u payload_off=%04X state_off=%04X "
            "state_dw=%u fetch_reg=%04X fetch_group=%u fetch_dw=%u "
            "mask=%08X value=%08X",
            static_cast<unsigned long long>(sample_number), role, table, i,
            pair.pair_index, pair.payload_offset, pair.state_offset,
            pair.state_dword_index, pair.fetch_register, pair.fetch_group,
            pair.fetch_group_dword, pair.mask, pair.value);
      }
      if (pair_count > logged_pair_count) {
        LogLine(
            "FM2_PLUME_DIRECT_COMPILED_STATE_MASK_PAIR_TRUNCATED n=%llu "
            "role=%s table=%08X entry=%u pair_count=%u logged=%u",
            static_cast<unsigned long long>(sample_number), role, table, i,
            pair_count, logged_pair_count);
      }
    }
  }

  const uint32_t trailing_offset =
      fm2nr::DirectDrawCompiledStateTrailingEntryOffset(
          summary.declared_payload_bytes);
  if (trailing_offset != 0 && trailing_offset + sizeof(uint32_t) <= byte_count) {
    const uint32_t target_offset = fm2nr::DirectDrawLoadBE16(table_bytes, trailing_offset);
    const uint32_t dword_count =
        fm2nr::DirectDrawLoadBE16(table_bytes, trailing_offset + sizeof(uint16_t));
    if (dword_count != 0) {
      const uint32_t payload_offset = trailing_offset + sizeof(uint32_t);
      const uint32_t payload_bytes =
          fm2nr::SaturatingMulU32(dword_count, sizeof(uint32_t));
      const bool truncated = payload_offset + payload_bytes > byte_count;
      const uint32_t payload_address = AddGuestOffsetOrZero(table, payload_offset);
      uint64_t payload_hash = 0;
      const bool payload_hash_ok =
          !truncated && payload_address != 0 &&
          HashGuestReadableRange(base, payload_address, payload_bytes, payload_hash);
      const uint32_t sample_bytes = payload_bytes > 64u ? 64u : payload_bytes;
      const std::string payload_sample =
          payload_address != 0 && sample_bytes != 0
              ? SnapshotGuestBytes(base, payload_address, sample_bytes)
              : std::string();
      LogLine(
          "FM2_PLUME_DIRECT_COMPILED_STATE_TRAILING_ENTRY n=%llu role=%s "
          "table=%08X table_off=%04X target_off=%04X dwords=%u "
          "payload_off=%04X payload_bytes=%u truncated=%u payload_hash_ok=%u "
          "payload_hash=%016llX sample_bytes=%u data=%s",
          static_cast<unsigned long long>(sample_number), role, table,
          trailing_offset, target_offset, dword_count, payload_offset,
          payload_bytes, truncated ? 1u : 0u, payload_hash_ok ? 1u : 0u,
          static_cast<unsigned long long>(payload_hash), sample_bytes,
          payload_sample.c_str());
    }
  }
}

void LogDirectDrawVertexShaderAnalysis(
    uint64_t sample_number, const char* role, const char* payload_kind,
    uint32_t resolved, const char* source, const char* endian_variant,
    uint32_t ucode_offset, uint32_t ucode_base, const uint32_t* ucode_dwords,
    uint32_t ucode_dword_count) {
  const fm2nr::DirectDrawVertexShaderAnalysisSummary analysis =
      fm2nr::AnalyzeDirectDrawVertexShaderUcode(ucode_dwords, ucode_dword_count);
  LogLine(
      "FM2_PLUME_DIRECT_SHADER_VERTEX_FETCH_ANALYSIS n=%llu role=%s kind=%s "
      "object=%08X source=%s endian=%s ucode_off=%04X ucode_base=%08X "
      "valid=%u ucode_dwords=%u cf_pairs=%u bindings=%u attributes=%u "
      "truncated=%u vfetch_bitmap0=%08X vfetch_bitmap1=%08X "
      "vfetch_bitmap2=%08X",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      source, endian_variant, ucode_offset, ucode_base, analysis.valid ? 1u : 0u,
      analysis.ucode_dword_count, analysis.cf_pair_index_bound, analysis.binding_count,
      analysis.attribute_count, analysis.attributes_truncated ? 1u : 0u,
      analysis.vertex_fetch_bitmap[0], analysis.vertex_fetch_bitmap[1],
      analysis.vertex_fetch_bitmap[2]);

  const uint32_t logged_attribute_count =
      analysis.attribute_count > fm2nr::kDirectDrawShaderAnalysisMaxAttributes
          ? fm2nr::kDirectDrawShaderAnalysisMaxAttributes
          : analysis.attribute_count;
  for (uint32_t i = 0; i < logged_attribute_count; ++i) {
    const auto& attr = analysis.attributes[i];
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_VERTEX_FETCH_ATTR n=%llu role=%s kind=%s "
        "object=%08X source=%s endian=%s ucode_off=%04X ucode_base=%08X "
        "attr=%u binding=%u binding_attr=%u fetch_constant=%u "
        "stride_words=%u offset_words=%d format=%u src_reg=%u dst_reg=%u "
        "write_mask=%X mini=%u signed=%u integer=%u",
        static_cast<unsigned long long>(sample_number), role, payload_kind,
        resolved, source, endian_variant, ucode_offset, ucode_base, i,
        attr.binding_index, attr.attribute_index, attr.fetch_constant,
        attr.stride_words, attr.offset_words, attr.data_format,
        attr.source_register, attr.destination_register, attr.write_mask,
        attr.is_mini_fetch ? 1u : 0u, attr.is_signed ? 1u : 0u,
        attr.is_integer ? 1u : 0u);
  }
}

bool LogDirectDrawShaderUcodeBounds(uint8_t* base, uint64_t sample_number, const char* role,
                                    const char* payload_kind, uint32_t resolved,
                                    uint32_t shader_type, uint32_t ucode_offset,
                                    uint32_t ucode_base, uint32_t known_payload_bytes,
                                    uint32_t ucode_dump_count,
                                    uint32_t& structural_hash_bytes,
                                    uint64_t& structural_hash) {
  structural_hash_bytes = 0;
  structural_hash = 0;
  constexpr uint32_t kMaxUcodeDwords = fm2nr::kDirectDrawShaderByteDumpMax / sizeof(uint32_t);
  if (ucode_base == 0 || ucode_dump_count < sizeof(uint32_t)) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE_BOUNDS n=%llu role=%s kind=%s object=%08X "
        "type=%08X ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X "
        "scan_bytes=%u empty=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, ucode_offset, ucode_base, known_payload_bytes, ucode_dump_count);
    return false;
  }

  uint32_t ucode_dword_count = ucode_dump_count / sizeof(uint32_t);
  if (ucode_dword_count > kMaxUcodeDwords) {
    ucode_dword_count = kMaxUcodeDwords;
  }
  const uint32_t readable_bytes = ucode_dword_count * sizeof(uint32_t);
  if (!GuestReadableRange(base, ucode_base, readable_bytes)) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE_BOUNDS n=%llu role=%s kind=%s object=%08X "
        "type=%08X ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X "
        "scan_bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, ucode_offset, ucode_base, known_payload_bytes, readable_bytes);
    return false;
  }

  uint32_t ucode_dwords[kMaxUcodeDwords] = {};
  for (uint32_t i = 0; i < ucode_dword_count; ++i) {
    ucode_dwords[i] = TryLoadU32(base, ucode_base + i * sizeof(uint32_t));
  }

  if (shader_type == fm2nr::kDirectDrawVertexShaderTypeTag) {
    LogDirectDrawVertexShaderAnalysis(
        sample_number, role, payload_kind, resolved, "fixed", "loaded",
        ucode_offset, ucode_base, ucode_dwords, ucode_dword_count);

    uint32_t swapped_ucode_dwords[kMaxUcodeDwords] = {};
    for (uint32_t i = 0; i < ucode_dword_count; ++i) {
      swapped_ucode_dwords[i] =
          fm2nr::DirectDrawLittleEndianValueFromGuestDword(ucode_dwords[i]);
    }
    LogDirectDrawVertexShaderAnalysis(
        sample_number, role, payload_kind, resolved, "fixed", "byteswap",
        ucode_offset, ucode_base, swapped_ucode_dwords, ucode_dword_count);
  }

  const fm2nr::DirectDrawShaderUcodeBounds bounds =
      fm2nr::AnalyzeXenosUcodeBounds(ucode_dwords, ucode_dword_count);
  bool structural_hash_ok = false;
  if (bounds.valid && bounds.total_used_bytes != 0 &&
      bounds.total_used_bytes <= readable_bytes) {
    structural_hash_bytes = bounds.total_used_bytes;
    structural_hash_ok =
        HashGuestReadableRange(base, ucode_base, structural_hash_bytes, structural_hash);
    if (!structural_hash_ok) {
      structural_hash_bytes = 0;
      structural_hash = 0;
    }
  }
  LogLine(
      "FM2_PLUME_DIRECT_SHADER_UCODE_BOUNDS n=%llu role=%s kind=%s object=%08X "
      "type=%08X ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X "
      "valid=%u saw_end=%u truncated=%u scanned_bytes=%u scanned_dwords=%u "
      "cf_pairs_avail=%u cf_pairs=%u cf_bytes=%u first_exec_cf=%u "
      "first_exec_op=%X first_exec_addr=%u execs=%u exec_high_water=%u "
      "exec_bytes=%u total_bytes=%u structural_hash_bytes=%u "
      "structural_hash=%016llX",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, ucode_offset, ucode_base, known_payload_bytes, bounds.valid ? 1u : 0u,
      bounds.saw_exec_end ? 1u : 0u, bounds.truncated ? 1u : 0u,
      bounds.scanned_bytes, bounds.scanned_dwords, bounds.cf_pair_count_available,
      bounds.cf_pair_index_bound, bounds.cf_byte_count, bounds.first_exec_cf_index,
      bounds.first_exec_opcode, bounds.first_exec_address, bounds.exec_instruction_count,
      bounds.exec_high_water_instruction, bounds.exec_high_water_bytes,
      bounds.total_used_bytes, structural_hash_bytes,
      static_cast<unsigned long long>(structural_hash));
  return structural_hash_ok;
}

bool LogDirectDrawShaderUcodeCandidate(uint8_t* base, uint64_t sample_number, const char* role,
                                       const char* payload_kind, uint32_t resolved,
                                       uint32_t shader_type, uint32_t payload_base,
                                       uint32_t known_payload_bytes,
                                       uint32_t current_ucode_offset,
                                       uint32_t payload_scan_bytes,
                                       uint32_t& structural_hash_bytes,
                                       uint64_t& structural_hash) {
  structural_hash_bytes = 0;
  structural_hash = 0;
  constexpr uint32_t kMaxPayloadDwords =
      fm2nr::kDirectDrawShaderByteDumpMax / sizeof(uint32_t);
  uint32_t payload_dword_count = payload_scan_bytes / sizeof(uint32_t);
  if (payload_dword_count > kMaxPayloadDwords) {
    payload_dword_count = kMaxPayloadDwords;
  }
  const uint32_t readable_bytes = payload_dword_count * sizeof(uint32_t);
  if (payload_base == 0 || readable_bytes < fm2nr::kXenosUcodeControlFlowPairDwordCount *
                                               sizeof(uint32_t)) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE_CANDIDATE n=%llu role=%s kind=%s "
        "object=%08X type=%08X payload_base=%08X known_payload_bytes=%08X "
        "scan_bytes=%u current_ucode_off=%04X valid=0 empty=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_base, known_payload_bytes, readable_bytes,
        current_ucode_offset);
    return false;
  }
  if (!GuestReadableRange(base, payload_base, readable_bytes)) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE_CANDIDATE n=%llu role=%s kind=%s "
        "object=%08X type=%08X payload_base=%08X known_payload_bytes=%08X "
        "scan_bytes=%u current_ucode_off=%04X valid=0 unreadable=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_base, known_payload_bytes, readable_bytes,
        current_ucode_offset);
    return false;
  }

  uint32_t payload_dwords[kMaxPayloadDwords] = {};
  for (uint32_t i = 0; i < payload_dword_count; ++i) {
    payload_dwords[i] = TryLoadU32(base, payload_base + i * sizeof(uint32_t));
  }

  constexpr uint32_t kMaxLoggedCandidates = 16u;
  fm2nr::DirectDrawShaderUcodeCandidate candidates[kMaxLoggedCandidates] = {};
  const uint32_t candidate_count = fm2nr::FindTopXenosUcodeCandidates(
      payload_dwords, payload_dword_count, candidates, kMaxLoggedCandidates);
  fm2nr::DirectDrawShaderUcodeCandidate selected_candidate;
  uint32_t selected_rank = 0;
  bool selected_by_fetch = false;
  if (candidate_count != 0) {
    selected_candidate = candidates[0];
  }

  for (uint32_t rank = 0; rank < candidate_count; ++rank) {
    const fm2nr::DirectDrawShaderUcodeCandidate& candidate = candidates[rank];
    const uint32_t candidate_base = AddGuestOffsetOrZero(payload_base, candidate.byte_offset);
    const fm2nr::DirectDrawShaderUcodeBounds& bounds = candidate.bounds;
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE_CANDIDATE_TOP n=%llu role=%s kind=%s "
        "object=%08X type=%08X payload_base=%08X rank=%u candidate_off=%04X "
        "candidate_base=%08X saw_end=%u truncated=%u scanned_bytes=%u "
        "scanned_dwords=%u cf_pairs_avail=%u cf_pairs=%u cf_bytes=%u "
        "first_exec_cf=%u first_exec_op=%X first_exec_addr=%u execs=%u "
        "exec_high_water=%u exec_bytes=%u total_bytes=%u",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_base, rank, candidate.byte_offset, candidate_base,
        bounds.saw_exec_end ? 1u : 0u, bounds.truncated ? 1u : 0u,
        bounds.scanned_bytes, bounds.scanned_dwords, bounds.cf_pair_count_available,
        bounds.cf_pair_index_bound, bounds.cf_byte_count, bounds.first_exec_cf_index,
        bounds.first_exec_opcode, bounds.first_exec_address, bounds.exec_instruction_count,
        bounds.exec_high_water_instruction, bounds.exec_high_water_bytes,
        bounds.total_used_bytes);

    if (shader_type != fm2nr::kDirectDrawVertexShaderTypeTag ||
        candidate.bounds.total_used_bytes == 0) {
      continue;
    }

    const uint32_t candidate_ucode_dwords =
        candidate.bounds.total_used_bytes / sizeof(uint32_t);
    const fm2nr::DirectDrawVertexShaderAnalysisSummary analysis =
        fm2nr::AnalyzeDirectDrawVertexShaderUcode(
            payload_dwords + candidate.dword_offset, candidate_ucode_dwords);
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_VERTEX_FETCH_SUMMARY n=%llu role=%s kind=%s "
        "object=%08X type=%08X payload_base=%08X rank=%u candidate_off=%04X "
        "candidate_base=%08X valid=%u ucode_dwords=%u cf_pairs=%u "
        "bindings=%u attributes=%u truncated=%u vfetch_bitmap0=%08X "
        "vfetch_bitmap1=%08X vfetch_bitmap2=%08X",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_base, rank, candidate.byte_offset, candidate_base,
        analysis.valid ? 1u : 0u, analysis.ucode_dword_count,
        analysis.cf_pair_index_bound, analysis.binding_count, analysis.attribute_count,
        analysis.attributes_truncated ? 1u : 0u, analysis.vertex_fetch_bitmap[0],
        analysis.vertex_fetch_bitmap[1], analysis.vertex_fetch_bitmap[2]);
    const uint32_t logged_attribute_count =
        analysis.attribute_count > fm2nr::kDirectDrawShaderAnalysisMaxAttributes
            ? fm2nr::kDirectDrawShaderAnalysisMaxAttributes
            : analysis.attribute_count;
    for (uint32_t i = 0; i < logged_attribute_count; ++i) {
      const auto& attr = analysis.attributes[i];
      LogLine(
          "FM2_PLUME_DIRECT_SHADER_VERTEX_FETCH n=%llu role=%s kind=%s "
          "object=%08X type=%08X payload_base=%08X rank=%u candidate_off=%04X "
          "candidate_base=%08X attr=%u binding=%u binding_attr=%u "
          "fetch_constant=%u stride_words=%u offset_words=%d format=%u "
          "src_reg=%u dst_reg=%u write_mask=%X mini=%u signed=%u integer=%u",
          static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
          shader_type, payload_base, rank, candidate.byte_offset, candidate_base, i,
          attr.binding_index, attr.attribute_index, attr.fetch_constant,
          attr.stride_words, attr.offset_words, attr.data_format, attr.source_register,
          attr.destination_register, attr.write_mask, attr.is_mini_fetch ? 1u : 0u,
          attr.is_signed ? 1u : 0u, attr.is_integer ? 1u : 0u);
    }

    if (!selected_by_fetch && analysis.valid && analysis.attribute_count != 0) {
      selected_candidate = candidate;
      selected_rank = rank;
      selected_by_fetch = true;
    }
  }

  const fm2nr::DirectDrawShaderUcodeCandidate& candidate = selected_candidate;
  uint32_t candidate_base = 0;
  bool structural_hash_ok = false;
  if (candidate.valid) {
    candidate_base = AddGuestOffsetOrZero(payload_base, candidate.byte_offset);
    if (candidate_base != 0 && candidate.bounds.total_used_bytes != 0 &&
        candidate.byte_offset + candidate.bounds.total_used_bytes <= readable_bytes) {
      structural_hash_bytes = candidate.bounds.total_used_bytes;
      structural_hash_ok =
          HashGuestReadableRange(base, candidate_base, structural_hash_bytes,
                                 structural_hash);
      if (!structural_hash_ok) {
        structural_hash_bytes = 0;
        structural_hash = 0;
      }
    }
  }

  const fm2nr::DirectDrawShaderUcodeBounds& bounds = candidate.bounds;
  LogLine(
      "FM2_PLUME_DIRECT_SHADER_UCODE_CANDIDATE n=%llu role=%s kind=%s "
      "object=%08X type=%08X payload_base=%08X known_payload_bytes=%08X "
      "scan_bytes=%u current_ucode_off=%04X valid=%u candidate_count=%u "
      "selected_rank=%u selected_by_fetch=%u candidate_off=%04X "
      "candidate_base=%08X saw_end=%u truncated=%u scanned_bytes=%u "
      "scanned_dwords=%u cf_pairs_avail=%u cf_pairs=%u cf_bytes=%u "
      "first_exec_cf=%u first_exec_op=%X first_exec_addr=%u execs=%u "
      "exec_high_water=%u exec_bytes=%u total_bytes=%u "
      "structural_hash_bytes=%u structural_hash=%016llX",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, payload_base, known_payload_bytes, readable_bytes, current_ucode_offset,
      candidate.valid ? 1u : 0u, candidate_count, selected_rank,
      selected_by_fetch ? 1u : 0u, candidate.byte_offset, candidate_base,
      bounds.saw_exec_end ? 1u : 0u, bounds.truncated ? 1u : 0u,
      bounds.scanned_bytes, bounds.scanned_dwords, bounds.cf_pair_count_available,
      bounds.cf_pair_index_bound, bounds.cf_byte_count, bounds.first_exec_cf_index,
      bounds.first_exec_opcode, bounds.first_exec_address, bounds.exec_instruction_count,
      bounds.exec_high_water_instruction, bounds.exec_high_water_bytes,
      bounds.total_used_bytes, structural_hash_bytes,
      static_cast<unsigned long long>(structural_hash));
  return structural_hash_ok;
}

fm2nr::DirectDrawShaderKeySummary LogDirectDrawShaderPayload(
    uint8_t* base, uint64_t sample_number, const char* role, uint32_t resolved,
    uint32_t state_table_shader_payload_bytes) {
  uint32_t byte_count = REXCVAR_GET(fm2_plume_trace_direct_shader_bytes);
  if (byte_count == 0 || resolved == 0) {
    return {};
  }
  if (byte_count > fm2nr::kDirectDrawShaderByteDumpMax) {
    byte_count = fm2nr::kDirectDrawShaderByteDumpMax;
  }

  const uint32_t shader_type = TryLoadU32(base, resolved);
  const uint32_t payload_offset =
      fm2nr::DirectDrawShaderPayloadGpuBaseOffsetForType(shader_type);
  const uint32_t ucode_offset =
      fm2nr::DirectDrawShaderPayloadUcodeOffsetForType(shader_type);
  const char* payload_kind = "unknown";
  if (shader_type == fm2nr::kDirectDrawVertexShaderTypeTag) {
    payload_kind = "vertex_payload";
  } else if (shader_type == fm2nr::kDirectDrawPixelShaderTypeTag) {
    payload_kind = "pixel_payload";
  } else {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER n=%llu role=%s object=%08X type=%08X "
        "unsupported_type=1",
        static_cast<unsigned long long>(sample_number), role, resolved, shader_type);
    return {};
  }

  const uint32_t gpu_base = TryLoadU32AtOffset(base, resolved, payload_offset);
  const uint32_t size_field = TryLoadU32AtOffset(base, resolved, payload_offset + 4u);
  uint32_t known_payload_bytes = size_field;
  if (shader_type == fm2nr::kDirectDrawVertexShaderTypeTag &&
      state_table_shader_payload_bytes != 0) {
    known_payload_bytes = state_table_shader_payload_bytes;
  }
  if (shader_type == fm2nr::kDirectDrawPixelShaderTypeTag) {
    const uint32_t pixel_payload_bytes = TryLoadU32AtOffset(
        base, resolved, fm2nr::kDirectDrawPixelShaderPayloadByteCountOffset);
    if (pixel_payload_bytes != 0) {
      known_payload_bytes = pixel_payload_bytes;
    }
  }
  const uint32_t ucode_base =
      gpu_base != 0 ? AddGuestOffsetOrZero(gpu_base, ucode_offset) : 0;
  const uint32_t w30 = TryLoadU32AtOffset(base, resolved, 0x30u);
  const uint32_t w30_le = shader_type == fm2nr::kDirectDrawVertexShaderTypeTag
                              ? fm2nr::DirectDrawLittleEndianValueFromGuestDword(w30)
                              : 0u;

  LogLine(
      "FM2_PLUME_DIRECT_SHADER_META n=%llu role=%s kind=%s object=%08X type=%08X "
      "payload_off=%04X gpu_base=%08X size_field=%08X known_payload_bytes=%08X "
      "ucode_off=%04X ucode_base=%08X w30_le=%08X "
      "w18=%08X w1c=%08X w20=%08X w24=%08X w28=%08X w2c=%08X "
      "w30=%08X w34=%08X w38=%08X w3c=%08X",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, payload_offset, gpu_base, size_field, known_payload_bytes, ucode_offset,
      ucode_base, w30_le, TryLoadU32AtOffset(base, resolved, 0x18u),
      TryLoadU32AtOffset(base, resolved, 0x1Cu),
      TryLoadU32AtOffset(base, resolved, 0x20u),
      TryLoadU32AtOffset(base, resolved, 0x24u),
      TryLoadU32AtOffset(base, resolved, 0x28u),
      TryLoadU32AtOffset(base, resolved, 0x2Cu),
      w30, TryLoadU32AtOffset(base, resolved, 0x34u),
      TryLoadU32AtOffset(base, resolved, 0x38u),
      TryLoadU32AtOffset(base, resolved, 0x3Cu));

  if (gpu_base == 0) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER n=%llu role=%s kind=%s object=%08X type=%08X "
        "payload_off=%04X gpu_base=00000000 size_field=%08X known_payload_bytes=%08X "
        "empty=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_offset, size_field, known_payload_bytes);
    return {};
  }

  const uint32_t dump_count =
      fm2nr::BoundedShaderPayloadDumpByteCount(byte_count, known_payload_bytes);

  const std::string bytes = SnapshotGuestBytes(base, gpu_base, dump_count);
  if (bytes.empty()) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER n=%llu role=%s kind=%s object=%08X type=%08X "
        "payload_off=%04X gpu_base=%08X size_field=%08X known_payload_bytes=%08X "
        "bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, payload_offset, gpu_base, size_field, known_payload_bytes, dump_count);
    return {};
  }

  LogLine(
      "FM2_PLUME_DIRECT_SHADER n=%llu role=%s kind=%s object=%08X type=%08X "
      "payload_off=%04X gpu_base=%08X size_field=%08X known_payload_bytes=%08X "
      "bytes=%u data=%s",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, payload_offset, gpu_base, size_field, known_payload_bytes, dump_count,
      bytes.c_str());

  const uint32_t payload_hash_bytes =
      known_payload_bytes != 0 ? known_payload_bytes : dump_count;
  uint64_t payload_hash = 0;
  const bool payload_hash_ok =
      HashGuestReadableRange(base, gpu_base, payload_hash_bytes, payload_hash);
  LogLine(
      "FM2_PLUME_DIRECT_SHADER_HASH n=%llu role=%s kind=%s object=%08X "
      "type=%08X payload_base=%08X payload_bytes=%u payload_hash_ok=%u "
      "payload_hash=%016llX",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, gpu_base, payload_hash_bytes, payload_hash_ok ? 1u : 0u,
      static_cast<unsigned long long>(payload_hash));

  const uint32_t ucode_dump_count =
      fm2nr::BoundedShaderUcodeDumpByteCount(byte_count, known_payload_bytes, ucode_offset);
  uint32_t candidate_structural_hash_bytes = 0;
  uint64_t candidate_structural_hash = 0;
  const bool candidate_structural_hash_ok = LogDirectDrawShaderUcodeCandidate(
      base, sample_number, role, payload_kind, resolved, shader_type, gpu_base,
      known_payload_bytes, ucode_offset, dump_count, candidate_structural_hash_bytes,
      candidate_structural_hash);
  uint32_t structural_hash_bytes = 0;
  uint64_t structural_hash = 0;
  bool structural_hash_ok = LogDirectDrawShaderUcodeBounds(
      base, sample_number, role, payload_kind, resolved, shader_type, ucode_offset,
      ucode_base, known_payload_bytes, ucode_dump_count, structural_hash_bytes,
      structural_hash);
  if (!structural_hash_ok && candidate_structural_hash_ok) {
    structural_hash_bytes = candidate_structural_hash_bytes;
    structural_hash = candidate_structural_hash;
    structural_hash_ok = true;
  }
  if (ucode_base == 0 || ucode_dump_count == 0) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE n=%llu role=%s kind=%s object=%08X type=%08X "
        "ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X bytes=%u empty=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, ucode_offset, ucode_base, known_payload_bytes, ucode_dump_count);
    return fm2nr::BuildDirectDrawShaderKeySummary(
        gpu_base, payload_hash_bytes, payload_hash_ok, payload_hash,
        structural_hash_bytes, structural_hash_ok, structural_hash);
  }

  const std::string ucode_bytes = SnapshotGuestBytes(base, ucode_base, ucode_dump_count);
  if (ucode_bytes.empty()) {
    LogLine(
        "FM2_PLUME_DIRECT_SHADER_UCODE n=%llu role=%s kind=%s object=%08X type=%08X "
        "ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
        shader_type, ucode_offset, ucode_base, known_payload_bytes, ucode_dump_count);
    return fm2nr::BuildDirectDrawShaderKeySummary(
        gpu_base, payload_hash_bytes, payload_hash_ok, payload_hash,
        structural_hash_bytes, structural_hash_ok, structural_hash);
  }

  LogLine(
      "FM2_PLUME_DIRECT_SHADER_UCODE n=%llu role=%s kind=%s object=%08X type=%08X "
      "ucode_off=%04X ucode_base=%08X known_payload_bytes=%08X bytes=%u data=%s",
      static_cast<unsigned long long>(sample_number), role, payload_kind, resolved,
      shader_type, ucode_offset, ucode_base, known_payload_bytes, ucode_dump_count,
      ucode_bytes.c_str());
  return fm2nr::BuildDirectDrawShaderKeySummary(
      gpu_base, payload_hash_bytes, payload_hash_ok, payload_hash,
      structural_hash_bytes, structural_hash_ok, structural_hash);
}

fm2nr::DirectDrawShaderKeySummary LogDirectDrawStateObject(
    uint8_t* base, uint64_t sample_number, uint32_t direct_render_ctx, const char* role,
    uint32_t ctx_handle_offset, uint32_t table_base_offset,
    uint32_t table_offset_field) {
  const uint32_t ctx_slot = AddGuestOffsetOrZero(direct_render_ctx, ctx_handle_offset);
  const uint32_t handle = TryLoadU32(base, ctx_slot);
  const uint32_t handle_vtable = TryLoadU32(base, handle);
  const uint32_t resolved = TryLoadU32AtOffset(
      base, handle, fm2nr::kDirectDrawStateHandleResolvedObjectOffset);
  const uint32_t resolved_vtable = TryLoadU32(base, resolved);
  const uint32_t table_rel = TryLoadU32AtOffset(base, resolved, table_offset_field);

  uint32_t table = 0;
  if (table_rel != 0) {
    const uint32_t table_base = AddGuestOffsetOrZero(resolved, table_base_offset);
    table = AddGuestOffsetOrZero(table_base, table_rel);
  }

  const uint32_t table_w00 = TryLoadU32AtOffset(base, table, 0x00u);
  const uint32_t table_w04 = TryLoadU32AtOffset(base, table, 0x04u);
  const uint32_t table_w08 = TryLoadU32AtOffset(base, table, 0x08u);
  const uint32_t table_w0c = TryLoadU32AtOffset(base, table, 0x0Cu);
  const uint32_t table_payload_bytes =
      TryLoadU32AtOffset(base, table, fm2nr::kDirectDrawCompiledStateHeaderSize - 4u);
  const uint32_t shader_payload_bytes = TryLoadU32AtOffset(
      base, table, fm2nr::kDirectDrawVertexShaderTablePayloadByteCountOffset);

  LogLine(
      "FM2_PLUME_DIRECT_STATE n=%llu role=%s ctx_off=%04X ctx_slot=%08X handle=%08X "
      "handle_vt=%08X resolved=%08X resolved_vt=%08X table_rel=%08X table=%08X "
      "t00=%08X t04=%08X t08=%08X t0c=%08X payload_bytes=%08X "
      "shader_payload_bytes=%08X",
      static_cast<unsigned long long>(sample_number), role, ctx_handle_offset, ctx_slot,
      handle, handle_vtable, resolved, resolved_vtable, table_rel, table, table_w00,
      table_w04, table_w08, table_w0c, table_payload_bytes, shader_payload_bytes);

  LogDirectDrawCompiledStateTable(base, sample_number, role, table);
  LogDirectDrawStateBytes(base, sample_number, role, "handle", handle);
  LogDirectDrawStateBytes(base, sample_number, role, "resolved", resolved);
  return LogDirectDrawShaderPayload(base, sample_number, role, resolved, shader_payload_bytes);
}

void LogDirectDrawStream0PositionStats(
    uint8_t* base, uint64_t sample_number, uint32_t record_index,
    uint32_t resource, const fm2nr::DirectDrawBufferViewSummary& view) {
  if (view.upload_guest_base == 0 || view.hash_bytes == 0) {
    return;
  }

  if (!GuestReadableRange(base, view.upload_guest_base, view.hash_bytes)) {
    LogLine(
        "FM2_PLUME_DIRECT_STREAM0_POS_STATS n=%llu rec_i=%08X "
        "resource=%08X gpu_base=%08X upload_base=%08X bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), record_index, resource,
        view.gpu_base, view.upload_guest_base, view.hash_bytes);
    return;
  }

  const uint8_t* upload_bytes =
      reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(view.upload_guest_base));
  const fm2nr::DirectDrawReplayFloat3PositionStats stats =
      fm2nr::AnalyzeDirectDrawReplayFloat3PositionStats(
          upload_bytes, view.hash_bytes, view.element_bytes, 0u,
          kDirectDrawPositionStatsMaxVertices);
  LogLine(
      "FM2_PLUME_DIRECT_STREAM0_POS_STATS n=%llu rec_i=%08X "
      "resource=%08X gpu_base=%08X upload_base=%08X bytes=%u "
      "valid=%u stride=%u pos_off=%u vertices=%u sampled=%u finite=%u "
      "has_bounds=%u min=(%.6f,%.6f,%.6f) max=(%.6f,%.6f,%.6f)",
      static_cast<unsigned long long>(sample_number), record_index, resource,
      view.gpu_base, view.upload_guest_base, view.hash_bytes,
      stats.valid ? 1u : 0u, stats.stride, stats.position_offset,
      stats.vertex_count, stats.sampled_vertices, stats.finite_vertices,
      stats.has_finite_bounds ? 1u : 0u, stats.min_x, stats.min_y,
      stats.min_z, stats.max_x, stats.max_y, stats.max_z);
}

const char* DirectDrawTransformInterpretationName(
    fm2nr::DirectDrawReplayTransformInterpretation interpretation) {
  switch (interpretation) {
    case fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip:
      return "row";
    case fm2nr::DirectDrawReplayTransformInterpretation::kColumnMajorClip:
      return "column";
  }
  return "unknown";
}

void LogDirectDrawTransformCandidateStats(
    const uint8_t* upload_bytes, uint32_t byte_count, uint64_t sample_number,
    uint32_t record_index, const char* name, uint32_t first_constant,
    const fm2nr::DirectDrawDebugReplayTransform& transform,
    fm2nr::DirectDrawReplayTransformInterpretation interpretation,
    uint32_t stride, uint32_t max_sample_vertices) {
  const fm2nr::DirectDrawReplayClipPositionStats stats =
      fm2nr::AnalyzeDirectDrawReplayClipPositionStats(
          upload_bytes, byte_count, stride, 0u, transform, interpretation,
          max_sample_vertices);
  LogLine(
      "FM2_PLUME_DIRECT_TRANSFORM_CANDIDATE n=%llu rec_i=%08X "
      "name=%s first=%u interp=%s valid=%u transform_valid=%u "
      "stride=%u vertices=%u sampled=%u finite=%u clip_finite=%u "
      "projectable=%u xy_inside=%u d3d_inside=%u gl_inside=%u "
      "has_clip=%u clip_min=(%.6f,%.6f,%.6f,%.6f) "
      "clip_max=(%.6f,%.6f,%.6f,%.6f) has_ndc=%u "
      "ndc_min=(%.6f,%.6f,%.6f) ndc_max=(%.6f,%.6f,%.6f)",
      static_cast<unsigned long long>(sample_number), record_index, name,
      first_constant, DirectDrawTransformInterpretationName(interpretation),
      stats.valid ? 1u : 0u, stats.transform_valid ? 1u : 0u, stats.stride,
      stats.vertex_count, stats.sampled_vertices, stats.finite_vertices,
      stats.clip_finite_vertices, stats.projectable_vertices,
      stats.xy_inside_vertices, stats.d3d_xyz_inside_vertices,
      stats.gl_xyz_inside_vertices, stats.has_clip_bounds ? 1u : 0u,
      stats.min_clip_x, stats.min_clip_y, stats.min_clip_z, stats.min_w,
      stats.max_clip_x, stats.max_clip_y, stats.max_clip_z, stats.max_w,
      stats.has_ndc_bounds ? 1u : 0u, stats.min_ndc_x, stats.min_ndc_y,
      stats.min_ndc_z, stats.max_ndc_x, stats.max_ndc_y, stats.max_ndc_z);
}

void LogDirectDrawTransformCandidates(
    uint8_t* base, uint64_t sample_number, uint32_t record_index,
    const fm2nr::DirectDrawBufferViewSummary& stream0_view,
    const uint32_t* constants) {
  const uint32_t max_sample_vertices =
      REXCVAR_GET(fm2_plume_trace_direct_transform_candidates);
  if (max_sample_vertices == 0 || !constants ||
      stream0_view.upload_guest_base == 0 || stream0_view.hash_bytes == 0 ||
      stream0_view.element_bytes == 0) {
    return;
  }
  if (!GuestReadableRange(base, stream0_view.upload_guest_base,
                          stream0_view.hash_bytes)) {
    LogLine(
        "FM2_PLUME_DIRECT_TRANSFORM_CANDIDATES n=%llu rec_i=%08X "
        "upload_base=%08X bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), record_index,
        stream0_view.upload_guest_base, stream0_view.hash_bytes);
    return;
  }

  const uint8_t* upload_bytes = reinterpret_cast<const uint8_t*>(
      REX_RAW_ADDR(stream0_view.upload_guest_base));
  struct CandidateRange {
    const char* name;
    uint32_t first_constant;
  };
  constexpr CandidateRange kCandidateRanges[] = {
      {"c0", 0u},   {"c23", 23u}, {"c28", 28u}, {"c36", 36u},
      {"c60", 60u}, {"c68", 68u}, {"c76", 76u}, {"c252", 252u},
  };

  std::array<fm2nr::DirectDrawDebugReplayTransform,
             std::size(kCandidateRanges)>
      transforms = {};
  for (uint32_t i = 0; i < std::size(kCandidateRanges); ++i) {
    transforms[i] = fm2nr::BuildDirectDrawDebugReplayTransformFromConstants(
        constants, 256u, kCandidateRanges[i].first_constant);
    LogDirectDrawTransformCandidateStats(
        upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
        kCandidateRanges[i].name, kCandidateRanges[i].first_constant,
        transforms[i],
        fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip,
        stream0_view.element_bytes, max_sample_vertices);
    LogDirectDrawTransformCandidateStats(
        upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
        kCandidateRanges[i].name, kCandidateRanges[i].first_constant,
        transforms[i],
        fm2nr::DirectDrawReplayTransformInterpretation::kColumnMajorClip,
        stream0_view.element_bytes, max_sample_vertices);
  }

  const auto c0_c28 =
      fm2nr::MultiplyDirectDrawReplayRowMajorTransforms(transforms[0],
                                                        transforms[2], 0u);
  const auto c36_c28 =
      fm2nr::MultiplyDirectDrawReplayRowMajorTransforms(transforms[3],
                                                        transforms[2], 36u);
  const auto c36_c60 =
      fm2nr::MultiplyDirectDrawReplayRowMajorTransforms(transforms[3],
                                                        transforms[4], 36u);
  const auto c36_c76 =
      fm2nr::MultiplyDirectDrawReplayRowMajorTransforms(transforms[3],
                                                        transforms[6], 36u);
  LogDirectDrawTransformCandidateStats(
      upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
      "c0_mul_c28", 0u, c0_c28,
      fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip,
      stream0_view.element_bytes, max_sample_vertices);
  LogDirectDrawTransformCandidateStats(
      upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
      "c36_mul_c28", 36u, c36_c28,
      fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip,
      stream0_view.element_bytes, max_sample_vertices);
  LogDirectDrawTransformCandidateStats(
      upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
      "c36_mul_c60", 36u, c36_c60,
      fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip,
      stream0_view.element_bytes, max_sample_vertices);
  LogDirectDrawTransformCandidateStats(
      upload_bytes, stream0_view.hash_bytes, sample_number, record_index,
      "c36_mul_c76", 36u, c36_c76,
      fm2nr::DirectDrawReplayTransformInterpretation::kRowMajorClip,
      stream0_view.element_bytes, max_sample_vertices);
}

fm2nr::DirectDrawDebugReplayTransform BuildDirectReplayTransformForSource(
    const uint32_t* constants, const std::string& source) {
  return fm2nr::BuildDirectDrawDebugReplayTransformFromSource(constants, 256u,
                                                              source);
}

fm2nr::DirectDrawBufferViewSummary LogDirectDrawD3DResource(
    uint8_t* base, uint64_t sample_number, uint32_t record_index, const char* role,
    uint32_t resource, uint32_t descriptor_w04, uint32_t descriptor_w08) {
  if (!resource || !GuestReadableRange(base, resource, fm2nr::kD3DResourceDecodeSize)) {
    LogLine(
        "FM2_PLUME_DIRECT_D3DRESOURCE n=%llu rec_i=%08X role=%s resource=%08X "
        "unreadable=1 desc_w04=%08X desc_w08=%08X",
        static_cast<unsigned long long>(sample_number), record_index, role, resource,
        descriptor_w04, descriptor_w08);
    return {};
  }

  const uint32_t gpu_base =
      TryLoadU32(base, resource + fm2nr::kD3DResourceGpuBaseOffset);
  const uint32_t byte_size =
      TryLoadU32(base, resource + fm2nr::kD3DResourceSizeOffset);
  const bool index_buffer = std::strcmp(role, "index") == 0;
  fm2nr::DirectDrawBufferViewSummary view = fm2nr::BuildDirectDrawBufferViewSummary(
      gpu_base, byte_size, descriptor_w04, descriptor_w08, index_buffer, false, 0);
  uint64_t buffer_hash = 0;
  const bool buffer_hash_ok =
      HashGuestReadableRange(base, view.upload_guest_base, view.hash_bytes,
                             buffer_hash);
  view.hash_ok = buffer_hash_ok;
  view.hash = buffer_hash;
  LogLine(
      "FM2_PLUME_DIRECT_D3DRESOURCE n=%llu rec_i=%08X role=%s resource=%08X "
      "r00=%08X r04=%08X r08=%08X r0c=%08X r10=%08X r14=%08X "
      "gpu_base=%08X upload_base=%08X byte_size=%08X readable_bytes=%08X desc_w04=%08X "
      "desc_w08=%08X view_bytes=%08X hash_bytes=%08X hash_ok=%u hash=%016llX",
      static_cast<unsigned long long>(sample_number), record_index, role, resource,
      TryLoadU32(base, resource + 0x00u), TryLoadU32(base, resource + 0x04u),
      TryLoadU32(base, resource + 0x08u), TryLoadU32(base, resource + 0x0Cu),
      TryLoadU32(base, resource + 0x10u), TryLoadU32(base, resource + 0x14u), gpu_base,
      view.upload_guest_base, byte_size, view.readable_bytes, descriptor_w04,
      descriptor_w08, view.view_bytes, view.hash_bytes, buffer_hash_ok ? 1u : 0u,
      static_cast<unsigned long long>(buffer_hash));

  if (std::strcmp(role, "stream0") == 0) {
    LogDirectDrawStream0PositionStats(base, sample_number, record_index, resource,
                                      view);
  }

  uint32_t byte_count = REXCVAR_GET(fm2_plume_trace_direct_buffer_bytes);
  if (byte_count == 0 || view.upload_guest_base == 0 || view.hash_bytes == 0) {
    return view;
  }
  if (byte_count > 64u) {
    byte_count = 64u;
  }
  if (byte_count > view.hash_bytes) {
    byte_count = view.hash_bytes;
  }

  const std::string bytes =
      SnapshotGuestBytes(base, view.upload_guest_base, byte_count);
  if (bytes.empty()) {
    LogLine(
        "FM2_PLUME_DIRECT_BUFFER n=%llu rec_i=%08X role=%s resource=%08X "
        "gpu_base=%08X upload_base=%08X bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), record_index, role, resource,
        gpu_base, view.upload_guest_base, byte_count);
    return view;
  }

  LogLine(
      "FM2_PLUME_DIRECT_BUFFER n=%llu rec_i=%08X role=%s resource=%08X "
      "gpu_base=%08X upload_base=%08X bytes=%u data=%s",
      static_cast<unsigned long long>(sample_number), record_index, role, resource,
      gpu_base, view.upload_guest_base, byte_count, bytes.c_str());
  return view;
}

fm2nr::DirectDrawBufferViewSummary LogNativeReplayD3DResource(
    uint8_t* base, uint64_t sample_number, uint32_t record_index,
    const char* role, uint32_t resource, uint32_t byte_offset,
    uint32_t stride_or_format, bool index_buffer) {
  if (!resource ||
      !GuestReadableRange(base, resource, fm2nr::kD3DResourceDecodeSize)) {
    LogLine(
        "FM2_PLUME_NATIVE_D3DRESOURCE n=%llu rec_i=%08X role=%s "
        "resource=%08X unreadable=1 byte_offset=%u stride_or_format=%u "
        "index=%u",
        static_cast<unsigned long long>(sample_number), record_index, role,
        resource, byte_offset, stride_or_format, index_buffer ? 1u : 0u);
    return {};
  }

  const uint32_t gpu_base =
      TryLoadU32(base, resource + fm2nr::kD3DResourceGpuBaseOffset);
  const uint32_t byte_size =
      TryLoadU32(base, resource + fm2nr::kD3DResourceSizeOffset);
  const uint32_t readable_bytes =
      fm2nr::DirectDrawResourceReadableByteCount(byte_size);
  const uint32_t element_bytes =
      index_buffer ? fm2nr::DirectDrawIndexElementByteCount(stride_or_format)
                   : stride_or_format;
  if (gpu_base == 0 || element_bytes == 0 || byte_offset >= readable_bytes) {
    LogLine(
        "FM2_PLUME_NATIVE_D3DRESOURCE n=%llu rec_i=%08X role=%s "
        "resource=%08X gpu_base=%08X byte_size=%08X readable_bytes=%08X "
        "byte_offset=%u stride_or_format=%u element_bytes=%u invalid_view=1",
        static_cast<unsigned long long>(sample_number), record_index, role,
        resource, gpu_base, byte_size, readable_bytes, byte_offset,
        stride_or_format, element_bytes);
    return {};
  }

  const uint32_t upload_base = AddGuestOffsetOrZero(gpu_base, byte_offset);
  const uint32_t upload_bytes = readable_bytes - byte_offset;
  const uint32_t descriptor_count = upload_bytes / element_bytes;
  fm2nr::DirectDrawBufferViewSummary view =
      fm2nr::BuildDirectDrawBufferViewSummary(
          upload_base, upload_bytes, descriptor_count, stride_or_format,
          index_buffer, false, 0);
  uint64_t buffer_hash = 0;
  const bool buffer_hash_ok =
      HashGuestReadableRange(base, view.upload_guest_base, view.hash_bytes,
                             buffer_hash);
  view.hash_ok = buffer_hash_ok;
  view.hash = buffer_hash;

  LogLine(
      "FM2_PLUME_NATIVE_D3DRESOURCE n=%llu rec_i=%08X role=%s resource=%08X "
      "gpu_base=%08X upload_base=%08X byte_size=%08X readable_bytes=%08X "
      "byte_offset=%u stride_or_format=%u descriptor_count=%u view_bytes=%08X "
      "hash_bytes=%08X hash_ok=%u hash=%016llX index=%u",
      static_cast<unsigned long long>(sample_number), record_index, role,
      resource, gpu_base, view.upload_guest_base, byte_size, readable_bytes,
      byte_offset, stride_or_format, descriptor_count, view.view_bytes,
      view.hash_bytes, buffer_hash_ok ? 1u : 0u,
      static_cast<unsigned long long>(buffer_hash), index_buffer ? 1u : 0u);
  return view;
}

void MaybeArmPlumeD3DDirtyStateAfterDirectTrace() {
  const uint32_t sample_limit =
      REXCVAR_GET(fm2_plume_trace_render_context_after_direct_limit);
  const bool already_armed =
      g_plume_render_context_after_direct_armed.load(std::memory_order_relaxed) != 0;
  if (!fm2nr::ShouldArmD3DCommandContextAfterDirectTrace(sample_limit,
                                                         already_armed)) {
    return;
  }

  uint8_t expected = 0;
  if (!g_plume_render_context_after_direct_armed.compare_exchange_strong(
          expected, 1, std::memory_order_relaxed)) {
    return;
  }

  g_plume_render_context_after_direct_remaining.store(sample_limit,
                                                      std::memory_order_relaxed);
  LogLine("FM2_PLUME_D3D_DIRTY_STATE_AFTER_DIRECT_ARM limit=%u", sample_limit);
}

bool TryConsumePlumeD3DDirtyStateAfterDirectTraceSample() {
  uint32_t remaining =
      g_plume_render_context_after_direct_remaining.load(std::memory_order_relaxed);
  while (fm2nr::ShouldTraceD3DCommandContextAfterDirectSample(remaining)) {
    if (g_plume_render_context_after_direct_remaining.compare_exchange_weak(
            remaining, remaining - 1u, std::memory_order_relaxed)) {
      return true;
    }
  }
  return false;
}

void RememberDirectReplayPlan(uint64_t sample_number, uint32_t record_index,
                              const fm2nr::DirectDrawDebugReplayPlan& plan) {
  if (!plan.ready) {
    return;
  }

  std::lock_guard lock(g_plume_direct_replay_plan_mutex);
  RememberedDirectReplayPlan& remembered =
      g_plume_direct_replay_plans[g_plume_direct_replay_plan_next];
  remembered.valid = true;
  remembered.sample_number = sample_number;
  remembered.record_index = record_index;
  remembered.plan = plan;
  g_plume_direct_replay_plan_next =
      (g_plume_direct_replay_plan_next + 1u) % kRememberedDirectReplayPlanCount;
  if (g_plume_direct_replay_plan_count < kRememberedDirectReplayPlanCount) {
    ++g_plume_direct_replay_plan_count;
  }
}

void LogDirectReplayFetchMatches(
    uint64_t sample_number, bool after_direct,
    const fm2nr::D3DCommandContextFetchGroupSummary& group,
    uint32_t vertex_index,
    const fm2nr::D3DCommandContextVertexFetchSummary& vertex_fetch) {
  if (!fm2nr::D3DCommandContextVertexFetchIsValid(vertex_fetch)) {
    return;
  }

  std::lock_guard lock(g_plume_direct_replay_plan_mutex);
  for (uint32_t remembered_index = 0;
       remembered_index < g_plume_direct_replay_plan_count; ++remembered_index) {
    const RememberedDirectReplayPlan& remembered =
        g_plume_direct_replay_plans[remembered_index];
    if (!remembered.valid) {
      continue;
    }

    const fm2nr::DirectDrawDebugReplayPlan& plan = remembered.plan;
    const uint32_t stream_count = plan.stream_count < 2u ? plan.stream_count : 2u;
    for (uint32_t stream_index = 0; stream_index < stream_count; ++stream_index) {
      const fm2nr::DirectDrawReplayVertexFetchMatch match =
          fm2nr::MatchDirectDrawReplayStreamToVertexFetch(
              plan.streams[stream_index], group.group_index, vertex_index,
              vertex_fetch);
      if (!match.valid) {
        continue;
      }

      LogLine(
          "FM2_PLUME_D3D_VERTEX_FETCH_REPLAY_MATCH n=%llu after_direct=%u "
          "direct_n=%llu rec_i=%08X stream_slot=%u fetch_group=%u "
          "fetch_slot=%u fetch_const=%u stream_base=%08X "
          "stream_upload_base=%08X stream_fetch_base=%08X stream_bytes=%u fetch_base=%08X "
          "fetch_bytes=%u stream_hash=%016llX",
          static_cast<unsigned long long>(sample_number), after_direct ? 1u : 0u,
          static_cast<unsigned long long>(remembered.sample_number),
          remembered.record_index, match.stream_slot, match.fetch_group,
          match.fetch_group_slot, match.fetch_constant, match.stream_guest_base,
          plan.streams[stream_index].upload_guest_base, match.stream_fetch_base,
          match.stream_bytes, match.fetch_base, match.fetch_bytes,
          static_cast<unsigned long long>(plan.streams[stream_index].hash));
    }
  }
}

void LogD3DCommandContextFetchGroups(uint8_t* base, uint64_t sample_number,
                                     bool after_direct, uint32_t state_shadow) {
  uint32_t group_limit =
      REXCVAR_GET(fm2_plume_trace_render_context_fetch_group_limit);
  if (group_limit == 0 || state_shadow == 0) {
    return;
  }
  if (group_limit > fm2nr::kD3DCommandContextFetchGroupCount) {
    group_limit = fm2nr::kD3DCommandContextFetchGroupCount;
  }

  const uint32_t byte_count =
      group_limit * fm2nr::kD3DCommandContextFetchGroupByteCount;
  if (!base || !GuestReadableRange(base, state_shadow, byte_count)) {
    LogLine(
        "FM2_PLUME_D3D_FETCH_GROUPS n=%llu after_direct=%u "
        "state_shadow=%08X requested_groups=%u bytes=%u unreadable=1",
        static_cast<unsigned long long>(sample_number), after_direct ? 1u : 0u,
        state_shadow, group_limit, byte_count);
    return;
  }

  const uint8_t* state_shadow_bytes =
      reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(state_shadow));
  for (uint32_t group_index = 0; group_index < group_limit; ++group_index) {
    const fm2nr::D3DCommandContextFetchGroupSummary group =
        fm2nr::DecodeD3DCommandContextFetchGroup(
            state_shadow_bytes, byte_count, group_index);
    if (!group.valid) {
      break;
    }

    if (group.kind == fm2nr::D3DCommandContextFetchGroupKind::kTexture) {
      const auto& texture = group.texture;
      LogLine(
          "FM2_PLUME_D3D_FETCH_GROUP n=%llu after_direct=%u "
          "group=%u reg_base=%04X kind=%s "
          "w0=%08X w1=%08X w2=%08X w3=%08X w4=%08X w5=%08X "
          "type=%u format=%u endian=%u base=%08X mip=%08X "
          "dimension=%u pitch=%u tiled=%u width=%u height=%u depth_or_stack=%u",
          static_cast<unsigned long long>(sample_number), after_direct ? 1u : 0u,
          group.group_index, group.register_base,
          fm2nr::D3DCommandContextFetchGroupKindName(group.kind),
          group.dwords[0], group.dwords[1], group.dwords[2], group.dwords[3],
          group.dwords[4], group.dwords[5], texture.type, texture.format,
          texture.endian, texture.base_address, texture.mip_address,
          texture.dimension, texture.pitch, texture.tiled, texture.width,
          texture.height, texture.depth_or_stack);
      continue;
    }

    LogLine(
        "FM2_PLUME_D3D_FETCH_GROUP n=%llu after_direct=%u "
        "group=%u reg_base=%04X kind=%s "
        "w0=%08X w1=%08X w2=%08X w3=%08X w4=%08X w5=%08X "
        "v0_type=%u v0_base=%08X v0_endian=%u v0_size_words=%u v0_size_bytes=%u "
        "v1_type=%u v1_base=%08X v1_endian=%u v1_size_words=%u v1_size_bytes=%u "
        "v2_type=%u v2_base=%08X v2_endian=%u v2_size_words=%u v2_size_bytes=%u",
        static_cast<unsigned long long>(sample_number), after_direct ? 1u : 0u,
        group.group_index, group.register_base,
        fm2nr::D3DCommandContextFetchGroupKindName(group.kind),
        group.dwords[0], group.dwords[1], group.dwords[2], group.dwords[3],
        group.dwords[4], group.dwords[5], group.vertex[0].type,
        group.vertex[0].address_bytes, group.vertex[0].endian,
        group.vertex[0].size_words, group.vertex[0].size_bytes,
        group.vertex[1].type, group.vertex[1].address_bytes,
        group.vertex[1].endian, group.vertex[1].size_words,
        group.vertex[1].size_bytes, group.vertex[2].type,
        group.vertex[2].address_bytes, group.vertex[2].endian,
        group.vertex[2].size_words, group.vertex[2].size_bytes);

    for (uint32_t vertex_index = 0; vertex_index < 3u; ++vertex_index) {
      const auto& vertex_fetch = group.vertex[vertex_index];
      if (!fm2nr::D3DCommandContextVertexFetchIsValid(vertex_fetch)) {
        continue;
      }
      const uint32_t raw_dword_index = vertex_index * 2u;
      LogLine(
          "FM2_PLUME_D3D_VERTEX_FETCH n=%llu after_direct=%u "
          "group=%u slot=%u fetch_const=%u reg_base=%04X "
          "raw0=%08X raw1=%08X type=%u base=%08X endian=%u "
          "size_words=%u size_bytes=%u",
          static_cast<unsigned long long>(sample_number),
          after_direct ? 1u : 0u, group.group_index, vertex_index,
          fm2nr::D3DCommandContextVertexFetchConstantIndex(group.group_index,
                                                           vertex_index),
          group.register_base + raw_dword_index, group.dwords[raw_dword_index],
          group.dwords[raw_dword_index + 1u], vertex_fetch.type,
          vertex_fetch.address_bytes, vertex_fetch.endian,
          vertex_fetch.size_words, vertex_fetch.size_bytes);
      LogDirectReplayFetchMatches(sample_number, after_direct, group,
                                  vertex_index, vertex_fetch);
    }
  }
}

void MaybeLogPlumeDirectIndexedDrawDecode(
    uint32_t direct_render_ctx, uint32_t draw_iface,
    const fm2nr::DirectDrawLiveDrawFilter& live_draw_filter = {},
    const fm2nr::NativeStateSnapshot* native_snapshot_override = nullptr,
    bool allow_native_direct_submit = true) {
  const bool trace_direct_decode = REXCVAR_GET(fm2_plume_trace_direct_decode);
  const bool compare_replay = fm2::native_renderer::WantsCompareWindow();
  const bool native_direct_draw =
      allow_native_direct_submit &&
      fm2::native_renderer::WantsNativeDirectDraw();
  const bool native_direct_draw_live_batch =
      fm2::native_renderer::WantsNativeDirectDrawLiveBatch();
  if (!fm2nr::ShouldDecodeDirectDrawForPlumeSubmission(
          trace_direct_decode, compare_replay, native_direct_draw)) {
    return;
  }

  bool trace_sample = false;
  const uint32_t sample_limit =
      REXCVAR_GET(fm2_plume_trace_direct_decode_limit);
  const uint32_t sample_skip = REXCVAR_GET(fm2_plume_trace_direct_decode_skip);
  const uint64_t sample_ix =
      g_plume_direct_decode_samples.fetch_add(1, std::memory_order_relaxed);
  if (trace_direct_decode && sample_limit != 0) {
    trace_sample = fm2nr::ShouldTraceDirectDrawSample(
        sample_ix, sample_skip, sample_limit);
  }
  if (!trace_sample && !compare_replay && !native_direct_draw) {
    return;
  }

  uint8_t* base = GuestBase();
  if (!base || !GuestReadableRange(base, direct_render_ctx, 0x5AC)) {
    LogLine("FM2_PLUME_DIRECT_DECODE n=%llu ctx=%08X iface=%08X invalid_ctx=1",
            static_cast<unsigned long long>(sample_ix + 1), direct_render_ctx, draw_iface);
    return;
  }
  const fm2nr::NativeStateSnapshot native_snapshot =
      native_snapshot_override ? *native_snapshot_override
                               : fm2nr::SnapshotNativeStateForDirectDraw(
                                     direct_render_ctx);

  constexpr uint32_t kMaxDecodedRecords = 256;
  constexpr uint32_t kMaxDecodedSegments = 1024;

  const uint8_t built = TryLoadU8(base, direct_render_ctx + fm2nr::kDirectDrawCtxBuiltOffset);
  const uint32_t record_begin =
      TryLoadU32(base, direct_render_ctx + fm2nr::kDirectDrawCtxRecordBeginOffset);
  const uint32_t record_end =
      TryLoadU32(base, direct_render_ctx + fm2nr::kDirectDrawCtxRecordEndOffset);
  const uint32_t record_count = fm2nr::BoundedVectorCount(
      record_begin, record_end, fm2nr::kDirectDrawRecordStride, kMaxDecodedRecords);
  const uint32_t record_scan =
      live_draw_filter.enabled
          ? record_count
          : fm2nr::DirectPlumeSubmissionRecordScanCount(
                record_count,
                REXCVAR_GET(fm2_plume_trace_direct_decode_record_limit),
                compare_replay, native_direct_draw);

  LogLine(
      "FM2_PLUME_DIRECT_DECODE n=%llu ctx=%08X iface=%08X built=%u "
      "rec_begin=%08X rec_end=%08X rec_count=%u scan=%u",
      static_cast<unsigned long long>(sample_ix + 1), direct_render_ctx, draw_iface,
      static_cast<unsigned>(built), record_begin, record_end, record_count, record_scan);
  LogDirectDrawVSFloatConstants(sample_ix + 1, "direct_decode");
  MaybeArmPlumeD3DDirtyStateAfterDirectTrace();

  LogDirectDrawInterfaceSlots(base, sample_ix + 1, draw_iface);
  const fm2nr::DirectDrawShaderKeySummary vertex_shader =
      LogDirectDrawStateObject(base, sample_ix + 1, direct_render_ctx, "vertex_shader",
                               fm2nr::kDirectDrawCtxVertexShaderHandleOffset,
                               fm2nr::kDirectDrawVertexShaderTableBaseOffset,
                               fm2nr::kDirectDrawVertexShaderTableOffsetField);
  const fm2nr::DirectDrawShaderKeySummary pixel_shader =
      LogDirectDrawStateObject(base, sample_ix + 1, direct_render_ctx, "pixel_shader",
                               fm2nr::kDirectDrawCtxSlot28StateHandleOffset,
                               fm2nr::kDirectDrawSlot28StateTableBaseOffset,
                               fm2nr::kDirectDrawSlot28StateTableOffsetField);
  LogDirectDrawResourceDescriptor(base, sample_ix + 1, 0xFFFFFFFFu, "stream1",
                                  direct_render_ctx + fm2nr::kDirectDrawCtxStream1Offset);
  const uint32_t stream1_resource =
      TryLoadU32(base, direct_render_ctx + fm2nr::kDirectDrawCtxStream1Offset);
  const uint32_t stream1_w04 =
      TryLoadU32(base, direct_render_ctx + fm2nr::kDirectDrawCtxStream1Offset + 0x04u);
  const uint32_t stream1_w08 =
      TryLoadU32(base, direct_render_ctx + fm2nr::kDirectDrawCtxStream1Offset + 0x08u);
  const fm2nr::DirectDrawBufferViewSummary stream1_view = LogDirectDrawD3DResource(
      base, sample_ix + 1, 0xFFFFFFFFu, "stream1", stream1_resource, stream1_w04,
      stream1_w08);
  std::vector<fm2nr::DirectDrawReplaySubmission> compare_submissions;
  if (compare_replay) {
    compare_submissions.reserve(record_scan);
  }

  for (uint32_t record_index = 0; record_index < record_scan; ++record_index) {
    const uint32_t record = record_begin + record_index * fm2nr::kDirectDrawRecordStride;
    if (!GuestReadableRange(base, record, fm2nr::kDirectDrawRecordStride)) {
      LogLine("FM2_PLUME_DIRECT_RECORD n=%llu rec_i=%u rec=%08X unreadable=1",
              static_cast<unsigned long long>(sample_ix + 1), record_index, record);
      continue;
    }

    const uint32_t holder = TryLoadU32(base, record + fm2nr::kDirectDrawRecordHolderOffset);
    const uint32_t bind0 = TryLoadU32(base, record + fm2nr::kDirectDrawRecordStream0Offset);
    const uint32_t index_resource =
        TryLoadU32(base, record + fm2nr::kDirectDrawRecordIndexResourceOffset);
    const uint32_t stream0_resource = TryLoadU32(base, bind0 + 0x00u);
    const uint32_t stream0_w04 = TryLoadU32(base, bind0 + 0x04u);
    const uint32_t stream0_w08 = TryLoadU32(base, bind0 + 0x08u);
    const uint32_t index_d3d_resource = TryLoadU32(base, index_resource + 0x00u);
    const uint32_t index_w04 = TryLoadU32(base, index_resource + 0x04u);
    const uint32_t index_w08 = TryLoadU32(base, index_resource + 0x08u);
    uint32_t segment_begin = 0;
    uint32_t segment_end = 0;
    uint32_t segment_count = 0;
    uint32_t first_segment_index = 0xFFFFFFFFu;
    uint32_t first_segment = 0;
    uint16_t first_start = 0;
    uint16_t first_index_count = 0;
    fm2nr::DirectDrawReplayTopology first_topology =
        fm2nr::DirectDrawReplayTopology::kUnknown;
    fm2nr::DirectDrawSegmentSummary first_segment_summary;

    if (holder && GuestReadableRange(base, holder + fm2nr::kDirectDrawHolderSegmentBeginOffset,
                                     8)) {
      segment_begin =
          TryLoadU32(base, holder + fm2nr::kDirectDrawHolderSegmentBeginOffset);
      segment_end = TryLoadU32(base, holder + fm2nr::kDirectDrawHolderSegmentEndOffset);
      segment_count = fm2nr::BoundedVectorCount(
          segment_begin, segment_end, fm2nr::kDirectDrawSegmentStride, kMaxDecodedSegments);
      const uint32_t segment_scan = segment_count < 8u ? segment_count : 8u;
      for (uint32_t segment_index = 0; segment_index < segment_scan; ++segment_index) {
        const uint32_t segment =
            segment_begin + segment_index * fm2nr::kDirectDrawSegmentStride;
        if (!GuestReadableRange(base, segment, fm2nr::kDirectDrawSegmentStride)) {
          continue;
        }
        const auto segment_summary =
            fm2nr::DecodeDirectDrawSegmentSummary(
                reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(segment)),
                fm2nr::kDirectDrawSegmentStride);
        if (!segment_summary.valid) {
          continue;
        }
        LogLine(
            "FM2_PLUME_DIRECT_SEGMENT n=%llu rec_i=%08X seg_i=%u "
            "seg=%08X raw_w0=%04X raw_w2=%04X start=%u index_count=%u",
            static_cast<unsigned long long>(sample_ix + 1), record_index,
            segment_index, segment, segment_summary.raw_w0,
            segment_summary.raw_w2,
            static_cast<unsigned>(segment_summary.start_index),
            static_cast<unsigned>(segment_summary.index_count));
        if (segment_summary.index_count == 0) {
          continue;
        }
        if (first_segment_index != 0xFFFFFFFFu) {
          continue;
        }
        first_segment_index = segment_index;
        first_segment = segment;
        first_start = segment_summary.start_index;
        first_index_count = segment_summary.index_count;
        first_segment_summary = segment_summary;
        first_topology =
            fm2nr::DirectDrawReplayTopologyFromSegmentSummary(segment_summary);
      }
    }

    if (!fm2nr::DirectDrawLiveDrawFilterMatchesRecord(
            live_draw_filter, stream0_resource, index_d3d_resource,
            first_segment_summary)) {
      continue;
    }

    const uint32_t primitive_count =
        fm2nr::DirectDrawReplayPrimitiveCountFromIndexCount(
            first_topology, first_index_count);
    LogLine(
        "FM2_PLUME_DIRECT_RECORD n=%llu rec_i=%u rec=%08X holder=%08X "
        "bind0=%08X index_res=%08X seg_begin=%08X seg_end=%08X seg_count=%u "
        "first_seg_i=%08X first_seg=%08X topology=%u start=%u "
        "index_count=%u prim_count=%u",
        static_cast<unsigned long long>(sample_ix + 1), record_index, record, holder, bind0,
        index_resource, segment_begin, segment_end, segment_count, first_segment_index,
        first_segment, static_cast<unsigned>(first_topology),
        static_cast<unsigned>(first_start),
        static_cast<unsigned>(first_index_count), primitive_count);
    LogDirectDrawResourceDescriptor(base, sample_ix + 1, record_index, "stream0", bind0);
    const fm2nr::DirectDrawBufferViewSummary stream0_view = LogDirectDrawD3DResource(
        base, sample_ix + 1, record_index, "stream0", stream0_resource, stream0_w04,
        stream0_w08);
    LogDirectDrawResourceDescriptor(base, sample_ix + 1, record_index, "index",
                                    index_resource);
    const fm2nr::DirectDrawBufferViewSummary index_view = LogDirectDrawD3DResource(
        base, sample_ix + 1, record_index, "index", index_d3d_resource, index_w04,
        index_w08);
    const fm2nr::DirectDrawIndexedPacketSummary packet =
        fm2nr::BuildDirectDrawIndexedPacketSummary(
            record_index, first_topology, first_start, first_index_count,
            stream0_view, stream1_view,
            index_view, vertex_shader, pixel_shader);
    LogLine(
        "FM2_PLUME_DIRECT_PACKET n=%llu rec_i=%08X first_index=%u "
        "index_count=%u prim_count=%u buffers_ready=%u shader_keys=%u "
        "vertex_ucode_key=%u pixel_ucode_key=%u full_ucode_keys=%u "
        "debug_replay=%u stream0_hash=%016llX stream1_hash=%016llX "
        "index_hash=%016llX vertex_payload_hash=%016llX "
        "pixel_payload_hash=%016llX",
        static_cast<unsigned long long>(sample_ix + 1), record_index, packet.first_index,
        packet.index_count, packet.primitive_count,
        packet.HasReplayableBuffers() ? 1u : 0u,
        packet.HasStableShaderPayloadKeys() ? 1u : 0u,
        packet.HasVertexStructuralUcodeKey() ? 1u : 0u,
        packet.HasPixelStructuralUcodeKey() ? 1u : 0u,
        packet.HasCompleteStructuralUcodeKeys() ? 1u : 0u,
        packet.CanAttemptDebugReplay() ? 1u : 0u,
        static_cast<unsigned long long>(packet.stream0.hash),
        static_cast<unsigned long long>(packet.stream1.hash),
        static_cast<unsigned long long>(packet.index.hash),
        static_cast<unsigned long long>(packet.vertex_shader.payload_hash),
        static_cast<unsigned long long>(packet.pixel_shader.payload_hash));
    fm2nr::DirectDrawDebugReplayPlan replay_plan =
        fm2nr::BuildDirectDrawDebugReplayPlan(packet, native_snapshot);
    const std::string transform_source =
        REXCVAR_GET(fm2_plume_direct_replay_transform_source);
    std::string transform_source_log = transform_source;
    if (const uint32_t* constants = DirectDrawVSFloatConstantRegisters()) {
      if (transform_source == "auto") {
        const fm2nr::DirectDrawReplayTransformInterpretation interpretation =
            fm2nr::DirectDrawReplayTransformInterpretation::kColumnMajorClip;
        if (stream0_view.upload_guest_base != 0 &&
            stream0_view.hash_bytes != 0 &&
            stream0_view.element_bytes != 0 &&
            GuestReadableRange(base, stream0_view.upload_guest_base,
                               stream0_view.hash_bytes)) {
          const uint8_t* upload_bytes = reinterpret_cast<const uint8_t*>(
              REX_RAW_ADDR(stream0_view.upload_guest_base));
          const fm2nr::DirectDrawDebugReplayTransformSelection selection =
              fm2nr::SelectDirectDrawDebugReplayTransformCandidate(
                  upload_bytes, stream0_view.hash_bytes,
                  stream0_view.element_bytes, constants, 256u, interpretation,
                  kDirectDrawPositionStatsMaxVertices);
          if (selection.valid) {
            replay_plan.transform = selection.transform;
            transform_source_log =
                std::string("auto:") + std::string(selection.source_name);
          }
          LogLine(
              "FM2_PLUME_DIRECT_AUTO_TRANSFORM n=%llu rec_i=%08X "
              "valid=%u selected=%s interp=%s stride=%u sampled=%u "
              "projectable=%u xy_inside=%u d3d_inside=%u "
              "has_ndc=%u ndc_min=(%.6f,%.6f,%.6f) "
              "ndc_max=(%.6f,%.6f,%.6f)",
              static_cast<unsigned long long>(sample_ix + 1), record_index,
              selection.valid ? 1u : 0u,
              selection.valid ? selection.source_name.data() : "none",
              DirectDrawTransformInterpretationName(interpretation),
              stream0_view.element_bytes, selection.stats.sampled_vertices,
              selection.stats.projectable_vertices,
              selection.stats.xy_inside_vertices,
              selection.stats.d3d_xyz_inside_vertices,
              selection.stats.has_ndc_bounds ? 1u : 0u,
              selection.stats.min_ndc_x, selection.stats.min_ndc_y,
              selection.stats.min_ndc_z, selection.stats.max_ndc_x,
              selection.stats.max_ndc_y, selection.stats.max_ndc_z);
        } else {
          LogLine(
              "FM2_PLUME_DIRECT_AUTO_TRANSFORM n=%llu rec_i=%08X "
              "valid=0 selected=none unreadable_stream0=1 "
              "upload_base=%08X bytes=%u stride=%u",
              static_cast<unsigned long long>(sample_ix + 1), record_index,
              stream0_view.upload_guest_base, stream0_view.hash_bytes,
              stream0_view.element_bytes);
        }
      } else {
        replay_plan.transform =
            BuildDirectReplayTransformForSource(constants, transform_source);
      }
      LogDirectDrawTransformCandidates(base, sample_ix + 1, record_index,
                                       stream0_view, constants);
    }
    LogLine(
        "FM2_PLUME_DIRECT_REPLAY_PLAN n=%llu rec_i=%08X ready=%u "
        "topology=%u index_format=%u stream_count=%u "
        "transform_valid=%u transform_first=%u transform_source=%s "
        "s0_slot=%u s0_stride=%u s0_base=%08X s0_upload_base=%08X "
        "s0_bytes=%08X s0_hash=%016llX s1_slot=%u s1_stride=%u "
        "s1_base=%08X s1_upload_base=%08X s1_bytes=%08X "
        "s1_hash=%016llX index_base=%08X index_upload_base=%08X "
        "index_bytes=%08X index_hash=%016llX draw_index_count=%u "
        "draw_instances=%u draw_start_index=%u draw_base_vertex=%d "
        "draw_start_instance=%u vertex_payload_hash=%016llX "
        "pixel_payload_hash=%016llX vertex_structural_ucode=%u "
        "pixel_structural_ucode=%u",
        static_cast<unsigned long long>(sample_ix + 1), record_index,
        replay_plan.ready ? 1u : 0u,
        static_cast<unsigned>(replay_plan.topology),
        static_cast<unsigned>(replay_plan.index_format), replay_plan.stream_count,
        replay_plan.transform.valid ? 1u : 0u, replay_plan.transform.first_constant,
        transform_source_log.c_str(),
        replay_plan.streams[0].slot, replay_plan.streams[0].stride,
        replay_plan.streams[0].guest_base, replay_plan.streams[0].upload_guest_base,
        replay_plan.streams[0].upload_bytes,
        static_cast<unsigned long long>(replay_plan.streams[0].hash),
        replay_plan.streams[1].slot, replay_plan.streams[1].stride,
        replay_plan.streams[1].guest_base, replay_plan.streams[1].upload_guest_base,
        replay_plan.streams[1].upload_bytes,
        static_cast<unsigned long long>(replay_plan.streams[1].hash),
        replay_plan.index.guest_base, replay_plan.index.upload_guest_base,
        replay_plan.index.upload_bytes, static_cast<unsigned long long>(replay_plan.index.hash),
        replay_plan.draw.index_count, replay_plan.draw.instance_count,
        replay_plan.draw.start_index, replay_plan.draw.base_vertex,
        replay_plan.draw.start_instance,
        static_cast<unsigned long long>(replay_plan.vertex_payload_hash),
        static_cast<unsigned long long>(replay_plan.pixel_payload_hash),
        replay_plan.has_vertex_structural_ucode ? 1u : 0u,
        replay_plan.has_pixel_structural_ucode ? 1u : 0u);
    const auto& native = replay_plan.native_state;
    if (native.valid) {
      LogLine(
          "FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE n=%llu rec_i=%08X "
          "ctx=%08X seq=%llu direct_ctx=%08X iface=%08X "
          "vs=%08X ps=%08X s0_valid=%u s0_res=%08X s0_offset=%u "
          "s0_stride=%u s0_dirty=%08X s1_valid=%u s1_res=%08X "
          "s1_offset=%u s1_stride=%u s1_dirty=%08X ib=%08X "
          "surf=%08X surf_arg=%u viewport_valid=%u viewport_mode=%u "
          "texture_fetch_valid=%u texture_fetch_low=%02X "
          "texture_fetch_mid=%02X clear_valid=%u clear_color_byte=%02X "
          "clear_flags=%02X pass_valid=%u submit=%08X pass_ctx=%08X "
          "pass_flags=%08X drawable=%08X draw_cb=%08X wireframe=%u "
          "draw_mode=%u pass_marker=%u",
          static_cast<unsigned long long>(sample_ix + 1), record_index,
          native.render_context,
          static_cast<unsigned long long>(native.sequence),
          native.direct_render_context, native.draw_iface,
          native.vertex_shader, native.pixel_shader,
          native.streams[0].valid ? 1u : 0u, native.streams[0].resource,
          native.streams[0].byte_offset, native.streams[0].stride_bytes,
          native.streams[0].dirty_mask, native.streams[1].valid ? 1u : 0u,
          native.streams[1].resource, native.streams[1].byte_offset,
          native.streams[1].stride_bytes, native.streams[1].dirty_mask,
          native.index_resource, native.bound_surface,
          native.bound_surface_arg, native.viewport.valid ? 1u : 0u,
          native.viewport.viewport_mode,
          native.texture_fetch.valid ? 1u : 0u,
          static_cast<unsigned>(native.texture_fetch.fetch_bits_low),
          static_cast<unsigned>(native.texture_fetch.fetch_bits_mid),
          native.clear.valid ? 1u : 0u,
          static_cast<unsigned>(native.clear.clear_color_byte),
          static_cast<unsigned>(native.clear.clear_flags),
          native.pass.valid ? 1u : 0u, native.pass.submit_object,
          native.pass.tls_or_pass_context, native.pass.pass_flags,
          native.pass.drawable, native.pass.draw_callback,
          native.pass.wireframe, native.pass.draw_mode,
          native.pass.pass_marker);
    }
    RememberDirectReplayPlan(sample_ix + 1, record_index, replay_plan);
    fm2nr::DirectDrawDebugReplayPlan submission_plan = replay_plan;
    fm2nr::DirectDrawReplaySourceBytes submission_sources = {
        reinterpret_cast<const uint8_t*>(
            REX_RAW_ADDR(replay_plan.streams[0].upload_guest_base)),
        reinterpret_cast<const uint8_t*>(
            REX_RAW_ADDR(replay_plan.streams[1].upload_guest_base)),
        reinterpret_cast<const uint8_t*>(
            REX_RAW_ADDR(replay_plan.index.upload_guest_base)),
    };
    if (fm2nr::ShouldPromoteDirectReplayToNativeLayout(
            compare_replay, native_direct_draw,
            native_direct_draw_live_batch) &&
        replay_plan.ready &&
        fm2nr::DirectDrawReplayNativeLayoutFromState(native) ==
            fm2nr::DirectDrawReplayPipelineLayout::kNativePosition28Side12) {
      const fm2nr::DirectDrawBufferViewSummary native_stream0 =
          LogNativeReplayD3DResource(
              base, sample_ix + 1, record_index, "native_stream0",
              native.streams[0].resource, native.streams[0].byte_offset,
              native.streams[0].stride_bytes, false);
      const fm2nr::DirectDrawBufferViewSummary native_stream1 =
          LogNativeReplayD3DResource(
              base, sample_ix + 1, record_index, "native_stream1",
              native.streams[1].resource, native.streams[1].byte_offset,
              native.streams[1].stride_bytes, false);
      const fm2nr::DirectDrawBufferViewSummary native_index =
          LogNativeReplayD3DResource(
              base, sample_ix + 1, record_index, "native_index",
              native.index_resource, 0u, index_view.descriptor_stride_or_format,
              true);
      const fm2nr::DirectDrawDebugReplayPlan native_plan =
          fm2nr::BuildDirectDrawNativeLayoutReplayPlan(
              replay_plan, native_stream0, native_stream1, native_index);
      LogLine(
          "FM2_PLUME_NATIVE_REPLAY_PLAN n=%llu rec_i=%08X ready=%u "
          "layout=%s s0_base=%08X s0_upload_base=%08X s0_stride=%u "
          "s0_bytes=%08X s1_base=%08X s1_upload_base=%08X s1_stride=%u "
          "s1_bytes=%08X index_base=%08X index_upload_base=%08X "
          "index_bytes=%08X",
          static_cast<unsigned long long>(sample_ix + 1), record_index,
          native_plan.ready ? 1u : 0u,
          fm2nr::DirectDrawReplayPipelineLayoutName(
              fm2nr::DirectDrawReplayPipelineLayoutForPlan(native_plan)),
          native_plan.streams[0].guest_base,
          native_plan.streams[0].upload_guest_base,
          native_plan.streams[0].stride, native_plan.streams[0].upload_bytes,
          native_plan.streams[1].guest_base,
          native_plan.streams[1].upload_guest_base,
          native_plan.streams[1].stride, native_plan.streams[1].upload_bytes,
          native_plan.index.guest_base, native_plan.index.upload_guest_base,
          native_plan.index.upload_bytes);
      if (native_plan.ready) {
        submission_plan = native_plan;
        submission_sources = {
            reinterpret_cast<const uint8_t*>(
                REX_RAW_ADDR(native_plan.streams[0].upload_guest_base)),
            reinterpret_cast<const uint8_t*>(
                REX_RAW_ADDR(native_plan.streams[1].upload_guest_base)),
            reinterpret_cast<const uint8_t*>(
                REX_RAW_ADDR(native_plan.index.upload_guest_base)),
        };
      }
    }
    if (compare_replay && submission_plan.ready) {
      compare_submissions.push_back({submission_plan, submission_sources});
    } else if (native_direct_draw && submission_plan.ready &&
        fm2nr::ShouldSubmitDirectDebugReplayRecord(
            record_index, REXCVAR_GET(fm2_plume_direct_replay_record_index))) {
      const bool submitted = fm2::native_renderer::SubmitNativeDirectDraw(
          submission_plan, submission_sources);
      LogLine(
          "FM2_PLUME_NATIVE_DIRECT_DRAW_RESULT n=%llu rec_i=%08X "
          "submitted=%u mode=%s layout=%s",
          static_cast<unsigned long long>(sample_ix + 1), record_index,
          submitted ? 1u : 0u,
          fm2::native_renderer::GetModeName(fm2::native_renderer::GetMode()),
          fm2nr::DirectDrawReplayPipelineLayoutName(
              fm2nr::DirectDrawReplayPipelineLayoutForPlan(submission_plan)));
      if (fm2nr::ShouldStopDirectPlumeRecordScanAfterNativeAttempt(
              native_direct_draw, compare_replay,
              native_direct_draw_live_batch)) {
        break;
      }
    } else if (replay_plan.ready &&
        fm2::native_renderer::WantsDirectDebugReplay() &&
        fm2nr::ShouldSubmitDirectDebugReplayRecord(
            record_index, REXCVAR_GET(fm2_plume_direct_replay_record_index))) {
      const bool submitted = fm2::native_renderer::SubmitDirectDebugReplay(
          replay_plan, {
                           reinterpret_cast<const uint8_t*>(
                               REX_RAW_ADDR(replay_plan.streams[0].upload_guest_base)),
                           reinterpret_cast<const uint8_t*>(
                               REX_RAW_ADDR(replay_plan.streams[1].upload_guest_base)),
                           reinterpret_cast<const uint8_t*>(
                               REX_RAW_ADDR(replay_plan.index.upload_guest_base)),
                       });
      LogLine(
          "FM2_PLUME_DEBUG_REPLAY_RESULT n=%llu rec_i=%08X submitted=%u "
          "mode=%s",
          static_cast<unsigned long long>(sample_ix + 1), record_index,
          submitted ? 1u : 0u,
          fm2::native_renderer::GetModeName(fm2::native_renderer::GetMode()));
    }
  }
  if (!compare_submissions.empty()) {
    const bool submitted = fm2::native_renderer::SubmitDirectDebugReplayBatch(
        compare_submissions.data(),
        static_cast<uint32_t>(compare_submissions.size()));
    LogLine(
        "FM2_PLUME_COMPARE_REPLAY_RESULT n=%llu submissions=%u submitted=%u "
        "mode=%s",
        static_cast<unsigned long long>(sample_ix + 1),
        static_cast<uint32_t>(compare_submissions.size()), submitted ? 1u : 0u,
        fm2::native_renderer::GetModeName(fm2::native_renderer::GetMode()));
  }
}

void MaybeLogPlumeD3DDirtyStateEntry(uint32_t render_context, uint32_t mask_object,
                                     uint32_t arg5) {
  const uint32_t sample_limit = REXCVAR_GET(fm2_plume_trace_render_context_limit);
  const uint32_t after_direct_limit =
      REXCVAR_GET(fm2_plume_trace_render_context_after_direct_limit);
  const uint32_t after_direct_remaining =
      g_plume_render_context_after_direct_remaining.load(std::memory_order_relaxed);
  if (sample_limit == 0 && after_direct_limit == 0 && after_direct_remaining == 0) {
    return;
  }
  const uint32_t sample_skip = REXCVAR_GET(fm2_plume_trace_render_context_skip);

  const uint64_t sample_ix =
      g_plume_render_context_samples.fetch_add(1, std::memory_order_relaxed);
  const bool trace_window =
      fm2nr::ShouldTraceD3DCommandContextSample(sample_ix, sample_skip,
                                                sample_limit);
  const bool trace_after_direct =
      TryConsumePlumeD3DDirtyStateAfterDirectTraceSample();
  if (!trace_window && !trace_after_direct) {
    return;
  }

  uint8_t* base = GuestBase();
  const uint32_t state_shadow = AddGuestOffsetOrZero(
      render_context, fm2nr::kD3DCommandContextStateShadowOffset);
  const bool ctx_readable =
      base && render_context != 0 && GuestReadableRange(base, render_context, 40);
  const bool mask_readable =
      base && mask_object != 0 && GuestReadableRange(base, mask_object + 24u, 40);
  const bool state_shadow_readable =
      state_shadow != 0 && GuestReadableRange(base, state_shadow, sizeof(uint32_t));

  const uint64_t ctx00 = TryLoadU64AtOffset(base, render_context, 0x00u);
  const uint64_t ctx08 = TryLoadU64AtOffset(base, render_context, 0x08u);
  const uint64_t ctx10 = TryLoadU64AtOffset(base, render_context, 0x10u);
  const uint64_t ctx18 = TryLoadU64AtOffset(base, render_context, 0x18u);
  const uint64_t ctx20 = TryLoadU64AtOffset(base, render_context, 0x20u);
  const uint64_t mask18 = TryLoadU64AtOffset(base, mask_object, 0x18u);
  const uint64_t mask20 = TryLoadU64AtOffset(base, mask_object, 0x20u);
  const uint64_t mask28 = TryLoadU64AtOffset(base, mask_object, 0x28u);
  const uint64_t mask30 = TryLoadU64AtOffset(base, mask_object, 0x30u);
  const uint64_t mask38 = TryLoadU64AtOffset(base, mask_object, 0x38u);

  LogLine(
      "FM2_PLUME_D3D_DIRTY_STATE_ENTRY n=%llu after_direct=%u "
      "ctx=%08X mask_obj=%08X arg5=%08X "
      "state_shadow=%08X ctx_readable=%u mask_readable=%u state_shadow_readable=%u "
      "ctx00=%016llX ctx08=%016llX ctx10=%016llX ctx18=%016llX ctx20=%016llX "
      "mask18=%016llX mask20=%016llX mask28=%016llX mask30=%016llX mask38=%016llX "
      "eff00=%016llX eff08=%016llX eff10=%016llX eff18=%016llX eff20=%016llX",
      static_cast<unsigned long long>(sample_ix + 1), trace_after_direct ? 1u : 0u,
      render_context, mask_object, arg5, state_shadow, ctx_readable ? 1u : 0u,
      mask_readable ? 1u : 0u,
      state_shadow_readable ? 1u : 0u, static_cast<unsigned long long>(ctx00),
      static_cast<unsigned long long>(ctx08), static_cast<unsigned long long>(ctx10),
      static_cast<unsigned long long>(ctx18), static_cast<unsigned long long>(ctx20),
      static_cast<unsigned long long>(mask18), static_cast<unsigned long long>(mask20),
      static_cast<unsigned long long>(mask28), static_cast<unsigned long long>(mask30),
      static_cast<unsigned long long>(mask38), static_cast<unsigned long long>(ctx00 & mask18),
      static_cast<unsigned long long>(ctx08 & mask20),
      static_cast<unsigned long long>(ctx10 & mask28),
      static_cast<unsigned long long>(ctx18 & mask30),
      static_cast<unsigned long long>(ctx20 & mask38));

  if (state_shadow_readable) {
    LogDirectDrawStateBytes(base, sample_ix + 1, "d3d_ctx", "state_shadow",
                            state_shadow);
    LogD3DCommandContextFetchGroups(base, sample_ix + 1, trace_after_direct,
                                    state_shadow);
  }
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

void FM2PlumeTraceBuildObjectPassEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                                       PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                                       PPCRegister& r9, PPCRegister& r10) {
  fm2::native_renderer::RecordBuildObjectPassEntry({
      r3.u32,
      r4.u32,
      r5.u32,
      r6.u32,
      r7.u32,
      r8.u32,
      r9.u32,
      r10.u32,
  });
}

void FM2PlumeTracePixelShaderState(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativePixelShaderStateArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceVertexShaderState(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeVertexShaderStateArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceVertexStreamBinding(PPCRegister& r3, PPCRegister& r4,
                                      PPCRegister& r5, PPCRegister& r6,
                                      PPCRegister& r7, PPCRegister& r8) {
  fm2nr::RecordNativeVertexStreamBindingArgs(r3.u32, r4.u32, r5.u32, r6.u32,
                                             r7.u32, r8.u32);
}

void FM2PlumeTraceIndexBufferBinding(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeIndexBufferBindingArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceBoundSurface(PPCRegister& r3, PPCRegister& r4,
                               PPCRegister& r5) {
  fm2nr::RecordNativeBoundSurfaceArgs(r3.u32, r4.u32, r5.u32);
}

void FM2PlumeTraceViewportMode(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeViewportModeArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceTextureFetchLow(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeTextureFetchLowArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceTextureFetchMid(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeTextureFetchMidArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceClearColor(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeClearColorArgs(r3.u32, r4.u32);
}

void FM2PlumeTraceClearFlags(PPCRegister& r3, PPCRegister& r4) {
  fm2nr::RecordNativeClearFlagsArgs(r3.u32, r4.u32);
}

void FM2PlumeTracePassDrawWork(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                              PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                              PPCRegister& r9, PPCRegister& r10) {
  fm2nr::RecordNativePassDrawBoundaryArgs(r3.u32, r4.u32, r5.u32, r6.u32, r7.u32,
                                          r8.u32, r9.u32, r10.u32);
}

// Intentionally not manifest-hooked: BuildDirectIndexedDrawBuffers has a one-shot
// guard at direct_render_ctx+0x48. Use FM2PlumeTraceInstanceHybridDrawEntry.
void FM2PlumeTraceDirectIndexedDrawEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                                         PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                                         PPCRegister& r9, PPCRegister& r10) {
  fm2::native_renderer::RecordDirectIndexedDrawEntry({
      r3.u32,
      r4.u32,
      r5.u32,
      r6.u32,
      r7.u32,
      r8.u32,
      r9.u32,
      r10.u32,
  });
}

void FM2PlumeTraceInstanceHybridDrawEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                                          PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                                          PPCRegister& r9, PPCRegister& r10) {
  fm2::native_renderer::RecordInstanceHybridDrawEntry({
      r3.u32,
      r4.u32,
      r5.u32,
      r6.u32,
      r7.u32,
      r8.u32,
      r9.u32,
      r10.u32,
  });
  fm2nr::RecordNativeDirectDrawEntry({.direct_render_context = r3.u32,
                                      .draw_iface = r4.u32});
  MaybeLogPlumeNativeStateSnapshot(
      fm2nr::SnapshotNativeStateForDirectDraw(r3.u32));
  MaybeLogPlumeDirectIndexedDrawDecode(r3.u32, r4.u32, {}, nullptr, false);
}

void FM2PlumeTraceDirectIfaceIndexedDraw(PPCRegister& r3, PPCRegister& r4,
                                         PPCRegister& r5, PPCRegister& r6,
                                         PPCRegister& r7) {
  (void)r5;

  uint8_t* base = GuestBase();
  const uint32_t render_context = TryLoadU32AtOffset(base, r3.u32, 0x14u);
  if (render_context == 0) {
    return;
  }

  const fm2nr::NativeStateSnapshot snapshot =
      fm2nr::SnapshotNativeState(render_context);
  if (!snapshot.valid || !snapshot.last_direct_draw.valid) {
    return;
  }

  MaybeLogPlumeNativeStateSnapshot(snapshot);
  const fm2nr::DirectDrawLiveDrawFilter live_draw_filter =
      fm2nr::BuildDirectDrawLiveDrawFilter(
          r4.u32, r6.u32, r7.u32, snapshot.streams[0].resource,
          snapshot.index_buffer.resource);
  if (!live_draw_filter.enabled) {
    if (REXCVAR_GET(fm2_plume_trace_direct_decode) ||
        REXCVAR_GET(fm2_plume_native_state_trace)) {
      LogLine(
          "FM2_PLUME_DIRECT_LIVE_DRAW iface=%08X render_ctx=%08X "
          "direct_ctx=%08X primitive_type=%u start_index=%u "
          "primitive_count=%u filter_enabled=0 s0_valid=%u s0_res=%08X "
          "s0_stride=%u ib_valid=%u ib=%08X",
          r3.u32, render_context,
          snapshot.last_direct_draw.direct_render_context, r4.u32, r6.u32,
          r7.u32, snapshot.streams[0].valid ? 1u : 0u,
          snapshot.streams[0].resource, snapshot.streams[0].stride_bytes,
          snapshot.index_buffer.valid ? 1u : 0u,
          snapshot.index_buffer.resource);
    }
    return;
  }

  if (REXCVAR_GET(fm2_plume_trace_direct_decode) ||
      REXCVAR_GET(fm2_plume_native_state_trace)) {
    LogLine(
        "FM2_PLUME_DIRECT_LIVE_DRAW iface=%08X render_ctx=%08X "
        "direct_ctx=%08X primitive_type=%u start_index=%u "
        "primitive_count=%u topology=%u index_count=%u "
        "s0_res=%08X s0_stride=%u s1_valid=%u s1_res=%08X "
        "s1_stride=%u ib=%08X",
        r3.u32, render_context, snapshot.last_direct_draw.direct_render_context,
        r4.u32, live_draw_filter.start_index, r7.u32,
        static_cast<unsigned>(live_draw_filter.topology),
        live_draw_filter.index_count, live_draw_filter.stream0_resource,
        snapshot.streams[0].stride_bytes,
        snapshot.streams[1].valid ? 1u : 0u, snapshot.streams[1].resource,
        snapshot.streams[1].stride_bytes, live_draw_filter.index_resource);
  }

  MaybeLogPlumeDirectIndexedDrawDecode(
      snapshot.last_direct_draw.direct_render_context, r3.u32,
      live_draw_filter, &snapshot, true);
}

void FM2PlumeTraceExecuteBoundDrawPass(PPCRegister& r3, PPCRegister& r4,
                                       PPCRegister& r5) {
  // r3=pass_context, r4=pass_index, r5=draw_list_ptr
  // Called via vtable; no direct bl callers; fires per-frame per-pass.
  fm2nr::RecordNativePassDrawBoundaryArgs(r3.u32, r4.u32, r5.u32, 0, 0, 0, 0, 0);
}

void FM2PlumeTraceD3DDirtyStateEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  MaybeLogPlumeD3DDirtyStateEntry(r3.u32, r4.u32, r5.u32);
}

void FM2PlumeTracePresent(PPCRegister& r3) {
  fm2::native_renderer::FlushNativeDirectDrawOnPresent();
  fm2::native_renderer::RecordPresentEntry(r3.u32);
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
