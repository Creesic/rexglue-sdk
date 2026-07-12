# 2026-07-12 — D3DResource AddRef/Release + Enqueue setters

## Landed

1. **`sub_82369D90` / `sub_82369E08`** hooked as AddRef/Release for FM2 resources.
   - Guest body uses BE `lwarx`/`stwcx` on `+4`; FM2 `GuestResource::refCount` is host-LE `std::atomic`.
   - Non-FM2 → `__imp__sub_*` original.
   - Ref→0 → `ScheduleResourceDestruction` (no guest `sub_82369868` free).
2. Manifest names queued for next codegen: `D3DResource_AddRef` / `D3DResource_Release`
   (hooks still use `sub_*` until regenerate).
3. **Enqueue** for SetVertexShader / SetPixelShader / SetVertexDeclaration /
   SetStreamSource / SetIndices (with live-resource checks).

## Still next

- Regenerate codegen → rename hooks to `D3DResource_*`
- POD `RenderCommand` + `Proc*`
- Drop `RecordingMutex` once all GPU mutations are queue-owned
- Shader MSAA resolve pipelines if hardware resolve insufficient
