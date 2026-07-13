# 2026-07-12 — StretchRect format mismatch → DEVICE_REMOVED / black screen

## Cause
Deferred StretchRect (`ExecutePendingStretchRectCommands`) used `copyTextureRegion`
from scene RTs (`R16G16B16A16_*`) to resolve-aperture frontbuffers (`B8G8R8A8_*`).
D3D12 forbids that → `DXGI_ERROR_INVALID_CALL` (0x887A0001) → device lost latch →
black presents.

Confirmed earlier via temporary D3D12 debug-layer InfoQueue (since reverted;
RelWithDebInfo has `NDEBUG`).

## Fix
- Skip GPU copy/resolve when `src.format != dst.format`.
- On skip, PreferStretchRectSourceForPresent (may be overwritten by Swap).
- **Present selection:** if aperture format ≠ sticky scene RT format, blit the
  sticky RT (`sticky-rt-fmt`) — Swap re-sets empty aperture after mid-frame skips.
- Always drain pending StretchRect even when no copy barriers were needed.
- Same guard on immediate `ProcResolveToTexture` region path.
- `video.cpp`: clear framebuffers before swapchain resize; sync size at Init.

## Verify (2026-07-12/13 smoke)
- 3/3: Swap 301, **no** `GPU device lost latch`.
- When HDR resolve hits: `skip incompatible copy fmt=10 → fmt=24`, then
  `present kind=stretch-src` (sticky override until a compatible copy lands).
- Runs without HDR resolve still present `aperture` (same-format StretchRect OK).
