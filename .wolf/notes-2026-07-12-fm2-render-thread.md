# 2026-07-12 — FM2 dedicated render thread (Phase A–C)

## Done

### Phase A
- `CreateTexture`: depth → `DEPTH_TARGET`; `usage != 0` → `RENDER_TARGET`; else `NONE` (`d3d_resource_hooks.cpp`).
- One-shot `g_deviceLost` + `NoteDeviceLost` / `IsDeviceLost`; create failures early-out; plume still prints `GetDeviceRemovedReason` on create/fence fail.
- Present storm: `g_presentBusy` drops overlapping `Video::Present` while previous still in flight.

### Phase B
- `render_queue.{h,cpp}`: dedicated thread + sync `RenderQueue::Run` (inline if already on render thread / queue stopped).
- Live GPU path marshaled onto render thread: `Clear`, `ResolveToTexture` (StretchRect), `Draw*`, `BeginRenderStateFrame`, `Video::Present` / `WaitForGPU`.
- `DrawIndexedVerticesHook`: Flush + Draw in one `Run` job.
- Started from `Video::Init`, stopped in shutdown.

### Phase C
- `kNumFrames = 2` command lists/fences/semaphores; Present advances slot and waits only on the slot about to be reused.
- StretchRect/`ResolveToTexture` on render thread.

## Build
`cmake --build FM2/out/build/win-amd64-relwithdebinfo --target fm2` OK after these changes.
