# FM2 Native Renderer And Generator Notes

Date: 2026-06-16
Branch context: `FM2_WIN_Plume`

This note tracks the idea of using Plume for a title-native renderer path, with
FM2 as the first prototype. It is meant to be updated as we learn durable facts.

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
