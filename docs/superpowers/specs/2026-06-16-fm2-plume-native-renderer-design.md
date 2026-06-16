# FM2 Plume Native Renderer Design

Date: 2026-06-16

## Purpose

FM2 should move away from relying on ReXGlue's current Xenos emulation path for
final rendering. The target architecture is a title-native renderer that maps
FM2's own render concepts directly to Plume, following the same broad pattern as
UnleashedRecomp: intercept high-level title rendering behavior, translate the
title's resources and draw state into native renderer objects, and replay those
draws through Plume.

The first implementation must not erase the current backend immediately. The
current backend remains as boot/debug scaffolding while Plume proves ownership of
individual render paths. This keeps FM2 observable during bring-up and avoids a
black-screen state where shader, resource, pass, or command-buffer failures are
hard to separate.

## Current Evidence

Plume is present in the repository at `plume/` and provides a platform-neutral
render interface with D3D12 support on Windows. Its public API exposes the
objects needed for an FM2 native renderer: render interfaces, devices, command
queues, swapchains, command lists, framebuffers, buffers, textures, shaders,
pipelines, descriptor sets, and draw calls.

The ReXGlue runtime already has an explicit no-graphics-system path:
`Runtime initialized without graphics system (native rendering mode)`. That
means FM2 can eventually choose not to inject ReXGlue's Xenos graphics system.
For the first prototype, however, the current graphics path should stay alive
until Plume can render at least one real FM2 draw path.

FM2's `Fm2App::OnPreSetup` currently configures audio and does not force a
graphics backend. This is a practical integration point for FM2-specific runtime
choices, but the native renderer should be owned by FM2 code rather than by
generated recompilation output.

The most important FM2 render functions identified so far are:

- `FM2_Render_FramePipeline` at `0x82518DC0`
- `FM2_Render_SubmitPassWrapper` at `0x825181A8`
- `FM2_Render_ExecuteSortedDrawLists` at `0x8252FF00`
- `FM2_Render_CompileMissingPassBuffers` at `0x82531DC0`
- `FM2_Render_BuildObjectPassCommandBuffer` at `0x82531370`
- `FM2_Render_BuildDirectIndexedDrawBuffers` at `0x825380B8`
- `FM2_Render_InstanceHybridDrawPath` at `0x82539650`
- `FM2_Render_UiOrScreenDrawListSubmit` at `0x825B8A60`

These functions show that FM2 builds higher-level cached command buffers before
they reach the low-level Xenos command processor. The first native renderer work
should use those functions as the capture/replay boundary.

## Design Decision

Use a side-by-side FM2-native Plume renderer first.

This is not meant to become a permanent dual-renderer architecture. It is a
staged replacement path. The current Xenos-backed renderer keeps FM2 booting and
provides comparison output while Plume learns enough FM2 state to render selected
passes. Once a Plume path is correct enough, that path can be switched from
shadow mode to native mode and the old path can be skipped for that pass.

Native-only mode becomes the goal after these conditions are true:

- Plume can initialize, clear, and present in the FM2 window.
- FM2 hooks can capture stable draw/pass/resource metadata.
- One real FM2 indexed draw path can be replayed through Plume.
- Captured state is rich enough to diagnose missing shaders, buffers, textures,
  render targets, and constants without the Xenos backend.

## Architecture

### FM2 Native Renderer Layer

Add an FM2-owned native renderer layer under a path such as
`FM2/src/native_renderer/`. This layer is title-specific and may understand FM2
render structures, addresses, and pass concepts.

Initial responsibilities:

- Own Plume render interface/device/queue/swapchain lifetime.
- Create a minimal Plume frame loop for clear/present.
- Expose a small API that FM2 hooks can call without pulling Plume details into
  hook bodies.
- Store shadow-recorded draw packets and pass metadata.
- Provide cvars or equivalent runtime flags to enable tracing, shadow recording,
  native clear/present, and selected native replay.

The FM2 hook layer should remain thin. It should identify guest addresses,
arguments, and lifecycle points, then call into the native renderer layer. Plume
types should not spread through generated code or broad ReXGlue runtime code.

### Plume Build Integration

Plume should be added as a normal CMake target and linked into FM2 only where it
is needed. The first pass should avoid modifying generated FM2 files.

Candidate build steps:

- Add `add_subdirectory(../plume ...)` from FM2 or expose Plume from the root
  project in a way FM2 can consume.
- Keep `PLUME_BUILD_EXAMPLES` off.
- Link `plume` to `fm2`.
- Add FM2 native-renderer source files to `FM2_SOURCES`.

The build integration should be scoped so other title projects are not forced to
link Plume until they opt into a native renderer.

### Runtime Ownership

The first prototype should run with the existing ReXGlue graphics backend still
available unless a cvar explicitly selects native-only mode. The FM2 native
renderer should use the app/window context to create Plume presentation objects.

Long-term, FM2 should support:

- `xenos`: current ReXGlue renderer only.
- `shadow`: current renderer plus Plume capture/replay diagnostics.
- `hybrid`: Plume owns selected passes; current renderer handles the rest.
- `native`: Plume owns presentation and all supported FM2 render passes.

The first implementation only needs `xenos` and `shadow`, with a small
experimental Plume clear/present path behind a flag.

## Data Flow

### Shadow Recording

Hooks around FM2 render functions capture metadata while preserving original
behavior.

Useful hook points:

- `FM2_Render_BuildObjectPassCommandBuffer`: capture object/pass identity,
  cached command-buffer pointers, fixup references, and renderable context.
- `FM2_Render_BuildDirectIndexedDrawBuffers`: capture direct indexed draw
  arguments, vertex/index buffer references, and visible resource bindings.
- `FM2_Render_ExecuteSortedDrawLists`: observe execution order and associate
  sorted draw-list entries with previously captured command buffers.
- `FM2_Render_InstanceHybridDrawPath`: inspect instance rendering and decide
  whether the path uses cached command buffers or direct draw emission.
- `FM2_Render_UiOrScreenDrawListSubmit`: later UI/screen-space candidate, not
  part of the first real 3D draw milestone.

The first durable data structure should be an FM2 native draw packet. It should
store stable observed fields only. Fields should be added as evidence appears
from IDA, logs, captures, or live debugging.

Initial packet fields:

- Guest function and hook address that produced the packet.
- Guest object/renderable pointer when known.
- Guest pass or command-buffer pointer when known.
- Sort/execution order when observed.
- Draw type: indexed, instanced indexed, non-indexed, UI/screen-space, or
  unknown.
- Index count, instance count, start index, base vertex, and start instance when
  observed.
- Guest vertex/index buffer references when observed.
- Shader/resource/state identifiers when observed.
- Diagnostic status explaining why a packet is not replayable yet.

### Plume Replay

Replay should be opt-in and narrow.

The first real FM2 replay target should be a single direct indexed draw path,
preferably through `FM2_Render_BuildDirectIndexedDrawBuffers`, because the draw
arguments are expected to be more visible than in fully cached draw-list replay.

Replay stages:

1. Create Plume device, command queue, swapchain, and command list.
2. Clear and present from Plume with no FM2 resources.
3. Upload or reference one known vertex/index buffer.
4. Bind a minimal known shader pair or debug shader.
5. Issue one `drawIndexedInstanced` call through Plume.
6. Replace debug shader/resources with FM2-derived state.
7. Gate the original Xenos draw for that selected packet only after Plume output
   is useful.

## Error Handling And Diagnostics

The native renderer must fail soft during bring-up. A Plume initialization,
swapchain, or replay failure should log a precise reason and return control to
the existing FM2 path whenever that path is enabled.

Diagnostics should include:

- Current FM2 native renderer mode.
- Plume device/backend selected.
- Swapchain size and resize events.
- Per-frame counts for captured packets, replayable packets, replayed packets,
  skipped packets, and packets with missing required state.
- First missing requirement per skipped packet, such as missing shader, missing
  vertex declaration, missing index buffer, unsupported render target, or
  unknown constants.
- Optional focused dumps for one object/pass/command-buffer pointer.

Temporary diagnostics should be behind cvars or similarly controllable flags so
they can stay hot-reloadable during repeated FM2 runs.

## Testing And Verification

Verification should proceed in small milestones:

1. Configure/build FM2 with Plume linked and no behavior change.
2. Launch FM2 with current rendering and confirm no regression.
3. Enable shadow recording and confirm logs show stable packet counts without
   changing output.
4. Enable Plume clear/present in an isolated mode and confirm the swapchain is
   alive.
5. Capture one direct indexed draw packet and validate its fields against IDA
   and RenderDoc/Xenia comparison where useful.
6. Replay one draw through Plume with a debug shader, then with FM2-derived
   state.
7. Add an opt-in native replacement for that selected draw/pass and confirm the
   old path can be skipped only for that case.

Build verification should include the normal FM2 target:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Generated FM2 code should not be edited as the permanent solution. If a
generated-code experiment proves a hook or metadata theory, the durable change
belongs in the manifest, FM2 handwritten source, SDK code, or codegen.

## Risks

The main technical risks are shader translation, vertex declaration decoding,
texture/resource lifetime, render-target semantics, constant-buffer mapping, and
identifying exactly where FM2's cached command buffers stop being title-level
data and start being Xenos command-stream data.

The side-by-side design reduces these risks by making every native step
observable. If Plume replay is wrong, the existing renderer can still boot the
game, logs can still be compared, and packet capture can be refined without
losing all frame output.

## Initial Implementation Plan Boundary

The next implementation plan should cover only the first Plume adoption slice:

- Add Plume to the FM2 build.
- Add an FM2 native-renderer facade.
- Initialize/shutdown Plume without changing normal FM2 output.
- Add shadow-recording hooks around one or two named FM2 render functions.
- Add diagnostics proving that capture is stable.
- Add an optional Plume clear/present experiment.

It should not attempt full FM2 native rendering, shader cache generation,
resource lifetime replacement, UI rendering, or removal of the ReXGlue graphics
backend in the first slice.
