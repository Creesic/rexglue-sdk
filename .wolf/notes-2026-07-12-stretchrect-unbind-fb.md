# Unleashed StretchRect: unbind FB before drain (2026-07-12)

## Fix
`FlushPendingStretchRectCommands` now `setFramebuffer(nullptr)` before
barriers/copies — Unleashed leaves COLOR_WRITE before StretchRect sample/blit;
our 1x `copyTextureRegion` was running while the tile RT was still bound.

`ProcResolveToTexture` falls back to `g_lastPresentableRenderTarget` when
`g_renderTarget` is null (common around Swap).

## Evidence
Logs: `ResolveToTexture: deferred StretchRect 1280x720 -> 1280x720` (n=24),
`FlushPendingStretchRect: draining`, `nosrc=0`. Still intermittent Swap-1.

## Not done (stay Unleashed-clean)
No 080plume band rebasing / tile viewport offset / mid-CL resize.
Optional later: Unleashed shader-blit StretchRect instead of copyTextureRegion.
