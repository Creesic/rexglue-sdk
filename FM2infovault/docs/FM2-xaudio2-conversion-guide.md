# XAudio2 Conversion Guide (ReXGlue, Windows)

Last updated: May 30, 2026

## Goal

Provide a reusable pattern to migrate ReXGlue projects from SDL audio output to XAudio2 on Windows, while preserving a safe fallback path.

This guide is repository-generic and avoids machine-specific directory assumptions.

## Scope

This covers:
1. Adding an XAudio2 backend (`AudioSystem` + `AudioDriver`).
2. Wiring backend selection in shared app/runtime code.
3. Optional title-level forcing during bring-up.
4. Build/deploy and verification workflow.

This does not require FM2-specific hooks.

## Files Involved

Core backend files:
- `include/rex/audio/xaudio2/xaudio2_audio_driver.h`
- `include/rex/audio/xaudio2/xaudio2_audio_system.h`
- `src/audio/xaudio2/xaudio2_audio_driver.cpp`
- `src/audio/xaudio2/xaudio2_audio_system.cpp`

Build wiring:
- `src/audio/CMakeLists.txt`

Runtime backend selection:
- `src/ui/rex_app.cpp`

Optional title-level override (example):
- `<title>/src/<title>_app.h` or equivalent app hook file

## Step 1: Add The XAudio2 Backend

Implement `XAudio2AudioSystem` and `XAudio2AudioDriver`.

Recommended driver behavior:
1. Dynamically load `XAudio2_9.dll` (fallback `XAudio2_8.dll`).
2. Create XAudio2 objects on an MTA thread with `CoInitializeEx(..., COINIT_MULTITHREADED)`.
3. Create mastering voice + source voice using `WAVEFORMATEXTENSIBLE` float format.
4. Submit guest frames with bounded queueing and release the client semaphore on `OnBufferEnd`.
5. Apply `Clock::guest_time_scalar()` via `SetFrequencyRatio`.

## Step 2: Wire XAudio2 In Audio CMake

In `src/audio/CMakeLists.txt`, add Windows-only sources:

```cmake
if (WIN32)
    target_sources(rexaudio PRIVATE
        xaudio2/xaudio2_audio_system.cpp
        xaudio2/xaudio2_audio_driver.cpp
    )
endif()
```

Also link `ole32` on Windows:

```cmake
if (WIN32)
    target_link_libraries(rexaudio PRIVATE ole32)
endif()
```

## Step 3: Add Shared Runtime Backend Selection

In `src/ui/rex_app.cpp`:
1. Include `nop`, `sdl`, and (Windows-only) `xaudio2` audio system headers.
2. Define selector cvar:

```cpp
REXCVAR_DEFINE_STRING(audio_backend, "sdl", "Audio", "Audio backend: sdl, xaudio2, nop")
    .allowed({"sdl", "xaudio2", "nop"});
```

3. In `SetupPresentation()`, map to factory:
- `nop` -> `NopAudioSystem`
- `xaudio2` -> `XAudio2AudioSystem` (Windows only)
- default -> `SDLAudioSystem`

## Step 4: Optional Title-Level Force During Bring-Up

If needed, force XAudio2 in your title app hook while validating:

```cpp
void OnPreSetup(rex::RuntimeConfig& config) override {
#if REX_PLATFORM_WIN32
  config.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
#else
  config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
#endif
}
```

This keeps migration risk local to one title until fully validated.

## Step 5: Build And Deploy

From `<repo_root>`:

```powershell
cmake --build --preset win-amd64-relwithdebinfo --target install
```

If your title executable uses a local copy of `rexruntimerd.dll`, copy the fresh DLL from:
- `<repo_root>/out/install/win-amd64/bin/rexruntimerd.dll`

to your title build output directory before testing.

Then rebuild your title target (example):

```powershell
cmake --build --preset win-amd64-relwithdebinfo --target <title_target>
```

Important:
1. If DLL copy fails, close running title executables (file lock).
2. Always deploy a fresh runtime DLL before evaluating behavior.

## Step 6: Verify The Active Backend

Use at least one hard check:
1. Process module check: confirm `XAudio2_9.dll` (or `_8`) is loaded by your title process.
2. Runtime logs: confirm XAudio2 init path succeeds and no `CreateSourceVoice`/DLL-load errors appear.
3. Functional validation: menu/gameplay audio is real-time and stable.

## Troubleshooting

If no audio:
1. Check XAudio2 DLL load failure logs (`XAudio2_9.dll` / `XAudio2_8.dll`).
2. Confirm `CreateMasteringVoice` and `CreateSourceVoice` succeed.
3. Confirm the title is loading the freshly built runtime DLL.

If still on SDL unexpectedly:
1. Confirm backend-selection code is in the actual compiled `rex_app.cpp`.
2. Temporarily force XAudio2 at title-level `OnPreSetup`.

If pitch/speed is wrong:
1. Confirm `SetFrequencyRatio(Clock::guest_time_scalar())` is applied.
2. Confirm channel/frame conversion path is correct before submit.

## Recommended Rollout Pattern

1. Merge backend implementation + shared selector.
2. Validate per-title with temporary force override.
3. Keep SDL fallback for safety.
4. Remove temporary diagnostic markers after verification.
5. Document known-good title configurations in project docs.
