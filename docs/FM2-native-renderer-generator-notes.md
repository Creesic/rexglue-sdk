# FM2 Native Renderer And Generator Notes

Date: 2026-06-16
Branch context: `FM2_WIN_Plume`

This note tracks the idea of using Plume for a title-native renderer path, with
FM2 as the first prototype. It is meant to be updated as we learn durable facts.

> Cross-reference: `docs/FM2-native-renderer-gap-analysis.md` (2026-06-23)
> documents the cross-repo comparison (FM2 vs ReOdyssey vs UnleashedRecomp),
> the shared `render/` module lineage, and the mirror-vs-replace architectural
> gap. Read that first for the structural picture before the FM2-internal
> prototype details below.

## Current Conclusion

UnleashedRecomp has a native renderer, but not a universal Xbox 360 renderer.
It hooks Sonic Unleashed's high-level game rendering layer and translates those
game draw calls into Plume objects and commands.

That model is promising for FM2 if FM2 has a similarly hookable high-level D3D
or render-device layer. It does not remove the need to understand the title's
rendering model. The reusable part is the workflow and shared renderer runtime,
not a fully automatic conversion from arbitrary Xenos packets to native PC
rendering.

## What UnleashedRecomp Does

Observed from `C:\Users\Tera\Documents\GitHub\UnleashedRecomp`:

- Uses Plume as the host rendering hardware interface.
- Defines game-facing objects such as `GuestDevice`, `GuestTexture`,
  `GuestBuffer`, `GuestSurface`, `GuestShader`, and `GuestVertexDeclaration`.
- Hooks high-level guest rendering functions:
  - create device
  - create texture/surface/vertex buffer/index buffer
  - lock/unlock texture and buffers
  - set render target/depth surface
  - set texture/sampler/render state
  - set shaders
  - set stream source/indices
  - draw primitive / draw indexed primitive / draw primitive UP
  - present
- Converts guest D3D-like state and formats into Plume state.
- Queues commands to a host render thread, then flushes render state into Plume
  command lists.
- Uses `XenosRecomp` offline to build title-specific DXIL/SPIR-V shader cache
  data from the title shader archive and patched XEX.
- Adds title-specific fixes and special paths for MSAA resolves, copies,
  post-processing, shader specialization, UI, movie rendering, and engine bugs.

The important lesson is that it bypasses the original Xbox Vd/GPU path by
intercepting a high-level game renderer API. It does not try to emulate every
Xenos register and PM4 packet for every possible game.

## FM2 Direction

For FM2, the first milestone should be a Plume-backed native renderer prototype
for FM2 specifically, not a multi-game generator.

Desired initial shape:

1. Identify FM2's high-level rendering abstraction.
2. Hook a minimal set of FM2 rendering entrypoints.
3. Define FM2 guest-facing renderer structs in host code.
4. Translate FM2 render state/resource calls into Plume resources and commands.
5. Create a minimal Plume device/swapchain path.
6. Get a first visible frame or a controlled clear/present path.
7. Expand to resources, shaders, draw calls, resolves, and UI.
8. Only after the FM2 path works, extract common patterns into generator/runtime
   infrastructure.

Known FM2 render-related names already in this repo:

- `0x82375A40 = FM2_D3D_BeginCommandBufferBatch`
- `0x82375ED0 = FM2_D3D_EmitDirtyStateAndDrawList`
- `0x82376A58 = FM2_D3D_FinalizeCommandBufferBatch`

These are currently documented in `docs/FM2-ida-toml-function-notes.md` and
listed in `FM2/fm2_manifest.toml`. They look like command-buffer emission
functions, not necessarily the highest-level API boundary we want. They are a
good starting landmark, but the native renderer hook layer may need to sit above
them if FM2 has D3D-style create/set/draw wrappers.

## 2026-06-16 FM2 IDA Survey

IDA is connected to the FM2 `default.xex` image at base `0x82000000`.
The initial caller walk started from the three known D3D command-buffer
landmarks.

### Low-Level Command Buffer Layer

These functions are probably below the desired native-renderer boundary:

- `0x82375A40 = FM2_D3D_BeginCommandBufferBatch`
  - Sets up a D3D command-buffer batch, binds command-buffer ownership, clears
    dirty masks, copies optional state-mask arrays, and may drain pending GPU
    command work.
- `0x82375ED0 = FM2_D3D_EmitDirtyStateAndDrawList`
  - Emits dirty state and a draw-list command-buffer payload. Callers usually
    pass a cached command-buffer pointer selected from a per-object/pass table.
- `0x82376A58 = FM2_D3D_FinalizeCommandBufferBatch`
  - Patches packet lengths, flushes/finalizes the command buffer, and writes
    result status.

For a Plume-native renderer, directly replacing this layer would still require
understanding FM2's command-buffer encoding and a lot of Xenos-like state. It
is useful as a trace boundary, but probably not the best first hook point.

### Higher-Level Render Pipeline Candidates

The IDA names corresponding to these addresses were applied and mirrored into
`FM2/fm2_manifest.toml` on 2026-06-16.

| Address | Working description | Evidence | Native renderer relevance | Confidence |
| --- | --- | --- | --- | --- |
| `0x82518DC0` | Main frame/pipeline orchestrator | Sets view/render state, prepares camera data, compiles command buffers, then submits multiple render passes through helpers. | Good for understanding frame pass order; probably too high for replacing draw execution directly. | Medium |
| `0x825181A8` | Render pass submit wrapper | Stores a pass selector, calls a small setup helper, then calls `0x8252FF00`. | Good stable pass-boundary probe. | Medium |
| `0x8252FF00` | Sorted draw-list executor | Enters a critical section, selects render buckets, updates per-object state through `0x825222B0` / `0x82522418`, then calls `FM2_D3D_EmitDirtyStateAndDrawList`. | Strong candidate for intercepting "replay cached draw work" by pass. | Medium-high |
| `0x82531DC0` | Time-budgeted command-buffer compiler | Scans renderable lists for missing pass command buffers and calls `0x82531370` until an item/time budget is reached. | Strong candidate for replacing cached command-buffer construction with native draw metadata construction. | Medium-high |
| `0x82531370` | Per-object/per-pass command-buffer builder | Starts a command-buffer batch, calls `0x8250F7C0` to emit draw work, finalizes, creates texture/fixup records, then clones command buffers into per-pass slots. | Very important. This may be the FM2 equivalent of "record native draw packet for this material/pass". | High |
| `0x8250D950` | Higher-level scene/view traversal | Iterates a list of views or light modes, sets renderer interface state, calls `0x82509148`, then follow-up pass helpers. Contains string evidence around `LightMapOnly`. | Useful for pass naming and visual phase correlation, but likely above the hook layer. | Medium |
| `0x82509148` | Compact scene render entry | Prepares view state, optionally initializes, refreshes lists, calls `0x82531DC0`, then calls `0x8252FF00`. | Good test hook for one full world-render slice. | Medium-high |
| `0x82537998` | Direct-draw record resource resolver | Iterates direct-draw records and resolves missing resource/model pointers via a skinned-model resource lock. | Useful precondition for decoding direct draw record `+0x28`, `+0x2C`, and `+0x30`. | Medium-high |
| `0x8251B620` | Pass-template command-buffer setup | Switches on mode `0..10`, begins a batch, applies fixed render/depth states, marks dirty slots. | Useful for naming pass modes and reconstructing render-state templates. | Medium |
| `0x8251BA08` | Command-buffer finalize-and-clone helper | Applies fixed render states, finalizes the active batch, then clones `dword_829F4454`. | Below ideal hook layer; useful for cached command-buffer lifecycle. | Medium |
| `0x8251BC40` | Sorted object draw-list submit helper | Sorts up to 19 object slots by distance/score, updates object state, then emits selected draw-list entries. | Useful for object-order and transparency behavior. | Medium |
| `0x8251C688` | Object/pass draw traversal | Chooses pass buckets from object flags and alpha state, updates per-object constants/material state, then emits several cached draw lists. | Strong candidate for object-level render semantics once pass names are known. | Medium-high |
| `0x825380B8` | Direct indexed-draw command-buffer builder | Binds transforms/resources through a renderer interface, calls virtual draw methods, finalizes, then clones command buffers. | Very useful because it shows direct draw construction rather than only cached replay. | High |
| `0x82539650` | Instance renderer / hybrid cached-or-direct draw path | Sorts visible instances, uploads texture/constants, then either emits cached draw lists or calls interface methods to draw indexed primitives directly. | Good Plume bridge template because both state setup and draw parameters are visible. | Medium-high |
| `0x8253A680` | Instance renderer wrapper | Sets constants, prepares camera-dependent data, calls `0x82539650`, then follow-up helpers. | Useful for identifying one specialized render subsystem. | Medium |
| `0x825B8920` | Scoped render-batch begin | Switches active device/context, selects a command buffer, begins a batch, and marks the object active. | Likely useful for UI/offscreen scoped rendering. | Medium |
| `0x825B8688` | Scoped render-batch finalize | Releases current surfaces, finalizes the command buffer, restores previous context, and marks the object inactive. | Pair with `0x825B8920` for scoped/offscreen rendering. | Medium |
| `0x825B8A60` | Screen-space or UI draw-list submit | Computes a 2D transform, uploads constants with `0x8236D958`, selects a command buffer, then emits it. | Candidate for a separate native UI/2D path. | Medium |

### Initial Interpretation

FM2 appears to split rendering into two phases:

1. Build or refresh cached D3D command buffers for renderable/pass combinations.
2. Execute those cached command buffers during sorted pass traversal.

That is different from UnleashedRecomp's simpler-looking high-level D3D hook
surface, but it may still be workable. The most promising FM2-native path is
not to emulate the cached command-buffer bytes. Instead, use the command-buffer
compiler functions as the place to build native Plume draw metadata, then use
the pass-submit functions to execute that native metadata.

The first practical Plume prototype should therefore focus on a narrow pass:

1. Instrument `0x82509148`, `0x82531DC0`, `0x82531370`, and `0x8252FF00` with
   lightweight logging.
2. Correlate pass indices and cached draw-list slots with one RenderDoc capture.
3. Pick one simple world pass or the `0x825380B8` direct-draw path.
4. Mirror its vertex/index buffers, textures, constants, shaders, and draw
   arguments into a host-side native draw packet.
5. Replay that one packet through Plume while leaving the existing backend as
   fallback.

### Ghidra 90 Cross-Check

Ghidra 90 is connected to the same `default.xex`, but for this FM2 render pass
it is not currently giving a better caller read than IDA. It agrees with IDA on
the straightforward low-level direct calls to `0x82375A40` and `0x82376A58`,
but it only reports 10 callers to `0x82375ED0` where IDA reports 18.

The main reason is function-boundary recovery. Ghidra currently truncates some
large PPC functions at compiler save/prologue helper calls:

- `0x82518DC0` is treated as a 5-instruction function ending after
  `bl 0x824131A0`, so Ghidra misses the real frame-pipeline body and the direct
  call to `0x82531DC0` at `0x825191C8`.
- `0x82539650` is treated as a 5-instruction function ending after
  `bl 0x82413170` / `bl 0x824144C8`, so Ghidra misses its real instance-render
  body.
- `0x8253A680` is not recognized as a function in the current Ghidra view, while
  IDA decompiles it and sees the direct call to `0x82539650` at `0x8253A964`.

Ghidra did confirm useful data/vtable references:

- `0x82518DC0` is referenced from data at `0x820441D0` and `0x82045000`.
- IDA also sees those and an additional data reference at `0x8218F858`.

Conclusion: use IDA as the primary tool for this FM2 render-boundary survey,
and use Ghidra 90 as a secondary cross-check for data/vtable references and
simple direct xrefs. If we want Ghidra to help more here, the next step is to
repair its PPC save/prologue helper handling or manually recreate the affected
function bodies.

## First FM2 Step

The first step is not primarily loading the Unleashed XEX in IDA. The
Unleashed source already gives us the useful pattern.

The first step is an FM2 render-boundary survey:

1. Load or focus the FM2 XEX in IDA.
2. Start from the known FM2 D3D command-buffer functions above.
3. Walk callers upward until we find stable title-level APIs for:
   - device/context creation
   - present/swap
   - render target/depth target setup
   - texture/surface/buffer creation
   - texture/buffer lock/unlock or upload
   - shader creation/binding
   - vertex declaration/input layout setup
   - render state/sampler state updates
   - draw and draw indexed calls
4. Separately inspect import callsites for Vd/display functions to understand
   what path is still low-level.
5. Name useful functions in IDA and mirror durable names into
   `FM2/fm2_manifest.toml` and `docs/FM2-ida-toml-function-notes.md`.
6. Build a hook-candidate table before writing renderer code.

Loading the Unleashed XEX in IDA may still help later as a reference for how its
source hook names map back to original game function shapes, but it is optional.
For FM2 work, IDA time is better spent on FM2 first.

## Hook Candidate Table Template

Use this table while surveying FM2:

| Address | Proposed name | Evidence | Inputs/outputs | Native renderer action | Confidence |
| --- | --- | --- | --- | --- | --- |
| `0x82375A40` | `FM2_D3D_BeginCommandBufferBatch` | Existing name; command batch setup | Unknown | Maybe below desired hook layer | Medium |
| `0x82375ED0` | `FM2_D3D_EmitDirtyStateAndDrawList` | Existing name; emits dirty state/draw list | Unknown | Maybe below desired hook layer | Medium |
| `0x82376A58` | `FM2_D3D_FinalizeCommandBufferBatch` | Existing name; finalizes command buffer | Unknown | Maybe below desired hook layer | Medium |
| `0x82531370` | `FM2_Render_BuildObjectPassCommandBuffer` | Begins/finalizes a batch, emits pass draw work, creates texture fixups, clones command buffers. | Large render object pointer plus pass indices and device/context state. | Build native draw metadata for one object/pass. | High |
| `0x8252FF00` | `FM2_Render_ExecuteSortedDrawLists` | Iterates sorted renderable arrays and emits cached command-buffer pointers. | Render world/list object, pass/layer selectors, view context. | Execute native draw metadata by pass. | Medium-high |
| `0x82531DC0` | `FM2_Render_CompileMissingPassBuffers` | Time-budgeted scan for renderables missing cached command buffers; calls `0x82531370`. | Render world/list object, pass index, frame/view context. | Compile native packets lazily. | Medium-high |
| `0x82518DC0` | `FM2_Render_FramePipeline` | Orchestrates the frame and multiple pass submits. | Frame renderer object. | Trace pass ordering and choose prototype slice. | Medium |

Add new rows as IDA evidence accumulates.

## Generator Vision

After FM2, a generator workflow could become useful. The generator should be
viewed as an assistant that produces a strong starting point, not a fully
automatic renderer.

Potential generated artifacts:

- Hook candidate report from static analysis.
- Guest struct layout guesses.
- Function naming suggestions.
- Hook table scaffold.
- Plume resource wrapper scaffold.
- Format/state conversion tables.
- Shader-cache build rules.
- Render-command queue skeleton.
- Validation checklist and trace comparison harness.

Likely manual work per title:

- Confirming function boundaries and calling conventions.
- Correcting guest struct layouts.
- Handling engine-specific render paths and resource lifetime.
- Fixing post-processing, UI, movie, resolve, and copy behavior.
- Adding title-specific shader specializations.
- Validating against captures.

The reusable long-term architecture should be:

- A shared Plume native renderer runtime.
- A per-title native renderer layer.
- A generator that creates per-title scaffolding from discovered evidence.
- A fallback path through the existing Xenos backend when no safe native hook
  boundary is available.

## Risks

- FM2 may not expose a clean high-level render API boundary.
- The known FM2 D3D functions may already be too close to PM4 packet emission,
  forcing more Xenos semantics into the native path.
- Offline shader extraction may be different from Unleashed's `shader.ar`
  workflow.
- FM2 may rely on EDRAM/copy/resolve behavior that is easier to preserve in the
  current Xenos backend than in a native renderer.
- A generator can accelerate repeated work, but it cannot infer all title
  semantics without captures, names, and manual validation.

## Immediate Next Actions

1. Do the FM2 render-boundary survey in IDA.
2. Fill the hook candidate table in this document.
3. Decide whether FM2 has a high-level enough hook layer for an Unleashed-style
   renderer.
4. If yes, design the smallest Plume prototype path:
   - device/swapchain/present
   - backbuffer clear
   - one simple draw path
5. If no, pivot to a Plume backend under the existing Xenos command processor
   rather than a native FM2 renderer.

## June 16 Plume Integration Slice

- Build mode:
  - Branch: `FM2_WIN_Plume`
  - Plume linked into FM2: yes. FM2 adds `../plume` as a CMake subdirectory on
    Windows, links `plume`, `d3d12`, and `dxgi`, and defines `FM2_HAS_PLUME=1`.
  - FM2 build command: `cmake --build --preset win-amd64-relwithdebinfo --target fm2`
  - Verified result: build succeeds after the Plume device/swapchain path and
    after the render-packet hook codegen.
- Runtime modes:
  - `fm2_plume_mode=xenos`: current backend only. This remains the default and
    returns from native renderer initialization without creating Plume objects.
  - `fm2_plume_mode=shadow`: initializes a Plume D3D12 interface/device/queue
    while keeping the existing ReXGlue/Xenos renderer visible.
    Runtime check on June 17 confirmed `FM2 Plume device initialized
    backend=D3D12` and `FM2 Plume native renderer initialized mode=shadow`.
    No `FM2_PLUME_*` packet samples were found in the checked logs from that
    run. Follow-up found the trace sampling gate treated
    `fm2_plume_trace_log_interval=1` as "log nothing" because it checked
    `count % interval == 1`; the helper now logs count `1` and every
    `1 + N` sample, so interval `1` logs every captured packet.
  - `fm2_plume_mode=plume_clear`: disables ReXGlue graphics before runtime
    setup, creates a Plume swapchain from the FM2 window, and issues a one-shot
    clear/present when `fm2_plume_clear_on_init=true`. Initialization failure is
    fatal in this mode so the title does not continue with no visible renderer.
    Runtime check on June 17 confirmed this mode shows only the expected dark
    Plume clear.
- Hook capture:
  - `0x82531370` / `FM2_Render_BuildObjectPassCommandBuffer`:
    - Hook adapter: `FM2PlumeTraceBuildObjectPassEntry`
    - Captured registers: `r3` through `r10`
    - Generated call site: `FM2/generated/fm2_recomp.28.cpp`
    - Runtime observed count: 4036 in the June 17 `fm2_008` shadow run.
    - First runtime sample:
      `r3=4169EF00 r4=BA5CDD14 r5=00000000 r6=00000000 r7=00000009 r8=00000002 r9=7038FA00 r10=2E5DA300`
    - The paired second sample used the same `r3/r4/r5/r7/r8/r9/r10` values
      with `r6=00000001`, so `r6` is likely a per-pass or per-eye sub-index at
      this boundary.
  - `0x825380B8` / `FM2_Render_BuildDirectIndexedDrawBuffers`:
    - Hook adapter: `FM2PlumeTraceDirectIndexedDrawEntry`
    - Captured registers: `r3` through `r10`
    - Generated call site: `FM2/generated/fm2_recomp.28.cpp`
    - Runtime observed count: 1090 in the June 17 `fm2_008` shadow run.
    - First runtime sample:
      `r3=4162EC50 r4=2E0162C0 r5=829F4908 r6=00000014 r7=00000005 r8=0000007D r9=7038F950 r10=00000000`
    - Late runtime sample:
      `r3=4162EC50 r4=2E0162C0 r5=00000001 r6=00000014 r7=00000005 r8=0000007D r9=7038F950 r10=00000000`
    - `r3`, `r4`, `r6`, `r7`, `r8`, `r9`, and `r10` were stable across the
      early and late direct-draw samples checked. `r5` changed, so it needs IDA
      field/caller decoding before treating it as a draw argument.
- IDA direct-draw decode:
  - `FM2_Render_BuildDirectIndexedDrawBuffers` consumes only `r3` and `r4`.
    The runtime sample maps to `direct_render_ctx=4162EC50` and
    `draw_iface=2E0162C0`. Hook registers `r5` through `r10` are live-register
    residue at this boundary and should not be treated as function arguments.
  - `direct_render_ctx + 0x48` is a built/skip flag. If set, the direct builder
    returns without rebuilding cloned command buffers.
  - `direct_render_ctx + 0x5A4` and `+0x5A8` are the begin/end pointers for a
    vector of 0x34-byte direct-draw records. The observed builder count is
    `(end - begin) / 0x34`.
  - `FM2_Render_EnsureDirectDrawRecordResources` fills missing record resource
    pointers. For each 0x34-byte record it resolves fields at:
    - `record + 0x28`: resource/model holder pointer
    - `record + 0x2C`: resource bound through `draw_iface` slot `+0x64` as
      resource/stream slot `0`
    - `record + 0x30`: resource bound through `draw_iface` slot `+0x74`, likely
      the index buffer/resource
  - The resource/model holder at `record + 0x28` has an indexed-draw segment
    vector at holder `+0x10` / `+0x14`. Entries are 8 bytes, and the builder
    checks at most four entries per record.
  - Segment fields currently decoded:
    - `segment + 0x04`: start/index offset passed to the draw call
    - `segment + 0x06`: index count; zero skips the segment
    - primitive type is passed as `4`, and primitive count is
      `segment.index_count / 3`, matching a triangle-list path
  - `direct_render_ctx + 0x5B0` is also bound through `draw_iface` slot `+0x64`
    as resource/stream slot `1`.
  - Clone output vectors live at `direct_render_ctx + 0x0C`, `+0x1C`, `+0x2C`,
    and `+0x3C`; the direct builder stores one cloned command buffer per outer
    draw record into the segment-index-selected output vector.
  - The live/non-cached path inside `FM2_Render_InstanceHybridDrawPath` mirrors
    the same binds and final draw call, which raises confidence that this schema
    is the right first Plume replay boundary.
- Runtime direct-draw decode check:
  - The June 17 `fm2_009` shadow run produced direct-draw decode output in
    `C:\temp\fm2-clean.log`. The split FM2 app logs carried the packet counters
    and shutdown stats; hook `LogLine` output still goes to the temp log.
  - Shutdown stats for that run were:
    `build_object_pass=2051 direct_indexed_draw=1073 last_hook=82531370`.
  - First decode sample:
    `ctx=4029F8B0 iface=2E0162C0 built=0 rec_begin=41DFB830 rec_end=41DFBE7C rec_count=31 scan=4`.
    Samples `2..8` used the same context, interface, and record vector with
    `built=1`, confirming the direct builder flips the built/skip flag after
    the first build pass.
  - The record vector stride is confirmed as `0x34`:
    `41DFB830`, `41DFB864`, `41DFB898`, `41DFB8CC`.
  - First decoded records:
    - `rec_i=0 rec=41DFB830 holder=2E6A1BA0 bind0=2E6A1BE8 index_res=2E6A1BF4 seg_begin=2E7ECA80 seg_end=2E7ECAB8 seg_count=7 start=0 index_count=4062 prim_count=1354`
    - `rec_i=1 rec=41DFB864 holder=2E6A1AE0 bind0=2E6A1B28 index_res=2E6A1B34 seg_begin=2EDA2300 seg_end=2EDA2338 seg_count=7 start=0 index_count=4158 prim_count=1386`
    - `rec_i=2 rec=41DFB898 holder=2E6A1D20 bind0=2E6A1D68 index_res=2E6A1D74 seg_begin=2EDA2380 seg_end=2EDA23B8 seg_count=7 start=0 index_count=3822 prim_count=1274`
    - `rec_i=3 rec=41DFB8CC holder=2E6A1C60 bind0=2E6A1CA8 index_res=2E6A1CB4 seg_begin=2EDA23C0 seg_end=2EDA23F8 seg_count=7 start=0 index_count=4224 prim_count=1408`
  - The decoded resource pointer pattern is stable: `bind0` is holder `+0x48`
    and `index_res` is holder `+0x54`. This confirms the IDA read that
    `record + 0x2C` and `record + 0x30` are resource pointers resolved from
    the holder before binding through the draw interface.
  - Segment counts and primitive counts are coherent. The first four records
    each have `seg_count=7`, first segment index `0`, and
    `prim_count = index_count / 3`, matching a triangle-list direct indexed
    draw path.
  - The June 17 `fm2_010` run decoded the direct draw-interface vtable:
    `vtable=82108C88`, slot `+0x64=825B3220`, slot `+0x74=825B32C0`, and
    slot `+0x80=825B3320`.
    - Slot `+0x64` is stream binding. It consumes a 0x0C-byte descriptor:
      `w00 = D3D resource object`, `w04 = descriptor-side count/size`, and
      `w08 = stream stride`.
    - Slot `+0x74` is index binding. It consumes the index descriptor's `w00`
      D3D resource object.
    - Slot `+0x80` is indexed draw. It maps primitive type through a table at
      `0x82000B90` and forwards the start/primitive-count arguments to the
      lower draw helper.
  - Descriptor examples from `fm2_010`:
    - `stream1`: `desc=418CFE20 w00=2EC4C100 w04=00018746 w08=0000000C`.
    - `rec_i=0 stream0`: `desc=2E6A3B28 w00=2E0C7F60 w04=000007EB w08=00000020`.
    - `rec_i=0 index`: `desc=2E6A3B34 w00=2E0C7C40 w04=00002382 w08=00000001`.
  - The descriptor `w00` values are D3D resource objects, not raw vertex/index
    memory. IDA shows the low-level stream binder reads D3D resource object
    fields at `resource + 0x18` and `resource + 0x1C` for guest buffer base and
    byte size.
  - The June 17 `fm2_011` run used `fm2_plume_trace_direct_buffer_bytes=64`
    and confirmed buffer reads from the decoded D3D resources:
    - `stream1`: `desc=41A47770 resource=2E12ECA0 stride=0x0C
      gpu_base=B0C8C4C7 size_field=1012574A`. The low size bits match
      `0x18746 * 0x0C + 2`; the top `0x10000000` bit is resource metadata.
    - `rec_i=0 stream0`: `desc=2E691CA8 resource=2E0C7DC0 stride=0x20
      gpu_base=B0DB2CD7 size_field=1000FD62`. The low size bits match
      `0x07EB * 0x20 + 2`, so descriptor `w04` is a vertex count and `w08`
      is stride.
    - `rec_i=0 index`: `desc=2E691CB4 resource=2E0C7C40 index_type=1
      gpu_base=B0DC2A34 byte_size=00004704`. Descriptor `w04=0x2382` is the
      index count/capacity and `resource + 0x1C = w04 * 2`, so `w08=1` is a
      16-bit index format.
    - The index buffer byte dump starts as big-endian 16-bit values:
      `0000 0001 0002 0003 0004 0005 ...`. This proves `segment + 0x04`
      is a first-index value, not a byte offset.
    - Stream0 byte dumps are 32-byte vertex records, but the bytes are not a
      simple three-float position layout. The next missing piece is the vertex
      declaration / shader input interpretation, not buffer addressing.
  - The follow-up IDA pass identified the state setup immediately before the
    direct draw:
    - Draw interface slot `+0x30` resolves the handle copied from
      `direct_render_ctx + 0x4C`, then calls the render-context vertex shader
      state setter.
    - Draw interface slot `+0x28` resolves the handle copied from
      `direct_render_ctx + 0x6C`, then calls the render-context pixel shader
      state setter. The June 17 state trace confirmed the resolved resource
      type tag is `0x00100007`.
    - Both state paths retain the first dword of the source lock, then resolve
      the concrete state object through `handle + 0x48`.
    - The resolved vertex shader resource type tag is `0x00100006`; its current
      payload base is at `shader + 0x20` (`B0DB2260` in the June 17 run).
    - The resolved pixel shader resource type tag is `0x00100007`; its current
      payload base is at `shader + 0x18` (`B0DB21C0` in the June 17 run).
    - The vertex shader compiled-state table is addressed as
      `shader + 0x368 + *(shader + 0x37C)`.
    - The pixel shader compiled-state table is addressed as
      `state + 0x28 + *(state + 0x3C)`.
  - The June 17 shader payload run showed that the raw payload pointer is not
    always the exact microcode range:
    - The first sampled vertex shader resolved object was `4021EDC0`, type
      `0x00100006`, with payload base `B0BBEC20` at object `+0x20`.
      The first `0x30` payload bytes were zero header/padding; apparent Xenos
      control-flow microcode began at `B0BBEC50`.
    - The first sampled pixel shader resolved object was `2E1494B0`, type
      `0x00100007`, with payload base `B0BBEB80` at object `+0x18`.
      The object field at `+0x30` was `0x90`, and the previous 256-byte dump
      crossed into the vertex shader allocation at `B0BBEC20`. Pixel payload
      dumps must therefore be capped by the object byte-count field before
      hashing or disassembling.
    - A later run from the same first direct draw produced vertex shader object
      `412B05D0` and pixel shader object `2E75F4B0`, with the same payload
      bases. Pixel `known_payload_bytes=0x90` cleanly capped the dump. Vertex
      object field `+0x30` read as guest-endian `0x08030000`; interpreted as
      raw little-endian bytes this is `0x308`, a plausible vertex microcode
      size candidate. Treat it as a candidate until a full dump confirms the
      end boundary.
    - The current safe diagnostic path is to log raw payload bytes separately
      from derived microcode bytes. Do not feed arbitrary 256-byte windows into
      `Shader::AnalyzeUcode`; it follows control-flow instruction addresses and
      needs a real microcode range.
  - UnleashedRecomp shader cache pattern for comparison:
    - `CreateShader` hashes `function[1] + function[2]` bytes from the guest
      shader function pointer with `XXH3_64bits`.
    - That hash is looked up in generated `shader/shader_cache.cpp`; Plume then
      receives a host DXIL/SPIR-V shader from the generated cache.
    - FM2 needs the equivalent guest shader byte range and hash before we can
      map draws to generated host shaders in the same style.
- Current cvars:
  - `fm2_plume_mode`: `xenos`, `shadow`, or `plume_clear`
  - `fm2_plume_clear_on_init`: clear/present during `plume_clear` setup
  - `fm2_plume_trace_packets`: enables sampled packet logging
  - `fm2_plume_trace_log_interval`: logs one packet sample every N captures
    and uses `1` for every captured packet
  - `fm2_plume_trace_direct_decode`: enables sampled guest-memory decoding for
    `FM2_Render_BuildDirectIndexedDrawBuffers`
  - `fm2_plume_trace_direct_decode_limit`: maximum decoded direct-draw samples
    per process. The June 17 `fm2_009` run used `8` and captured the transition
    from `built=0` to repeated `built=1` samples.
  - `fm2_plume_trace_direct_decode_record_limit`: maximum direct-draw records
    inspected per decoded sample. The June 17 `fm2_009` run used `4`, which was
    enough to prove the record stride and holder/resource offsets.
  - `fm2_plume_trace_direct_buffer_bytes`: optional byte count, capped at 64,
    for dumping the first bytes at each decoded D3D resource buffer base.
  - `fm2_plume_trace_direct_state_bytes`: optional byte count, capped at 1024,
    for dumping decoded state handle, resolved state object, and compiled-state
    table bytes for the direct draw shader/state slots.
  - `fm2_plume_trace_direct_shader_bytes`: optional byte count, capped at 4096,
    for dumping decoded vertex/pixel shader payload bytes from resolved shader
    resources.
    After the vertex-size candidate appeared, the diagnostic cap was raised
    first to 1024, then to 4096 after the `0x308` candidate was shown not to be
    the end boundary.
  - `fm2_plume_trace_render_context_limit`: maximum lower-level D3D command
    context dirty-state samples to log from `FM2_D3D_EmitDirtyStateAndDrawList`.
    This is useful for broad sampling but can miss the direct-draw correlation
    point because heavy logging changes the sample/timing window.
  - `fm2_plume_trace_render_context_skip`: lower-level D3D command-context
    dirty-state samples to count before `fm2_plume_trace_render_context_limit`
    starts logging. A June 18 run with `skip=5500` and `limit=2000` proved the
    windowing works, but the first direct decode still landed just after the
    logged range.
  - `fm2_plume_trace_render_context_after_direct_limit`: preferred
    correlation knob for the current investigation. It arms once after the
    first `FM2_PLUME_DIRECT_DECODE` and logs the next N lower-level
    `FM2_D3D_EmitDirtyStateAndDrawList` dirty-state entries.
  - `fm2_plume_trace_render_context_fetch_group_limit`: optional decode count
    for each traced D3D command-context state shadow. The shadow is decoded as
    32 six-dword Xenos fetch groups starting at `ctx + 0x400`; use `32` when
    correlating Plume vertex buffers and texture resources.
- Current direct decode output:
  - `FM2_PLUME_DIRECT_DECODE`: direct context, draw interface, built flag, and
    direct-record vector begin/end/count.
  - `FM2_PLUME_DIRECT_IFACE`: draw-interface vtable plus slots `+0x28`,
    `+0x30`, `+0x64`, `+0x74`, and `+0x80`. The important goal is to identify
    the concrete virtual functions for state upload, stream binding, index
    binding, and draw submission.
  - `FM2_PLUME_DIRECT_RECORD`: record holder, stream0 descriptor pointer,
    index descriptor pointer, segment vector, first nonzero segment, start,
    index count, and triangle-list primitive count.
  - `FM2_PLUME_DIRECT_RESOURCE`: raw descriptor dwords for `stream0`, `stream1`,
    and `index`. The first pass logs `w00`, `w04`, `w08`, and adjacent `w0c`
    so we can infer which word is buffer base, size/stride/format, and index
    type before building a Plume packet.
  - `FM2_PLUME_DIRECT_D3DRESOURCE`: D3D resource object fields, including the
    guest buffer base at `resource + 0x18`, raw byte size at `resource + 0x1C`,
    the masked readable byte count (`byte_size & 0x0FFFFFFF`), descriptor-
    derived view byte count, bounded hash byte count, and `XXH3_64bits` hash
    for the guest buffer range that should become the Plume upload/view.
  - `FM2_PLUME_DIRECT_BUFFER`: optional raw byte dump from the D3D resource
    guest buffer, controlled by `fm2_plume_trace_direct_buffer_bytes`.
  - `FM2_PLUME_DIRECT_STATE`: direct shader/state handle snapshot for
    `vertex_shader` and `pixel_shader`, including the retained handle pointer,
    `handle + 0x48` resolved object pointer, compiled-state table relative
    offset, table address, table header dwords, and payload byte count.
  - `FM2_PLUME_DIRECT_STATE_BYTES`: optional raw byte dump for each decoded
    state handle, resolved object, and compiled-state table, controlled by
    `fm2_plume_trace_direct_state_bytes`. The same line tag is also used with
    `role=d3d_ctx kind=state_shadow` when dumping the lower-level D3D command
    context shadow at `ctx + 0x400`.
  - `FM2_PLUME_DIRECT_SHADER_META`: shader payload metadata for each decoded
    shader object, including payload pointer offset/base, known payload byte
    count when one is known, derived microcode offset/base, and object fields
    `+0x18` through `+0x3C`. For vertex shader objects it also reports
    `w30_le`, the object `+0x30` dword interpreted as raw little-endian bytes.
  - `FM2_PLUME_DIRECT_SHADER`: optional raw byte dump for each decoded shader
    payload, controlled by `fm2_plume_trace_direct_shader_bytes`. The vertex
    shader dumps from resolved shader object `+0x20`; the pixel shader dumps
    from resolved shader object `+0x18` and are capped by the observed pixel
    payload byte-count field when present.
  - `FM2_PLUME_DIRECT_SHADER_HASH`: `XXH3_64bits` over the known shader payload
    byte range. This is the FM2-side equivalent of the UnleashedRecomp shader
    cache key prerequisite: it gives us stable guest shader identity before
    generated host shaders exist.
  - `FM2_PLUME_DIRECT_SHADER_UCODE`: optional byte dump from the derived
    microcode start. Vertex shaders currently use payload `+0x30`; pixel
    shaders currently use payload `+0x00`.
  - `FM2_PLUME_DIRECT_SHADER_UCODE_BOUNDS`: Xenos control-flow scan summary for
    the derived microcode range. When the scan is valid and bounded, it also
    logs a structural `XXH3_64bits` hash over the actually used ucode bytes.
  - `FM2_PLUME_DIRECT_SHADER_UCODE_CANDIDATE`: whole-payload candidate scan for
    wrapped shader payloads. The scanner tests every 4-byte-aligned payload
    offset, requires a complete END-bearing control-flow scan, rejects all-zero
    CF-pair starts, and chooses the largest complete candidate by
    `total_used_bytes` with the lowest offset as the tie-breaker.
  - `FM2_PLUME_DIRECT_PACKET`: packet-level summary built from
    `DirectDrawIndexedPacketSummary` for each decoded direct record. It records
    first index, index count, primitive count, buffer readiness, shader payload
    key readiness, vertex/pixel structural ucode key readiness, debug replay
    readiness, and the stream0/stream1/index/vertex-shader/pixel-shader hashes
    that a first Plume replay path can use for cache identity and regression
    checks.
  - `FM2_PLUME_DIRECT_REPLAY_PLAN`: Plume-facing replay contract built from
    `DirectDrawDebugReplayPlan`. It converts a replayable packet into the
    upload/bind/draw shape for the first debug path: triangle-list topology,
    stream0 at vertex slot 0, stream1 at vertex slot 1, 16-bit/32-bit index
    format, index draw arguments, upload byte counts, and the shader payload
    hashes that will key the eventual generated-shader path.
  - `FM2_PLUME_D3D_DIRTY_STATE_AFTER_DIRECT_ARM`: emitted once when
    `fm2_plume_trace_render_context_after_direct_limit` arms after the first
    decoded direct draw.
  - `FM2_PLUME_D3D_DIRTY_STATE_ENTRY`: lower-level D3D command-context dirty
    state sample from `FM2_D3D_EmitDirtyStateAndDrawList`. The line records
    whether it came from the post-direct window (`after_direct=1`), the command
    context pointer, dirty-mask object, `r5`, `state_shadow = ctx + 0x400`,
    readability bits, context dirty words, mask words, and the effective
    masked dirty words.
  - `FM2_PLUME_D3D_FETCH_GROUP`: decoded texture or vertex-triplet fetch group
    from the lower-level D3D command-context state shadow. Texture lines include
    type, format, endian, base/mip addresses, dimensions, pitch, and tiling.
    Vertex-triplet lines include three raw vertex fetch constants decoded as
    type, guest buffer base, endian, and size.
  - `FM2_PLUME_D3D_VERTEX_FETCH`: emitted only for valid vertex fetch constants
    (`type=3` and nonzero size) found inside a decoded fetch group. It logs the
    fetch constant index, raw dwords, guest base, endian, and byte count.
  - `FM2_PLUME_D3D_VERTEX_FETCH_REPLAY_MATCH`: correlation between a decoded
    D3D vertex fetch constant and a remembered ready direct replay stream. The
    match normalizes direct replay guest bases with `base & 0x1FFFFFFC`, then
    requires exact fetch-base and byte-count agreement. This is the current
    bridge from the direct replay packet contract to Plume vertex-buffer binding
    inputs.
- Next replay gate:
  - Validate the vertex shader candidate with the ReXGlue/Xenia translator and
    extract fetch declarations. Pixel has a validated byte-count field at shader
    object `+0x30`; vertex payload size is now coming from the compiled-state
    table, and the fixed `payload + 0x30` ucode assumption has been replaced by
    a whole-payload candidate scan.
  - Decode the vertex declaration / shader input layout used with this direct
    draw path. Resource addressing is now good enough for a Plume packet, but
    stream0 appears packed/compressed rather than simple float position data.
  - Inspect slot `+0x28` and `+0x30` state setup, the data copied from
    `direct_render_ctx + 0x4C` and `+0x6C`, and the current shader/vertex
    declaration state installed before the direct draw.
  - A repeat run can raise `fm2_plume_trace_direct_decode_record_limit` if all
    31 records are needed:
    ```powershell
    .\out\build\win-amd64-relwithdebinfo\fm2.exe `
      --fm2_plume_mode shadow `
      --fm2_plume_trace_packets `
      --fm2_plume_trace_log_interval 120 `
      --fm2_plume_trace_direct_decode `
      --fm2_plume_trace_direct_decode_limit 8 `
      --fm2_plume_trace_direct_decode_record_limit 31 `
      --fm2_plume_trace_direct_buffer_bytes 64 `
      --fm2_plume_trace_direct_state_bytes 64 `
      --fm2_plume_trace_direct_shader_bytes 1024
    ```
  - Preferred repeatable smoke path for this trace is now the harness:
    ```powershell
    Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
    .\scripts\fm2\Invoke-FM2GameplaySmoke.ps1
    ```
    It launches the same Plume direct-decode flags with `--mnk_mode`, waits 10
    seconds, sends Space twice per second for 20 seconds, waits another 10
    seconds, then kills `fm2.exe`. It also resets `C:\temp\fm2-clean.log` before
    launch by default so stale traces are obvious.
    The harness now defaults to `-InputMethod postmessage`, which sends targeted
    Space down/up messages to the FM2 window. This is the most reliable
    automation path because FM2's MnK driver consumes the app's key messages and
    does not depend on Windows foreground focus. `-InputMethod sendinput`
    remains available when explicitly testing the physical-keyboard path, and
    `both` remains available as a diagnostic comparison mode. The embedded
    helper type is versioned as `NativeInputV6` so an existing PowerShell
    session cannot silently keep using the stale helper after script edits. The
    script still refuses to send input
    if no FM2 window handle is available, re-resolves the FM2 window handle
    during the tap loop, and sets `REX_MNK_MODE=1` in the child process
    environment so the runtime MnK driver is enabled even if the `--mnk_mode`
    cvar is registered after CLI parsing.
  - The harness now verifies the input receiver path by default. It enables
    `REX_MNK_TRACE_INPUT=1`, adds `--log_level info` if needed, scans the
    per-run FM2 app logs, and fails the smoke run unless the number of
    `MNK_INPUT_KEY event=down vk=32` and `MNK_INPUT_KEY event=up vk=32` lines
    matches the reported successful tap count and at least one
    `MNK_INPUT_STATE ... a=1` line is present. That proves the script Space tap
    became Xbox A in `XamInputGetState`; the Windows input API return count
    alone is not treated as proof.
  - The corrected harness was verified on June 18 with the focused Plume trace
    arguments: it selected the FM2 window, sent 40 Space taps, and FM2's own app
    log reported `keyDown=40`, `keyUp=40`, and `aDown=36`. The same run produced
    31 `FM2_PLUME_DIRECT_PACKET` lines and 31 `FM2_PLUME_DIRECT_REPLAY_PLAN`
    lines in `C:\temp\fm2-clean.log`. That is the current smoke signal that
    automation reached the direct render path.
  - Debug replay submission is intentionally separate from replay-plan logging.
    The hook now calls `SubmitDirectDebugReplay` only when
    `fm2_plume_debug_replay` is enabled, so ordinary packet-capture smoke runs
    should have zero `FM2_PLUME_DEBUG_REPLAY_RESULT` lines.
  - Same-window Plume swapchain creation in `fm2_plume_mode=shadow` is now
    behind `fm2_plume_debug_replay_shadow_swapchain` and defaults off. The
    previous attempt to create that swapchain whenever debug replay was enabled
    crashed before FM2 exposed a window, so the safer next direction is a
    separate diagnostic Plume window/offscreen target or a replacement mode that
    keeps enough guest render-work setup alive without the ReX backend.
    With `fm2_plume_debug_replay=1` and the shadow-swapchain experiment left
    off, the June 18 smoke run reached gameplay and logged 31 ready replay plans
    plus 31 `FM2_PLUME_DEBUG_REPLAY_RESULT submitted=0` lines. That is expected:
    the replay gate runs, but no Plume presentation target exists in shadow mode
    unless the explicit experiment cvar is enabled.
  - `fm2_plume_debug_replay_window` creates a separate no-activate Win32 tool
    window for Plume debug replay in shadow mode. This avoids creating a second
    swapchain on the FM2/ReX HWND. The harness excludes this diagnostic window
    title (`FM2 Plume Debug Replay`) when selecting the FM2 window for MnK Space
    taps; otherwise `Process.MainWindowHandle` can point at the diagnostic
    window and input will not advance FM2.
  - The harness can now capture and assert the Plume debug replay window:
    ```powershell
    .\scripts\fm2\Invoke-FM2GameplaySmoke.ps1 `
      -Fm2Args $fm2Args `
      -ReplayScreenshotPath C:\temp\fm2-plume-harness-capture.png `
      -VerifyReplayScreenshotNonDark $true
    ```
    It captures the `FM2 Plume Debug Replay` client area before killing
    `fm2.exe`, writes a PNG, samples the image, and fails when too few sampled
    pixels are above the dark-luminance threshold. The first June 18 visual
    smoke saved `C:\temp\fm2-plume-harness-capture.png` and reported
    `width=960`, `height=540`, `sampled=57600`, `nonDark=50618`,
    `avgLum=344.99`, proving Plume presents non-clear geometry, not only
    `presented=true`.
  - The debug replay vertex shader has since moved from arbitrary XOR scatter
    to a stream0 position probe: it reads `RAW0.xyz` as the first three
    endian-corrected 32-bit floats from stream0, projects x/y directly into
    clip space, and uses stream0 z plus existing raw data for diagnostic color.
    The June 18 position-probe visual smoke saved
    `C:\temp\fm2-plume-position-probe.png` and reported `width=960`,
    `height=540`, `sampled=57600`, `nonDark=8798`, `avgLum=56.03`. The image
    is a compact, coherent mesh-like shape instead of the earlier full-screen
    scatter, which is strong evidence that stream0 slot 0 begins with plausible
    FM2 vertex position floats. This is still not native shader execution:
    object/world/view/projection constants and the real vertex shader must be
    decoded or generated next.
  - The June 18 diagnostic-window smoke command:
    ```powershell
    $fm2Args = @(
      '--fm2_plume_mode','shadow',
      '--fm2_plume_debug_replay',
      '--fm2_plume_debug_replay_window',
      '--fm2_plume_debug_replay_limit','1',
      '--fm2_plume_trace_packets',
      '--fm2_plume_trace_log_interval','120',
      '--fm2_plume_trace_direct_decode',
      '--fm2_plume_trace_direct_decode_limit','1',
      '--fm2_plume_trace_direct_decode_record_limit','31',
      '--fm2_plume_trace_direct_buffer_bytes','64',
      '--fm2_plume_trace_direct_state_bytes','64',
      '--fm2_plume_trace_direct_shader_bytes','4096',
      '--mnk_mode'
    )
    .\scripts\fm2\Invoke-FM2GameplaySmoke.ps1 -Fm2Args $fm2Args
    ```
    selected the real FM2 window
    (`fm2 [rexglue-v0.8.0.77-dev.ge63fc4d-RelWithDebInfo]`), logged 31 ready
    replay plans, and produced one `FM2_PLUME_DEBUG_REPLAY_RESULT submitted=1`
    followed by 30 `submitted=0` results from the replay limit. This proves the
    first FM2 guest buffer upload and indexed draw can be submitted through
    Plume without taking over the main ReX swapchain.
  - The June 17 automated smoke run proved `w30_le=0x308` is not the vertex
    payload/ucode byte count. The 1024-byte vertex ucode dump still contained
    instruction-looking data at ucode offset `0x308` and payload-relative
    offset `0x2D8`. Use a one-sample trace with
    `--fm2_plume_trace_direct_shader_bytes 4096` for the next boundary pass.
  - The follow-up 4096-byte trace found the vertex compiled-state table field at
    `table + 0x2C` contains `0x4EC`, matching the observed vertex payload
    boundary. The direct-decode logger now records this as
    `shader_payload_bytes` and uses it as the vertex payload byte count when
    the resolved shader object has type `0x00100006`.
  - The June 18 parser correction established the vertex compiled-state table
    layout for the first direct-draw sample:
    - Header `h04=1`, `declared_payload_bytes=0x14`.
    - Entry 0 is a skip section at table offset `0x14`, `target_off=0x00FC`,
      `dwords=0x10`, payload offset `0x18`, payload bytes `4`.
    - The trailing state block starts after the declared payload, not four bytes
      before it. The correct trailing entry is at table offset `0x28`,
      `target_off=0`, `dwords=0x40`, payload offset `0x2C`, payload bytes
      `0x100`.
    - The trailing payload begins `00 00 04 EC 00 11 00 0C ...`; the first word
      matches the vertex payload byte count. Parser tag `parser=4` logs this as
      both a normal `FM2_PLUME_DIRECT_COMPILED_STATE_ENTRY` and
      `FM2_PLUME_DIRECT_COMPILED_STATE_TRAILING_ENTRY`.
    This trailing payload is the best next target for reconstructing FM2's
    native vertex/input-layout state, because the current vertex ucode candidate
    scan still reports no fetch declarations.
  - The June 18 smoke harness was corrected again after `sendinput`-only runs
    proved too dependent on Windows foreground behavior. FM2's MnK path consumes
    the app's key messages, so the harness now defaults to `postmessage`: it
    primes focus state and posts targeted Space down/up messages to the FM2
    window. This sends exactly the requested two logical Space taps per second.
    The run is still verified through FM2's own MnK trace lines, not through the
    host API return value alone.
  - The June 18 mask/value decoder logs
    `FM2_PLUME_DIRECT_COMPILED_STATE_MASK_PAIR` lines for the trailing vertex
    table entry. The sampled table has 32 mask/value pairs, mapping state dword
    offsets `0x00..0x7C` to inferred fetch-register slots `0x4800..0x481F`
    in six-dword fetch groups. Example from the latest sample:
    - pair 0: `payload_off=0x2C`, `state_off=0x0000`, `fetch_reg=0x4800`,
      `mask=0x000004EC`, `value=0x0011000C`.
    - pair 6: `state_off=0x0018`, `fetch_reg=0x4806`, `fetch_group=1`,
      `fetch_dw=0`, `mask=0x0030100F`, `value=0x00415011`.
    - pairs 30/31 map to `0x481E/0x481F` and were zero in the latest sample.
    These are patch operations (`dst = (dst & mask) | value`), not guaranteed
    final register values unless the prior command-context shadow is known.
  - A direct probe of `direct_render_ctx + 0x400` was intentionally removed
    after a full smoke run showed pointer/resource-looking data that did not
    match the compiled-state mask/value block. The `ctx + 0x400` shadow seen in
    IDA belongs to the lower-level D3D/render command context passed to
    `FM2_RenderContext_SetVertexShaderState` / dirty-state emission, not the
    higher-level direct-draw context passed to the current decode hook. Next
    instrumentation should hook or sample that lower-level context directly.
  - The lower-level D3D dirty-state hook at
    `FM2_D3D_EmitDirtyStateAndDrawList` now samples that command context
    directly. In the latest post-direct smoke, the first direct decode logged:
    ```text
    FM2_PLUME_DIRECT_DECODE n=1 ctx=41B9F990 iface=2E0162C0 built=0 rec_begin=41675450 rec_end=41675A9C rec_count=31 scan=31
    FM2_PLUME_D3D_DIRTY_STATE_AFTER_DIRECT_ARM limit=128
    ```
    The next lower-level dirty-state samples used command context `4004D100`
    and state shadow `4004D500`, confirming the IDA interpretation that
    `ctx + 0x400` is the real D3D command-context shadow.
  - The same post-direct smoke logged exactly 128
    `FM2_PLUME_D3D_DIRTY_STATE_ENTRY` lines with `after_direct=1` and 128
    `FM2_PLUME_DIRECT_STATE_BYTES role=d3d_ctx kind=state_shadow` dumps. The
    first entry was sample `n=56026` with
    `eff10=00000000001E0F47` and `eff18=00000000C0000001`, followed by
    repeated samples that commonly showed `eff18=0000000080000000`. This is
    the current bridge from the high-level direct draw packet to the lower-level
    state shadow that receives the compiled-state mask/value updates.
  - The first post-direct state-shadow bytes began:
    ```text
    84 02 48 02 10 E7 80 52 00 3F E1 FF 00 A8 0D 10
    00 00 02 43 10 E9 8A 18 82 02 48 02 09 DD 60 96
    ```
    Keep treating this as command-context shadow evidence, not as a direct
    vertex declaration yet. The next decoding step is to map the final
    `0x4800..0x481F` fetch constants from this shadow after applying the
    direct-draw compiled-state mask/value operations.
  - The June 18 fetch-shadow decoder established that `ctx + 0x400` is a
    0x300-byte Xenos fetch-constant shadow: 32 groups, 6 dwords per group. The
    focused smoke used:
    ```powershell
    .\scripts\fm2\Invoke-FM2GameplaySmoke.ps1 -Fm2Args @(
      '--fm2_plume_mode','shadow',
      '--fm2_plume_trace_packets',
      '--fm2_plume_trace_log_interval','120',
      '--fm2_plume_trace_direct_decode',
      '--fm2_plume_trace_direct_decode_limit','1',
      '--fm2_plume_trace_direct_decode_record_limit','31',
      '--fm2_plume_trace_direct_buffer_bytes','64',
      '--fm2_plume_trace_direct_state_bytes','768',
      '--fm2_plume_trace_direct_shader_bytes','1024',
      '--fm2_plume_trace_render_context_after_direct_limit','1',
      '--fm2_plume_trace_render_context_fetch_group_limit','32',
      '--mnk_mode',
      '--log_level','info'
    )
    ```
    The run logged `FM2_PLUME_D3D_FETCH_GROUP=32`,
    `FM2_PLUME_D3D_VERTEX_FETCH=2`, `FM2_PLUME_DIRECT_REPLAY_PLAN=31`,
    `FM2_PLUME_DIRECT_COMPILED_STATE_MASK_PAIR=32`, and zero
    `FM2_PLUME_DIRECT_SHADER_VERTEX_FETCH_ATTR` lines.
  - In that sample, the first fetch groups were texture constants:
    - group 0 / `reg_base=0x4800`: `format=18`, `endian=1`,
      `base=0x10E78000`, `mip=0x10E98000`, `dimension=1`, `width=512`,
      `height=512`.
    - group 1 / `reg_base=0x4806`: `format=22`, `endian=2`,
      `base=0x09DD6000`, `dimension=1`, `width=256`, `height=256`.
    - group 2 / `reg_base=0x480C`: `format=2`, `base=0x0823E000`,
      `dimension=1`, `width=88`, `height=24`.
    - group 3 / `reg_base=0x4812`: `format=2`, `base=0x0823B000`,
      `dimension=1`, `width=168`, `height=48`.
  - The same state-shadow sample had the currently important vertex data in
    group 31 / `reg_base=0x48BA`:
    ```text
    w0=00000001 w1=00000000 w2=109BF463 w3=1012574A w4=10E123FB w5=10000602
    ```
    It decodes to two valid vertex fetches:
    - fetch constant 94: `raw0=0x109BF463`, `raw1=0x1012574A`,
      `base=0x109BF460`, `endian=2`, `size_bytes=0x125748`.
    - fetch constant 95: `raw0=0x10E123FB`, `raw1=0x10000602`,
      `base=0x10E123F8`, `endian=2`, `size_bytes=0x600`.
    Fetch constant 94 matches the direct replay stream1 range after stripping
    the high guest-pointer/type bits from `s1_base=B09BF463` and aligning to
    `0x109BF460`, with the same `0x125748` byte count. This is the first
    concrete bridge from FM2's lower-level D3D fetch shadow to Plume buffer
    binding. Stream0 is still not visible in this one post-direct D3D shadow
    sample, so the next trace should correlate exact dirty-state timing or
    sample additional post-direct entries.
  - The follow-up smoke promoted that manual stream1 comparison into code. The
    decoder now has `MatchDirectDrawReplayStreamToVertexFetch`, and the runtime
    remembers up to 64 ready direct replay plans while fetch-shadow tracing is
    active. The same focused smoke logged exact counts:
    `FM2_PLUME_D3D_VERTEX_FETCH=2`,
    `FM2_PLUME_D3D_VERTEX_FETCH_REPLAY_MATCH=31`, and
    `FM2_PLUME_DIRECT_REPLAY_PLAN=31`. Every match was stream slot 1 /
    fetch constant 94 with `stream_base=B09BF463`,
    `stream_upload_base=B09BF460`, `stream_fetch_base=109BF460`,
    `stream_bytes=1201992`, `fetch_base=109BF460`, `fetch_bytes=1201992`,
    and `stream_hash=891376055F6CD53A`. This proves the shared stream1 binding
    can be recovered automatically for all 31 ready replay records in the
    sampled direct draw batch.
  - Use a larger `fm2_plume_trace_direct_state_bytes` value, such as `768` or
    `1024`, when the full compiled-state mask/value table is needed. The latest
    focused post-direct smoke used `768`, which was enough to dump the full
    fetch-constant shadow and the 32 mask/value pairs needed for the current
    stream/resource correlation work.
  - Fixed-offset vertex shader analysis at `payload + 0x30` was also tested
    with both loaded dword order and byte-swapped dwords. Both variants reported
    `valid=1` but `cf_pairs=0`, `bindings=0`, `attributes=0`, and empty
    `vfetch_bitmap*` fields. The top 15 whole-payload candidates likewise
    produced no vertex fetch attributes. Do not treat the current vertex
    structural candidate as an input-layout source.
  - Pixel shader payloads in this path currently validate as ordinary Xenos
    control-flow ucode: the observed pixel payload had a known byte count of
    `0x90`, structural used bytes of `0x84`, and a clean END-bearing exec
    control-flow scan. Vertex payloads still do not validate at fixed
    `payload + 0x30`; that offset sees first exec opcode `3` with address
    `3840`, outside the scanned payload. The whole-payload candidate scan now
    finds the best complete candidate at payload offset `0x268`
    (`candidate_base = B0BBEE88` in the sampled run), with used bytes `0x144`
    and structural hash `3028AB4DCA3D224A`. Treat this as a candidate until the
    translator confirms fetch declarations; an offline scan of the same payload
    produced several complete-looking smaller/overlapping candidates, so
    structural plausibility still needs validation.
  - The June 18 verified 4096-byte trace logged the top 15 structurally complete
    vertex ucode candidates for the sampled direct-draw vertex payload. Every
    candidate reported `bindings=0`, `attributes=0`, and empty
    `vfetch_bitmap*` fields in the vertex-fetch analyzer. The best candidate is
    therefore useful as a stable structural fingerprint, but not yet sufficient
    for generated-shader replay or input-layout recovery. Pixel still validates
    cleanly as a normal Xenos ucode payload. Next work should decode FM2's
    vertex payload wrapper and/or the vertex declaration state outside these
    candidate slices.
  - The June 18 aligned-upload correction established that buffer `gpu_base`
    values in this path carry low metadata bits. Replay uploads and buffer
    hashes now use `upload_guest_base = gpu_base & ~3`, while fetch-shadow
    matching still compares the normalized physical-style base
    `upload_guest_base & 0x1FFFFFFC`. The earlier hashes taken directly from
    raw bases such as `B09BF463` and `B0BBF697` were shifted by three bytes and
    should not be reused as replay keys. Current record-0 keys:
    - stream1: `gpu_base=B09BF463`, `upload_guest_base=B09BF460`,
      `view_bytes=0x125748`, `hash=891376055F6CD53A`
    - record 0 stream0: `gpu_base=B0BBF697`,
      `upload_guest_base=B0BBF694`, `view_bytes=0xFD60`,
      `hash=088CCFB61631658C`
    - record 0 index: `gpu_base=B0BCF3F4`,
      `upload_guest_base=B0BCF3F4`, `view_bytes=0x4704`,
      `hash=64270C4F8840C8FB`
    - vertex payload: `payload_bytes=0x4EC`,
      `payload_hash=924D29737CD56BDC`
    - pixel payload: `payload_bytes=0x90`,
      `payload_hash=225917CA19FF2FA7`
    - pixel structural ucode: `structural_hash_bytes=0x84`,
      `structural_hash=E4D8250D9CEA5612`
    - vertex structural ucode candidate: `candidate_off=0x268`,
      `structural_hash_bytes=0x144`,
      `structural_hash=3028AB4DCA3D224A`
  - The packet-summary smoke pass produced 31 `FM2_PLUME_DIRECT_PACKET` lines.
    All 31 had `buffers_ready=1`, `shader_keys=1`, and `debug_replay=1`, with
    no failed buffer or shader-payload hash reads. Record 0 had
    `first_index=0`, `index_count=4062`, `prim_count=1354`,
    `vertex_ucode_key=1`, `pixel_ucode_key=1`, and `full_ucode_keys=1` after
    candidate-scan fallback.
    Interpretation: a first debug-shader Plume replay can now consume the
    captured buffers and indexed draw arguments, while generated-shader replay
    still needs the vertex payload wrapper or vertex declaration decoded.
  - The replay-plan smoke pass produced 31 `FM2_PLUME_DIRECT_REPLAY_PLAN`
    lines, all with `ready=1`. Record 0 mapped to topology `1`
    (triangle list), index format `1` (16-bit), stream0 slot 0 stride `0x20`
    with `s0_base=B0BBF697`, `s0_upload_base=B0BBF694`, and `0xFD60`
    upload bytes, stream1 slot 1 stride `0x0C` with `s1_base=B09BF463`,
    `s1_upload_base=B09BF460`, and `0x125748` upload bytes, index base
    `B0BCF3F4` with `0x4704` upload bytes, `draw_index_count=4062`,
    `draw_instances=1`, `draw_start_index=0`, and `draw_base_vertex=0`.
    The same run submitted one Plume debug replay and the app log reported
    `FM2_PLUME_DEBUG_REPLAY_SUBMIT presented=true` with those upload bases.
    This is now the concrete input contract for the first Plume command-list
    replay implementation.
  - The June 18 position-stat smoke added
    `FM2_PLUME_DIRECT_STREAM0_POS_STATS` and verified that the stream0 position
    probe is reading plausible big-endian float3 data at byte offset 0 with
    stride `0x20`. The automated Plume smoke run saved
    `C:\temp\fm2-plume-pos-stats.png`, reported `nonDark=8800` and
    `avgLum=56.07`, and logged 31 position-stat records in
    `C:\temp\fm2-clean.log`. Record 0 had `vertices=2027`, `finite=2027`,
    `min=(-0.310680,-0.008298,-0.664231)`, and
    `max=(0.351440,1.464360,0.208522)`. Across all 31 records, every sampled
    vertex was finite (`totalFinite=59741`), vertex counts ranged
    `1565..2456`, and aggregate bounds were approximately
    `min=(-0.354551,-0.071900,-0.720329)` and
    `max=(0.526440,1.898650,0.209875)`.
    Interpretation: stream0 is local/object-space geometry with a consistent
    scale, not clip-space. The next renderer step should recover or inject the
    matching world/view/projection constants for this direct path and replace
    the debug shader's hardcoded position probe with a title-state transform.
  - The follow-up transform pass added host-order VS float constant extraction
    for replay plans. `DirectDrawDebugReplayPlan` now records
    `transform_valid` and `transform_first`; current smokes consistently build a
    valid transform from c28..c31. Record 0 logged
    `transform_valid=1 transform_first=28` with rows:
    c28 `(-3.2709331e-09, 0.00011646748, -1, 89.542145)`,
    c29 `(-2.808452e-05, 1, 0.00011646748, 1.1900483)`,
    c30 `(1, 2.808452e-05, -0, 289.91)`, and c31 `(0, -0, 0, 1)`.
    The Plume debug replay pipeline now has a vertex-stage push-constant block
    and the shader supports `fm2_plume_debug_replay_transform_mode=local`,
    `row_major_clip`, `column_major_clip`, and `row_major_clip_z_mid`. The
    default `local` mode keeps the existing diagnostic projection; the explicit
    transform modes consume the pushed transform rows for experiments.
  - Verification for the transform plumbing:
    `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
    followed by `unit_tests.exe "[fm2][plume]"` passed with 373 assertions in
    31 test cases, and the FM2 title build regenerated the debug replay DXIL and
    SPIR-V shader blobs. A shadow-mode smoke with
    `fm2_plume_debug_replay_transform_mode=local` produced
    `FM2_PLUME_DEBUG_REPLAY_SUBMIT presented=true ... transform_valid=1
    transform_first=28 transform_mode=local`, saved
    `C:\temp\fm2-plume-debug-replay-local.png`, and the screenshot sampler
    reported `nonDark=9355 avgLum=65.04`. A second smoke with
    `column_major_clip` submitted with `transform_mode=column_major_clip` and
    saved `C:\temp\fm2-plume-debug-replay-column-major-clip.png`
    (`nonDark=31495 avgLum=236.49`), but the image showed large clipped
    triangles rather than a correctly projected car. Interpretation: c28..c31 is
    real transform state and the Plume push-constant path works, but those rows
    alone are not the final clip-space transform. Next work should correlate
    c28..c31 with the other matrix-looking blocks, especially c36..c39 and
    c252..c255, or decode the FM2 vertex shader constant usage directly.
  - The next transform-correlation pass added
    `FM2_PLUME_DIRECT_TRANSFORM_CANDIDATE`, which scores sampled stream0
    positions through candidate constant blocks as row-major and column-major
    clip transforms. The focused smoke sampled all 2027 record-0 vertices. The
    strongest current native-placement candidate is `c36_mul_c28` as a
    row-major product: it produced `projectable=2027`, `xy_inside=2027`,
    `d3d_inside=0`, `gl_inside=0`, and tight NDC bounds
    `min=(-0.358687,0.717321,-1.150533)`,
    `max=(-0.352163,0.727459,-1.147395)`. This means c36*c28 maps the sampled
    object into a coherent, very small screen-space region, but the depth is
    just outside D3D's visible clip range. Other notable single-block scores:
    c28 column-major had `xy_inside=2023` but wide y/z bounds and only
    `d3d_inside=1011`; c36 column-major had `xy_inside=1606` but z was
    `1.007618..3.742261`; identity-like c68 only preserves local space.
  - `fm2_plume_direct_replay_transform_source` now controls which transform is
    put into the replay plan (`c28` by default, or `c36_mul_c28`). With
    `fm2_plume_direct_replay_transform_source=c36_mul_c28` and
    `fm2_plume_debug_replay_transform_mode=row_major_clip`, the Plume replay
    submitted with `transform_first=36` but rendered as a tiny clipped dot
    because z is outside D3D clip. With `row_major_clip_z_mid`, the shader keeps
    c36*c28 x/y/w and forces clip z to mid-depth; the smoke saved
    `C:\temp\fm2-plume-debug-replay-c36-mul-c28-zmid.png` and reported
    `nonDark=3345 avgLum=27.2`. The result is still a tiny native-positioned
    object, not a full debug-car view. Interpretation: c36*c28 is likely a real
    world/view/projection-stage candidate, but the sampled record is far/small
    in camera space or is missing an additional viewport/depth transform. Next
    useful step is to decode the vertex shader constant usage or score more
    direct records/passes to find a larger on-screen object and the correct
    depth convention.
  - The FM2 gameplay smoke harness now defaults to targeted window-message
    input (`InputMethod=postmessage`) instead of foreground-dependent
    `SendInput`. The targeted path uses `SendMessageTimeout` for focus and
    Space key down/up messages, so the harness does not count a queued or
    foreground-missed tap as success. The June 18 full trace smokes after this
    change consistently logged `keyDown=40 keyUp=40` and `aDown=36..37`.
  - `fm2_plume_trace_direct_decode_skip` now lets traces skip early direct
    indexed-draw calls without increasing log volume. A skip-8, limit-8 run
    sampled direct calls 9..16 and showed that `c0` interpreted as
    column-major clip is a strong candidate for several later records:
    sample 11/14 reached `D3DInside` around `0.915` in the aggregate. The
    existing `c36_mul_c28,row` candidate remains coherent for repeated early
    tiny objects but stays outside D3D depth for those samples.
  - `fm2_plume_direct_replay_transform_source` now also accepts `c0`,
    `fm2_plume_direct_replay_record_index` can filter the submitted debug
    replay record, and `fm2_plume_debug_replay_pipeline_topology` can force
    `triangle_list` or `triangle_strip` for diagnostic replay. These are
    instrumentation controls, not final renderer policy.
  - The important June 18 replay finding: sample 11, record 3, with
    `transform_source=c0`, `transform_mode=column_major_clip`, and
    `record_index=3` renders as a long triangle fan when forced through a
    triangle-list pipeline, but renders a recognizable seated human mesh when
    forced through a triangle-strip pipeline. Screenshot:
    `C:\temp\fm2-plume-debug-replay-c0-column-strip-skip10-rec3.png`
    (`nonDark=4784 avgLum=38.48`). Interpretation: at least this FM2 direct
    path batch is indexed triangle-strip geometry. Do not treat the current
    `prim_count=index_count/3` assumption as final native renderer semantics.
  - Follow-up smokes on June 18 refined that interpretation. The replay now has
    `fm2_plume_direct_replay_transform_source=auto`, which scores candidate VS
    constants against stream0 and rejects zero-area or large-XY-outlier
    projections. `fm2_plume_debug_replay_pipeline_topology=auto` carries the
    segment-header-inferred topology into the Plume pipeline; the app log shows
    `topology=2 plan_topology=2` for the sample-11/12 record-3 submissions.
    Even with bounded/full-stream transform scoring, transformed sample-12
    record-3 still rendered as a radial fan
    (`C:\temp\fm2-plume-debug-replay-auto-transform-fullsample-skip11-rec3.png`),
    while sample-11 record-3 with no accepted transform rendered a coherent
    local-space object
    (`C:\temp\fm2-plume-debug-replay-auto-transform-bounded-skip10-rec3.png`).
    Current interpretation: the remaining artifact is probably index/topology
    semantics, not just matrix selection. The next evidence step is to decode
    the converted index stream for the segment, including restart values and
    per-segment/subdraw boundaries, before changing the topology mapping again.
  - For a first Plume debug packet, use the confirmed buffer and draw
    arguments: 16-bit big-endian index data, `first_index = segment + 0x04`,
    `index_count = segment + 0x06`, stream0 stride `0x20`, stream1 stride
    `0x0C`, aligned upload bases `gpu_base & ~3`, and triangle-strip topology
    for the sample-11/record-3 character mesh. The original triangle-list
    assumption was useful for getting bytes on screen but is now contradicted
    by visual replay evidence.
  - Keep original Xenos draw enabled until a Plume debug draw presents from a
    captured packet.

## 2026-06-22 ReOdyssey Reference Pass

Reference project:
`C:\Users\Tera\Documents\GitHub\ReOdyssey`.

IDA context for this pass:

- IDB: `D:\Emulation\Games_Xbox_360\Forza2\default.xex.i64`
- Module: `default.xex`
- Image base: `0x82000000`
- Hex-Rays ready: yes

The important ReOdyssey lesson is architectural, not copy/paste. ReOdyssey
does not build a universal PM4/Xenos emulator for rendering. It hooks a
title-level D3D/RHI surface, stores game-facing resources as host objects, keeps
a host-side pipeline state, and flushes that state into Plume at draw time.

### Working Hypotheses

1. FM2 now has enough named render-context functions to build the same kind of
   host-side state tracker above PM4 packet emission.
   - Distinguishing evidence: shader, resource, vertex/index, surface,
     constant, and pass-boundary functions expose stable context slots and
     resource pointers.
   - Current status: supported by IDA decompilation listed below.
2. The existing direct-draw replay work is the right first draw source, but it
   should become an `FM2NativeDrawPacket` backed by a persistent state tracker,
   not remain a one-off debug replay path.
   - Distinguishing evidence: replay already has buffers, index arguments,
     shader payload hashes, constants, and topology evidence.
   - Current status: supported by June 18 replay notes.
3. The D3D PM4 emit cluster remains useful for validation and missing state, but
   it is too low-level to be the primary native renderer abstraction.
   - Distinguishing evidence: ReOdyssey succeeds by bypassing the guest GPU
     command stream and mirroring title D3D state before packet emission.
   - Current status: supported by ReOdyssey source and FM2 decompilation.

### ReOdyssey Architecture To Reuse

Source anchors:

- `src/render/guest_resources.h`
- `src/render/d3d_resource_hooks.cpp`
- `src/render/d3d_hooks.cpp`
- `src/render/render_state.cpp`
- `src/render/render_internal.h`
- `src/render/video.cpp`

Key pieces:

- `GuestResource` is a host-owned object with a magic value and a resource type.
  Derived types include textures, render targets, depth surfaces, buffers,
  vertex declarations, vertex shaders, and pixel shaders.
- `GuestBuffer` owns a Plume `RenderBuffer` plus mapped staging memory, byte
  size, and index format.
- `GuestShader` maps guest shader microcode to a generated shader-cache entry
  and owns a Plume `RenderShader` plus specialized shader variants.
- `Video::Init` creates the Plume interface, device, direct/copy queues,
  swapchain, descriptor sets, null texture descriptors, sampler descriptors,
  and a pipeline layout with root descriptors for constants.
- `ExecuteUpload` records copy work on a copy command list and waits for the
  copy fence, giving resource upload a simple synchronization contract.
- `PipelineState` stores vertex/pixel shaders, vertex declaration, blend,
  depth/stencil, cull mode, topology, vertex strides, render-target/depth
  formats, sample count, and shader specialization bits.
- `FlushRenderState` resolves pending texture copies, transitions render
  targets, binds framebuffer/viewport/scissor, gets or creates a pipeline,
  uploads constants, binds descriptor sets, and only then allows draw commands.
- Draw hooks call state setters, select or repair vertex declarations where the
  title is inconsistent, call `FlushRenderState`, and then issue Plume
  `drawIndexedInstanced` or `drawInstanced`.
- `Video::Present` resolves pending guest render-target content and blits the
  selected guest present source to the Plume swapchain.

For FM2, this means the next durable unit should be a native state layer, not a
larger PM4 decoder. The PM4 decoder/debug replay remains a probe and fallback
oracle while the higher-level state layer comes online.

### FM2 Hook Surface From Current IDA Names

The newest IDA names expose a practical first state-tracker surface:

- `0x8236DD10 = FM2_RenderContext_SetPixelShaderState`
  - Stores the resolved pixel shader object at render context `ctx+0x307C`.
  - If `shader+0x3C` is nonzero, the compiled state table is
    `shader+0x28+relative_offset`.
  - Runtime trace already identified pixel shader type tag `0x00100007` and
    payload pointer at `shader+0x18`.
- `0x8236E010 = FM2_RenderContext_SetVertexShaderState`
  - Stores the resolved vertex shader object at render context `ctx+0x3080`.
  - If `shader+0x37C` is nonzero, the compiled state table is
    `shader+0x368+relative_offset`.
  - Runtime trace already identified vertex shader type tag `0x00100006` and
    payload pointer at `shader+0x20`.
- `0x82375078 = FM2_RenderContext_SetShaderResourceState`
  - Allocates or advances a command buffer for shader/resource state packets.
  - This is useful validation, but still lower-level than the desired native
    state abstraction.
- `0x82370E48 = FM2_RenderContext_BindVertexStream`
  - Accepts stream slot, D3DResource pointer, byte offset, stride-like value,
    and dirty mask.
  - Reads guest buffer base from `resource+0x18` and byte size from
    `resource+0x1C`, then stores the converted base and remaining size into the
    render context.
  - Updates per-stream resource state near `ctx+0x2F94` and per-stream stride
    bytes near `ctx+0x2FD8`.
- `0x82370F68 = FM2_RenderContext_BindIndexBuffer`
  - Stores the index D3DResource pointer at render context `ctx+0x2F7C`.
- `0x82371A30 = FM2_RenderContext_SetBoundSurface`
  - Stores the surface pointer at render context `ctx+0x2F90`.
  - Copies surface fields from `surface+0x1C` and `surface+0x20` into context
    render-target state and marks framebuffer-related dirty bits.
- `0x823715C0 = FM2_RenderContext_UploadConstantBlock`
  - Thin wrapper over `FM2_RenderContext_UploadFloat6Constants`.
  - Good hook candidate for logging small constant blocks, but not enough alone
    for full matrix/pipeline constants.
- `0x8250F7C0 = FM2_Render_EmitPassDrawWork`
  - Validates pass/renderable state, obtains the global render context and
    active command-buffer context, uploads pass transform constants for one
    path, sets render-context state bits for normal pass draw, then calls the
    supplied draw callback as `draw_callback(renderable, pass_flags, draw_arg)`.
  - This is a strong pass-level boundary for native state snapshots.

Other named setters worth recording into the same state layer:

- `FM2_RenderContext_SetZEnableBit`
- `FM2_RenderContext_SetAlphaBlendEnableBits`
- `FM2_RenderContext_SetCullEnableState`
- `FM2_RenderContext_SetAlphaTestState`
- `FM2_RenderContext_SetBlendModeBits`
- `FM2_RenderContext_SetDepthCompareBits`
- `FM2_RenderContext_SetStencilOpBits`
- `FM2_RenderContext_SetColorWriteMaskBits`
- `FM2_RenderContext_SetPolygonModeBits`
- `FM2_RenderContext_SetDepthStencilEnableState`
- `FM2_RenderContext_SetSamplerStateLowNibble`
- `FM2_RenderContext_SetSamplerStateMidNibble`
- `FM2_RenderContext_SetSamplerStateHighNibble`
- `FM2_RenderContext_SetSamplerStateTopNibble`
- `FM2_RenderContext_SetTextureFetchBitsLow`
- `FM2_RenderContext_SetTextureFetchBitsMid`
- `FM2_RenderContext_SetViewportModeAndApply`
- `FM2_RenderContext_ApplyViewportConstants`
- `FM2_RenderContext_ExportViewportConstants`
- `FM2_RenderContext_UploadMatrixConstants`
- `FM2_RenderContext_UploadMatrixConstantsFromPass`

### Proposed FM2 Native State Layer

The next FM2 implementation should add a small state layer beside the existing
debug replay path:

- `FM2NativeResource`
  - Guest pointer, resource kind, guest base, byte size, Plume resource pointer
    when promoted, and trace hashes for validation.
- `FM2NativeShader`
  - Guest shader pointer, type tag, payload pointer, payload byte count,
    structural hash, generated shader-cache lookup result, and Plume shader
    pointer once available.
- `FM2NativePipelineState`
  - Vertex shader, pixel shader, stream bindings, index binding, surface
    bindings, blend/depth/stencil/cull bits, sampler/texture fetch bits,
    topology, viewport/scissor, and shader constant snapshot metadata.
- `FM2NativeDrawPacket`
  - Snapshot of `FM2NativePipelineState` plus draw arguments, stream/index
    upload ranges, transform-source decision, and the original FM2 pass/draw
    addresses for correlation.

This state layer should be read-only at first. It should not suppress Xenos
draws. It should only observe FM2 state mutations and build draw packets that
can be compared against the existing direct-draw replay plan.

### First Code Slice

Recommended next code slice:

1. Add `FM2/src/native_renderer/fm2_native_state.h` and `.cpp`.
2. Keep the public API narrow and Plume-free initially:
   - record pixel shader state
   - record vertex shader state
   - record vertex stream binding
   - record index binding
   - record bound surface
   - record pass draw boundary
   - snapshot state for direct indexed draw replay
3. Add manifest hooks for only the read-only state records, behind cvars:
   - `fm2_plume_native_state_trace`
   - `fm2_plume_native_state_trace_limit`
   - `fm2_plume_native_state_draw_snapshot`
4. At `FM2PlumeTraceDirectIndexedDrawEntry`, merge the existing
   `DirectDrawDebugReplayPlan` with the newest native state snapshot.
5. Do not replace rendering yet. The success signal for this slice is a log
   line that shows one draw packet with:
   - vertex/pixel shader pointers and payload hashes
   - stream0/index resource pointers and upload ranges
   - bound surface pointer
   - pass flags/draw callback context
   - topology source
   - transform candidate source

### Generator Implication

The generator path still looks feasible only after several handwritten native
renderers exist. ReOdyssey plus FM2 suggest the generator should not consume raw
PM4 as its primary input. A more realistic generator workflow is:

1. Discover and name title render API functions.
2. Classify create/set/draw/present/resource-lock functions.
3. Generate title-specific hook stubs and state-record structs.
4. Let humans fill or verify field offsets from decompilation/runtime probes.
5. Generate Plume translation scaffolding from the verified state schema.
6. Keep title quirks as explicit overrides, not hidden heuristics.

For FM2, the immediate path is therefore handwritten and evidence-driven. The
automation target is the workflow around the handwritten renderer, not a
universal native renderer generator yet.

### 2026-06-22 First Native State Recorder Slice

Implemented files:

- `FM2/src/native_renderer/fm2_native_state.h`
- `FM2/src/native_renderer/fm2_native_state.cpp`
- `tests/unit/fm2/native_state_test.cpp`

Build wiring:

- `FM2/CMakeLists.txt` now compiles `fm2_native_state.cpp` into `fm2`.
- `tests/unit/CMakeLists.txt` now compiles the same source into `unit_tests`.

Runtime wiring:

- `FM2PlumeTraceDirectIndexedDrawEntry` now records the direct render context
  and draw-interface pointer into the native state recorder.
- New cvars:
  - `fm2_plume_native_state_trace`
  - `fm2_plume_native_state_trace_limit`
- When enabled, the hook emits `FM2_PLUME_NATIVE_STATE` before the existing
  direct decode log for the same direct draw entry.

Verification:

- Red test was confirmed first: the new `native_state_test.cpp` failed to build
  because `native_renderer/fm2_native_state.h` did not exist.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests` passed.
- `unit_tests.exe "[fm2][plume]"` passed with 486 assertions in 42 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`.
- Gameplay smoke command used the existing
  `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` harness with targeted
  `postmessage` input and these added runtime args:
  `--fm2_plume_native_state_trace --fm2_plume_native_state_trace_limit 8`.
- The smoke reached gameplay input successfully:
  `Space tap result: ok=40 failed=0`, and app input verification reported
  `keyDown=40 keyUp=40 aDown=32`.
- `C:\temp\fm2-clean.log` contained the new snapshot line immediately before
  the direct decode line:
  - `FM2_PLUME_NATIVE_STATE n=1 valid=1 ctx=4153A5B0 seq=1 ... direct_valid=1 direct_iface=2E0162C0 ...`
  - `FM2_PLUME_DIRECT_DECODE n=1 ctx=4153A5B0 iface=2E0162C0 ...`

### 2026-06-22 Native State Setter Hook Slice

The next implementation target above is now implemented.

Manifest/codegen hooks added:

- `0x8236DD10 = FM2_RenderContext_SetPixelShaderState`
- `0x8236E010 = FM2_RenderContext_SetVertexShaderState`
- `0x82370E48 = FM2_RenderContext_BindVertexStream`
- `0x82370F68 = FM2_RenderContext_BindIndexBuffer`
- `0x82371A30 = FM2_RenderContext_SetBoundSurface`

Hook adapters added in `FM2/src/fm2_hooks.cpp`:

- `FM2PlumeTracePixelShaderState`
- `FM2PlumeTraceVertexShaderState`
- `FM2PlumeTraceVertexStreamBinding`
- `FM2PlumeTraceIndexBufferBinding`
- `FM2PlumeTraceBoundSurface`

Recorder API additions:

- `RecordNative*Args` adapters mirror the captured PPC register values and are
  covered by unit tests.
- `SnapshotNativeStateForDirectDraw` pairs the most recent render-context
  pipeline state with the direct draw entry.

Runtime finding:

- The direct indexed draw hook's `r3` value is not the same object as the
  render-context setter `r3` value. A direct-context lookup produced
  `direct_valid=1` but zero shader/stream/index/surface state even though the
  global recorder sequence counter had advanced into the hundreds of millions.
- The fix is to track the latest pipeline render context separately and attach
  each direct draw entry to that pipeline snapshot without changing the direct
  draw context key.

Verification:

- Red adapter test was confirmed first: `native_state_test.cpp` failed to
  compile on missing `RecordNative*Args` functions.
- Red pairing test was confirmed next: `native_state_test.cpp` failed to
  compile on missing `SnapshotNativeStateForDirectDraw`.
- `unit_tests.exe "[fm2][plume]"` passed with 512 assertions in 44 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/` with the existing `fopen`/`getenv` deprecation warnings in
  `fm2_hooks.cpp`.
- Gameplay smoke via `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` reached gameplay
  input with `Space tap result: ok=40 failed=0` and input verification
  `keyDown=40 keyUp=40 aDown=36`.
- `C:\temp\fm2-clean.log` then showed populated native state:
  - `FM2_PLUME_NATIVE_STATE n=1 valid=1 ctx=4004D100 seq=167161782 vs_valid=1 vs=4181A600 ps_valid=1 ps=2E8F24B0 s0_valid=1 s0_res=BACACA50 s0_stride=16 s1_valid=1 s1_res=2ECFFA80 s1_stride=12 ib_valid=1 ib=BACACAE0 surf_valid=1 surf=2E049240 direct_valid=1 direct_iface=2E0162C0`
  - The paired direct decode immediately after used direct context
    `ctx=42950010`, confirming the two-context pairing is intentional.

Next implementation target:

1. Feed `SnapshotNativeStateForDirectDraw` into `DirectDrawDebugReplayPlan`
   construction instead of relying only on the direct record decode.
2. Add pass-boundary or draw-callback context once the native draw submit path
   needs `FM2_Render_EmitPassDrawWork` fields.
3. Start resolving shader payloads, vertex stream guest buffers, index buffers,
   and bound surface metadata from the paired snapshot into Plume resource
   handles.

### 2026-06-22 Plume Compare Window Slice

The first side-by-side comparison mode is now implemented. This does not replace
the ReX/Xenos renderer yet; it keeps the existing renderer in the main FM2 window
and opens a second Plume-backed HWND for native-renderer comparison output.

Runtime cvar:

- `fm2_plume_compare_window`

Expected launch shape:

```powershell
FM2\out\build\win-amd64-relwithdebinfo\fm2.exe `
  --fm2_plume_mode shadow `
  --fm2_plume_compare_window `
  --mnk_mode
```

Behavior:

- `shadow` mode still lets the original ReX/Xenos renderer own the normal game
  window and frame presentation.
- `fm2_plume_compare_window` creates the existing Plume diagnostic window path
  as a separate window titled `FM2 Plume Compare`.
- Compare-window mode implies direct debug replay internally. It no longer
  requires `fm2_plume_trace_direct_decode` just to decode replayable direct draw
  contexts.
- Compare-window mode bypasses the debug replay submit limit because this mode
  is meant to continuously compare against the live game, not sample one record.
- For each direct indexed draw entry, compare mode scans all decoded direct
  records currently admitted by the decode cap and submits every replay-ready
  record into a single Plume present when those records are compatible.
- The batch submit path acquires the Plume swapchain once, clears once, emits all
  compatible prepared draws, and presents once for that decoded direct context.

Implemented files:

- `FM2/src/native_renderer/fm2_direct_draw_decode.h`
- `FM2/src/native_renderer/fm2_native_renderer.h`
- `FM2/src/native_renderer/fm2_native_renderer.cpp`
- `FM2/src/fm2_hooks.cpp`
- `tests/unit/fm2/direct_draw_decode_test.cpp`

Unit coverage:

- `ShouldDecodeDirectDrawForCompareReplay` keeps trace and compare gating
  explicit.
- `DirectCompareReplayRecordScanCount` documents that compare mode scans the
  available decoded records unless a future explicit cap is provided.
- `DirectDebugReplaySubmitLimitReached` documents that compare-window mode
  ignores the trace-only submit limit.

Verification:

- Red test was confirmed first: `direct_draw_decode_test.cpp` failed to compile
  on missing compare replay policy helpers.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests` passed.
- `unit_tests.exe "[fm2][plume]"` passed with 521 assertions in 45 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/` with the existing `fopen`/`getenv` deprecation warnings in
  `fm2_hooks.cpp`.
- Gameplay smoke via `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` used these added
  runtime args:
  `--fm2_plume_mode shadow --fm2_plume_compare_window --mnk_mode`.
- The smoke reached gameplay input successfully:
  `Space tap result: ok=40 failed=0`, and app input verification reported
  `keyDown=40 keyUp=40 aDown=36`.
- FM2 app logs showed the comparison window and swapchain initialization:
  - `FM2 Plume debug replay window created hwnd=0x110157e`
  - `FM2 Plume swapchain initialized size=960x540 textures=2`
  - `FM2 Plume native renderer initialized mode=shadow`
- The same run produced repeated compare-window batch presents:
  - `FM2_PLUME_COMPARE_REPLAY_SUBMIT presented=true image=... size=960x540 draws=31 skipped=0 topology=2`
- `C:\temp\fm2-clean.log` showed compare mode decoding without the trace cvar:
  - `FM2_PLUME_DIRECT_DECODE n=689 ... rec_count=31 scan=31`
  - `FM2_PLUME_COMPARE_REPLAY_RESULT n=689 submissions=31 submitted=1 mode=shadow`

Current limitations:

- The second window is still powered by the direct debug replay path and debug
  shaders. It is not yet a render-target-accurate native renderer.
- The compare window presents per decoded direct context, not once per final game
  frame. It is useful for proving draw extraction and Plume submission, but not
  yet for exact frame composition.
- Batch replay currently requires compatible effective topology across the
  prepared records in the batch.
- Compare mode can produce very large diagnostic logs when the existing
  `LogLine` sink is enabled. One smoke run grew `C:\temp\fm2-clean.log` to about
  78 MB because compare mode decodes frequently.

Next implementation target:

1. Use the paired `SnapshotNativeStateForDirectDraw` pipeline snapshot to bind
   actual FM2 shader, vertex stream, index buffer, and surface resources instead
   of the direct debug replay placeholders.
2. Add a frame or pass boundary hook so the Plume comparison window can present
   once per original frame.
3. Throttle or split compare-window diagnostic logging so long gameplay runs do
   not balloon `C:\temp\fm2-clean.log`.

### 2026-06-22 Replay Plan Native-State Provenance Slice

The first part of the next target is now implemented. Each direct debug replay
plan can now carry the paired native pipeline snapshot captured by
`SnapshotNativeStateForDirectDraw`.

Important distinction:

- The plan now contains native resource provenance.
- The Plume draw still uses the direct debug replay buffers and debug shaders.
  This avoids pretending the output is fully native when the captured native
  stream layout does not match the current debug shader input contract.

Replay plan additions:

- `DirectDrawReplayNativeStateSummary`
- `DirectDrawReplayNativeStreamBinding`
- `DirectDrawDebugReplayPlan::native_state`
- `BuildDirectDrawDebugReplayPlan(packet, snapshot)`

Native fields carried per plan:

- Paired render context and recorder sequence.
- Direct render context and draw-interface pointer.
- Vertex shader object pointer.
- Pixel shader object pointer.
- Stream 0 and stream 1 resource pointer, byte offset, stride, and dirty mask.
- Index-buffer resource pointer.
- Bound surface pointer and surface argument.

Runtime wiring:

- `MaybeLogPlumeDirectIndexedDrawDecode` now captures
  `SnapshotNativeStateForDirectDraw(direct_render_ctx)` once per direct draw
  decode and passes it into each record's replay plan.
- Each record with a valid snapshot emits:
  `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE`.
- The Plume compare batch submit log now reports `native_state_draws`, so the
  app log can prove the comparison window received replay plans with native
  state attached.

Observed runtime behavior:

- Gameplay smoke reached a draw where every submitted comparison draw had native
  state attached:
  - `FM2_PLUME_COMPARE_REPLAY_SUBMIT presented=true image=... size=960x540 draws=31 skipped=0 native_state_draws=31 topology=2`
- `C:\temp\fm2-clean.log` showed concrete paired resource provenance:
  - `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE n=642 rec_i=0000000B ctx=4004D100 seq=223189951 direct_ctx=4209B700 iface=2E0162C0 vs=42099650 ps=2EEA54B0 s0_valid=1 s0_res=2EF2A900 s0_offset=0 s0_stride=28 s0_dirty=00000001 s1_valid=1 s1_res=2E660E40 s1_offset=0 s1_stride=12 s1_dirty=00000001 ib=2E2867E0 surf=2E049240 surf_arg=0`

Current limitation:

- The captured native stream 0 stride in the smoke was `28`, while the current
  debug replay Plume pipeline is hard-coded for stream 0 stride `0x20` and
  stream 1 stride `0x0C`. That confirms the native-state path is exposing real
  FM2 pipeline data, but the debug shader/input layout is not a valid consumer
  for all native stream layouts.

Verification:

- Red test was confirmed first: `direct_draw_decode_test.cpp` failed to compile
  because `BuildDirectDrawDebugReplayPlan(packet, snapshot)` did not exist.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests` passed.
- `unit_tests.exe "[fm2][plume]"` passed with 542 assertions in 46 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/` with the existing `fopen`/`getenv` deprecation warnings in
  `fm2_hooks.cpp`.
- Gameplay smoke via `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` used:
  `--fm2_plume_mode shadow --fm2_plume_compare_window --mnk_mode`.
- The smoke reached gameplay input successfully:
  `Space tap result: ok=40 failed=0`, and app input verification reported
  `keyDown=40 keyUp=40 aDown=36`.

Next implementation target:

1. Decode native stream/index D3DResource objects from the stored native resource
   pointers into explicit guest base, byte size, format, and byte-range metadata.
2. Add a native input-layout/shader path that can consume stream 0 stride `28`
   and other observed FM2 layouts instead of forcing the debug replay
   `0x20/0x0C` contract.
3. Promote vertex/pixel shader object pointers into a shader cache key that can
   select or generate a Plume shader for the native path.

### 2026-06-22 Native 28/12 Compare Layout Slice

The first native input-layout promotion is now implemented for the observed FM2
stream contract:

- Stream 0 slot `0`, stride `28`, position at byte offset `0`.
- Stream 1 slot `1`, stride `12`.
- Index format still comes from the direct-record index descriptor for now.

Added renderer pieces:

- `DirectDrawReplayPipelineLayout`
- `DirectDrawReplayPipelineLayoutForPlan`
- `DirectDrawReplayNativeLayoutFromState`
- `BuildDirectDrawNativeLayoutReplayPlan`
- `FM2/src/native_renderer/shaders/fm2_debug_replay_native28.vert.hlsl`

Runtime behavior:

- Direct replay plans still start as the old debug replay contract
  `debug_raw32_side12`.
- Compare mode now attempts a native promotion when the paired native snapshot
  reports `stream0 stride=28` and `stream1 stride=12`.
- The hook decodes the native stream/index D3DResource pointers into guest base,
  upload base, byte range, descriptor count, and hash metadata.
- If all three native buffers are readable and hashable, the compare submission
  uses the native buffer sources and `native_position28_side12` Plume pipeline.
- If native promotion is unsupported or unreadable, the old direct debug replay
  path remains the fallback.

Verification:

- Red test was confirmed first: `direct_draw_decode_test.cpp` failed to compile
  on missing layout/promotion helpers.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed.
- `unit_tests.exe "[fm2][plume]"` passed with 556 assertions in 47 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` regenerated
  and embedded `fm2DebugReplayNative28Vert` DXIL/SPIR-V, then linked `fm2.exe`.
- Gameplay smoke via `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` used:
  `--fm2_plume_mode shadow --fm2_plume_compare_window --mnk_mode`.
- The smoke reached gameplay input successfully:
  `Space tap result: ok=40 failed=0`, and app input verification reported
  `keyDown=40 keyUp=40 aDown=36`.
- FM2 app logs showed both pipeline variants during the run:
  - `FM2 Plume debug replay pipeline initialized topology=2 layout=debug_raw32_side12`
  - `FM2 Plume debug replay pipeline initialized topology=2 layout=native_position28_side12`
- The compare window then presented native-layout batches:
  - `FM2_PLUME_COMPARE_REPLAY_SUBMIT presented=true image=... size=960x540 draws=31 skipped=0 native_state_draws=31 native_layout_draws=31 topology=2 layout=native_position28_side12`
- `C:\temp\fm2-clean.log` showed native resource decode and promotion:
  - `FM2_PLUME_NATIVE_D3DRESOURCE n=631 rec_i=00000000 role=native_stream0 resource=2E61E140 gpu_base=A9E192C3 upload_base=A9E192C0 byte_size=1003D862 readable_bytes=0003D862 byte_offset=0 stride_or_format=28 descriptor_count=9000 view_bytes=0003D860 hash_bytes=0003D860 hash_ok=1 hash=5AC6954F6A62573B index=0`
  - `FM2_PLUME_NATIVE_REPLAY_PLAN n=631 rec_i=00000000 ready=1 layout=native_position28_side12 s0_base=A9E192C3 s0_upload_base=A9E192C0 s0_stride=28 s0_bytes=0003D860 s1_base=B09BF463 s1_upload_base=B09BF460 s1_stride=12 s1_bytes=00125748 index_base=A9E56B68 index_upload_base=A9E56B68 index_bytes=00007D00`

Current limitation:

- This is still a diagnostic native-layout shader, not generated FM2 shader
  translation. It consumes native stream bytes with a layout that matches the
  captured state, but it does not yet execute FM2's vertex/pixel shader logic.

Next implementation target:

1. Promote vertex/pixel shader object pointers into a stable shader-cache key.
2. Decode enough vertex declaration/fetch metadata to map native stream elements
   beyond the assumed position-at-offset-0 case.
3. Start generating or selecting Plume shaders from the captured FM2 shader
   payloads instead of using diagnostic visualization shaders.

### 2026-06-22 IDA Hook Surface Pivot

After comparing with `ReOdyssey` and `UnleashedRecomp`, the FM2 Plume path
should pivot from diagnostic replay toward the same physical pattern those
projects use:

```text
FM2 render call -> replacement hook -> native Plume state/resources -> Plume draw/present
```

IDA hook-surface pass result:

- `FM2_Render_InstanceHybridDrawPath` (`0x82539650`) is the best first
  per-frame product hook. It is the direct caller above
  `FM2_Render_BuildDirectIndexedDrawBuffers` and fires repeatedly for the
  hybrid draw path without the builder's one-shot guard.
- `FM2_Render_BuildDirectIndexedDrawBuffers` (`0x825380B8`) is a valuable
  direct-record compiler/discovery hook. It consumes `direct_render_ctx` and
  `draw_iface`, resolves direct draw records, binds surface/state, binds
  resource slots through draw-interface methods, issues indexed draw calls,
  finalizes batches, and stores clones, but the guard byte at
  `direct_render_ctx + 0x48` makes it unsuitable as the primary per-frame
  renderer hook.
- The direct indexed path exposes exactly the data the current diagnostic replay
  had to rediscover indirectly: stream resources, index resource, pass constants,
  surface, primitive type, start index, and primitive count.
- Render-context setters/binders should become the Plume state mirror:
  `FM2_RenderContext_SetPixelShaderState`,
  `FM2_RenderContext_SetVertexShaderState`,
  `FM2_RenderContext_BindVertexStream`,
  `FM2_RenderContext_BindIndexBuffer`, and
  `FM2_RenderContext_SetBoundSurface`.
- Present should start at `FM2_D3D_LazyInitPresentChain` and
  `FM2_D3D_TryPresentAndUpdateStatus`.
- Cached draw lists should come after direct indexed draw, using
  `FM2_Render_ExecuteBoundDrawPass`, `FM2_Render_WalkAndDispatchPm4DrawList`,
  and `FM2_Render_DrawIndexedPrimitive`.
- Low-level emitters such as `FM2_D3D_EmitDirtyStateAndDrawList`,
  `FM2_D3D_EmitDrawListStatePackets`, and
  `FM2_D3D_EmitSurfaceResolvePackets` remain useful diagnostics/fallbacks, but
  they are not the desired first product hook layer.

New IDA names from this pass are logged in
`docs/FM2-ida-renames-2026-06-22.md` and mirrored into
`FM2/fm2_manifest.toml`.

Immediate implementation target:

1. Keep the existing compare window for validation.
2. Add a Plume native state mirror fed by the render-context setters/binders.
3. Hook `FM2_Render_InstanceHybridDrawPath` for the first per-frame native
   Plume world-geometry draw, using `FM2_Render_BuildDirectIndexedDrawBuffers`
   only to validate direct-record layout/resource provenance.
4. Use `FM2_D3D_TryPresentAndUpdateStatus` as the first frame-present bridge
   once native draws are visible.

### 2026-06-22 Native Direct Draw Submission Boundary

The first product-path boundary is now separated from diagnostic debug replay.
The implementation still reuses the proven Plume direct replay upload/draw
primitive internally, but the hook and renderer now expose a distinct native
direct-draw path:

- `fm2_plume_native_direct_draw` enables native direct indexed draw submission.
- `fm2_plume_native_direct_draw_limit` defaults to `1`; `0` disables the limit.
- `WantsNativeDirectDraw()` keeps the `0x825380B8` direct-draw decoder active
  even when `fm2_plume_trace_direct_decode` and compare mode are disabled.
- `SubmitNativeDirectDraw()` tracks its own attempts/submitted/failed counters
  in `fm2::native_renderer::Stats`.
- Runtime log markers:
  - `FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT`
  - `FM2_PLUME_NATIVE_DIRECT_DRAW_RESULT`

Hook behavior:

- `MaybeLogPlumeDirectIndexedDrawDecode()` now uses
  `ShouldDecodeDirectDrawForPlumeSubmission(trace, compare, native_direct)`.
- Native direct draw scans all direct records while enabled, matching compare
  replay, so it can find the first ready native-layout record instead of being
  capped by trace-only decode limits.
- The native-layout promotion path is used by compare replay and one-shot native
  direct draw. If the paired native state reports stream 0 stride `28`, stream 1
  stride `12`, and a readable native index resource, the submission uses
  `native_position28_side12`.
- Unlimited live native batching intentionally keeps the per-record decoded
  replay buffers instead of promoting through paired native-state resources.
  Runtime logs showed the promoted native stream/index resources repeated the
  same triplet across many records (`A9E192C0` / `B09BF460` / `A9E56C4C`),
  matching the observed fixed car-like wireframe. The per-record decoded plans
  carry varied stream/index upload bases and auto-selected transforms, so they
  are the better diagnostic source until the product hook moves up to
  `FM2_Render_InstanceHybridDrawPath`.
- If native promotion is not ready, the submission path falls back to the
  existing direct replay plan shape for now.

Verification from this slice:

- Red test confirmed first: `unit_tests` failed to compile on missing
  `ShouldDecodeDirectDrawForPlumeSubmission` and
  `DirectPlumeSubmissionRecordScanCount`.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed after implementation.
- `unit_tests.exe "[fm2][plume]"` passed with 567 assertions in 48 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` linked
  `fm2.exe`; the build still reports the existing `fopen`/`getenv`
  deprecation warnings in `FM2/src/fm2_hooks.cpp`.
- Gameplay smoke with `--fm2_plume_mode shadow`,
  `--fm2_plume_native_direct_draw`, `--fm2_plume_native_direct_draw_limit 1`,
  `--fm2_plume_debug_replay_window`, and `--mnk_mode` reached gameplay input,
  submitted through the new native-direct API, and captured a non-dark Plume
  replay window:
  `C:\temp\fm2-native-direct-smoke.png`.
- Smoke log evidence:
  - `FM2_PLUME_NATIVE_DIRECT_DRAW_RESULT n=1 rec_i=00000000 submitted=1 mode=shadow layout=debug_raw32_side12`
  - `FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT attempt=1 submitted=1 mode=shadow topology=2 layout=debug_raw32_side12 index_count=4062 native_state=1 transform_valid=1`
  - `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE ... s0_stride=16 ... s1_stride=12 ...`

Current limitation:

- This is a native draw submission boundary, not yet a full FM2 native renderer.
  It still relies on the diagnostic replay shader/pipeline machinery beneath
  `RenderDirectDebugReplayLocked`.
- The first submitted smoke record used the fallback `debug_raw32_side12`
  replay layout because its paired native stream 0 state reported stride `16`,
  not the `28` expected by the current `native_position28_side12` promotion.
- The next replacement step is to split `RenderDirectDebugReplayLocked` into a
  renderer-neutral native direct draw executor, then move from diagnostic
  shaders to shader-cache keys derived from the FM2 vertex/pixel shader objects.

### 2026-06-22 Side-by-side Plume Wireframe Comparison

The Plume replay/comparison window can now be launched beside the original FM2
window and rendered in rasterizer wireframe mode.

New runtime switches:

- `--fm2_plume_debug_replay_side_by_side` places the separate Plume replay
  window to the right of the FM2 game window.
- `--fm2_plume_debug_replay_window_gap <pixels>` controls the gap; default is
  `24`.
- `--fm2_plume_wireframe` builds the Plume replay pipeline with wireframe fill.

Plume API/backend changes:

- `RenderGraphicsPipelineDesc` now has `fillMode`.
- D3D12 maps `RenderFillMode::WIREFRAME` to `D3D12_FILL_MODE_WIREFRAME`.
- Vulkan maps `RenderFillMode::WIREFRAME` to `VK_POLYGON_MODE_LINE`.
- Metal carries the fill mode through `MetalRenderState` and binds
  `MTL::TriangleFillModeLines`.

Useful launch modes:

```powershell
# Live side-by-side wireframe snapshots. This submits repeatedly.
--fm2_plume_mode shadow --fm2_plume_native_direct_draw `
--fm2_plume_native_direct_draw_limit 0 --fm2_plume_debug_replay_window `
--fm2_plume_debug_replay_side_by_side --fm2_plume_wireframe --mnk_mode

# Stable first visible wireframe draw for inspection/capture.
--fm2_plume_mode shadow --fm2_plume_native_direct_draw `
--fm2_plume_native_direct_draw_limit 1 --fm2_plume_debug_replay_window `
--fm2_plume_debug_replay_side_by_side --fm2_plume_wireframe --mnk_mode
```

Verification:

- `unit_tests.exe "[fm2][plume]"` passed with 577 assertions in 50 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` rebuilt
  Plume D3D12/Vulkan and linked `fm2.exe`.
- Broad root `install` was not used as proof because it currently stops in PPC
  test codegen with `--bin-dir is required`; FM2 builds Plume directly as a
  subdirectory.
- Live smoke with `--fm2_plume_native_direct_draw_limit 0` showed repeated
  successful native direct draw submissions and `fill=wireframe`.
- Stable smoke with `--fm2_plume_native_direct_draw_limit 1` captured visible
  wireframe geometry at `C:\temp\fm2-plume-wireframe-limit1.png`.
- Placement log from the stable run:
  `FM2 Plume debug replay window created hwnd=0x15e116a x=1450 y=130 size=976x579 side_by_side=1 host_x=130 host_y=130 host_size=1296x759`
- Pipeline/submission log from the stable run:
  `FM2 Plume debug replay pipeline initialized topology=2 layout=debug_raw32_side12 fill=wireframe`
  and
  `FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT attempt=1 submitted=1 mode=shadow topology=2 layout=debug_raw32_side12 index_count=4062 native_state=1 transform_valid=1`

Current limitation:

- The live side-by-side path now batches accepted direct replay records before
  presenting, but it is still not a true native frame. It composes compatible
  diagnostic replay records in chunks and clears once per chunk; it does not yet
  own the real FM2 frame/pass boundary, camera state, material state, or cached
  draw-list execution.
- The stable `limit=1` mode is the best current visual inspection mode for the
  first visible wireframe draw.

### 2026-06-22 Live Batch Follow-up

User observation after the first live-batch smoke: once initial gameplay starts,
the Plume replay window shows a messy wireframe resembling a car, ignores camera
position/environment/other track vehicles, and disappears after the race
countdown.

Log correlation from the same run:

- Native live batches continued presenting near process shutdown:
  `FM2_PLUME_NATIVE_LIVE_BATCH_SUBMIT presented=true ... draws=16 skipped=0`.
- The repeated car-like geometry came from native-state promotion, not from the
  batch renderer. `FM2_PLUME_NATIVE_REPLAY_PLAN` repeatedly selected the same
  native resource triplet:
  `s0_upload_base=A9E192C0`, `s1_upload_base=B09BF460`,
  `index_upload_base=A9E56C4C`.
- The decoded direct replay records in the same samples had varied stream/index
  upload bases (`B0BB...`, `B0BD...`, `B0D0...`) and auto-selected transforms.

Code change from this follow-up:

- Added live-batch policy helpers:
  `ShouldStopDirectPlumeRecordScanAfterNativeAttempt(..., live_batch)` and
  `ShouldPromoteDirectReplayToNativeLayout(compare, native, live_batch)`.
- Added cvars:
  `fm2_plume_native_direct_draw_live_batch` and
  `fm2_plume_native_direct_draw_live_batch_size`.
- `SubmitNativeDirectDraw()` now owns copied source bytes in a pending queue and
  flushes with `FM2_PLUME_NATIVE_LIVE_BATCH_SUBMIT`.
- Unlimited live native batching skips stale native-state promotion and submits
  the varied per-record `debug_raw32_side12` replay plans instead.

Verification:

- `unit_tests.exe "[fm2][plume]"` passed with 588 assertions in 50 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` rebuilt
  `fm2.exe`; only the pre-existing `fopen`/`getenv` deprecation warnings in
  `FM2/src/fm2_hooks.cpp` appeared.
- Gameplay smoke with `--fm2_plume_native_direct_draw_limit 0`,
  `--fm2_plume_native_direct_draw_live_batch`,
  `--fm2_plume_native_direct_draw_live_batch_size 16`,
  `--fm2_plume_debug_replay_window`, `--fm2_plume_debug_replay_side_by_side`,
  `--fm2_plume_wireframe`, and `--mnk_mode` reached gameplay input with
  `Space tap result: ok=40 failed=0`.

Next renderer step:

- Stop treating `0x825380B8` as the live renderer boundary. Use
  `FM2_Render_InstanceHybridDrawPath` for per-frame world geometry, keep
  render-context setters as the Plume state mirror, and use
  `FM2_D3D_TryPresentAndUpdateStatus` for present ownership.

### 2026-06-22 Native Draw Semantic State Handoff

This slice moves FM2 closer to the ReOdyssey/Unleashed native-renderer shape:
each draw plan now carries the semantic API state captured by the FM2 hooks,
instead of only the raw replay buffers and the older shader/stream/surface
subset.

Implemented files:

- `FM2/src/native_renderer/fm2_direct_draw_decode.h`
- `FM2/src/fm2_hooks.cpp`
- `FM2/src/native_renderer/fm2_native_renderer.cpp`
- `tests/unit/fm2/direct_draw_decode_test.cpp`

State now preserved in `DirectDrawReplayNativeStateSummary`:

- viewport mode from `FM2PlumeTraceViewportMode`
- texture-fetch low/mid bits from `FM2PlumeTraceTextureFetchLow/Mid`
- clear color byte and clear flags from `FM2PlumeTraceClearColor/Flags`
- pass boundary metadata from `FM2PlumeTracePassDrawWork`

Runtime trace updates:

- `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE` now logs
  `viewport_valid`, `texture_fetch_valid`, `clear_valid`, and `pass_valid`
  plus the relevant values.
- `FM2_PLUME_NATIVE_DIRECT_DRAW_SUBMIT` now logs whether viewport, texture
  fetch, clear, and pass state are present on the submitted draw plan.

Verification:

- Red test was confirmed first: `unit_tests` failed to compile because
  `DirectDrawReplayNativeStateSummary` had no `viewport`, `texture_fetch`,
  `clear`, or `pass` members.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  623 assertions in 52 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`; the build still emits the existing deprecated CRT warnings in
  `FM2/src/fm2_hooks.cpp`.
- Gameplay smoke via `scripts/fm2/Invoke-FM2GameplaySmoke.ps1` reached
  gameplay input with `Space tap result: ok=40 failed=0` and
  `keyDown=40 keyUp=40 aDown=37`.
- `C:\temp\fm2-clean.log` contained 18,097
  `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE` lines with the new fields; sample
  values included `viewport_valid=1`, `texture_fetch_valid=1`,
  `clear_valid=1`, `pass_valid=1`, and `pass_flags=00000001`.
- Fresh app logs `FM2/out/build/win-amd64-relwithdebinfo/logs/fm2_150*.log`
  contained live submissions with
  `viewport=1 texture_fetch=1 clear=1 pass=1 pass_flags=00000001`, and
  repeated `FM2_PLUME_NATIVE_LIVE_BATCH_SUBMIT` lines with
  `draws=16 skipped=0 native_state_draws=16`.

### 2026-06-22 Native Pass Command Boundary

This slice starts using the carried semantic state as a native-renderer
boundary instead of treating it as logging-only metadata.

Implemented files:

- `FM2/src/native_renderer/fm2_native_draw.h`
- `FM2/src/native_renderer/fm2_native_renderer.cpp`
- `tests/unit/fm2/native_draw_test.cpp`

New abstraction:

- `NativeDrawPacket` remains the per-draw semantic object: FM2 pass key,
  pipeline key, native/replay resource key, and indexed draw call.
- `NativePassCommand` is the next layer up. It groups ready draw packets that
  share a single FM2 pass key and rejects batches that are empty, contain a
  rejected draw packet, cross pass boundaries, or exceed the fixed pass-command
  draw limit.
- `BuildNativePassCommand()` is still host-side metadata, not a true Plume draw
  submitter. It defines the command shape that a real FM2 native pass backend
  can consume later.

Runtime behavior:

- `SubmitNativeDirectDraw()` now rejects native direct draw attempts whose
  `NativeDrawPacket` is incomplete, instead of blindly submitting decoded replay
  buffers through the native-direct path.
- Live native-direct batching now stores each pending draw's
  `NativeDrawPacket`.
- A pending live batch flushes when the next ready packet belongs to a different
  FM2 pass key, so pass boundaries are explicit before the eventual native pass
  submitter exists.
- `FlushNativeDirectDrawBatchLocked()` builds a `NativePassCommand` from pending
  packets before replay-backed submission. If the command is rejected, the batch
  is dropped with a `FM2_PLUME_NATIVE_PASS_REJECT` log. If accepted, the old
  replay-backed renderer still performs the visible draw for now and logs pass
  metadata in `FM2_PLUME_NATIVE_LIVE_BATCH_RESULT`.

Current limitation:

- This is not yet native Plume geometry/state submission. It is the control
  boundary immediately before that work: semantic packets are now required and
  grouped by FM2 pass, but accepted commands still render through the existing
  direct debug replay path.

Verification:

- Red test was confirmed first: `cmake --build --preset
  win-amd64-relwithdebinfo --target unit_tests` failed to compile because
  `NativePassCommand`, `NativePassCommandRejectReason`, and
  `BuildNativePassCommand()` did not exist.
- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed after implementation.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  693 assertions in 56 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`.

### 2026-06-22 Native Pass Plume Submitter

This slice replaces the replay-backed native-direct batch flush with a real
`NativePassCommand` submit boundary that records Plume command lists directly.
It is intentionally still conservative: the submitter only accepts packets that
share one pass, one topology, one index format, and one vertex layout.

Implemented files:

- `FM2/src/native_renderer/fm2_native_draw.h`
- `FM2/src/native_renderer/fm2_direct_draw_decode.h`
- `FM2/src/native_renderer/fm2_native_renderer.cpp`
- `tests/unit/fm2/native_draw_test.cpp`
- `tests/unit/fm2/direct_draw_decode_test.cpp`

New abstraction:

- `NativePassSubmitPlan` is the pure validation layer between
  `NativePassCommand` and Plume submission.
- Native-state resources are accepted only when the replay and native layouts
  match, such as `native_position28_side12`.
- Direct replay resources are accepted as a fallback when FM2 has valid semantic
  pass/pipeline state but the captured native stream/index binding is incomplete.
- Mixed pass, topology, index-format, or layout batches are rejected before any
  Plume command-list work.

Runtime behavior:

- `FlushNativeDirectDrawBatchLocked()` now builds a `NativePassCommand`, then
  submits it through `RenderNativePassCommandLocked()` instead of
  `RenderDirectDebugReplayBatchLocked()`.
- Non-live `SubmitNativeDirectDraw()` now routes a one-draw pass command through
  the same submitter path.
- `fm2_plume_native_direct_draw_live_batch` still owns source bytes per draw, so
  the submitter can build Plume vertex/index buffers after the original decoded
  source pointers have gone out of scope.
- The submitter binds vertex streams using the semantic packet stream slots and
  strides, then draws with the packet's indexed draw call.

Important limitation:

- The first native-layout-only attempt rejected every live pass submit with
  `missing_native_resources`. The captured native state had pass and pipeline
  data, but the stream snapshot was not usable: stream 0 appeared as stride 16,
  stream 1 had `resource=0` and `stride=0`, while the direct replay records had
  the expected `debug_raw32_side12` stream/index uploads.
- Because of that, the current smoke run uses the direct-resource fallback:
  `NativePassCommand` is live, Plume submission is live, but the active layout is
  still `debug_raw32_side12` and the pipeline is still the debug replay shader
  path. This is not yet the final FM2 native world renderer shape used by
  ReOdyssey/Unleashed.

Verification:

- Red tests were added first for `NativePassSubmitPlan`: native-layout accept,
  replay-layout reject, and direct-resource fallback accept.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  712 assertions in 59 test cases after the fallback was added.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`.
- Gameplay smoke with gamepad-A automation, live native batching, side-by-side
  replay window, and wireframe reached gameplay input with
  `Gamepad A tap result: ok=40 failed=0`.
- Fresh logs in
  `FM2/out/build/win-amd64-relwithdebinfo/logs/fm2_161*.log` contained
  800 `FM2_PLUME_NATIVE_LIVE_BATCH_SUBMIT` lines across `fm2_161.log`
  and `fm2_161.1.log`, including `presented=1 size=960x540 draws=16
  skipped=0 topology=2 layout=debug_raw32_side12`.
- The same `fm2_161*.log` set had no
  `FM2_PLUME_NATIVE_DIRECT_DRAW_REJECT`,
  `FM2_PLUME_NATIVE_PASS_SUBMIT_REJECT`, or
  `FM2_PLUME_NATIVE_PASS_REJECT` lines.
- Screenshot QA wrote
  `FM2/out/build/win-amd64-relwithdebinfo/fm2-native-pass-smoke.png` and
  reported `status=ok`, `width=960`, `height=540`, `nonDark=57563`, and
  `avgLum=73.76`.

Next renderer step:

- Fix or replace the native stream/index binding capture so the submitter can
  use `native_position28_side12` resources instead of the direct replay fallback.
  The most relevant FM2 hook candidates remain the pass setup and stream binding
  cluster:
  `FM2_Render_BindPassVertexStreamsWithConstants`,
  `FM2_Render_SetupPassShaderAndVertexStreams`,
  `FM2_Render_BindPassVertexStreamBySlot`, and the instance draw path around
  `FM2_Render_InstanceHybridDrawPath`.

### 2026-06-22 Post-Bind Direct Interface Draw Hook

The first native-pass submitter still captured stale stream/index state because
the `FM2_Render_InstanceHybridDrawPath` hook at `0x82539650` runs before the
direct interface binds the current draw resources. Fresh traces showed
`FM2_PLUME_NATIVE_STATE` and `FM2_PLUME_DIRECT_REPLAY_NATIVE_STATE` stuck at
stream 0 stride 16, stream 1 resource 0/stride 0, and an old index buffer even
while the direct replay records had complete per-record resources.

IDA confirmed the direct interface vtable shape used by this path:

- `+0x64 -> 0x825B3220`: stream descriptor thunk, tail-calls
  `FM2_RenderContext_BindVertexStream`.
- `+0x74 -> 0x825B32C0`: index descriptor thunk, tail-calls
  `FM2_RenderContext_BindIndexBuffer`.
- `+0x80 -> 0x825B3320`: indexed draw thunk, after stream/index binding, then
  branches to the D3D draw packet emitter.

Implemented follow-up:

- Added manifest hook `FM2PlumeTraceDirectIfaceIndexedDraw` at `0x825B3320`.
- The hook reads `draw_iface + 0x14` for the render context, snapshots the
  current native state after the stream/index thunks, and builds a
  `DirectDrawLiveDrawFilter` from primitive type, start index, primitive count,
  current stream 0 resource, and current index resource.
- `MaybeLogPlumeDirectIndexedDrawDecode()` now accepts that filter and a
  snapshot override, scans the direct records for the matching live draw, and
  submits only the matching record.
- The older instance-entry hook still records the direct render context and can
  trace, but it no longer submits native direct draws from stale pre-bind state.
- `BuildNativeDrawResourceKey()` now uses native-state resources only when the
  captured native layout is supported. A complete post-bind snapshot with the
  current `debug_raw32_side12` layout falls back to direct replay resources
  instead of rejecting as `unsupported_native_layout`.

Verification:

- `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  passed.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  729 assertions in 61 test cases.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen`
  regenerated the hook glue.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`; only the existing deprecated CRT warnings in `FM2/src/fm2_hooks.cpp`
  appeared.

Remaining limitation:

- This corrects the capture timing and record selection, but it does not create
  a new FM2-native world layout by itself. If the post-bind live state is
  complete but still describes `debug_raw32_side12`, Plume intentionally uses
  the direct-resource fallback until a supported native FM2 geometry layout is
  identified.

### 2026-06-23 D3D Hook Call-Through Rebuild

The FM2 `d3d_hooks.cpp` replacement pass must preserve the generated FM2
function bodies. `REX_HOOK` overrides the weak generated symbol, so hooks for
render-context state setters and present now call the corresponding generated
`__imp__*` function first, then mirror state into Plume.

Implemented scope:

- Kept the FM2 render-context state mirror on verified call-through boundaries:
  `FM2_RenderContext_SetPixelShaderState`,
  `FM2_RenderContext_SetVertexShaderState`,
  `FM2_RenderContext_BindVertexStream`,
  `FM2_RenderContext_BindIndexBuffer`,
  `FM2_RenderContext_SetBoundSurface`, the verified packed-state helpers now
  named `FM2_RenderContext_SetDepthStencilEnableState`,
  `FM2_RenderContext_SetAlphaBlendEnableBits`,
  `FM2_RenderContext_SetAlphaTestState`,
  `FM2_RenderContext_SetDepthCompareBits`,
  `FM2_RenderContext_SetColorWriteMaskBits`,
  `FM2_RenderContext_SetClipPlane0Enable` through
  `FM2_RenderContext_SetClipPlane3Enable`, and
  `FM2_D3D_TryPresentAndUpdateStatus`.
- Named the remaining active generated helper imports used by this file:
  `FM2_D3DVertexBuffer_Lock` at `0x82369FA0` and
  `FM2_D3DSurface_GetDesc` at `0x8236C0E8`.
- Removed stale ReOdyssey-style replacements for FM2 title wrappers whose
  physical prototypes do not match FM2, including the resource creation,
  lock/unlock, generated draw wrapper, and low-level emit-packet hooks.
- Restored the generated manifest callback definitions in `fm2_hooks.cpp` so
  `FM2PlumeTrace*` hooks link and feed `fm2_native_state` /
  `fm2_native_renderer` counters.
- Added `src/native_renderer/fm2_native_renderer.cpp` and
  `src/native_renderer/fm2_native_state.cpp` to the FM2 target and generated
  the native replay shader DXIL headers used by that renderer.

Verification:

- `cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen`
  passed from `FM2/`, regenerating the final helper names.
- `cmake --build --preset win-amd64-relwithdebinfo --target fm2` passed from
  `FM2/`.
- `rg` found no remaining `sub_82369FA0` or `sub_8236C0E8` references in
  `FM2/generated` or `FM2/src/render/d3d_hooks.cpp`.
- The FM2 sub-build has no local `unit_tests` target, but the root
  `cmake --build --preset win-amd64-relwithdebinfo --target unit_tests`
  target passed.
- `out/win-amd64/RelWithDebInfo/unit_tests.exe "[fm2][plume]"` passed with
  729 assertions in 61 test cases.

Current limitation:

- The current `FM2PlumeTraceDirectIfaceIndexedDraw` shim records the post-bind
  direct-render context (`draw_iface + 0x14`) and draw arguments, but this
  checkout does not expose the older hook-side direct-record decoder described
  above. The next clean product step is to add a small native-renderer API that
  consumes the recorded live draw state and submits through the existing
  `SubmitNativeDirectDraw`/native batch path.

### 2026-06-23 Phase 2 Shader Cache Seed

Phase 2 has started with the smallest offline shader-cache slice:

- Built ReOdyssey's vendored `XenosRecomp` from
  `C:/Users/Tera/Documents/GitHub/ReOdyssey/thirdparty/XenosRecomp` into the
  local scratch build directory `.cache/xenosrecomp-clangcl-build`.
- The first configure attempt picked MSYS GCC and failed in `dxc-bin` because
  architecture detection produced an empty string. The working scratch
  configure used the same `clang-cl` toolchain as this repo and passed
  `-DCMAKE_OSX_ARCHITECTURES=AMD64`, which the `dxc-bin` CMake file reads
  before `CMAKE_SYSTEM_PROCESSOR`.
- Ran the generated tool in directory-cache mode against `FM2/assets`, not the
  single XEX path. `XenosRecomp` only emits `shader_cache.cpp` in directory
  mode; single-file mode emits HLSL for one shader container.
- The output replaced the placeholder `FM2/generated/shader_cache.cpp`.
- Added `scripts/fm2/Update-FM2ShaderCache.ps1` as the tracked regeneration
  wrapper. By default it expects `ReOdyssey` next to this repo; pass
  `-ReOdysseyRoot` if the reference checkout lives elsewhere. The raw commands
  below are the expanded form of that script.

Commands used:

```powershell
cmake -S C:\Users\Tera\Documents\GitHub\ReOdyssey\thirdparty\XenosRecomp `
  -B .cache\xenosrecomp-clangcl-build `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -DCMAKE_OSX_ARCHITECTURES=AMD64 `
  -DCMAKE_C_COMPILER="C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-cl.exe" `
  -DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-cl.exe"

cmake --build .cache\xenosrecomp-clangcl-build --target XenosRecomp --config RelWithDebInfo

.cache\xenosrecomp-clangcl-build\XenosRecomp\XenosRecomp.exe `
  FM2\assets `
  .cache\fm2_shader_cache_candidate.cpp `
  C:\Users\Tera\Documents\GitHub\ReOdyssey\thirdparty\XenosRecomp\XenosRecomp\shader_common.h `
  -j 8

$cacheText = Get-Content -LiteralPath .cache\fm2_shader_cache_candidate.cpp -Raw
$cacheText = $cacheText -replace '("[^"]*") \},', '$1, nullptr },'
Set-Content -LiteralPath .cache\fm2_shader_cache_candidate.cpp `
  -Value $cacheText -NoNewline -Encoding utf8

Copy-Item -LiteralPath .cache\fm2_shader_cache_candidate.cpp `
  -Destination FM2\generated\shader_cache.cpp -Force
```

Preferred repeatable command after the initial tool build exists:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fm2\Update-FM2ShaderCache.ps1 -SkipBuild
```

First pass cache contents:

- `g_shaderCacheEntryCount = 58`.
- `g_dxilCacheDecompressedSize = 235568`.
- `g_spirvCacheDecompressedSize = 20292`.
- Entries are sorted by hash and have normalized `/` source filenames.
- All generated entries currently have `spec_constants_mask = 0`.
- The discovered shaders come from `FM2/assets/Media/tracks/...` files in this
  pass; this does not prove that all live gameplay shaders are covered.

Code/test changes:

- Added `tests/unit/fm2/shader_cache_test.cpp` to assert the generated cache is
  nonempty, sorted by hash, has bounded DXIL/SPIR-V offsets, and uses normalized
  `FM2/assets/...` filenames.
- Added `FM2/generated/shader_cache.cpp` to the root unit test target so the
  test validates the same generated cache compiled into FM2.
- `scripts/fm2/Update-FM2ShaderCache.ps1` post-processes generated cache
  entries to append the FM2/ReOdyssey runtime `guest_shader` back-pointer as
  `nullptr`. ReOdyssey's `XenosRecomp` output omits that local runtime field,
  and adding it in the generated `.cpp` keeps the cache warning-clean without
  relying on ignored `FM2/generated/shader_cache.h` edits.
- `FM2/generated` remains ignored by git; the script is the durable source of
  truth for refreshing the generated cache in a clean workspace.

Verification:

- The new `[fm2][plume][shader-cache]` test failed before replacing the
  placeholder cache with `g_shaderCacheEntryCount == 0`.
- After replacing the cache, the same test passed with 469 assertions in 1 test
  case.
- The immediate next runtime check is to run FM2 with shader-cache miss logging
  enabled and verify whether live menu/gameplay shaders hit this cache or dump
  additional `missed_shaders/*.bin` containers for a second XenosRecomp pass.

Runtime check follow-up, 2026-06-23:

- Black-screen FM2 runs from `FM2/out/build/win-amd64-relwithdebinfo/fm2.exe`
  produced no `missed_shaders` directory under the repo or under
  `C:\Users\Tera\Documents\GitHub`.
- Recent logs `fm2_173.log` through `fm2_175.log` contain `Video::Init`
  Plume device/swapchain setup and `Runtime initialized without graphics system
  (native rendering mode)`, but no `Shader cache MISS`, `CreateShader:`, or
  native draw skip lines.
- This does not prove the 58-entry shader cache is complete. The miss dumper is
  attached to the ReOdyssey-style `CreateVertexShader`/`CreatePixelShader`
  helper path in `FM2/src/render/d3d_resource_hooks.cpp`, while FM2's documented
  shader surface is the title resource path
  `FM2_Render_Load*ShaderResourceById` / `FM2_Render_GetOrCreate*ShaderResourceById`.
- Next evidence step: hook or trace the FM2 shader resource load path and dump
  the resolved payload/microcode range from the loaded resource object, then
  feed confirmed shader containers into the XenosRecomp cache flow.
