# FM2 Shader Probe — design

## Purpose

Debug tool to interactively find the correct pixel shader for a given on-screen
draw in the FM2 `plume_native` renderer. The renderer currently binds a
color-only pixel shader (no texture sample) to the press-start background/logo
draws, so they render black. Sampling shaders exist in the embedded cache
(`g_shaderCacheEntries`, 213 entries). This tool lets the user step through the
frame's draws, solo one, and cycle its pixel shader through every cache entry
until the draw renders correctly — revealing the shader that *should* be bound,
which then feeds root-cause analysis of why the wrong one was selected.

Opt-in and zero-cost when off. This is a debugging aid, not a shipping feature.

## Scope (YAGNI)

- IN: per-frame draw indexing, select+solo a draw, override its PS with any cache
  entry, keyboard control, one log line per change.
- OUT: magenta in-context tint (needs a dedicated embedded solid-color shader —
  deferred; solo covers the "which draw is selected" need), saving/persisting a
  found mapping, cycling vertex shaders as VS, any UI beyond keys + log.

## Integration point

All changes live in `FM2/src/render/render_state.cpp` (plus a cvar declaration).
The hook is the existing draw prologue — the function that does
`PipelineState pipelineState = g_pipelineState; ... GetPipeline(pipelineState);
setPipeline(...)` (the same site where the temporary `FM2_PSTEX` diagnostic was
added and reverted). It runs once per draw and has the `PipelineState`, the
render target, and `g_shaderCacheEntries` in scope. No changes to the shader
cache, `LoadShader`, or the guest `SetPixelShaderNative` hooks.

Draw-type coverage: FM2 has several draw entry points (indexed, non-indexed, and
UP variants). The probe must apply to whichever shared prologue records the draw
command so all press-start draws are indexable and skippable. The plan confirms
whether these entry points share one prologue (single hook) or need the override
+ solo-skip applied at each; the design intent is that every recorded draw is
counted, selectable, and overridable.

## State

- cvar `fm2_shader_probe` (bool, default `false`) — master enable.
- Runtime globals (file-static in render_state.cpp):
  - `g_probeSelectedDraw` (int, default 0) — which draw index is targeted.
  - `g_probePsIndex` (int, default 0) — cache entry index 0..212 to bind as PS.
  - `g_probeSolo` (bool, default true) — render only the selected draw.
  - `g_probeDrawCounter` (int) — per-frame draw index, reset to 0 in
    `BeginRenderStateFrame`.
  - `g_probeLastFrameDrawCount` (int) — captured at frame end for clamping.

When `fm2_shader_probe` is false, the only added cost is one boolean check per
draw; all probe logic is skipped.

## Behavior

Per draw, in the prologue (probe enabled only):
1. `drawIdx = g_probeDrawCounter++`.
2. If `g_probeSolo` and `drawIdx != g_probeSelectedDraw`: skip recording this
   draw (do not bind pipeline / do not emit the draw command). Implemented by
   an early return in the draw-recording function before the
   `drawIndexed/draw` call, guarded by the probe flag.
3. If `drawIdx == g_probeSelectedDraw`: set `pipelineState.pixelShader =
   ProbeShaderForIndex(g_probePsIndex)` before `GetPipeline`. The rest of the
   pipeline (VS, constants, textures, RT) is untouched, so a *sampling* trial
   shader will read the draw's real bindless `texture2DIndices`.

`ProbeShaderForIndex(i)`: returns a cached `GuestShader*` (one per index, created
lazily and reused) whose `shaderCacheEntry = &g_shaderCacheEntries[i]`, `type =
PixelShader`. `LoadShader` then pulls that entry's DXIL, using
`entry.spec_constants_mask`. If entry `i` is actually a vertex shader, PSO
creation fails; the existing null-pipeline guard skips the draw and we log
`psoOk=0` so the user steps past it.

## Controls

Polled once per frame via `GetAsyncKeyState`, edge-detected against a static
prev-state, and acted on only when the FM2 window is foreground
(`GetForegroundWindow`). Polling happens in `BeginRenderStateFrame`.

- `F6` / `F7` — selected draw −1 / +1, clamped to `[0, lastFrameDrawCount-1]`.
- `F8` / `F9` — PS index −1 / +1, wrapping in `[0, 212]`.
- `F5` — toggle solo.

On any change, append one line to `C:\temp\fm2-clean.log`:
```
PROBE draw=<sel>/<count> solo=<0|1> psIndex=<i> hash=0x%016llX file=<entry.filename> psoOk=<0|1>
```
(`psoOk` reflects whether the last frame's override built a pipeline.)

## Data flow

`GetAsyncKeyState` (BeginRenderStateFrame) → mutates probe globals + logs →
draw prologue reads globals → overrides `pipelineState.pixelShader` for the
selected draw and/or skips non-selected draws → `GetPipeline`/`setPipeline` →
draw recorded (or skipped) → user observes on screen.

## Edge cases

- Probe off: one bool check per draw, nothing else.
- `selectedDraw` beyond current frame's draw count: clamp on read.
- Invalid trial shader (VS bound as PS): PSO fails → draw skipped → `psoOk=0`.
- Frame with 0 draws: no-op.
- Keys while unfocused: ignored (foreground check).

## Verification

Manual, since it's an interactive visual tool:
1. Build fm2, launch via `launch-fm2-plume-native-clean.bat --fm2_shader_probe 1`.
2. At press-start, `F6/F7` until the soloed draw is the fullscreen background.
3. `F8/F9` through indices; confirm the log advances and the on-screen draw
   changes; find the index where the background texture renders correctly.
4. Record that entry's `hash`/`filename` from the log — this is the shader that
   should be bound. Compare against the shader the renderer currently binds to
   that draw to diagnose the mis-selection.

Success = the tool reliably solos/cycles and at least one cache entry makes the
bg draw show its texture (proving the correct shader exists and is reachable),
with its hash logged.
