# 2026-07-12 — POD RenderCommand + Proc* (slice 3: Present/Create)

## Landed

- `RenderQueue::Run(RenderCommand)` — sync POD (Present / Wait / creates)
- POD: `ExecutePresent`, `WaitForGpu`, `BeginRenderStateFrame`,
  `CreateTextureHost`, `CreateSurfaceHost`
- `WaitForGpu` skips Dispatch's `RecordingMutex` (guest holds it across Run)
- PresentImpl still monolithic (blit+submit+swapchain); typed entry only

## Still uses std::function Run/Enqueue

- `ExecuteUpload` (arbitrary copy-queue lambdas)
- `TranslateGuestTexture` create path (Xenos info capture)

## Still next

- Split Present like Unleashed (`ExecuteCommandList` wait + guest present +
  `BeginCommandList`)
- Specialize Unlock/upload into POD; drop remaining `Run(fn)`
- Drop `RecordingMutex` once only render thread mutates GPU state
