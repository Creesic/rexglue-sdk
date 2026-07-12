# 2026-07-12 — POD RenderCommand + Proc* (slice 4: Present split)

## Landed

- Unleashed Present split:
  - `ExecuteCommandList` — blit + end CL + GPU submit (render thread)
  - Guest `PresentAndAdvanceFrame` — swapchain present + 2-frame advance
    (holds `RecordingMutex` so Dispatch cannot race `g_frame`)
  - `BeginCommandList` — `OnRecordingFrameReady` / upload reset (render thread)

## Still uses std::function Run/Enqueue

- `ExecuteUpload` (arbitrary copy-queue lambdas)
- `TranslateGuestTexture` create path

## Still next

- Specialize Unlock/upload into POD; drop remaining `Run(fn)`
- Optional async `Enqueue`+atomic wait for ExecuteCommandList (needs
  `g_readyForCommands` gate)
- Drop `RecordingMutex` once only render thread mutates GPU state
