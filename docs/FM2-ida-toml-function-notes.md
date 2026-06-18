# FM2 IDA TOML Function Notes

May 22 first-pass IDA naming pass for the functions and hook sites listed in
`FM2/fm2_manifest.toml` / `FM2/fm2_config.toml`.

The TOML contains a mix of true function starts, one-instruction branch thunks,
adjustor labels, and C++ EH cleanup landing pads. True function starts were
renamed in IDA with an `FM2_` prefix. Label-only entries were left as labels and
given IDA comments.

The same names are now burned into `FM2/fm2_manifest.toml` using the ReXGlue
manual function override form:

```toml
0x82000000 = { name = "MyFunction" }
```

`FM2/generated/rexglue.cmake` invokes codegen with `fm2_manifest.toml`, so this
is the config that matters for regenerated FM2 output.

## Renamed Functions

| Address | IDA name | Notes |
| --- | --- | --- |
| `0x824E5A48` | `FM2_Noop` | No-op indirect-call placeholder. |
| `0x8227BD08` | `FM2_StartQueuedTask_VTable8200F160` | Queues deferred task with params vtable `0x8200F160`. |
| `0x822792A0` | `FM2_StartQueuedTask_VTable8200ECF4` | Queues deferred task with params vtable `0x8200ECF4`. |
| `0x8243C058` | `FM2_GetStreamBytesRead` | Simple accessor returning field at `+0x18`. |
| `0x8243C140` | `FM2_BufferedFileReadAsyncAware` | Buffered read path with page-protection and async completion handling. |
| `0x8243C8D0` | `FM2_BufferedFileRead` | Sequential buffered read/refill path. |
| `0x82603BE0` | `FM2_ReleaseOwnedChildObjects` | Releases owned child slots; matches `FM2SkipBadChildSlot` hook context. |
| `0x8242A3C8` | `FM2_CallNestedObjectIfEnabled` | Calls nested vfunc `+0x2C` only when byte `+0x30` is set. |
| `0x8234D348` | `FM2_UpdateListEntriesAndNotifyManager` | Walks intrusive list and performs per-entry notifications. |
| `0x8234D4F8` | `FM2_ClearListEntryBlendWeights` | Walks intrusive list and clears entry float at `+0xB4`. |
| `0x8234D5A8` | `FM2_TriggerMatchingListEntryActions` | Updates state and triggers active matching entries. |
| `0x82375A40` | `FM2_D3D_BeginCommandBufferBatch` | Begins/setup D3D command batch, dirty masks, regions, draw-list state. |
| `0x82375ED0` | `FM2_D3D_EmitDirtyStateAndDrawList` | Emits dirty D3D state and draw-list packets. |
| `0x82376A58` | `FM2_D3D_FinalizeCommandBufferBatch` | Patches packet lengths, flushes/finalizes command buffer status. |
| `0x82540160` | `FM2_SpliceResultObjectsIntoList` | Ref-counted handle/object manipulation and intrusive-list splice. |
| `0x821D3F00` | `FM2_QueueDeferredAudioManagerUpdate` | Queue helper for `CAudioManagerDeferred::CParams2IAudioManagerUpdate`. |
| `0x82276AA0` | `FM2_QueueDeferredVFunc0C_ByteParam` | Queue helper for callback thunk at `0x82276A88`. |
| `0x822786B0` | `FM2_QueueDeferredVFuncD0_TwoU32Params` | Queue helper for callback thunk at `0x82278698`. |
| `0x82279028` | `FM2_QueueDeferredVFunc40_TwoU32Params` | Queue helper for callback thunk at `0x82279010`. |
| `0x8227D0B8` | `FM2_QueueDeferredVFuncD4_ThreeQwordParams` | Queue helper for callback thunk at `0x8227D098`. |
| `0x82551D08` | `FM2_FindAndReplaceDelimitedTextRange` | Nearby helper for TOML branch thunk `0x82551CF8`. |
| `0x823E9BF0` | `FM2_ReleaseObjectMinus8` | Adjusts object pointer by `-8`, then calls vfunc `+0x14`. |
| `0x8260E718` | `FM2_InvokeChildStateReset` | Calls `sub_825FA868(*(this+0x0C), 0)`. |
| `0x82610218` | `FM2_LookupNestedObjectByKey` | Calls `sub_8260FD50(*(a2+4), a1)`. |
| `0x82768510` | `FM2_STL_ConstructArray4` | Repeated construction helper for 4-byte elements. |
| `0x827685EC` | `FM2_STL_CleanupArray4` | EH cleanup helper for 4-byte elements. |
| `0x8276C930` | `FM2_STL_ConstructArray40_A` | Repeated construction helper for 40-byte elements. |
| `0x8276CEC0` | `FM2_STL_CopyConstructRange40_A` | Range copy-construction helper for 40-byte elements. |
| `0x8276CA0C` | `FM2_STL_CleanupArray40_A` | EH cleanup helper for 40-byte elements. |
| `0x8276CFA4` | `FM2_STL_CleanupArray40_B` | EH cleanup helper for 40-byte elements. |
| `0x827A1BF0` | `FM2_STL_CopyConstructRange40_B` | Range copy-construction helper for 40-byte elements. |
| `0x827A1CD4` | `FM2_STL_CleanupArray40_C` | EH cleanup helper for 40-byte elements. |
| `0x827A1A04` | `FM2_STL_CleanupArray40_D` | EH cleanup helper for 40-byte elements. |
| `0x827A7AB8` | `FM2_STL_ConstructArray4176` | Repeated construction helper for 4176-byte elements. |
| `0x827A7E70` | `FM2_STL_CopyConstructRange4176` | Range copy-construction helper for 4176-byte elements. |
| `0x827A7B94` | `FM2_STL_CleanupArray4176_A` | EH cleanup helper for 4176-byte elements. |
| `0x827A7F54` | `FM2_STL_CleanupArray4176_B` | EH cleanup helper for 4176-byte elements. |
| `0x827AE678` | `FM2_STL_CopyConstructRange8` | Range copy-construction helper for 8-byte elements. |
| `0x827AE75C` | `FM2_STL_CleanupArray8` | EH cleanup helper for 8-byte elements. |
| `0x827F9D20` | `FM2_InitListNodeBundle_F9D20` | Initializes list/node bundle around `sub_827FA918`. |
| `0x827FD400` | `FM2_InitListNodeBundle_FD400` | Initializes list/node bundle around `sub_827FCF60`. |
| `0x827A0468` | `FM2_InitListNodeBundle_A0468` | Initializes list/node bundle around `sub_8277F5E0`. |
| `0x82785F88` | `FM2_InitListNodeBundle_85F88` | Initializes list/node bundle around `sub_82786180`. |
| `0x827A0F50` | `FM2_InitListNodeBundle_A0F50` | Initializes list/node bundle around `sub_827A1108`. |

## Commented Label-Only TOML Entries

These are in the TOML but IDA currently treats them as labels, thunks, or EH
landing pads rather than standalone function starts:

`0x821D3EE8`, `0x82276A88`, `0x82278698`, `0x82279010`, `0x8227D098`,
`0x823E9C30`, `0x823F70A0`, `0x823F79F4`, `0x823F7A48`, `0x823F8A24`,
`0x8243F898`, `0x82551CF8`, `0x82573CA8`, `0x82575598`, `0x825B31A0`,
`0x825B3288`, `0x825DCAD8`, `0x825DCAE0`, `0x825E58F0`, `0x8266A7BC`,
`0x8266A7CC`, `0x8266D6F8`, `0x82680D58`, `0x826A8600`, `0x82768AD4`,
`0x8276C388`, `0x8277EBF0`, `0x827860E8`, `0x8279FF14`, `0x827A05C8`,
`0x827A10AC`, `0x827A7034`, `0x827C3D1C`, `0x827CBAB0`, `0x827F9E80`,
`0x827FD560`.

Most of the `0x827x` labels are compiler-generated STL/EH cleanup helpers. The
important game-specific discoveries from this pass are the D3D command-buffer
cluster at `0x82375A40..0x82376A58`, the intrusive-list helpers at
`0x8234D348..0x8234D5A8`, the buffered-read functions at `0x8243C140` and
`0x8243C8D0`, and the deferred callback thunks around `0x821D3EE8` /
`0x82276A88` / `0x82278698` / `0x82279010` / `0x8227D098`.

---

## June 16 Update: FM2 Native Renderer Survey

IDA names and comments were added for the strongest native-renderer hook
candidates. The same names were added to `FM2/fm2_manifest.toml` so regenerated
code keeps stable symbols. Behavior details are still confidence-weighted until
captures/logs confirm them.

| Address | Working name | Evidence |
| --- | --- | --- |
| `0x82518DC0` | `FM2_Render_FramePipeline` | Main frame/pipeline orchestrator. Sets render/camera state, compiles draw buffers, and submits multiple render passes. |
| `0x825181A8` | `FM2_Render_SubmitPassWrapper` | Simple pass-submit wrapper around `0x8252FF00`; stores a pass selector before submit. |
| `0x8252FF00` | `FM2_Render_ExecuteSortedDrawLists` | Iterates sorted renderable arrays, updates object state, and emits cached draw-list command buffers through `FM2_D3D_EmitDirtyStateAndDrawList`. |
| `0x82531DC0` | `FM2_Render_CompileMissingPassBuffers` | Time-budgeted scan for renderables missing cached pass command buffers; calls `0x82531370`. |
| `0x82531370` | `FM2_Render_BuildObjectPassCommandBuffer` | Begins/finalizes a command-buffer batch, emits pass draw work through `0x8250F7C0`, creates texture/fixup records, and clones command buffers. |
| `0x82535C40` | `FM2_Render_DestroySkinnedModelResourceLock` | Destroys a skinned-model resource lock and releases the owned handle if present. |
| `0x825372C8` | `FM2_Render_InitSkinnedModelResourceLock` | Initializes a `TResourceLock<TResourceHandle<CSkinnedModelResourceType, CSkinnedModelResource>, 0>` from a direct-draw record resource handle. |
| `0x82537998` | `FM2_Render_EnsureDirectDrawRecordResources` | Iterates the direct-draw record vector and resolves missing record resource pointers at record `+0x28`, `+0x2C`, and `+0x30`. |
| `0x82509148` | `FM2_Render_SceneSliceEntry` | Prepares a scene/view slice, calls the command-buffer compiler, then executes sorted draw lists. |
| `0x8250D950` | `FM2_Render_ViewTraversal` | Higher-level scene/view traversal; iterates view or light-mode entries and calls `0x82509148`. |
| `0x825380B8` | `FM2_Render_BuildDirectIndexedDrawBuffers` | Uses renderer-interface calls to bind resources and issue indexed primitive draws, then clones generated command buffers. |
| `0x82539650` | `FM2_Render_InstanceHybridDrawPath` | Sorts visible instances, uploads constants/textures, then either emits cached draw lists or directly calls indexed draw interface methods. |
| `0x8253A680` | `FM2_Render_InstancePathWrapper` | Prepares camera/constants for the instance renderer and calls `0x82539650`. |
| `0x8259F6F0` | `FM2_Render_InitPixelShaderResource` | Initializes a pixel shader resource object; sets the resource vtable, default id `-1`, and clears payload/state. |
| `0x8259F750` | `FM2_Render_InitVertexShaderResource` | Initializes a vertex shader resource object; sets the resource vtable, default id `-1`, and clears payload/state. |
| `0x8259FA90` | `FM2_Render_FindPixelShaderResourceById` | Searches the global pixel shader resource vector by integer id stored at resource `+0x44`. |
| `0x8259FBA8` | `FM2_Render_FindVertexShaderResourceById` | Searches the global vertex shader resource vector by integer id stored at resource `+0x44`. |
| `0x825A1608` | `FM2_Render_GetOrCreatePixelShaderResourceById` | Locked pixel shader cache get-or-create helper; allocates a `0x4C` resource, writes id at `+0x44`, marks `+0x31`, and pushes it into the global vector. |
| `0x825A16E0` | `FM2_Render_GetOrCreateVertexShaderResourceById` | Vertex shader parallel to `0x825A1608`; same global-cache get-or-create pattern for vertex shader resources. |
| `0x825A2158` | `FM2_Render_LoadPixelShaderResourceById` | Public wrapper for pixel shader id lookup/create; follows with the resource-manager load/wait call using `0x60000` flags/timeout. |
| `0x825A21C8` | `FM2_Render_LoadVertexShaderResourceById` | Public wrapper for vertex shader id lookup/create; confirmed by caller `0x82537598` passing the resulting handle to a `TResourceLock<TResourceHandle<CVertexShaderResourceType, CVertexShaderResource>, 0>`. |
| `0x825B8920` | `FM2_Render_ScopedBatchBegin` | Switches to a scoped command buffer/context and begins a batch. |
| `0x825B8688` | `FM2_Render_ScopedBatchFinalize` | Finalizes scoped batch, releases current surfaces, and restores previous context. |
| `0x825B8A60` | `FM2_Render_UiOrScreenDrawListSubmit` | Computes a 2D transform, uploads constants, then emits a selected draw-list command buffer. |

Initial interpretation: FM2 already separates rendering into cached
command-buffer construction and later sorted command-buffer execution. A
Plume-native prototype should probably not decode command-buffer bytes first.
The better first experiment is to instrument `0x82531370` and `0x8252FF00`,
build native draw metadata in parallel with FM2's command-buffer construction,
then replay one narrow pass through Plume while leaving the current backend as
fallback.

Ghidra 90 cross-check from the same `default.xex`: use Ghidra as a secondary
source for this render survey, not as the primary caller graph. It agrees on
many simple low-level direct xrefs, but it currently truncates large PPC
functions such as `0x82518DC0` and `0x82539650` at compiler save/prologue helper
calls, and does not recognize `0x8253A680` as a function. Because of that it
misses important direct calls including `0x82518DC0 -> 0x82531DC0` at
`0x825191C8` and `0x8253A680 -> 0x82539650` at `0x8253A964`. It did confirm
data/vtable refs to `0x82518DC0` at `0x820441D0` and `0x82045000`; IDA also sees
those plus `0x8218F858`.

June 18 Ghidra/IDA naming sync: IDA already has the render/direct-draw cluster
above as real functions, and entry comments now describe the observed role of
each function. Ghidra accepted names for the command-buffer batch functions,
direct-draw resource resolver, object/pass compile and execute helpers, frame
pipeline, instance renderer helpers, scoped batch finalizer, and UI/screen draw
submit entry. `0x8253A680` was manually created as
`FM2_Render_InstancePathWrapper`. Ghidra still cannot cleanly model
`0x825380B8` or `0x825B8920` as standalone functions in the current analysis
because they appear to overlap existing function bodies, and the newly created
`0x825B8A60` body is truncated after the vector save prologue. Treat those
Ghidra bodies as navigation labels only; use IDA as the authoritative view for
their control flow.

June 18 shader-resource cache follow-up: Ghidra cursor function
`Function_825A1608` is the pixel shader resource get-or-create helper. IDA and
Ghidra now name the pixel/vertex shader cache pair at `0x8259F6F0`,
`0x8259F750`, `0x8259FA90`, `0x8259FBA8`, `0x825A1608`, `0x825A16E0`,
`0x825A2158`, and `0x825A21C8`. The pixel/vertex split is based on the paired
callers around `0x82537598`: `0x825A21C8` produces a handle later wrapped by
`TResourceLock<TResourceHandle<CVertexShaderResourceType, CVertexShaderResource>, 0>`,
so the parallel `0x825A1608` path is the pixel shader side.

---

## May 30 Update: Missing Indirect Call Target Crash

- Crash observed in `C:\temp\fm2-clean.log`:
  - `[2026-05-30 23:25:13.002] [FATAL] Call to invalid or unregistered function at guest address 0x821D4658`
- Added manifest override so codegen registers the target:
  - `0x821D4658 = { name = "FM2_IndirectTarget_821D4658" }`
- Follow-up crash in the same call cluster:
  - `[2026-05-30 23:57:45.521] [FATAL] ... guest address 0x821D4668`
  - Added `0x821D4668 = { name = "FM2_IndirectTarget_821D4668" }`
- Current status:
  - Treated as an indirect-call target pending IDA classification (true function start vs thunk/label).

## May 24 Update: Audio Signal Gate

### `0x8220A4E8` — `FM2_SignalGate` (TASK 6D root cause)

**What it does:** This is the function that signals the FMOD stream event at
~30 Hz. It is called by the mixer at ~64 Hz (every mixer cycle) but implements
a 2-call divider: it only calls NtSetEvent when an internal counter exceeds 1,
then resets the counter to 0.

**Pseudocode:**
```c
int FM2_SignalGate(int a1) {
    ++dword_829C24C8;                    // increment call counter
    int obj = sub_82218258(a1);          // get FMOD stream object
    bool v2;
    if (*(uint8_t*)(obj + 1054)) {       // field_41E: fast path check
        v2 = (dword_829C24C8 > 1);      // signal only when counter > 1
    } else {
        int obj2 = sub_82218258(obj);    // slow path: double deref
        if (!*(uint8_t*)(obj2 + 1053) || byte_829C24C7 || dword_829C24C8 > 1)
            v2 = 1;                      // signal always
    }
    if (v2) {
        NtSetEvent((void*)dword_829C24C0);  // signal stream event (F800004C)
        dword_829C24C8 = 0;              // reset counter
    }
    dword_829C24CC = mftb();             // timestamp
}
```

**Key globals:**
| Address | Name | Purpose |
|---------|------|---------|
| `0x829C24C0` | `dword_829C24C0` | Stream event handle (F800004C) |
| `0x829C24C8` | `dword_829C24C8` | Call counter (increments, resets to 0 on signal) |
| `0x829C24CC` | `dword_829C24CC` | Last signal timestamp (timebase) |
| `0x829C24C7` | `byte_829C24C7` | Force-signal flag (slow path only) |

**Why 30 Hz:** Counter starts at 0. Call 1: counter=1, check `1>1`=false, no signal.
Call 2: counter=2, check `2>1`=true, NtSetEvent, reset to 0. Repeat → ~32 Hz.
Observed: exactly 30/sec in log.

**Generated code:** `FM2/generated/fm2_recomp.2.cpp` line 3348.

**TASK6D caller evidence:** `caller=8220A578` = return address after the `bl NtSetEvent`
instruction inside `sub_8220A4E8`. The `bl` instruction at `0x8220A56C` calls
`Nt_SetEvent` (which wraps to `sub_8240C4F8`), and the return address is `0x8220A578`.

**Manifest entry (for stable naming):**
```toml
0x8220A4E8 = { name = "FM2_SignalGate" }
```

### `0x82218258` — Called by SignalGate

Called twice by `sub_8220A4E8` (once for initial obj fetch, once for slow path
double-deref). Likely returns the FMOD System or Channel object. Needs
investigation if the signal gate behavior is to be modified.

### `0x82177B60`, `0x822868E8` — SignalGate Callers

These are vtable/function pointer addresses that `sub_8220A4E8` is called through.
IDA shows xrefs from data (not code), confirming indirect dispatch. The mixer
stores a function pointer at one of these addresses and calls it each cycle.

## May 31 Update: `0x82369340` Profiling Hotspot

- Renamed in Ghidra: `Function_82369340` -> `FM2_ProducerProgressGuard_82369340`.
- Behavior from decompilation:
  - Calls a thread/context accessor (`FUN_824131ac`) and reads a producer object.
  - Executes a short fixed spin/delay sequence (decompiler shows a tiny countdown loop; this matches prior `db16cyc` observations in the same helper).
  - If producer flag bit `+0x2A3D & 4` is clear, samples current tick and producer progress fields.
  - If elapsed since last-progress tick is under `5000`, returns `1` (keep waiting).
  - Otherwise calls `Function_82373E38` (recovery/timeout path) and returns `0`.
- Related callee `0x82373E38` sets producer state bits and rewinds a queue/index field before continuing, consistent with a progress-timeout recovery path.
- Manifest mirror added:
  - `0x82369340 = { name = "FM2_ProducerProgressGuard_82369340" }`

## May 31 Update: `0x82697F08` APU Mix Core

- Named in IDA as `FM2_ApuMixRenderCore_82697F08`.
- Ghidra label/comments added at:
  - entry `0x82697F08`
  - exit path A `0x826983A0`
  - exit path B `0x826983C0`
  - callsites `0x826A7180` and `0x826A71D0`
- Manifest mirror added:
  - `0x82697F08 = { name = "FM2_ApuMixRenderCore_82697F08" }`
- Runtime instrumentation hooks added for timing/call counts:
  - entry `0x82697F08` -> `FM2ApuMixRenderEnter82697F08`
  - exits `0x826983A0` / `0x826983C0` -> `FM2ApuMixRenderExitA/B...`

## June 1 Update: FMOD/XMA LR Hotspots + New Trace Labels

From latest `C:\temp\fm2-clean.log`:

- dominant LR pair: `0x82693910` / `0x82693998` (codec-read disable/enable returns)
- recurring XMA reinit LRs: `0x82692B24`, `0x82692C8C`, `0x82692CA4`
- additional caller-chain hotspots around `Function_8268C670` read path.

Added manifest labels for the FMOD/XMA chain:

- `0x8268C670` -> `FM2_FmodStreamRead_8268C670`
- `0x82692AF0` -> `FM2_FmodXmaContextReinit_82692AF0`
- `0x82692B24` -> `FM2_FmodXmaContextReinit_PostDisable_82692B24`
- `0x82692C8C` -> `FM2_FmodXmaContextReinit_PostInit_82692C8C`
- `0x82692CA4` -> `FM2_FmodXmaContextReinit_PostEnable_82692CA4`
- `0x82693910` -> `FM2_FmodCodecRead_PostDisable_82693910`
- `0x82693998` -> `FM2_FmodCodecRead_PostEnable_82693998`
- `0x826939A8` -> `FM2_FmodCodecSeek_826939A8`
- `0x82693AC0` -> `FM2_FmodCodecSeek_CallRead_82693AC0`
- `0x826776E0` -> `FM2_FmodStreamReadDispatch_826776E0`
- `0x826778C4` -> `FM2_FmodStreamReadDispatch_CallRead_826778C4`
- `0x826949B0` -> `FM2_FmodCodecReadChunk_CallRead_826949B0`
- `0x82695480` -> `FM2_FmodCodecReadConvert_82695480`
- `0x8269550C` -> `FM2_FmodCodecReadConvert_CallRead_8269550C`
- `0x826A3E80` -> `FM2_FmodCodecReadPaged_826A3E80`
- `0x826A40A8` -> `FM2_FmodCodecReadPaged_CallRead_826A40A8`

Also labeled new non-audio LR hotspots seen in the same log burst so future traces
resolve immediately (allocator/string/path sites), including:

- `0x821D15B0`, `0x821D0E74`, `0x821D0448`
- `0x821D2550`, `0x821D266C`, `0x821D2704`, `0x821D2878`
- `0x82430CF0`, `0x824318F0`
- `0x825CF560`, `0x8259F3A0`, `0x825345A8`, `0x822097C8`
- `0x821EA544`, `0x825ADE8C`

## June 1 Update: APU Mix Gain/Matrix Chain (Ghidra)

Deep pass in Ghidra around `0x82697F08` confirms the split between:

- voice gain/matrix setup
- per-buffer accumulation into output channels

Newly labeled addresses:

- `0x8269DB60` -> `FM2_ApuMixCoeffSetIdentity_8269DB60`
- `0x8269DD80` -> `FM2_ApuMixCoeffUpdateDelta_8269DD80`
- `0x8269DF48` -> `FM2_ApuMixCoeffIsIdentity_8269DF48`
- `0x8269DFE0` -> `FM2_ApuMixSetBaseGain_8269DFE0`
- `0x8269E000` -> `FM2_ApuMixBuildPanMatrix_8269E000`
- `0x8269E6B0` -> `FM2_ApuMixSetOutputMatrix_8269E6B0`
- `0x8269E8F0` -> `FM2_ApuMixAccumulateVoice_8269E8F0`
- `0x826A62A0` -> `FM2_AudioVoiceSetVolumeQuantized_826A62A0`
- `0x826A75E8` -> `FM2_AudioSourceSetVolume_826A75E8`
- `0x826A8628` -> `FM2_AudioVoiceApplyOutputMatrix_826A8628`

Key behavior observed:

- `FM2_ApuMixRenderCore_82697F08` calls `FM2_ApuMixBuildPanMatrix_8269E000` then `FM2_ApuMixAccumulateVoice_8269E8F0` for active voices.
- `FM2_ApuMixSetBaseGain_8269DFE0` writes the scalar gain field at `+0x5C` (and a sync field at `+0x60`), which feeds matrix generation.
- `FM2_ApuMixCoeffUpdateDelta_8269DD80` computes smoothing deltas using `+0x8C` and sets a short ramp window (`+0x88 = 0x40`) when change is above threshold.
- `FM2_ApuMixAccumulateVoice_8269E8F0` applies source samples through per-channel coefficient matrices (`+0x2C..+0x40`) into the mix buffer.
- Callers near `0x826A85E4` multiply requested volume by extra class/bus scalars (`obj+0x54->0xD4` and `obj+0x54->0x88->0x40`) before forwarding to set-base-gain path.

Implication for overlay interpretation:

- If the overlay tracks pre-mix source level or decoded PCM magnitude, it can stay "hot" while audible output is quieter; attenuation may be happening later via matrix coefficients and/or bus scalars in this chain.
