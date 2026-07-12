# 2026-07-12 — Black screen after audio works (post-POD queue)

## Symptom
User: game audio plays; screen still black. POD RenderQueue work was
architectural (not expected to fix pixels).

## Prior evidence (still relevant)
Present/blit path was already alive (`blitting present-source 1280x720`).
Black = empty/wrong RT content or DEVICE_REMOVED shortly after boot.
See `notes-2026-07-12-fm2-black-screen.md`.

## Next (priority)
1. Fresh run log triage: device-lost latch? blit vs fallback clear? pipeline rejects? Translate create fails?
2. If DEVICE_REMOVED: find first killer (DRED / stderr GetDeviceRemovedReason) — likely bad upload/draw/barrier.
3. If GPU stays healthy: prove draws write the RT (magenta Clear / RenderDoc) then constants/samplers/resolve.
