/**
 * TEMP_DIAG V2: FMOD decode cadence per-second diagnostics + write-watch
 *
 * Adds per-second rate logging for:
 *   - Mixer calls/sec (sub_8220A4E8 entry rate)
 *   - NtSetEvent calls/sec for tracked stream event handles
 *   - Wait wakes/sec on tracked stream event handles
 *   - XMA decode calls/sec (XMADisableContext)
 *   - Decoded PCM frames/sec
 *   - Submitted frames/sec (SDL audio callback)
 *   - Host audio queue depth
 *   - BuffersQueued (FMOD internal)
 *   - Audio underrun count
 *
 * Also provides write-watch logging for:
 *   - FMOD object +0x41E (field_41E) and +0x41D (field_41D)
 *   - Global 0x829C24C7 (byte force-signal flag)
 *   - Global 0x829C24C8 (counter)
 * Logs: writer PC/LR, old value, new value, thread, timestamp
 *
 * All logging uses REXKRNL_ERROR to bypass log_level filters.
 * Uses fmt {} format style (not printf % style) for spdlog compatibility.
 * Include from xma_gap_diag.h — do NOT include directly from .cpp files.
 * Remove this file and all #include references when done.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <deque>

#include <rex/logging.h>
#include <rex/system/xthread.h>
#include <rex/runtime.h>

namespace xma_gap_diag_v2 {

// ============================================================================
// Per-Second Rate Counters
// ============================================================================

struct PerSecCounters {
    // Mixer calls (sub_8220A4E8)
    std::atomic<uint32_t> mixer_calls{0};

    // NtSetEvent for tracked stream event handles
    std::atomic<uint32_t> stream_signals{0};

    // Wait wakes on tracked stream event handles (STATUS_SUCCESS)
    std::atomic<uint32_t> stream_wakeups{0};

    // XMA decodes (XMADisableContext calls)
    std::atomic<uint32_t> xma_decodes{0};

    // Decoded PCM frames (accumulated from all contexts)
    std::atomic<uint64_t> pcm_frames_decoded{0};

    // Submitted frames to SDL
    std::atomic<uint64_t> frames_submitted{0};

    // Host queue depth (current)
    std::atomic<uint32_t> host_queue_depth{0};

    // BuffersQueued (FMOD internal, sampled)
    std::atomic<uint32_t> fmod_buffers_queued{0};

    // Underrun count
    std::atomic<uint32_t> underrun_count{0};

    // Worker loop diagnostics (NEW)
    std::atomic<uint32_t> worker_sweeps{0};        // WorkerThreadMain loop iterations
    std::atomic<uint32_t> work_decode_iters{0};    // Total Decode+Consume iterations
    std::atomic<uint32_t> work_no_space{0};        // Work() exit: not enough ring space
    std::atomic<uint32_t> work_no_input{0};        // Work() exit: no valid input
    std::atomic<uint32_t> work_invalid{0};         // Work() skip: not allocated/enabled
    std::atomic<uint32_t> work_valid_zero{0};      // Work() skip: output_buffer_valid==0
    std::atomic<uint32_t> work_consume_only{0};    // Work() path: consume-only drain

    void Reset() {
        mixer_calls.store(0, std::memory_order_relaxed);
        stream_signals.store(0, std::memory_order_relaxed);
        stream_wakeups.store(0, std::memory_order_relaxed);
        xma_decodes.store(0, std::memory_order_relaxed);
        pcm_frames_decoded.store(0, std::memory_order_relaxed);
        frames_submitted.store(0, std::memory_order_relaxed);
        // Don't reset queue depth or underrun count — they're cumulative/current
        fmod_buffers_queued.store(0, std::memory_order_relaxed);
        worker_sweeps.store(0, std::memory_order_relaxed);
        work_decode_iters.store(0, std::memory_order_relaxed);
        work_no_space.store(0, std::memory_order_relaxed);
        work_no_input.store(0, std::memory_order_relaxed);
        work_invalid.store(0, std::memory_order_relaxed);
        work_valid_zero.store(0, std::memory_order_relaxed);
        work_consume_only.store(0, std::memory_order_relaxed);
    }
};

inline PerSecCounters g_per_sec;

// ============================================================================
// Increment helpers (called from instrumentation points)
// ============================================================================

inline void IncMixerCall() {
    g_per_sec.mixer_calls.fetch_add(1, std::memory_order_relaxed);
}

inline void IncStreamSignal() {
    g_per_sec.stream_signals.fetch_add(1, std::memory_order_relaxed);
}

inline void IncStreamWakeup() {
    g_per_sec.stream_wakeups.fetch_add(1, std::memory_order_relaxed);
}

inline void IncXmaDecode() {
    g_per_sec.xma_decodes.fetch_add(1, std::memory_order_relaxed);
}

inline void AddPcmFrames(uint64_t frames) {
    g_per_sec.pcm_frames_decoded.fetch_add(frames, std::memory_order_relaxed);
}

inline void AddSubmittedFrames(uint64_t frames) {
    g_per_sec.frames_submitted.fetch_add(frames, std::memory_order_relaxed);
}

inline void SetHostQueueDepth(uint32_t depth) {
    g_per_sec.host_queue_depth.store(depth, std::memory_order_relaxed);
}

inline void SetFmodBuffersQueued(uint32_t count) {
    g_per_sec.fmod_buffers_queued.store(count, std::memory_order_relaxed);
}

inline void IncUnderrun() {
    g_per_sec.underrun_count.fetch_add(1, std::memory_order_relaxed);
}

inline void IncWorkerSweep() {
    g_per_sec.worker_sweeps.fetch_add(1, std::memory_order_relaxed);
}
inline void AddWorkDecodeIters(uint32_t n) {
    g_per_sec.work_decode_iters.fetch_add(n, std::memory_order_relaxed);
}
inline void IncWorkNoSpace() {
    g_per_sec.work_no_space.fetch_add(1, std::memory_order_relaxed);
}
inline void IncWorkNoInput() {
    g_per_sec.work_no_input.fetch_add(1, std::memory_order_relaxed);
}
inline void IncWorkInvalid() {
    g_per_sec.work_invalid.fetch_add(1, std::memory_order_relaxed);
}
inline void IncWorkValidZero() {
    g_per_sec.work_valid_zero.fetch_add(1, std::memory_order_relaxed);
}
inline void IncWorkConsumeOnly() {
    g_per_sec.work_consume_only.fetch_add(1, std::memory_order_relaxed);
}

// ============================================================================
// Utility
// ============================================================================

inline double WallSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

inline uint64_t GetGuestLR() {
    auto* ctx = rex::runtime::current_ppc_context();
    return ctx ? ctx->lr : 0;
}

inline uint32_t GetGuestTID() {
    auto* thread = rex::system::XThread::GetCurrentThread();
    return thread ? thread->thread_id() : 0;
}

inline uint32_t GetNativeTID() {
    return rex::system::XThread::GetCurrentThreadId();
}

// ============================================================================
// Per-Second Timer Thread
// ============================================================================

inline std::atomic<bool> g_timer_running{false};
inline std::thread g_timer_thread;

inline void PerSecTimerLoop() {
    int second_num = 0;
    while (g_timer_running.load(std::memory_order_relaxed)) {
        // Sleep for 1 second
        for (int i = 0; i < 10 && g_timer_running.load(std::memory_order_relaxed); i++) {
            rex::thread::Sleep(std::chrono::milliseconds(100));
        }
        if (!g_timer_running.load(std::memory_order_relaxed)) break;

        second_num++;

        uint32_t mixer = g_per_sec.mixer_calls.exchange(0, std::memory_order_relaxed);
        uint32_t signals = g_per_sec.stream_signals.exchange(0, std::memory_order_relaxed);
        uint32_t wakeups = g_per_sec.stream_wakeups.exchange(0, std::memory_order_relaxed);
        uint32_t decodes = g_per_sec.xma_decodes.exchange(0, std::memory_order_relaxed);
        uint64_t pcm_frames = g_per_sec.pcm_frames_decoded.exchange(0, std::memory_order_relaxed);
        uint64_t submitted = g_per_sec.frames_submitted.exchange(0, std::memory_order_relaxed);
        uint32_t queue = g_per_sec.host_queue_depth.load(std::memory_order_relaxed);
        uint32_t bufq = g_per_sec.fmod_buffers_queued.load(std::memory_order_relaxed);
        uint32_t underruns = g_per_sec.underrun_count.load(std::memory_order_relaxed);
        uint32_t sweeps = g_per_sec.worker_sweeps.exchange(0, std::memory_order_relaxed);
        uint32_t decode_iters = g_per_sec.work_decode_iters.exchange(0, std::memory_order_relaxed);
        uint32_t no_space = g_per_sec.work_no_space.exchange(0, std::memory_order_relaxed);
        uint32_t no_input = g_per_sec.work_no_input.exchange(0, std::memory_order_relaxed);
        uint32_t invalid = g_per_sec.work_invalid.exchange(0, std::memory_order_relaxed);
        uint32_t valid_zero = g_per_sec.work_valid_zero.exchange(0, std::memory_order_relaxed);
        uint32_t consume_only = g_per_sec.work_consume_only.exchange(0, std::memory_order_relaxed);

        // Ratio analysis
        double decode_to_signal = signals > 0 ? (double)decodes / signals : 0.0;
        double frames_to_decode = decodes > 0 ? (double)pcm_frames / decodes : 0.0;

        REXKRNL_ERROR(
            "FMOD_PERSEC t={} mixer={} signals={} wakeups={} decodes={} "
            "pcm_frames={} submitted={} queue={} bufq={} underruns={} "
            "decode/sig={:.2f} frames/decode={:.0f} "
            "sweeps={} iters={} no_space={} no_input={} invalid={} "
            "valid0={} consume_only={}",
            second_num, mixer, signals, wakeups, decodes,
            (unsigned)pcm_frames, (unsigned)submitted, queue, bufq, underruns,
            decode_to_signal, frames_to_decode,
            sweeps, decode_iters, no_space, no_input, invalid,
            valid_zero, consume_only);

        // Key determination starting at second 3 (let things stabilize)
        if (second_num >= 3) {
            // If decodes/sec >= 90, we're at the target rate
            if (decodes >= 90) {
                REXKRNL_ERROR(
                    "FMOD_PERSEC_RESULT: decodes/sec={} >= 90 TARGET REACHED "
                    "- sub_8220A4E8 divider was the primary bottleneck",
                    decodes);
            } else if (decodes >= 60) {
                REXKRNL_ERROR(
                    "FMOD_PERSEC_RESULT: decodes/sec={} ~64 Hz - doubled from 30 "
                    "but stream thread only decodes 1 chunk per wake. "
                    "Need to investigate per-wake decode count or mixer clock rate.",
                    decodes);
                // Check decode-to-signal ratio
                if (decode_to_signal < 1.5) {
                    REXKRNL_ERROR(
                        "FMOD_PERSEC_RESULT: decode/sig ratio={:.2f} confirms "
                        "1:1 wake-to-decode. Stream thread wakes at {} Hz but "
                        "only decodes once per wake.",
                        decode_to_signal, signals);
                } else if (decode_to_signal > 3.0) {
                    REXKRNL_ERROR(
                        "FMOD_PERSEC_RESULT: decode/sig ratio={:.2f} - stream thread "
                        "is doing multiple decodes per wake! Additional bottleneck "
                        "is mixer/service clock at {} Hz.",
                        decode_to_signal, mixer);
                }
            }

            // Check if submitted frames match decoded frames
            if (submitted > 0 && pcm_frames > 0) {
                double sub_to_dec = (double)submitted / pcm_frames;
                if (sub_to_dec < 0.5) {
                    REXKRNL_ERROR(
                        "FMOD_PERSEC_WARNING: submitted/decoded ratio={:.2f} - "
                        "audio submission is lagging behind decode!",
                        sub_to_dec);
                }
            }

            // Underrun analysis
            if (underruns > 0) {
                REXKRNL_ERROR(
                    "FMOD_PERSEC_WARNING: {} underruns this second - "
                    "audio output is starving", underruns);
            }
        }
    }
}

inline void StartPerSecTimer() {
    if (g_timer_running.exchange(true, std::memory_order_relaxed)) return;
    g_timer_thread = std::thread(PerSecTimerLoop);
    REXKRNL_ERROR("FMOD_PERSEC: timer thread started");
}

inline void StopPerSecTimer() {
    if (!g_timer_running.exchange(false, std::memory_order_relaxed)) return;
    if (g_timer_thread.joinable()) {
        g_timer_thread.join();
    }
    REXKRNL_ERROR("FMOD_PERSEC: timer thread stopped");
}

// ============================================================================
// Write-Watch for FMOD globals
//
// We poll these addresses periodically since we can't easily hook writes
// in the recompiler. The FMOD object fields (+0x41E, +0x41D) are trickier
// since the object pointer is dynamic — we detect it from the FMOD_GATE_FAST
// log messages, but for write-watch we also poll from the sub_8220A4E8 generated
// code reads.
//
// Fixed-address globals:
//   0x829C24C8: dword counter (incremented each mixer call, reset on signal)
//   0x829C24C7: byte force-signal flag (slow path only)
//
// Dynamic-address FMOD object fields (detected at runtime):
//   [FMOD_OBJECT_PTR + 1054]: byte field_41E (fast path enable)
//   [FMOD_OBJECT_PTR + 1053]: byte field_41D (slow path condition)
// ============================================================================

struct WriteWatchState {
    // Fixed globals
    std::atomic<uint32_t> counter_829C24C8{0};
    std::atomic<uint8_t> flag_829C24C7{0};

    // FMOD object pointer (detected from generated code logs)
    std::atomic<uint32_t> fmod_object_ptr{0};

    // Last known field values (to detect changes)
    std::atomic<uint8_t> field_41E{0};
    std::atomic<uint8_t> field_41D{0};

    // Write log buffer (ring)
    struct WriteEntry {
        uint32_t address;
        uint32_t pc;        // guest LR of writer
        uint32_t old_value;
        uint32_t new_value;
        uint32_t guest_tid;
        uint32_t native_tid;
        double wall_sec;
        char what[32];
    };
    static constexpr size_t kMaxEntries = 256;
    std::mutex entries_mutex;
    std::deque<WriteEntry> entries;
};

inline WriteWatchState g_ww;

// Log a write-watch event
inline void LogWrite(uint32_t addr, uint32_t old_val, uint32_t new_val,
                     uint32_t pc, const char* what) {
    WriteWatchState::WriteEntry e{};
    e.address = addr;
    e.pc = pc;
    e.old_value = old_val;
    e.new_value = new_val;
    e.guest_tid = GetGuestTID();
    e.native_tid = GetNativeTID();
    e.wall_sec = WallSec();
    strncpy_s(e.what, what, sizeof(e.what) - 1);

    REXKRNL_ERROR(
        "FMOD_WRITE addr={:08X} what={} old={:08X} new={:08X} pc={:08X} gtid={} ntid={} t={:.3f}",
        addr, what, old_val, new_val, pc, e.guest_tid, e.native_tid, e.wall_sec);

    std::lock_guard<std::mutex> lock(g_ww.entries_mutex);
    g_ww.entries.push_back(e);
    if (g_ww.entries.size() > WriteWatchState::kMaxEntries) {
        g_ww.entries.pop_front();
    }
}

// Called from generated code (sub_8220A4E8 hooks) to report FMOD object pointer
inline void ReportFmodObject(uint32_t fmod_ptr) {
    uint32_t prev = g_ww.fmod_object_ptr.exchange(fmod_ptr, std::memory_order_relaxed);
    if (prev != fmod_ptr) {
        REXKRNL_ERROR("FMOD_WRITE: FMOD object pointer detected/changed: {:08X} -> {:08X}", prev, fmod_ptr);
    }
}

// Poll fixed-address write-watches
inline void PollFixedWriteWatches() {
    // Write-watch for 0x829C24C8 and 0x829C24C7 is done through generated
    // code hooks in sub_8220A4E8 since we can't read guest memory from a
    // host timer thread without a PPC context.
}

// Write-watch hook for the counter at 0x829C24C8
inline void WatchCounterWrite(uint32_t old_val, uint32_t new_val, uint32_t pc) {
    LogWrite(0x829C24C8, old_val, new_val, pc, "COUNTER_829C24C8");
    g_ww.counter_829C24C8.store(new_val, std::memory_order_relaxed);
}

// Write-watch hook for byte flag at 0x829C24C7
inline void WatchFlagWrite(uint32_t old_val, uint32_t new_val, uint32_t pc) {
    LogWrite(0x829C24C7, old_val, new_val, pc, "FLAG_829C24C7");
    g_ww.flag_829C24C7.store((uint8_t)new_val, std::memory_order_relaxed);
}

// Write-watch hook for FMOD object fields
inline void WatchField41EWrite(uint32_t fmod_ptr, uint32_t old_val, uint32_t new_val, uint32_t pc) {
    LogWrite(fmod_ptr + 1054, old_val, new_val, pc, "FMOD_FIELD_41E");
    g_ww.field_41E.store((uint8_t)new_val, std::memory_order_relaxed);
    ReportFmodObject(fmod_ptr);
}

inline void WatchField41DWrite(uint32_t fmod_ptr, uint32_t old_val, uint32_t new_val, uint32_t pc) {
    LogWrite(fmod_ptr + 1053, old_val, new_val, pc, "FMOD_FIELD_41D");
    g_ww.field_41D.store((uint8_t)new_val, std::memory_order_relaxed);
    ReportFmodObject(fmod_ptr);
}

// FMOD object field read logging (from generated sub_8220A4E8)
inline void LogFieldRead(const char* field_name, uint32_t fmod_ptr, uint8_t value) {
    uint32_t addr = fmod_ptr + (strcmp(field_name, "41E") == 0 ? 1054 : 1053);
    REXKRNL_ERROR("FMOD_FIELD_READ addr={:08X} field_{} value={} fmod={:08X} LR={:08X}",
                   addr, field_name, (unsigned)value, fmod_ptr, (uint32_t)GetGuestLR());
}

// ============================================================================
// Initialization / Cleanup
// ============================================================================

inline void Init() {
    StartPerSecTimer();
    REXKRNL_ERROR("FMOD_DIAG_V2: initialized per-sec timer and write-watch");
}

inline void Shutdown() {
    StopPerSecTimer();
    REXKRNL_ERROR("FMOD_DIAG_V2: shutdown");
}

}  // namespace xma_gap_diag_v2
