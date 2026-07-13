# Create-time tile grow (2026-07-12 night)

## Change
`ProcCreateSurfaceHost`: guest `1280x256` EDRAM tiles allocate host `1280x720`
(`tileGrownFromHeight=256`). `FlushRenderState` expands matching viewport/scissor
to full host height. Swap no longer sync-`BeginRenderStateFrame` after Present
(Present already opens the next CL).

## Why not mid-CL ResizeTileSurface
Prior mid-CL grow → `DXGI_ERROR_INVALID_CALL` / DEVICE_REMOVED.

## Smoke
~3/8 reach Swap 301; aperture present `1280x720`. Still intermittent Swap-1
stall after resolve ~24 / DebugOptions.ini. Black may remain if draws/resolves
do not fill the grown RT (080plume also rebases band srcRects + immediate
band flush; create-time grow is Fix #18 create-side only).

## Do not retry without evidence
- Exempt `CopyTextureFromUpload` from `RecordingMutex` / guest-thread direct copy
- Release `RecordingMutex` across DXGI present + fence wait (0/10 OK)
