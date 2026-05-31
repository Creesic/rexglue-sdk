# FM2 Audio Decode Throughput Analysis — 2026-05-27

## Summary

The FM2 stretched audio issue has been narrowed to a **33% decode throughput deficit**. The XMA decoder produces exactly 512 PCM frames per decode cycle, but only decodes ~64 times/sec instead of the required ~94 (48000 / 512). This results in ~32K frames/sec produced vs ~48K frames/sec consumed, causing persistent underruns.

**Update (active-context scan optimization):** Eliminated 77K/sec idle context scans. `invalid` dropped to 0, but decode rate remained at ~64 Hz — confirming the bottleneck is FMOD's stream clock rate, not worker scan overhead.

## Evidence

### Wrap-Safe Byte Counter

A persistent ring-wrap aliasing bug masked the true decode output. The `output_buffer_write_offset` is stored in 256-byte blocks on a 2048-byte ring buffer. When exactly one full ring cycle occurs per decode (8 blocks × 256 = 2048), the offset goes 0→0, making the delta appear as 0 bytes.

Fix: added `diag_cumulative_output_bytes_` atomic counter to `XmaContext`, incremented in `Consume()` with actual bytes written. Read+reset after `BlockOnContext()` in `XMADisableContext_entry`.

**Result:** every decode now shows `out_bytes=2048` (512 frames × 4 bytes/frame stereo 16-bit), confirming the decoder IS producing data.

### Per-Second Metrics (steady state)

```
FMOD_PERSEC t=6  decodes=64  pcm_frames=32768  submitted=52480  underruns=1
FMOD_PERSEC t=7  decodes=64  pcm_frames=32768  submitted=52736  underruns=1
```

- **64 decodes/sec × 512 frames = 32,768 frames/sec produced**
- **48,000 frames/sec needed** (48kHz sample rate)
- **Deficit: ~15,232 frames/sec (32%)**

### Per-Decode Metrics

```
FMOD_CODEC_READ FINISH ... out_bytes=2048 pcm_frames=512 comp_bytes=0
```

- Every decode produces exactly 2048 bytes / 512 frames
- `comp_bytes=0` is misleading — input offsets in guest memory may not reflect worker-side consumption (see Open Questions)

### Gate Cadence

```
TASK6_GATE_SUMMARY ... calls=60 pred_true=30 pred_false=30 override_forced=30
```

- Gate callback fires at ~60 Hz (stable across all sessions)
- Gate override is active ~50% of calls
- Even with forced gate (doubling signal cadence to ~120 Hz), underruns persist — the bottleneck is downstream in decode throughput, not signal cadence

### Underrun History

| Session | Gate Override | Decodes/sec | Underruns/sec | Audio Status |
|---------|--------------|-------------|---------------|--------------|
| Pre-fix | None | ~30 | 2 | Stretched |
| +Gate override | Forced 50% | ~64-87 | 2 | Still stretched |
| +Wrap-safe counter | Forced 50% | ~64 | 1 | Still stretched |

## Architecture: FM2 Audio Pipeline

```
Game code
  │
  ├─ XMAInitializeContext (sets up input/output buffers, sample rate, channels)
  ├─ XMASetInputBuffer0/1 (feeds compressed XMA packets)
  ├─ XMAEnableContext → KickContext → worker thread Work() loop
  ├─ XMADisableContext → BlockOnContext → wait for worker
  └─ XMAGetOutputBufferWriteOffset → reads ring buffer write pointer
       │
       ▼
  Worker Thread (XmaContext::Work)
    ├─ PrepareOutputRingBuffer (maps guest output buffer to RingBuffer)
    ├─ Decode (ffmpeg XMAFRAMES decoder → raw_frame_)
    ├─ Consume (Write subframes to output ring buffer)
    │   └─ diag_cumulative_output_bytes_ += bytes_written
    └─ StoreContextMerged (write back context to guest memory)
       │
       ▼
  FMOD Stream Thread — "FMOD stream thread"
    ├─ Waits on event (dword_829C24C0) via gate callback
    ├─ Calls FMOD XMA Codec read callback (sub_82693B18 → sub_826938E8):
    │   1. XMADisableContext(ctx, wait=1) → blocks until worker done
    │   2. Function_82692CC8 → fills input buffers, checks output ring state
    │   3. memcpy 2048 bytes (512 frames) from ring to FMOD buffer
    │   4. XMAEnableContext(ctx) → kicks worker for next decode
    └─ Returns to FMOD mixer for mixing
       │
       ▼
  FMOD Mixer Thread — "FMOD mixer thread"
    ├─ Runs at ~60 Hz (Xbox 360 vsync cadence)
    ├─ Reads from FMOD stream buffer queue
    └─ Submits to SDL audio callback
       │
       ▼
  SDL Audio Driver
    ├─ SDLCallback: pulls channel_samples_=256 frames per callback
    ├─ At 48kHz → ~187 callbacks/sec → 48K frames/sec output
    └─ Converts BE sequential → LE interleaved → SDL_PutAudioStreamData
```

### Key XEX Constants (from IDA/Function_82692AF0)

| Parameter | Value | Effect |
|-----------|-------|--------|
| `OutputBufferSize` | 8 blocks | 8 × 256 = 2048 byte ring |
| `NumSubframesToDecode` | 4 | 4 × 256 = 1024 bytes per decode iteration |
| `channel_samples_` | 256 | SDL pulls 256 frames per callback |
| Sample rate | 48000 Hz | Decoder table index 3 |
| Codec read size | 2048 bytes | Hardcoded in `Function_82692CC8` → `*a3 = 2048` |

### FMOD XMA Codec VTable (from IDA/sub_82693B48)

```
+0x00: "FMOD XMA Codec"
+0x08: flags (65792 = 0x10100)
+0x14: numFields (10)
+0x18: open callback (sub_82693AE8 → sub_826934C0)
+0x1C: close callback (sub_82693B00)
+0x20: read callback (sub_82693B18 → sub_826938E8)  ← the hot path
+0x28: ? callback (sub_82693B30)
+0x3C: 22
+0x40: 228
```

### Codec Read Call Chain (from Ghidra xrefs)

```
FMOD stream thread
  → sub_82693B48 vtable[read] → sub_82693B18 (thunk)
    → sub_826938E8 (FMOD XMA Codec read)
      → XMADisableContext (blocks)
      → sub_82692CC8 (fill input buffers + check output ring)
      → memcpy 2048 bytes from ring
      → XMAEnableContext (kicks worker)
```

### Worker Scan Optimization Results

| Metric | Before (320-scan) | After (active-list) | Change |
|--------|-------------------|---------------------|--------|
| invalid | 77,678/s | **0/s** | Eliminated |
| sweeps | 243/s | 192/s | -21% |
| iters | 163/s | 126/s | -23% |
| decodes | 82/s | 64/s | ~same |
| pcm_frames | 40,960/s | 30,976/s | ~same |
| underruns | 60/s | 55/s | ~same |

**Conclusion:** Scan overhead was not the bottleneck. FMOD stream clock rate is the limiter.

## Key Files

- `src/audio/xma_context.cpp` — `Work()`, `Consume()`, `Decode()`, `StoreContextMerged()`
- `src/audio/xma_decoder.cpp` — `WorkerThreadMain()`, `BlockOnContext()`, `KickContext()`
- `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` — `XMADisableContext_entry`, `XMAEnableContext_entry`
- `src/kernel/xboxkrnl/xma_gap_diag.h` — `LogCodecReadFinish`, `LogCodecReadStart`, per-second counters
- `include/rex/audio/xma/context.h` — `XmaContext` class, `diag_cumulative_output_bytes_`
- `include/rex/audio/xma/decoder.h` — `XmaDecoder`, `ReadAndResetOutputBytes()`
- `include/rex/memory/ring_buffer.h` — `RingBuffer` class

## Open Questions

1. **Why does FMOD's mixer clock run at ~60 Hz instead of ~94 Hz?**
   - FMOD's mixer thread ("FMOD mixer thread") runs at the Xbox 360 vsync rate (60 Hz)
   - Each tick calls the stream's codec_read callback
   - On real Xbox 360 hardware, XMA hardware decodes asynchronously and continuously, so the ring always has data when the mixer reads
   - In our emulation, decode is synchronous within the Enable→Disable window

2. **`comp_bytes=0` mystery mostly resolved:** Input offsets show no change in guest memory between Start and Finish. This is because `Function_82692CC8` fills input buffers from the FMOD stream BEFORE calling Enable, and Disable returns after the worker processes. The input offset changes happen inside the worker but may not persist to the guest-visible context depending on the decode path.

3. **Why did underruns jump from 1-2 to 55-60?** The jump coincided with a fm2_hooks.cpp fix by the user. This suggests a separate issue in the hook configuration may be affecting audio, independent of the XMA throughput.

## Next Steps

1. **Primary target: FMOD mixer clock rate.** The mixer runs at ~60 Hz. We need it to run faster, OR produce more data per tick.
   - Option A: **Hook the XMA context init** to increase `OutputBufferSize` from 8 to 16 blocks (4096 bytes). This lets the worker do 4 decode iterations per Work() call (1024 bytes each), but the codec_read still reads only 2048 bytes. The extra data stays in the ring as a buffer.
   - Option B: **Hook `Function_82692CC8`** to report more bytes available when the ring has data, causing codec_read to read more per call.
   - Option C: **Speed up the worker's ring-fill cycle** so the ring is always full when the mixer reads. This is already the case — the bottleneck is FMOD's read rate, not the worker.

2. **Trace the exact FMOD mixer timing:** Add a hook at codec_read entry/exit to measure the time between consecutive calls. This will confirm the 15.6ms interval and help identify what controls it.

3. **Investigate the underrun regression** (1→55/s) separately from the throughput issue.
