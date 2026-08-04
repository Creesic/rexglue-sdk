# ReXGlue 0.9.0 migration (2026-08-04)

## Upstream baseline

- Official release: `rexglue/rexglue-sdk` tag `v0.9.0` (`3eb9b511`).
- Previous FM2 branch merge base: `cd778a8b`, immediately before the `v0.8.0`
  tag.
- The 0.9.0 tag is merged into `FM2_WIN_Plume`; the existing FM2 and runtime
  commits remain on their original side of the merge.

## Integration decisions

- Adopted the 0.9.0 runtime-loaded Xenos GPU plugin and SDL3 window/event
  backend.
- FM2 requests and stages the `xenos` plugin for modes that use ReX graphics.
  Plume-only mode explicitly clears the plugin and continues to own graphics.
- Preserved the selectable SDL/XAudio2/NOP audio backends, FM2 audio/FPS
  overlays, keyboard/mouse tracing without forced cursor capture, same-guest-CPU
  spinlock yield, and the Windows high-resolution timer/MMCSS requests.
- Moved the normal user-close diagnostic from the retired Win32 window
  implementation to the SDL close-request path.
- Combined the 0.9.0 DLL `PPCImageInfo` validation with the existing portable
  shared-library filename and executable-directory lookup.
- Adopted 0.9.0 unconditional generated-file writes, XMA decoder reset on
  context release, metadata root, achievement UI, and close lifecycle hooks.
- Updated the FM2 XMA diagnostics for the 0.9.0 context layout, which no longer
  exposes `loop_subframe_start` as a separate decoder snapshot field.
- Fixed the 0.9.0 migration scanner on Windows so CRLF source files are
  rewritten without changing their line endings, generated CMake drift ignores
  LF/CRLF-only differences, and legacy config basenames are recognized after a
  path separator.
- Kept FM2 shader-analysis unit coverage by compiling the analysis sources into
  the test executable. The Xenos GPU implementation itself now lives in the
  separately loaded `rexgpu-xenos` plugin.
- Removed the capped `FM2_VSASM` diagnostic from
  `FM2/src/render/d3d_resource_hooks.cpp`; it directly constructed the now
  plugin-private `rex::graphics::Shader` type and was not part of the rendering
  path.

## Title migration

- `FM2/fm2_manifest.toml` now pins `sdk_version = "0.9.0"`.
- `FM2/CMakeLists.txt` calls
  `rexglue_setup_target(fm2 GPU_PLUGINS xenos)`.
- Regenerated `FM2/generated/` with the 0.9.0 `fm2_codegen` target so generated
  SDK glue matches the new runtime ABI and plugin staging helper.

## Verification

- Full SDK/runtime `install` build completed with the throttled CMake launcher.
- FM2 codegen completed against `default.xex` using ReXGlue 0.9.0.
- FM2 RelWithDebInfo build completed and staged `fm2.exe`, `rexruntimerd.dll`,
  and `rexgpu-xenosrd.dll` together.
- The complete CTest run executed 1,775 tests. All 1,458 PPC tests passed. The
  0.9.0 migration/template fixture failures found by that run were corrected;
  final results are recorded in the merge handoff after restoring the user's
  pre-existing test edits.
- Live gameplay validation remains separate from build and automated-test
  evidence.
