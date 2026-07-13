# Direction: Unleashed-clean, not 080plume (2026-07-12)

User: stay away from more ReXGlue080plume ports; keep FM2 renderer clean like UnleashedRecomp.

## Unleashed patterns we already follow
- Render-thread POD queue + `RecordingMutex`
- Deferred StretchRect (`sourceSurface` / `destinationTextures` / flush before Present+draw)
- Present split: ExecuteCommandList → guest present → BeginCommandList
- SetTexture / TranslateGuestTexture / MSAA resolve vs 1x copy

## Unleashed does NOT have
- `ResizeTileSurface`, `g_tileViewportOffsetY`, band destY rebasing, predicated-tiling state machine
- Sonic Unleashed is not FM2 EDRAM tiled the same way

## FM2-only exceptions (keep minimal)
- Create-time `1280x256` → host `1280x720` + VP/scissor expand = size override, not a tiling port
- Aperture present (XDK resolve dest keyed by header +32) if Unleashed present-from-RT is wrong for FM2 Swap

## Next black/startup work (Unleashed lens)
1. Prove deferred StretchRect actually copies into the aperture texture before Present
2. Stabilize Swap-1 without Present-mutex / copy-queue exemptions that already failed
3. Do not port 080plume band flush / tile offset machinery
