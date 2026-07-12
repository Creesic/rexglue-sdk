# 2026-07-12 — Log triage: fm2_104.log (black + audio)

Log: `FM2/out/build/win-amd64-relwithdebinfo/logs/fm2_104.log`
Run window: 19:20:05 → 19:20:11 (~6s, user closed window).

## Verdict

**DEVICE_REMOVED path.** Not “healthy GPU drawing empty RT.”
Present/blit and first PSO succeed, then the device dies within ~140ms.
Black + continued audio after that is expected (GPU latched dead, CPU/audio keep going).

## Timeline

| t | Event |
|---|--------|
| 06.024 | Video::Init OK (RTX 4090), swapchain 1280x720 |
| 06.079 | render thread started |
| 06.126 | `ConvertFormat: unknown D3DFORMAT 0x2D20014A (gpu format 10) → R8G8B8A8` |
| 06.127 | First Swap → ECL: non-null RT; **blitting 1280x720 format=24** |
| 06.237–.259 | 6× `TranslateGuestTexture` OK (fmt=50 R8, various sizes) |
| 06.257 | **first PSO built successfully** (0 ms) |
| 06.265 | **GPU device lost latch (CreateTexture)** — 1280x720 `fmt=0x18280186` |
| 06.272+ | SetTexture → bound null (20×); Swap storm (2400+ by 08.347) |
| 06.413+ | tw/td trap flood (825); cache:\ missing (unrelated) |

## Counts (signal)

| Pattern | Count |
|---------|------:|
| blitting present-source | 1 |
| no blit / fallback | 0 |
| first PSO | 1 |
| CreateGraphicsPipeline rejected | 0 |
| TranslateGuestTexture success | 6 |
| TranslateGuestTexture failed | 0 |
| GPU device lost | 1 |
| CreateTexture: Plume failed | 1 |
| SetTexture bound null | 20 |
| Swap hook log lines | 9 (2400+ calls) |
| waitForCommandFence timeout | 0 |

## Implications

1. Step **2** next: find what *first* removes the device (CreateTexture latch is likely the *observer*, not necessarily the killer). Suspects: unknown format `0x2D20014A`, early Translate/upload/copy, first draws after PSO, barrier misuse.
2. Capture **stderr** for Plume `GetDeviceRemovedReason` (not always in spdlog).
3. Present storm still huge after death — coalescing only helps while Present still runs; after latch Present returns early, so Swap spam is guest-side spinning on a dead GPU path.
