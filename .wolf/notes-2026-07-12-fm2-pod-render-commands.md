# 2026-07-12 — POD RenderCommand + Proc* (slice 1)

## Landed

- `render_commands.h`: POD `RenderCommandType` / `RenderCommand` union
- `RenderQueue::Enqueue(const RenderCommand&)` alongside `std::function` path
- `DispatchRenderCommand` → Proc* for: DestructResource, SetViewport/Scissor,
  SetRT/ImplicitRT/DS, SetRenderState, SetTexture/Base, shaders, decl,
  stream/indices
- SetDepthState / SetStencilState / SetViewportEnable also Enqueued (fn for now)
- Fixed SetRenderTarget null→implicit resolve to happen on render thread
  (was racing `g_implicitRenderTarget` on guest)

## Still next

- Convert Clear / Draw / Present / Create / DrawUP to POD (drop most `Run(fn)`)
- Optional moodycamel `BlockingConcurrentQueue` like Unleashed
- Drop `RecordingMutex` once only render thread touches GPU state
- After codegen: rename AddRef/Release hooks to `D3DResource_*`
