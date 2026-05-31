# FM2 Audio Stutter Investigation

## Date: 2026-05-22

## Pipeline Architecture
- SDL: 48kHz, 256 samples/ch, 6ch, ~188Hz callback rate
- Host: AudioSystem worker thread → guest callback → produce frame → queue → SDL consume
- Guest: `sub_823E83C8` (callback) ↔ `sub_823E8250` (worker thread) via events `byte_829F1014` and `byte_829F1004`

## Fixes Applied

### 1. SDL Silence-Path Death Spiral (FIXED)
- **File:** `src/audio/sdl/sdl_audio_driver.cpp`
- **Bug:** When queue was empty (silence path), semaphore was NOT released → worker thread blocked forever
- **Fix:** Added `semaphore_->Release(1, nullptr)` in silence/underrun branch
- **Result:** Startup queue drain now recovers (confirmed RECOVERED at count=4)

### 2. Infinite Kernel Wait Safety Cap (TEMP_BYPASS)
- **File:** `src/system/xobject.cpp` — Wait(), SignalAndWait(), WaitMultiple()
- **Bug:** At ~64s, guest callback `sub_823E83C8` enters `KeWaitForMultipleObjects(2, [byte_829F1004, byte_829F0FE0], WaitAny, ..., timeout=NULL)` and blocks forever
- **Root cause:** Worker thread `sub_823E8250` exits its loop when `*(dword_829F1024+300)==0`, but the callback then enters the wait-path expecting the worker to respond. Race condition in guest audio engine.
- **Bypass:** All NULL timeout kernel waits capped at 5 seconds. Logs `TEMP_BYPASS` message when safety timeout fires.
- **Expected:** Audio recovers after 5s gap instead of permanent silence.
- **Real fix needed:** Understand guest audio state machine — why `*(a1+300)` becomes 0 causing worker exit, then `*(a1+304)` becomes non-zero triggering wait path.

## Guest Audio Engine Functions

### sub_823E83C8 (Audio Callback)
- Called from host AudioSystem worker thread via `0x823EBF20` thunk
- Two paths:
  - **Render path** (`*(a1+304)==0`): calls `sub_823E7B68` + `sub_823E7C50` directly (fast, ~0.1ms)
  - **Wait path** (`*(a1+304)!=0`): signals `byte_829F1014`, then `KeWaitForMultipleObjects(2, [byte_829F1004, byte_829F0FE0], WaitAny, ..., NULL)` — blocks until worker signals completion or terminate
- `a1 = dword_829F1024` (global audio context)

### sub_823E8250 (Audio Worker Thread)
- Spawned by `sub_823E85E0` (audio engine init) via CreateThread
- Loop: `KeWaitForSingleObject(byte_829F1014, ..., NULL)` → process → `KeSetEvent(byte_829F1004)`
- Exits loop when `*(dword_829F1024+300)==0`
- Also releases semaphore `unk_829F0FF0` when no render work and not terminating

### sub_823E85E0 (Audio Engine Init)
- Initializes KEVENT `byte_829F1014` (signaled=1, manual reset)
- Initializes KEVENT `byte_829F1004` (signaled=1, manual reset)
- Initializes KEVENT `byte_829F0FE0` (terminate event)
- Initializes KSEMAPHORE `unk_829F0FF0` (count=0, limit=6)
- Spawns worker thread with `sub_823E8250` as start address

### sub_823E7D88 (Audio Engine Shutdown)
- Signals `byte_829F1014` to wake worker for termination
- Cleans up resources

## Key Addresses
- `dword_829F1024`: Global audio engine context pointer
- `byte_829F1014`: Callback → Worker signal (manual-reset event)
- `byte_829F1004`: Worker → Callback completion signal (manual-reset event)
- `byte_829F0FE0`: Terminate event
- `unk_829F0FF0`: Flow control semaphore (limit=6)
- `*(dword_829F1024+300)`: Work pending flag (also worker loop exit condition when 0)
- `*(dword_829F1024+304)`: Wait-path trigger flag

## Timing Data
- Pre-fix: 24-frame silence burst at ~47s, queue drains to 0, never recovers
- Post-fix (semaphore): startup drain recovers, but second drain at ~64s still permanent
- Second drain: worker thread (t83588) went silent after count=12000, no timeout logged
- Callbacks are fast (0.09-0.10ms), no guest blocking in normal path
- GPU healthy during audio stall (31ms frames, no GPU-side stall)

## Remaining Questions
- What sets `*(a1+304)` to non-zero? (triggers wait path)
- What sets `*(a1+300)` to 0? (causes worker thread exit)
- Are these set by other threads or by the callback itself?
- Does the safety timeout actually let the game recover gracefully?
- Is the ~64s timing deterministic or variable?
