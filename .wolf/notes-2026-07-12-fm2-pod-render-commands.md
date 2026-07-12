# 2026-07-12 — POD RenderCommand + Proc* (slice 6: depth/stencil/viewport)

## Landed

- POD Unlock / upload / Translate create (slice 5) — `ExecuteUpload` gone
- POD `SetViewportEnable` / `SetDepthState` / `SetStencilState` — last
  `Enqueue(fn)` lambdas in render/ retired

## Still next

- Optional async `Enqueue`+atomic wait for ExecuteCommandList (needs
  `g_readyForCommands` gate)
- Drop `RecordingMutex` once only render thread mutates GPU state
- DDS `LoadTextureFromMemory` create still on caller thread (upload is POD)

## Also landed (slice 7)

- Deleted `Run(fn)` / `Enqueue(fn)` — queue is POD-only
