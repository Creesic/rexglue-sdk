# Adding the XAudio2 Audio Backend

Ported from `ReXGlue080plume/docs/FM2-xaudio2-conversion-guide.md` (renamed —
despite the old filename, nothing here is FM2-specific; it's a generic SDK
audio backend). That version described a backend that was unconditionally
compiled into every Windows build of `rexaudio` and unconditionally wired
into the shared `audio_backend` cvar dispatch. This version documents what
was actually built instead: **xaudio2 is opt-in**, following the same pattern
as GPU emulation plugins (`GPU_PLUGINS xenos`) — a project only pays for it,
and only gets `--audio_backend=xaudio2`, if it explicitly asks for it in its
own `CMakeLists.txt`.

## Files Involved

Backend implementation:
- `include/rex/audio/xaudio2/xaudio2_audio_driver.h`
- `include/rex/audio/xaudio2/xaudio2_audio_system.h`
- `src/audio/xaudio2/xaudio2_audio_driver.cpp`
- `src/audio/xaudio2/xaudio2_audio_system.cpp`

Build wiring (SDK-side, one-time):
- `src/audio/CMakeLists.txt` — defines the opt-in `rexaudio-xaudio2` target
- `cmake/rexglue_helpers.cmake` — `rexglue_configure_target()`'s `AUDIO_BACKENDS` argument
- `cmake/rexglue_install.cmake` — exports the target for installed-SDK (`find_package`) consumers

Runtime backend selection (SDK-side, one-time):
- `src/ui/rex_app.cpp`

Per-project opt-in (each downstream project that wants it):
- `<project>/CMakeLists.txt` — one word added to an existing `rexglue_setup_target()` call

## Step 1: The Backend Itself

`XAudio2AudioSystem`/`XAudio2AudioDriver` implement the SDK's `AudioSystem`/`AudioDriver`
interfaces exactly like the `sdl` and `nop` backends do — no changes needed there
if you're porting from elsewhere, since those base interfaces are stable.

Driver behavior:
1. Dynamically load `XAudio2_9.dll` (fallback `XAudio2_8.dll`) — never link an import lib.
2. Create XAudio2 objects on a dedicated MTA thread via `CoInitializeEx(..., COINIT_MULTITHREADED)`.
3. Create a mastering voice + source voice using a `WAVEFORMATEXTENSIBLE` float format.
4. Submit guest frames with bounded queueing (`kFrameCount`), releasing the
   client semaphore from `OnBufferEnd`.
5. Apply `Clock::guest_time_scalar()` via `SetFrequencyRatio` so playback speed
   tracks the guest's own clock scaling.
6. Optionally downmix the guest's 6-channel (5.1) output to stereo, gated by
   `audio_force_stereo_mix` (default on) and tunable via six
   `audio_stereo_downmix_*` coefficient cvars plus `audio_stereo_downmix_norm`.

**Deliberately left out** (present in the original bring-up version, removed
for the clean/upstream version): a per-second diagnostic block that logged
average/peak channel levels via `REXAPU_ERROR` — unconditionally, every
second, regardless of any flag, at the wrong log severity. It had no effect
on the audio path itself (pure telemetry) and existed only to debug a
specific FM2 issue. If you need that kind of instrumentation again, add it
back gated behind a cvar and at `INFO`/`DEBUG` level, not unconditionally at
`ERROR`.

## Step 2: Wire It Into `src/audio/CMakeLists.txt` — As An Opt-In Target

Do **not** add the sources directly to the `rexaudio` target. Give it its own
target instead, so consumers that don't ask for it never compile or link it:

```cmake
# XAudio2 backend (Windows-only). Opt-in via AUDIO_BACKENDS xaudio2 in
# rexglue_setup_target()/rexglue_configure_target().
if(WIN32)
    add_library(rexaudio-xaudio2 STATIC
        xaudio2/xaudio2_audio_system.cpp
        xaudio2/xaudio2_audio_driver.cpp
    )
    add_library(rex::audio-xaudio2 ALIAS rexaudio-xaudio2)
    set_target_properties(rexaudio-xaudio2 PROPERTIES EXCLUDE_FROM_ALL ON)

    target_include_directories(rexaudio-xaudio2 PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_libraries(rexaudio-xaudio2 PUBLIC rex::audio PRIVATE ole32)
endif()
```

`EXCLUDE_FROM_ALL` means it won't even build as part of the SDK's own default
build unless something links it. Export it for the installed-SDK path too
(`cmake/rexglue_install.cmake`):

```cmake
if(WIN32)
    set_target_properties(rexaudio-xaudio2 PROPERTIES EXPORT_NAME audio-xaudio2)
endif()
...
if(WIN32)
    list(APPEND REXGLUE_INSTALL_TARGETS rexaudio-xaudio2)
endif()
```

## Step 3: Teach `rexglue_configure_target()` About `AUDIO_BACKENDS`

In `cmake/rexglue_helpers.cmake`, add `AUDIO_BACKENDS` alongside the existing
`GPU_PLUGINS` multi-value argument, and — unlike GPU plugins, which are
runtime-loaded and need no compile-time signal — link the backend and set a
`REXGLUE_HAS_AUDIO_<NAME>` compile definition so the consumer's own
`rex_app.cpp` compilation knows the symbol exists:

```cmake
cmake_parse_arguments(ARG "" "" "GPU_PLUGINS;AUDIO_BACKENDS" ${ARGN})
...
foreach(_backend IN LISTS ARG_AUDIO_BACKENDS)
    if(TARGET rexaudio-${_backend})
        set(_backend_target rexaudio-${_backend})
    elseif(TARGET rex::audio-${_backend})
        set(_backend_target rex::audio-${_backend})
    else()
        message(FATAL_ERROR
            "rexglue_configure_target: unknown audio backend '${_backend}' "
            "(no target rexaudio-${_backend} or rex::audio-${_backend})")
    endif()
    target_link_libraries(${target_name} PRIVATE ${_backend_target})
    string(TOUPPER "${_backend}" _backend_upper)
    target_compile_definitions(${target_name} PRIVATE REXGLUE_HAS_AUDIO_${_backend_upper})
endforeach()
```

## Step 4: Shared Runtime Backend Selection (compile-time gated)

`rex_app.cpp` is compiled directly into *every* consumer target (it's not a
shared library), including ones that never opt into xaudio2. So the
`xaudio2` branch — and its `#include` — must be gated behind the compile
definition Step 3 sets, not just `REX_PLATFORM_WIN32`:

```cpp
#include <rex/audio/nop/nop_audio_system.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#if defined(REXGLUE_HAS_AUDIO_XAUDIO2)
#include <rex/audio/xaudio2/xaudio2_audio_system.h>
#endif

#if defined(REXGLUE_HAS_AUDIO_XAUDIO2)
REXCVAR_DEFINE_STRING(audio_backend, "sdl", "Audio", "Audio backend: sdl, xaudio2, nop")
    .allowed({"sdl", "xaudio2", "nop"});
#else
REXCVAR_DEFINE_STRING(audio_backend, "sdl", "Audio", "Audio backend: sdl, nop")
    .allowed({"sdl", "nop"});
#endif
```

Dispatch in `SetupPresentation()`:

```cpp
const auto backend = REXCVAR_GET(audio_backend);
if (backend == "nop") {
  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::nop::NopAudioSystem);
}
#if defined(REXGLUE_HAS_AUDIO_XAUDIO2)
else if (backend == "xaudio2") {
  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
}
#endif
else {
  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
}
```

Default stays `sdl` either way — opting in only adds a choice, it never
changes what a project gets if it doesn't ask.

## Step 5: Opt In From Your Project

One line in `<project>/CMakeLists.txt`:

```cmake
rexglue_setup_target(fm2 GPU_PLUGINS xenos AUDIO_BACKENDS xaudio2)
```

That's the entire per-project cost. No title-level C++ override is needed
just to make `--audio_backend=xaudio2` work — the cvar dispatch in Step 4
handles it once the compile definition is set.

If you want to *force* xaudio2 regardless of the cvar (e.g. during bring-up,
to rule out the cvar path itself), you can still override it in your title's
`OnPreSetup`, but you still need the `AUDIO_BACKENDS xaudio2` opt-in for the
symbol to exist:

```cpp
void OnPreSetup(rex::RuntimeConfig& config) override {
#if defined(REXGLUE_HAS_AUDIO_XAUDIO2)
  config.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
#endif
}
```

## Step 6: Build

In-tree (`REXSDK_DIR` pointing at the SDK source, as FM2 does):

```powershell
cmake --build FM2/out/build/win-amd64-relwithdebinfo --config RelWithDebInfo --target fm2
```

`rexaudio-xaudio2` builds automatically as a dependency once your target
links it — no separate build step, no DLL to stage (it's statically linked,
unlike GPU plugins).

For an **installed SDK** consumer (`find_package(rexglue)` instead of
`add_subdirectory`), install the SDK first so the exported
`rex::audio-xaudio2` target is available:

```powershell
cmake --build --preset win-amd64-relwithdebinfo --target install
```

## Step 7: Verify The Active Backend

Launch with `--audio_backend=xaudio2` and check the log for, in order:

```
XAudio2AudioDriver init: <freq> Hz, in_ch=6, out_ch=<2 or 6>, frame_size=<n> bytes
XAudio2 source voice ready: <freq> Hz, in_ch=6, out_ch=<2 or 6>, samples_per_frame=<n>
XAudio2AudioDriver init complete
```

If `out_ch=2`, the 5.1→stereo downmix is active (`audio_force_stereo_mix`
default). Launch again with no `--audio_backend` flag and confirm those three
lines are *absent* — that confirms the default still falls through to `sdl`
and opting in didn't change behavior for anyone who doesn't ask for it.

## Troubleshooting

No audio / init fails:
1. Check for `Failed to load XAudio2 runtime DLL` — means neither
   `XAudio2_9.dll` nor `XAudio2_8.dll` is present on the system.
2. Check for `CreateMasteringVoice`/`CreateSourceVoice` failure HRESULTs in the log.

`--audio_backend=xaudio2` silently falls back to `sdl`:
1. Confirm your project's `rexglue_setup_target()` call actually includes
   `AUDIO_BACKENDS xaudio2` — without it, `REXGLUE_HAS_AUDIO_XAUDIO2` is never
   defined, the `xaudio2` branch doesn't exist in your build, and the cvar's
   `.allowed()` list won't even include `"xaudio2"`.
2. Reconfigure (not just rebuild) after changing `CMakeLists.txt` — CMake
   needs to re-run to pick up the new argument.

Pitch/speed wrong:
1. Confirm `SetFrequencyRatio(Clock::guest_time_scalar())` is still being
   called every `SubmitFrame`.
2. Confirm the channel/downmix conversion path matches the guest's actual
   channel count (`frame_channels_`).
