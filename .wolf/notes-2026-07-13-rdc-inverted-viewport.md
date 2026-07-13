# 2026-07-13 — RenderDoc fm2mmgrok5.rdc: inverted viewport depth → black RT

## Capture
`C:\Users\Tera\Documents\GitHub\renderdoccaps\fm2mmgrok5.rdc` (D3D12, 20 draws)

## Evidence
- Scene RT `ResourceId::325` after last draw (EID 250): **100% near-black** (max RGBA=0).
- Swapchain after present blit (EID 284): also 100% black (samples black aperture `355`).
- Draws EID 64 / 108: **SamplesPassed=0** with `depth_clip=true`.
- Viewport on those draws: `minDepth=1.0`, `maxDepth=0.0` (empty D3D12 depth range).
- Present blit PS sampler showed Null (secondary; RT was already empty).

## Cause
Guest reverse-Z sets `minZ > maxZ`. `ProcSetViewport` correctly sets
`SPEC_CONSTANT_REVERSE_Z`, but `FlushRenderState` passed the inverted range
straight to `setViewports`. Unleashed swaps min/max when applying the host
viewport.

## Fix
In `FlushRenderState` viewport flush: if `vp.minDepth > vp.maxDepth`, swap
before `setViewports` (keep SPEC_CONSTANT for shaders).
