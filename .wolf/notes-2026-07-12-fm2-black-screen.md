# 2026-07-12 — FM2 black screen + intermittent startup hang

## Evidence (`logs/fm2_094.log`, `fm2_098.log`)

Present path is alive:
- `Video::Init` OK, swapchain valid
- `PrepareFramePresent: first non-null render target`
- `Video::Present: blitting present-source 1280x720`
- `CreateGraphicsPipeline: first PSO built successfully`

So black is **not** “never presents”. Content to the RT / blit source is empty or wrong.

### Black-screen causes (ranked)

1. **XG textures bound null** (pre-fix) → shaders sample null. Fixed by porting `TranslateGuestTexture` + wiring `SetTextureHook`. Log after fix: early `TranslateGuestTexture: base=… fmt=50 … -> desc N` successes.
2. **Prim type 8 skipped** (`D3DPT_RECTLIST`) — DEST returned UNKNOWN and skipped draws; SOURCE maps unknown → triangle list. Fixed `ConvertPrimitiveType` + QUADLIST/FAN re-index.
3. **DEVICE_REMOVED cascade** — after ~1s, hundreds of `TranslateGuestTexture: failed to create` + `CreateTexture: Plume createTexture failed (… fmt=0x18280186)` + guest `tw/td trap`. Once the device is gone, creates fail and guest traps on null resources.
4. Still open: **Resolve / compositing**, MSAA, sampler dirty tracking (audit backlog).

### Hang causes

1. `waitForCommandFence(INFINITE)` after DEVICE_REMOVED → process freeze. **Fix:** 5s timeout + log `GetDeviceRemovedReason`.
2. `EnsureFrameStarted` retrying failed `ResizeBuffers` forever under `RecordingMutex`. **Fix:** stop after 8 consecutive resize failures.
3. CreateResource spam on dead device from uncached Translate failures (felt like hang). **Fix:** `g_failedGuestTextureBases` cache + warn-once.

## Fixes landed this session

- `ConvertPrimitiveType` matches SOURCE (RECTLIST/QUADLIST/FAN → triangle list)
- QUADLIST/FAN index expansion in `DrawVertices` / `DrawUserPointerVertices`
- `TranslateGuestTexture` / `TranslateGuestTextureFetch` port + SetTexture wiring
- Fence wait timeout; resize fail streak; failed XG-texture base cache

## Next to chase DEVICE_REMOVED root

What kills the device between first successful XG uploads (~14:06:09.18) and the create-fail flood (~14:06:09.30)? Likely bad GPU work (upload copy, draw, barrier). Enable DRED / capture stderr `CreateResource` / `GetDeviceRemovedReason` on the first failure.
