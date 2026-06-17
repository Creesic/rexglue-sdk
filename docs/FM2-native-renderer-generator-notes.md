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
    guest buffer base at `resource + 0x18` and byte size at `resource + 0x1C`.
  - `FM2_PLUME_DIRECT_BUFFER`: optional raw byte dump from the D3D resource
    guest buffer, controlled by `fm2_plume_trace_direct_buffer_bytes`.
- Next replay gate:
  - Decode the resource descriptors behind `record + 0x2C`, `record + 0x30`,
    and `direct_render_ctx + 0x5B0`. The immediate goal is to map the captured
    vertex stream and index resource pointers to guest buffer memory, stride,
    format, and index type.
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
      --fm2_plume_trace_direct_buffer_bytes 64
    ```
  - Confirm whether `segment + 0x04` is first index or byte offset by comparing
    it against the decoded index resource layout.
  - Keep original Xenos draw enabled until a Plume debug draw presents from a
    captured packet.
