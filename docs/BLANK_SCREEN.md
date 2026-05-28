# FM2 Blank Screen — Root Cause Analysis

## Summary
FM2's GPU system initializes correctly (Metal device created, presenter connected, ring buffer running at 60Hz VSync), but **zero `DRAW` packets reach the GPU**. The game thread stalls during startup waiting for audio assets to load, and never reaches its render loop.

## Why It Doesn't Draw

### Blocking: FMOD Audio File Resolution
The game uses FMOD for audio. FMOD opens files with **bare filenames** — no device prefix:
```
NtCreateFile('Placeholder.fsb')      → 0xC000000F (file not found)
NtCreateFile('UIFrontEnd.fsb')       → 0xC000000F
NtCreateFile('UIInGame.fsb')         → 0xC000000F
```

The VFS resolver (`virtual_file_system.cpp:105`) fails because no device mount path matches a bare filename. These files exist at `game:\Media\Audio\Placeholder\Placeholder.fsb` (mounted from `FM2/assets/Media/Audio/…`) but the guest FMOD code doesn't prepend `mediaPath="Game:\Media\Audio"` from `AudioEngineConfig.xml`.

**Fix**: Patch `ResolvePath()` to prepend `game:\` when no device prefix is found. This was implemented but not yet tested because the SDK build has an unrelated CMake install-export issue.

### The v0.8.0 Regression (Already Fixed)
The update from v0.7.x to v0.8.0 introduced a blank screen because `REX_HAS_METAL=1` stopped propagating to the consumer binary. In v0.7.x, `rexgraphics` was linked `PUBLIC` from `rexruntime`, so the compile definition propagated transitively. In v0.8.0 this was changed to `PRIVATE` (to fix install-export errors with `imgui`).

**Fix**: Keep `rexgraphics` as `PRIVATE` but explicitly re-declare backend defines:
```cmake
target_compile_definitions(rexruntime PUBLIC REX_HAS_METAL=1)
```
This is already done in `src/kernel/CMakeLists.txt:76`.

### Non-Blocking Issues (Can Ignore)
- XMA null-context crashes in `xboxkrnl_audio_xma.cpp` — already fixed with null guards
- Duplicate ObjC class `RexMetalView` in `RexWindowDelegate` — fixed by linking only `rex::system`, not individual object libs
- `cache:\` and `update:\` device-not-found warnings — non-fatal, logged by guest code

## Next Step
Build and run FM2 with the VFS bare-path fix applied to confirm the `Placeholder.fsb` loads and the render loop starts producing DRAW packets.
