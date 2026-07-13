# Black + intermittent startup (2026-07-12 evening)

## Black screen (still)
Aperture present is correct (fbBase 0x096A7000). Create-time tile grow landed
(`7a119518`: host 1280x720). If still black, next is 080plume band srcRect
rebasing + immediate band flush (not mid-CL ResizeTileSurface).

## Intermittent startup
Often stalls ~2s after first Swap (resolve n~24, DebugOptions.ini). Sometimes
crashes (CrashDumps). Improvements landed:
- TryPresent no longer calls Video::Present (Swap-only; avoids g_presentBusy race)
- CreateTranslatedTextureHost dispatches without RecordingMutex (avoids deadlock
  vs Present fence wait)

Still flaky (~3/5 reach Swap 301 in smoke tests). Device lost still possible
on TranslateGuestTexture after INVALID_CALL.
