# FM2 Audio Pipeline Map

## Date: 2026-05-23

## Symptom
Audio sounds stretched/slow, almost like half-speed playback, with possible buffer underruns.

## Architecture Overview

```
Game (recompiled PPC → x86)
  │
  ├─ XAudioRegisterRenderDriverClient(callback, arg)
  │     → AudioSystem::RegisterClient → SDLAudioDriver::Initialize
  │       → SDL_OpenAudioDeviceStream(48kHz, F32LE, 6ch, SDLCallback)
  │
  ├─ XAudioSubmitRenderDriverFrame(driver, samples_ptr)
  │     → AudioSystem::SubmitFrame → SDLAudioDriver::SubmitFrame
  │       → memcpy guest→heap → push to frames_queued_
  │
  ├─ XAudioGetRenderDriverTic()
  │     → Clock::QueryGuestTickCount() (guest tick counter)
  │
  └─ XMA*() functions
        → XmaDecoder::WorkerThreadMain → XmaContext::Work
          → XmaContext::Decode (FFmpeg XMA2 decoder)
          → XmaContext::ConvertFrame (float→int16 BE PCM)
          → Write to guest memory ring buffer

Host Audio Output:
  SDL Audio Device (48kHz, F32LE, 2ch or 6ch, pull model ~188Hz)
    ← SDLCallback pulls from frames_queued_
    ← Releases semaphore per consumed frame
    ← On underrun: writes silence, also releases semaphore

Worker Thread (AudioSystem::WorkerThreadMain):
  Waits on client semaphores → wakes → executes guest callback
  → Guest callback produces one frame of PCM samples
  → Calls XAudioSubmitRenderDriverFrame internally
```

## 1. Entry Points from Game/Recompiled Code into the Audio Shim

### XAudio Render Driver API

| # | Function | File | Line | What It Does |
|---|----------|------|------|--------------|
| 1a | `XAudioRegisterRenderDriverClient_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 55 | Game registers an audio client (callback + arg). Returns opaque driver handle `0x4155xxxx` encoding the client index. |
| 1b | `XAudioSubmitRenderDriverFrame_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 93 | Game submits one frame of PCM samples (guest VA). Extracts client index from driver handle, calls `AudioSystem::SubmitFrame`. |
| 1c | `XAudioUnregisterRenderDriverClient_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 84 | Game tears down a client. |
| 1d | `XAudioGetRenderDriverTic` (REX_HOOK_RAW) | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 143 | Returns `Clock::QueryGuestTickCount()` in r3. Used by guest for buffer position/timing. |
| 1e | `XAudioGetSpeakerConfig_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 28 | Returns `0x00010001` (stereo config). |
| 1f | `XAudioGetVoiceCategoryVolumeChangeMask_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 33 | Sleeps 1us, returns 0 (no volume changes). |
| 1g | `XAudioGetVoiceCategoryVolume_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 44 | Returns volume 1.0f. |
| 1h | `XAudioEnableDucker_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 51 | Stub — returns success. |
| 1i | All `REX_EXPORT_STUB` entries | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 125-161 | RenderDriverInitialize, Lock, SetVoiceCategoryVolume, DigitalBypass*, SubmitDigitalPacket, QueryDriverPerformance, GetRenderDriverThread, SpeakerConfig*, Suspend*, MEC*, CaptureFrame, Ducker*, UnderrunCount, SetProcessFrameCallback — all stubbed (no-op). |

### XMA Entry Points (separate decode path)

| # | Function | File | Line | What It Does |
|---|----------|------|------|--------------|
| 1j | `XMACreateContext_entry` / `XMAReleaseContext_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | 66/79 | Allocate/release an XMA decoder context from the 320-slot pool. |
| 1k | `XMAInitializeContext_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | 125 | Sets up input/output buffers, loop data, sample rate, channel count on an XMA context. |
| 1l | `XMAEnableContext_entry` / `XMADisableContext_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | 325/330 | Kick/stop decode via register file writes. `XMADisableContext` also blocks until decode finishes. |
| 1m | `XMABlockWhileInUse_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | 341 | Spin-waits until both input buffers are invalid. |
| 1n | Buffer valid/offset getters/setters | `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | 208-318 | `XMASetInputBuffer0/1`, `XMAIsInputBuffer0/1Valid`, `XMASetOutputBufferValid`, `XMAGet/SetOutputBufferReadOffset`, `XMAGetOutputBufferWriteOffset`, `XMAGetPacketMetadata`, `XMASetLoopData`, `XMAGet/SetInputBufferReadOffset`. |

## 2. Voice Creation / Client Registration

There are **no IXAudio2SourceVoice / IXAudio2MasteringVoice** abstractions. The Xbox 360 XAudio model is a single "render driver client" per game, not per-voice.

| Step | Function | File | Line | Detail |
|------|----------|------|------|--------|
| Create | `AudioSystem::RegisterClient` | `src/audio/audio_system.cpp` | 273 | Finds free slot in `clients_[8]`, pre-releases semaphore by `queued_frames_` (default 8), calls virtual `CreateDriver`. |
| Create driver | `SDLAudioSystem::CreateDriver` | `src/audio/sdl/sdl_audio_system.cpp` | 32 | Creates `SDLAudioDriver`, calls its `Initialize()`. |
| SDL init | `SDLAudioDriver::Initialize` | `src/audio/sdl/sdl_audio_driver.cpp` | 38 | Opens SDL audio device: **48 kHz, float32, 6ch** (falls back to 2ch stereo). Opens `SDL_OpenAudioDeviceStream` with `SDLCallback` as audio callback. Resumes device immediately. |

### Audio Format Constants

Defined in `include/rex/audio/sdl/sdl_audio_driver.h`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `frame_frequency_` | 48000 | Sample rate |
| `frame_channels_` | 6 | Channel count (5.1 surround) |
| `channel_samples_` | 256 | Samples per channel per frame |
| `frame_samples_` | 1536 | Total floats per frame (6 × 256) |
| `frame_size_` | 6144 | Bytes per frame (1536 × sizeof(float)) |

Expected SDL callback rate: `48000 / 256 = 187.5 Hz` (~188 callbacks/sec).

## 3. Buffer Submission

| Step | Function | File | Line | Detail |
|------|----------|------|------|--------|
| Guest submit | `XAudioSubmitRenderDriverFrame_entry` | `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | 93 | Validates driver handle, calls `AudioSystem::SubmitFrame(index, samples_ptr)`. |
| System submit | `AudioSystem::SubmitFrame` | `src/audio/audio_system.cpp` | 306 | Acquires global lock, calls `clients_[index].driver->SubmitFrame(samples_ptr)`. |
| SDL submit | `SDLAudioDriver::SubmitFrame` | `src/audio/sdl/sdl_audio_driver.cpp` | 104 | Translates guest VA to host, copies 6144 bytes from guest memory into a heap-allocated frame buffer, pushes onto `frames_queued_`. Does **not** release semaphore (that happens in SDL callback when frames are consumed). |

## 4. Audio Callback / Buffer-End Handling

### 4a. Host-side: SDL Audio Callback

| Function | File | Line | Detail |
|----------|------|------|--------|
| `SDLAudioDriver::SDLCallback` | `src/audio/sdl/sdl_audio_driver.cpp` | 153 | Called by SDL when it needs more audio data (~188 Hz). `additional_amount` bytes requested. Loops, pulling from `frames_queued_`. For each consumed frame, releases `semaphore_` by 1 (unblocks worker). On underrun (empty queue), writes silence and **also** releases semaphore (fix for death spiral bug). |

### 4b. Worker Thread (bridges SDL ↔ guest)

| Function | File | Line | Detail |
|----------|------|------|--------|
| `AudioSystem::WorkerThreadMain` | `src/audio/audio_system.cpp` | 96 | Runs on `XHostThread` ("Audio Worker"). Main loop: `WaitAny` on client semaphores + shutdown event. When a client semaphore fires, executes the guest callback (`function_dispatcher_->Execute`) which produces a frame. |

**Data flow:**
```
SDL consumes frame → releases semaphore
  → worker thread wakes → calls guest callback
  → guest produces frame → SubmitFrame queues it
  → SDL consumes it
```

### 4c. Guest-side Audio Engine (FM2-specific)

From `docs/FM2-audio-notes.md`:

| Function | Address | Role |
|----------|---------|------|
| `sub_823E83C8` | Audio callback | Two paths — render path (fast, ~0.1ms) and wait path (blocks on `KeWaitForMultipleObjects`). |
| `sub_823E8250` | Audio worker thread | Loops on `byte_829F1014` event, signals `byte_829F1004` on completion. |
| `sub_823E85E0` | Audio engine init | Creates events (`byte_829F1014`, `byte_829F1004`, `byte_829F0FE0`), semaphore (`unk_829F0FF0`, limit=6), spawns worker thread. |
| `sub_823E7D88` | Audio engine shutdown | Signals terminate event, cleans up. |

**Key guest addresses:**
- `dword_829F1024`: Global audio engine context pointer
- `*(dword_829F1024+300)`: Work pending flag (worker loop exit condition when 0)
- `*(dword_829F1024+304)`: Wait-path trigger flag

**Known issue:** At ~64s, guest callback enters infinite `KeWaitForMultipleObjects` (NULL timeout). Mitigated by TEMP_BYPASS 5-second safety cap in `src/system/xobject.cpp`.

## 5. Decode / Conversion Path

### 5a. XMA Decode (compressed → PCM)

| Step | Function | File | Line | Detail |
|------|----------|------|------|--------|
| Worker loop | `XmaDecoder::WorkerThreadMain` | `src/audio/xma_decoder.cpp` | 140 | Polls all 320 contexts, calls `context.Work()` on enabled ones. |
| Context work | `XmaContext::Work` | `src/audio/xma_context.cpp` | 95 | Locks context, reads `XMA_CONTEXT_DATA` from guest memory, calls `Decode()` + `Consume()`. |
| Frame decode | `XmaContext::Decode` | `src/audio/xma_context.cpp` | 507 | Parses XMA packet headers, extracts frame, sends to FFmpeg (`avcodec_send_packet` / `avcodec_receive_frame`), converts output to big-endian int16 PCM. |
| Sample conversion | `XmaContext::ConvertFrame` | `src/audio/xma_context.cpp` | 693 | FFmpeg float → int16 BE, interleaving stereo. Uses SSE2 on x86-64. |
| Output ring buffer | `XmaContext::Consume` | `src/audio/xma_context.cpp` | 404 | Writes decoded subframes (256-byte blocks) into guest memory ring buffer. |
| Output setup | `XmaContext::PrepareOutputRingBuffer` | `src/audio/xma_context.cpp` | 304 | Creates ring buffer view over guest physical memory based on context offsets. |
| Codec setup | `XmaContext::PrepareDecoder` | `src/audio/xma_context.cpp` | 450 | Re-initializes FFmpeg context when sample rate or channel count changes. Uses `AV_CODEC_ID_XMAFRAMES`. |

### 5b. Channel/Format Conversion (PCM output → SDL)

Defined in `include/rex/audio/conversion.h`:

| Function | Line | Detail |
|----------|------|--------|
| `conversion::sequential_6_BE_to_interleaved_2_LE` | 43 | 6ch BE float sequential → 2ch LE float interleaved. Downmix: `L = (FL + BL + FC/2) × 0.4`, `R = (FR + BR + FC/2) × 0.4`. Discards LFE. SSE2 path on x86-64. |
| `conversion::sequential_6_BE_to_interleaved_6_LE` | 22 | 6ch BE float sequential → 6ch LE float interleaved. Just byte-swap + interleave. SSE2 path on x86-64. |

## 6. Host Backend Output Path (SDL3)

```
SDL Audio Device (48kHz, F32LE, 2ch or 6ch)
   ↑ SDLCallback (pull model, ~188Hz)
   ↑ SDL_PutAudioStreamData(stream, data, len)
   |
SDLAudioDriver::SDLCallback [sdl_audio_driver.cpp:153]
   ↑ reads from frames_queued_ (std::queue<float*>)
   ↑ each frame = 1536 floats (6ch × 256 samples)
   ↑ on underrun: writes silence, releases semaphore
   |
SDLAudioDriver::SubmitFrame [sdl_audio_driver.cpp:104]
   ↑ copies guest memory → heap frame → pushes to queue
   |
AudioSystem::SubmitFrame [audio_system.cpp:306]
   ↑ dispatches to correct client's driver
   |
XAudioSubmitRenderDriverFrame_entry [xboxkrnl_audio.cpp:93]
   ↑ entry from recompiled game code
```

SDL device opened via `SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec, SDLCallback, this)`.

## 7. Existing Logging / Debug Flags

| Flag/Cvar | File | Default | Purpose |
|-----------|------|---------|---------|
| `audio_mute` | `src/audio/sdl/sdl_audio_driver.cpp:26` | false | BOOL — mutes output (writes silence to SDL). |
| `ffmpeg_verbose` | `src/audio/xma_decoder.cpp:29` | false | BOOL — enables FFmpeg debug+verbose logging. |
| `audio_maxqframes` | `src/audio/audio_system.cpp:28` | 8 | INT32 (range 4-64) — max queued audio frames, controls semaphore pre-release count. |

### TEMP_DIAG logging already in place

| Log prefix | File | Line | What it logs |
|------------|------|------|--------------|
| `AUDIO_DIAG_PRE:` | `src/audio/audio_system.cpp` | 151 | Before guest callback dispatch (first 5 + every 500th). |
| `AUDIO_DIAG:` | `src/audio/audio_system.cpp` | 197 | After guest callback returns: r13, TLS identity, audio context +0x12C/+0x130, callback duration, pump count. (First 20 + every 500th + when tls_id==0 or ctx+12C==0). |
| `AUDIO_DIAG: queue DRAINING/RECOVERED` | `src/audio/sdl/sdl_audio_driver.cpp` | 179-183 | Queue depth transitions near empty (≤2 entering drain, >3 recovering). |
| `AUDIO_DIAG: SILENCE underrun` | `src/audio/sdl/sdl_audio_driver.cpp` | 188 | Every silence frame (first 50 + every 100th). |
| `SDL_AUDIO_STATS:` | `src/audio/sdl/sdl_audio_driver.cpp` | 237 | Every ~2s (375 callbacks): real_frames, silence_frames, total, elapsed, real_rate, expected=188/s, fill%. |

### Tracy/Profile counters

| Macro | File | Line | What it tracks |
|-------|------|------|----------------|
| `PROFILE_BUFFER_QUEUE_DEPTH` | `src/audio/sdl/sdl_audio_driver.cpp` | 129 | Queue depth after each SubmitFrame. |
| `PROFILE_XMA_FRAME_DECODED` | `src/audio/xma_decoder.cpp` | 149 | Count of XMA frames decoded. |

## 8. Files/Functions Relevant But Not To Be Touched Yet

| File/Function | Why Relevant | Why Not Touch |
|---------------|-------------|---------------|
| `src/audio/xma_context.cpp` — `Decode()` (line 507-691) | If XMA decode produces wrong sample rate or double-counts frames, audio would be stretched. | 768 lines of XMA bitstream parsing + FFmpeg. Wrong changes corrupt decode. Need evidence first. |
| `include/rex/audio/xma/context.h` — `XMA_CONTEXT_DATA` bitfields | `sample_rate` field (2-bit enum → 24/32/44.1/48kHz) feeds `PrepareDecoder`. If misread, FFmpeg decodes at wrong rate → pitch/stretch. | Guest-memory layout must match hardware exactly. |
| `src/core/clock.cpp:158` — `Clock::QueryGuestTickCount()` | Feeds `XAudioGetRenderDriverTic`. If tick rate doesn't match what guest expects (10 MHz Xbox 360 timer), guest-side buffer management mis-paces. | Used globally (graphics too). Must verify tick rate independently before touching. |
| `src/system/xobject.cpp` — TEMP_BYPASS 5-second safety cap | Related to the ~64s stall, but that is a separate issue from "always sounds stretched/slow." | Already documented as TEMP_BYPASS. Not the half-speed symptom. |
| `src/audio/nop/nop_audio_system.cpp` | NOP audio system — not used in FM2 builds. | Irrelevant to the bug. |
| `include/rex/audio/xma/helpers.h` | XMA packet parsing helpers. | Supporting infrastructure, no evidence of issues. |
| `include/rex/audio/xma/register_file.h` | XMA MMIO register file. | Supporting infrastructure, no evidence of issues. |
| `include/rex/audio/xma/register_table.inc` | XMA register definitions table. | Supporting infrastructure, no evidence of issues. |
| All files under `FM2/generated/` | Contains recompiled game code calling XAudio/XMA entry points. | Generated code — fixes go in manifest/TOML, not here. |

## 9. Hypothesis Table — Half-Speed / Stretched Audio

Ordered by likelihood based on architecture:

| # | Hypothesis | Evidence Needed | Where to Instrument |
|---|-----------|----------------|---------------------|
| H1 | SDL callback fires at half the expected rate (SDL buffer too large) | Measure actual `SDLCallback` call frequency from `SDL_AUDIO_STATS` real_rate value | `sdl_audio_driver.cpp` SDLCallback — already logging via SDL_AUDIO_STATS |
| H2 | Guest produces frames at half the expected rate (guest callback paces itself via `XAudioGetRenderDriverTic` with wrong tick rate) | Compare `QueryGuestTickCount()` frequency against Xbox 360 10 MHz expectation; log tic values and deltas | `xboxkrnl_audio.cpp` XAudioGetRenderDriverTic hook, `src/core/clock.cpp` |
| H3 | Semaphore rhythm wrong — worker wakes too infrequently, queue drains, silence fills gaps (perceived as slow/stretched) | Correlate SDL_AUDIO_STATS fill% with AUDIO_DIAG pump timing | Already instrumented — need to read logs |
| H4 | XMA decode outputs at 24 kHz but SDL plays at 48 kHz (or vice versa) | Log `data->sample_rate` in `XmaContext::Decode` and `av_context_->sample_rate` in `PrepareDecoder` | `xma_context.cpp` Decode + PrepareDecoder |
| H5 | 6ch→2ch conversion drops every other sample or misindexes | Unlikely given SSE2 code looks correct, but could verify by dumping pre/post conversion | `include/rex/audio/conversion.h` |

## 10. Complete File Index

| File | Role |
|------|------|
| `src/kernel/xboxkrnl/xboxkrnl_audio.cpp` | XAudio render driver kernel shim (register, submit, unregister, tic, volume, ducker stubs) |
| `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` | XMA context management kernel shim (create, release, init, enable/disable, buffer valid/offset) |
| `src/audio/audio_system.cpp` | AudioSystem base: worker thread, client registration, SubmitFrame dispatch |
| `include/rex/audio/audio_system.h` | AudioSystem class declaration (clients array, semaphores, wait handles) |
| `src/audio/audio_driver.cpp` | AudioDriver base (just ctor/dtor) |
| `include/rex/audio/audio_driver.h` | AudioDriver abstract base (SubmitFrame = 0) |
| `src/audio/sdl/sdl_audio_system.cpp` | SDLAudioSystem: CreateDriver/DestroyDriver factory |
| `include/rex/audio/sdl/sdl_audio_system.h` | SDLAudioSystem class |
| `src/audio/sdl/sdl_audio_driver.cpp` | SDLAudioDriver: Initialize (SDL device open), SubmitFrame (queue), SDLCallback (pull/convert/output), Shutdown |
| `include/rex/audio/sdl/sdl_audio_driver.h` | SDLAudioDriver class, format constants (48kHz/6ch/256 samples) |
| `src/audio/xma_decoder.cpp` | XmaDecoder: worker thread, context pool, MMIO register read/write, register-file kick/lock/clear |
| `include/rex/audio/xma/decoder.h` | XmaDecoder class declaration |
| `src/audio/xma_context.cpp` | XmaContext: Work/Decode/Consume/ConvertFrame (FFmpeg XMA2 decode pipeline) |
| `include/rex/audio/xma/context.h` | XMA_CONTEXT_DATA bitfield struct, XmaContext class, constants (packets, frames, subframes) |
| `include/rex/audio/conversion.h` | `sequential_6_BE_to_interleaved_2_LE` and `_6_LE` (SSE2 + scalar fallback) |
| `include/rex/audio/flags.h` | Cvar declarations: `audio_mute`, `ffmpeg_verbose` |
| `src/audio/nop/nop_audio_system.cpp` | NOP audio system (unused in FM2) |
| `include/rex/audio/nop/nop_audio_system.h` | NOP audio system class |
| `include/rex/audio/xma/helpers.h` | XMA packet parsing helpers |
| `include/rex/audio/xma/register_file.h` | XMA MMIO register file |
| `include/rex/audio/xma/register_table.inc` | XMA register definitions |
| `include/rex/system/interfaces/audio.h` | IAudioSystem abstract interface (Setup/Shutdown) |
| `src/core/clock.cpp` | `Clock::QueryGuestTickCount()` — feeds `XAudioGetRenderDriverTic` |
| `docs/FM2-audio-notes.md` | Existing notes on audio stutter investigation, guest audio engine functions, timing data |
