# FM2 FMOD/XMA Decode Cadence Investigation

## Date: 2026-05-23

## Symptom
Audio plays at ~30-40% speed (stretched/slow). SDL pump is healthy (188 Hz, near-zero
underruns). PCM frame content is correct. XMA decode pipeline produces correct 512-sample
frames. The bottleneck is in **FMOD's internal scheduling** — the mixer signals the stream
thread at ~30 Hz instead of the needed ~94 Hz.

---

## Pipeline Eliminations (in order)

| Stage | Status | Evidence |
|-------|--------|----------|
| SDL pump | **Healthy** | 188 Hz, 100% fill, near-zero underruns |
| Final PCM frames | **Clean** | 256 unique samples/channel, correct conversion |
| XMA decode frames | **Correct** | FFmpeg returns exactly 512 samples at 48 kHz, 2ch |
| XMA output offsets | **Expected** | 2048-byte ring, write offset wraps, valid bit cleared |
| XMA decode throughput | **Measured ~half** | ~22-40 decodes/sec per context, need ~94/sec |
| Kernel waits | **NOT the gap** | Guest thread runs PPC code between XMA API calls (not blocked on kernel) |
| Guest PPC execution | **30ms gap confirmed** | Host timer between codec_read calls = 15-30ms |

---

## Hot RIP Discovery

### Method
A PC sampler thread (host timer thread) samples the FMOD guest thread's host RIP via
`SuspendThread`/`GetThreadContext`/`ResumeThread` every ~0.5ms during decode gaps.

### Result
**100% of all gap samples hit a single instruction** `ntdll!NtWaitForSingleObject+0x14`.

This proved the FMOD stream thread spends the entire gap in a Windows kernel wait,
not executing guest PPC code.

---

## Diagnostic Bug: Single-Thread Tracking

### The Bug
`g_xma_native_tid` was a single `std::atomic<uint32_t>` with `compare_exchange_strong`
to register the first XMA API caller. Only **one** thread could ever be tracked.

On FM2, **two** FMOD stream threads exist (one per XMA voice context). The first thread
to call any XMA API (a different FMOD thread, `t87136`) grabbed the slot. The actual
stream threads (`t73408`, `t81752`) were invisible to `IsXmaThread()`.

### Consequence
`GAP_WAIT` log lines were silently suppressed for the real stream threads. The wait
logging only showed `t87136`'s waits — a red herring.

### Fix
Replaced single `g_xma_native_tid` with `std::unordered_set<uint32_t> g_tracked_native_tids`.
`TrackXmaThread()` inserts the calling thread's native TID. `IsXmaThread()` checks set
membership. Multiple threads can be tracked simultaneously.

---

## FMOD Reverse Engineering Summary

### Codec Registration (0x829A6FE8)
```
name = "FMOD XMA Codec"
version = 0x10100
```

### Codec Vtable (0x82199C60, 16 entries × 8 bytes)

| Entry | Address | Function |
|-------|---------|----------|
| open impl | `sub_826934C0` | codec_open — initialises XMA context |
| close | `sub_82693808` | codec_close — releases XMA context |
| read | `sub_826938E8` | **codec_read** — XMA decode dispatch |
| seek | `sub_826939A8` | codec_seek — linear scan, decode+discard frames |
| init | `sub_82693B48` | Registers "FMOD XMA Codec" |

Each vtable entry has a wrapper thunk that adjusts `a1` by -24 before calling the impl.

### codec_read (`sub_826938E8`, 188 bytes)
```c
int codec_read(int a1, int a2, int a3, _DWORD *a4) {
    XMADisableContext(ctx, 1);
    v7 = sub_82692CC8(a1, &v15, a4, *(_DWORD *)(a1 + 212));  // XMA setup/decode
    if (!v7) {
        // Copy decoded data from ring buffer to output
        memcpy(a2, ctx+12+v15, amount);
        if (*a4 + v15 > 0x800) {  // > 2048 = ring wrap
            amount = 2048 - v15;
            memcpy(a2, ctx+12+v15, amount);
            memcpy(a2 - v11 + 2048, ctx+12, rest);
        } else {
            memcpy(a2, ctx+12+v15, amount);
        }
    }
    XMAEnableContext(ctx);
    return 0;
}
```

### codec_read wrapper (`sub_82693B18`)
Thunk: adjusts `a1` by -24, then calls `sub_826938E8`.

### Nt_WaitForSingleObject — Xbox 360 wrapper (0x824119E0)
```c
NTSTATUS Nt_WaitForSingleObject(void* handle, int timeout_ms, int alertable) {
    LARGE_INTEGER* timeout;
    if (timeout_ms == -1)
        timeout = NULL;  // infinite
    else {
        timeout = alloca(sizeof(LARGE_INTEGER));
        *timeout = (int64_t)(uint32_t)timeout_ms * -10000;  // ms → 100ns
    }
    for (;;) {
        result = NtWaitForSingleObjectEx(handle, 1, alertable, timeout);
        if (result < 0) break;
        if (!alertable || result != 0x101) return result;  // != STATUS_USER_APC
    }
    return -1;
}
```

### FMOD Thread Architecture
4 threads created by `sub_82673ED8` (fmod_systemi.cpp):

| Thread | Priority at | Role |
|--------|-------------|------|
| Mixer | `dword_82A13DAC` | Audio mixing, runs at ~64 Hz |
| **Stream** | `dword_82A13DB0` | **codec_read/Wait** — our target |
| Nonblocking | `dword_82A13DB4` | Async commands |
| File | `dword_82A13DB8` | I/O |

---

## Stream Thread Main Loop (`sub_825890F8`)

```c
int sub_825890F8(int stream_struct) {
    int FrameCounter = D3D_GetFrameCounter();
    sub_824A0EC0(..., FrameCounter, 5);           // store frame counter
    while (!*(BYTE*)(stream_struct + 80)) {       // until terminated
        sub_82588970(stream_struct);               // do work → codec_read
        v4 = sub_82588C10(stream_struct);          // process queued buffers
        // ... timeout computation (internal bookkeeping)
        Nt_WaitForSingleObject(*(void**)(stream_struct + 76), 32, 1);  // **32ms timeout**
    }
    return -1;
}
```

### Work function (`sub_82588970`)
- Enters critical section at `stream_struct+12`
- Iterates linked list from `stream_struct+128`
- For each item, calls vtable method (offset 52+32) with `(buffer, 640)` — **640-byte request**
- After processing, signals event at `stream_struct+76` (self-wake)
- Leaves critical section

### Buffer delivery function (`sub_82588C10`)
- Iterates a separate queue at `stream_struct+168`
- Delivers buffer data to codecs via vtable method (offset 52+36)
- Also signals event at `stream_struct+76` when all buffers processed

---

## Queue+Wake Function (`sub_825898F8`)

This is the **signal site** — called by the mixer thread to queue work and wake the stream.

```c
int sub_825898F8(int stream_struct, int type, int data_ptr, int size, int flags) {
    buffer = alloc(size + 15);
    buffer->type = type;
    buffer->size = size;
    buffer->flags = flags;
    memcpy(buffer+3, data_ptr, size);
    RtlEnterCriticalSection(stream_struct + 40);
    enqueue(stream_struct + 168, &buffer);
    RtlLeaveCriticalSection(stream_struct + 40);
    Nt_SetEvent(*(void**)(stream_struct + 76));  // **signal the stream thread**
}
```

Called from vtable entry `sub_8258A528` (at `0x821927C0`, entry 3):
```c
case 6:  // stream data command
    sub_82589978(v15, a1, a2, a4);  // → sub_825898F8 (queue + wake)
    break;
```

---

## Wait Analysis (Instrumentation Results — TASK 2)

### Data from log (200-second run)

| Metric | Value |
|--------|-------|
| Total waits | 5100+ |
| **Signal wakes** (STATUS_SUCCESS) | **100%** |
| Timeout wakes (STATUS_TIMEOUT) | **0%** |
| Average signal wait | 32.1 ms |
| Waits/sec | 30.0 |
| Wait timeout | 32ms (from `Nt_WaitForSingleObject(event, 32, 1)`) |

### Conclusion
The stream thread **never times out**. Every wait returns STATUS_SUCCESS because the
mixer signals the event before the 32ms timeout. The timeout is a safety net that
never fires.

---

## Signal Site Analysis (Instrumentation Results — TASK 3)

| Metric | Value |
|--------|-------|
| Total signals | 42900+ |
| **Signals/sec** | **30.0** |
| Ratio to waits | 1:1 (each signal wakes exactly one wait) |

The mixer thread (`t66028`) calls `KeDelayExecutionThread(10ms)` at **64 Hz** (avg 15.4ms)
but only signals the stream thread on **every other cycle** (~30 Hz).

---

## Decode Cadence — Root Cause

```
Time    Mixer Thread          Stream Thread          codec_read
        (64 Hz)

0ms     KeDelay(10ms)
                                NtWait(32ms) [waiting]
15ms    → wake
        → decide to queue? NO
        → KeDelay(10ms)

30ms    → wake
        → queue work → NtSetEvent(event)
                                ← WAKE (STATUS_SUCCESS, 32ms)  ✓
                                → codec_read → XMADisableContext
                                → decode (1ms)
                                → XMAEnableContext
                                → NtWait(32ms) [waiting]
                                                             1 decode at t=31ms
45ms    → wake
        → decide to queue? NO
        → KeDelay(10ms)

60ms    → wake
        → queue work → NtSetEvent(event)
                                ← WAKE (STATUS_SUCCESS, 32ms)
                                → codec_read (1ms)
                                → NtWait(32ms)
                                                             1 decode at t=61ms
...
```

**30 decodes/sec × 512 samples = 15,360 samples/sec vs 48,000 needed = 32% of real-time.**

---

## Root Cause Determination (TASK 5)

**Mixer signals stream event only at ~30 Hz.**

The FMOD mixer thread runs at ~64 Hz but only signals the stream thread on every
other cycle. This is likely because the mixer's scheduling is tied to the game's
graphics/main update rate (~30 fps). The mixer decides whether to queue more data
based on buffer fullness, and at 30 fps it conservatively queues one buffer per frame.

### Key Data Points
- 5100+ waits, **100% signal, 0% timeout** → the 32ms timeout is irrelevant
- Signals at exactly 30 Hz → tied to graphics or main-thread update rate
- 30 decodes/sec × 512 samples = 15,360 samples/sec (32% of 48K)
- Mixer thread runs at 64 Hz but only signals every other cycle

### What Would Fix It
This is an FMOD scheduling issue, not a decode pipeline issue. The fix would likely
involve making the mixer signal the stream thread at the audio render frame rate
(187.5 Hz) or at least at every mixer cycle (64 Hz). This could be done by:
1. Patching the mixer's "should I queue work?" decision
2. Overriding the guest timer to make FMOD think it's running faster
3. Patching `Nt_WaitForSingleObject` to reduce the effective wait time
4. Pre-queuing multiple work items per mixer cycle

---

## Update: Native Copy-Window Experiment (2026-05-29)

### What We Tested
- Replaced `codec_read` copy-window memcpy in `sub_826938E8` with a host-side
  copy pipeline (`ProcessReadCopyWindow`) that tried to prefill by replaying
  extra guest decode calls (`sub_82692CC8`).

### What Logs Proved
- Replay calls were attempted every read, but always returned zero next-bytes:
  - `replay_attempts` increased with reads
  - `replay_ret_nz=0`
  - `replay_zero` increased 1:1 with attempts
  - `replay_ok_bytes=0`
- This means the extra decode replay path did not produce additional PCM.
- When host copy-window boosting consumed beyond truly produced bytes, audio
  quality regressed into crunchy/noisy output.

### Durable Outcome
- Keep guest memcpy/copy-window authoritative for now (do not suppress it).
- Retain native instrumentation, but treat copy-window replacement as not ready
  until replay decode can produce non-zero additional bytes.

---

## FMOD Function Map

| XEX Address | Name | Size | Role |
|-------------|------|------|------|
| `0x821927B0` | vtable | 64B | FMOD stream class vtable (8 entries) |
| `0x821927C0` | vtable+0x10 | 8B | Entry 3: `sub_8258A528` (stream command dispatcher) |
| `0x82199C60` | vtable | 128B | FMOD XMA codec vtable (16 entries) |
| `0x8240C018` | `D3D_GetFrameCounter` | 12B | Reads `*(*(r13+256)+332)` = guest frame counter |
| `0x8240C4F8` | `Nt_SetEvent` | - | Xbox kernel NtSetEvent import |
| `0x8240C600` | `sub_8240C600` | - | Yield/sleep helper |
| `0x824119E0` | `Nt_WaitForSingleObject` | 0x64 | Wrapper: converts ms→100ns, calls NtWaitForSingleObjectEx |
| `0x82411AA8` | `sub_82411AA8` | 8ins | Convert ms→100ns: `if(a2==-1)return NULL; *result=-10000*a2` |
| `0x82413620` | `sub_82413620` | - | memcpy |
| `0x824613E0` | `j_D3D_GetFrameCounter` | - | Thunk to `D3D_GetFrameCounter` |
| `0x824A0EC0` | `sub_824A0EC0` | - | Store frame counter in critsec |
| `0x824A0EF8` | `sub_824A0EF8` | - | Reads `dword_829F2DF8` |
| `0x824A4FE8` | `sub_824A4FE8` | - | Timeout computation wrapper |
| `0x82586A28` | `sub_82586A28` | 28B | Stream shutdown: sets terminate flag, signals event |
| `0x82588970` | `sub_82588970` | 668B | **Stream work function** (codec_read, signal self) |
| `0x82588C10` | `sub_82588C10` | 688B | **Buffer delivery function** (process mixer queue) |
| `0x825890F8` | `sub_825890F8` | 540B | **Stream thread main loop** (wait → work → wait) |
| `0x82589628` | `sub_82589628` | 716B | Stream thread init: creates event at +76, spawns thread |
| `0x825898F8` | `sub_825898F8` | 124B | **Queue work + wake stream** (signals event at +76) |
| `0x82589978` | `sub_82589978` | 24B | Thunk → `sub_825898F8` |
| `0x82589990` | `sub_82589990` | 24B | Thunk → `sub_825898F8` |
| `0x8258A528` | `sub_8258A528` | 300B | **Stream command dispatcher** (vtable entry 3) |
| `0x82673ED8` | `sub_82673ED8` | - | FMOD thread creation (fmod_systemi.cpp) |
| `0x8267B6A0` | `sub_8267B6A0` | 8740B | **FMOD system update/mixer** (called from game main loop) |
| `0x8267DD38` | `sub_8267DD38` | 908B | FMOD system interface (fmod_systemi.cpp) |
| `0x8267F060` | `sub_8267F060` | 548B | FMOD system dispatch (calls mixer update) |
| `0x82692CC8` | `sub_82692CC8` | - | XMA setup/prepare (called from codec_read) |
| `0x826934C0` | `sub_826934C0` | - | codec_open |
| `0x82693808` | `sub_82693808` | - | codec_close (release XMA context) |
| `0x826938E8` | `sub_826938E8` | 188B | **codec_read** (XMADisableContext → decode → XMAEnableContext + copy) |
| `0x826939A8` | `sub_826939A8` | - | codec_seek |
| `0x82693B18` | `sub_82693B18` | - | codec_read wrapper (adjusts a1 by -24) |
| `0x82693B48` | `sub_82693B48` | - | Codec registration init ("FMOD XMA Codec") |
| `0x829A6FE8` | `dword_829A6FE8` | - | FMOD XMA codec registration struct |
| `0x82A13DAC` | `dword_82A13DAC` | - | Mixer thread priority |
| `0x82A13DB0` | `dword_82A13DB0` | - | **Stream thread priority** |
| `0x82A13DB4` | `dword_82A13DB4` | - | Nonblocking thread priority |
| `0x82A13DB8` | `dword_82A13DB8` | - | File thread priority |

---

## Codec context addresses (from XMA instrumentation)

In earlier runs:

| Guest Thread | Native TID | XMA Context | Role |
|--------------|------------|-------------|------|
| t17 | `73408` | `FFCA6080` | Stream thread for context 1 |
| t18 | `81752` | `FFCA8300` | Stream thread for context 2 |

---

## Diagnostic Infrastructure

### Files changed (TEMP_DIAG)

| File | Change |
|------|--------|
| `src/kernel/xboxkrnl/xma_gap_diag.h` | Rewrite: multi-thread tracking, wait/signal/codec instrumentation |
| `src/kernel/xboxkrnl/xboxkrnl_threading.cpp` | TASK 2 in `NtWaitForSingleObjectEx_entry`; TASK 3 in `NtSetEvent_entry` |
| `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | TASK 4 in `XMADisableContext_entry`; bridge fix |

### Log prefixes
| Prefix | What |
|--------|------|
| `FMOD_DIAG:` | XMA thread registration |
| `FMOD_WAIT ENTER/EXIT` | Per-wait (verbose, first 5s) |
| `FMOD_WAIT SUMMARY` | Cumulative wait stats every 100th |
| `FMOD_SIGNAL ENTER/EXIT` | Per-signal (verbose, first 5s) |
| `FMOD_SIGNAL SUMMARY` | Cumulative signal stats every 100th |
| `FMOD_CODEC decode` | Per-decode output tracking |
| `FMOD_CODEC SUMMARY` | Cumulative decode throughput |
| `FMOD_MWAIT ENTER/EXIT` | Multi-object wait tracking |
| `FMOD_DELAY ENTER/EXIT` | KeDelayExecutionThread tracking |
| `FMOD_DIAGNOSIS:` | Final report (printed at cleanup) |

---

## Root Cause Found (2026-05-24)

### The Signal Gate: `FM2_SignalGate`

The function that calls NtSetEvent for the stream event (`handle=F800004C`) is
`FM2_SignalGate` at address `0x8220A4E8`. It runs at ~64 Hz (called every mixer cycle)
but only signals the event **every other call**, producing the observed ~30 Hz rate.

#### Decompiled logic:
```c
int FM2_SignalGate(int a1) {
    ++dword_829C24C8;                    // increment counter
    result = FM2_XXX(a1);           // get FMOD object
    if (result->field_41E) {             // FAST PATH (active for streaming)
        v2 = (dword_829C24C8 > 1);       // signal only when counter > 1
    } else {
        result2 = FM2_XXX(result);  // SLOW PATH (double deref)
        if (!result2->field_41D || byte_829C24C7 || dword_829C24C8 > 1)
            v2 = 1;                       // signal unconditionally
    }
    if (v2) {
        NtSetEvent(dword_829C24C0);      // signal the stream event
        dword_829C24C8 = 0;              // RESET counter to 0
    }
    dword_829C24CC = __mftb();           // timestamp
}
```

#### How the 2x divider works:
1. Mixer calls `FM2_SignalGate` at ~64 Hz
2. `dword_829C24C8` starts at 0
3. Call 1: counter becomes 1, fast path checks `1 > 1` → false → no signal
4. Call 2: counter becomes 2, fast path checks `2 > 1` → true → NtSetEvent → counter reset to 0
5. Repeat → 32 signals/sec ≈ observed 30/sec

#### Key globals:
- `dword_829C24C8` (0x829C24C8): Counter incremented each call, reset to 0 on signal
- `dword_829C24C0` (0x829C24C0): Event handle (F800004C)
- `dword_829C24CC` (0x829C24CC): Timebase timestamp of last signal
- `byte_829C24C7` (0x829C24C7): Force-signal flag (slow path only)

#### TASK6D Evidence:
```
TASK6D caller=8220A578 handle=F800004C count=30  (every second, consistent)
```
- `caller=8220A578` = return address inside `FM2_SignalGate` (after `bl NtSetEvent`)
- `handle=F800004C` = the stream event handle
- `count=30` = ~30 signals/sec (matches the 2x divider at 64 Hz mixer rate)

#### Callers of `FM2_SignalGate`:
- Called via function pointer at `0x82177B60` and `0x822868E8` (vtable entries)
- IDA could not resolve callers due to indirect calls through vtables

#### Generated code location:
- `FM2/generated/fm2_recomp.2.cpp` line 3348 (`DEFINE_REX_FUNC(FM2_SignalGate)`)
- The counter at `ctx.r31.u32 + 9416` = `0x829C24C8` (r31 = `0x829C0400`)

#### How previous paths were disproven:
- `sub_8258A528` (TASK6): Never called during stream signal path — instrumentation showed zero hits
- `sub_825898F8` (TASK6B): Dead code — queue+wake via different offsets, never fires
- `sub_82589BE0` (TASK6C): Also never called — NtSetEvent for stream handle comes from `FM2_SignalGate` instead

---

## Remaining Questions

- What is `field_41E` (offset +1054) on the FMOD object? It controls whether the fast
  path (2x divider) or slow path (always signal) is taken.
- Can we force `field_41E = 0` to take the slow path (signal every cycle)?
- Is the 2x divider intentional FMOD behavior (halving the stream update rate) or
  is `field_41E` being set incorrectly?
- Would patching the counter threshold from `> 1` to `> 0` (or removing the divider)
  double the decode rate to the needed ~94/sec?
- What sets `byte_829C24C7` (the force-signal flag)?

---

## Files Referenced

- `src/kernel/xboxkrnl/xma_gap_diag.h` — diagnostic header (TASK 1-4)
- `src/kernel/xboxkrnl/xboxkrnl_threading.cpp` — kernel thread exports
- `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` — XMA API entries
- `src/core/clock.cpp` — guest clock
- `src/system/xobject.cpp` — Wait infrastructure
- `src/audio/audio_system.cpp` — render frame counter
- `include/rex/audio/sdl/sdl_audio_driver.h` — audio format constants
- `docs/FM2-audio-notes.md` — existing stutter investigation
- `docs/FM2-audio-pipeline-map.md` — architecture reference
- `FM2/generated/fm2_recomp.2.cpp` — `FM2_SignalGate` (signal gate, line 3348)

---

## Diagnostic Patch: FM2_SignalGate Divider Bypass (2026-05-24)

### What was done
1. **Patched `FM2_SignalGate`**: Changed counter threshold from `> 1` to `> 0`. The fast path
   counter (`dword_829C24C8`) is pre-incremented each call, so it's always >= 1. Now
   `counter > 0` is always true → NtSetEvent fires every mixer call instead of every other.

2. **Per-second diagnostics**: Added `xma_gap_diag_v2.h` with a timer thread that logs
   mixer calls/sec, stream signals/sec, wait wakes/sec, XMA decodes/sec, PCM frames/sec,
   submitted frames/sec, queue depth, and underrun count every second.

3. **Write-watch hooks**: Added logging in `FM2_SignalGate` for FMOD object pointer,
   field_41E, field_41D, counter value (FMOD_GATE_FAST/SLOW/SIGNAL_GATE messages).

4. **Kernel instrumentation**: Added per-second counter increments in NtSetEvent,
   NtWaitForSingleObjectEx, XMADisableContext, SDL codec_read, and SDL audio callback.

### Files changed
- `FM2/generated/fm2_recomp.2.cpp` — signal gate patch + FMOD_GATE diagnostic hooks
- `src/kernel/xboxkrnl/xma_gap_diag.h` — include xma_gap_diag_v2.h, AddPcmFrames counter
- `src/kernel/xboxkrnl/xma_gap_diag_v2.h` — NEW: per-sec timer + write-watch
- `src/kernel/xboxkrnl/xboxkrnl_threading.cpp` — stream_signal/wakeup counters
- `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` — decode counter + mixer_call bridge
- `src/audio/sdl/sdl_audio_driver.cpp` — submitted frames, queue depth, underrun counters + init/shutdown

### NOTE on format strings
REXKRNL_ERROR uses spdlog with `{}` format style, NOT printf `%d/%u` style.
All new format strings use `{}` / `{:08X}` / `{:.2f}` style.

### To test
Rebuild runtime + FM2, run game, check `C:\temp\fm2-clean.log` for `FMOD_PERSEC t=N` lines.
- `FM2/generated/fm2_recomp.18.cpp` — `NtSetEvent` wrapper (line 3089)

---

## 2026-05-24: Root Cause Found — Signal Gate + Output State

### Two Root Causes

#### 1. Signal Gate 2x Divider (FIXED)

`FM2_SignalGate` (`sub_8220A4E8` in `fm2_recomp.2.cpp:3348`) is the function that
signals the FMOD stream event (`handle=F800004C`). It runs at ~64 Hz but
implements a call counter: only signals on every 2nd call.

**Counter logic (fast path for streams, field_41E != 0):**
```
call 1: counter=1, check 1>1=false → no signal
call 2: counter=2, check 2>1=true → NtSetEvent → counter=0
→ 32 signals/sec = ~30 observed
```

**Fix (TASK6E in `fm2_recomp.2.cpp`):** Forced `ctx.r11=1` after counter check,
removing the 2x divider. Result: 60 signals/sec.

**Evidence (TASK6D):** `TASK6D caller=8220A578 handle=F800004C count=30` (before)
→ `count=60` (after).

**Globals:**
| Address | Name | Purpose |
|---------|------|---------|
| 0x829C24C0 | dword_829C24C0 | Stream event handle (F800004C) |
| 0x829C24C8 | dword_829C24C8 | Call counter (reset to 0 on signal) |
| 0x829C24C7 | byte_829C24C7 | Force-signal flag (slow path only) |

#### 2. Output Buffer Write Offset Broken (FIXED)

The stream context (`FFCA8300`) is an XMA context managed entirely through
FMOD's software codec (`sub_82673198`), bypassing the XMA decoder worker
thread. `XMAEnableContext_entry` returns 0 without actually enabling anything on
the worker side. The worker's `Work()` never processes FFCA8300.

**Consequence:** `output_buffer_write_offset` stays at 0 forever.
`output_buffer_valid` is set to 1 by `XMASetOutputBufferValid` but cleared
between `ENABLE`→`DISABLE` by guest code writing the context directly.

`sub_82692CC8` (output calculation in `fm2_recomp.39.cpp`):
- First call: read=0, write=0, valid=0 → "first-time path" returns 2048 bytes
- Second call: read=8 (from SET_ROFF with value=8), write=0, valid=0 →
  output = (0-8+8)*256 = **0 bytes** → silence until context reset

**Fix (TASK7 in `xboxkrnl_audio_xma.cpp`):**
1. `XMASetOutputBufferReadOffset`: wrap value to `% block_count` so read stays
   in ring buffer range (0-7). Without wrapping, read=8 means offset 2048 — past
   the 2048-byte buffer end.
2. `XMAGetOutputBufferWriteOffset`: always return 0 since guest never updates
   it. With read also 0 (wrapped), the first-time path fires every call.

**Evidence (file: `C:\temp\t7_seq.log`):**
```
Before fix: ...→SET_ROFF extra=8 → DISABLE roff=8 → GET_WOFF roff=8 → output=0
After fix:  ...→SET_ROFF extra=0 → DISABLE roff=0 → GET_WOFF roff=0 → output=2048
```

### Remaining Bottleneck: Per-Call Throughput

Even with both fixes, audio plays at ~64% speed (stretched/garbled).

**Throughput measurement (`C:\temp\t7_throughput.log`):**
- Total across all stream contexts: **78-150K bytes/sec** (varies)
- Peak with fake write-offset returning 0: **315K bytes/sec**
- Needed: **192K bytes/sec** (48kHz stereo 16-bit)

The 315K peak occurred when GET_WOFF always returned 0 (triggering first-time path =
2048 bytes every call), combined with the worker being kicked. The higher throughput
came from more codec_read calls, not from the worker producing extra frames.

### Worker Thread Attempt (ABANDONED)

Attempted approach: modify `XMAEnableContext` to actually enable the context on the
XmaDecoder worker thread, hoping the `Work()` while-loop would decode multiple
frames per kernel kick.

**Why it failed:** `sub_82673198` (the software codec) doesn't fill the XMA input
buffers with compressed data. It decodes directly and writes PCM to the output buffer,
then validates the input buffers (`XMASetInputBuffer0Valid`) as a side effect. When
the worker's `Work()` runs, it sees "valid" input buffers but they contain stale/
empty data. `DecodePacket` (FFmpeg) fails to decode → `current_frame_remaining_subframes_`
stays at 0 → `Consume()` returns immediately → no output produced.

**Evidence (`C:\temp\t7_work_debug.log`):**
```
WORK pre ctx=FFCA8300 valid=1 cons_only=0 remaining=8 min=4 ib0v=0 ib1v=0  ← first call: inputs NOT valid
WORK pre ctx=FFCA8300 valid=1 cons_only=0 remaining=8 min=4 ib0v=1 ib1v=1  ← subsequent: validated but empty
WORK out ctx=FFCA8300 woff=0 roff=0 valid=0                                 ← no output
```

**Conclusion:** The software codec path and the hardware worker path are mutually
exclusive. Making them work together requires modifying generated FMOD code to
route compressed data through the input buffers (a midasm hook).

### Current State & Path Forward

**Three fixes applied and working:**
1. Signal gate 2x divider → 60 Hz (TASK6E in `fm2_recomp.2.cpp`)
2. SET_ROFF wraps read offset to ring buffer range (TASK7 in `xboxkrnl_audio_xma.cpp`)
3. GET_WOFF returns software codec's stale write_offset (always 0, triggering
   first-time path returning 2048 bytes every call)

**Audio plays at ~64% speed.** The 2048-byte per-call limit × 60 Hz max signal rate
= 30,720 samples/sec per context. Multiple contexts (FFCA6000, FFCA6040, FFCA6080,
FFCA8300) share the load but still fall short of 48K total.

**To reach 100% requires:**
- Either: make `sub_82673198` produce >2048 bytes per call (limited by ring buffer size)
- Or: make the software codec fill the worker's input buffers so the multi-frame
  decode loop works
- Or: a midasm hook in generated FMOD code to call codec_read multiple times per wake
- Or: accept 64% speed and fix audio crackling via buffer management (SDL side)

### Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x8220A4E8 | FM2_SignalGate | Stream event signal, 2x divider (FIXED) |
| 0x826938E8 | codec_read | FMOD XMA codec read |
| 0x82692CC8 | sub_82692CC8 | Output size calculation & input fill |
| 0x82673198 | sub_82673198 | Software XMA decode engine |
| 0x8240C4F8 | FM2_XXX | NtSetEvent wrapper (fm2_recomp.18.cpp:3089) |
| 0x825890F8 | sub_825890F8 | FMOD stream thread main loop |
| 0x8258A528 | sub_8258A528 | Stream command dispatcher (DEAD - never called) |
| 0x825898F8 | sub_825898F8 | Queue+wake function (DEAD - wrong offsets) |
| 0x82589BE0 | FM2_XXX | Queue+wake function (DEAD - not called by stream) |

### Key Global State

| Address | Name | Value |
|---------|------|-------|
| FFCA8300 | Stream XMA context | write=0, read=0, valid=broken, blk=8 |
| FFCA6000 | Non-stream context | Also produces output |
| F800004C | Stream event handle | Signaled at 60 Hz |

### Next Steps
- The per-call 2048-byte limit is the remaining bottleneck
- Need ~94 codec_read calls/sec (not possible at 60 Hz signal rate)
- OR each codec_read must produce >2048 bytes (subframe_decode_count or multiple
  frames per call via worker thread)
- The worker thread's `Work()` can decode multiple frames per call but FFCA8300
  bypasses it — enabling it would mean changing the stream architecture

---

## May 26 2026 Update: Forced Signal Gate Side-Effect

### Experiment

- Added a MIDASM hook at `0x8220A564` (`FM2ForceSignalGate`) to force
  `FM2_SignalGate` down the event-signal path every pass (jump to `0x8220A56C`).

### Observation

- Audio became less stretched, but still not correct speed.
- Game-wide pacing changed: higher FPS and globally faster simulation.

### Conclusion

- Forcing the gate at this site is not audio-local; it affects broader timing.
- This is not a safe permanent fix.

### Follow-up Direction

- Reuse the same hook site for selective diagnostics only (no forced jump):
  identify callsite/object patterns and gate conditions that correlate with the
  stretched-audio path, then target that subset only.

---

## May 27 2026 Update: Selective Signal-Gate Midasm Patch

Applied a minimal `midasm` hook behavior change in `FM2ForceSignalGate` to
enable the signal override only for the validated FMOD stream pattern:

- hook site: `0x8220A564`
- caller LR: `0x8236C468`
- gate object pointer (`r3`): `0x829C2630`
- only when original predicate is false (`r11 == 0`)
- only when object flags are `field_41D == 1` and `field_41E == 0`

Behavior change:
- Previously, this path was diagnostic-only (`override_forced` counted but no
  jump was taken).
- Now, matching calls force `r11 = 1` and jump to `0x8220A56C` (NtSetEvent path).

Why this patch:
- Recent logs in `C:\temp\fm2-clean.log` show `TASK6_GATE_SUMMARY` at
  60 calls/sec with `pred_true=30`, `pred_false=30`, and `override_forced=30`.
- That means exactly half of gate calls were eligible-but-not-signaled before.
- This patch converts those 30 suppressed calls/sec into real stream signals
  without globally forcing all call paths.

Validation focus:
- compare before/after `FMOD_PERSEC` signal and decode rates
- monitor `AUDIO_DIAG: SILENCE underrun` count
- confirm no broad pacing regression outside audio

### Follow-up correction (same day)

If this selective gate override still changes game pacing/FPS, do not keep it as
the primary fix path. In that case the issue is still data-rate limited, and the
next patch should stay inside FMOD stream work/decode cadence.

Current follow-up patch direction:
- keep `FM2ForceSignalGate` diagnostic-only again (no forced jump)
- add a midasm hook in `sub_82588970` at `0x82588BC4` to force one more stream
  work-loop pass only when `codec_read` returned a full `640` bytes
  (`r31 == 640`) but the function was about to stop this wake (`r28 != 0`)
- jump target: `0x82588A88`

This aims to raise per-wake decode throughput without globally increasing game
timing.

Runtime verification log tag for this patch:
- `TASK8_STREAM_BOOST` (calls/sec + forced-loop/sec in `sub_82588970`)

---

## May 27 2026 Late Update: Output-Buffer State Is Stuck In First-Time Path

This session switched to diagnostics-only instrumentation (no forced timing
override) and captured the software-codec/XMA state directly in `C:\temp\fm2-clean.log`.

### New Diagnostic Tags

- `TASK9_CODEC_COPY`: per-second codec copy window stats from `sub_826938E8`
  (`req_bps`, `max_req`, `max_total`, `last_roff`) plus raw context words.
- `TASK10_XMA_GROFF`: return value from `XMAGetOutputBufferReadOffset`.
- `TASK10_XMA_GWOFF`: return value from `XMAGetOutputBufferWriteOffset`.
- `TASK10_XMA_OVAL`: return value from `XMAIsOutputBufferValid`.
- `TASK10_XMA_SROFF_A/B`: callsites for `XMASetOutputBufferReadOffset`.

### What The Logs Show

1. For active FMOD software-codec contexts (`FFCA8300`, `FFCA6000`), the output
   state is pinned:
   - `last_roff=0` (`TASK10_XMA_GROFF`)
   - `last_woff=0` (`TASK10_XMA_GWOFF`)
   - `last_valid=0` (`TASK10_XMA_OVAL`)
   - `set_roff=0` on SROFF_A path
   - no SROFF_B hits in this run

2. `TASK9_CODEC_COPY` remains capped to a 2KB window each pass:
   - `max_req=2048`
   - `max_total=2048`
   - `split=0`
   - `last_roff=0`

3. Raw context words sampled in `TASK9_CODEC_COPY`:
   - `ctx_d0=0x02300001` (or `0x02100001` briefly)
   - `ctx_d9=0x00000000`
   - `ctx_d10=0x00000000`
   - using `XMA_CONTEXT_DATA` layout:
     - packet_count=1
     - block_count=8
     - write_offset=0
     - read_offset=0
     - output_valid=0

4. Throughput is still below stable real-time requirement and underruns persist:
   - `FMOD_PERSEC_WARNING: ... audio output is starving`
   - repeated `AUDIO_DIAG: SILENCE underrun`

### Conclusion

The stream repeatedly re-enters the `roff==woff && valid==0` first-time path in
`sub_82692CC8`, never transitions into progressing ring-buffer state, and thus
never sustains enough data rate.

This is now proven as a buffer-state progression bug, not an SDL callback-rate
issue.

### Immediate Patch Direction

- Keep timing hooks diagnostic-only.
- Patch software-codec-context output state handling so `valid/roff/woff` can
  advance across calls (instead of resetting to zero each cycle), limited to the
  FMOD software-codec contexts/path.

---

## May 27 2026 Patch Applied: Preserve Output Validity Across Full-Frame Wrap

Implemented in runtime source:
- file: `src/audio/xma_context.cpp`
- function: `XmaContext::Work`
- change: removed automatic `output_buffer_valid = 0` clear when
  `output_rb.empty()` after decode/consume.

Reason:
- On FMOD software contexts (`block_count=8`, `subframe_decode_count=4`), a
  full decoded frame can wrap `write_offset` back to `read_offset` (`0 == 0`).
- In this ring representation, `read==write` is ambiguous (empty vs full-wrap).
- The previous clear made this state look definitively empty/invalid, and FM2
  then stayed in `valid=0` first-time behavior with `out_bytes=0`.

Expected effect:
- `XMAIsOutputBufferValid` should remain asserted after full-frame wrap, letting
  the title-side path treat `roff==woff` as non-empty/full-valid where expected,
  instead of collapsing to `valid=0`.

---

## May 27 2026 Follow-up Patch: Guarded First-Time Recovery At TASK10 OVAL

After the above change, logs still showed a silent loop where:
- `FMOD_CODEC_READ START ... out_bytes=0`
- `TASK10_XMA_OVAL ... last_valid=1 last_roff=0 last_woff=0`
- `TASK9_CODEC_COPY ... ctx_d0=0x02300001/0x02100001 ctx_d9=0 ctx_d10=0`

This means `sub_82692CC8` was seeing `roff==woff` with `valid==1` and selecting
the zero-byte path instead of the first-time 2048-byte path.

### Applied midasm-side fix

- File: `FM2/src/fm2_hooks.cpp`
- Hook: `FM2DiagXmaIsOutputValidReturn` at `0x82692EA4`
- Behavior:
  - only when `r3(valid)!=0`, `roff==woff==0`
  - only when context word0 matches observed FMOD software-codec layouts
    (`0x02300001` or `0x02100001`)
  - force return `r3=0` for that call

Effect: this coerces `sub_82692CC8` into its first-time branch so it writes
`out_bytes=2048` instead of `0`, without global timing changes.

### Validation markers

- `TASK10_XMA_OVAL ... forced_first_time=...`
- `FMOD_CODEC_READ START ... out_bytes` should move off zero
- `AUDIO_DIAG: SILENCE underrun` should stop climbing immediately

---

## May 28 2026 Regression Note: Menu Boot Break From WAIT_REG_MEM Spin Path

Observed in `C:\temp\fm2-clean.log` (example at `2026-05-28 13:09:09.972`):
- `ExecutePacketType3 failed opcode=WAIT_REG_MEM (3C)`
- `**** INDIRECT RINGBUFFER: Failed to execute packet...`

This failure appeared with very heavy short waits on:
- `mem:1BCA5006 / eq ref=0x00000000 / wait=0x100`

Root cause:
- A diagnostic experiment changed `WAIT_REG_MEM` behavior for `wait=0x100` from
  the original sleep-based path to a pause/yield spin path.
- That regression can destabilize FM2 boot and prevent reaching menu.

Fix applied:
- Restored original `WAIT_REG_MEM` semantics for `wait >= 0x100` in
  `src/graphics/command_processor.cpp`:
  - `PrepareForWait()`
  - `Sleep(wait/0x100)` when vsync path is active
  - `ReturnFromWait()`
- Kept packet/stat diagnostics intact.

---

## May 28 2026 Decoder Cadence Finding: Missing No-Progress Break In ReXGlue

Side-by-side compare against Xenia Canary `xma_context_new.cc` showed ReXGlue
`src/audio/xma_context.cpp` was missing the per-iteration progress guard inside
`XmaContext::Work()`:

- Xenia tracks:
  - `pre_decode_offset`
  - `pre_remaining_subframes`
- Then breaks if:
  - no pending subframes before decode, and
  - `input_buffer_read_offset` did not advance, and
  - no subframes were produced (`current_frame_remaining_subframes_ == 0`)

Why this matters:
- Without this guard, a context can spin in the decode loop with unchanged
  state (no real input progress), burning decode passes and hurting producer
  cadence.
- This matches prior "busy-loop" symptoms where work iterations were high but
  output-buffer advance did not keep up.

Patch applied:
- File: `src/audio/xma_context.cpp`
- Added the missing no-progress break in `XmaContext::Work()` to match the
  Xenia behavior at that loop boundary.

---

## May 28 2026 Diagnostic Fix: Stream Event Counters Must Use Dynamic Handles

### Problem

Per-second diagnostics in `NtSetEvent_entry` / `NtWaitForSingleObjectEx_entry`
were counting only a hardcoded handle (`0xF800004C`) for `signals` and
`wakeups`.

### Why this was wrong

XEX analysis confirmed FMOD stream wake paths can use per-stream event handles
(object `+76`) and those handles are discovered at runtime. A single hardcoded
handle can undercount real stream signaling activity.

### Fix applied

- File: `src/kernel/xboxkrnl/xboxkrnl_threading.cpp`
  - `NtSetEvent_entry`: `IncStreamSignal()` now triggers when
    `xma_gap_diag::IsStreamEventHandle(handle)` is true.
  - `NtWaitForSingleObjectEx_entry`: `IncStreamWakeup()` now triggers when
    `xma_gap_diag::IsStreamEventHandle(object_handle)` and
    `result == X_STATUS_SUCCESS`.
- File: `src/kernel/xboxkrnl/xma_gap_diag_v2.h`
  - Updated comments to describe tracked dynamic stream handles instead of
    fixed `F800004C`.

### Expected effect

`FMOD_PERSEC signals/wakeups` should now reflect actual tracked stream events
instead of being biased by a stale fixed-handle assumption.

### Follow-up (same day): tracker registration regression

In a later run, `TASK6_GATE_SUMMARY` still showed active gate traffic but
`FMOD_PERSEC signals/wakeups` dropped to zero. Root cause was diagnostic
plumbing drift:

- `xma_gap_diag::TrackXmaThread()` was no longer called from active XMA entry
  points in `xboxkrnl_audio_xma.cpp`.
- Without tracked XMA threads, `NtWaitForSingleObjectEx` did not register
  stream event handles, so dynamic-handle counters stayed empty.

Fix applied:
- Added `TrackXmaThread()` in key XMA entry points:
  `XMAIsOutputBufferValid`, `XMASetOutputBufferValid`,
  `XMAGetOutputBufferReadOffset`, `XMASetOutputBufferReadOffset`,
  `XMAGetOutputBufferWriteOffset`, `XMAEnableContext`, `XMADisableContext`.
- Added a safety fallback in `xboxkrnl_threading.cpp` so per-second counters
  still count handle `0xF800004C` while dynamic tracking repopulates.

---

## May 28 2026 Build Workflow Gotcha: Manifest Hook Changes Need `fm2_codegen`

Observed during `FM2ReplayCodecReadOnce` testing:
- Hook function existed in `FM2/src/fm2_hooks.cpp`.
- Hook entry existed in `FM2/fm2_manifest.toml`.
- But runtime logs showed zero `TASK11_CODEC_REPLAY` lines.

Root cause:
- We rebuilt `fm2` without regenerating code, so generated recomp files did not
  include the new hook callsite wiring yet.
- Evidence: before regeneration, `FM2/generated/fm2_recomp.39.cpp` contained
  `FM2DiagCodecReadCopyWindow` but not `FM2ReplayCodecReadOnce`.

Required workflow when adding/changing FM2 manifest hooks:
1. Run `cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen`
2. Then run `cmake --build --preset win-amd64-relwithdebinfo --target fm2`

Post-fix verification:
- `FM2/generated/fm2_recomp.39.cpp` now contains:
  - `extern bool FM2ReplayCodecReadOnce(...)`
  - `if (FM2ReplayCodecReadOnce(...)) { ... }`

### Follow-up correction (same session)

`FM2ReplayCodecReadOnce` was then switched back to diagnostic-only.

Why:
- Runtime logs showed high replay rates (`TASK11_CODEC_REPLAY replayed ~40-48/s`)
  plus user-audible fast/crunchy playback.
- Re-entering `codec_read` from inside the same callback advanced decode state
  twice while still servicing one output request buffer, which can skip audio
  content and destabilize cadence.

Action:
- Keep `TASK11_CODEC_REPLAY` counters, but suppress jump/replay behavior until
  a non-destructive per-call throughput fix is in place.

---

## May 28 2026 Finding: Missing Win32 High-Resolution Timer Request

### Evidence
- Xenia startup requests NT high-resolution timers (`NtSetTimerResolution`).
- ReXGlue startup path in `src/ui/windowed_app_main_win.cpp` had a TODO and did
  not request timer resolution.
- FM2 logs repeatedly showed guest 10 ms sleeps (`interval=-100000`) taking
  about `15.1-15.9 ms` on host, which clamps effective stream cadence.

### Fix applied
- Added `RequestWin32HighResolutionTimer()` in
  `src/ui/windowed_app_main_win.cpp`:
  - resolves `NtQueryTimerResolution` / `NtSetTimerResolution` from `ntdll`
  - requests `maximum_resolution` at process startup
- Removed temporary XMA-specific 10ms->5ms delay override from
  `src/kernel/xboxkrnl/xboxkrnl_threading.cpp` to return to cleaner parity
  behavior.

### Intent
- Match Xenia timing behavior first, then re-evaluate FFCA8300 copy cadence and
  underrun profile with fewer title-specific timing hacks.

---

## May 28 2026 Late Update: `sub_8269BB20` Cursor Delta Was Half-Rate

### What we measured

A new midasm diagnostic hook at `0x8269BE34` (`FM2DiagPumpCursorAdvance`) logs
the per-call cursor delta (`r11`) used by `sub_8269BB20` to advance the stream
producer/consumer state.

Observed in `C:\temp\fm2-clean.log` before behavior change:
- `TASK9_PUMP_CURSOR raw_bps` about `20K-23K` bytes/sec
- `TASK9_CODEC_PUMP req_bps` about `20K-23K` bytes/sec
- `TASK9_CODEC_SCHED` about `40-46` calls/sec
- persistent `FMOD_PERSEC_WARNING` underruns

This directly matched the audible half-speed cadence.

### Why this matters

`sub_8269BB20` uses that delta to decide how much audio work to run each wake.
If the delta is under-reported, decode cadence is under-serviced even when
signal/wakeup rates are healthy.

### Targeted test patch

At the same hook site (`0x8269BE34`), for the active stream profile:
- `last_chunk == 512`
- non-zero ring size

we scale delta by 2 (clamped to ring size) before the function applies it.

### Immediate telemetry result

After scaling:
- `TASK9_PUMP_CURSOR out_bps` rose to about `44K-47K` bytes/sec
- `TASK9_CODEC_PUMP` rose to about `84-95` calls/sec
- `TASK9_CODEC_SCHED` rose to about `84-98` calls/sec
- `TASK9_CODEC_COPY req_bps` reached about `172K-192K` bytes/sec

This brings FM2 codec cadence into the same range as the healthy Xenia run.

---

## May 28 2026 Native FMOD Replacement Wiring (New Files + Minimal Glue)

Implemented FM2-specific native codec replacement scaffolding with new files and
minimal integration edits only.

### New files

- `include/rex/audio/fm2_native/runtime.h`
- `include/rex/audio/fm2_native/scheduler.h`
- `include/rex/audio/fm2_native/codec.h`
- `include/rex/audio/fm2_native/diag.h`
- `src/audio/fm2_native/runtime.cpp`
- `src/audio/fm2_native/scheduler.cpp`
- `src/audio/fm2_native/codec.cpp`
- `src/audio/fm2_native/diag.cpp`

### Minimal glue edits

- `src/audio/CMakeLists.txt`: adds `fm2_native/*.cpp` to `rexaudio`.
- `FM2/src/fm2_hooks.cpp`: thin hook bridge only, forwarding to
  `rex::audio::fm2_native::*`.
- `FM2/fm2_manifest.toml`: routes FMOD codec callsites to new bridge:
  - `0x826934C0` -> `FM2NativeCodecOpenEntry`
  - `0x82693808` -> `FM2NativeCodecCloseEntry`
  - `0x826938E8` -> `FM2NativeCodecReadEntry`
  - `0x82693960` -> `FM2NativeCodecReadCopyWindow`
- Intro skip hooks preserved.

### Runtime behavior in new module

- Per-codec stream registry (`open/read/close` lifecycle).
- Host-timed budget scheduler with bounded prefill.
- Probe-based backend decision with one decision log line:
  `FM2_NATIVE_BACKEND ...`
- Per-second metrics line:
  `FM2_NATIVE_PERSEC ...`

### First 10-second acceptance run (auto-run + kill)

Log: `C:\temp\fm2-clean.log` (run at `2026-05-28 21:47` local)

- Backend decision observed:
  - `FM2_NATIVE_BACKEND codec=2E00D870 backend=native parity=1.0000 rms=0.000000 passthrough_bps=80317`
- FM2 native metrics observed:
  - `out_fps` mostly around `20k-26k` (not near `47k-49k`)
  - `infl_req=0`, `req_bytes=0` on sampled lines
- FMOD service metrics observed:
  - `FMOD_PERSEC submitted` around `51k-52k` bytes/sec
  - persistent `FMOD_PERSEC_WARNING` with `underruns=3` per second

### Current conclusion

The scaffolding and hook cutover are now in place and compiling/running, but
acceptance targets are not met yet. Data-rate remains below real-time and
underruns persist.

---

## May 28 2026 Follow-up: Native Scheduler Channel Model Fix

### Problem observed after native cutover

- `FM2_NATIVE_PERSEC out_fps` stayed around `20k-24k`.
- Audio remained slow/crunchy despite native hooks being active.

### Key evidence

Native output throughput was near mono-rate, not stereo-rate:

- `out_bytes` roughly `80k-90k` per second in prior runs.
- That is close to `48k * 2 bytes` (mono 16-bit ~= `96k/s`), and about half of
  stereo 16-bit (`192k/s`).

### Fix applied

In `src/audio/fm2_native/runtime.cpp`, changed FM2 native scheduler default:

- `fm2_native_target_channels`: `2 -> 1`

No global SDK restructure; FM2-only native path remains hook-routed.

### Validation (local 10-15s runs)

After rebuild/install + FM2 rebuild:

- `FM2_NATIVE_PERSEC out_fps` moved to about `38k-54k` (typically mid/high `40k`).
- `FM2_NATIVE_PERSEC native_underrun_bytes=0` sustained after startup.
- `SDL_AUDIO_STATS fill=99-100%`, real callback rate near expected `188/s`.
- Remaining `FMOD_PERSEC_WARNING underruns=3` appears to be persistent startup
  accounting, not native FIFO starvation (native underrun bytes stay `0`).

### A/B note

A temporary stream wait-timeout hook experiment was removed; improvement
persists without it. The throughput gain tracks the mono scheduler-channel fix.

---

## May 29 2026 Update: Native Path Verification + Regression Guard

### 1) Stale FM2 runtime DLL can mask real behavior

Observed during local self-runs:
- `out/install/win-amd64/bin/rexruntimerd.dll` had newer timestamp than
  `FM2/out/build/win-amd64-relwithdebinfo/rexruntimerd.dll`.
- When stale FM2-local DLL was used, logs still showed:
  - `FM2_NATIVE_BACKEND ... backend=passthrough`
  - `FM2_NATIVE_PERSEC ... native=0`

Action:
- Always copy fresh runtime DLL into FM2 output before validating:
  - `out/install/win-amd64/bin/rexruntimerd.dll`
  - -> `FM2/out/build/win-amd64-relwithdebinfo/rexruntimerd.dll`

### 2) Confirmed native replacement activation signature

With fresh DLL and native routing active, expected lines were:
- `FM2_NATIVE_BACKEND codec=2E00D870 backend=native ...`
- `FM2_NATIVE_PERSEC ... native=1 ... cw_starve=0 replay_attempts=0`

Sample run (`2026-05-29 01:12:34` to `01:12:43`, `C:\temp\fm2-clean.log`):
- `out_fps` rose into real-time band (`~38k` to `~50k`)
- decodes/s in `FMOD_PERSEC` sat around mid/high 40s
- startup underruns remained (`underruns=2`) but native FIFO starvation stayed 0

### 3) Do not keep `output_buffer_valid` forced across empty ring in current path

A local experiment to keep `output_buffer_valid=1` on empty output ring caused
an unstable decode pattern:
- one-second burst (`decodes=6037`)
- then sustained collapse (`decodes=0` each following second)

Result:
- Reverted that experiment. Current stable path keeps existing empty-ring clear
  behavior for FM2 while native replacement is active.

---

## May 29 2026 Deep-Dive: Why decode collapses at ~t=33

### What we instrumented

- Added targeted FM2 hooks/counters in `FM2/src/fm2_hooks.cpp` + manifest:
  - stream stale telemetry (`FM2_STREAM_STALE`)
  - queue-producer counters (`qpush`, `qwrite`)
  - stream event dispatch counter (`dispatch_count`)
  - decode return probe at `0x82693924` (`FM2_DECODE_RET`)
- Self-ran `fm2.exe` for ~42-55s and parsed `C:\temp\fm2-clean.log`.

### Durable observations

- Decode service is healthy until about `t=32`, then hard-drops at `t=33`:
  - `FMOD_PERSEC t=31 decodes~50`
  - `FMOD_PERSEC t=32 decodes~30-40`
  - `FMOD_PERSEC t=33 decodes=0` and remains 0.
- Stream object stays alive during collapse:
  - `term80=0`
  - `gate184=00000000`
  - event handle present (`ev76=F80019B0/F80019AC` depending run).
- Work queue pointers remain empty while stalled:
  - `q168=00000000`, `q172=00000000`.
- Producer counters plateau exactly when decode stops:
  - `qpush` stops at ~228
  - `qwrite` stops at ~15
- Forcing stream event dispatch every keepalive (`dispatch_count` rising to
  hundreds) did **not** prevent collapse.
- Decode-return probe showed no transition to decoder error:
  - no `FM2_DECODE_RET` lines with nonzero return / zero produced bytes before collapse.
  - implication: collapse is **upstream scheduling cessation** (FM2 stops
    calling `sub_826938E8`), not `sub_82692CC8` returning EOF/error.

### Experiments that did not fix collapse

- Force queue signal dispatch hook (`0x8258A634`) alone.
- Additional forced event signaling from keepalive loop.
- Native copy-byte cadence clamp (`copy_bytes = target_bytes` in hard-native mode).

### Current hypothesis (best-supported)

- The issue is not "decode fails"; it is that FM2's producer/scheduler path
  stops enqueueing work for this stream after a deterministic point (~t=33).
- The next useful target is the **producer gate/state transition** that stops
  queue writes (`qpush/qwrite`) rather than the decode callback itself.

## May 29 2026 Additional Finding: Default Thread Assignment Drift vs Xenia

Observation:
- ReX and Xenia diverged in no-affinity thread assignment behavior.
- In Xenia, threads without explicit processor assignment inherit the parent's
  hardware thread.
- In ReX, no-affinity threads were round-robined across 6 hardware threads.

Why this matters:
- FM2 has tight producer/consumer synchronization in the audio path.
- Different default hardware-thread placement can materially change wake/queue
  jitter, even when explicit cadence constants are identical.

Action taken:
- ReX `GetFakeCpuNumber(0)` in `src/system/xthread.cpp` was aligned to Xenia
  semantics: inherit parent CPU when available; only fall back to round-robin
  when there is no parent thread context.

Status:
- This is a semantic parity patch (not another cadence constant tweak). Runtime
  validation is required to confirm impact on FM2 underrun/crunch behavior.

## May 29 2026 Additional Finding: Missing Thread Priority/Quantum Init

Observation:
- ReX was not initializing new `X_KTHREAD` priority/quantum fields from the
  owning process defaults during `InitializeGuestObject`.
- Xenia initializes thread priority state from process defaults at thread
  creation.

Patch:
- In `src/system/xthread.cpp` (`InitializeGuestObject`), when `process_ptr`
  resolves:
  - set `guest_thread->priority` from `target_process->unk_19`
  - set `guest_thread->quantum` from `target_process->quantum`
  - mirror process priority bytes into `unk_B8/B9/BA`
  - initialize host-side `priority_` / `base_priority_` from
    `target_process->unk_19`

Quick validation run (`2026-05-29 11:25`, ~12s):
- `REX_XMA_EXPORT_PERSEC`: still ~40-44/s (insufficient for clean audio)
- `FMOD_PERSEC` decodes: still mid-40s
- underruns improved slightly from ~4/s to ~3/s in this short sample, but
  starvation remains.

## May 29 2026 Producer-Gate Patch Cycle (FM2-only hooks)

Implemented:
- Added FM2-only producer diagnostics and bounded force hook path in
  `FM2/src/fm2_hooks.cpp` + `FM2/fm2_manifest.toml`:
  - gate decision hook: `0x8220A564` (`FM2ProducerGateForceMaybe`)
  - signal path counter hook: `0x8220A56C` (`FM2DiagSignalPathA56C`)
  - queue write hooks: `0x8258AB18` / `0x8258AB10`
  - codec read heartbeat hook: `0x826938E8`
- Added per-second log line:
  - `FM2_GATE_PERSEC ... gate_calls gate_blocked gate_forced setevent_calls qpush qwrite read_calls underrun_delta ...`
- Added bounded forcing policy:
  - force only on blocked gate + active stream + stalled queue + recent read heartbeat
  - rate-limited to 1 per 8 ms and max 16/sec.
- Kept intro skip hooks unchanged.
- Kept delay hook active by default, with env toggle:
  - `REX_FM2_DELAY_PATCH=0` disables `FM2AdjustDelayInterval11D20` for A/B.
- Build note: current `FM2/generated/rexglue.cmake` was temporarily forced to
  prefer installed SDK package lookup (disabled monorepo source-tree auto-pick)
  so FM2 links reliably in this workspace/toolchain.

### Runtime results

Phase A (delay hook enabled, `2026-05-29 13:48`, 60s):
- Early run improved underrun rate versus previous baseline:
  - `FMOD_PERSEC_WARNING` settled around `underruns=1/sec` (was commonly `3/sec`).
- Decodes remained near mid-40s/low-50s initially.
- Collapse still occurred around `t=33-36`:
  - `read_calls` dropped to `0`
  - `gate_forced` dropped to `0`
  - `decodes` then stuck at `0` while signals continued (`~32-33/s`).
- Interpretation: this patch improved early starvation but did not solve the
  deterministic producer stop.

Phase B (delay hook disabled, `REX_FM2_DELAY_PATCH=0`, `2026-05-29 13:49`, 60s):
- Worse than Phase A:
  - `underruns` stayed around `3/sec`
  - same collapse pattern around `t=33-36` to `decodes=0`.
- Keep delay hook enabled for now.

### Additional attempt rejected

- Increasing read-heartbeat grace from `250ms` to `2000ms` was tested
  (`2026-05-29 13:51`) and rolled back:
  - underruns worsened (up to `4/sec`)
  - collapse still occurred.

### Durable conclusion

- FM2-only producer-gate forcing is now instrumented and bounded, and A/B
  behavior is measurable.
- It can reduce early underruns (best seen with delay hook enabled), but does
  not prevent the deterministic scheduler collapse where `sub_826938E8` calls
  stop.
- Next investigation target should be upstream state transition(s) that cease
  codec-read scheduling, not just gate threshold cadence.

## May 29 2026 Follow-up: Gate Cap Increase + Throughput Ceiling

Applied (FM2-only):
- `kGateForceMinIntervalMs`: `8 -> 4`
- `kGateForceMaxPerSec`: `16 -> 64`

Observed in self-run 10-12s samples:
- `FM2_GATE_PERSEC` moved from ~`setevent_calls=38/s` to ~`58-60/s`.
- `FMOD_PERSEC signals` moved to ~`61-64/s`.
- `FMOD_PERSEC decodes` remained ~`42-52/s` (still far below 90+ target).
- `c670_cb` matched decode cadence and `c670_out_bytes` showed ~`2048` bytes per callback,
  confirming callback size is fine but callback **frequency** is still too low.

Tested and removed:
- Hook attempt at `0x82588BC4` (`FM2ForceExtraWorkLoop88BC4`) to force extra
  stream-work loop.
- Result: `stream_boost` never triggered on this execution path (`0` every second),
  so it was removed to keep active patchset minimal.

Current conclusion:
- The producer-gate divider suppression is now mostly bypassed.
- Remaining slowdown is due to a second cadence gate upstream of callback
  invocation count (not event signaling alone).

## May 29 2026: May-27 Patch Replay Matrix (FM2-only, 12s each)

Replayed May-27 candidate behaviors one-by-one from a clean hook baseline
using runtime toggles:
- `REX_FM2_MAY27_GATE`
- `REX_FM2_MAY27_PRESERVE_VALID`
- `REX_FM2_MAY27_FIRSTTIME`

Average metrics (warmup skipped, `t>=3`):

| Case | avg signals/s | avg decodes/s | dec range | avg underruns/s |
|---|---:|---:|---:|---:|
| baseline | 32.4 | 44.6 | 41-48 | 3 |
| gate_only (selective) | 32.4 | 45.2 | 38-51 | 4 |
| preserve_only | 32.4 | 0.0 | 0-0 | 3 |
| firsttime_only | 32.4 | 46.0 | 44-48 | 4 |
| gate+preserve | 32.4 | 0.0 | 0-0 | 2 |
| gate+firsttime | 32.4 | 44.9 | 41-47 | 3 |
| preserve+firsttime | 32.4 | 0.0 | 0-0 | 4 |
| all_three | 32.4 | 0.0 | 0-0 | 4 |

Control (May-26 style broad gate force):
- `REX_FM2_MAY27_GATE=1` + `REX_FM2_MAY27_GATE_FORCEALL=1`
- `avg signals/s=64.1`, `avg decodes/s=46.5`, `avg underruns/s=4`

Interpretation:
- The remembered "speed-up" behavior aligns with **gate force increasing
  signal rate** (broad force control reproduces this).
- The May-27 **selective** gate condition as currently replayed does not
  materially change signal rate (likely not matching active callsite/object
  pattern in this build/run).
- The May-27 preserve-valid patch by itself collapses decode service to zero
  (silent path) unless paired with additional state-recovery logic not yet
  reproduced exactly.

## May 30 2026 Triple-Check: Where Wake/Service Speed Is Set (Ghidra re-verify)

Re-validated in Ghidra (`default.xex`) that FM2 stream service speed is governed
by event-signal cadence plus the stream wait timeout fallback.

### 1) Producer queue+signal write path

- `0x825898F8` (`sub_825898F8`) performs queue payload copy and then signals:
  - `0x82589964: lwz r3, 0x4C(r31)` (event handle load)
  - `0x82589968: bl 0x8240C4F8` (`Nt_SetEvent`)
- `0x82589978` and `0x82589990` are wrappers that forward into `sub_825898F8`.
- `0x8258A528` dispatches to those wrappers based on command/type branches:
  - `0x8258A638: bl 0x82589978`
  - `0x8258A61C: bl 0x82589990`

### 2) Stream thread wait cadence fallback

- Stream loop (`0x825890F8` region) calls:
  - `0x82589164: li r4, 0x20` (32 ms)
  - `0x8258916C: bl 0x824119E0` (`Nt_WaitForSingleObject` wrapper)
- `0x824119E0` uses helper `0x82411AA8` to convert ms to relative NT interval:
  - `mulli r11, r11, -0x2710` (ms × -10000 in 100ns units)
  - `-1` timeout maps to `NULL` (infinite wait) path.

### 3) Gate-side suppression/allow logic

- `0x8220A4E8` (`FM2_SignalGate`) increments gate counter at `dword_829C24C8`,
  checks object bytes at `+0x41D/+0x41E`, and conditionally signals:
  - decision point: `0x8220A564`
  - signal call: `0x8220A56C -> bl Nt_SetEvent`
  - counter reset after signal: `0x8220A57C`

### Practical conclusion

- The **write site** is `sub_825898F8` (via wrappers/dispatcher).
- The effective service “speed” is not a single scalar: it is
  `producer signal cadence` + `32ms wait fallback`.
- Any persistent slowdown can come from:
  - lower signal cadence (gate/producer suppression), or
  - stream waiting out timeout more often than expected.

## May 30 2026 IDA + Ghidra Cross-check: What Is Not The Cadence Knob

Cross-checked in both IDA and Ghidra:

- `0x823EB998` is an atomic increment into `0x829F1040` (PIX/audio counter bucket),
  not a wake/scheduler timing controller.
- `0x823EB9D0` rotates/copies those counter buckets (`0x829F1040 -> 0x829F105C`)
  and clears the source once per frame; also telemetry-only.
- `0x826938E8` read path is hard-bounded by a `0x800` ring window and wrap copy.
  Forcing request size to `4096` at `0x82693954` cannot create a true larger payload
  path here and worsened underruns in 12s A/B.

Operational takeaway:
- Stop treating `0x823EB998/0x823EB9D0` as pacing controls.
- Keep focus on real cadence-producing paths:
  - producer enqueue+signal (`0x8258A528 -> 0x82589978/0x82589990 -> 0x825898F8`)
  - worker wait loop fallback (`0x825890F8`, `Nt_WaitForSingleObject(..., 32ms, 1)`).

## May 30 2026: Active Scheduler Gate Found (`0x8236C4F0`) With New Branch Counters

Added FM2-only diagnostics at the active scheduler gate path:
- `0x8236C5AC` over-threshold branch hit
- `0x8236C618` queued path hit
- `0x8236C640` immediate-submit path hit

12s run (`C:\\temp\\fm2-clean.log`, wait override disabled with `REX_FM2_PUMP_WAIT_MS=0`):
- `FM2_FMOD_UP_PERSEC`: callback/dispatch stayed high (`cb~188/s`, `disp~188/s`)
- `FM2_FMOD_SCHED_PERSEC`: scheduler/submit cadence stayed low (`hit~29-31/s`)
- `FM2_FMOD_READ_PERSEC`: decode/read cadence tracked scheduler (`~40-46/s`)
- `FMOD_PERSEC`: decode stayed around `~42-48/s`, far below Xenia's known `~124/s`

Interpretation:
- The dominant limiter is now clearly between callback/dispatch and scheduler submit.
- ReX is processing many callback ticks, but only scheduling/submitting decode work about `~30/s`.
- This explains why signal/write tuning alone never reached Xenia-like decode cadence.

### Mode Test: `REX_FM2_SCHED_MODE2=1` (A/B)

Baseline medians:
- scheduler hits: `30/s` (`mode=1`, mixed immediate/queued)
- decode: `44.5/s`

`SCHED_MODE2` medians:
- scheduler hits: `30/s` (`mode=2`, effectively all queued in this run)
- decode: `42.5/s` (worse)

Conclusion:
- Forcing mode 2 is not the fix.
- The unresolved difference is upstream frequency/eligibility of scheduler submit calls, not just mode field selection.
