# 2026-07-12 — POD RenderCommand + Proc* (slice 2: Clear/Draw)

## Landed

- POD commands: `Clear`, `ResolveToTexture`, `DrawPrimitive`,
  `DrawIndexedPrimitive`, `DrawPrimitiveUP`
- Guest Clear/Resolve/Draw/DrawUP/DrawIndexed now `Enqueue` (async);
  Present's `Run` still drains FIFO before submit
- `DrawIndexedVertices` API — one command for flush+draw (replaces hook's
  nested `Run`)

## Still next

- Convert Present / WaitForGPU / Create* / ExecuteUpload to POD
- Optional moodycamel `BlockingConcurrentQueue`
- Drop `RecordingMutex` once only render thread touches GPU state
