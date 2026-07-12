# 2026-07-12 — POD RenderCommand + Proc* (slice 5: Unlock / upload)

## Landed

- POD Unlock / upload path:
  - `UnlockTextureRect` / `UnlockBuffer16` / `UnlockBuffer32` — async Enqueue
  - `CopyBufferFromUpload` / `CopyTextureFromUpload` — sync copy-queue Procs
  - `CreateTranslatedTextureHost` — sync Translate create (replaces Run(fn))
- `UploadFrameData` + `RetainTempUploadBuffer` for graphics-CL staging
  (cleared in `OnRecordingFrameReady`)
- Retired `ExecuteUpload(std::function)`

## Still next

- Optional async `Enqueue`+atomic wait for ExecuteCommandList (needs
  `g_readyForCommands` gate)
- Drop `RecordingMutex` once only render thread mutates GPU state
- Optional: retire remaining `Run(fn)` / `Enqueue(fn)` if any leftovers
- DDS `LoadTextureFromMemory` create still on caller thread (upload is POD)
