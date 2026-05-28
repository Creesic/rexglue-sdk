# FM2 on macOS (rexsglue-sdk v0.8.0) — Handoff Document

## Project Status
FM2 built and runs on macOS ARM64 (Apple M1 Max, macOS 26.3.1) via the Metal4 backend. The GPU initializes, MetalProvider connects at 2560x1440, VSync fires at 60Hz, the ring buffer processes packets — **but the game is blocked on audio file loading**, preventing any draws from reaching the screen.

## Build System
- **Generator**: Ninja (Xcode generator has a fundamental bug with duplicate source filenames in bundled OBJECT libraries)
- **Compiler**: Apple Clang (Xcode default) — CMakeLists.txt patched to accept `AppleClang` in addition to `Clang`
- **SDK**: Built via `add_subdirectory` from FM2's CMake (`REXSDK_DIR`), not find_package
- **Build command**: `cmake --build /Users/tera/Documents/GitHub/rexglue-sdk/FM2/out/build/mac-arm64-relwithdebinfo --target fm2`
- **Binary**: `FM2/out/build/mac-arm64-relwithdebinfo/fm2` (117MB ARM64 binary)

## Key Patches Made to rexglue-sdk v0.8.0

### 1. `CMakeLists.txt` — Accept AppleClang (line 83)
```cmake
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
```
Without this, SDK's `FATAL_ERROR` on AppleClang blocks building with Xcode's compiler.

### 2. `cmake/rexglue_helpers.cmake` — macOS entry point (line 186)
Added `elseif(APPLE)` branch to use `windowed_app_main_mac.mm` instead of the GTK-based `windowed_app_main_posix.cpp`. Without this, macOS builds tried to `#include <gtk/gtk.h>` and failed.

### 3. `src/core/CMakeLists.txt` — Add `system_posix.cpp` to Apple sources (line 62)
The Apple platform sources in rexcore didn't include `system_posix.cpp`, which provides `ShowSimpleMessageBox()`. Without it, `rex_app.cpp` fails to link when `rexruntime` is compiled as OBJECT.

### 4. `FM2/CMakeLists.txt` — Add OBJC/OBJCXX languages (line 8)
```cmake
project(fm2 LANGUAGES C CXX OBJC OBJCXX)
```
Ninja generator needs explicit OBJCXX language for `.mm` files.

### 5. `FM2/generated/rexglue.cmake` — Remove extra library links (line 43)
Changed `target_link_libraries` from `rex::core rex::system rex::kernel rex::graphics rex::ui` to just `rex::system`. This prevents duplicate ObjC classes (`RexMetalView`, `RexWindowDelegate`) being compiled into both the fm2 binary and `librexruntimerd.dylib`, which caused ObjC runtime warnings and potential crashes.

### 6. `src/kernel/CMakeLists.txt` — PUBLIC link for rexgraphics
Changed `rexgraphics` from `PRIVATE` to `PUBLIC` so that `REX_HAS_METAL=1` propagates to the fm2 binary's `rex_app.cpp`. Without this, `SetupPresentation()` sees no `#if` branch active and creates a null graphics config.

### 7. `src/kernel/xboxkrnl/xboxkrnl_audio_xma.cpp` — Null context guards
Added `XMA_GUARD_CONTEXT` macro to all 20 XMA entry functions. FM2's guest code calls XMA functions (e.g., `XMADisableContext`, `XMAEnableContext`, `XMAIsInputBuffer0Valid`) with null context pointers, causing null dereference crashes.

### 8. `src/audio/xma_decoder.cpp` — Null context guard in `BlockOnContext` (line 229)
Changed from `assert_true(context_id >= 0)` to `if (context_id < 0) { return true; }`. Prevents `contexts_[-1]` access.

### 9. `src/filesystem/virtual_file_system.cpp` — Bare path resolution (line 122)
When a bare filename (no device prefix like `game:\`) is given, prepend `game:\` and retry. FM2's FMOD implementation uses relative paths without device prefixes.

## Current Blocking Issue

**The game runs but doesn't show anything on screen.** The VFS bare-path fix (patch #9) is now working — `Placeholder.fsb`, `UIFrontEnd.fsb`, and `UIInGame.fsb` resolve correctly through `game:\`. FMOD audio file loading succeeds.

The game boots, initializes the Metal4 backend, creates a window, and issues 5 draws successfully. However, the game never calls `XE_SWAP` (PM4_XE_SWAP) to present the rendered frame to the screen. The presenter reports "No guest output texture" because `IssueSwap` is never called.

The game enters a tight loop polling register 0xA31 (waiting for bit 31 to clear), which immediately succeeds (unknown register returns 0). The game continues running indefinitely without ever swapping.

**Root cause**: FM2's boot sequence draws some initial content, then waits for a condition (possibly audio initialization completion, a timer, or another thread) before calling the first swap. The exact blocker preventing the first XE_SWAP call needs investigation.

## Audio Files
The `.fsb` files are now accessible via the VFS bare-path fix. No audio-related VFS errors in the log.

## SDK Build Status
The SDK builds successfully after the following fixes:

### 10. `src/filesystem/virtual_file_system.cpp` — Bare path fix (line 123)
The original patch used `utf8_contains()` which doesn't exist. Changed to `utf8_find_first_of(normalized_path, ":") == string_view::npos`.

### 11. `src/kernel/CMakeLists.txt` — PRIVATE link for rexgraphics (line 69)
Changed `rexruntime` from PUBLIC to PRIVATE link for `rexgraphics` and manually propagate compile definitions:
```cmake
target_link_libraries(rexruntime PRIVATE rexgraphics o1heap rexcore rexfilesystem rexui rexaudio rexinput imgui)
if(REXGLUE_USE_METAL)
    target_compile_definitions(rexruntime PUBLIC REX_HAS_METAL=1)
endif()
```

### 12. `cmake/rexglue_install.cmake` — Conditional transitive deps
Added `if(TARGET ...)` guards for conditionally-defined targets in the install export set.

### 13. Metal shader converter dylibs
`libmetalirconverter.dylib` and `libdxilconv.dylib` must be copied to the output directory (${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) for the runtime to load. They live in `thirdparty/metal-shader-converter/lib/` and `thirdparty/dxilconv/lib/`.

## Xcode Project
- Location: `FM2/out/build/fm2-xcode/fm2.xcodeproj`
- The fm2 scheme has working directory set to `FM2/`
- **Do not build from Xcode** — the Xcode generator produces broken linker commands due to hashed `.o` filenames for duplicate sources
- Use Xcode only for code browsing or attaching to the Ninja-built process for debugging

## How to Build & Run
```bash
# Build SDK runtime
cmake -S /Users/tera/Documents/GitHub/rexglue-sdk -B /Users/tera/Documents/GitHub/rexglue-sdk/out/build/mac-arm64-relwithdebinfo \
  -G Ninja -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /Users/tera/Documents/GitHub/rexglue-sdk/out/build/mac-arm64-relwithdebinfo --target rexruntime

# Build FM2 (clean)
rm -rf FM2/out/build/mac-arm64-relwithdebinfo
cmake -S FM2 -B FM2/out/build/mac-arm64-relwithdebinfo \
  -G Ninja -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DREXSDK_DIR=/Users/tera/Documents/GitHub/rexglue-sdk
cmake --build FM2/out/build/mac-arm64-relwithdebinfo --target fm2

# Run
FM2/out/build/mac-arm64-relwithdebinfo/fm2
```

## What Works
- Metal4 backend: initializes MTL4 queue, creates arg tables, renders
- Presenter: SetWindowSurfaceFromUIThread, MetalPresenter connected at 2560x1440
- VFS: mounts assets at `game:\`, resolves game:\default.xex
- VFS: bare filename resolution (Placeholder.fsb etc.) via `game:\` prefix
- CPU: recompiled code executes, WAIT_REG_MEM advances
- GPU: ring buffer initialized, packets dispatched, CP processes draws (5+ draws issued)
- XMA audio stubs: null context guards prevent crashes
- FMOD audio: .fsb files load successfully (no VFS errors)

## What Doesn't Work
- **No visible output** — game never calls XE_SWAP to present frames
- **No audio output** — FMOD loads files but audio playback not verified
- `cache:\` device — not mounted (expected, needs user data dir)
- `update:\` device — not mounted (expected)
