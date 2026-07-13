# Black + intermittent startup (2026-07-12 evening)

## Black screen (still)
Aperture present is correct (fbBase 0x096A7000) but the host texture is empty:
tile RTs are 1280x256; FM2 records one pass that hardware would replay per band.
Naive ResizeTileSurface mid-CL caused DXGI_ERROR_INVALID_CALL / crashes — needs
safer port (discard-after-grow, retire-after-fence) from ReXGlue080plume.

## Intermittent startup
Often stalls ~2s after first Swap (resolve n~24, DebugOptions.ini). Sometimes
crashes (CrashDumps). Improvements landed:
- TryPresent no longer calls Video::Present (Swap-only; avoids g_presentBusy race)
- CreateTranslatedTextureHost dispatches without RecordingMutex (avoids deadlock
  vs Present fence wait)

Still flaky (~3/5 reach Swap 301 in smoke tests). Device lost still possible
on TranslateGuestTexture after INVALID_CALL.
