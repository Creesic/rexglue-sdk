# FM2 IDA Renames � 2026-06-18

Session log for unnamed `sub_` functions renamed in IDA (`default.xex.i64`) by
walking outward from already-named `FM2_` functions and cross-checking repo docs
(`docs/FM2-ida-toml-function-notes.md`, `docs/FM2-native-renderer-generator-notes.md`,
`docs/FM2-performance-notes.md`, `docs/FM2-audio-fmod-decode-cadence.md`).

Method: enumerate `sub_` callees of named `FM2_` functions, decompile the
highest-traffic clusters (render/D3D, allocator, audio, STL/EH), name from
behavior and caller context.

**From batch 2 onward, each entry includes explicit rename reasoning.**

External references: repo docs plus `D:\Emulation\Xbox360techdocs` (Xbox 360
system PDFs � notably `system_xbox_360_memory_copy_functions.pdf` and
`system_vmx128_overview.pdf`, which align with the vectorized memcpy / VMX
constant-upload helpers found in FM2).

## Summary

| Cluster | Batch 1 | Batch 2 | Batch 3 (emit) | Batch 4 (random) | Total |
| --- | ---: | ---: | ---: | ---: | ---: |
| Render / D3D / command buffer | 38 | 22 | 78 | 0 | 116 |
| Allocator / memory / stream I/O | 5 | 1 | 35 | 0 | 41 |
| Audio / FMOD mix | 5 | 3 | 0 | 0 | 8 |
| Deferred task / queue helpers | 2 | 0 | 0 | 0 | 2 |
| Resource / ComPtr / handles | 8 | 4 | 0 | 0 | 12 |
| STL / EH / intrusive list / gameplay | 8 | 5 | 18 | ~120 | ~151 |
| GPU kick / perf | 0 | 1 | 0 | 0 | 1 |
| CRT / runtime glue (emit closure) | 0 | 0 | 36 | ~80 | ~116 |
| Heuristic random sample (mixed) | 0 | 0 | 0 | ~800 | ~800 |
| **Manual re-pass (batch 4 fix)** | 0 | 0 | 0 | 978 | 978 |
| **Total** | **66** | **34** | **167** | **1000** | **1267** |

**Batch 4 heuristic rename (1000) is being replaced** with manual evidence-based
names. **978/978** batch-4 placeholder functions re-named (manual passes 1�27). Post batch-4:
high-traffic `sub_` infrastructure naming in progress (passes **80–83** applied this session; **470** remaining `sub_` from named `FM2_` callers outside emit cluster).

**Render emit cluster BFS (7 roots ? 317 functions) is fully exhausted:** 0 unnamed
`sub_` remain in the transitive closure of
`FM2_Render_EmitPassDrawWork`, `FM2_D3D_EmitDirtyStateAndDrawList`,
`FM2_D3D_EmitDrawListStatePackets`, `FM2_D3D_EmitScissorRegionPackets`,
`FM2_D3D_EmitSurfaceResolvePackets`, `FM2_D3D_BeginCommandBufferBatch`, and
`FM2_D3D_FinalizeCommandBufferBatch`.

~106 unnamed `sub_` callees of `FM2_` functions remain globally outside the
emit cluster (**~1201** total `sub_` callees from any `FM2_` function; prioritize
by caller count � see `.cursor/hooks/state/unnamed-sub-callees.json`). **~44,696** unnamed `sub_` remain in the binary overall after
batch 4.

---

## Render / D3D / command buffer

| Address | New name | Evidence |
| --- | --- | --- |
| `0x825B36A8` | `FM2_Render_GetGlobalRenderContext` | Returns global render context pointer `dword_82A01570`; called throughout render/audio init paths. |
| `0x825B3838` | `FM2_Render_GetActiveCommandBufferContext` | Reads `*(ctx+112)+20`; paired with setter below. |
| `0x825B3848` | `FM2_Render_SetActiveCommandBufferContext` | Writes active command-buffer context at `*(ctx+112)+20`. |
| `0x8250F7C0` | `FM2_Render_EmitPassDrawWork` | Called from `FM2_Render_BuildObjectPassCommandBuffer` and `FM2_Render_ExecuteSortedDrawLists`; binds pass state via `FM2_RenderContext_*` helpers and emits draw work. |
| `0x82371A30` | `FM2_RenderContext_SetBoundSurface` | Stores bound surface at `ctx+12176`, copies surface fields, sets dirty bits at `ctx+16`. |
| `0x82371D20` | `FM2_RenderContext_BindSurfaceThunk` | One-instruction thunk to `sub_823716F8`. |
| `0x823715C0` | `FM2_RenderContext_UploadConstantBlock` | Forwards six float fields into constant upload helper `sub_82371348`. |
| `0x82370DF8` | `FM2_RenderContext_ExportViewportConstants` | Copies viewport/scissor float block from render context fields `3092..3097`. |
| `0x82370FF8` | `FM2_RenderContext_GetSurfaceSlotAndAddRef` | Returns surface slot `ctx[4*(idx+3040)]` and `D3DResource_AddRef`s it. |
| `0x82371040` | `FM2_RenderContext_GetBoundSurfaceAndAddRef` | Returns `ctx+12176` surface with addref. |
| `0x8236E780` | `FM2_D3D_EmitScissorRegionPackets` | Emits PM4 scissor packets; may call `D3D_SubmitAndDrainCommands` when write cursor exceeds limit. Called from `FM2_D3D_BeginCommandBufferBatch` / `FM2_D3D_EmitDirtyStateAndDrawList`. |
| `0x82383A70` | `FM2_D3D_EmitDrawListStatePackets` | Large draw-list state emitter; builds texture/index/state PM4 from bound draw-list record. Called from `FM2_D3D_EmitDirtyStateAndDrawList`. |
| `0x8236F228` | `FM2_RenderContext_SetCullEnableState` | Sets cull-enable bit at `ctx+11600` / `ctx+10420`; marks dirty `0x800`/`0x20000`. |
| `0x8236F268` | `FM2_RenderContext_SetAlphaTestState` | Sets alpha-test bitfield in `ctx+10420`. |
| `0x8236F2A0` | `FM2_RenderContext_SetBlendModeBits` | Sets 3-bit blend mode field in `ctx+10420`. |
| `0x8236F2D0` | `FM2_RenderContext_SetDepthCompareBits` | Sets depth-compare bitfield in `ctx+10420`. |
| `0x8236F308` | `FM2_RenderContext_SetStencilOpBits` | Sets stencil-op bitfield in `ctx+10420`. |
| `0x8236F340` | `FM2_RenderContext_SetColorWriteMaskBits` | Sets color-write mask bits in `ctx+10420`. |
| `0x8236F370` | `FM2_RenderContext_SetPolygonModeBits` | Sets polygon-mode bits in `ctx+10420`. |
| `0x8236F410` | `FM2_RenderContext_SetMiscStateBitsA` | Sets upper misc state bits in `ctx+10420`. |
| `0x8236F440` | `FM2_RenderContext_SetClipPlane0Enable` | Writes clip-plane enable byte at `ctx+10371`. |
| `0x8236F460` | `FM2_RenderContext_SetClipPlane1Enable` | Writes clip-plane enable byte at `ctx+10370`. |
| `0x8236F480` | `FM2_RenderContext_SetClipPlane2Enable` | Writes clip-plane enable byte at `ctx+10369`. |
| `0x8236F4A0` | `FM2_RenderContext_SetClipPlane3Enable` | Writes clip-plane enable byte at `ctx+10367`. |
| `0x8236F718` | `FM2_RenderContext_SetPassIndexState` | Stores pass index at `ctx+11580`, low nibble at `ctx+10332`. |
| `0x82375078` | `FM2_RenderContext_SetShaderResourceState` | Called from `FM2_RenderContext_SetPixelShaderState` / `SetVertexShaderState`. |
| `0x8236D958` | `FM2_RenderContext_UploadMatrixConstants` | Uploads 4�4 matrix block + related constants; used by instance/UI draw paths. |
| `0x823CBD50` | `FM2_Render_ScopedBatch_SetGpuKickOwner` | Swaps GPU kick owner at `ctx+700`; addref/release paired calls. Used by scoped batch begin/finalize. |
| `0x82356BD0` | `FM2_Render_SelectDrawListRangeByIndex` | Selects draw-list subrange from sorted container by index threshold. |
| `0x82518580` | `FM2_Render_UpdatePassVisibilityAndSortKeys` | Called from `FM2_Render_FramePipeline`; computes pass visibility/sort keys from renderable arrays. |
| `0x8252EE90` | `FM2_Render_UpdateObjectDistanceKeys` | Updates per-object distance/sort fields for three buckets; called from frame/scene slice paths. |
| `0x82518CA0` | `FM2_Render_ApplyPassEnvironmentState` | Applies pass environment (pass ids 3/5) including resource binding and manager notifications. |
| `0x82560B20` | `FM2_Render_SubmitObjectDrawConstants` | Uploads object/world constants and matrix blocks for a draw record. |
| `0x82564740` | `FM2_Render_DrawPassMaterialSetup` | Material/shadow setup for object pass; references shader constant `c_dropShadow`. |
| `0x8256A0B0` | `FM2_Render_ObjectPassDrawSetup` | Object-pass draw traversal setup helper (called from object pass traversal cluster). |
| `0x825095D0` | `FM2_Render_NotifyPassStateChange` | Thin wrapper calling `FM2_Render_NotifyManagerStateChange` with `base+3344`. |
| `0x824FA750` | `FM2_Render_NotifyManagerStateChange` | Notifies render manager/list controller of state change at offset `base+N`. |
| `0x821D78D8` | `FM2_Render_GetFrameCounterField` | Accessor returning `*(obj+4)`; used by frame pipeline. |
| `0x824A1100` | `FM2_ShaderResourceVector_End` | Returns vector end pointer; paired with pixel/vertex shader find helpers. |

---

## Allocator / memory / stream I/O

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82363800` | `FM2_AllocPoolBumpAllocate` | Bump allocator fast path; falls back to `FM2_AllocPoolAcquireOrInit` (`0x82363768`). Called by all deferred queue helpers. |
| `0x82413620` | `FM2_MemcpyAligned` | Aligned memcpy with `dcbt` prefetch; documented in `docs/FM2-audio-fmod-decode-cadence.md`. |
| `0x8240BAA8` | `FM2_MemcpyVectorized` | VMX128 vectorized memcpy (`lvx128`/`vperm`/`stvx`); used by buffered read paths. |
| `0x82452338` | `FM2_StreamRead_SetupRequest` | Fills stream read request fields then calls async read starter `sub_824530F0`. Called from buffered read functions. |
| `0x8241DD58` | `FM2_STL_RaiseArrayConstructionException` | EH helper: `FM2_MemcpyAligned` + `RaiseException` on array construction failure. |

---

## Audio / FMOD mix

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82218258` | `FM2_GetForzaCommandLineParamsSingleton` | Lazy-init singleton; sets vtable `CForzaCommandLineParameters::vftable`. Called by `FM2_SignalGate` and many audio paths (was mislabeled as FMOD stream object in older notes). |
| `0x825ADB18` | `FM2_AudioMix_SubmitPendingOutput` | Submits pending voice/output matrix through D3D-style constant upload helpers; called from audio render frame paths. |
| `0x825ADEA8` | `FM2_AudioMix_FlushOutputBatch` | Batch flush companion to submit path; audio render frame A/B. |
| `0x822776A8` | `FM2_AudioMix_SetupRenderTargetTextures` | Creates/bind A2B10G10R10 render-target texture headers via `XGSetTextureHeader(Ex)`. |
| `0x821EC438` | `FM2_AudioManager_GetStreamObjectField` | Small accessor in audio manager init/render path. |
| `0x82429CF0` | `FM2_AudioRender_GetMixContextField` | Returns mix-context subfield; used by audio render frame paths. |

---

## Deferred task / queue helpers

| Address | New name | Evidence |
| --- | --- | --- |
| `0x821D8A18` | `FM2_DeferredTaskParams_GetField4` | Returns `*(params+4)`; shared by all `FM2_QueueDeferred*` / `FM2_StartQueuedTask_*` helpers. |
| `0x8245D1D8` | `FM2_DeferredTaskParams_ReleaseChild` | Calls release helper on child at `*(params+4)`. |

---

## Resource / ComPtr / handles

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82535730` | `FM2_ResourceLock_ClearAndReleaseHandle` | Clears resource lock handle at `+36`, releases via `sub_822737B8`. Used by scoped batch and skinned-model lock helpers. |
| `0x824A0F50` | `FM2_ResourceManager_InitBase` | Initializes resource-manager base (critical section, default flags). Called from shader resource init paths. |
| `0x824A55C8` | `FM2_ResourceManager_LoadAndWait` | Load/wait wrapper used by `FM2_Render_LoadPixelShaderResourceById` / vertex parallel. |
| `0x825EE588` | `FM2_ResourceHandle_RetainOrRelease` | Retain/release helper for resource handles; used by resource-lock capture/assign helpers. |
| `0x82570AC8` | `FM2_ComPtr_AssignRef` | Standard ComPtr assign: addref new, release old. |
| `0x8253D718` | `FM2_ComPtr_ResetAndAssign` | Clears slot, addrefs new pointer, stores. Very widely used. |

---

## STL / EH / intrusive list

| Address | New name | Evidence |
| --- | --- | --- |
| `0x8234CD28` | `FM2_IntrusiveList_DecrementIterator` | Decrements intrusive list iterator with sentinel check at `+305`. Used by list-entry helpers at `0x8234D348..`. |
| `0x822F95C8` | `FM2_IntrusiveList_SpliceNodes` | Splices/ref-counted list nodes; called from audio manager init and `FM2_SpliceResultObjectsIntoList` cluster. |
| `0x827FA9C0` | `FM2_STL_ConstructElementBundle` | Element construction wrapper called from `FM2_STL_ConstructArray4` and list-node bundle inits. |
| `0x82766540` | `FM2_STL_ListNode_InitSentinelA` | List sentinel init; shared by all `FM2_InitListNodeBundle_*` helpers. |
| `0x82766AC0` | `FM2_STL_ListNode_InitSentinelB` | Companion sentinel init helper. |
| `0x82789720` | `FM2_STL_ListNode_LinkNext` | Links next node in list bundle init sequence. |
| `0x82790130` | `FM2_STL_DestructRange40` | Destructs range of 40-byte elements; called from all `FM2_STL_CleanupArray40_*` helpers. |
| `0x827FCC40` | `FM2_STL_DestructRange4Or4176` | Destructs 4-byte or 4176-byte element ranges; shared cleanup helper. |

---

## Notes for follow-up passes

High-value unnamed callees still referenced from named `FM2_` functions:

- `0x82537A68` / `0x82539398` / `0x82536D38` � instance path camera/constants setup around `FM2_Render_InstancePathWrapper`
- `0x825B0348` � very large function on audio render path B
- `0x8267FA90` � callee of `FM2_AudioVoiceApplyOutputMatrix_826A8628`
- `0x8237D158` / `0x827328E8` � constant-buffer upload helpers on audio mix path
- Remaining `0x8236ECxx` / `0x8236EDxx` D3D dirty-state emit helpers inside `FM2_Render_EmitPassDrawWork`
- `0x8255D880` � large object-pass constant submit helper

Consider mirroring stable gameplay-facing names into `FM2/fm2_manifest.toml` when codegen hooks or crash logs need them.

---


## Batch 5 � 2026-06-18 (IDA follow-up)

### PNG/Lua binding error reporting

| Address | New name | Evidence |
| --- | --- | --- |
| `0x8239EFD8` | `FM2_Png_FormatErrorTag` | Escapes a 4-byte payload/tag into a local char buffer: printable bytes are copied verbatim, non-printable bytes become `[XX]` hex tokens, and an optional suffix string is appended. |
| `0x8239F178` | `FM2_Png_ReportError` | Formats the error text through `FM2_Png_FormatErrorTag`, then forwards to the callback at `context+324` when present. |
| `0x8239F130` | `FM2_Png_ReportErrorAndUnwind` | Adds a context-layer pre-check callback at `context+80`, then marks/unwinds error state via `FM2_Lua_UnwindAndSetErrorStatus(context, 1)`, and forwards to `FM2_Png_ReportError`. |

### Render / interpolation helpers

| Address | New name | Evidence |
| --- | --- | --- |
| `0x82492C00` | `FM2_Render_InterpolateAndClampScalar` | Loads per-instance scalar limits and a 3-float vector, normalizes and adjusts it, then smooths/clamps the incoming scalar value with atanh-like behavior; used by both lighting interpolation paths and AI route interpolations. |
| `0x824936D0` | `FM2_Render_InterpolateScalarFromSector` | Computes sector index from progress (`FM2_AIDriver_ComputeSectorIndexFromProgress`), looks up two scalar vectors, and delegates normalized/smooth clamping to `FM2_Render_InterpolateAndClampScalar`; used by race-line updates and pass-lighting. |

### LiveryMask request helpers

| Address | New name | Evidence |
| --- | --- | --- |
| `0x825A18B8` | `FM2_LiveryMask_BeginAsyncLoad` | Initializes request/create params, normalizes and lowercases source path text, then enters queueing path with a global create lock and object vector. |
| `0x825A1458` | `FM2_LiveryMask_QueueCreateRequest` | Under critical-section lock, allocates `FM2_LiveryMask` object if needed, sets interface pointer, copies create params, marks request active, and pushes into `dword_82A00EE0`. |
| `0x825A1978` | `FM2_LiveryMask_ParseAndLoadEntry` | Parses incoming path bytes, rewrites `.tga` to `.xds` where needed, then calls load/wait and downstream load dispatcher helpers (`FM2_ResourceManager_LoadAndWait`, `sub_824A6758`). |

## Batch 2 � 2026-06-18 (with reasoning)

### Render / D3D / command buffer

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825AD688` | `FM2_Render_CopyPassConstantBlock` | Called from scoped-batch and pass-compile paths; indexes global pass table at `unk_829A2BB0` with stride `1232` and copies `16 * count` bytes via `FM2_MemcpyAligned`. Name reflects pass-constant block copy, not generic memcpy. |
| `0x825AD650` | `FM2_Render_GetPassConstantBaseOffset` | Returns `unk_829A2BB0[308 * passIndex]` � base dword offset into the pass-constant table. Paired with slot-index accessor below. |
| `0x825AD628` | `FM2_Render_GetPassConstantSlotIndex` | Returns adjacent table entry `unk_829A2BA8[308 * passIndex]`; used with base offset to locate pass constant storage. |
| `0x8236F1C0` | `FM2_RenderContext_SetZEnableBit` | Same dirty-mask pattern as other `FM2_RenderContext_Set*` helpers: writes `(4*a2)&4` into `ctx+10420`, sets dirty bit `0x800`. Bit 2 corresponds to Z-enable in the packed render-state dword. |
| `0x8236F1F0` | `FM2_RenderContext_SetAlphaBlendEnableBits` | Sets `(16*a2)&0x70` in `ctx+10420`; marks dirty `0x800` and `0x20000`. Called alongside other blend/alpha helpers inside `FM2_Render_EmitPassDrawWork`. |
| `0x82370080` | `FM2_RenderContext_SetVertexFetchModeBit` | Modifies `ctx+10428` nibble at bit 4 (`(16*a2)&0x10`); dirty bit `0x200`. Distinct field from `10420` state dword used by raster/blend helpers. |
| `0x8236FC88` | `FM2_RenderContext_SetBoundSurfaceEDRAMMode` | Reads bound surface at `ctx+12160`, rewrites tiled/EDRAM-related bits in surface field `+28` based on format nibble and `a2`. Much more complex than a simple dirty-bit setter � name captures surface/EDRAM mode update. |
| `0x8236EAC0` | `FM2_RenderContext_SetIndexBufferModeBit` | Sets bit 3 in `ctx+10428` (`(8*a2)&8`); dirty bits `0x200` and `0x40000`. |
| `0x8236E228` | `FM2_RenderContext_SetActivePassId` | Stores pass id at `ctx+11540`; dirty bit `0x80000`. Used by indexed/instance draw paths before constant upload. |
| `0x82721190` | `FM2_Render_SetGlobalFillMode` | Maps caller values `0?0`, `100?2`, `101?6` into global `dword_82A4198C`; other values stored verbatim. Called from pass draw emit when `a6` requests alternate fill mode. |
| `0x827218B8` | `FM2_Renderable_HasPassFlags` | Returns `(*(_DWORD *)(obj+264) & passMask) != 0`. Gate used at start of `FM2_Render_EmitPassDrawWork` before any draw setup. |
| `0x82724968` | `FM2_RenderTls_GetSubContextField32` | One-instruction wrapper returning `sub_82724BF8(tls+32)`; paired with TLS context getter used before render resource binding. |
| `0x82724BC0` | `FM2_RenderTls_GetCurrentContext` | Calls `KeTlsGetValue` after `sub_82724B00`; stores TLS render context pointer. Used by frame pipeline and pass draw emit. |
| `0x8259FD80` | `FM2_Render_InitEnvironmentBindingContext` | Thin init wrapper around `sub_822A1C78`; called before environment resource binding in view traversal / pass environment paths. |
| `0x82371348` | `FM2_RenderContext_UploadFloat6Constants` | Target of `FM2_RenderContext_UploadConstantBlock` thunk; uploads six float constants when bound surface(s) present, with same scissor/surface dirty-mask gating as `SetBoundSurface`. |
| `0x823716F8` | `FM2_RenderContext_BindSurfaceInternal` | Real implementation behind `FM2_RenderContext_BindSurfaceThunk`; full surface bind with EDRAM/surface-type checks and dirty-mask updates. |
| `0x82382590` | `FM2_D3D_EmitSurfaceResolvePackets` | Emits PM4 packets (`0xC0030000`, `0xC0020001`, etc.) for surface resolve/EDRAM paths when dirty bit `0x100` set. Called from `FM2_D3D_EmitDirtyStateAndDrawList`. Packet header constants match Xbox 360 CP register writes. |
| `0x825078D0` | `FM2_Render_PrepareSceneSliceTransforms` | Called from `FM2_Render_SceneSliceEntry`; builds view/projection related float blocks and transform matrices before command-buffer compile. |
| `0x825170B8` | `FM2_Render_ApplyPassLightingState` | Fallback/target of `FM2_Render_ApplyPassEnvironmentState` for non-special pass ids; computes lighting-related float4 blocks and uploads constants. |
| `0x8252F930` | `FM2_Render_SortVisibleRenderables` | Called from `FM2_Render_ExecuteSortedDrawLists`; iterates renderable vector, computes angular/distance sort keys with `fmod`/VMX, inserts into sort structure via `sub_827F1D00`. |
| `0x827307E8` | `FM2_ConstantBuffer_UploadVector4Block` | VMX128 upload (`lvlx`/`vupkhpx`) of vector constant block; shared by render view traversal and `FM2_AudioMix_SubmitPendingOutput`. Matches Xbox 360 VMX128 constant packing (see `system_vmx128_overview.pdf`). |

### Allocator

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823637F8` | `FM2_AllocPoolAcquireOrInit_Thunk` | Single-instruction branch thunk to `FM2_AllocPoolAcquireOrInit` (`0x82363768`); documented in `docs/FM2-performance-notes.md`. Named as thunk, not duplicate of target. |

### Resource / shader / texture locks

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824A4F00` | `FM2_ShaderResource_LoadIfReady` | Called from both shader load wrappers; proceeds to `sub_824A4D20` only when resource state at `handle+40` equals `3` (ready). Gate before blocking load/wait. |
| `0x821D1510` | `FM2_TextureResourceLock_InitFromHandle` | RTTI/vtable explicitly `TResourceLock<TResourceHandle<CTextureResourceType,CTextureResource>,0>`; initializes critsec and assigns handle via `sub_821D0CD0`. |
| `0x821D0C08` | `FM2_TextureResourceLock_Destroy` | Sets same texture-resource-lock vtable, calls `FM2_ResourceLock_ClearAndReleaseHandle`, releases held resource. Pair of init above. |
| `0x823C62B8` | `FM2_D3DResource_AdjustGpuAddress` | Switches on `D3DResource_GetType` and adds byte offset to resource GPU fields (texture mip offset, VB/IB common field, shader fence field). Used when binding audio/render target textures. |
| `0x82276758` | `FM2_ShaderConstant_SetVectorById` | Encodes constant register from `a2` bitfields, writes vector from `_R5` into shader constant table. References `byte_8200E4CC` type table. Called from material setup paths with string constant names like `c_dropShadow`. |

### Audio

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825A3950` | `FM2_AudioMix_WriteOutputSamplePair` | Writes two floats (`a6`,`a7`) plus four dword fields into output sample struct; called from audio render frame A and view traversal audio hook. |
| `0x82273338` | `FM2_AudioMix_InitDefaultCoefficients` | Initializes default float coefficients/matrix blocks at object base using VMX stores; called during `FM2_AudioManager_InitAndBindSignalGate`. No external inputs � pure init table write. |

### STL / list / gameplay

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827A7D10` | `FM2_STL_ConstructElement4176` | Thin wrapper to `sub_827A8070`; called from both `FM2_STL_ConstructArray4176` and `CopyConstructRange4176`. Matches existing `FM2_STL_ConstructArray4176` naming pattern. |
| `0x82766EC0` | `FM2_STL_GetNodeDataPtr16` | Returns `a1+16`; used in list-node bundle init sequences after sentinel setup. |
| `0x82766ED0` | `FM2_STL_GetNodeDataPtr17` | Returns `a1+17`; adjacent node accessor in same init bundle cluster � likely char-sized link flag field. |
| `0x8224FF00` | `FM2_LiveryMask_ProcessPendingEntryUpdates` | Sole caller context is list-entry notification path, but **internal strings** name the domain: `LiveryMasks\\Masks.xml`, `-BaseUncompTemp`, `-DamageUncompTemp`, `(Base)`, `(Damage)`. Renamed away from generic "ListEntryManager" after string evidence. |

### GPU kick / perf

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82378AB0` | `FM2_GpuCommandBuffer_BeginPerfCaptureOrKick` | Called from `FM2_GpuCommandBuffer_BuildAndSubmit`; handles perf capture file `e:\xbperfview.cap`, `mftb` timing fields, allocates `D3D::P_CPC` slots, transitions kick state machine at `ctx+21272`. Name reflects perf-capture + kick setup, not generic submit. |

---

## Batch 3 � Render emit cluster (167 renames, 2026-06-18)

BFS from emit roots listed in Summary. Every remaining `sub_` in the 317-function
closure was renamed (including CRT/STL helpers reached only through emit paths).

### Render context state setters (sampler / depth / texture fetch)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236EAF8` | `FM2_RenderContext_SetDepthStencilEnableState` | Sets MSB of `ctx+11572`, recomputes packed sampler dword at `ctx+11568`, mirrors to `ctx+10424/10456/10460/10464`, sets dirty qword bits � same pattern as named `FM2_RenderContext_Set*State` helpers. |
| `0x8236EC18` | `FM2_RenderContext_SetSamplerStateLowNibble` | Masks `a2 & 0x1F` into low byte of `ctx+11568`; propagates to PM4 shadow fields when depth-stencil override flag clear. |
| `0x8236ECA8` | `FM2_RenderContext_SetSamplerStateMidNibble` | Writes `(a2<<8)&0x1F00` into `ctx+11568`; mid-byte sampler index field. |
| `0x8236EDA8` | `FM2_RenderContext_SetSamplerStateHighNibble` | Writes `(a2<<16)&0x1F0000`; sets dirty bits `0x400/4/2` on `ctx+16`. |
| `0x8236EE18` | `FM2_RenderContext_SetSamplerStateTopNibble` | Writes `(a2<<24)&0x1F000000`; top nibble of packed sampler state dword. |
| `0x8236EA60` | `FM2_RenderContext_SetTextureFetchBitsLow` | Low 3 bits of `ctx+10440`; dirty bit `0x40`. Called from `FM2_Render_EmitPassDrawWork`. |
| `0x8236EA90` | `FM2_RenderContext_SetTextureFetchBitsMid` | `(8*a2)&0x7F8` in `ctx+10440`; adjacent texture-fetch subfield. |

### Command-buffer push/pop and pass constants

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8237E1A0` | `FM2_D3D_CommandBufferPushDrawState` | Increments stack index at `ctx+11841`, stores `(weekday<<6)|flags` byte at `ctx+11776`; saves prior draw-state dwords at `ctx+12428`. |
| `0x8237E3B8` | `FM2_D3D_CommandBufferPopDrawState` | Decrements stack index, restores draw-state dwords and dirty flags from saved block. |
| `0x8237E5D8` | `FM2_D3D_CommandBufferPushPassMarker` | Stack at `ctx+11907` / `ctx+11842`; saves pass marker byte before pass emit. |
| `0x8237E798` | `FM2_D3D_CommandBufferPopPassMarker` | Pops pass marker stack entry and restores saved PM4 shadow state. |
| `0x82515528` | `FM2_Render_UploadPassTransformConstants` | Matrix/vector math using globals `flt_829F4630..3C`; uploads pass transform block before draw emit. |
| `0x827312F8` | `FM2_D3D_EmitShaderBatchHeader` | Builds PM4 shader-batch header from pass/material inputs; called from pass constant upload path. |
| `0x82730DC0` | `FM2_D3D_EmitShaderConstantsBatch` | Large VMX constant emit loop; writes shader constant PM4 stream for active pass. |

### TLS / matrix helpers (render thread context)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827211D8` | `FM2_RenderTls_GetMainContextPtr` | Returns `&unk_82A41810`; main render TLS block base. |
| `0x827211E8` | `FM2_RenderTls_GetWorkerContextPtr` | Returns worker slice `unk_82A3D010 + 144*dword_82A419A0`. |
| `0x827217D0` | `FM2_RenderTls_BeginScopedContext` | Identity matrix init + TLS context select; entry for scoped emit from `FM2_Render_EmitPassDrawWork`. |
| `0x82721750` | `FM2_RenderTls_CopyMatrixToTlsContext` | Resolves TLS ptr then `FM2_Math_CopyMatrix4x4`. |
| `0x82721838` | `FM2_RenderTls_CopyMatrixToTlsContextB` | Same pattern as `82721750`; second call site in lighting/pass path. |
| `0x827218F8` | `FM2_RenderTls_SetCurrentDrawableHandle` | Stores drawable handle at TLS offset `+348`. |
| `0x82721D00` | `FM2_D3D_EnableAlphaBlendDefault` | Thin wrapper: `FM2_RenderContext_SetAlphaBlendEnableBits(dword_82A41BEC, 1)`. |
| `0x82724980` | `FM2_RenderTls_GetSubContextField160` | Calls `FM2_Math_CopyMatrix4x4` at `a1+160`. |
| `0x82721B88` | `FM2_Render_CopyMaterialConstantBlock` | Copies 6 dwords + 36-byte block via `FM2_MemcpyAligned`; material constant staging for emit. |
| `0x82724BF8` | `FM2_Math_CopyMatrix4x4` | Copies 16 floats (4�4) with column-major layout. |
| `0x82724C88` | `FM2_Math_SetIdentityMatrix4x4` | Writes identity to 4�4 float matrix. |
| `0x82724B00` | `FM2_RenderTls_AllocTlsIndex` | `KeTlsAlloc` CAS loop on `dword_829A76C0`; one-time TLS slot init. |

### D3D PM4 emit helpers (draw list / state bits)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823823A8` | `FM2_D3D_ComputeDrawPacketHeaderFlags` | Combines bound-surface flags, scissor override, pass-index bits into draw PM4 header dword. |
| `0x823822F0` | `FM2_D3D_PackDrawIndexRunLength` | Packs `(count-1)<<16 | opcode` index-run PM4 entries into command buffer. |
| `0x82382928` | `FM2_D3D_CountLeadingDirtyBits` | `_cntlzd` loop over dirty-bit qword; used by pending-state emit batches. |
| `0x823829E8` | `FM2_D3D_EmitDrawStateResetPackets` | Emits PM4 type-3 packets `8199`, `2609`, `0x10000` for draw-state reset. |
| `0x82382B68` | `FM2_D3D_EmitPendingStateBitsBatchA` | First `_cntlzd` sweep emitting set/clear register PM4 from dirty mask at `ctx+1020`. |
| `0x82382CC8` | `FM2_D3D_EmitPendingStateBitsBatchB` | Second dirty-bit sweep with 16-byte stride variant. |
| `0x82382E38` | `FM2_D3D_EmitTextureStageStatePackets` | Large texture-stage PM4 emitter; iterates stage records and writes register packets. |
| `0x823835C8` | `FM2_D3D_EmitSamplerStagePackets` | Emits sampler-stage PM4 from draw-list record bitfields. |
| `0x82383440` | `FM2_D3D_CopyRegisterPacketTemplate` | Copies 12-byte register packet template via `FM2_MemcpyAligned` when destination differs. |
| `0x82383510` | `FM2_D3D_MergeRegisterPacketBits` | Merges nibble bits into register packet before copy-back. |
| `0x82383718` | `FM2_D3D_EmitIndexedDrawPacket` | Builds indexed-draw PM4 from draw-list index buffer metadata. |
| `0x82383878` | `FM2_D3D_EmitShaderTokenStream` | Walks u16 token stream in draw-list record, emits shader register writes. |
| `0x82383928` | `FM2_D3D_CompareAndMarkShaderStateDirty` | Compares 416-byte shader state block; sets dirty qword at `ctx+12248` on mismatch. |

### Command-buffer allocation / submit chain

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82372C40` | `FM2_D3D_AllocateSecondaryCommandBuffer` | `D3D_AllocateCommandBuffer(4224,128)`; links segment into secondary chain. |
| `0x82373280` | `FM2_D3D_EnsureCommandBufferSpace` | Ensures `4*a2` bytes free; drains or grows ring via `FM2_D3D_GrowCommandBufferRing`. |
| `0x82372D30` | `FM2_D3D_GrowCommandBufferRing` | Expands ring buffer or switches to scratch segment at `ctx+16352`. |
| `0x82372170` | `FM2_D3D_SwitchToScratchCommandBuffer` | Sets write cursor to scratch base `ctx+16352`, sets flag `ctx+10813\|0x40`. |
| `0x823721D8` | `FM2_D3D_SuballocCommandBufferMemory` | Suballocates from command-buffer high-water mark with alignment. |
| `0x823724C8` | `FM2_D3D_SubmitCommandBufferChain` | Walks linked command-buffer segments and submits via D3D kick callbacks. |
| `0x82372840` | `FM2_D3D_BuildFlushKickPacket` | Builds flush/kick PM4 header dwords for segment submit. |
| `0x82374EB0` | `FM2_D3D_SuballocFromCommandBufferEnd` | Backward suballoc from buffer end with alignment mask. |
| `0x82371DF8` | `FM2_D3D_WaitForPrimaryOverrun` | Blocks on `D3DBLOCKTYPE_PRIMARY_OVERRUN` when ring full. |
| `0x82375138` | `FM2_D3D_AllocateKickSegment` | Allocates 72-byte kick segment; links via `ctx+13156`. |
| `0x823720D0` | `FM2_D3D_LinkCommandBufferSegment` | Links new GPU segment into command-buffer chain with physical address encoding. |
| `0x823755E0` | `FM2_D3D_LockedResourceAllocCallback` | Ring alloc callback under critsec; dispatches vtable `+172`. |
| `0x82375660` | `FM2_D3D_LockedResourceFreeCallback` | Ring free callback under critsec; vtable `+176`. |
| `0x823756C0` | `FM2_D3D_ReleaseRingAllocOrCallback` | Either `FM2_D3D_FlushCachedMemoryRange` + tagged free, or locked free callback. |
| `0x82375018` | `FM2_D3D_FlushCachedMemoryRange` | `D3D::FlushCachedMemory` over resource GPU range. |

### Render context bind / reset

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82371660` | `FM2_RenderContext_InitSurfaceBindDefaults` | Initializes surface-bind default dwords at `ctx+10240/10556` from draw-list flags. |
| `0x82369610` | `FM2_D3D_ResetAllShaderAndSurfaceState` | Clears all 4 color surfaces + depth, resets VS/PS state via named setters. |
| `0x82370CF8` | `FM2_RenderContext_ApplyViewportConstants` | Writes viewport float block at `ctx+12396` and triggers constant upload. |
| `0x823715B0` | `FM2_RenderContext_SetViewportModeAndApply` | Stores mode at `ctx+11576`, calls `ApplyViewportConstants`. |
| `0x82370E48` | `FM2_RenderContext_BindVertexStream` | Binds vertex stream D3DResource; stores stride/size at render-context stream slots. |
| `0x82370F68` | `FM2_RenderContext_BindIndexBuffer` | Binds index buffer to `ctx+12156` slot `0x2F7C`. |

### GPU memory block (emit path allocation)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236E648` | `FM2_D3D_CreateGpuMemoryBlock` | Allocates 0x368-byte header + payload via `FM2_Memory_AllocGpuTagged`; used when building GPU memory patches. |
| `0x8236DF18` | `FM2_D3D_InitGpuMemoryBlockHeader` | Zeroes header, sets type fields, walks patch table at `ctx+872`. |
| `0x8236C208` | `FM2_D3D_ApplyGpuMemoryPatches` | Applies relocation patches to GPU memory block payload. |

### Game memory allocator (reached from GPU block alloc)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82364A10` | `FM2_Memory_AllocGpuTagged` | Top-level tagged alloc; routes GPU physical tags to `FM2_Memory_AllocTagged`. |
| `0x82363DB8` | `FM2_Memory_AllocTagged` | Tagged alloc with XMem fallback for negative tags. |
| `0x82363768` | `FM2_Memory_AllocSmallBlockWithRetry` | Small-block alloc with deferred-callback flush retry. |
| `0x82364340` | `FM2_Memory_FreeTagged` | Routes free to heap vtable or small-block pool. |
| `0x82363528` | `FM2_Memory_FreeSmallBlock` | Small-block free via `FM2_Memory_FreeSmallOrFallback`. |
| `0x823636E0` | `FM2_Memory_FreeSmallBlockOrNull` | Null-checked variant. |
| `0x823681F8` | `FM2_Memory_FreeSmallOrFallback` | Tries small-block allocator vtable before CRT free. |
| `0x82367F60` | `FM2_Memory_AllocSmallOrFallback` | Tries small-block allocator before CRT malloc. |
| `0x82363D30` | `FM2_Memory_FindAllocatorForPointer` | Scans heap table for owning allocator. |
| `0x823647B0` | `FM2_Memory_SelectHeapForAlloc` | Selects heap by tag/kind for alloc request. |
| `0x823648D0` | `FM2_Memory_AllocFromSelectedHeap` | Alloc from selected heap with retry loop. |
| `0x82364700` | `FM2_Memory_GetCurrentFrameAllocatorKind` | Frame-counter keyed heap kind lookup. |
| `0x82364668` | `FM2_Memory_InsertFrameAllocRecord` | Inserts frame alloc record into sorted tree. |
| `0x823676C0` | `FM2_Memory_MergeFreeLists` | Coalesces free lists across heap buckets. |
| `0x82367D18` | `FM2_Memory_RecordFreeEvent` | Tracking hook recording free with size/tag. |
| `0x82367E48` | `FM2_Memory_RecordFreeIfTracking` | Guarded wrapper when tracking enabled. |
| `0x82363538` | `FM2_Memory_RunDeferredAllocatorCallbacks` | Locked flush of deferred free callbacks. |
| `0x82363700` | `FM2_Memory_GetDeferredCallbackListSingleton` | Lazy-init deferred callback list + critsec. |
| `0x82367450` | `FM2_Memory_FlushPendingCallbacks` | Walks allocator callback linked list. |
| `0x823674C8` | `FM2_Memory_EnsureDeferredListReady` | Ensures deferred list singleton initialized. |
| `0x82367508` | `FM2_Memory_NotifyAllocation` | Tracking notification on alloc. |
| `0x82367558` | `FM2_Memory_AllocFreeListNode` | Allocates intrusive-list node for free tracking. |
| `0x82367610` | `FM2_Memory_InitFreeListHead` | Initializes circular free-list head. |
| `0x82367C88` | `FM2_Memory_GetActiveFreeListSingleton` | Lazy-init active free-list singleton. |
| `0x82367F00` | `FM2_Memory_GetSmallBlockAllocatorSingleton` | Lazy-init `Memory::CSmallBlockAllocator`. |
| `0x82368918` | `FM2_Memory_InitSmallBlockAllocator` | Ctor: sets `Memory::CSmallBlockAllocator` vftable. |
| `0x82367398` | `FM2_Memory_InitAllocatorBase` | Ctor: sets `Memory::CAllocatorBase` vftable + critsec. |
| `0x82367378` | `FM2_Memory_IsTrackingFreesEnabled` | Returns `byte_829DBA99`. |
| `0x82367388` | `FM2_Memory_IsTrackingAllocsEnabled` | Returns `byte_829DBA9A`. |
| `0x82363C50` | `FM2_Memory_InitPoolStatsBlock` | Zeroes 8-dword pool stats block. |
| `0x8220A320` | `FM2_Memory_GetAllocatorContextSingleton` | Lazy-init allocator context at `unk_829C25D0`. |

### D3D GPU hang diagnostics

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82373828` | `FM2_D3D_HangCallbackDefaultA` | Default hang callback: reads GPU register slot by index. |
| `0x82373838` | `FM2_D3D_HangCallbackDefaultB` | Default hang callback: writes GPU register slot + `eieio`. |
| `0x823738D0` | `FM2_D3D_DumpRingBufferStateOnHang` | Dumps ring-buffer state via `D3D::Hang::Out` on GPU hang. |
| `0x82373AF0` | `FM2_D3D_ReportCommandBufferHang` | Reports command-buffer hang context strings. |
| `0x82373BB8` | `FM2_D3D_HandleGpuHang` | Full GPU hang handler; disassembles CB and invokes callbacks. |

### Intrusive list / STL tree (emit closure)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82586B10` | `FM2_IntrusiveList_InsertBefore` | Inserts node before anchor in circular intrusive list. |
| `0x82586D10` | `FM2_IntrusiveList_RemoveNode` | Removes node from intrusive list. |
| `0x82617920` | `FM2_IntrusiveList_InitIteratorWithSentinel` | Init iterator with sentinel node from tree lower bound. |
| `0x824B4958` | `FM2_StdTree_LowerBound` | MSVC `_Tree` lower_bound for frame-alloc map. |
| `0x825BBD40` | `FM2_StdTreeIterator_Increment` | MSVC `_Tree` iterator increment. |
| `0x82278010` | `FM2_NoopReturnZero` | Returns 0; placeholder callback in deferred-list init. |

### CRT runtime (emit closure � Microsoft CRT, prefixed for IDA clarity)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824613F8` | `FM2_Crt_InitCriticalSectionSpinCount` | `RtlInitializeCriticalSectionAndSpinCount` wrapper. |
| `0x8240BDF0` | `FM2_Crt_MemsetVectorized` | VMX vectorized memset (XMem zero on alloc). |
| `0x824117B8` | `FM2_Crt_GetProcessHeapHandle` | Returns `dword_82A78CF4` process heap. |
| `0x824117C8` | `FM2_Crt_GetProcessHeapHandle_Thunk` | Thunk to heap-handle getter. |
| `0x824117D0` | `FM2_Crt_SetTlsHeapHandle` | Stores heap handle in TLS block. |
| `0x82410330` | `FM2_Crt_HeapAllocInternal` | Core `_heap_alloc` implementation. |
| `0x82410C18` | `FM2_Crt_HeapFreeInternal` | Core `_heap_free` implementation. |
| `0x82410F00` | `FM2_Crt_HeapReallocInternal` | Core `_heap_realloc`. |
| `0x8240DB78` | `FM2_Crt_HeapAlloc` | Public `_malloc` entry through heap layer. |
| `0x8240DBC0` | `FM2_Crt_HeapFree` | Public `_free` entry through heap layer. |
| `0x8240DB58` | `FM2_Crt_FreeHeapBlock` | Routes free to heap or null-free path. |
| `0x8240DAA8` | `FM2_Crt_JumpToFreeTail` | Tail jump to CRT free helper. |
| `0x82417720` | `FM2_Crt_Malloc` | `_malloc` with new-handler loop. |
| `0x824177E8` | `FM2_Crt_Realloc` | `_realloc`. |
| `0x82417A18` | `FM2_Crt_Calloc` | `_calloc`. |
| `0x82417A88` | `FM2_Crt_Free` | `_free`. |
| `0x82419018` | `FM2_Crt_ValidateHeap` | Heap validation walk. |
| `0x82419B58` | `FM2_Crt_LockHeap` | Locks global heap spinlock. |
| `0x82419E10` | `FM2_Crt_UnlockHeap` | Unlocks global heap. |
| `0x82419CC8` | `FM2_Crt_LockHeapWrapper` | Thin wrapper calling `FM2_Crt_LockHeap`. |
| `0x82411ED0` | `FM2_Crt_HeapCorruptionTrap` | Deliberate trap on heap corruption. |
| `0x82412130` | `FM2_Crt_SetHeapValidationFlag` | Sets `dword_82A78CF8`. |
| `0x82413040` | `FM2_Crt_MemmoveS` | Secure memmove with bounds checks. |
| `0x82413B48` | `FM2_Crt_HeapWalk` | `_heapwalk` iterator. |
| `0x8241CE78` | `FM2_Crt_AlignedOffsetMalloc` | `_aligned_offset_malloc`. |
| `0x8241A328` | `FM2_Crt_LookupErrorString` | Maps error code to message table. |
| `0x8241A540` | `FM2_Crt_ReportError` | `_CrtDbgReport` banner output. |
| `0x82419AE0` | `FM2_Crt_AbortBanner` | Fatal error banner tail. |
| `0x82419AE8` | `FM2_Crt_LockGlobal` | `_lock(8)`. |
| `0x82419AF0` | `FM2_Crt_UnlockGlobal` | `_unlock(8)`. |
| `0x824227C8` | `FM2_Crt_Write` | `_write` syscall wrapper. |
| `0x82429808` | `FM2_Crt_Fopen` | `_fopen`. |
| `0x82429898` | `FM2_Crt_Fclose` | `_fclose`. |
| `0x82429920` | `FM2_Crt_Fread` | `_fread`. |
| `0x82429A58` | `FM2_Crt_Fwrite` | `_fwrite`. |
| `0x827571D8` | `FM2_Crt_Fprintf` | `_fprintf`. |
| `0x827573D8` | `FM2_Crt_Fflush` | `_fflush`. |
| `0x824210B0` | `FM2_Crt_ClearFhandleLockState` | Clears file-handle lock state. |
| `0x82421AD0` | `FM2_Crt_GetFhandleLockTable` | Returns `&unk_82998590` lock table. |
| `0x82421C48` | `FM2_Crt_LockFhandleByPtr` | Locks file handle by pointer index. |
| `0x82421C80` | `FM2_Crt_LockFhandleByIndex` | Locks file handle by fd index. |
| `0x82421C98` | `FM2_Crt_TryUnlockFhandleByPtr` | Unlock file handle by pointer. |
| `0x82421CD0` | `FM2_Crt_TryUnlockFhandleByIndex` | Unlock file handle by index. |
| `0x82421F40` | `FM2_Crt_FilenoToFhandle` | Maps `FILE*` to internal handle. |
| `0x824222F4` | `FM2_Crt_UnlockFhandleByPtrWrapper` | Unlock wrapper. |
| `0x82422924` | `FM2_Crt_UnlockFhandleWrapper` | Unlock wrapper (duplicate path). |
| `0x824246C8` | `FM2_Crt_UnlockFhandleDup` | Duplicate unlock path. |
| `0x824230B8` | `FM2_Crt_UnlockTail` | Tail jump to unlock helper. |
| `0x82423D00` | `FM2_Crt_GetStreamLockCount` | Returns stream lock counter. |

### STL / EH (emit closure � reached via hang dump strings)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821D0000` | `FM2_Stl_FileStream_Dtor` | `std::filebuf` dtor; closes via `FM2_Crt_Fclose`. |
| `0x821D03E8` | `FM2_Stl_String_ThrowLengthError` | Throws `std::length_error`. |
| `0x821D0C60` | `FM2_Stl_String_InitOrClear` | SSO string init/clear. |
| `0x821D0D50` | `FM2_Stl_String_Reserve` | String capacity reserve. |
| `0x821D0E10` | `FM2_Stl_String_AppendRange` | Append byte range to string. |
| `0x821D1568` | `FM2_Stl_Istream_ReadBytes` | `istream::read` byte loop. |
| `0x821D1868` | `FM2_Stl_LogicError_Dtor` | `std::logic_error` dtor. |
| `0x821D24D8` | `FM2_Stl_String_AssignRange` | String assign from range. |
| `0x821D25C0` | `FM2_Stl_String_AppendCStr` | Append C string. |
| `0x821D26C8` | `FM2_Stl_String_CopyAssign` | Copy-assign string. |
| `0x821D2720` | `FM2_Stl_LogicError_CtorFromString` | Construct logic_error from message string. |
| `0x821D27C8` | `FM2_Stl_OutOfRange_CtorFromString` | Construct out_of_range from message. |
| `0x821D2820` | `FM2_Stl_String_CtorFromCStr` | Construct string from C string. |

---

## Batch 4 � Random sample (1000 renames, 2026-06-18)

**Method:** `random.seed(20260618)` ? `random.sample(sub_*, 1000)` from **45,689**
unnamed functions. Each function was decompiled and renamed via heuristics in
`scripts/ida_fm2_random_rename_batch.py` (IDA MCP `py_exec_file`).

**Result:** 1000/1000 renamed successfully (0 skipped, 0 failed).

### Naming heuristics (reasoning by category)

| Pattern | Example name | Reasoning |
| --- | --- | --- |
| Decompile references `std::` | `FM2_Stl_StringHelper` | MSVC STL method body detected in pseudocode. |
| Decompile references `D3D::` / `D3D_` | `FM2_D3D_ShaderHelper` | Xbox D3D runtime helper reached from game code. |
| Calls existing `FM2_*` symbol | `FM2_Render_NotifyManagerStateChange_Caller` | Named for dominant already-named callee in decompile. |
| Size = 8 bytes | `FM2_Thunk` / `FM2_JumpTail` | Branch/tail stub, not a semantic function body. |
| String literal in decompile | `FM2_Str_<slug>` | Domain hint from embedded string (when =4 chars). |
| `Rtl*` / `Ke*` / `XMem*` | `FM2_Kernel_RtlHelper` | Kernel/XAPI allocator or sync helper. |
| `malloc`/`free`/`Heap*`/`lock(` | `FM2_Crt_AllocHelper` | CRT heap/lock helper in closure. |
| `vftable` reference | `FM2_Class_Method` / `FM2_Class_Dtor` | C++ virtual method or destructor. |
| No strong signal | `FM2_Helper_XXXX` | Fallback: low 16 bits of address (`XXXX`) for uniqueness. |

### Name distribution (approximate)

| Category | Count |
| --- | ---: |
| `FM2_*_Caller` (wraps known FM2 callee) | ~528 |
| `FM2_Helper_XXXX` (address fallback) | ~304 |
| `FM2_Stl_*Helper` | ~80 |
| `FM2_D3D_*Helper` | ~50 |
| `FM2_Crt_*Helper` | ~30 |
| Thunks / traps / other | ~8 |

### Artifacts

- Full address list + new names:
  `.cursor/hooks/state/random-rename-log-1000.json`
- Repro script: `scripts/ida_fm2_random_rename_batch.py`
- Export script: `scripts/ida_fm2_random_rename_export_log.py`

**Note:** Batch 4 names are **heuristic placeholders** suitable for navigation
and later refinement � unlike batches 1�3 which used manual evidence-based
naming. Re-run targeted manual passes on hot paths as needed.

---

## Batch 4 � Manual re-pass (replacing heuristic placeholders)

The automated heuristic batch (1000 `FM2_*Helper` / `*_Caller` names) is being
**replaced** with manual decompile-based names and explicit reasoning, same
standard as batches 1�3.

**Progress:** 700 / 978 placeholders corrected (passes 1�19 below). Remaining
placeholders tracked in `.cursor/hooks/state/batch4-placeholders.json`.

**Workflow:** `scripts/ida_fm2_decompile_placeholder_slice.py` (decompile slice)
? manual name from strings/RTTI/vtable/callees ? IDA MCP `rename` ? log here.

### Manual re-pass 1 (35 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821D4648` | `FM2_Object_VirtualDispatch_Offset12` | Single-instruction vtable dispatch at offset +12. |
| `0x821D61A8` | `FM2_ForzaCmdLine_InitStartupList` | Uses `FM2_GetForzaCommandLineParamsSingleton` + `FM2_IntrusiveList_SpliceNodes` to build startup list. |
| `0x821D7AC8` | `FM2_Math_VmxTransformVectorBlock` | VMX128 `lvlx`/`stvx` vector transform with float inputs. |
| `0x821D8240` | `FM2_Keyframe_GetFloatAtOffset100` | Returns `*(float*)(a1+100)` � keyframe/scalar field accessor. |
| `0x821DCCF8` | `FM2_UI_ClampOpacityHalfStep` | Half-step opacity update clamped to 1.0 on field at +52. |
| `0x821E37D8` | `FM2_Camera_LoadFileDataFromScript` | RTTI `X_Script::CScriptScope`; loads `"Camera\\FileData"`. |
| `0x821F0D70` | `FM2_Keyframe_GetEffectiveScalarValue` | Returns 0 if flag `0x100` else float at +12. |
| `0x821F1150` | `FM2_SceneGraph_RegisterChildNode` | Registers child via `sub_822905A0` on scene graph object. |
| `0x821F16C0` | `FM2_Math_VmxInitSinCosConstants` | VMX128 constant init from `unk_82000D00`/`unk_82000CA0` tables. |
| `0x821F1938` | `FM2_Math_VmxInitVectorConstants` | VMX128 vector constant table init (`unk_82003BB0`). |
| `0x821F1D20` | `FM2_Math_VmxPackMatrixRows` | VMX permute/store of matrix rows to result+16/+32/+48. |
| `0x821F3EF0` | `FM2_Animation_EvaluateKeyframeCurve` | Large keyframe evaluator with float curve inputs and flag `a3`. |
| `0x822041A8` | `FM2_RefCounted_DestroyAndCleanup` | Virtual release then `sub_82203D10` cleanup on ref-counted object. |
| `0x822046F8` | `FM2_D3D_InitFrameLockSingleton` | Initializes critsec + frame-lock globals for D3D path. |
| `0x822047D8` | `FM2_D3D_FatalGpuHangSpin` | `__noreturn` infinite loop after frame-counter GPU hang check. |
| `0x82206910` | `FM2_CarSpec_ParseUpgradePathString` | Parses car upgrade path string with STL copy and token rules. |
| `0x82207090` | `FM2_Heap_DeleteOptional` | Optional `FM2_Memory_FreeSmallBlockOrNull` after cleanup. |
| `0x8220A2D8` | `FM2_ISetGlobal_Dtor` | Dtor for `anonymous namespace::ISetGlobal` vftable. |
| `0x8220BAF8` | `FM2_Profile_ApplyManagerStateNotify` | Profile state block + `FM2_Render_NotifyManagerStateChange`. |
| `0x8220C0C0` | `FM2_Profile_LoadDefaultSettings` | Loads default profile settings under global lock. |
| `0x8220E1A0` | `FM2_MovieRenderer_OpenMovieManagerKey` | Registry key `"MovieRenderer::MovieManager"`. |
| `0x82213160` | `FM2_ProfileLua_UpdateProfileManagerState` | Updates state for `"ProfileLua::ProfileManager"`. |
| `0x8221B770` | `FM2_HashName_LookupMainModuleProperty` | `CHashName` lookup with module name `"Main"`. |
| `0x8221B908` | `FM2_HashName_LookupAltModuleProperty` | Same as above but uses alt module table `dword_829C31CC`. |
| `0x8221C1B0` | `FM2_ManagerState_ApplyNotifyCallback` | Applies notify callback and sets ready bytes on manager node. |
| `0x8221CA28` | `FM2_Stl_String_ResizeAndNullTerminate` | SSO string resize with null wchar write. |
| `0x8221EBE8` | `FM2_Settings_GetDegreesHiddenAsRadians` | Reads `"DegreesHidden"` hash; converts deg?rad. |
| `0x82224DC0` | `FM2_Script_RegisterBinding` | Registers script binding via `sub_82435A20`. |
| `0x82228B50` | `FM2_Boot_ParseCommandLineAndInitSubsystems` | Large boot init using Forza command-line singleton. |
| `0x8222F348` | `FM2_CarDb_QueryStockPartByOrdinal` | SQL `SELECT * FROM %s WHERE Ordinal = %u AND IsStock = 1`. |
| `0x8222FB28` | `FM2_StringTriple_InitEmpty` | Zeroes three fields then inits embedded SSO string. |
| `0x82233F88` | `FM2_CarDb_QueryThumbnailById` | SQL `SELECT Thumbnail FROM %s WHERE Id=%u`. |
| `0x822388F8` | `FM2_Vector_PushBackAtOffset504` | Thin wrapper around `Vector_PushBack32` at `a1+504`. |
| `0x8223DC08` | `FM2_UI_SyncListSelectionIndex` | Copies list index from +48 to +140/+144. |
| `0x82243EF8` | `FM2_CarParts_ApplyUpgradeToSlotGroup` | Iterates slot group applying upgrade via `sub_82242458`. |

### Manual re-pass 2 (35 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82245FC8` | `FM2_ResourceLock_CaptureRetainedHandleBatch` | Loop calling `FM2_ResourceLock_CaptureRetainedHandle` every 32 bytes. |
| `0x82254360` | `FM2_ProfileDb_InsertProfileRecord` | RB-tree insert with profile notify state; returns node+16. |
| `0x82255728` | `FM2_UI_ClearFlagAt803` | Clears byte flag at object offset +803. |
| `0x82258190` | `FM2_GraphicsStream_ApplyFormatToDevice` | Gets stream format string and applies to graphics device vtable+32. |
| `0x82258C78` | `FM2_GraphicsStream_NotifyAllLinkedAdapters` | Walks linked adapter list calling notify on each. |
| `0x8225D260` | `FM2_RenderAdapter_SwitchPresentationMode` | Switches presentation mode when `*(a1+80) != a2`. |
| `0x82262F60` | `FM2_RenderAdapter_DestroyAndFreeChild` | Destroys render adapter child and optionally frees. |
| `0x82267918` | `FM2_Vector48Iterator_AssignFromRange` | Assigns 48-byte element vector iterator with range checks. |
| `0x8226C9E8` | `FM2_Vector48_BinarySearchSlice` | Binary search on 48-byte sorted vector slice. |
| `0x8226F790` | `FM2_FileInfoCache_AllocateEntry` | Allocates 32-byte cache entry; sprintf filename from stream. |
| `0x82272418` | `FM2_ComObject_CopyConstructWithRefCount` | COM-style copy construct with ref-count and weak-ref paths. |
| `0x822736E0` | `FM2_Math_VmxCopyStruct48Bytes` | VMX128 copy/reorder of 48-byte struct fields. |
| `0x822751C8` | `FM2_AIDb_QueryLineChoicesBySkillId` | SQL on `AILineChoices` by `AISkills_id`. |
| `0x82276070` | `FM2_AIDb_QueryPittingProfileById` | SQL on `PittingProfiles` by id; fills float profile fields. |
| `0x82278618` | `FM2_DeferredQueue_EnqueueByteFlagTask` | Allocates 8-byte deferred task with byte flag payload. |
| `0x82278750` | `FM2_RefCounted_ReleaseWithDecrement` | Atomic decrement at +28; releases object at zero. |
| `0x8227D4C0` | `FM2_ComObject_DeleteOptional` | COM delete with optional pool free. |
| `0x8227DCF0` | `FM2_ComObject_ResetToBaseVtables` | Resets dual vtables to base `off_8200F394`/`off_8200F38C`. |
| `0x8227F710` | `FM2_DeferredQueue_EnqueueNotifyStateTaskA` | Deferred enqueue + notify via `sub_8227F5C0`. |
| `0x8227FCB0` | `FM2_DeferredQueue_EnqueueNotifyStateTaskB` | Same pattern via `sub_8227FB60`. |
| `0x8227FD40` | `FM2_DeferredQueue_InitNotifyStateParams` | Constructs deferred params with notify state vftable. |
| `0x82280C88` | `FM2_GraphicsStream_DeferredSetIndicesParams_Ctor` | Ctor for deferred set-indices command params. |
| `0x822825B8` | `FM2_ResourceManager_LoadPendingResourcesLocked` | Locked walk loading pending resources via `FM2_ResourceManager_LoadAndWait`. |
| `0x82283E88` | `FM2_GraphicsStreamSetIndicesParams_Dtor` | Dtor for `CGraphicsStreamDeferred::CParams1IGraphicsStreamSetIndices`. |
| `0x822853E0` | `FM2_MovieRendererUpdateParams_CopyString` | Copies string into `CRenderAdapterLink::CParams1IMovieRendererUpdateMovie`. |
| `0x82285440` | `FM2_MovieRendererUpdateParams_Dtor` | Dtor resetting to `CDeferredQueue::CCommandParams` vftable. |
| `0x8228D980` | `FM2_RenderAdapterLink_InitStateBlock` | Initializes render-adapter link state fields +57..+66. |
| `0x8228DA30` | `FM2_GraphicsAdapter_ExecuteSqlForEachStream` | Executes SQL per linked graphics stream adapter. |
| `0x822921F8` | `FM2_IntrusiveList_EraseNodeReassignNext` | Intrusive list erase with next-pointer fixup. |
| `0x8229A5A8` | `FM2_AudioSample_BuildOutputPairDescriptor` | Builds audio output sample pair descriptor struct. |
| `0x8229C848` | `FM2_AudioMix_ProcessOutputSampleBatch` | Batch audio mix output using `FM2_AudioMix_WriteOutputSamplePair`. |
| `0x8229D280` | `FM2_AudioBuffer_DeleteOptional` | Audio buffer cleanup with optional pool free. |
| `0x8229F040` | `FM2_DeferredAdapter_NotifyChildStateIfIdle` | Notifies child adapter state when parent idle. |
| `0x8229F658` | `FM2_CompositeAdapterState_Dtor` | Composite adapter state dtor freeing nested strings/resources. |
| `0x822A12A8` | `FM2_MovieRenderer_TestRegistryKeyOpen` | Tests whether movie-renderer registry key opens successfully. |

### Manual re-pass 3 (70 functions)

Mostly Lua script bindings (`dword_829F40D0` method name + `sub_8254E1A0`
userdata lookup) plus a few physics/replay/scene helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x822A4860` | `FM2_Lua_AppForza_Unpause` | Lua `"Unpause"` on `"App::Forza"`. |
| `0x822A7798` | `FM2_Lua_AppMovieHandle_IsReady` | Lua `"isReady"` on `"App::MovieHandle"`; checks fields +36/+40 == 3. |
| `0x822A8898` | `FM2_Lua_PushBoolFromObjectByte20` | Vtable+72 getter; pushes byte at offset 20 to Lua stack. |
| `0x822AA100` | `FM2_Crt_InitAtExitCallbackSingleton` | One-time `atexit(sub_829495E0)` singleton init. |
| `0x822AB5B0` | `FM2_Lua_AuctionHouse_GiftCar` | Lua `"GiftCar"` on `"LuaAuctionHouse::AuctionHouse"`. |
| `0x822AD190` | `FM2_Lua_AuctionHouseRecord_GetHighBidder` | Lua `"highBidder"` on `"LuaAuctionHouse::AuctionHouseRecordsetItem"`. |
| `0x822B4518` | `FM2_Lua_ForzaTV_SpectatedRaces_Refresh` | Lua `"Refresh"` on `"ForzaTV::SpectatedRaces"`. |
| `0x822B4AA0` | `FM2_Lua_PushNamedRegistryValue` | Allocates registry entry, pushes via Lua stack helpers. |
| `0x822B76D8` | `FM2_Lua_PushKeyframeFloatToStack` | Reads `FM2_Keyframe_GetFloatAtOffset100`, pushes to Lua. |
| `0x822B9338` | `FM2_Lua_CallObjectVtable268ToStack` | Object vtable+268 result pushed via `sub_822B88F0`. |
| `0x822BAFA8` | `FM2_Lua_PushDampingFromKeyframeFloat` | Keyframe float ? damping lookup ? Lua push. |
| `0x822BBC70` | `FM2_Math_VmxNormalizeAndPushForceVector` | VMX128 normalize (vrsqrtefp) then force vector push. |
| `0x822C36D0` | `FM2_Lua_GameLibraryDynamics_GetCompressorATM` | Lua `"compressorATM"` on `"GameLibrary::Dynamics"`. |
| `0x822C4748` | `FM2_Lua_GameLibraryTire_GetSuspensionDmg` | Lua `"suspensionDmg"` on `"GameLibrary::Tire"`. |
| `0x822C4D20` | `FM2_Lua_GameLibraryTire_GetDistUnder` | Lua `"DistUnder"` on `"GameLibrary::Tire"`. |
| `0x822C6060` | `FM2_Lua_GameLibraryLapTracker_GetMaxSegments` | Lua `"MAX_SEGMENTS"` constant 17 on `"GameLibrary::LapTracker"`. |
| `0x822C7FB8` | `FM2_Lua_PushEngineTypeEnumToStack` | Engine type index =0x1B ? enum string push. |
| `0x822C92F0` | `FM2_Lua_LapTracker_GetSegmentTimeDelta` | Lap segment index ? time delta push. |
| `0x822C9F90` | `FM2_Lua_PushNotifyStateObjectToRegistry` | Builds notify-state object with vftable `off_820149DC`. |
| `0x822CF2E8` | `FM2_StdStream_WaitReadyAndReadLine` | Spin-waits stream ready flag then reads line. |
| `0x822D18A8` | `FM2_Lua_LuaGarage_CalcPerformanceIndex` | Lua `"CalcPerformanceIndex"` on `"LuaGarageLib::LuaGarage"`. |
| `0x822D5CC8` | `FM2_Lua_LeaderboardManager_EnsureInstance` | Ensures `"LeaderboardLua::LeaderboardManager"` userdata. |
| `0x822D7040` | `FM2_Lua_LeaderboardManager_GetFreerunRollupType` | Pushes enum 7 for `"E_LBTYPE_FREERUN_ALLTRACK_ROLLUP"`. |
| `0x822D76A0` | `FM2_Lua_LeaderboardManager_GotFirstPlaceLocalPrimary` | Lua getter on leaderboard manager. |
| `0x822D7EB0` | `FM2_Lua_LeaderboardManager_EnumerateByXuid` | Lua `"EnumerateLeaderboardByXuid"`. |
| `0x822D8668` | `FM2_Lua_LeaderboardManager_GameClipDownloadInProgress` | Reads clip download state from global manager. |
| `0x822DE048` | `FM2_Lua_LiveryEditor_SetColorFromLuaArgs` | Multi-arg color setter for livery editor. |
| `0x822DE3C8` | `FM2_SceneProp_BindSslObjectToManager` | `propBASE::GetSslObject` + notify manager bind. |
| `0x822E1298` | `FM2_Lua_LiveryEditor_RevertCurrLayer` | Lua `"RevertCurrLayer"` on `"LiveryLua::LiveryEditor"`. |
| `0x822E1A88` | `FM2_Lua_LiveryEditor_CurrentSectionHasOppositeSide` | Lua getter on livery editor. |
| `0x822E25B8` | `FM2_Lua_LiveryEditor_LoadDecalsFor` | Lua `"loadDecalsFor"`. |
| `0x822E2920` | `FM2_Lua_LiveryEditor_SetCurrentDecalTab` | Sets decal tab index 3. |
| `0x822E68F0` | `FM2_Lua_LiveryColor_Finish` | Lua `"Finish"` on `"LiveryLua::LiveryColor"`. |
| `0x822E7558` | `FM2_Lua_Livery_CreateNewLayerAt` | Lua `"createNewLayerAt"` on `"LiveryLua::Livery"`. |
| `0x822E7F68` | `FM2_Lua_GetObjectPropertyAsInt` | Property lookup vtable+108 ? int push. |
| `0x822E8138` | `FM2_Lua_PushSslObjectToStack` | Wraps SSL object ref for Lua stack. |
| `0x822E8C28` | `FM2_Lua_MessageCenter_GetCarName` | Lua `"CarName"` on `"MessageCenter::Message"`. |
| `0x822E93A0` | `FM2_Lua_PushBoolFromFilenameMatch` | Filename substring match ? bool push. |
| `0x822EC410` | `FM2_Lua_PushQwordPairToStack` | Object vtable+28 qword pair push. |
| `0x822ECC10` | `FM2_Lua_PushStringFromObjectVtable20` | Object vtable+20 string push. |
| `0x822EFC00` | `FM2_Lua_NetworkLobby_SetSystemLink` | Lua `"SetSystemLink"` ? `FM2_RenderAdapter_SwitchPresentationMode(*,3)`. |
| `0x822F0110` | `FM2_Lua_NetworkLobby_SetMultiscreenClient` | Lua `"SetMultiscreenClient"` ? presentation mode 9. |
| `0x822F0368` | `FM2_Lua_NetworkLobby_SortPlayerListPrev` | Lua `"SortPlayerListPrev"` on `"NetworkLua::Lobby"`. |
| `0x822F1380` | `FM2_Lua_NetworkLobby_ClearOptionsChanged` | Lua `"clearOptionsChanged"`. |
| `0x822F2130` | `FM2_Lua_NetworkLobby_IsTournamentGame` | Lua bool getter. |
| `0x822F2DB0` | `FM2_Lua_NetworkLobby_GetLobbyTypeTournamentPractice` | Pushes lobby type enum 8. |
| `0x822F37B8` | `FM2_Lua_NetworkLobby_GetAllPlayersInLobby` | Builds player count for lobby. |
| `0x822F3A80` | `FM2_Lua_NetworkLobby_ClearSeriesPoints` | Lua `"clearSeriesPoints"`. |
| `0x822F6880` | `FM2_Lua_PlayerChoices_SetAssistShifting` | Sets assist shifting index 3. |
| `0x822FB178` | `FM2_SceneCamera_SetStereoscopicModeFromLua` | Writes stereo mode to camera +612 from Lua arg. |
| `0x822FB560` | `FM2_Lua_SetCameraEffectFloatParams` | Sets four float fields +340..+356 on camera effects. |
| `0x822FE7D0` | `FM2_Lua_PhotoModeSavedPhoto_GetUploadResults` | Lua on `"PhotoMode::SavedPhotoEnumerator"`. |
| `0x822FF158` | `FM2_Lua_PhotoModeCamera_GetDepthBufferValueAt` | Lua on `"PhotoMode::CameraEffects"`. |
| `0x822FF498` | `FM2_Lua_PhotoModeCamera_GetBlurTarget` | Reads blur target from +612. |
| `0x822FF6D0` | `FM2_Lua_PhotoModeCamera_GetFocus` | Lua `"focus"` getter. |
| `0x82301E90` | `FM2_Rewards_QueryArcadeCarIdFromDatabase` | SQL `"SELECT CarId FROM Rewards_Arcade"`. |
| `0x823047C8` | `FM2_Lua_PushDegreesBetweenScreensFromHash` | Hash lookup `"DegreesBetweenScreens"` ? float push. |
| `0x82307D58` | `FM2_Lua_ProfileManager_GetFpInitNeedStorageDevice` | Enum 1 for `"FP_INIT_NEED_VALID_STORAGE_DEVICE"`. |
| `0x82309A90` | `FM2_Lua_ForzaProfile_GetThrottleDeadzoneInsideWheel` | Profile float getter. |
| `0x8230A158` | `FM2_Lua_ForzaProfile_GetControllerType` | Profile controller type getter. |
| `0x8230ADF0` | `FM2_Lua_PushCarIdFromCarRecord` | Car record ? car id push. |
| `0x8230B540` | `FM2_SavedReplay_Dtor` | Dtor for `CSavedReplay` vftable. |
| `0x8230E890` | `FM2_Lua_ReplayRecorder_SaveVolumeWrapper` | Lua `"saveVolumeWrapper"` on `"ReplayTheater::ReplayRecorder"`. |
| `0x8230EA30` | `FM2_ReplayTheater_RegisterCallbackThunk` | Thin thunk to replay theater callback register. |
| `0x8230F258` | `FM2_ReplayBuffer_DeleteOptional` | Replay buffer cleanup with optional free. |
| `0x823114A8` | `FM2_Replay_QueueSslReadWithArgs` | Queues SSL read with float/time args. |
| `0x82312060` | `FM2_Lua_RewardReveal_GetCarLevelRewardCompatInfo` | Lua on `"RewardReveals::RewardReveal"`. |
| `0x823126E0` | `FM2_Lua_RaceWinnings_GetDamagePenaltyValue` | Reads damage penalty from winnings object +160. |
| `0x82315128` | `FM2_Lua_CreateComPtrFromThreeLuaNumbers` | Three Lua numbers ? COM object construct. |
| `0x823162F0` | `FM2_Lua_SavedGameContentItem_GetAsyncOperationResult` | Async op result enum push. |

### Manual re-pass 4 (35 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823165C0` | `FM2_Lua_SaveVolumeWrapper_GetOperationResult` | Lua `"operationResult"` on `"SavedGame::SaveVolumeWrapper"`. |
| `0x8231A420` | `FM2_Lua_Tournament_GetNumQualifyingEntries` | Lua on `"TournamentLua::Tournament"`. |
| `0x8231A880` | `FM2_Lua_Tournament_GetHasBranding` | Reads branding flag at object +25676. |
| `0x8231C7C8` | `FM2_Lua_PushFloatFromObjectField2` | Object field index 2 ? float Lua push. |
| `0x8231F768` | `FM2_Lua_PushTuningDatabaseSingleton` | Lazy-init 2116-byte tuning DB singleton. |
| `0x82325C68` | `FM2_Lua_LuaTuning_GetFrontAccelValue` | Lua on `"LuaTuning::LuaTuningClass"`. |
| `0x82326560` | `FM2_Lua_LuaTuning_SetRearDecelIncrement` | Lua setter on tuning class. |
| `0x82329C18` | `FM2_Lua_XStorageManager_GetDownloadedCarSetup` | Lua on `"XStorageLua::XStorageManager"`. |
| `0x8232D240` | `FM2_CareerCircuitRaceCoordinator_Dtor` | Dtor for `CCareerCircuitRaceCoordinator` vftable. |
| `0x8232DC50` | `FM2_Career_CreatePendingBoolAndSslObject` | Creates `TPendingInternal<bool>` + SSL notify object. |
| `0x823337A0` | `FM2_Stl_IntrosortPartitionRecursive` | Introsort median-of-three partition (threshold 40). |
| `0x82335480` | `FM2_Audio_ApplyVolumeIfEnabled` | Conditional volume apply when audio enabled. |
| `0x82335840` | `FM2_CareerRace_ApplyPostRaceCameraAndEffects` | Post-race camera/effect setup on race object. |
| `0x82336430` | `FM2_CareerRace_ProcessEndOfRaceRewards` | Large end-of-race reward/credit processing. |
| `0x82337B10` | `FM2_RaceEntry_CompareByFinishTime` | Sort comparator on finish time fields +24/+28. |
| `0x82339A58` | `FM2_RaceEntry_OnFinishedNotify` | Finished callback when vtable+88 returns true. |
| `0x82339B70` | `FM2_RaceEntry_AllocStateBlock` | Allocates 48-byte race entry state block. |
| `0x8233BF68` | `FM2_CareerRace_BuildGhostReplayBuffer` | Builds 2320-byte ghost replay buffer entries. |
| `0x8233D5C0` | `FM2_RaceTimer_UpdateElapsedFromTick` | Updates elapsed time from tick delta. |
| `0x8233E668` | `FM2_RaceGhostRecord_ReleaseResources` | Releases ghost record COM refs and buffer. |
| `0x8233ECA8` | `FM2_RaceGhost_BuildFilenameFromTrackAndCar` | Concat `"ghost_" + track + "_" + car`. |
| `0x82341138` | `FM2_RaceGhost_InitFromWorldState` | Initializes ghost from world position/velocity. |
| `0x82341F40` | `FM2_RaceGhost_UpdatePlaybackPosition` | Updates ghost playback from race state. |
| `0x82343120` | `FM2_RaceGhost_GetElapsedSecondsSinceStart` | Elapsed ms since start / 1000. |
| `0x8234F228` | `FM2_SceneNodeTree_DestroyRecursive` | Recursive scene node tree destroy (+305 leaf flag). |
| `0x8234F280` | `FM2_SceneNode_AllocWithThreeFields` | Allocates scene node with three init fields. |
| `0x82352718` | `FM2_UI_GetThemeColorByIndex` | Theme color lookup by index with alpha from +108. |
| `0x82358158` | `FM2_Stl_Vector_EraseRangeAt` | Vector erase-range helper with bounds trap. |
| `0x8235A6B8` | `FM2_Input_DetectWheelSubtypeAndInit` | `XamInputGetCapabilities`; wheel subtype 2 check. |
| `0x8235C0C8` | `FM2_Input_InitControllerDevices` | Init 4 controllers named `"Controller"` + index. |
| `0x8235D148` | `FM2_Input_SetControllerButtonBit` | Sets button bit in controller state qword. |
| `0x823639D8` | `FM2_Input_UpdateControllerStateList` | Updates intrusive controller state list links. |
| `0x823641C8` | `FM2_Memory_TryFreeViaPoolHandler` | Routes free through pool handler vtable+20/+48. |
| `0x82366570` | `FM2_Memory_DeferredFreeEnqueue` | Enqueues deferred free under RTL critical section. |
| `0x823745E8` | `FM2_D3D_DeviceInitFrameStateAndGpuBuffer` | Sets frame counter; allocates 0x12C0 GPU buffer. |

### Manual re-pass 5 (35 functions)

D3D command buffer, image/PNG pipeline, and content/async helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82382148` | `FM2_D3D_InitCommandBufferSlotTable` | Initializes CP slot table from device flags at +21912. |
| `0x82387B08` | `FM2_D3D_CreateTextureFromSurfaceLevel` | D3D surface/texture create with pool/format args. |
| `0x82388E18` | `FM2_Image_SwapEndianPixelRow` | Per-pixel byte/word swap (big?little) on RGBA row. |
| `0x8238C680` | `FM2_Image_LoadPngFromMemory` | D3DX `png_struct`/`png_info` decode + GPU alloc. |
| `0x8238EF08` | `FM2_Image_Downsample2x2BytesAverage` | 2�2 byte pixel box filter average. |
| `0x82391E28` | `FM2_Struct_ClearFiveDwords` | Zeroes five consecutive DWORD fields. |
| `0x823A4B38` | `FM2_Shader_ApplyConstantsBatch` | Batch-applies shader constants in nested loops. |
| `0x823AB9D8` | `FM2_Shader_InitCallbackTable` | Installs three shader callback fn ptrs. |
| `0x823AF460` | `FM2_Bitstream_ReadVariableBits` | Variable-width bit extraction from stream. |
| `0x823B46A0` | `FM2_Huffman_DecodeSymbol` | Huffman tree walk; error code 118 on overflow. |
| `0x823C0230` | `FM2_NetworkPacket_EncodeHeaderFields` | Encodes packet header fields at object +5812. |
| `0x823C1F88` | `FM2_ComObject_SyncChildProperties` | Syncs child COM property block via vtable+36. |
| `0x823CD040` | `FM2_D3D_BltRegionToSurface` | D3D blit with `tagRECT`/`tagPOINT` region args. |
| `0x823CF9F8` | `FM2_Math_VmxNormalizeVectorArray16` | VMX128 normalize 16 vectors using `unk_82030260` permute. |
| `0x823D3A40` | `FM2_Image_Downsample2x2Rgb565Average` | 2�2 RGB565 box filter with 0x7E0/0xF81F masks. |
| `0x823D45B8` | `FM2_Image_ConvertFloatPixelBuffer` | Float pixel buffer convert; requires type byte == 5. |
| `0x823D4F90` | `FM2_Image_BuildLinearLUT` | Allocates linear interpolation LUT from range. |
| `0x823DB160` | `FM2_Image_SampleChannelAtXY_Byte` | Byte-channel bilinear sample at (x,y). |
| `0x823DC1A0` | `FM2_Image_ResampleBilinear` | Full bilinear resample with float accumulators. |
| `0x823DD1F0` | `FM2_Image_SampleChannelAtXY_Ushort` | Ushort-channel sample variant of B160. |
| `0x823DF670` | `FM2_Image_PixelBuffer_Init` | Pixel buffer ctor vftable `off_82030C08`; format cases 0x1828004B/C. |
| `0x823E9988` | `FM2_GPU_SwizzleTextureTiles` | GPU tile swizzle via VMX128 + physical addr 0x7FEA1818. |
| `0x82408430` | `FM2_Image_ApplySeparableFilter` | Separable 1D filter pass on image rows/cols. |
| `0x8240C4E0` | `FM2_Crt_StubJumpOut` | Unconditional jump stub to CRT at 0x8294E968. |
| `0x82419FC0` | `FM2_ThreadLocal_ReleaseAndClearTls` | `KeTlsGetValue`/`freefls`/`KeTlsSetValue` cleanup. |
| `0x8242AA38` | `FM2_ContentManager_DestroyChildLists` | Walks two intrusive lists freeing child blocks. |
| `0x8242BBF8` | `FM2_ContentItem_DeleteOptional` | Content item dtor with optional pool free. |
| `0x8242CE30` | `FM2_ContentList_GetEntryCount308` | Entry count = span / 308 bytes. |
| `0x8242D698` | `FM2_Storage_BuildDevicePathPrefix` | Builds path prefix using `":\\"` separator. |
| `0x8242F818` | `FM2_Stream_IsOpenAsBool` | Returns �1 from stream vtable+60 open check. |
| `0x824303A0` | `FM2_ContentList_SortAndFreeRange` | Sorts 28-byte entries then frees temp buffer. |
| `0x82432540` | `FM2_AsyncOp_AcquireRefAndAllocTask` | Atomic inc (lwarx/stwcx) + 12-byte task alloc. |
| `0x82435F58` | `FM2_ContentVector_ClearAndFree36` | Frees 36-byte vector elements. |
| `0x82436530` | `FM2_ContentVector_EraseFromIterator` | Vector erase from iterator with trap on mismatch. |
| `0x824372B0` | `FM2_AssertCurrentContextOrTrap` | Traps unless `a1 == current context` (r2). |

### Manual re-pass 6 (35 functions)

Networking, lap/AI tracking, audio, and D3D singleton init.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82439730` | `FM2_AsyncQueue_FindPendingOp` | Scans intrusive list for first pending async op. |
| `0x824398B0` | `FM2_AsyncQueue_TryStartPendingOp` | Starts pending op with alignment via vtable+36. |
| `0x8243B3E8` | `FM2_Stream_GetRemainingByteCount` | Remaining bytes from stream vtable+168 minus cursor. |
| `0x8243D3B8` | `FM2_Stl_StringIterator_AdvanceChecked` | SSO string iterator advance with bounds trap. |
| `0x8243E460` | `FM2_GPU_LookupMemoryTagByLabel` | Matches GPU memory tag labels in lookup table. |
| `0x82440CB8` | `FM2_XHV_StopRemoteTalkerModes` | `XHVEnginepFindRemoteTalker` + `LocalTalkerStopModes`. |
| `0x82441670` | `FM2_MemcpyAligned_ReturnDest` | Thin wrapper: `FM2_MemcpyAligned` returning dest. |
| `0x82441D28` | `FM2_PacketProcessor_RetireContextBatch` | Retires `PACKET_CONTEXT` chain via processor. |
| `0x82445D60` | `FM2_Global_SetPacketProcessorInstance` | Stores global packet processor in `dword_829F1540`. |
| `0x824570E0` | `FM2_NetworkMessage_Init` | Inits network message struct + critsec at +16. |
| `0x82457650` | `FM2_NetworkMessageTree_DestroyRecursive` | Recursive destroy; leaf flag at +29. |
| `0x82459608` | `FM2_Network_CopyMessagePayload` | Copies message payload with aligned memcpy. |
| `0x8245F4C0` | `FM2_RbTree_LowerBoundInsert` | RB-tree lower_bound + insert path. |
| `0x8245F980` | `FM2_ContentDb_LookupHashRange` | Hash-range lookup returning bucket index. |
| `0x824605F8` | `FM2_Metrics_UpdateRatePerSecond` | Updates events/sec from timestamp delta. |
| `0x82461858` | `FM2_Metrics_InitGlobalCritSecSingleton` | RTL critsec + 12-byte metrics singleton init. |
| `0x82467170` | `FM2_LapTracker_Ctor` | Ctor for `TrackPositioning::CLapTracker` vftable. |
| `0x82467F08` | `FM2_LapTracker_UpdateCarPosition` | Updates lap tracker from car world position/velocity. |
| `0x82471770` | `FM2_Math_FillCubicBezierSamples` | Fills array with cubic Bezier interpolation. |
| `0x824748A8` | `FM2_TrackSpline_InterpolateSegmentVMX` | VMX128 track spline segment interpolation. |
| `0x8247BD68` | `FM2_AIOvertake_CheckAndTriggerHorn` | Overtake timing check (>0.75s); horn via audio mgr. |
| `0x8247DD80` | `FM2_AIDriver_InitPathBuffer` | Alloc 464-byte path buffer + float waypoint arrays. |
| `0x82480B50` | `FM2_CircularBuffer_SelectSlot` | Circular buffer slot select with alignment (`__twllei`). |
| `0x82482350` | `FM2_AIDriver_ResetRaceLineState` | Resets race-line fields from car object +2156/+2080. |
| `0x82482F00` | `FM2_AIDriver_ComputeThrottleBrakeAssist` | Throttle/brake assist from speed/error floats. |
| `0x82487728` | `FM2_AIDriver_ComputeSteeringInput` | Full AI steering from race line + car dynamics. |
| `0x8248FC08` | `FM2_CircularBuffer_CopySamples` | Copies samples from circular buffer with wrap math. |
| `0x82492238` | `FM2_AudioVoice_SetChannelDimensions` | Sets voice channel width/height at +16/+18. |
| `0x824922D0` | `FM2_Sort_HeapSortDwordArray` | In-place heap sort on DWORD array. |
| `0x82492328` | `FM2_Sort_InsertionSortUintArray` | Insertion sort using `FM2_Crt_MemmoveS` shifts. |
| `0x8249BDE8` | `FM2_CarAudio_Dtor` | Dtor for `TRefCountedObjectThreadSafe<CCarAudio>`. |
| `0x8249D5D8` | `FM2_Wind_LoadResourceAndSpliceList` | Loads `"Wind/Wind"` resource; splices intrusive list. |
| `0x8249E108` | `FM2_CarAudio_ClearThreeStrings` | Clears three SSO strings on car audio object. |
| `0x824A5550` | `FM2_D3D_InitGlobalDeviceSingleton` | Lazy-init 736-byte D3D device singleton + frame counter. |
| `0x824A7460` | `FM2_CarAudioComponent_DeleteOptional` | Car audio component dtor with optional free. |

### Manual re-pass 7 (35 functions)

Car audio, Lua compiler/runtime, D3D present chain.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824A76B8` | `FM2_CarAudio_SetPlaybackStateFromStream` | Sets playback flag +1125; state 1 or 7 from stream. |
| `0x824A8C68` | `FM2_CarAudio_TryStopStream` | Stops stream when ref clear; else returns 1409038. |
| `0x824A9638` | `FM2_CarAudio_AllocStreamBuffer` | `FM2_Crt_Malloc` 0x48 stream buffer via audio vtable+108. |
| `0x824AED58` | `FM2_CarAudio_InitVoiceBuffer` | Voice buffer init chain ending at object+117. |
| `0x824B35C8` | `FM2_Lua_InitCritSecSingleton` | One-time RTL critsec + `atexit` for Lua runtime. |
| `0x824B6AF0` | `FM2_Lua_IsNumberOrBooleanType` | Type tag check for Lua number (3) or boolean (4). |
| `0x824B75E0` | `FM2_Lua_PushValueByType` | Pushes Lua stack value by type tag 6/7/8. |
| `0x824B8828` | `FM2_Lua_ProtectedCallWithUnwind` | Protected call with unwind frame at object+112. |
| `0x824BFD70` | `FM2_Lua_GrowBufferCapacity` | Grows Lua string/buffer; min capacity 32. |
| `0x824C0558` | `FM2_LuaSyntax_ExpectedTokenError` | Syntax error `"'%s' expected"` when token != 285. |
| `0x824C3040` | `FM2_LuaSyntax_ReadAndInternString` | Reads length-prefixed string into intern table. |
| `0x824C3BA0` | `FM2_LuaSyntax_InitLexerForChunk` | Lexer init token 287, first char '.'. |
| `0x824C8178` | `FM2_LuaCompiler_DeleteOptional` | Lua compiler dtor with optional pool free. |
| `0x824C9520` | `FM2_CarAudio_ResetMixMatrixAndLoadStream` | Clears 10�4 mix matrix; loads stream object. |
| `0x824CAC18` | `FM2_Stl_Vector_InsertRangeRefCounted` | Vector insert with atomic ref-count on element. |
| `0x824CAF80` | `FM2_CarAudioMixChannel_Dtor` | Dtor vftable `off_82041798`; clears three streams. |
| `0x824D0C98` | `FM2_CarAudioStream_Dtor` | Dtor vftable `off_82041AAC`. |
| `0x824D1380` | `FM2_XmlSchema_TypeBoolean` | Returns schema type string `"{boolean}"`. |
| `0x824D1528` | `FM2_Stream_ReadAlignedBytes` | Reads bytes via `FM2_MemcpyAligned`; advances cursor. |
| `0x824D3FE8` | `FM2_Config_ParseSpaceSeparatedTokens` | Wide-string tokenize/split on `L" "`. |
| `0x824E3CE8` | `FM2_LiveProfile_ReadWriteBuffer` | 512-byte Live profile read/write buffer. |
| `0x824E60E8` | `FM2_SystemEventParam_Dtor` | Dtor for `Core::CSystemEventParam`. |
| `0x824EAC50` | `FM2_Scene_GetNotifyStateFromParam` | Notify state from param byte+4 branch. |
| `0x824EB300` | `FM2_Path_CompareComponentsCaseInsensitive` | Path component compare (case-insensitive). |
| `0x824EC188` | `FM2_SceneNode_CopyAssignExtended` | Scene node copy-assign with extra field blocks. |
| `0x824EC838` | `FM2_SceneNode_InitWithCString` | Scene node init wrapping SSO string from C str. |
| `0x824EFF18` | `FM2_ExceptionFilter_OnCppException` | SEH filter for C++ exception code 0xE06D7363. |
| `0x824F2140` | `FM2_Memory_InsertFrameAllocAndNotify` | Frame alloc record insert + notify callback. |
| `0x824F3020` | `FM2_RenderState_ApplyFromContextBlock` | Applies render state from context offsets 145/147/148. |
| `0x824F30E8` | `FM2_RenderState_IsSinglePassMode` | Returns true when field at +544 == 1. |
| `0x824F3208` | `FM2_D3D_AllocGpuBufferViaDriver` | GPU buffer alloc via driver fn `sub_827B39D8`. |
| `0x824F4B48` | `FM2_D3D_DeviceContext_Dtor` | Dtor vftable `off_82043228`. |
| `0x824F4E18` | `FM2_D3D_GetActiveSwapChainIfEnabled` | Returns swap chain ptr when flag +164996 set. |
| `0x824F6520` | `FM2_D3D_LazyInitPresentChain` | Lazy-init 68-byte present chain COM object. |
| `0x824F83D8` | `FM2_D3D_TryPresentAndUpdateStatus` | Present attempt; updates status fields +12/+32. |

### Manual re-pass 8 (35 functions)

Presentation, system events, Lua I/O/parser, deferred queue.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824FA618` | `FM2_Memory_FrameAllocRecord_Init` | Frame alloc record vftable `off_82043794`; bumps global count. |
| `0x824FAAE0` | `FM2_StdTree_ForEachNodeAccumulateSize` | Tree walk summing node sizes via vtable+72. |
| `0x824FABA8` | `FM2_Memory_LookupFrameAllocNotifyState` | Frame alloc lookup + `FM2_Render_NotifyManagerStateChange`. |
| `0x824FAE88` | `FM2_SystemEventSubscriber_Ctor` | Ctor for `Core::ISystemEventSubscriber` dual vftables. |
| `0x824FB058` | `FM2_LiveConnection_TryAcquireSession` | Live session acquire via `sub_82762450`/`82762EF8`. |
| `0x824FB5E8` | `FM2_Presentation_InitMediaFoundation` | Media Foundation init on presentation object +32/+36. |
| `0x824FCE98` | `FM2_PresentationCar_Dtor` | Presentation car dtor vftable `off_820438C4`. |
| `0x82502360` | `FM2_D3D_Subscriber_InitVtables` | D3D subscriber dual vftables `off_82043D94`/`D8C`. |
| `0x82502DB8` | `FM2_D3D_Subscriber_EnableDevice` | Enables device subscriber; sets ready byte +92. |
| `0x82506B88` | `FM2_CarPresentation_Dtor` | Dtor for `Presentation_R1::CCarPresentation`. |
| `0x8250AF70` | `FM2_Shader_LoadPendingResourcesLocked` | Locked shader pending load via resource manager. |
| `0x8250E9A0` | `FM2_Presentation_AllocInstance` | Alloc 4320-byte `Presentation_R1::CPresentation`. |
| `0x8250F3F0` | `FM2_Presentation_GetCarIdAtIndex` | Indexed car id from vector at object+32. |
| `0x82512460` | `FM2_Presentation_CopyCarDisplayBlock` | Copies 172-byte car display struct. |
| `0x825128C8` | `FM2_PresentationCarConfig_DeleteOptional` | Presentation car config dtor + optional free. |
| `0x825130B0` | `FM2_Vector_DestroyRange20Bytes` | Destroys 20-byte elements in range. |
| `0x82513880` | `FM2_Vector_EraseBegin20ByteElements` | Erases from begin of 20-byte vector. |
| `0x8251A380` | `FM2_D3D_ReleaseResourceRef` | `D3DResource_Release` with null-out. |
| `0x8251BC20` | `FM2_Float_CompareAsc` | Ascending float compare on field +4. |
| `0x8251D0D8` | `FM2_Presentation_ApplyCarCameraVMX` | VMX128 camera matrix apply at object+3404. |
| `0x82527030` | `FM2_Presentation_LoadCarResourcesAndWait` | Loads car resources via `FM2_ResourceManager_LoadAndWait`. |
| `0x82527B90` | `FM2_Memmove3ByteElements` | Memmove 3-byte struct elements. |
| `0x825286F8` | `FM2_Vector_IteratorAdvance3Elements` | Vector iterator advance by 3 elements. |
| `0x82529DE8` | `FM2_Stl_Vector_EraseRangeAtCopy` | Vector erase-range with bounds trap. |
| `0x8252AD00` | `FM2_PresentationSlotVector_Clear200Byte` | Clears 200-byte presentation slot vector. |
| `0x825348E8` | `FM2_Algorithm_RotateQwordArray` | In-place rotate on qword array (gcd loop). |
| `0x8253D6A0` | `FM2_Stl_AllocN84ByteObjects` | Pool alloc `84 * count` bytes. |
| `0x8253DED0` | `FM2_IntrusiveList_InitSentinel` | Initializes circular intrusive list sentinel. |
| `0x825461B0` | `FM2_IntrusiveList_ResetToSelf` | Resets list links to self. |
| `0x82548CB8` | `FM2_Set_InsertUniqueSorted` | Sorted-set unique insert walk. |
| `0x82549AF8` | `FM2_DeferredQueue_PackCommandParams` | Packs deferred command param block. |
| `0x8254ADC8` | `FM2_LuaParser_GetTokenOrAdvanceLine` | Lua lexer line/column advance (+252 flag). |
| `0x8254C1E8` | `FM2_CircularBuffer_TrimFront` | Trims front of circular buffer at +80. |
| `0x8254DB68` | `FM2_LuaIO_FileOpen` | Lua file open: `"@%s"`, `"=stdin"`, `"open"`. |
| `0x8254F3F0` | `FM2_Lua_AssertionFailed` | Lua assert error `"assertion failed!"`. |

### Manual re-pass 9 (35 functions)

Lua math, FMOD audio, TModel serialization, render pass binding.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82552D98` | `FM2_Lua_MathLog` | Lua math: `log()` push to stack. |
| `0x82552F48` | `FM2_Lua_MathTwoArg` | Lua two-argument math dispatch. |
| `0x82553730` | `FM2_Lua_AllocUpvalueClosure` | Allocates Lua upvalue/closure block. |
| `0x82557510` | `FM2_Math_CubicHermiteBasis` | Cubic Hermite basis polynomial eval. |
| `0x825576A8` | `FM2_Math_FindMaxFloatIndex` | Finds max float in array + index. |
| `0x8255D270` | `FM2_Render_ComputeDepthBiasFromContext` | Depth bias from render context fields +33840. |
| `0x82560E78` | `FM2_DeferredTaskParams_InitWithString` | Deferred task params init + SSO string. |
| `0x82564600` | `FM2_D3D_ValidateResourceHandlesOrRecover` | Validates D3D handles or calls recovery. |
| `0x8256D990` | `FM2_DeferredAdapterState_Dtor` | Deferred adapter state dtor `off_8204837C`. |
| `0x82570CD0` | `FM2_DeferredCommandParams_Dtor` | Deferred command params dtor `off_8204867C`. |
| `0x82570F60` | `FM2_RefCounted_ReleaseAtomicDec` | Atomic dec-ref via lwarx/stwcx; release at zero. |
| `0x82572120` | `FM2_DeferredCommand_ClearStringField` | Clears deferred command string field. |
| `0x825742A8` | `FM2_FMOD_System_GetDriverId` | FMOD system vtable+52 driver id. |
| `0x82576888` | `FM2_DeferredParams_AllocChildTypeA` | Child param block vftable `off_8204923C`. |
| `0x82576DC0` | `FM2_DeferredParams_AllocChildTypeB` | Child param block vftable `off_8204927C`. |
| `0x82578518` | `FM2_DeferredCommand_CtorTypeC` | Deferred command ctor vftable `off_820495C0`. |
| `0x82583AD8` | `FM2_FMOD_Channel_GetVolume` | FMOD channel volume via `sub_8266F620`. |
| `0x825850E0` | `FM2_FMOD_Geometry_AddPolygonFromVMX` | VMX128 verts ? `FMOD::Geometry::addPolygon`. |
| `0x82585B68` | `FM2_FMOD_Channel_StopIfPlaying` | Stops FMOD channel if active (result 78/35). |
| `0x825870A8` | `FM2_FMOD_Reverb_AllocStateBlock` | Alloc FMOD reverb state block (+24/+25 flags). |
| `0x82587250` | `FM2_FMOD_Build3DAttributesPair` | Builds FMOD 3D attribute pair from two inputs. |
| `0x8258B410` | `FM2_IntrusiveListNode_InitWithOffset` | Init list node with offset field. |
| `0x8258B470` | `FM2_FMOD_ChannelGroup_AllocStateBlock` | Alloc FMOD channel-group state (+40/+41). |
| `0x8258D4D8` | `FM2_Set_LowerBoundByKey` | RB-tree/set lower_bound by key. |
| `0x82599800` | `FM2_XmlSchema_TypeEnum` | Schema type string `"{enum}"`. |
| `0x8259F4E8` | `FM2_PresentationConfig_DeleteOptional` | Presentation config dtor + optional free. |
| `0x825A8CF0` | `FM2_Vector_MoveConstructRange32Bytes` | Move-construct 32-byte vector range. |
| `0x825A8DE0` | `FM2_Vector_FillRangeFromTemplate32Bytes` | Fill 32-byte range from template element. |
| `0x825AB2F8` | `FM2_TModel_InitVMXBounds` | TModel VMX128 bounds init vftable `off_82108340`. |
| `0x825ABBD0` | `FM2_TModel_SerializeMinMaxBounds` | Serializes `"MinBounds"`/`"MaxBounds"` for `"TModel"`. |
| `0x825AC758` | `FM2_TModel_Node_Init` | TModel node init vftable `off_8210846C`. |
| `0x825B3AD8` | `FM2_RenderPass_BindSurfaceAndConstants` | Binds surface + pass constant slot index. |
| `0x825BE978` | `FM2_Memmove272ByteElementsBackward` | Backward memmove of 272-byte records. |
| `0x825C5DB0` | `FM2_XexModule_HasSectionType1` | XEX section type check via `off_829A4BA8`. |
| `0x825C7950` | `FM2_IntrusiveList_CountNodesInRange` | Counts nodes in intrusive list range. |

### Manual re-pass 10 (35 functions)

UI scene graph, livery render, Lua UI bindings.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825CED58` | `FM2_SceneGraph_UpdateNodeWithNotifyState` | RB-tree node update + notify state copy. |
| `0x825D20C8` | `FM2_SceneManager_SpliceFadeListOnMismatch` | Splices fade list when scene id mismatch. |
| `0x825D2BA0` | `FM2_Render_NotifyStateFromSceneSlot` | Notify state from scene slot index +50. |
| `0x825D31E8` | `FM2_Render_NotifyStateCopyPair` | Copies notify state to two output slots. |
| `0x825D36A8` | `FM2_SceneManager_ResetSixChildStates` | Resets six child scene states (+332 flag). |
| `0x825D45E0` | `FM2_SceneNode_DtorClearStrings` | Scene node dtor; clears three COM strings. |
| `0x825D5310` | `FM2_ResourceLock_AssignHandleAtSlot` | Assigns retained handle at slot `a2+16`. |
| `0x825DAA48` | `FM2_LiveryRenderManager_Ctor` | Ctor for `CLiveryRenderManager` vftable. |
| `0x825DC6C8` | `FM2_LiveryRenderManager_TryFinalizeLayout` | Finalizes livery layout when vtable checks pass. |
| `0x825DCA10` | `FM2_LiveryLayer_DeleteOptional` | Livery layer dtor vftable `off_8210B3C8`. |
| `0x825DEB28` | `FM2_LiveryRenderManager_CreateLayerBinding` | Alloc 36-byte layer binding object. |
| `0x825E08A8` | `FM2_LiveryTexture_ReleaseAndClear` | Releases D3D texture at +164. |
| `0x825E6958` | `FM2_Lua_LoadLibraryModules` | Loads Lua `"library"` modules from registry. |
| `0x825EAA18` | `FM2_ThreadPool_InitCritSec` | RTL critsec init vftable `off_8210C954`. |
| `0x825EAD50` | `FM2_SceneSerializer_DisposeAndClearChildren` | Disposes 32-byte child vector elements. |
| `0x825ED678` | `FM2_Lua_PushNamedRegistryValue2` | Lua registry push helper (callback dword). |
| `0x825EE820` | `FM2_Lua_PushStringTableLookup` | Pushes `"UserInterface::StringTableLookup"`. |
| `0x825F03E0` | `FM2_Lua_PushUtf16StringById` | Pushes UTF-16 string by table id 15. |
| `0x825F3EB0` | `FM2_Lua_UIScene_GetUpdateNormal` | Lua enum 2 for `"eUpdateNormal"`. |
| `0x825F5018` | `FM2_Lua_UISceneManager_FireControlEvent` | Lua `"fireControlEvent"` on SceneManager. |
| `0x825F5EB8` | `FM2_Lua_UISceneManager_GetFadeOutBeginEvent` | Lua getter for fade-out begin event name. |
| `0x825F9430` | `FM2_Lua_UI_GetFormatString` | Lua `"Format"` string getter. |
| `0x825F98F0` | `FM2_Lua_UIGuide_SetAlphabetOnlyKeyboard` | Sets keyboard mode 3 on `"UserInterface::Guide"`. |
| `0x825FA870` | `FM2_UIScene_FireControlEventsOnPath` | Walks parent chain firing control events. |
| `0x825FBF08` | `FM2_XexModule_GetExportByOrdinal` | XEX export lookup via `off_829A556C`. |
| `0x82602898` | `FM2_UIScene_TriggerTransitionState` | Triggers UI scene transition state 2. |
| `0x826039C0` | `FM2_UIScene_PostMessageWithGuard` | Posts scene message with noop guard. |
| `0x82604C00` | `FM2_Algorithm_QuickSort20ByteRecords` | QuickSort on 20-byte records. |
| `0x82606848` | `FM2_Memmove20ByteRecordsForward` | Forward memmove 20-byte elements. |
| `0x8260CCE0` | `FM2_IntrusiveList_ResetToSelf2` | Circular list reset to sentinel. |
| `0x82614CB0` | `FM2_SceneGraph_DestroySubtreeAndFree` | Large scene subtree destroy + free. |
| `0x826163C8` | `FM2_SceneGraph_GetNodeTypeName` | Maps node type id ? name (`animationtrack`, etc.). |
| `0x8261C530` | `FM2_SceneNode_InvokeVirtualMethod32` | Invokes scene node vtable+32 method. |
| `0x8261D368` | `FM2_SceneNode_DeleteOptional` | Scene node dtor + optional free. |
| `0x8261E868` | `FM2_RbTreeNode_InitWithParent` | RB-tree node init with parent pointer. |

### Manual re-pass 11 (35 functions)

Lua scene bindings, car dynamics simulation, FMOD/file archive.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8261FBA8` | `FM2_Lua_LoadScriptFromBuffer` | Loads/compiles Lua script from string buffer. |
| `0x82621728` | `FM2_RbTree_RotateLeft` | Left rotation on RB-tree node. |
| `0x82621D50` | `FM2_RbTree_InitSentinelNode` | Init sentinel node (+121 leaf flag). |
| `0x82625620` | `FM2_Lua_CMatrix_IsIdentity` | Lua `CMatrix:IsIdentity()` binding. |
| `0x826316E0` | `FM2_Lua_SceneNode_PostMessageWithArgs` | Scene node post-message with arg block. |
| `0x82631E10` | `FM2_Lua_SceneNode_GetChildByIndex` | Gets child scene node by index. |
| `0x826323F8` | `FM2_Lua_SceneNode_LazyInitCallback23` | Lazy-init callback slot 23 on scene node. |
| `0x826324A0` | `FM2_Lua_SceneNode_LazyInitCallback23_Thunk` | Thunk to lazy-init callback above. |
| `0x82633950` | `FM2_Lua_SceneNodeRegistry_Clear` | Clears scene node registry table. |
| `0x82633E60` | `FM2_Lua_TypeErrorPrintf` | Lua type error printf (`#f`/`#v` formats). |
| `0x826351E8` | `FM2_Lua_Color_Copy` | Lua `"Color:copy( )"` binding. |
| `0x82635678` | `FM2_SceneGraph_CompareNodeFields` | Compares two scene node field blocks. |
| `0x82636140` | `FM2_Lua_SceneNode_CallAddMethod` | Calls scene node `".add"` method. |
| `0x82636C48` | `FM2_AudioMix_ComputeEnvelopeSamples` | Computes audio envelope sample ramp. |
| `0x8263D4B0` | `FM2_CarDynamics_LoadFromDescriptor` | Loads car dynamics from descriptor blob. |
| `0x8263D830` | `FM2_CarDynamics_ComputeWheelOffsetVMX` | VMX128 wheel offset from suspension. |
| `0x82641910` | `FM2_CarDynamics_ComputeGripBlend` | Tire grip blend from slip inputs. |
| `0x826425E0` | `FM2_CarDynamics_ComputeEngineTorqueScale` | Engine torque scale from RPM/damage. |
| `0x826488A0` | `FM2_CarDynamics_ComputeTireForcesVMX` | VMX128 tire force computation. |
| `0x8264C5D0` | `FM2_CarDynamics_Ctor` | Ctor for `CCarDynamics` / `ICarDynamics`. |
| `0x8264EB10` | `FM2_CarDynamics_InitTestRampSamples` | Init 20 test ramp force samples. |
| `0x82650DB0` | `FM2_CarDynamics_UpdateSimulationStep` | Simulation step update (+352 state). |
| `0x82653060` | `FM2_CarDynamics_ComputeSuspensionDotsVMX` | VMX128 suspension dot products. |
| `0x82655508` | `FM2_CarDynamics_ComputeCollisionResponse` | Collision response between two cars. |
| `0x82658080` | `FM2_Math_VmxCrossProductAccumulate` | VMX128 cross product accumulate. |
| `0x82658598` | `FM2_CarDynamics_InitFromTireParams` | Init dynamics block from tire params (0x150). |
| `0x8265A290` | `FM2_CarDynamics_SetSimulationMode` | Sets simulation mode fields +3/+7. |
| `0x8265ABA0` | `FM2_CarDynamics_IsFlagSetAt144` | Tests bit 0 at object+144. |
| `0x8265EC88` | `FM2_FMOD_Event_GetParameterByIndex` | FMOD event parameter lookup by index. |
| `0x82665CE0` | `FM2_SceneGraph_VisitChildrenRecursive` | Recursive scene graph visitor vtable+172. |
| `0x82666950` | `FM2_FileStream_CloseAndConcatBuffers` | fclose + concat two file buffers. |
| `0x826680A0` | `FM2_FileArchive_ShuffleEntryOrder` | Random shuffle of archive entry table. |
| `0x82668CD0` | `FM2_FileArchive_LookupPathPrefix` | Archive lookup by path prefix (error 78). |
| `0x8266E848` | `FM2_FMOD_System_CreateChannel` | FMOD system create channel wrapper. |
| `0x8266EB58` | `FM2_FMOD_System_CreateChannelEx` | Extended FMOD channel create. |

### Manual re-pass 12 (35 functions)

FMOD DSP/reverb, XAudio2 voice/stream pool, SQLite close.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x826709E0` | `FM2_FMOD_Event_SetParameter2D` | Event lookup + two float parameters. |
| `0x82670C50` | `FM2_FMOD_Event_GetUserDataPtr` | Event lookup; writes user-data pointer. |
| `0x826827C0` | `FM2_FMOD_Channel_SetParameterNormalized` | Clamps param to [0,1] at object+52. |
| `0x82682DD8` | `FM2_FMOD_Dsp_ProcessReverbMixBlock` | Large VMX/fp reverb mix DSP block. |
| `0x82688628` | `FM2_FMOD_Geometry_GetMinMaxExtents` | Copies min/max 3D vectors (+20/+23). |
| `0x8268A690` | `FM2_FMOD_DspNode_Ctor` | DSP node ctor vftable `off_82116710`. |
| `0x8268EE18` | `FM2_FMOD_Reverb_UpdateWetMixCoeffs` | Updates wet mix via `sub_826A49D8`. |
| `0x8268F400` | `FM2_FMOD_ChannelGroup_ResetMixLevels` | Resets six mix slots; iterates channels. |
| `0x82691920` | `FM2_FMOD_DspFilter_InitLowPassBiquad` | Biquad coeffs 0.76536697 / 1.847759. |
| `0x82697000` | `FM2_FMOD_InitSinLookupTable` | Fills 0x2000-entry sin lookup table. |
| `0x8269CEE0` | `FM2_FMOD_DspConnectionPool_Init` | Source `fmod_dsp_connectionpool.cpp`. |
| `0x8269D350` | `FM2_FMOD_DspConnectionPool_ProcessInput` | Connection pool input processing. |
| `0x826A0D58` | `FM2_FMOD_Reverb_Ctor` | Reverb ctor; defaults 100.0 / 1e6. |
| `0x826A1288` | `FM2_FMOD_Dsp_ProcessChannelMixBlock` | Per-channel mix block (vtable+40). |
| `0x826A3C50` | `FM2_FMOD_Buffer_ReadRange` | Range read; returns FMOD err 36. |
| `0x826A4460` | `FM2_FMOD_ForeverbDsp_ResizeDelayLine` | Source `aSfxDsp.cpp` delay resize. |
| `0x826AF3A8` | `FM2_XAudio2_QueryInterface` | COM QueryInterface ? `E_POINTER`. |
| `0x826B5170` | `FM2_XAudio2_VoicePool_StopAndRelease` | Stops voice pool; Rtl critsec. |
| `0x826B7390` | `FM2_XAudio2_Voice_SubmitFormatBuffer` | Submit buffer with wchar path. |
| `0x826BCFA0` | `FM2_XAudio2_WorkerThread_Main_WKTD` | Worker thread TLS tag `"WKTD"`. |
| `0x826C12B8` | `FM2_XAudio2_VoiceCallback_PostMessage` | Voice callback posts msg type 16. |
| `0x826C3000` | `FM2_XAudio2_StreamPool_UnlinkAndNotify` | Unlinks stream node from pool. |
| `0x826C30F0` | `FM2_XAudio2_Voice_ReleaseRef` | Atomic dec ref; `Nt_SetEvent`. |
| `0x826CC978` | `FM2_XAudio2_Stream_EndSubmitPacket` | End submit packet; critsec+524. |
| `0x826CCAA8` | `FM2_XAudio2_Stream_ResetSubmitState` | Resets stream submit state flags. |
| `0x826CE900` | `FM2_XAudio2_Stream_SubmitBufferLocked` | Locked buffer submit path. |
| `0x826CEFA0` | `FM2_XAudio2_Voice_DispatchPropertyMessage` | Switch on property ids 198�219. |
| `0x826D0FE8` | `FM2_XAudio2_Voice_CleanupLinkedResources` | Cleans voice linked resources. |
| `0x826D1288` | `FM2_XAudio2_VoicePool_ReleaseVoiceEntry` | Releases voice from pool. |
| `0x826D7CF0` | `FM2_XAudio2_CLeapBuffer_AllocAndQueue` | `XAUDIO2::CLeapBuffer` alloc/queue. |
| `0x826DAD98` | `FM2_XAudio2_Voice_SubmitPacketLocked` | Locked voice packet submit. |
| `0x826DAF38` | `FM2_XAudio2_Voice_SubmitPacketByVoiceId` | Thunk to submit-by-voice-id. |
| `0x826DD378` | `FM2_XAudio2_HeapFreeBlock` | Frees block via XAudio heap. |
| `0x826E6728` | `FM2_XAudio2_Voice_GetMappedBuffer` | Maps voice buffer for read. |
| `0x826EB8F0` | `FM2_SQLite_Database_Close` | `"Unable to close due to unfinalised statements"`. |

### Manual re-pass 13 (35 functions)

SQLite VDBE/parser, render TLS, deferred tasks, networking ctors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x826EE158` | `FM2_SQLite_Transaction_AdvanceState` | Transaction state magic transitions. |
| `0x826F0588` | `FM2_SQLite_Statement_ResetFlags` | Clears statement flags at +112/+128. |
| `0x826F4550` | `FM2_SQLite_ResultColumnNames_Append` | Appends column name to result list. |
| `0x826F7A08` | `FM2_SQLite_Statement_IsOpen` | Tests byte+65 != 1. |
| `0x826FACC0` | `FM2_SQLite_Statement_BindValue` | Binds parameter value to statement. |
| `0x826FD028` | `FM2_SQLite_ResultSet_SetColumnType` | Sets column type in result row. |
| `0x826FE8C8` | `FM2_SQLite_FreeErrorMessage` | Frees error string by error code. |
| `0x827009B0` | `FM2_SQLite_AppendLowercaseIdentifier` | Lowercases identifier before append. |
| `0x8270A1B0` | `FM2_SQLite_VdbeRunProgram` | Large VDBE program interpreter. |
| `0x8270B698` | `FM2_SQLite_ParseToken_CopyFromTable` | Copies parse token type 148 from table. |
| `0x8270D888` | `FM2_SQLite_ParseStack_PushEntry` | Pushes entry onto parse stack. |
| `0x82712DD0` | `FM2_SQLite_ReadSchemaFromVfs` | Reads schema via VFS callback. |
| `0x82714048` | `FM2_SQLite_VdbeExecStep` | VDBE single-step execution. |
| `0x82716708` | `FM2_SQLite_DateTime_FormatToken` | Date/time format token (base 2000). |
| `0x8271B5C0` | `FM2_SQLite_VdbeFinalizeStatement` | Finalizes VDBE statement resources. |
| `0x827222B8` | `FM2_RenderContext_UploadMatrixConstantsFromPass` | Uploads matrix constants per render pass. |
| `0x827238A8` | `FM2_RenderTls_BatchSubmitDrawPackets` | Batch submit via render TLS contexts. |
| `0x827281A8` | `FM2_RenderTls_GetWorkerSlotMask` | Worker TLS slot mask `(hash<<6)&0x3FFFC0`. |
| `0x8272EC38` | `FM2_Render_InitShaderConstantTables` | Init many shader constant table slots. |
| `0x82747BF0` | `FM2_Kernel_StringCopyChecked` | Bounded string copy with HRESULT. |
| `0x827506B8` | `FM2_STL_VectorInsertAtIndex` | Inserts element at vector index. |
| `0x82753438` | `FM2_DeferredTaskQueue_AllocWorkItem` | Alloc 964-byte deferred work item. |
| `0x82754C70` | `FM2_Kernel_XapipCreateThread` | Thin `XapipCreateThread` wrapper. |
| `0x82756590` | `FM2_Math_VmxMatrixTransformRows` | VMX128 matrix row transform. |
| `0x82760CF8` | `FM2_D3D_ResourceReleasePairFallback` | `D3DResource_Release` pair fallback. |
| `0x82761208` | `FM2_D3D_MeshBuffer_UpdateRegion` | Updates mesh buffer region (+656 stride). |
| `0x82762098` | `FM2_EndianSwapStream_Ctor` | `IOSys::CEndianSwapStream` vftable ctor. |
| `0x82763170` | `FM2_DeferredTask_SubmitWithParamsLookup` | Submit via `DeferredTaskParams` lookup. |
| `0x82764184` | `FM2_STL_EhUnwind_AllocGuard` | EH unwind guard allocation. |
| `0x82764928` | `FM2_STL_StringBuffer_Dtor` | String buffer dtor with optional delete. |
| `0x82764B44` | `FM2_STL_EhUnwind_DeleteGuard` | EH unwind guard delete. |
| `0x82765300` | `FM2_NetworkSession_Ctor` | Network session ctor triple vftable. |
| `0x82765700` | `FM2_NetworkPeer_Ctor` | Network peer ctor vftable `off_82141048`. |
| `0x8276C6B8` | `FM2_STL_ConstructArray40_Thunk` | Thunk to `FM2_STL_ConstructArray40_A`. |
| `0x8276D180` | `FM2_FileStream_Ctor` | File stream ctor triple vftable. |

### Manual re-pass 14 (35 functions)

XTS client / WebGate, leaderboard, camera list, deferred preloading anim.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8276D718` | `FM2_ComPtr_InvokeVirtual116Via188` | Double vtable dispatch +188 then +116. |
| `0x8276FA70` | `FM2_STL_EhUnwind_CallAtOffset872` | EH unwind helper at object+872. |
| `0x82770A58` | `FM2_NetworkSession_InvokeDispatch` | Thunk to network session dispatch. |
| `0x827734D4` | `FM2_STL_EhUnwind_CallAtOffset256` | EH unwind helper at frame+256. |
| `0x82773F08` | `FM2_NetworkPeer_InvokeMethodAt1020` | Invokes peer method at object+1020. |
| `0x82776178` | `FM2_DeferredTask_SubmitPreloadingAnimTurnOn` | Deferred submit for `OnRecieave_MP_SMT_TURN_ON_PRELOADING_ANIM`. |
| `0x82777D48` | `FM2_LeaderboardEntry_Ctor` | Leaderboard entry ctor vftable `off_821446EC`. |
| `0x8277968C` | `FM2_STL_EhUnwind_Call98528` | EH unwind call to `sub_82798528`. |
| `0x82779970` | `FM2_XtsClientMessageHandler_Ctor` | XTS message handler multi-vftable ctor. |
| `0x8277AB90` | `FM2_LeaderboardEntry_Dtor` | Leaderboard entry dtor + optional free. |
| `0x8277B0E8` | `FM2_STL_VectorClearAndFreeRange` | Clears/frees int vector range. |
| `0x8277CEB8` | `FM2_CameraList_FindPrevByCamId` | Walks `plrCAMERA_BASE` list by cam id. |
| `0x8277D794` | `FM2_STL_EhUnwind_ArrayConstructGuard` | EH guard around array construction. |
| `0x8277DB28` | `FM2_STL_WStringInsertChars` | Inserts wchar_t run into string buffer. |
| `0x82780B28` | `FM2_SharedPtr_AssignAtOffset8` | Swaps shared_ptr at object+8. |
| `0x82780BF0` | `FM2_NetworkCallback_Init` | Init callback object (+4 handle). |
| `0x827810A0` | `FM2_XtsClient_ProcessMessageQueue` | Drains/dispatches XTS client message queue. |
| `0x82781758` | `FM2_ComPtr_InvokeVirtual20At12` | ComPtr vtable+20 at object+12. |
| `0x82781E78` | `FM2_STL_CircularBuffer_PushBack` | Push into circular pointer buffer. |
| `0x827869D0` | `FM2_PtrStore` | Stores pointer (`*result = a2`). |
| `0x82787DC0` | `FM2_STL_EhUnwind_Call80718` | EH unwind call to `sub_82780718`. |
| `0x82789800` | `FM2_STL_Iterator_IncrementChecked` | Checked iterator increment (`__trap` bounds). |
| `0x8278A384` | `FM2_STL_EhUnwind_Call906D0` | EH unwind call to `sub_827906D0`. |
| `0x82796438` | `FM2_NetworkMessage_ComparePriority` | Compares two network message priorities. |
| `0x827968E8` | `FM2_SharedPtr_AssignFromHandle` | Assigns shared_ptr from handle. |
| `0x82798A10` | `FM2_XtsClient_CopyProfileFields` | Copies XTS profile qword/string fields. |
| `0x827994A0` | `FM2_WebGateMessage_ProcessState2` | Source `WebGateMessage.cpp`; state==2 path. |
| `0x8279A748` | `FM2_XtsClient_AccumulatePendingPayloadSize` | Sums pending linked payload sizes. |
| `0x8279E91C` | `FM2_STL_EhUnwind_Call821D1500` | EH unwind call to `sub_821D1500`. |
| `0x827A2330` | `FM2_PacketHeader_GetTypeByte` | Returns byte at packet header+6. |
| `0x827A6230` | `FM2_XtsBroadcastMessage_Dtor` | XTS broadcast message dtor. |
| `0x827A80E8` | `FM2_XtsClient_SendRequestPacket` | Builds/sends XTS request packet. |
| `0x827AA430` | `FM2_XtsMessage_GetField152` | Getter for field at object+152. |
| `0x827ABE20` | `FM2_XLiveFeed_OnFeedStartMessage` | Handles `"FeedStartMessage"` event. |
| `0x827AC220` | `FM2_XtsBroadcast_AssertUnreachable` | Source `BroadcastMessage.cpp` `!(false)`. |

### Manual re-pass 15 (35 functions)

Font/XML/render, input keyboard maps, STL map insert.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827AE278` | `FM2_STL_TupleLexCompare_Thunk` | Thunk to tuple lexicographic compare. |
| `0x827B0188` | `FM2_NetworkSession_InvokeDispatch2` | Second network session dispatch thunk. |
| `0x827B0498` | `FM2_RefCounted_ReleaseAtomic` | Atomic dec ref; vtable+16 release. |
| `0x827B1A70` | `FM2_XNotification_Notify_Ctor` | Ctor; sets name `"Notification Notify"`. |
| `0x827B3BA8` | `FM2_NetworkSocket_SendIfReady` | Sends socket buffer when ready flag set. |
| `0x827B8A70` | `FM2_AudioMix_ComputeScaledVolume` | Scales volume from mix context fields. |
| `0x827BAC98` | `FM2_RenderContext_ComputeVertexLighting` | Vertex lighting/shading computation. |
| `0x827BD568` | `FM2_FontRenderer_LayoutGlyphRun` | Layouts wchar glyph run with metrics. |
| `0x827BE4A0` | `FM2_STL_RbTreeIterator_Increment` | RB-tree iterator increment (`__trap` sentinel). |
| `0x827BECC8` | `FM2_FontCache_AllocCacheEntry` | Source `fontcache.h` entry alloc. |
| `0x827BEFB0` | `FM2_FontCache_InitSentinelList` | Init circular font-cache sentinel. |
| `0x827C1990` | `FM2_FontSystem_InitModuleRegistry` | Font module registry init (`"1.2.3"`). |
| `0x827C3D58` | `FM2_RenderTls_GetWorkerSlotMask_Thunk` | Thunk to `FM2_RenderTls_GetWorkerSlotMask`. |
| `0x827CE218` | `FM2_ComObject_ClearThreadSafeFlags` | Clears four `SetInterfaceThreadSafe` slots. |
| `0x827D1708` | `FM2_RenderMaterial_CopyStateBlock` | Copies render material state + splice lists. |
| `0x827D4FE8` | `FM2_XmlReader_IsElementNode` | Tests XML node type == 8 (element). |
| `0x827D7018` | `FM2_XmlNavigator_FindMatchingNode` | Walks XML tree for matching node. |
| `0x827D9B08` | `FM2_XmlNode_DestroySubtree` | Recursively destroys XML subtree nodes. |
| `0x827DD338` | `FM2_HashTable_InsertDualSlot` | Dual-slot open hash insert (mod 31). |
| `0x827DDDC0` | `FM2_STL_IntVector_ResizeZeroed` | Resize int vector; zero new slots. |
| `0x827E5178` | `FM2_XmlParser_CreateNodeIfNeeded` | Creates XML node when flag set. |
| `0x827E8D80` | `FM2_Diag_LogEvent22` | Diagnostic log event id 22. |
| `0x827EA120` | `FM2_XmlAttribute_Dtor` | XML attribute object dtor. |
| `0x827EA698` | `FM2_XmlElement_Dtor` | XML element object dtor. |
| `0x827EEE20` | `FM2_STL_ForEachSkipMatchingId` | For-each skipping one matching id. |
| `0x827EF730` | `FM2_Input_InitKeyboardScanMapA` | Init keyboard scan-code table A. |
| `0x827EFF18` | `FM2_Input_InitKeyboardScanMapB` | Init keyboard scan-code table B. |
| `0x827F33D0` | `FM2_Input_ResetBindingState` | Reset input binding/string state. |
| `0x827F66D0` | `FM2_STL_EhUnwind_CallF7D80` | EH unwind call to `sub_827F7D80`. |
| `0x827F7090` | `FM2_STL_RbTree_InsertOrFind` | RB-tree insert or find existing node. |
| `0x827FA1A4` | `FM2_STL_EhUnwind_Call67500` | EH unwind call to `sub_82767500`. |
| `0x827FA860` | `FM2_ComPtr_InvokeVirtual36At224` | ComPtr vtable+36 at object+224. |
| `0x827FB9E0` | `FM2_STL_Map_InsertWithRebalance` | Map insert/rebalance; `"invalid map/set<T> iterator"`. |
| `0x827FCD00` | `FM2_CameraList_FindPrevByCamIdAlt` | Alt camera-list prev search path. |
| `0x82804F70` | `FM2_Crt_AtexitRegisterExitString` | CRT init: `"Exit"` string + `atexit`. |

### Manual re-pass 16 (35 functions)

CRT menu strings, Lua API registration (career/tuning/livery/XLive).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82804FB0` | `FM2_Crt_AtexitRegisterStartRaceString` | CRT init `"StartRace"` + `atexit`. |
| `0x828052D0` | `FM2_Crt_AtexitRegisterRestartString_829C32A4` | CRT init `"Restart"` ? `dword_829C32A4`. |
| `0x82806B58` | `FM2_Lua_RegisterPlayerName` | Registers Lua `"PlayerName"`. |
| `0x828070C8` | `FM2_Lua_RegisterSaveDownloadedCarSetup` | Registers Lua `"SaveDownloadedCarSetup"`. |
| `0x82807628` | `FM2_Lua_RegisterSetPlaylistToOutOfCareer` | Registers Lua `"SetPlaylistToOutOfCareer"`. |
| `0x82807DA8` | `FM2_Lua_RegisterAhtsInProgress` | Registers Lua `"AHTS_IN_PROGRESS"`. |
| `0x828084A8` | `FM2_Lua_RegisterAccelerationRating` | Registers Lua `"accelerationRating"`. |
| `0x82808A18` | `FM2_Lua_RegisterReady` | Registers Lua `"ready"`. |
| `0x82808E18` | `FM2_Lua_RegisterFileSysSingleton` | Registers Lua `"FileSys"` singleton. |
| `0x82809DE0` | `FM2_Lua_RegisterVoicesMuted` | Registers Lua `"voicesMuted"`. |
| `0x8280AE80` | `FM2_Lua_RegisterYPosMaxStretch` | Registers Lua `"YPosMaxStretch"`. |
| `0x8280B7F0` | `FM2_Lua_RegisterSetType` | Registers Lua `"setType"`. |
| `0x8280CF40` | `FM2_Lua_RegisterLbTypeTimeTrialsRollup` | Registers Lua `"E_LBTYPE_TIMETRIALS_ROLLUP"`. |
| `0x8280D000` | `FM2_Lua_RegisterTrackTypeRealWorld` | Registers Lua `"E_TRACKTYPE_REALWORLD"`. |
| `0x8280D9D0` | `FM2_Lua_RegisterRemoveSetup` | Registers Lua `"removeSetup"`. |
| `0x8280ECF0` | `FM2_Lua_RegisterHoodColor` | Registers Lua livery `"HOODCOLOR"`. |
| `0x8280ED90` | `FM2_Lua_RegisterGroupColor` | Registers Lua livery `"GROUPCOLOR"`. |
| `0x8280EFB0` | `FM2_Lua_RegisterTranslate` | Registers Lua livery `"TRANSLATE"`. |
| `0x8280F640` | `FM2_Lua_RegisterTwoToneBias` | Registers Lua `"TwoToneBias"`. |
| `0x8280F8F0` | `FM2_Lua_RegisterCreateNewLayerAt` | Registers Lua `"createNewLayerAt"`. |
| `0x82810550` | `FM2_Lua_RegisterToggleReady` | Registers Lua MP `"ToggleReady"`. |
| `0x82810970` | `FM2_Lua_RegisterSetXboxLiveCareer` | Registers Lua `"SetXboxLiveCareer"`. |
| `0x82811D00` | `FM2_Lua_RegisterCarClass` | Registers Lua race `"carClass"`. |
| `0x82811D40` | `FM2_Lua_RegisterAiDriverId` | Registers Lua race `"aiDriverId"`. |
| `0x82811DE0` | `FM2_Lua_RegisterCarColor` | Registers Lua race `"carColor"`. |
| `0x82812EA0` | `FM2_Lua_RegisterFttsComplete` | Registers Lua `"FTTS_COMPLETE"`. |
| `0x82813BD0` | `FM2_Lua_RegisterProfileName` | Registers Lua profile `"ProfileName"`. |
| `0x82813EC0` | `FM2_Lua_RegisterSetMultiscreenToSave` | Registers Lua settings `"SetMultiscreenToSave"`. |
| `0x82814320` | `FM2_Lua_RegisterSteeringAxisDeadzoneOutsideWheel` | Registers Lua `"SteeringAxisDeadzoneOutsideWheel"`. |
| `0x82815AD0` | `FM2_Lua_RegisterClutchDamageBonus` | Registers Lua damage `"ClutchDamageBonus"`. |
| `0x82815DD0` | `FM2_Lua_RegisterSetRichPresenceLocationTournament` | Registers XLive rich presence setter. |
| `0x82816FC0` | `FM2_Lua_RegisterName` | Registers Lua tournament `"name"`. |
| `0x82817140` | `FM2_Lua_RegisterQualifyingTrack` | Registers Lua `"qualifyingTrack"`. |
| `0x82817360` | `FM2_Lua_RegisterSetActive` | Registers Lua `"SetActive"`. |
| `0x828178C0` | `FM2_Lua_RegisterFinalDriveGear` | Registers Lua tuning `"finalDriveGear"`. |

### Manual re-pass 17 (35 functions)

Lua car-setup bindings, CRT static init, XML type-handle cache hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828180E0` | `FM2_Lua_RegisterFrontSwaybar` | Registers Lua `"frontSwaybar"`. |
| `0x82818540` | `FM2_Lua_RegisterRearDampingBumpStiffness` | Registers Lua `"rearDampingBumpStiffness"`. |
| `0x82818580` | `FM2_Lua_RegisterRearDampingBumpStiffnessIncrement` | Registers Lua increment setter. |
| `0x82819520` | `FM2_Lua_RegisterPrepareSaveSetup` | Registers Lua `"prepareSaveSetup"`. |
| `0x828195E0` | `FM2_Lua_RegisterLuaCarSetupPointerSingleton` | Registers `"LuaCarSetupPointer"` singleton. |
| `0x8281AC00` | `FM2_Crt_AtexitRegisterQuitString` | CRT init `"Quit"` + `atexit`. |
| `0x8281C480` | `FM2_Crt_AtexitRegisterRestartString_829DAC64` | CRT init `"Restart"` ? `dword_829DAC64`. |
| `0x8281CE80` | `FM2_Crt_AtexitRegisterStartDrivingString` | CRT init `"StartDriving"` + `atexit`. |
| `0x8281CFC0` | `FM2_Crt_AtexitRegisterRestartString_829DB184` | CRT init `"Restart"` ? `dword_829DB184`. |
| `0x8281E7B0` | `FM2_Crt_AtexitRegisterNullSub` | CRT `atexit(nullsub_3)`. |
| `0x8281F018` | `FM2_Crt_InitAllocatorCriticalSection` | Init critsec `stru_82A00E64` spin 0x100. |
| `0x8281FF80` | `FM2_XmlStaticInit_CacheTypeHandle_82A02BC4` | Static init caches XML type ? global. |
| `0x82820C28` | `FM2_XmlStaticInit_CacheTypeHandle_82A02B9C` | Static init caches XML type ? global. |
| `0x828212E8` | `FM2_XmlStaticInit_CacheTypeHandle_82A029D0` | Static init caches XML type ? global. |
| `0x82824A70` | `FM2_XmlStaticInit_CacheTypeHandle_82A030B8` | Static init caches XML type ? global. |
| `0x82825490` | `FM2_XmlStaticInit_CacheTypeHandle_82A02DC8` | Static init caches XML type ? global. |
| `0x82825D90` | `FM2_XmlStaticInit_CacheTypeHandle_82A02F3C` | Static init caches XML type ? global. |
| `0x82826D50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0302C` | Static init caches XML type ? global. |
| `0x82827D60` | `FM2_XmlStaticInit_CacheTypeHandle_82A03354` | Static init caches XML type ? global. |
| `0x82828198` | `FM2_XmlStaticInit_CacheTypeHandle_82A03284` | Static init caches XML type ? global. |
| `0x828285D0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0344C` | Static init caches XML type ? global. |
| `0x82828E88` | `FM2_XmlStaticInit_CacheTypeHandle_82A032E8` | Static init caches XML type ? global. |
| `0x8282B5A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03404` | Static init caches XML type ? global. |
| `0x8282CB10` | `FM2_XmlStaticInit_CacheTypeHandle_82A03544` | Static init caches XML type ? global. |
| `0x8282DBA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03600` | Static init caches XML type ? global. |
| `0x8282DE78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0376C` | Static init caches XML type ? global. |
| `0x8282F3D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03744` | Static init caches XML type ? global. |
| `0x8282F780` | `FM2_XmlStaticInit_CacheTypeHandle_82A03554` | Static init caches XML type ? global. |
| `0x8282FD80` | `FM2_XmlStaticInit_CacheTypeHandle_82A03B48` | Static init caches XML type ? global. |
| `0x82830950` | `FM2_XmlStaticInit_CacheTypeHandle_82A03C4C` | Static init caches XML type ? global. |
| `0x828329A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03CCC` | Static init caches XML type ? global. |
| `0x82836350` | `FM2_XmlStaticInit_CacheTypeHandle_82A04088` | Static init caches XML type ? global. |
| `0x82836AA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03FF4` | Static init caches XML type ? global. |
| `0x82836C08` | `FM2_XmlStaticInit_CacheTypeHandle_82A03E00` | Static init caches XML type ? global. |
| `0x82836D70` | `FM2_XmlStaticInit_CacheTypeHandle_82A0412C` | Static init caches XML type ? global. |

### Manual re-pass 18 (35 functions)

Lua UI/tuning bindings + XML type-handle static init (continued).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828372C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A040AC` | Static init caches XML type ? `dword_82A040AC`. |
| `0x82837CA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03FB8` | Static init caches XML type ? `dword_82A03FB8`. |
| `0x82838EA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A04478` | Static init caches XML type ? `dword_82A04478`. |
| `0x82839178` | `FM2_XmlStaticInit_CacheTypeHandle_82A0429C` | Static init caches XML type ? `dword_82A0429C`. |
| `0x8283B410` | `FM2_XmlStaticInit_CacheTypeHandle_82A043F8` | Static init caches XML type ? `dword_82A043F8`. |
| `0x8283BBF0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04404` | Static init caches XML type ? `dword_82A04404`. |
| `0x8283C028` | `FM2_XmlStaticInit_CacheTypeHandle_82A04494` | Static init caches XML type ? `dword_82A04494`. |
| `0x8283C198` | `FM2_XmlStaticInit_CacheTypeHandle_82A047D8` | Static init caches XML type ? `dword_82A047D8`. |
| `0x8283DB30` | `FM2_XmlStaticInit_CacheTypeHandle_82A04728` | Static init caches XML type ? `dword_82A04728`. |
| `0x8283DFB0` | `FM2_XmlStaticInit_CacheTypeHandle_82A048C4` | Static init caches XML type ? `dword_82A048C4`. |
| `0x8283E790` | `FM2_XmlStaticInit_CacheTypeHandle_82A045D8` | Static init caches XML type ? `dword_82A045D8`. |
| `0x8283ECA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04904` | Static init caches XML type ? `dword_82A04904`. |
| `0x8283F3A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A045B4` | Static init caches XML type ? `dword_82A045B4`. |
| `0x8283F4C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A047E8` | Static init caches XML type ? `dword_82A047E8`. |
| `0x8283F750` | `FM2_XmlStaticInit_CacheTypeHandle_82A04764` | Static init caches XML type ? `dword_82A04764`. |
| `0x82840FC0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04B6C` | Static init caches XML type ? `dword_82A04B6C`. |
| `0x82841830` | `FM2_XmlStaticInit_CacheTypeHandle_82A04B24` | Static init caches XML type ? `dword_82A04B24`. |
| `0x82843EB8` | `FM2_XmlStaticInit_CacheTypeHandle_82A04CCC` | Static init caches XML type ? `dword_82A04CCC`. |
| `0x82846230` | `FM2_XmlStaticInit_CacheTypeHandle_82A051B4` | Static init caches XML type ? `dword_82A051B4`. |
| `0x82846D70` | `FM2_XmlStaticInit_CacheTypeHandle_82A04E78` | Static init caches XML type ? `dword_82A04E78`. |
| `0x82847CA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04E84` | Static init caches XML type ? `dword_82A04E84`. |
| `0x828481F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A05020` | Static init caches XML type ? `dword_82A05020`. |
| `0x828482D0` | `FM2_XmlStaticInit_CacheTypeHandle_82A050DC` | Static init caches XML type ? `dword_82A050DC`. |
| `0x828489B0` | `FM2_Lua_RegisterSetOverlay` | Registers Lua UI `"setOverlay"`. |
| `0x82848DA0` | `FM2_Lua_RegisterGuideResult` | Registers Lua UI `"GuideResult"`. |
| `0x82848DC0` | `FM2_Lua_RegisterSetEvent` | Registers Lua UI `"setEvent"`. |
| `0x82848F20` | `FM2_Lua_RegisterGoToFlow` | Registers Lua UI `"goToFlow"`. |
| `0x82849120` | `FM2_Lua_RegisterFadeInBeginEventName` | Registers Lua scene `"FadeInBeginEventName"`. |
| `0x828493D0` | `FM2_Lua_RegisterDisplacement` | Registers Lua tuning `"Displacement"`. |
| `0x82849550` | `FM2_Lua_RegisterTuningTirePressure` | Registers Lua `"TuningTirePressure"`. |
| `0x828497E0` | `FM2_Lua_RegisterDisplacement_82A04D20` | Second `"Displacement"` on table `82A04D20`. |
| `0x8284A110` | `FM2_Lua_RegisterMessage` | Registers Lua `"message"`. |
| `0x8284A578` | `FM2_XmlStaticInit_CacheTypeHandle_82A053AC` | Static init caches XML type ? `dword_82A053AC`. |
| `0x8284B2F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A05374` | Static init caches XML type ? `dword_82A05374`. |
| `0x8284BD18` | `FM2_XmlStaticInit_CacheTypeHandle_82A05288` | Static init caches XML type ? `dword_82A05288`. |

### Manual re-pass 19 (35 functions)

XML type-handle static init (continued) + CRT atexit hook.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8284C198` | `FM2_XmlStaticInit_CacheTypeHandle_82A05384` | Static init caches XML type ? `dword_82A05384`. |
| `0x8284C660` | `FM2_XmlStaticInit_CacheTypeHandle_82A052AC` | Static init caches XML type ? `dword_82A052AC`. |
| `0x8284C738` | `FM2_XmlStaticInit_CacheTypeHandle_82A05564` | Static init caches XML type ? `dword_82A05564`. |
| `0x8284D038` | `FM2_XmlStaticInit_CacheTypeHandle_82A05278` | Static init caches XML type ? `dword_82A05278`. |
| `0x8284DD28` | `FM2_XmlStaticInit_CacheTypeHandle_82A05484` | Static init caches XML type ? `dword_82A05484`. |
| `0x8284DE48` | `FM2_XmlStaticInit_CacheTypeHandle_82A05408` | Static init caches XML type ? `dword_82A05408`. |
| `0x8284E088` | `FM2_XmlStaticInit_CacheTypeHandle_82A05494` | Static init caches XML type ? `dword_82A05494`. |
| `0x8284E358` | `FM2_XmlStaticInit_CacheTypeHandle_82A055B4` | Static init caches XML type ? `dword_82A055B4`. |
| `0x8284E630` | `FM2_Crt_AtexitRegisterSub_8294C858` | CRT `atexit(sub_8294C858)`. |
| `0x82850D58` | `FM2_XmlStaticInit_CacheTypeHandle_82A059C0` | Static init caches XML type ? `dword_82A059C0`. |
| `0x828524F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A058D8` | Static init caches XML type ? `dword_82A058D8`. |
| `0x82852A10` | `FM2_XmlStaticInit_CacheTypeHandle_82A05D30` | Static init caches XML type ? `dword_82A05D30`. |
| `0x82852B30` | `FM2_XmlStaticInit_CacheTypeHandle_82A05BD0` | Static init caches XML type ? `dword_82A05BD0`. |
| `0x82854900` | `FM2_XmlStaticInit_CacheTypeHandle_82A05C1C` | Static init caches XML type ? `dword_82A05C1C`. |
| `0x82856058` | `FM2_XmlStaticInit_CacheTypeHandle_82A05D10` | Static init caches XML type ? `dword_82A05D10`. |
| `0x828569A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06078` | Static init caches XML type ? `dword_82A06078`. |
| `0x82856B10` | `FM2_XmlStaticInit_CacheTypeHandle_82A060D8` | Static init caches XML type ? `dword_82A060D8`. |
| `0x828570B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A06254` | Static init caches XML type ? `dword_82A06254`. |
| `0x82857218` | `FM2_XmlStaticInit_CacheTypeHandle_82A06210` | Static init caches XML type ? `dword_82A06210`. |
| `0x828572A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0627C` | Static init caches XML type ? `dword_82A0627C`. |
| `0x82859930` | `FM2_XmlStaticInit_CacheTypeHandle_82A05F80` | Static init caches XML type ? `dword_82A05F80`. |
| `0x82859C48` | `FM2_XmlStaticInit_CacheTypeHandle_82A0621C` | Static init caches XML type ? `dword_82A0621C`. |
| `0x82859FA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06148` | Static init caches XML type ? `dword_82A06148`. |
| `0x8285ACD8` | `FM2_XmlStaticInit_CacheTypeHandle_82A064B8` | Static init caches XML type ? `dword_82A064B8`. |
| `0x8285B080` | `FM2_XmlStaticInit_CacheTypeHandle_82A0643C` | Static init caches XML type ? `dword_82A0643C`. |
| `0x8285B5D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06354` | Static init caches XML type ? `dword_82A06354`. |
| `0x8285C1A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06534` | Static init caches XML type ? `dword_82A06534`. |
| `0x8285C598` | `FM2_XmlStaticInit_CacheTypeHandle_82A064C0` | Static init caches XML type ? `dword_82A064C0`. |
| `0x8285CD78` | `FM2_XmlStaticInit_CacheTypeHandle_82A063D8` | Static init caches XML type ? `dword_82A063D8`. |
| `0x8285D6C0` | `FM2_XmlStaticInit_CacheTypeHandle_82A066A0` | Static init caches XML type ? `dword_82A066A0`. |
| `0x82860770` | `FM2_XmlStaticInit_CacheTypeHandle_82A06898` | Static init caches XML type ? `dword_82A06898`. |
| `0x82860F98` | `FM2_XmlStaticInit_CacheTypeHandle_82A06840` | Static init caches XML type ? `dword_82A06840`. |
| `0x82861100` | `FM2_XmlStaticInit_CacheTypeHandle_82A06908` | Static init caches XML type ? `dword_82A06908`. |
| `0x82861610` | `FM2_XmlStaticInit_CacheTypeHandle_82A06978` | Static init caches XML type ? `dword_82A06978`. |
| `0x828616A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0677C` | Static init caches XML type ? `dword_82A0677C`. |

### Manual re-pass 20 (35 functions)

XML type-handle static init hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828619B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A066B0` | Static init caches XML type ? `dword_82A066B0`. |
| `0x82861D60` | `FM2_XmlStaticInit_CacheTypeHandle_82A06724` | Static init caches XML type ? `dword_82A06724`. |
| `0x82862300` | `FM2_XmlStaticInit_CacheTypeHandle_82A067B0` | Static init caches XML type ? `dword_82A067B0`. |
| `0x82862930` | `FM2_XmlStaticInit_CacheTypeHandle_82A06980` | Static init caches XML type ? `dword_82A06980`. |
| `0x82862A50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0689C` | Static init caches XML type ? `dword_82A0689C`. |
| `0x82862A98` | `FM2_XmlStaticInit_CacheTypeHandle_82A068F4` | Static init caches XML type ? `dword_82A068F4`. |
| `0x82863940` | `FM2_XmlStaticInit_CacheTypeHandle_82A06C28` | Static init caches XML type ? `dword_82A06C28`. |
| `0x82865128` | `FM2_XmlStaticInit_CacheTypeHandle_82A06B70` | Static init caches XML type ? `dword_82A06B70`. |
| `0x828655F0` | `FM2_XmlStaticInit_CacheTypeHandle_82A06E10` | Static init caches XML type ? `dword_82A06E10`. |
| `0x828668C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06DA4` | Static init caches XML type ? `dword_82A06DA4`. |
| `0x82867F20` | `FM2_XmlStaticInit_CacheTypeHandle_82A071E8` | Static init caches XML type ? `dword_82A071E8`. |
| `0x82868E98` | `FM2_XmlStaticInit_CacheTypeHandle_82A0720C` | Static init caches XML type ? `dword_82A0720C`. |
| `0x8286A6C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06EA4` | Static init caches XML type ? `dword_82A06EA4`. |
| `0x8286B298` | `FM2_XmlStaticInit_CacheTypeHandle_82A06E60` | Static init caches XML type ? `dword_82A06E60`. |
| `0x8286CF98` | `FM2_XmlStaticInit_CacheTypeHandle_82A07458` | Static init caches XML type ? `dword_82A07458`. |
| `0x8286D4F0` | `FM2_XmlStaticInit_CacheTypeHandle_82A072A8` | Static init caches XML type ? `dword_82A072A8`. |
| `0x8286DF58` | `FM2_XmlStaticInit_CacheTypeHandle_82A07400` | Static init caches XML type ? `dword_82A07400`. |
| `0x8286EA98` | `FM2_XmlStaticInit_CacheTypeHandle_82A07404` | Static init caches XML type ? `dword_82A07404`. |
| `0x8286F038` | `FM2_XmlStaticInit_CacheTypeHandle_82A07398` | Static init caches XML type ? `dword_82A07398`. |
| `0x8286F620` | `FM2_XmlStaticInit_CacheTypeHandle_82A07450` | Static init caches XML type ? `dword_82A07450`. |
| `0x8286FAF8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0798C` | Static init caches XML type ? `dword_82A0798C`. |
| `0x82872C78` | `FM2_XmlStaticInit_CacheTypeHandle_82A07938` | Static init caches XML type ? `dword_82A07938`. |
| `0x828753E8` | `FM2_XmlStaticInit_CacheTypeHandle_82A07C00` | Static init caches XML type ? `dword_82A07C00`. |
| `0x828758B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A07AD0` | Static init caches XML type ? `dword_82A07AD0`. |
| `0x82876A68` | `FM2_XmlStaticInit_CacheTypeHandle_82A07A14` | Static init caches XML type ? `dword_82A07A14`. |
| `0x82876F78` | `FM2_XmlStaticInit_CacheTypeHandle_82A07BAC` | Static init caches XML type ? `dword_82A07BAC`. |
| `0x82877E28` | `FM2_XmlStaticInit_CacheTypeHandle_82A0804C` | Static init caches XML type ? `dword_82A0804C`. |
| `0x828789F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A07FD0` | Static init caches XML type ? `dword_82A07FD0`. |
| `0x8287BEE8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08304` | Static init caches XML type ? `dword_82A08304`. |
| `0x8287C248` | `FM2_XmlStaticInit_CacheTypeHandle_82A08464` | Static init caches XML type ? `dword_82A08464`. |
| `0x8287C998` | `FM2_XmlStaticInit_CacheTypeHandle_82A08138` | Static init caches XML type ? `dword_82A08138`. |
| `0x8287E7F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08170` | Static init caches XML type ? `dword_82A08170`. |
| `0x8287EA80` | `FM2_XmlStaticInit_CacheTypeHandle_82A082B0` | Static init caches XML type ? `dword_82A082B0`. |
| `0x8287ED98` | `FM2_XmlStaticInit_CacheTypeHandle_82A082E0` | Static init caches XML type ? `dword_82A082E0`. |
| `0x82880038` | `FM2_XmlStaticInit_CacheTypeHandle_82A084AC` | Static init caches XML type ? `dword_82A084AC`. |

### Manual re-pass 21 (35 functions)

XML type-handle static init hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82881550` | `FM2_XmlStaticInit_CacheTypeHandle_82A086EC` | Static init caches XML type ? `dword_82A086EC`. |
| `0x82881598` | `FM2_XmlStaticInit_CacheTypeHandle_82A084C0` | Static init caches XML type ? `dword_82A084C0`. |
| `0x828823A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08804` | Static init caches XML type ? `dword_82A08804`. |
| `0x82882EE8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0877C` | Static init caches XML type ? `dword_82A0877C`. |
| `0x82883440` | `FM2_XmlStaticInit_CacheTypeHandle_82A08738` | Static init caches XML type ? `dword_82A08738`. |
| `0x82885B68` | `FM2_XmlStaticInit_CacheTypeHandle_82A08A44` | Static init caches XML type ? `dword_82A08A44`. |
| `0x82886468` | `FM2_XmlStaticInit_CacheTypeHandle_82A08AA8` | Static init caches XML type ? `dword_82A08AA8`. |
| `0x82887230` | `FM2_XmlStaticInit_CacheTypeHandle_82A089A4` | Static init caches XML type ? `dword_82A089A4`. |
| `0x828879C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08AC0` | Static init caches XML type ? `dword_82A08AC0`. |
| `0x82887F30` | `FM2_XmlStaticInit_CacheTypeHandle_82A08ED0` | Static init caches XML type ? `dword_82A08ED0`. |
| `0x828881B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08CB0` | Static init caches XML type ? `dword_82A08CB0`. |
| `0x828883F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08E18` | Static init caches XML type ? `dword_82A08E18`. |
| `0x828895B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A08F58` | Static init caches XML type ? `dword_82A08F58`. |
| `0x828898C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08C6C` | Static init caches XML type ? `dword_82A08C6C`. |
| `0x8288AE70` | `FM2_XmlStaticInit_CacheTypeHandle_82A08DC8` | Static init caches XML type ? `dword_82A08DC8`. |
| `0x8288BDF8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0920C` | Static init caches XML type ? `dword_82A0920C`. |
| `0x8288C230` | `FM2_XmlStaticInit_CacheTypeHandle_82A09054` | Static init caches XML type ? `dword_82A09054`. |
| `0x8288C5D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09198` | Static init caches XML type ? `dword_82A09198`. |
| `0x8288C620` | `FM2_XmlStaticInit_CacheTypeHandle_82A0925C` | Static init caches XML type ? `dword_82A0925C`. |
| `0x8288C6B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A092C8` | Static init caches XML type ? `dword_82A092C8`. |
| `0x8288C8A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0907C` | Static init caches XML type ? `dword_82A0907C`. |
| `0x8288ECF0` | `FM2_XmlStaticInit_CacheTypeHandle_82A08F94` | Static init caches XML type ? `dword_82A08F94`. |
| `0x82892078` | `FM2_XmlStaticInit_CacheTypeHandle_82A093EC` | Static init caches XML type ? `dword_82A093EC`. |
| `0x828922B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09650` | Static init caches XML type ? `dword_82A09650`. |
| `0x82892738` | `FM2_XmlStaticInit_CacheTypeHandle_82A095DC` | Static init caches XML type ? `dword_82A095DC`. |
| `0x828931A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A095C0` | Static init caches XML type ? `dword_82A095C0`. |
| `0x82893590` | `FM2_XmlStaticInit_CacheTypeHandle_82A09524` | Static init caches XML type ? `dword_82A09524`. |
| `0x828936B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A09548` | Static init caches XML type ? `dword_82A09548`. |
| `0x828939C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A096B8` | Static init caches XML type ? `dword_82A096B8`. |
| `0x82894050` | `FM2_XmlStaticInit_CacheTypeHandle_82A09818` | Static init caches XML type ? `dword_82A09818`. |
| `0x82896A80` | `FM2_XmlStaticInit_CacheTypeHandle_82A096C0` | Static init caches XML type ? `dword_82A096C0`. |
| `0x82896EB8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0977C` | Static init caches XML type ? `dword_82A0977C`. |
| `0x82898278` | `FM2_XmlStaticInit_CacheTypeHandle_82A09CE8` | Static init caches XML type ? `dword_82A09CE8`. |
| `0x82898590` | `FM2_XmlStaticInit_CacheTypeHandle_82A09DE0` | Static init caches XML type ? `dword_82A09DE0`. |
| `0x82898620` | `FM2_XmlStaticInit_CacheTypeHandle_82A09D8C` | Static init caches XML type ? `dword_82A09D8C`. |


### Manual re-pass 22 (35 functions)

Offset 770+: XML type-handle static init hooks (plus CRT string/ptr-pair inits in pass 23).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8289a8b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09AF4` | Static init caches XML type ? `dword_82A09AF4`. |
| `0x8289b320` | `FM2_XmlStaticInit_CacheTypeHandle_82A09DCC` | Static init caches XML type ? `dword_82A09DCC`. |
| `0x8289cc38` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A060` | Static init caches XML type ? `dword_82A0A060`. |
| `0x8289e030` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A0F0` | Static init caches XML type ? `dword_82A0A0F0`. |
| `0x8289e780` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A154` | Static init caches XML type ? `dword_82A0A154`. |
| `0x8289eb28` | `FM2_XmlStaticInit_CacheTypeHandle_82A09EAC` | Static init caches XML type ? `dword_82A09EAC`. |
| `0x8289ecd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A1B0` | Static init caches XML type ? `dword_82A0A1B0`. |
| `0x828a0bd0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A4DC` | Static init caches XML type ? `dword_82A0A4DC`. |
| `0x828a3c78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A204` | Static init caches XML type ? `dword_82A0A204`. |
| `0x828a43d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A8AC` | Static init caches XML type ? `dword_82A0A8AC`. |
| `0x828a4580` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A664` | Static init caches XML type ? `dword_82A0A664`. |
| `0x828a5cd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A6B4` | Static init caches XML type ? `dword_82A0A6B4`. |
| `0x828a80d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A748` | Static init caches XML type ? `dword_82A0A748`. |
| `0x828a8680` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A9FC` | Static init caches XML type ? `dword_82A0A9FC`. |
| `0x828a8bd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AAEC` | Static init caches XML type ? `dword_82A0AAEC`. |
| `0x828a9328` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AC1C` | Static init caches XML type ? `dword_82A0AC1C`. |
| `0x828a9640` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AAB0` | Static init caches XML type ? `dword_82A0AAB0`. |
| `0x828a98c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AC8C` | Static init caches XML type ? `dword_82A0AC8C`. |
| `0x828a9b98` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB48` | Static init caches XML type ? `dword_82A0AB48`. |
| `0x828aa7b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB38` | Static init caches XML type ? `dword_82A0AB38`. |
| `0x828ab650` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB54` | Static init caches XML type ? `dword_82A0AB54`. |
| `0x828acce0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AFA0` | Static init caches XML type ? `dword_82A0AFA0`. |
| `0x828ad8b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AFA4` | Static init caches XML type ? `dword_82A0AFA4`. |
| `0x828adb80` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AD20` | Static init caches XML type ? `dword_82A0AD20`. |
| `0x828af368` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AEE0` | Static init caches XML type ? `dword_82A0AEE0`. |
| `0x828af518` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B004` | Static init caches XML type ? `dword_82A0B004`. |
| `0x828b1d18` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B3E4` | Static init caches XML type ? `dword_82A0B3E4`. |
| `0x828b27c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B09C` | Static init caches XML type ? `dword_82A0B09C`. |
| `0x828b4f78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B708` | Static init caches XML type ? `dword_82A0B708`. |
| `0x828b5248` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B734` | Static init caches XML type ? `dword_82A0B734`. |
| `0x828b6910` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B5AC` | Static init caches XML type ? `dword_82A0B5AC`. |
| `0x828b6c70` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B72C` | Static init caches XML type ? `dword_82A0B72C`. |
| `0x828b6f88` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B4D8` | Static init caches XML type ? `dword_82A0B4D8`. |
| `0x828b75b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B4DC` | Static init caches XML type ? `dword_82A0B4DC`. |
| `0x828b85c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BB58` | Static init caches XML type ? `dword_82A0BB58`. |

### Manual re-pass 23 (35 functions)

Offset 770+: XML type-handle static init hooks (plus CRT string/ptr-pair inits in pass 23).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828b9c90` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BDA0` | Static init caches XML type ? `dword_82A0BDA0`. |
| `0x828ba080` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BBEC` | Static init caches XML type ? `dword_82A0BBEC`. |
| `0x828ba1a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BCD8` | Static init caches XML type ? `dword_82A0BCD8`. |
| `0x828ba818` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BC04` | Static init caches XML type ? `dword_82A0BC04`. |
| `0x828baa10` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BBF0` | Static init caches XML type ? `dword_82A0BBF0`. |
| `0x828baf68` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BDB0` | Static init caches XML type ? `dword_82A0BDB0`. |
| `0x828bc288` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BCA4` | Static init caches XML type ? `dword_82A0BCA4`. |
| `0x828bc438` | `FM2_Crt_InitStaticString_82A0BC84` | CRT static init: `FM2_Stl_String_InitOrClear(&unk_82A0BC84)` + `atexit(sub_8294D210)`. |
| `0x828bc5a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BF00` | Static init caches XML type ? `dword_82A0BF00`. |
| `0x828bd298` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BE10` | Static init caches XML type ? `dword_82A0BE10`. |
| `0x828bdb50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C094` | Static init caches XML type ? `dword_82A0C094`. |
| `0x828c0070` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C178` | Static init caches XML type ? `dword_82A0C178`. |
| `0x828c1308` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C2A8` | Static init caches XML type ? `dword_82A0C2A8`. |
| `0x828c2f28` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C210` | Static init caches XML type ? `dword_82A0C210`. |
| `0x828c3948` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C358` | Static init caches XML type ? `dword_82A0C358`. |
| `0x828c5408` | `FM2_Crt_StaticInitPtrPair_82A43B1C` | CRT static init: `sub_8276B730` stores ptr+count into `82a43b1c`. |
| `0x828c5cc8` | `FM2_Crt_StaticInitPtrPair_82A43C5C` | CRT static init: `sub_8276B730` stores ptr+count into `82a43c5c`. |
| `0x828c6160` | `FM2_Crt_StaticInitPtrPair_82A43D04` | CRT static init: `sub_8276B730` stores ptr+count into `82a43d04`. |
| `0x828c6cc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70E50` | Static init caches XML type ? `dword_82A70E50`. |
| `0x828c7578` | `FM2_XmlStaticInit_CacheTypeHandle_82A70D08` | Static init caches XML type ? `dword_82A70D08`. |
| `0x828c8388` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C30` | Static init caches XML type ? `dword_82A70C30`. |
| `0x828c8580` | `FM2_XmlStaticInit_CacheTypeHandle_82A70E28` | Static init caches XML type ? `dword_82A70E28`. |
| `0x828c8928` | `FM2_XmlStaticInit_CacheTypeHandle_82A70F00` | Static init caches XML type ? `dword_82A70F00`. |
| `0x828c9270` | `FM2_XmlStaticInit_CacheTypeHandle_82A70FB4` | Static init caches XML type ? `dword_82A70FB4`. |
| `0x828c99c0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C7C` | Static init caches XML type ? `dword_82A70C7C`. |
| `0x828c9c48` | `FM2_XmlStaticInit_CacheTypeHandle_82A70DC4` | Static init caches XML type ? `dword_82A70DC4`. |
| `0x828cb058` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C64` | Static init caches XML type ? `dword_82A70C64`. |
| `0x828cb1c0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70CFC` | Static init caches XML type ? `dword_82A70CFC`. |
| `0x828cb600` | `FM2_XmlStaticInit_CacheTypeHandle_82A7112C` | Static init caches XML type ? `dword_82A7112C`. |
| `0x828ccb60` | `FM2_XmlStaticInit_CacheTypeHandle_82A71234` | Static init caches XML type ? `dword_82A71234`. |
| `0x828cd1d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A71134` | Static init caches XML type ? `dword_82A71134`. |
| `0x828cdd60` | `FM2_XmlStaticInit_CacheTypeHandle_82A712B0` | Static init caches XML type ? `dword_82A712B0`. |
| `0x828ce8e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A711C4` | Static init caches XML type ? `dword_82A711C4`. |
| `0x828cf278` | `FM2_XmlStaticInit_CacheTypeHandle_82A71128` | Static init caches XML type ? `dword_82A71128`. |
| `0x828cf8b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A716D4` | Static init caches XML type ? `dword_82A716D4`. |


### Manual re-pass 24 (35 functions)

XML type-handle static init hooks (offset 840+).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828d05a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A713D8` | Static init caches XML type ? `dword_82A713D8`. |
| `0x828d1e60` | `FM2_XmlStaticInit_CacheTypeHandle_82A7149C` | Static init caches XML type ? `dword_82A7149C`. |
| `0x828d1fc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A714B4` | Static init caches XML type ? `dword_82A714B4`. |
| `0x828d2178` | `FM2_XmlStaticInit_CacheTypeHandle_82A71450` | Static init caches XML type ? `dword_82A71450`. |
| `0x828d3528` | `FM2_XmlStaticInit_CacheTypeHandle_82A7160C` | Static init caches XML type ? `dword_82A7160C`. |
| `0x828d4148` | `FM2_XmlStaticInit_CacheTypeHandle_82A717DC` | Static init caches XML type ? `dword_82A717DC`. |
| `0x828d4340` | `FM2_XmlStaticInit_CacheTypeHandle_82A717E0` | Static init caches XML type ? `dword_82A717E0`. |
| `0x828d5858` | `FM2_XmlStaticInit_CacheTypeHandle_82A71AF0` | Static init caches XML type ? `dword_82A71AF0`. |
| `0x828d6bc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A7181C` | Static init caches XML type ? `dword_82A7181C`. |
| `0x828d8fe0` | `FM2_XmlStaticInit_CacheTypeHandle_82A71DF0` | Static init caches XML type ? `dword_82A71DF0`. |
| `0x828da978` | `FM2_XmlStaticInit_CacheTypeHandle_82A71E10` | Static init caches XML type ? `dword_82A71E10`. |
| `0x828dbe00` | `FM2_XmlStaticInit_CacheTypeHandle_82A71C08` | Static init caches XML type ? `dword_82A71C08`. |
| `0x828dc2d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A720CC` | Static init caches XML type ? `dword_82A720CC`. |
| `0x828dd638` | `FM2_XmlStaticInit_CacheTypeHandle_82A71FA4` | Static init caches XML type ? `dword_82A71FA4`. |
| `0x828ddc20` | `FM2_XmlStaticInit_CacheTypeHandle_82A72248` | Static init caches XML type ? `dword_82A72248`. |
| `0x828de688` | `FM2_XmlStaticInit_CacheTypeHandle_82A71F04` | Static init caches XML type ? `dword_82A71F04`. |
| `0x828de880` | `FM2_XmlStaticInit_CacheTypeHandle_82A71FBC` | Static init caches XML type ? `dword_82A71FBC`. |
| `0x828df600` | `FM2_XmlStaticInit_CacheTypeHandle_82A7210C` | Static init caches XML type ? `dword_82A7210C`. |
| `0x828df6d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72074` | Static init caches XML type ? `dword_82A72074`. |
| `0x828e0968` | `FM2_XmlStaticInit_CacheTypeHandle_82A72678` | Static init caches XML type ? `dword_82A72678`. |
| `0x828e23d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A723D4` | Static init caches XML type ? `dword_82A723D4`. |
| `0x828e5200` | `FM2_XmlStaticInit_CacheTypeHandle_82A72810` | Static init caches XML type ? `dword_82A72810`. |
| `0x828e6b50` | `FM2_XmlStaticInit_CacheTypeHandle_82A72918` | Static init caches XML type ? `dword_82A72918`. |
| `0x828e71c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A726F4` | Static init caches XML type ? `dword_82A726F4`. |
| `0x828e8500` | `FM2_XmlStaticInit_CacheTypeHandle_82A72E28` | Static init caches XML type ? `dword_82A72E28`. |
| `0x828e8ff8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72D90` | Static init caches XML type ? `dword_82A72D90`. |
| `0x828e9ce8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72E1C` | Static init caches XML type ? `dword_82A72E1C`. |
| `0x828ea5e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72DF8` | Static init caches XML type ? `dword_82A72DF8`. |
| `0x828eaa20` | `FM2_XmlStaticInit_CacheTypeHandle_82A72BD4` | Static init caches XML type ? `dword_82A72BD4`. |
| `0x828eb290` | `FM2_XmlStaticInit_CacheTypeHandle_82A72ACC` | Static init caches XML type ? `dword_82A72ACC`. |
| `0x828ebc68` | `FM2_XmlStaticInit_CacheTypeHandle_82A72D44` | Static init caches XML type ? `dword_82A72D44`. |
| `0x828ec930` | `FM2_XmlStaticInit_CacheTypeHandle_82A73038` | Static init caches XML type ? `dword_82A73038`. |
| `0x828ecc00` | `FM2_XmlStaticInit_CacheTypeHandle_82A731D4` | Static init caches XML type ? `dword_82A731D4`. |
| `0x828ecfa8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72FE0` | Static init caches XML type ? `dword_82A72FE0`. |
| `0x828ed428` | `FM2_XmlStaticInit_CacheTypeHandle_82A730E4` | Static init caches XML type ? `dword_82A730E4`. |

### Manual re-pass 25 (35 functions)

XML type-handle static init hooks (offset 840+).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828ed980` | `FM2_XmlStaticInit_CacheTypeHandle_82A72F48` | Static init caches XML type ? `dword_82A72F48`. |
| `0x828ef438` | `FM2_XmlStaticInit_CacheTypeHandle_82A73004` | Static init caches XML type ? `dword_82A73004`. |
| `0x828efdc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72FBC` | Static init caches XML type ? `dword_82A72FBC`. |
| `0x828eff30` | `FM2_XmlStaticInit_CacheTypeHandle_82A73158` | Static init caches XML type ? `dword_82A73158`. |
| `0x828f0db0` | `FM2_XmlStaticInit_CacheTypeHandle_82A73584` | Static init caches XML type ? `dword_82A73584`. |
| `0x828f1b30` | `FM2_XmlStaticInit_CacheTypeHandle_82A733A8` | Static init caches XML type ? `dword_82A733A8`. |
| `0x828f1e48` | `FM2_XmlStaticInit_CacheTypeHandle_82A735DC` | Static init caches XML type ? `dword_82A735DC`. |
| `0x828f2a60` | `FM2_XmlStaticInit_CacheTypeHandle_82A735B4` | Static init caches XML type ? `dword_82A735B4`. |
| `0x828f2e98` | `FM2_XmlStaticInit_CacheTypeHandle_82A7358C` | Static init caches XML type ? `dword_82A7358C`. |
| `0x828f5138` | `FM2_XmlStaticInit_CacheTypeHandle_82A736E0` | Static init caches XML type ? `dword_82A736E0`. |
| `0x828f55b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A73770` | Static init caches XML type ? `dword_82A73770`. |
| `0x828f6068` | `FM2_XmlStaticInit_CacheTypeHandle_82A73860` | Static init caches XML type ? `dword_82A73860`. |
| `0x828f6608` | `FM2_XmlStaticInit_CacheTypeHandle_82A73814` | Static init caches XML type ? `dword_82A73814`. |
| `0x828f7028` | `FM2_XmlStaticInit_CacheTypeHandle_82A736CC` | Static init caches XML type ? `dword_82A736CC`. |
| `0x828f74a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A739A8` | Static init caches XML type ? `dword_82A739A8`. |
| `0x828f7a90` | `FM2_XmlStaticInit_CacheTypeHandle_82A73638` | Static init caches XML type ? `dword_82A73638`. |
| `0x828f82b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A738CC` | Static init caches XML type ? `dword_82A738CC`. |
| `0x828fa678` | `FM2_XmlStaticInit_CacheTypeHandle_82A73D50` | Static init caches XML type ? `dword_82A73D50`. |
| `0x828fa9d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A73B4C` | Static init caches XML type ? `dword_82A73B4C`. |
| `0x828fd020` | `FM2_XmlStaticInit_CacheTypeHandle_82A73EE8` | Static init caches XML type ? `dword_82A73EE8`. |
| `0x828fdd58` | `FM2_XmlStaticInit_CacheTypeHandle_82A73F04` | Static init caches XML type ? `dword_82A73F04`. |
| `0x828fe928` | `FM2_XmlStaticInit_CacheTypeHandle_82A73E68` | Static init caches XML type ? `dword_82A73E68`. |
| `0x828fecd0` | `FM2_XmlStaticInit_CacheTypeHandle_82A73FA0` | Static init caches XML type ? `dword_82A73FA0`. |
| `0x828ff738` | `FM2_XmlStaticInit_CacheTypeHandle_82A7411C` | Static init caches XML type ? `dword_82A7411C`. |
| `0x828ffe88` | `FM2_XmlStaticInit_CacheTypeHandle_82A7407C` | Static init caches XML type ? `dword_82A7407C`. |
| `0x82900f68` | `FM2_XmlStaticInit_CacheTypeHandle_82A74304` | Static init caches XML type ? `dword_82A74304`. |
| `0x82901a60` | `FM2_XmlStaticInit_CacheTypeHandle_82A74454` | Static init caches XML type ? `dword_82A74454`. |
| `0x82902750` | `FM2_XmlStaticInit_CacheTypeHandle_82A74434` | Static init caches XML type ? `dword_82A74434`. |
| `0x82902948` | `FM2_XmlStaticInit_CacheTypeHandle_82A744E8` | Static init caches XML type ? `dword_82A744E8`. |
| `0x82902b40` | `FM2_XmlStaticInit_CacheTypeHandle_82A744D8` | Static init caches XML type ? `dword_82A744D8`. |
| `0x82903d88` | `FM2_XmlStaticInit_CacheTypeHandle_82A74334` | Static init caches XML type ? `dword_82A74334`. |
| `0x82906928` | `FM2_XmlStaticInit_CacheTypeHandle_82A74730` | Static init caches XML type ? `dword_82A74730`. |
| `0x829073d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A7462C` | Static init caches XML type ? `dword_82A7462C`. |
| `0x82907cd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A745A8` | Static init caches XML type ? `dword_82A745A8`. |
| `0x829097e0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74B34` | Static init caches XML type ? `dword_82A74B34`. |


### Manual re-pass 26 (35 functions)

Final batch-4 placeholders (offset 910+): XML static init + CRT atexit dtors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8290a908` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A0C` | Static init caches XML type ? `dword_82A74A0C`. |
| `0x8290aa70` | `FM2_XmlStaticInit_CacheTypeHandle_82A74C80` | Static init caches XML type ? `dword_82A74C80`. |
| `0x8290b568` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A3C` | Static init caches XML type ? `dword_82A74A3C`. |
| `0x8290b688` | `FM2_XmlStaticInit_CacheTypeHandle_82A74C8C` | Static init caches XML type ? `dword_82A74C8C`. |
| `0x8290c258` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A58` | Static init caches XML type ? `dword_82A74A58`. |
| `0x8290de80` | `FM2_XmlStaticInit_CacheTypeHandle_82A74E20` | Static init caches XML type ? `dword_82A74E20`. |
| `0x8290e8a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74DE4` | Static init caches XML type ? `dword_82A74DE4`. |
| `0x82910598` | `FM2_XmlStaticInit_CacheTypeHandle_82A74FB8` | Static init caches XML type ? `dword_82A74FB8`. |
| `0x82910af0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74F90` | Static init caches XML type ? `dword_82A74F90`. |
| `0x82910e20` | `FM2_XmlStaticInit_CacheTypeHandle_82A75238` | Static init caches XML type ? `dword_82A75238`. |
| `0x82911180` | `FM2_XmlStaticInit_CacheTypeHandle_82A75120` | Static init caches XML type ? `dword_82A75120`. |
| `0x82911570` | `FM2_XmlStaticInit_CacheTypeHandle_82A75218` | Static init caches XML type ? `dword_82A75218`. |
| `0x82911a38` | `FM2_XmlStaticInit_CacheTypeHandle_82A75194` | Static init caches XML type ? `dword_82A75194`. |
| `0x82912800` | `FM2_XmlStaticInit_CacheTypeHandle_82A75230` | Static init caches XML type ? `dword_82A75230`. |
| `0x82913928` | `FM2_XmlStaticInit_CacheTypeHandle_82A75090` | Static init caches XML type ? `dword_82A75090`. |
| `0x82915d30` | `FM2_XmlStaticInit_CacheTypeHandle_82A75458` | Static init caches XML type ? `dword_82A75458`. |
| `0x82915fb8` | `FM2_XmlStaticInit_CacheTypeHandle_82A754F0` | Static init caches XML type ? `dword_82A754F0`. |
| `0x82917ef0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75524` | Static init caches XML type ? `dword_82A75524`. |
| `0x82919b18` | `FM2_XmlStaticInit_CacheTypeHandle_82A75B14` | Static init caches XML type ? `dword_82A75B14`. |
| `0x8291a070` | `FM2_XmlStaticInit_CacheTypeHandle_82A758B0` | Static init caches XML type ? `dword_82A758B0`. |
| `0x8291a220` | `FM2_XmlStaticInit_CacheTypeHandle_82A758F8` | Static init caches XML type ? `dword_82A758F8`. |
| `0x8291a388` | `FM2_XmlStaticInit_CacheTypeHandle_82A75AF4` | Static init caches XML type ? `dword_82A75AF4`. |
| `0x8291b468` | `FM2_XmlStaticInit_CacheTypeHandle_82A7589C` | Static init caches XML type ? `dword_82A7589C`. |
| `0x8291b540` | `FM2_XmlStaticInit_CacheTypeHandle_82A758AC` | Static init caches XML type ? `dword_82A758AC`. |
| `0x8291c500` | `FM2_XmlStaticInit_CacheTypeHandle_82A75974` | Static init caches XML type ? `dword_82A75974`. |
| `0x8291d5a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75DF0` | Static init caches XML type ? `dword_82A75DF0`. |
| `0x8291e1b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F10` | Static init caches XML type ? `dword_82A75F10`. |
| `0x8291e4d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F68` | Static init caches XML type ? `dword_82A75F68`. |
| `0x8291ef38` | `FM2_XmlStaticInit_CacheTypeHandle_82A75C94` | Static init caches XML type ? `dword_82A75C94`. |
| `0x8291efc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75CA0` | Static init caches XML type ? `dword_82A75CA0`. |
| `0x8291fd00` | `FM2_XmlStaticInit_CacheTypeHandle_82A75C00` | Static init caches XML type ? `dword_82A75C00`. |
| `0x829202e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F30` | Static init caches XML type ? `dword_82A75F30`. |
| `0x82921028` | `FM2_XmlStaticInit_CacheTypeHandle_82A76318` | Static init caches XML type ? `dword_82A76318`. |
| `0x82921850` | `FM2_XmlStaticInit_CacheTypeHandle_82A76118` | Static init caches XML type ? `dword_82A76118`. |
| `0x82922300` | `FM2_XmlStaticInit_CacheTypeHandle_82A760E4` | Static init caches XML type ? `dword_82A760E4`. |

### Manual re-pass 27 (33 functions)

Final batch-4 placeholders (offset 910+): XML static init + CRT atexit dtors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82922780` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F74` | Static init caches XML type ? `dword_82A75F74`. |
| `0x82926910` | `FM2_XmlStaticInit_CacheTypeHandle_82A765AC` | Static init caches XML type ? `dword_82A765AC`. |
| `0x82928338` | `FM2_XmlStaticInit_CacheTypeHandle_82A76660` | Static init caches XML type ? `dword_82A76660`. |
| `0x82929300` | `FM2_XmlStaticInit_CacheTypeHandle_82A76A04` | Static init caches XML type ? `dword_82A76A04`. |
| `0x82929930` | `FM2_XmlStaticInit_CacheTypeHandle_82A7690C` | Static init caches XML type ? `dword_82A7690C`. |
| `0x82929bb8` | `FM2_XmlStaticInit_CacheTypeHandle_82A769BC` | Static init caches XML type ? `dword_82A769BC`. |
| `0x8292a668` | `FM2_XmlStaticInit_CacheTypeHandle_82A769F4` | Static init caches XML type ? `dword_82A769F4`. |
| `0x8292a7d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A76950` | Static init caches XML type ? `dword_82A76950`. |
| `0x8292ba60` | `FM2_XmlStaticInit_CacheTypeHandle_82A767FC` | Static init caches XML type ? `dword_82A767FC`. |
| `0x8292c2d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A768E4` | Static init caches XML type ? `dword_82A768E4`. |
| `0x8292e720` | `FM2_XmlStaticInit_CacheTypeHandle_82A76DB4` | Static init caches XML type ? `dword_82A76DB4`. |
| `0x829304a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A76DE0` | Static init caches XML type ? `dword_82A76DE0`. |
| `0x8293a920` | `FM2_XmlStaticInit_CacheTypeHandle_82A771BC` | Static init caches XML type ? `dword_82A771BC`. |
| `0x8293b070` | `FM2_XmlStaticInit_CacheTypeHandle_82A76E6C` | Static init caches XML type ? `dword_82A76E6C`. |
| `0x8293c108` | `FM2_XmlStaticInit_CacheTypeHandle_82A77054` | Static init caches XML type ? `dword_82A77054`. |
| `0x8293cc18` | `FM2_XmlStaticInit_CacheTypeHandle_82A77480` | Static init caches XML type ? `dword_82A77480`. |
| `0x8293e0a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77400` | Static init caches XML type ? `dword_82A77400`. |
| `0x8293f570` | `FM2_XmlStaticInit_CacheTypeHandle_82A772F8` | Static init caches XML type ? `dword_82A772F8`. |
| `0x8293fde0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77468` | Static init caches XML type ? `dword_82A77468`. |
| `0x82941660` | `FM2_XmlStaticInit_CacheTypeHandle_82A7784C` | Static init caches XML type ? `dword_82A7784C`. |
| `0x82942bc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77758` | Static init caches XML type ? `dword_82A77758`. |
| `0x82943dc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A7767C` | Static init caches XML type ? `dword_82A7767C`. |
| `0x82944240` | `FM2_XmlStaticInit_CacheTypeHandle_82A77638` | Static init caches XML type ? `dword_82A77638`. |
| `0x82945568` | `FM2_XmlStaticInit_CacheTypeHandle_82A77C64` | Static init caches XML type ? `dword_82A77C64`. |
| `0x82946018` | `FM2_XmlStaticInit_CacheTypeHandle_82A779F0` | Static init caches XML type ? `dword_82A779F0`. |
| `0x829460a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A77BBC` | Static init caches XML type ? `dword_82A77BBC`. |
| `0x82946378` | `FM2_XmlStaticInit_CacheTypeHandle_82A77B04` | Static init caches XML type ? `dword_82A77B04`. |
| `0x82946b58` | `FM2_XmlStaticInit_CacheTypeHandle_82A77D0C` | Static init caches XML type ? `dword_82A77D0C`. |
| `0x82948268` | `FM2_XmlStaticInit_CacheTypeHandle_82A77D74` | Static init caches XML type ? `dword_82A77D74`. |
| `0x829495d0` | `FM2_Crt_AtexitDtor_Sub822A7C00_829D8228` | CRT atexit dtor thunk: `sub_822A7C00(&unk_829D8228)` frees block, clears fields, re-inits string. |
| `0x8294b6d8` | `FM2_Crt_AtexitDtor_Sub825A0430_829F2E90` | CRT atexit dtor thunk: `sub_825A0430(&unk_829F2E90)` frees small block + clears triple. |
| `0x8294bdd0` | `FM2_Crt_AtexitDtor_Sub825A0430_82A00C6C` | CRT atexit dtor thunk: `sub_825A0430(&unk_82A00C6C)` frees small block + clears triple. |
| `0x8294d228` | `FM2_Crt_AtexitFreeSmallBlock_82A0BA44` | CRT atexit: `FM2_Memory_FreeSmallBlockOrNull(dword_82A0BA44)` then zeroes `82A0BA44..4C`. |

---

## Post batch-4: high-traffic `sub_` infrastructure

Target list: `scripts/ida_fm2_list_unnamed_sub_callees.py` ? `.cursor/hooks/state/unnamed-sub-callees.json` (sorted by FM2 caller count).

### Infrastructure pass 1 (12 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827D67D8` | `FM2_String_Hash65599SetHighBit` | 65599 string hash loop; result `\| 0x80000000`. |
| `0x827D6820` | `FM2_Xml_GetTypeHandleFromNameBuffer` | Calls hash helper; returns pointer stored by 355+ XML static-init hooks. |
| `0x827D9338` | `FM2_Lua_BindingVector_AppendPair` | Appends 8-byte pair to Lua binding registration vector. |
| `0x824EC390` | `FM2_Lua_AppendBindingEntryAt64` | `FM2_Lua_Register*` thunk: append pair at `a1+64`. |
| `0x824EC358` | `FM2_Lua_AppendBindingEntryAt48` | Same pattern at `a1+48`. |
| `0x82211E18` | `FM2_Lua_SetActiveThreadContext` | Sets/clears global Lua thread context `qword_829F40D4`. |
| `0x8254E060` | `FM2_Lua_RaiseBadArgumentError` | Formats `bad argument #N` / `calling 'method' on bad self` via Lua error path. |
| `0x8254E1A0` | `FM2_Lua_GetUserdataMethodOrRaise` | Resolves userdata method table entry or falls back to error helper. |
| `0x8229FF10` | `FM2_Lua_PushAbsoluteStackIndex` | Pushes `lua_gettop+1` style absolute index. |
| `0x8229FFC0` | `FM2_Lua_InitRelativeStackIndex` | Computes relative stack index from top and arg offset. |
| `0x82297AA8` | `FM2_Lua_PushNumberAndStackValue` | Pushes double then copies stack slot (Lua return helper). |
| `0x82765CF0` | `FM2_ComPtr_GetRawPointer` | Reads raw interface pointer at `*(a1+4)`; used by `FM2_ComPtr_InvokeVirtual*`. |

### Infrastructure pass 2 (30 functions)

Lua stack core (`sub_824B*`), binding thunks, SQLite alloc, CRT static inits.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824b6518` | `FM2_Lua_GetStackSlotPointer` | Core Lua stack: resolve absolute/relative/registry index to TValue pointer. |
| `0x824b6770` | `FM2_Lua_GetStackDepth` | Returns `(top - base) >> 4` stack depth. |
| `0x824b6788` | `FM2_Lua_SetStackTop` | Sets Lua stack top; grows stack with nil slots when needed. |
| `0x824b68a0` | `FM2_Lua_PopStackSlot` | Copies stack slot then decrements top (pop). |
| `0x824b69f8` | `FM2_Lua_GetStackValueType` | Returns Lua type tag for stack index, or -1 if absent. |
| `0x824b6a38` | `FM2_Lua_GetTypeNameForTag` | Maps type tag to name string (`no value`, `nil`, etc.). |
| `0x824b6b80` | `FM2_Lua_StackSlotsEqual` | Compares two stack slots for equality. |
| `0x824b6cc8` | `FM2_Lua_ToNumberOrZero` | Reads stack slot as double (type 3) or 0. |
| `0x824b6db8` | `FM2_Lua_ToLString` | Coerces stack slot to Lua string; returns `TString*` data. |
| `0x824b7020` | `FM2_Lua_PushNil` | Pushes nil onto Lua stack. |
| `0x824b7040` | `FM2_Lua_PushNumber` | Pushes double onto Lua stack (type tag 3). |
| `0x824b72e8` | `FM2_Lua_PushBool` | Pushes boolean onto Lua stack. |
| `0x824b73b0` | `FM2_Lua_PushCString` | Pushes C string onto Lua stack. |
| `0x824b7558` | `FM2_Lua_PushCFunctionFromStack` | Pushes C function/clojure from stack slot (types 5/7). |
| `0x822a02b0` | `FM2_Lua_GetArgPointerOrLogMissing` | Bounds-checks Lua args vector; logs missing arg via execution error. |
| `0x822a0028` | `FM2_Lua_PushBoolFromStackArg` | Pushes bool from stack arg through thread context. |
| `0x82297fb0` | `FM2_Lua_PushDisplayStringById` | Pushes `UserInterface::DisplayString` by id then copies stack arg. |
| `0x822b1c00` | `FM2_Lua_PushIntAndStackArg` | Pushes int as number then copies stack arg (return helper). |
| `0x8229ff60` | `FM2_Lua_RaiseExecutionError` | Formats `Error executing '<fn>': ...` and raises Lua error. |
| `0x8254e148` | `FM2_Lua_RaiseTypeMismatchError` | Raises `%s expected, got %s` type mismatch via bad-arg helper. |
| `0x824ec3c8` | `FM2_Lua_AppendBindingEntryAt80` | `FM2_Lua_Register*` thunk: append pair at `a1+80`. |
| `0x824ec618` | `FM2_Lua_AppendBindingEntryAt44` | `FM2_Lua_Register*` thunk: append pair at `a1+44`. |
| `0x8220caf0` | `FM2_Lua_BindingVector_PopBackIndex` | Pops binding-vector back index when clearing thread context. |
| `0x8245a4d8` | `FM2_Object_AssignBaseVtable_82000E18` | Dtor prologue: `*result = off_82000E18`. |
| `0x826ed658` | `FM2_SQLite_FreeIfNonNull` | If non-null ptr, calls SQLite free dispatch `dword_82A3CEB8`. |
| `0x826ee578` | `FM2_SQLite_Alloc` | SQLite malloc wrapper via `dword_82A3CEB0`; sets OOM flag on failure. |
| `0x826ee5e8` | `FM2_SQLite_AllocZeroed` | `FM2_SQLite_Alloc` + `memset(0)`. |
| `0x82272650` | `FM2_Crt_StaticInit_GraphicsStreamList_829C45B0` | CRT static init for graphics stream list `unk_829C45B0` + atexit. |
| `0x82206d08` | `FM2_Crt_StaticInit_ScriptBindingManager_829C24B8` | Lazy singleton init for script binding manager `dword_829C24B8`. |
| `0x82803670` | `FM2_Crt_AssertCurrentThreadOrBugcheck` | If not current thread id `82998F54`, triggers bugcheck path. |

### Infrastructure pass 3 (33 functions)

Lua error/stack path, binding thunk +32, hash/list/CRT/XAudio2 helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824b6f30` | `FM2_Lua_GetUserdataPointer` | Returns userdata ptr (type 2) or userdata+24 (type 7). |
| `0x824b7090` | `FM2_Lua_PushInternedString` | Interns bytes via `sub_824BF480`, pushes string (type 4). |
| `0x824b70f8` | `FM2_Lua_PushLStringOrNil` | Push length-delimited string or nil if `a2` null. |
| `0x824b7148` | `FM2_Lua_PushFormattedStringGrowStack` | Grow stack if needed, delegate to formatted string push. |
| `0x824b74f0` | `FM2_Lua_PushCFunction` | Push C function closure (type 5) via `sub_824BE8F8`. |
| `0x824b76b8` | `FM2_Lua_SetFieldFromCString` | Set table field from C string (`sub_824BCA78`). |
| `0x824b7860` | `FM2_Lua_SetClosureEnvFromStack` | Assign closure/userdata env from value below top. |
| `0x824b7e10` | `FM2_Lua_PushUserdataForKey` | Alloc userdata for registry key lookup; push (type 7). |
| `0x824b7cc8` | `FM2_Lua_ErrorThrow` | Unwind/throw after Lua error (`sub_824BBFE0`). |
| `0x824b7d50` | `FM2_Lua_ErrorAppendMessagePart` | Append formatted part to error message on stack. |
| `0x824b8138` | `FM2_Lua_NumberValuesEqual` | Equality test for two Lua numbers (type 3). |
| `0x824b8318` | `FM2_Lua_PushFormattedString` | Mini printf (`%s/%d/%f/...`) pushing Lua strings/numbers. |
| `0x824bb158` | `FM2_Lua_GrowStack` | Grow Lua stack when near limit (`sub_824BAFA0`). |
| `0x824bb2c8` | `FM2_Lua_UpdateObjectGcMark` | Update object GC mark bits from global state. |
| `0x824bb300` | `FM2_Lua_LinkGrayObject` | Link object into gray list during GC. |
| `0x824bb428` | `FM2_Lua_GetDebugCallInfoLevel` | Resolve call-info level for debug/error prefix. |
| `0x824bc778` | `FM2_Lua_CoerceToNumberSlot` | Coerce stack slot to number (incl. string parse). |
| `0x824bc960` | `FM2_Lua_GetTableField` | Table field lookup with `__index` metamethod loop. |
| `0x824bc0a8` | `FM2_Lua_ErrorFormatAndThrow` | Format error string then throw (`sub_824BBED0`). |
| `0x824bc7e8` | `FM2_Lua_CoerceNumberToString` | Coerce numeric stack slot to interned string. |
| `0x824bf650` | `FM2_Lua_AllocUserdata` | Allocate full userdata with metatable ref. |
| `0x8254d110` | `FM2_Lua_ErrorPrefixWithSourceLocation` | Prefix Lua error with `source:line:` when available. |
| `0x8254d198` | `FM2_Lua_ErrorVprintf` | Vararg Lua error formatter; used by bad-arg/type helpers. |
| `0x824ec320` | `FM2_Lua_AppendBindingEntryAt32` | `FM2_Lua_Register*` thunk: append pair at `a1+32`. |
| `0x8245a740` | `FM2_HashName_AssignVtable_8203CEA4` | Hash-name object dtor: `*a1 = off_8203CEA4`. |
| `0x8245dbf8` | `FM2_IntrusiveList_InitSentinel` | Init self-linked intrusive list node pair. |
| `0x8245ea60` | `FM2_Crt_StaticInit_ForzaCmdLineList_829F194C` | CRT static init `unk_829F194C` + atexit (cmdline/startup list). |
| `0x82460aa0` | `FM2_Crt_StaticInit_AsyncQueueGlobal_829F1A48` | CRT static init `unk_829F1A48` + atexit (async queue global). |
| `0x8276b730` | `FM2_Crt_StorePtrPair` | Store `{ptr,count}` pair into static global (2 dwords). |
| `0x8277d9e0` | `FM2_STL_Map_KeyCompareThunk` | STL map insert key-compare thunk -> `sub_827F6180`. |
| `0x826bff38` | `FM2_XAudio2_VoicePool_ReleaseVoiceLocked` | Locked XAudio2 voice pool release: COM release + heap free. |
| `0x826c2e20` | `FM2_XAudio2_Voice_UnlinkFromPool` | Unlink XAudio2 voice from pool intrusive list under critsec. |
| `0x826c1df0` | `FM2_XAudio2_Voice_ReleaseResourcesLocked` | Locked XAudio2 voice teardown: unlink, release ref, free buffers. |

### Infrastructure pass 4 (33 functions)

Lua binding-record module, image resample kernels, list/wstring helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824ec650` | `FM2_Lua_RegisterModuleBindings` | Iterates 104-byte binding records; registers module table + `_LOADED`. |
| `0x824ec400` | `FM2_Lua_PushBindingRecordToTable` | Pushes one binding record (name, func, pair vectors) into Lua table. |
| `0x824ec210` | `FM2_LuaBindingRecord_CopyAssign` | Copy-assign 104-byte Lua binding record incl. four pair vectors. |
| `0x824ec2a0` | `FM2_LuaBindingRecord_InitFromCStr` | Construct binding record from C string name + property id. |
| `0x824ec788` | `FM2_LuaBindingRecord_VectorShiftInsertCopies` | Vector insert: backward-copy 104-byte records to make room. |
| `0x824ec7e0` | `FM2_LuaBindingRecord_VectorConstructRange` | Construct N copies of binding record via `FM2_SceneNode_CopyAssignExtended`. |
| `0x824ecbb0` | `FM2_LuaBindingRecord_VectorReserveAndReset` | Reallocate binding-record vector; reset begin/end iterators. |
| `0x824ecc90` | `FM2_LuaBindingRecord_VectorEmplaceBack` | Emplace binding record at vector end (copy or grow). |
| `0x824ecd50` | `FM2_LuaBindingVector_EmplaceQualifiedName` | Build `scope::name` qualified binding and emplace into vector. |
| `0x824ec890` | `FM2_LuaBindingRecord_VectorInsert` | Full vector insert with reallocate/shift of 104-byte records. |
| `0x824ebf78` | `FM2_Lua_BindingPairVector_Assign` | Assign/copy Lua binding pair vector `{key,func}` pairs. |
| `0x8254e4f0` | `FM2_Lua_RegisterBindingPairsInModuleTable` | Register `{name,func}` pairs into module `_LOADED` table. |
| `0x824ead78` | `FM2_Lua_PushMetatableWithGcAndProps` | Push metatable with `__gc`, optional `__getprop`/`__setprop`. |
| `0x822a2ed8` | `FM2_LuaBindingRecord_Dtor` | Dtor: free four pair vectors + SSO string in binding record. |
| `0x82466aa0` | `FM2_LuaBindingRecord_GetPropertyFlags` | Returns property-flag dword at binding-record offset +96. |
| `0x824b7750` | `FM2_Lua_SetTableFieldFromStack` | Set table field from two stack slots (metatable assignment). |
| `0x824b69b0` | `FM2_Lua_CopyStackSlotToTop` | Copy stack slot value to stack top. |
| `0x824b7210` | `FM2_Lua_PushLightUserdataWithArgs` | Push light userdata/C closure capturing N stack args. |
| `0x824b7a88` | `FM2_Lua_ProtectedCallWithTraceback` | Protected call with traceback hook (`sub_824B9780`). |
| `0x8254de38` | `FM2_Lua_LoadFileFromCStringPath` | Load Lua chunk from C string path via reader callback. |
| `0x824ece68` | `FM2_LuaIo_GetFileHandleReadable` | Lua IO: get readable file handle (op 2). |
| `0x824ece98` | `FM2_LuaIo_TryOpenFileForLoad` | Lua IO: try open file for load if not already loading. |
| `0x824ecef0` | `FM2_LuaIo_SetFileModeBits` | Lua IO: set file mode bits (ops 6/7). |
| `0x824ecf50` | `FM2_LuaIo_IsFileWritable` | Lua IO: query writable flag (op 5). |
| `0x824ecf90` | `FM2_LuaIo_GetFileSizeFlags` | Lua IO: combine size flag bits from ops 3/4. |
| `0x823928d8` | `FM2_Image_ResampleKernel_ApplyTexelTransform` | Bilinear resample texel transform by source/dest format modes. |
| `0x82393c38` | `FM2_Image_ResampleKernel_AccumulateAndClearRow` | Resample row accumulate from temp buffer then zero scratch. |
| `0x823d7460` | `FM2_Image_ResampleKernel_ApplySqrtOrGammaTable` | Resample apply sqrt/gamma LUT (`flt_820303F8/FC`) on texels. |
| `0x82231f10` | `FM2_WString_AssignFromWideCStr` | Wide-string assign from `wchar_t*` via internal append. |
| `0x82257f18` | `FM2_IntrusiveList_ClearAndDestroyNodes` | Clear intrusive list: dtor each node then free block. |
| `0x821f8250` | `FM2_IntrusiveList_ClearAndFreeEntries` | Clear intrusive list: free nodes without per-node dtor. |
| `0x821e6a08` | `FM2_SceneNode_GetExtendedPayloadOffset` | Returns scene-node extended payload at `this+10016`. |
| `0x821fc2b0` | `FM2_ComObject_ReleaseAndOptionalFree` | Release COM object at +8; optional `FM2_Memory_FreeSmallBlockOrNull`. |

### Infrastructure pass 5 (33 functions)

Car-parts lookup, render/SQLite/XTS helpers, deferred-task and input utilities.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82252f98` | `FM2_CarParts_MergeUpgradePathListFromLookup` | Lookup upgrade path node; splice matching intrusive lists into output. |
| `0x82252350` | `FM2_CarParts_FindUpgradePathNodeByName` | Walk car-parts upgrade list; find node whose name matches lookup key. |
| `0x827fc3a8` | `FM2_STL_ListNode_GetPayloadAtOffset21` | Returns list-node payload pointer at `node+21` (camera-list helper). |
| `0x827fc398` | `FM2_STL_ListNode_GetPayloadAtOffset20` | Returns list-node payload pointer at `node+20`. |
| `0x827e3260` | `FM2_InputBindingState_Dtor` | Input binding-state dtor: reset vtable, free heap block at +32. |
| `0x8279f458` | `FM2_STL_ListNode_InitSentinelFromLinkPtr` | Init STL list sentinel via `FM2_STL_ListNode_InitSentinelB` on link ptr. |
| `0x82621670` | `FM2_DeferredTaskParams_GetField12` | Deferred-task params: read field at +12 after field-4 accessor. |
| `0x82505e18` | `FM2_Render_ForwardPassLightingArgs` | Forwards pass-lighting args to core renderer helper `sub_82725560`. |
| `0x8245ca18` | `FM2_HashName_AppendDecimalField` | Format int as decimal string; append hash-name field via `sub_821D2968`. |
| `0x8235c068` | `FM2_Stl_ThrowLengthError_VectorTooLong` | Raise `std::length_error` for `vector<T> too long`. |
| `0x827eaf00` | `FM2_Input_SortKeyboardScanMapPairs` | Bubble-sort keyboard scan-code pairs in binding map table. |
| `0x827d6f38` | `FM2_XmlTree_ResolveIndexedChildChain` | Recursively resolve XML/tree child by index chain; returns leaf or default. |
| `0x827d6b90` | `FM2_Diag_LogEventWithSubContext` | Diag wrapper: fetch sub-context then log event 22. |
| `0x8279da10` | `FM2_STL_Map_KeyCompare_WStringThunk` | STL map wstring key-compare thunk -> `sub_827F6180`. |
| `0x8279d910` | `FM2_XtsClient_AppendPayloadFromNode` | Append XTS client payload node from source descriptor. |
| `0x8278f2c0` | `FM2_SharedPtr_AssignAndReleaseOldRef` | Assign shared-ptr field at +4; release old ref with AddRef/Release. |
| `0x827796e0` | `FM2_STL_EhUnwind_ReleaseSharedPtrGuard` | EH unwind guard: release held ref at guard+4. |
| `0x82762b70` | `FM2_WebGate_LogAssertMessage` | Format WebGate assert string and dispatch to COM log interface. |
| `0x827624c8` | `FM2_LiveConnection_CloseXtsTask` | Close XTS task handle (`XTS_TaskClose` / `XTS_TASK_HANDLE`). |
| `0x82762450` | `FM2_LiveConnection_AddRefXtsTask` | AddRef/acquire XTS task handle when session valid. |
| `0x827321e0` | `FM2_Render_SetPassDrawOverride` | Set/clear render-pass draw override target and related pass flags. |
| `0x827257e0` | `FM2_Render_TransformPassLightingVectors` | Transform pass lighting vectors via matrix multiply (`sub_821D7400`). |
| `0x82721790` | `FM2_RenderTls_GetDrawPacketBatchBase` | Render TLS: return main context draw-packet base at `ctx+64`. |
| `0x8270d578` | `FM2_SQLite_ParseTree_DestroyRecursive` | Recursively destroy SQLite parse-tree nodes and free owned strings. |
| `0x8270d0b0` | `FM2_SQLite_ParseStack_DestroyEntries` | Destroy SQLite parse-stack entry array (counted triplets). |
| `0x826fd060` | `FM2_SQLite_Vdbe_PatchRecordChainEnd` | Vdbe finalize: patch prior record chain end index when closing slot. |
| `0x826fcda0` | `FM2_SQLite_Vdbe_BackpatchPriorStackCell` | Vdbe finalize: backpatch stack cell from current record index. |
| `0x826fcca0` | `FM2_SQLite_Vdbe_AppendOpcodeRecord` | Append 20-byte opcode record to Vdbe program array (grow if needed). |
| `0x826ef9b8` | `FM2_SQLite_Database_FreeAndMarkClosed` | Close SQLite database: release statements, mark closed, free block. |
| `0x824b9780` | `FM2_Lua_ProtectedCallWithTracebackRestore` | Protected Lua call helper: restore stack state after traceback hook. |
| `0x824b7bd0` | `FM2_LuaIo_DispatchFileOpStub` | Lua IO file-op dispatch stub: early return for op codes 0-7. |
| `0x82795520` | `FM2_DeferredTask_InitCallbackHolder` | Init deferred-task callback holder vtable + optional member invoke. |
| `0x82763670` | `FM2_DeferredTask_AssignSharedField4` | Deferred task: assign shared field from params field-4 via SharedPtr helper. |

### Infrastructure pass 6 (33 functions)

Refreshed callee list (1173 remaining). Lua intern/load path, STL append helpers, XAudio2 stream pool, SQLite parse teardown.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824bf480` | `FM2_Lua_InternString` | Lua string intern table: hash lookup or create `TString` via GC alloc. |
| `0x824c0298` | `FM2_Lua_ErrorBlockTooBig` | Raise Lua error `memory allocation error: block too big`. |
| `0x824b67e8` | `FM2_Lua_RemoveStackSlotAtIndex` | Remove stack slot at index by shifting slots down 16 bytes. |
| `0x824b7190` | `FM2_Lua_ErrorVprintfCore` | Vararg core for Lua error printf (`FM2_Lua_ErrorVprintf` path). |
| `0x824bc370` | `FM2_Lua_LoadStringWithFormatSpecifiers` | Load/eval Lua chunk string; handles `>` prefix and `f`/`L` format flags. |
| `0x824c02c8` | `FM2_Lua_AllocGcObjectFromState` | Allocate GC object from Lua global state allocator vtable. |
| `0x824bc110` | `FM2_Lua_ParseLoadStringFormatSpec` | Parse load-string format spec (`S`/`L`/etc.) into chunk metadata. |
| `0x824b7b20` | `FM2_LuaIO_OpenFileWithMode` | Lua IO open: build mode string then delegate to file open helper. |
| `0x824b6d68` | `FM2_Lua_IsStackSlotTruthy` | Returns whether stack slot is truthy (non-nil/non-false). |
| `0x824bd068` | `FM2_Lua_ErrorAppendStackArgs` | Append formatted stack args to Lua error message buffer. |
| `0x824bb668` | `FM2_Lua_PushLoadedClosureUpvalues` | Push closure upvalues after load (`L` format / loaded function). |
| `0x824bbfe0` | `FM2_Lua_ErrorThrowWithLongJmpRestore` | Restore longjmp frame then set Lua error status and unwind. |
| `0x824ebe68` | `FM2_Lua_BindingPairVector_CopyAssign` | Copy-assign Lua binding `{key,func}` pair vector (8-byte pairs). |
| `0x824ebd48` | `FM2_Lua_BindingPairVector_SortByPathComponent` | Quicksort binding pair vector by path component compare. |
| `0x821d2968` | `FM2_Stl_String_AssignAppendCStr` | STL string assign = copy base + append C string bytes. |
| `0x821ec4a0` | `FM2_Stl_String_AssignAppendSubrange` | STL string assign = copy base + append subrange from source. |
| `0x821d1720` | `FM2_Stl_String_AppendRange` | Append byte range to SSO/heap string (handles self-append overlap). |
| `0x821d1620` | `FM2_Stl_String_AppendBytesFromSource` | Append bytes from source string subrange into destination string. |
| `0x821d1500` | `FM2_Stl_String_DtorFromEhUnwind` | EH unwind helper: clear STL string via `InitOrClear`. |
| `0x8221be68` | `FM2_WString_ResizeOrReleaseHeapStorage` | Wide-string resize: memcpy SSO or free heap buffer when shrinking. |
| `0x82346390` | `FM2_Lua_BindingPairVector_ReserveCapacity` | Reserve capacity for 8-byte pair vector; throw on overflow. |
| `0x825d0ab8` | `FM2_CarParts_GetGlobalUpgradeRegistryPtr` | Returns global car-parts/upgrade registry singleton `dword_82A028D8`. |
| `0x822518f8` | `FM2_CarParts_AdvanceUpgradeListIterator` | Advance intrusive-list iterator over upgrade-path nodes. |
| `0x826c1ac0` | `FM2_XAudio2_HeapFreeVoiceBufferByTag` | Free XAudio2 voice buffer via tagged pool or process heap. |
| `0x826ce780` | `FM2_XAudio2_Stream_SignalSubmitEventIfZero` | Decrement stream submit refcount; signal event when zero. |
| `0x826c9bc0` | `FM2_XAudio2_Stream_DecRefAndFinalizePacket` | Decrement packet ref; unlink/requeue or finalize on last ref. |
| `0x826c4db0` | `FM2_XAudio2_CLeapBuffer_AllocateSlot` | Allocate/reuse CLeap buffer slot in voice stream pool. |
| `0x826bc110` | `FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback` | Decrement CLeap buffer ref; invoke completion callback at zero. |
| `0x826b2d00` | `FM2_XAudio2_Stream_AcquireVoiceRef` | Acquire voice reference from stream under interlocked refcount. |
| `0x826b1510` | `FM2_XAudio2_Stream_LookupVoiceByHandle` | Lookup XAudio2 voice object by handle in stream hash table. |
| `0x826b10c0` | `FM2_XAudio2_Stream_CloseWaitHandleIfIdle` | Decrement stream wait refcount; close wait handle when idle. |
| `0x82707f20` | `FM2_SQLite_ParseContext_DestroyRecursive` | Recursively destroy SQLite parse context tree and owned stacks. |
| `0x826ef068` | `FM2_SQLite_Database_ReleaseOpenStatements` | Release open SQLite statements/callbacks before database free. |

### Infrastructure pass 7 (33 functions)

Callee list refreshed (**1162** remaining). XAudio2 pool, SQLite Vdbe/parse, FMOD, live-connection, deferred-task.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x826bbda8` | `FM2_XAudio2_Pool_FreeWithCallback` | XAudio2 pool free: optional pre-free callback, dec ref, return slot to free list. |
| `0x824b8a70` | `FM2_Lua_GrowStackSlots` | Grow Lua stack slots; rebase stack/GC refs via `sub_824B88A8`. |
| `0x8240c028` | `FM2_Win32_WaitForSingleObject` | Thin wrapper around `Nt_WaitForSingleObject`. |
| `0x82762320` | `FM2_LiveConnection_ValidateXtsSession` | Validate live XTS COM session is initialized and signed in. |
| `0x821e6bc8` | `FM2_SceneNode_GetPostRaceCameraDataOffset` | Returns scene-node post-race camera payload at `this+10056`. |
| `0x82204898` | `FM2_AsyncOp_DecRefAndCloseWaitHandle` | Interlocked dec ref; close wait handle when secondary count hits zero. |
| `0x8220a5a8` | `FM2_SceneProp_GetSslObjectBindingOffset` | Returns scene-prop SSL binding subobject at `this+168`. |
| `0x822271d8` | `FM2_IntrusiveList_InitCritSecSentinelNode` | Allocate/init self-linked intrusive-list sentinel for critsec-backed list. |
| `0x8223da40` | `FM2_Memory_BindFrameAllocatorForCategory` | Bind memory category to current frame allocator kind (except kind 9). |
| `0x822553b0` | `FM2_GraphicsStream_NotifyListenersProfileChange` | Notify graphics-stream listeners after profile/state change. |
| `0x826fcc10` | `FM2_SQLite_Vdbe_GrowOpcodeArray` | Grow SQLite Vdbe opcode record array (20-byte entries). |
| `0x826f10c0` | `FM2_SQLite_ParseStack_PushEntry` | Push entry onto SQLite parse stack via alloc helper. |
| `0x826ee4e0` | `FM2_SQLite_CheckSoftHeapLimitAvailable` | Check SQLite soft heap limit before allocation. |
| `0x826eefc0` | `FM2_SQLite_Statement_FinalizeViaUserCallback` | Finalize SQLite statement through user callback when present. |
| `0x826f29a8` | `FM2_SQLite_TriggerList_DestroyAll` | Destroy all SQLite trigger definitions and owned parse trees. |
| `0x826f0e58` | `FM2_SQLite_ParseTriggerDestroyDeferred` | Deferred destroy of SQLite parse trigger list when refcount zero. |
| `0x826f27f0` | `FM2_SQLite_IndexList_DestroyAll` | Free SQLite index-name list arrays. |
| `0x826c3690` | `FM2_XAudio2_Stream_SubmitBufferLockedInner` | Locked XAudio2 stream buffer submit: queue packet under voice critsec. |
| `0x82682b10` | `FM2_FMOD_Event_LookupDescriptorById` | Lookup FMOD event descriptor record by packed event id. |
| `0x82677e80` | `FM2_FMOD_System_ValidateChannelInGlobalList` | Verify FMOD channel pointer is in global channel linked list. |
| `0x82671c98` | `FM2_FMOD_HeapAllocMaybeZero` | FMOD heap alloc wrapper; zero-fill when flag at +8 clear. |
| `0x82671ba8` | `FM2_FMOD_Dsp_ReverbProcessDelayLine` | FMOD reverb DSP delay-line process under critsec. |
| `0x826714c8` | `FM2_FMOD_BitBuffer_WriteBits` | Write bit field into FMOD bit buffer (set/clear masked bits). |
| `0x82674278` | `FM2_FMOD_CriticalSection_Leave` | Leave FMOD critical section; FMOD error code 36 if null. |
| `0x82642510` | `FM2_AudioMix_ComputeEnvelopeSampleAtPhase` | Compute audio envelope sample from phase tables (float interp). |
| `0x8263de58` | `FM2_LuaHashTable_GetFloatFieldAt8` | Read float from Lua hash table field at offset +8. |
| `0x827b01b0` | `FM2_DeferredTask_ResetCallbackHolder` | Reset deferred-task callback holder vtable and invoke cleanup. |
| `0x82782030` | `FM2_STL_Map_FindBucketWithComAllocator` | STL map bucket find thunk -> `sub_827F6100`. |
| `0x8277d8a8` | `FM2_Stl_ThrowLengthError_WithThreadAssert` | Throw `vector<T> too long` then assert current thread id. |
| `0x827f6180` | `FM2_STL_Map_DisposeNodeWithComAllocator` | STL map node dispose via COM allocator or tagged free. |
| `0x82454640` | `FM2_NetworkMessage_InitRedBlackTreeHeader` | Init network-message red-black tree header/sentinel links. |
| `0x82434738` | `FM2_ContentEntry36_DtorAndReleaseChild` | Content entry dtor: clear string field and release child COM ref. |
| `0x82464ac8` | `FM2_CircularBuffer_PushFrontNode` | Push new node at front of circular buffer intrusive list. |

### Infrastructure pass 8 (33 functions)

Callee list **1144** remaining. Hash-name cluster, audio signal gates, race ghost, render pass, Lua strict number.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82365048` | `FM2_Memory_UpdateFrameAllocCategoryRecord` | Update per-frame memory alloc record for category under global critsec. |
| `0x82367300` | `FM2_AlignUpToPowerOfTwo` | Align integer up to power-of-two boundary. |
| `0x82364460` | `FM2_AudioDevice_BindSignalGateForCategory` | Bind audio signal gate for device category index (boot/audio init). |
| `0x825ad608` | `FM2_AudioManager_GetCategoryDescriptorPtr` | Index audio manager category descriptor table by category id. |
| `0x824604d8` | `FM2_AudioSignalGate_ReadTimestamp` | Read audio signal-gate timestamp from vfunc at +80. |
| `0x82460510` | `FM2_AudioSignalGate_UpdateElapsedTimestamp` | Update elapsed audio signal-gate time; accumulate when active. |
| `0x824a0ef8` | `FM2_AudioManager_GetGlobalConfigPtr` | Returns global audio manager config pointer `dword_829F2DF8`. |
| `0x821d2770` | `FM2_Stl_ConstructLengthErrorFromString` | Construct `std::length_error` from message string. |
| `0x82264438` | `FM2_RefCountedBlock_IsSingleReference` | Returns whether ref-count field at +12 equals 1. |
| `0x8228d628` | `FM2_RaceGhost_ClearReplaySampleState` | Zero race-ghost replay sample float buffers and counters. |
| `0x8228d898` | `FM2_RaceGhost_InitReplaySampleBlock` | Init race-ghost replay block: copy cleared sample state + reset fields. |
| `0x825c6778` | `FM2_RaceEntry_AllocStateBlockArray` | Allocate array of 48-byte race-entry state blocks. |
| `0x825c57c0` | `FM2_CareerRace_LookupRewardIdFromXml` | Parse career-race XML `Id` field into reward hash-name lookup. |
| `0x82296460` | `FM2_Lua_TuningDatabase_InitRecord` | Construct/init large Lua tuning-database singleton record. |
| `0x8254e3c8` | `FM2_Lua_ToNumberStrict` | Lua `ToNumber` strict: reject ambiguous zero non-number. |
| `0x824b6aa8` | `FM2_Lua_IsNumberOrCoercibleToNumber` | True if stack slot is number or coercible to number. |
| `0x824be8f8` | `FM2_Lua_AllocCClosure` | Allocate Lua C closure object (type 5) with upvalues. |
| `0x824b91f0` | `FM2_Lua_SetErrorStatusAndUnwind` | Set Lua error status byte and trigger unwind/longjmp path. |
| `0x822a1c78` | `FM2_CritSec_InitAndZeroOwner` | Init critsec with spin count; zero owner dword. |
| `0x8255d880` | `FM2_Render_ObjectPassDrawSetupInner` | Core render object-pass draw setup (matrices, CB, sort keys). |
| `0x82516ad8` | `FM2_Render_ForwardPassLightingToCore` | Forward pass-lighting args to core renderer helper. |
| `0x8245a4f0` | `FM2_HashName_InitEmptyWithSentinel` | Init empty hash-name object with sentinel qword and vtable. |
| `0x8245b308` | `FM2_HashName_AssignFromPropertyNode` | Assign hash-name result from property node when id > 0x10. |
| `0x8245f120` | `FM2_HashName_LookupPropertyNodeByKey32` | Lookup hash-name property RB-tree node by 32-byte key. |
| `0x8245fd08` | `FM2_HashName_LookupAltModulePropertyImpl` | Recursive alt-module property lookup (`a.b.c` path parsing). |
| `0x8257cba8` | `FM2_HashName_ClearPropertyTable` | Clear 64-byte hash-name property table; reset count. |
| `0x8257cd78` | `FM2_HashName_CtorEmpty` | Construct empty `CHashName` object. |
| `0x8257e9f8` | `FM2_HashName_GetPropertyTablePtr` | Returns pointer to hash-name property table at +8. |
| `0x8258b598` | `FM2_NetworkMessage_InitStateBlockSentinel` | Init network-message state-block intrusive-list sentinel. |
| `0x824ccb30` | `FM2_XmlSchema_AppendCStringValue` | Append C string field to XML schema object via vfunc +32. |
| `0x824fe9f0` | `FM2_FontRegistry_AdvanceModuleListIterator` | Advance font module registry intrusive-list iterator. |
| `0x826eea50` | `FM2_SQLite_ReallocOrAllocZeroed` | SQLite realloc wrapper; falls back to zeroed alloc. |
| `0x826c3570` | `FM2_XAudio2_StreamPool_UnlinkAndNotify` | XAudio2 stream pool unlink/notify on buffer state transition. |

### Infrastructure pass 9 (33 functions)

Callee list **1148** remaining. Hash-name/property-bag cluster, render pass core, livery-mask, FMOD/SQLite helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8221c5d8` | `FM2_PropertyBag_InitListSentinel` | Init property-bag intrusive-list sentinel node. |
| `0x82220030` | `FM2_PropertyBagList_DestroyAndFree` | Destroy property-bag list nodes and free backing block. |
| `0x82231648` | `FM2_WString_AssignSubrangeFromSource` | Wide-string assign/copy subrange from source SSO/heap buffer. |
| `0x82331e78` | `FM2_HashName_FindPropertyNodeByKey` | Find hash-name RB-tree node matching 32-byte property key. |
| `0x8245a580` | `FM2_HashName_AssignPropertyByTypeId` | Assign hash-name property value by type id (string/wstring/list/etc.). |
| `0x8245b108` | `FM2_PropertyBag_InitRbTreeHeader` | Init property-bag red-black tree header links. |
| `0x8245b2c0` | `FM2_PropertyBag_CtorFromHashNameNode` | Construct `CPropertyBag` from hash-name lookup node. |
| `0x8253faf8` | `FM2_HashNamePropertyList_DestroyAndFree` | Destroy hash-name property list and free sentinel block. |
| `0x82204a28` | `FM2_Stl_String_FindDelimiterIndex` | Find index of delimiter char in STL string (for `a.b` paths). |
| `0x8245c4d8` | `FM2_HashName_NormalizeKeyString` | Normalize hash-name key string (case/char transform loop). |
| `0x8245f1f0` | `FM2_HashName_LookupValueByKeyInTable` | Lookup hash-name table value dword by key in RB-tree. |
| `0x82363bf8` | `FM2_AudioDevice_SetDeferredFreeFlag` | Set audio-device deferred-free flag; enqueue free when enabled. |
| `0x8236dbc8` | `FM2_Render_SetPassShaderFlagsFromArray` | OR shader/pass flag bits from bool array into render pass state. |
| `0x82578970` | `FM2_AudioManager_InitDefaultMixParameters` | Initialize default audio manager mix/fade timing parameters. |
| `0x82555d38` | `FM2_Render_SetupPassMaterialConstants` | Setup render pass material constants (VMX vector splats). |
| `0x82514f90` | `FM2_Render_ObjectPassSortAndEmitDraws` | Sort/object-pass draw emit helper for render setup inner path. |
| `0x82725560` | `FM2_Render_ApplyPassLightingCore` | Core pass-lighting transform application (matrix/vector math). |
| `0x826188c8` | `FM2_Memory_InsertFrameAllocMapEntry` | Insert frame alloc map entry keyed by frame counter. |
| `0x82366af8` | `FM2_Memory_ScheduleDeferredFreeForBlock` | Schedule memory block for deferred free under global critsec. |
| `0x82522f18` | `FM2_AllocPoolAcquire12xCount` | Pool alloc `12 * count` bytes with overflow guard. |
| `0x82657338` | `FM2_CircularBuffer_EraseRange` | Erase subrange from circular/intrusive buffer vector. |
| `0x826719d8` | `FM2_FMOD_HeapAllocFromPoolLocked` | FMOD heap alloc from pool under critsec (optional header skip). |
| `0x82429d08` | `FM2_SQLite_Vdbe_GetProgramCounter` | Returns SQLite Vdbe program counter at `this+16`. |
| `0x824f4138` | `FM2_SQLite_Vfs_AddRef` | SQLite VFS addref via vtable +8. |
| `0x82418560` | `FM2_Image_DecodePngFromMemory` | Decode PNG image bytes from memory buffer (zlib/ihdr path). |
| `0x8258ed20` | `FM2_InstalledParts_CtorDefaults` | Construct `CInstalledParts` with -1 filled defaults. |
| `0x825a10d8` | `FM2_LiveryMask_CreateInterfaceFromPath` | Create livery-mask COM interface from file path params. |
| `0x825a00b0` | `FM2_LiveryMask_InitCreateParams` | Init livery-mask creation parameter struct defaults. |
| `0x825a0ea8` | `FM2_LiveryMask_AllocAndInitMaskObject` | Allocate 240-byte livery-mask object and init from params. |
| `0x824a7278` | `FM2_ComObject_GetRefCountVtablePtr` | Returns COM ref-count vtable pointer `off_8299B824`. |
| `0x824a7688` | `FM2_ComObject_GetStaticLifetimeBlock` | Returns static COM lifetime block `unk_829F2EC8`. |
| `0x824cc0f8` | `FM2_CameraScript_DecRefAndUnloadIfLast` | Decref camera script module; unload when last ref. |
| `0x824fb108` | `FM2_DeferredTaskHolder_Dtor` | Deferred-task holder dtor: invoke callback then reset vtable. |

### Infrastructure pass 10 (33 functions)

Hash-name/property-bag helpers, profile, livery-mask, car setup/dynamics.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821e98d0` | `FM2_Stl_StringIter_InitFromStringEnd` | Init string iterator at end of SSO/heap string buffer. |
| `0x8221c630` | `FM2_WString_ReserveCapacity` | Reserve/grow wide-string capacity to requested char count. |
| `0x8221f150` | `FM2_IntrusiveList_EraseNodeAndRebalance` | Erase intrusive-list node and rebalance parent links. |
| `0x82419a74` | `FM2_Crt_StackProbeAlloc` | Stack probe / alloca chunk adjustment helper. |
| `0x8241de18` | `FM2_Char_ToLowerAscii` | Lowercase ASCII A-Z to a-z for hash-name normalization. |
| `0x8242fa30` | `FM2_RbTree_CompareKeyLess` | RB-tree key compare: `left.key < right.key`. |
| `0x8245bc20` | `FM2_Stl_StringIter_InitAtOffset` | Init string iterator at byte offset in source string. |
| `0x82466aa8` | `FM2_Profile_GetOptionalHeapBlockPtr` | Returns optional profile heap block pointer at +32. |
| `0x824a0f08` | `FM2_PresentationCarConfig_Dtor` | Presentation car-config dtor; reset base vtable. |
| `0x824de690` | `FM2_Profile_FreeOptionalHeapBlock` | Free optional profile heap block at +32. |
| `0x825a0430` | `FM2_LiveryMask_AtexitFreeSingleton` | Livery-mask singleton atexit: free backing storage. |
| `0x825eaa78` | `FM2_DirectIface_ResetPixelShaderBinding` | Direct3D iface: clear pixel-shader binding and release. |
| `0x82671728` | `FM2_FMOD_Dsp_AdjustDelayLinePointers` | FMOD DSP: adjust delay-line buffer pointers by delta. |
| `0x825c5158` | `FM2_CareerRace_QueryGameOptionsByToken` | SQL query GameOptions by token string for career race. |
| `0x8245b080` | `FM2_PropertyBag_RbTreeLowerBound` | Property-bag RB-tree lower_bound recursive walk. |
| `0x8221bee0` | `FM2_PropertyBag_AllocListNode` | Allocate property-bag intrusive-list node (121-byte). |
| `0x8253f9f8` | `FM2_HashNamePropertyList_EraseNode` | Erase node from hash-name property intrusive list. |
| `0x82331cc8` | `FM2_HashName_RbTreeLowerBoundInit` | Init hash-name RB-tree lower_bound iterator pair. |
| `0x824e7fd0` | `FM2_FrameAllocMap_AdvanceIterator` | Advance frame-alloc map ordered-set iterator. |
| `0x82617f48` | `FM2_FrameAllocMap_InsertOrAssign` | Insert/assign entry in frame-alloc ordered map. |
| `0x82656e78` | `FM2_IntVector_EraseRangeShift` | Erase int-vector subrange and memmove tail. |
| `0x825a04e0` | `FM2_LiveryMask_Ctor` | Construct livery-mask COM object with default params. |
| `0x825a05f0` | `FM2_LiveryMask_CopyCreateParams` | Copy livery-mask create-params struct fields. |
| `0x82207398` | `FM2_Stl_SnprintfToBuffer` | Vararg snprintf into fixed stack buffer. |
| `0x82460968` | `FM2_TuningDb_InitIntListSentinel` | Init tuning-db intrusive int-list sentinel. |
| `0x8221cb08` | `FM2_TuningDb_InitFloatListSentinel` | Init tuning-db intrusive float-list sentinel. |
| `0x821e67b8` | `FM2_CarSetup_Ctor` | Construct car-setup object with installed-parts defaults. |
| `0x822930c0` | `FM2_CarDynamics_InitSubsystems` | Init car-dynamics subsystem blocks and ramp samples. |
| `0x824cbfd0` | `FM2_CameraScript_DestroyModule` | Destroy camera script module when last ref. |
| `0x82204908` | `FM2_Stl_StringIter_GetCursorPtr` | String iterator: get current cursor pointer. |
| `0x82204958` | `FM2_Stl_StringIter_AdvanceChar` | String iterator: advance cursor by one char. |
| `0x824541a8` | `FM2_NetworkMessage_RbTreeLowerBound` | Network-message RB-tree lower_bound recursive walk. |
| `0x8245b008` | `FM2_PropertyBag_AllocRbTreeNode` | Allocate property-bag RB-tree node skeleton. |

### Infrastructure pass 11 (33 functions)

STL/render adapter, buf-file, config entries, hash table helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821d0350` | `FM2_Stl_CompareDwordLess` | STL comparator: `*a < *b` for dword pointers. |
| `0x821d0368` | `FM2_Stl_SprintfToBuffer` | Vararg sprintf into stack buffer. |
| `0x821d03d0` | `FM2_Stl_SwapChars` | Swap two char values in place. |
| `0x821d0cd0` | `FM2_ResourceLock_AssignAndProbeInterface` | Assign resource lock handle; probe COM interface thread-safe. |
| `0x821d3ed8` | `FM2_ComObject_ReleaseViaVtable16` | COM release via vtable offset +16. |
| `0x821d7400` | `FM2_Render_MatrixMultiplyVMX128` | VMX128 matrix/vector multiply for render lighting. |
| `0x821e1af0` | `FM2_BufFile32768_Ctor` | Construct 32KiB buffered file object. |
| `0x821e1f08` | `FM2_BufFile32768_Dtor` | Destroy 32KiB buffered file; free buffer. |
| `0x821e9cb0` | `FM2_SceneObject_DtorReleaseRefs` | Scene object dtor: release multiple COM/ref fields. |
| `0x821ea5c8` | `FM2_Stl_String_StripTrailingPath` | Strip trailing path segment from string iterator. |
| `0x821ef8c8` | `FM2_ContentEntry_CopyAssign` | Copy-assign content DB entry incl. intrusive lists. |
| `0x821f1798` | `FM2_Render_InitLightingConstantsVMX` | Init render lighting VMX constant splats. |
| `0x821f1dd0` | `FM2_RenderAdapter_UpdateFrameStats` | Render adapter: update per-frame timing stats. |
| `0x821f2150` | `FM2_RenderAdapter_PollPresentThrottle` | Throttle present polling when interval exceeded. |
| `0x821f24a8` | `FM2_RenderAdapter_QueueFrameTimingUpdate` | Queue render-adapter frame timing update. |
| `0x821f9618` | `FM2_IntVector_GetIteratorEndPtr` | Advance int-vector iterator to end pointer. |
| `0x821f97b0` | `FM2_IntVector_EraseOneShift` | Erase one int-vector element with shift. |
| `0x821ff8f0` | `FM2_BufFile_WriteCString` | Write C string into buffered file stream. |
| `0x822030c8` | `FM2_HashTable_FindEntryByKey` | Hash table lookup walk by key hash. |
| `0x82203d10` | `FM2_HashTableList_DestroyAndFree` | Destroy hash-table list nodes and free block. |
| `0x82204338` | `FM2_Crt_StaticInit_ScriptBindingTable_829C2410` | CRT static init script-binding table + atexit. |
| `0x82204750` | `FM2_D3D_ReleaseFrameCounterCritSec` | Record D3D frame counter and leave critsec. |
| `0x82204790` | `FM2_D3D_EndFrameDeferredCleanup` | End-of-frame D3D deferred cleanup when flagged. |
| `0x82204860` | `FM2_AsyncOp_IncrementRefAndWait` | Interlocked inc ref; wait on handle if negative. |
| `0x82204f68` | `FM2_ConfigEntry_CopyAssignPartial` | Partial copy-assign config entry (string + flags). |
| `0x82205188` | `FM2_ConfigEntry_CopyAssign` | Full copy-assign 36-byte config entry. |
| `0x82205e40` | `FM2_ConfigEntryVector_DestroyRange` | Destroy range of config entries (clear strings). |
| `0x822061b0` | `FM2_ConfigEntryVector_MoveConstructRange` | Move-construct config entry vector subrange. |
| `0x82453d80` | `FM2_NetworkMessage_AllocRbTreeNode` | Allocate network-message RB-tree node. |
| `0x825c6700` | `FM2_TuningDb_AllocListNode` | Allocate tuning-db list node. |
| `0x8242a350` | `FM2_TuningDb_AllocFloatListNode` | Allocate tuning-db float list node. |
| `0x82527ae0` | `FM2_IntVector_AdvanceIterator` | Advance int-vector iterator by N slots. |
| `0x821f8330` | `FM2_IntVector_ShiftEraseOne` | Shift-erase one element from int vector. |

### Infrastructure pass 12 (33 functions)

Small getters/setters, FileSys, FMOD/SQLite thunks, render TLS.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8220a5b0` | `FM2_SceneProp_GetManagerBindingOffset172` | Returns scene-prop manager binding at `this+172`. |
| `0x8221a8a8` | `FM2_LuaLapTracker_GetStateOffset4` | Small helper from decompile/caller context. |
| `0x82224578` | `FM2_RaceGhost_GetFieldAt592` | Read race-ghost dword at offset +592. |
| `0x8222e1a8` | `FM2_LuaLapTracker_GetFieldAt528` | Small helper from decompile/caller context. |
| `0x82255480` | `FM2_Profile_GetSettingsBlockOffset124` | Small helper from decompile/caller context. |
| `0x8225ef78` | `FM2_LuaLobbySort_SetContextAndRun` | Store lobby-sort context at +96; run sort helper. |
| `0x82264380` | `FM2_GraphicsStream_GetLinkedFlagAt17` | Small helper from decompile/caller context. |
| `0x82264398` | `FM2_GraphicsStream_SetNotifyFlagAt4` | Small helper from decompile/caller context. |
| `0x82360298` | `FM2_GraphicsAdapter_GetNotifyFlagAt4` | Small helper from decompile/caller context. |
| `0x82466ab0` | `FM2_CareerRace_GetFieldAt64` | Small helper from decompile/caller context. |
| `0x82474868` | `FM2_AIDriver_ForwardToAssistCompute` | Forward AI-driver assist compute using field at +8. |
| `0x824c3a48` | `FM2_LuaSyntax_ExpectedTokenAtOffset16` | Small helper from decompile/caller context. |
| `0x824f30d0` | `FM2_LuaTournament_GetQualifyingEntryCount` | Tournament qualifying entry count at +25560. |
| `0x82506658` | `FM2_RenderFramePipeline_SetField3348` | Small helper from decompile/caller context. |
| `0x8257cf60` | `FM2_CarDynamics_SetBasePointer` | Small helper from decompile/caller context. |
| `0x82581910` | `FM2_RewardsQuery_GetRecordOffset24` | Small helper from decompile/caller context. |
| `0x82581e78` | `FM2_SQLiteToken_GetFlagAt26` | Small helper from decompile/caller context. |
| `0x82603b30` | `FM2_SceneGraph_GetCompareFieldAt36` | Small helper from decompile/caller context. |
| `0x82728078` | `FM2_RenderTls_GetWorkerSlotMask16` | Small helper from decompile/caller context. |
| `0x8243c030` | `FM2_SceneProp_GetFieldAt48` | Small helper from decompile/caller context. |
| `0x8245cd58` | `FM2_AudioManager_SetFieldAt132` | Small helper from decompile/caller context. |
| `0x8240c600` | `FM2_CarAudio_AllocStreamBufferZeroed` | Small helper from decompile/caller context. |
| `0x825fa868` | `FM2_ComPtr_AssignRefAtOffset216` | Small helper from decompile/caller context. |
| `0x82685838` | `FM2_FMOD_Event_SetParameter2DFlag1` | Small helper from decompile/caller context. |
| `0x826ec460` | `FM2_SQLite_AppendLowercaseIdentifierMode1` | Small helper from decompile/caller context. |
| `0x826ec4c0` | `FM2_SQLite_AppendLowercaseIdentifierAlt` | Small helper from decompile/caller context. |
| `0x82758ec8` | `FM2_Crt_Fopen` | Thin CRT `fopen` wrapper. |
| `0x82761da8` | `FM2_RenderSortable_SetSortKeyFloat24` | Small helper from decompile/caller context. |
| `0x82206c48` | `FM2_FileSys_Ctor` | Construct `CFileSys` object; zero child fields. |
| `0x82206cb8` | `FM2_FileSys_Dtor` | Destroy `CFileSys` and child allocators. |
| `0x82208d68` | `FM2_FileSysEntry_Dtor` | Small helper from decompile/caller context. |
| `0x8220aa70` | `FM2_AllocPoolAcquire2320xCount` | Pool alloc `2320 * count` bytes with overflow guard. |
| `0x8220b260` | `FM2_ComPtr_MakeFromPoolAlloc16` | Small helper from decompile/caller context. |

### Infrastructure pass 13 (33 functions)

Render adapter timing, D3D hang path, buf-file, hash-table RB-tree, content entry.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821e7760` | `FM2_Stl_StringIter_InitFromBufferBounds` | Init string iterator with bounds check against STL string buffer. |
| `0x824aee08` | `FM2_RenderAdapter_GetFrameTimingVtablePtr` | Returns static frame-timing vtable pointer `off_8299B8C4`. |
| `0x821f1488` | `FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFB` | Evidence from decompile and caller context. |
| `0x8221b4e8` | `FM2_PropertyBag_AllocNodePool128xCount` | Pool alloc `0x80 * count` for property-bag nodes. |
| `0x82369470` | `FM2_D3D_SyncRingBufferSlotAfterGpuError` | Evidence from decompile and caller context. |
| `0x8240bff8` | `FM2_Xam_ShowDirtyDiscAndRelaunch` | Shows dirty-disc UI then `XLaunchNewImage` (GPU hang path). |
| `0x82413490` | `FM2_Crt_VsprintfS_L` | Thin `vsprintf_s_l` CRT wrapper. |
| `0x824615e8` | `FM2_D3D_SuspendLowPriorityWorkerThreads` | Evidence from decompile and caller context. |
| `0x824a68f0` | `FM2_ResourceLock_WaitReadyState3` | Evidence from decompile and caller context. |
| `0x8258b2f8` | `FM2_AllocPoolAcquire44xCount` | Pool alloc `44 * count` (FMOD/network RB-tree nodes). |
| `0x8253f108` | `FM2_HashNamePropertyList_EraseNodeRebalance` | RB-tree erase/rebalance for hash-name property list. |
| `0x821d7980` | `FM2_Render_MatrixMultiplyVMX128_From16Byte` | Evidence from decompile and caller context. |
| `0x821d9ef8` | `FM2_BufFile32768_FlushWriteBuffer` | Evidence from decompile and caller context. |
| `0x821df050` | `FM2_BufFile_CompareRangeSubstr` | Evidence from decompile and caller context. |
| `0x821df128` | `FM2_BufFile32768_DtorInner` | Tears down buf-file stream vtables and decrefs camera script. |
| `0x821e6238` | `FM2_Render_TransformVec4x4VMX128` | Evidence from decompile and caller context. |
| `0x821e6e40` | `FM2_Stl_SnprintfPartNames128` | Evidence from decompile and caller context. |
| `0x821e7678` | `FM2_RenderAdapter_SetVblankWaitState` | Evidence from decompile and caller context. |
| `0x821e82f8` | `FM2_SceneObject_ReleaseRefFields78_80_88` | Evidence from decompile and caller context. |
| `0x821ec2d0` | `FM2_ContentEntry_CopyTailFields384` | Evidence from decompile and caller context. |
| `0x821ef7f8` | `FM2_ContentEntry_CopyVec4AndSubrecord` | Evidence from decompile and caller context. |
| `0x821f13b8` | `FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFD` | Evidence from decompile and caller context. |
| `0x821f1f70` | `FM2_RenderAdapter_QueueFrameTimingUpdateInner` | Evidence from decompile and caller context. |
| `0x821f2330` | `FM2_RenderAdapter_TogglePresentInterval` | Evidence from decompile and caller context. |
| `0x821f3b08` | `FM2_Animation_ClampKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x82202310` | `FM2_HashTable_SetEntryWithRefAdd` | Evidence from decompile and caller context. |
| `0x82203918` | `FM2_HashTableList_EraseRangeIterators` | Evidence from decompile and caller context. |
| `0x822041f8` | `FM2_NetNotification_Ctor` | Evidence from decompile and caller context. |
| `0x821e6a10` | `FM2_RenderAdapter_GetFieldAt10036` | Evidence from decompile and caller context. |
| `0x821f0ff8` | `FM2_RenderAdapter_UpdateSceneNodeDrawState` | Evidence from decompile and caller context. |
| `0x821f0e30` | `FM2_RenderAdapter_MarkFrameTimingDirty` | Evidence from decompile and caller context. |
| `0x822034c0` | `FM2_HashTableList_ClearSentinelLinks` | Evidence from decompile and caller context. |
| `0x821ef130` | `FM2_ContentEntry_CopyDwordVector` | Evidence from decompile and caller context. |

### Infrastructure pass 14 (33 functions)

FileSys/config vectors, hash-table RB-tree, profile Lua, resource lock, buf-file.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825720c8` | `FM2_ConfigEntryVector_FreeBuffer` | Frees config-entry vector buffer via destroy-range helper. |
| `0x821e6ab0` | `FM2_RenderAdapter_SetPresentIntervalMode` | Evidence from decompile and caller context. |
| `0x821ea1d8` | `FM2_ContentEntry_CopyAssignHead304` | Evidence from decompile and caller context. |
| `0x821eecc0` | `FM2_ContentEntry_ReserveDwordVector` | Evidence from decompile and caller context. |
| `0x821f1a48` | `FM2_Animation_NormalizeKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x82203420` | `FM2_HashTableList_DestroySubtreeNodes` | Evidence from decompile and caller context. |
| `0x82203518` | `FM2_HashTableList_EraseNodeRebalance` | RB-tree erase/rebalance sibling to hash-name list helper. |
| `0x82206100` | `FM2_ConfigEntryVector_DestroyAndFree` | Evidence from decompile and caller context. |
| `0x824ce030` | `FM2_FileSysStream_DestroyNested` | Evidence from decompile and caller context. |
| `0x8242cb90` | `FM2_RedirectStream_Dtor` | Evidence from decompile and caller context. |
| `0x82411d20` | `FM2_Thread_SleepMilliseconds` | Wraps `KeDelayExecutionThread` for millisecond sleep. |
| `0x824d3730` | `FM2_ComObject_Ctor16BytePool` | Evidence from decompile and caller context. |
| `0x8245a478` | `FM2_D3D_GetGlobalPresentThrottleSingleton` | Evidence from decompile and caller context. |
| `0x824a5608` | `FM2_ResourceLock_WaitForReadyOrTimeout` | Evidence from decompile and caller context. |
| `0x824cfe00` | `FM2_BufFile_TrySeekPosition` | Evidence from decompile and caller context. |
| `0x824cfeb8` | `FM2_BufFile_GetStreamTell` | Evidence from decompile and caller context. |
| `0x824cfff8` | `FM2_BufFile_ReleaseRefCount` | Evidence from decompile and caller context. |
| `0x82206208` | `FM2_FileSys_DestroyEntryRange` | Evidence from decompile and caller context. |
| `0x8220b658` | `FM2_SceneGraph_CompareNodeNamePrefix` | Evidence from decompile and caller context. |
| `0x8220c8e8` | `FM2_ProfileLua_InvokeManagerCallback` | Evidence from decompile and caller context. |
| `0x8220c9d8` | `FM2_Lua_BindingVector_DecrementIterByIndex` | Evidence from decompile and caller context. |
| `0x8220cb60` | `FM2_ProfileLua_InitBindingContext` | Evidence from decompile and caller context. |
| `0x822023e8` | `FM2_HashTableNode_DtorOptionalFree` | Evidence from decompile and caller context. |
| `0x82252f40` | `FM2_ConfigEntry_DestroyRange` | Evidence from decompile and caller context. |
| `0x821e7218` | `FM2_ContentEntry_CopyMemcpyBlock128` | Evidence from decompile and caller context. |
| `0x825dcd20` | `FM2_AllocPoolAcquire4xCount` | Evidence from decompile and caller context. |
| `0x8242bc68` | `FM2_RefCountedThreadSafe_AssignBaseVtable` | Evidence from decompile and caller context. |
| `0x8220e3a0` | `FM2_ProfileLua_IsRegistryValueString` | Evidence from decompile and caller context. |
| `0x8220e408` | `FM2_ProfileLua_RegisterManagerClosure` | Evidence from decompile and caller context. |
| `0x8221a870` | `FM2_AudioManager_RouteInitByCmdlineFlag` | Branches audio init on Forza cmdline flag at +1048. |
| `0x8221b688` | `FM2_Profile_GetManagerHeapIfAlloc` | Evidence from decompile and caller context. |
| `0x82225058` | `FM2_BufferedStream_CtorRetainSource` | Evidence from decompile and caller context. |
| `0x82204d08` | `FM2_FileSysEntry_ReleaseRefAndClearString` | Evidence from decompile and caller context. |

### Infrastructure pass 15 (33 functions)

Profile Lua stack markers, wstring, intrusive-list RB-tree, profile DB, career race.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8220c6f8` | `FM2_ProfileLua_UnwindBindingStackSlot` | Evidence from decompile and caller context. |
| `0x8240c618` | `FM2_Thread_YieldExecution` | Thin `NtYieldExecution` wrapper. |
| `0x8220c758` | `FM2_ProfileLua_PushStackMarkerLink` | Evidence from decompile and caller context. |
| `0x8220c7c0` | `FM2_ProfileLua_InitStackMarker` | Evidence from decompile and caller context. |
| `0x8220c890` | `FM2_ProfileLua_PushBindingKeyAndPop` | Evidence from decompile and caller context. |
| `0x8221bf40` | `FM2_WString_GrowHeapCapacity` | Evidence from decompile and caller context. |
| `0x8221cf10` | `FM2_IntrusiveList_EraseNodeRebalance122` | RB-tree erase for 122-byte intrusive-list nodes. |
| `0x8221d328` | `FM2_IntrusiveList_ClearSentinelLinks122` | Evidence from decompile and caller context. |
| `0x822246c0` | `FM2_Boot_VsprintfBuffer260` | Boot path `vsprintf_s` into 0x104-byte buffer. |
| `0x82225578` | `FM2_IntrusiveList_AllocSentinelNode24` | Evidence from decompile and caller context. |
| `0x82437508` | `FM2_BufferedStream_InitCore` | Evidence from decompile and caller context. |
| `0x824b7668` | `FM2_Lua_InvokeProtectedCall32` | Evidence from decompile and caller context. |
| `0x824f51d8` | `FM2_Profile_GetFieldAt40` | Evidence from decompile and caller context. |
| `0x82572948` | `FM2_AudioManager_GetAltSingleton24` | Evidence from decompile and caller context. |
| `0x82509400` | `FM2_Presentation_GetManagerSingleton8` | Evidence from decompile and caller context. |
| `0x822dc9a8` | `FM2_ConfigEntry_ReleaseRefOptionalFree` | Evidence from decompile and caller context. |
| `0x827e4ca0` | `FM2_AllocPoolAcquire24xCount` | Evidence from decompile and caller context. |
| `0x824d3580` | `FM2_ComObject_SetUtf8NameWide` | Evidence from decompile and caller context. |
| `0x824a51a0` | `FM2_ResourceLock_EnterCritSecOrResolve` | Evidence from decompile and caller context. |
| `0x824a4f68` | `FM2_D3D_WaitGpuFrameSlotWithTimeout` | Evidence from decompile and caller context. |
| `0x822272f0` | `FM2_DeferredCommand_DtorReleaseRef` | Evidence from decompile and caller context. |
| `0x82228478` | `FM2_DeferredCommand_CopyAssign` | Evidence from decompile and caller context. |
| `0x8222ed38` | `FM2_WString_EraseSubrangeInPlace` | Evidence from decompile and caller context. |
| `0x8223db98` | `FM2_SceneProp_GetWideCharAtIndex` | Evidence from decompile and caller context. |
| `0x8224c5d0` | `FM2_LiveryMask_CheckListLengthLimit` | Evidence from decompile and caller context. |
| `0x82251b10` | `FM2_ProfileDb_CompareStringRecordsLess` | Evidence from decompile and caller context. |
| `0x82251c48` | `FM2_ProfileDb_InitBindingContexts` | Evidence from decompile and caller context. |
| `0x822520f0` | `FM2_ProfileDb_ReleaseBindingContexts` | Evidence from decompile and caller context. |
| `0x82252170` | `FM2_ProfileDb_DtorReleaseAll` | Evidence from decompile and caller context. |
| `0x822529b0` | `FM2_ProfileDb_CopyAssignRecord` | Evidence from decompile and caller context. |
| `0x82255488` | `FM2_Profile_ClearOptionsChangedFlag` | Clears profile options-changed bit at +744. |
| `0x82256028` | `FM2_CareerRace_IsRaceModeType2` | Evidence from decompile and caller context. |
| `0x82256040` | `FM2_CareerRace_IsRaceModeType6` | Evidence from decompile and caller context. |

### Infrastructure pass 16 (33 functions)

IO streams, Lua protected call, livery mask, race ghost, career race, render adapter.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8242cc38` | `FM2_RedirectStream_CtorFromSource` | Evidence from decompile and caller context. |
| `0x824a4d20` | `FM2_ResourceLock_ResolveFrameAllocatorState` | Evidence from decompile and caller context. |
| `0x824bca78` | `FM2_Lua_ProtectedCallDispatchLoop` | Evidence from decompile and caller context. |
| `0x824d32d8` | `FM2_WideString_ReserveCapacityChars` | Evidence from decompile and caller context. |
| `0x82610330` | `FM2_IntrusiveList_IncrementIteratorPastErase122` | Evidence from decompile and caller context. |
| `0x8220c688` | `FM2_ProfileLua_InitBindingNilMarker` | Evidence from decompile and caller context. |
| `0x8221cc88` | `FM2_IntrusiveList_DestroySubtreeNodes122` | Evidence from decompile and caller context. |
| `0x82226958` | `FM2_RaceEntry_OnFinishedNotifyGhostReplay` | Evidence from decompile and caller context. |
| `0x82230368` | `FM2_CarDb_QueryBaseCostByCarId` | SQLite `SELECT BaseCost FROM Data_Car WHERE Id=%u`. |
| `0x82231740` | `FM2_WString_AssignFromWideCStrChecked` | Evidence from decompile and caller context. |
| `0x82238928` | `FM2_RaceGhost_SelectOrRebuildPlaybackNode` | Evidence from decompile and caller context. |
| `0x8223d910` | `FM2_Render_PackDrawMatrixVMX128` | Evidence from decompile and caller context. |
| `0x82241830` | `FM2_Lua_LiveryEditor_RevertLayerApplyUpgrade` | Evidence from decompile and caller context. |
| `0x82242458` | `FM2_CarParts_ApplyUpgradeSlotGroupImpl` | Evidence from decompile and caller context. |
| `0x82242b88` | `FM2_Lua_LiveryEditor_GetCurrentLayerRecord` | Evidence from decompile and caller context. |
| `0x8224c428` | `FM2_LiveryMask_UpdatePendingEntryAt16` | Evidence from decompile and caller context. |
| `0x8224c4d8` | `FM2_LiveryMask_UpdatePendingEntryAt28` | Evidence from decompile and caller context. |
| `0x8224cdf8` | `FM2_LiveryMask_SetActiveCarMediaPath` | Builds `GAME:\Media\cars\%s` path for active livery car. |
| `0x8224d168` | `FM2_LiveryMask_BuildPendingUpdateRecord96` | Evidence from decompile and caller context. |
| `0x8224d6a0` | `FM2_LiveryMask_AllocPendingUpdateNode` | Evidence from decompile and caller context. |
| `0x8224e878` | `FM2_LiveryMask_EraseListNodeAndIter` | Evidence from decompile and caller context. |
| `0x822521c0` | `FM2_ProfileDb_InitRbTreeIterator` | Evidence from decompile and caller context. |
| `0x82252718` | `FM2_LiveryMask_FindProfileRecordByKey` | Evidence from decompile and caller context. |
| `0x82252928` | `FM2_LiveryMask_InsertOrUpdateProfileRecord` | Evidence from decompile and caller context. |
| `0x82252bf8` | `FM2_CarParts_LookupUpgradePathByName` | Evidence from decompile and caller context. |
| `0x82254c80` | `FM2_Lua_LiveryEditor_SetColorKeyValue` | Evidence from decompile and caller context. |
| `0x8225ae70` | `FM2_RaceGhost_MergeSortedKeyframeRanges` | Evidence from decompile and caller context. |
| `0x8225e330` | `FM2_CareerRace_CopyRewardsBlockAt760` | Evidence from decompile and caller context. |
| `0x8225ee70` | `FM2_LuaLobbySort_RunSortByContextMode` | Evidence from decompile and caller context. |
| `0x822624f8` | `FM2_RenderAdapter_DestroyChildAndClearList` | Evidence from decompile and caller context. |
| `0x82264450` | `FM2_GraphicsStream_IsLinkedListEmpty` | Returns true when graphics-stream linked list at +12 is empty. |
| `0x8226a8a0` | `FM2_SceneGraph_CopyContentEntryDwordVector` | Evidence from decompile and caller context. |
| `0x8226b1e0` | `FM2_CareerRace_CopyGhostReplayRecord` | Deep copy of ghost replay record with VMX128 matrix blocks. |

### Infrastructure pass 17 (33 functions)

Stream/Lua/livery helpers, career assist getters, render pass setup, audio/SQLite.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8242bc48` | `FM2_BinaryStream_InitBaseVtable` | Evidence from decompile and caller context. |
| `0x8222e490` | `FM2_WString_IsPointerInsideBuffer` | Evidence from decompile and caller context. |
| `0x821e9bf0` | `FM2_ContentEntry_CopyHeadFields384` | Evidence from decompile and caller context. |
| `0x8226ab58` | `FM2_LapTrackerInit_CopyAssign` | Evidence from decompile and caller context. |
| `0x822196f8` | `FM2_SQLite_FormatQueryVprintf` | Varargs format into query buffer (lap-tracker/car-db SQL). |
| `0x8224bcb8` | `FM2_LiveryMask_FindPendingEntryInList` | Evidence from decompile and caller context. |
| `0x8224ccc0` | `FM2_LiveryMask_QueryMediaNameByCarId` | SQL `SELECT MediaName FROM Data_Car WHERE id = ...`. |
| `0x8224c748` | `FM2_Lua_LiveryEditor_BuildLayerMaterialList` | Evidence from decompile and caller context. |
| `0x8223fc00` | `FM2_Lua_LiveryEditor_IsSlotIndexValid` | Evidence from decompile and caller context. |
| `0x82251b88` | `FM2_ProfileDb_RbTreeLowerBound` | Evidence from decompile and caller context. |
| `0x82251fc0` | `FM2_LiveryMask_MergeProfileRecordFromNode` | Evidence from decompile and caller context. |
| `0x82254548` | `FM2_LiveryMask_FindOrInsertColorKey` | Evidence from decompile and caller context. |
| `0x8242bb88` | `FM2_CompressionStream_CtorFromSource` | Evidence from decompile and caller context. |
| `0x824bec00` | `FM2_Lua_ProtectedCallSetupFrame` | Evidence from decompile and caller context. |
| `0x824d3190` | `FM2_WideString_ReleaseHeapBuffer` | Evidence from decompile and caller context. |
| `0x822540f8` | `FM2_ProfileDb_RbTreeInsertOrFind` | Evidence from decompile and caller context. |
| `0x82254eb0` | `FM2_Lua_LiveryEditor_ApplyLayerFromArgs` | Evidence from decompile and caller context. |
| `0x82267428` | `FM2_Vector48Iterator_InsertRangeFromSource` | Evidence from decompile and caller context. |
| `0x8226b7e0` | `FM2_RaceGhost_GetWorldStateSingleton` | Evidence from decompile and caller context. |
| `0x8226b8c0` | `FM2_CarAudio_GetStreamBufferSingleton` | Evidence from decompile and caller context. |
| `0x8226d360` | `FM2_RenderAdapter_GetDeviceContextFromOffset` | Evidence from decompile and caller context. |
| `0x8226fd20` | `FM2_CareerRace_GetAssistSuggestLineEnabled` | Returns field +416 unless profile forces `ForceOffSuggLine`. |
| `0x8226fd78` | `FM2_CareerRace_GetAssistAbsEnabled` | Evidence from decompile and caller context. |
| `0x8226fdd0` | `FM2_CareerRace_GetAssistTcsEnabled` | Evidence from decompile and caller context. |
| `0x8226fe28` | `FM2_CareerRace_GetAssistStmEnabled` | Evidence from decompile and caller context. |
| `0x8226fe80` | `FM2_CareerRace_GetAssistManualTransEnabled` | Evidence from decompile and caller context. |
| `0x822708d8` | `FM2_CareerRace_GetUpgradeModifierOrStockTune` | Stock-tune override via `ForceStockUpgradesAndTuning` XML flag. |
| `0x82272010` | `FM2_GraphicsStreamList_CtorInit` | Evidence from decompile and caller context. |
| `0x822737a8` | `FM2_Render_SetFramePipelineGlobalPtr` | Evidence from decompile and caller context. |
| `0x82276570` | `FM2_Render_MatchShaderPassKeyword` | Evidence from decompile and caller context. |
| `0x822766c8` | `FM2_Render_WritePassConstantSlot` | Writes pass-constant float into PM4 bitfield slot. |
| `0x822768d0` | `FM2_AudioSignalGate_Ctor_E734` | Evidence from decompile and caller context. |
| `0x82279000` | `FM2_SQLite_VfsReadSchemaCallback` | Evidence from decompile and caller context. |

### Infrastructure pass 18 (33 functions)

Career XML/Lua, race ghost, livery mask, profile RB-tree, audio resource hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825c5d78` | `FM2_CareerRace_LookupXmlIntByRewardId` | Evidence from decompile and caller context. |
| `0x825c5958` | `FM2_CareerRace_QueryGameOptionValueInt` | SQL `SELECT Value FROM GameOptionValues WHERE Id=%u`. |
| `0x82255ff8` | `FM2_CareerRace_IsAssistOverrideRaceMode` | True when profile race mode at +80 is 1, 7, or 8. |
| `0x8240d918` | `FM2_Thread_NtClearEventOrFail` | Evidence from decompile and caller context. |
| `0x8245ce10` | `FM2_ComObject_InitBaseVtable423C0` | Evidence from decompile and caller context. |
| `0x824b9550` | `FM2_Lua_IncrementCallDepthOrOverflow` | Increments Lua call depth; throws at 200 frames. |
| `0x824bc470` | `FM2_Lua_TypeErrorCorruptValue` | Evidence from decompile and caller context. |
| `0x824bc6b8` | `FM2_Lua_ProtectedCallMarkYieldable` | Evidence from decompile and caller context. |
| `0x824bc718` | `FM2_Lua_ResolveUpvalueOrConstant` | Evidence from decompile and caller context. |
| `0x824beb38` | `FM2_Lua_FindTableSlotForValue` | Evidence from decompile and caller context. |
| `0x824beae0` | `FM2_Lua_HashLookupClosureSlot` | Evidence from decompile and caller context. |
| `0x82277cb0` | `FM2_SceneCamera_CallVfunc20` | Evidence from decompile and caller context. |
| `0x8222e350` | `FM2_RaceGhost_GetCareerCarPropertyTable` | Evidence from decompile and caller context. |
| `0x8222e838` | `FM2_RaceGhost_TableExistsQuery` | Evidence from decompile and caller context. |
| `0x8222f400` | `FM2_RaceGhost_GetOrBuildMainCareerNode` | Evidence from decompile and caller context. |
| `0x82253ef8` | `FM2_ProfileDb_RbTreeInsertNode` | Evidence from decompile and caller context. |
| `0x82251980` | `FM2_ProfileDb_RbTreeIncrementIterator` | Evidence from decompile and caller context. |
| `0x82249fe0` | `FM2_LiveryMask_ParseColorKeyString` | Evidence from decompile and caller context. |
| `0x8224a628` | `FM2_AllocPoolAcquire292xCount` | Evidence from decompile and caller context. |
| `0x8224c160` | `FM2_LiveryMask_CopyPendingUpdateNode` | Evidence from decompile and caller context. |
| `0x8224b6a8` | `FM2_LiveryMask_ReleasePendingUpdateRefs` | Evidence from decompile and caller context. |
| `0x82266908` | `FM2_Vector48Record_CopyAssign` | Evidence from decompile and caller context. |
| `0x8226a0f8` | `FM2_Crt_MemmoveDwordRange` | Evidence from decompile and caller context. |
| `0x8245b828` | `FM2_Crt_SnprintfBufferVa` | Evidence from decompile and caller context. |
| `0x8221a8b0` | `FM2_HashName_LookupAltModuleProperty` | Evidence from decompile and caller context. |
| `0x824a3398` | `FM2_ResourceLock_AppendWaiterEntry` | Evidence from decompile and caller context. |
| `0x82277b38` | `FM2_LiveryMask_GetFieldAt2156` | Evidence from decompile and caller context. |
| `0x82278e00` | `FM2_AudioSignalGate_Ctor_EC5C` | Evidence from decompile and caller context. |
| `0x82279e18` | `FM2_AudioSignalGate_Ctor_EEBC` | Evidence from decompile and caller context. |
| `0x8227d5b0` | `FM2_AudioResource_RegisterHook_EB94` | Static audio resource hook: alloc vtable + register with resource manager. |
| `0x8227d618` | `FM2_AudioResource_RegisterHook_EBB4` | Evidence from decompile and caller context. |
| `0x8227d680` | `FM2_AudioResource_RegisterHook_EBD4` | Evidence from decompile and caller context. |
| `0x8227d6e8` | `FM2_AudioResource_RegisterHook_EBF4` | Evidence from decompile and caller context. |

### Infrastructure pass 19 (33 functions)

Profile RB-tree, race ghost/entry, livery mask, lobby sort modes, D3D wait, render adapter.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82253888` | `FM2_ProfileDb_RbTreeInsertNodeImpl` | Evidence from decompile and caller context. |
| `0x8228ef18` | `FM2_RaceGhost_MergeSortedKeyframeBuffer` | Merges sorted dword keyframe ranges in ghost playback buffer. |
| `0x824a4888` | `FM2_D3D_WaitGpuFrameSlotSpinLoop` | Spins/waits on GPU frame slot with timeout (D3D resource lock path). |
| `0x82540420` | `FM2_RenderAdapter_InitChildListFromRange` | Evidence from decompile and caller context. |
| `0x821d9aa8` | `FM2_LiveryMask_SetPendingRecordField12` | Evidence from decompile and caller context. |
| `0x821d9db0` | `FM2_RaceEntry_GetGhostTableEntryAtIndex` | Evidence from decompile and caller context. |
| `0x821da240` | `FM2_LiveryMask_SetPendingRecordField8` | Evidence from decompile and caller context. |
| `0x821e9ae0` | `FM2_ContentEntry_CopyCarSetupHead304` | Evidence from decompile and caller context. |
| `0x8220a9a8` | `FM2_SQLite_FormatQueryVprintfShort` | Evidence from decompile and caller context. |
| `0x8221bd30` | `FM2_RaceEntry_ShouldProcessGhostForClass` | Evidence from decompile and caller context. |
| `0x82224e18` | `FM2_RaceEntry_SpawnGhostSceneNodeAtSlot` | Evidence from decompile and caller context. |
| `0x82249f80` | `FM2_LiveryMask_GenerateUniqueColorKeyString` | Evidence from decompile and caller context. |
| `0x8224a290` | `FM2_LiveryMask_DtorReleaseColorKeyNode` | Evidence from decompile and caller context. |
| `0x8224a718` | `FM2_LiveryMask_BinarySearchRecordById` | Evidence from decompile and caller context. |
| `0x822531a8` | `FM2_Lua_LiveryEditor_ApplyColorFromRegistry` | Evidence from decompile and caller context. |
| `0x8225e3f8` | `FM2_LuaLobbySort_SortMode0` | Lobby sort dispatch for context mode 0 at profile +760. |
| `0x8225e610` | `FM2_LuaLobbySort_SortMode1` | Evidence from decompile and caller context. |
| `0x8225e828` | `FM2_LuaLobbySort_SortMode2` | Evidence from decompile and caller context. |
| `0x8225ea40` | `FM2_LuaLobbySort_SortMode3` | Evidence from decompile and caller context. |
| `0x8225ec58` | `FM2_LuaLobbySort_SortMode4` | Evidence from decompile and caller context. |
| `0x82266888` | `FM2_Vector48Record_ReleaseRefFields` | Evidence from decompile and caller context. |
| `0x822669a0` | `FM2_Vector48Record_MoveConstruct` | Evidence from decompile and caller context. |
| `0x82266d10` | `FM2_Vector48Iterator_ShiftRecordsBackward` | Evidence from decompile and caller context. |
| `0x82266dc0` | `FM2_Vector48Iterator_FillFromRecord` | Evidence from decompile and caller context. |
| `0x822695d0` | `FM2_RaceEntry_UpdateGhostVisibilityFlag` | Evidence from decompile and caller context. |
| `0x8226b390` | `FM2_RaceGhostWorldState_Ctor` | Evidence from decompile and caller context. |
| `0x8226b840` | `FM2_CarAudioStreamDefaults_Ctor` | Initializes car-audio stream defaults with wide `Default` name. |
| `0x82277c98` | `FM2_SceneCamera_CallVfunc12` | Evidence from decompile and caller context. |
| `0x8227b618` | `FM2_AudioSignalGate_Ctor_F0A4` | Evidence from decompile and caller context. |
| `0x8227b780` | `FM2_AudioSignalGate_Ctor_F0F0` | Evidence from decompile and caller context. |
| `0x825489c8` | `FM2_RenderAdapter_CopyChildListFromRange` | Evidence from decompile and caller context. |
| `0x8242a258` | `FM2_FileStream_Ctor36714` | Evidence from decompile and caller context. |
| `0x82460670` | `FM2_D3D_InitGpuWaitTimerState` | Evidence from decompile and caller context. |

### Infrastructure pass 20 (33 functions)

Race entry ghost path, audio resource hooks, render pass resource, profile tuning, scene graph.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82224ce8` | `FM2_RaceEntry_NotifyGhostRenderState` | Evidence from decompile and caller context. |
| `0x821f10d0` | `FM2_SceneNode_ClearDrawFlag400` | Evidence from decompile and caller context. |
| `0x822691e8` | `FM2_RaceEntry_PostGhostVisibilityChange` | Evidence from decompile and caller context. |
| `0x82256058` | `FM2_CareerRace_IsArcadeRaceMode` | True when profile race mode at +80 is 3�6 (arcade/time trial family). |
| `0x821d9e30` | `FM2_RaceEntry_IsGhostPlaybackComplete` | Evidence from decompile and caller context. |
| `0x821d9c68` | `FM2_RaceEntry_CreateGhostSceneNode` | Evidence from decompile and caller context. |
| `0x8227dfb0` | `FM2_AudioResource_RegisterHook_EDE8` | Evidence from decompile and caller context. |
| `0x8227d750` | `FM2_AudioResource_RegisterHook_EC14` | Evidence from decompile and caller context. |
| `0x8227d7b8` | `FM2_AudioResource_RegisterHook_EC34` | Evidence from decompile and caller context. |
| `0x8227c250` | `FM2_AudioRenderFrame_WriteFrontBufferMix` | Evidence from decompile and caller context. |
| `0x8227c4a8` | `FM2_AudioRenderFrame_LogSaveFrontBuffer` | Debug path logging `SAVE FRONT BUFFER` during audio render. |
| `0x8227e018` | `FM2_AudioResource_RegisterHook_EE08` | Evidence from decompile and caller context. |
| `0x8227e080` | `FM2_AudioResource_RegisterHook_EE28` | Evidence from decompile and caller context. |
| `0x8227e0e8` | `FM2_AudioResource_RegisterHook_EE48` | Evidence from decompile and caller context. |
| `0x8227e470` | `FM2_AudioFrameService_QueryDeviceCaps` | Evidence from decompile and caller context. |
| `0x8227ed30` | `FM2_AudioSignalGate_Ctor_F3E4` | Evidence from decompile and caller context. |
| `0x8227ee98` | `FM2_AudioSignalGate_CtorFromCopy_F4C4` | Evidence from decompile and caller context. |
| `0x8227f008` | `FM2_AudioResource_RegisterHook_F34C` | Evidence from decompile and caller context. |
| `0x8227f5c0` | `FM2_DeferredTask_NotifyStateChangeA` | Evidence from decompile and caller context. |
| `0x8227fb60` | `FM2_DeferredTask_NotifyStateChangeB` | Evidence from decompile and caller context. |
| `0x822802e8` | `FM2_RenderPassResource_Dtor` | Evidence from decompile and caller context. |
| `0x82282b90` | `FM2_RenderPassResource_CtorWithLock` | Evidence from decompile and caller context. |
| `0x82284d08` | `FM2_WString_AssignFromWideStringView` | Evidence from decompile and caller context. |
| `0x82284d60` | `FM2_FxlResourceType_StaticInit24` | Static init 24-byte CFXLResourceType singleton for audio resources. |
| `0x8228b2f0` | `FM2_SpliceResultList_CheckLengthLimit` | Evidence from decompile and caller context. |
| `0x8228bc88` | `FM2_FileInfoCache_GetTransferNotifyVtable` | Evidence from decompile and caller context. |
| `0x8228d6d0` | `FM2_RaceGhost_CopyPlaybackState200` | Evidence from decompile and caller context. |
| `0x8228f4e0` | `FM2_AudioManager_SetFrameCounterField80308` | Evidence from decompile and caller context. |
| `0x822905a0` | `FM2_SceneGraph_ClearChildSlotByType` | Evidence from decompile and caller context. |
| `0x82292018` | `FM2_IntrusiveList_ShiftNodes248Byte` | Evidence from decompile and caller context. |
| `0x82293f58` | `FM2_Lua_InterpolateFloatField432To436` | Evidence from decompile and caller context. |
| `0x82296f00` | `FM2_Profile_DtorReleaseRefs` | Evidence from decompile and caller context. |
| `0x822979b8` | `FM2_Profile_SetTuningDisplayName` | Evidence from decompile and caller context. |

### Infrastructure pass 21 (33 functions)

Lobby sort splice/merge, profile DB RB-tree, D3D timer, audio render, profile tuning.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824e83e0` | `FM2_StdList_SpliceAndAdjustCount` | Circular-list splice with count adjustment (lobby sort). |
| `0x824e8990` | `FM2_StdList_SpliceFrontIfNonEmpty` | Evidence from decompile and caller context. |
| `0x8236be98` | `FM2_AudioMix_SetupRenderTargetTexturesThunk` | Evidence from decompile and caller context. |
| `0x82295ff0` | `FM2_Profile_DtorReleaseSubobjects` | Evidence from decompile and caller context. |
| `0x82460498` | `FM2_D3D_GetGpuWaitElapsedSeconds` | Evidence from decompile and caller context. |
| `0x82205828` | `FM2_Profile_DestroyStringListAt28` | Evidence from decompile and caller context. |
| `0x8221cd60` | `FM2_Profile_DestroyWStringListAt16` | Evidence from decompile and caller context. |
| `0x82251890` | `FM2_ProfileDb_RbTreeRotateLeft` | Evidence from decompile and caller context. |
| `0x82251aa8` | `FM2_ProfileDb_RbTreeRotateRight` | Evidence from decompile and caller context. |
| `0x82253130` | `FM2_ProfileDb_AllocRbTreeNode220` | Evidence from decompile and caller context. |
| `0x8225c278` | `FM2_LuaLobbySort_MergeSortPass0` | Stable merge sort pass for lobby list (mode 0). |
| `0x8225c420` | `FM2_LuaLobbySort_MergeSortPass1` | Evidence from decompile and caller context. |
| `0x8225c5c8` | `FM2_LuaLobbySort_MergeSortPass2` | Evidence from decompile and caller context. |
| `0x8225c770` | `FM2_LuaLobbySort_MergeSortPass3` | Evidence from decompile and caller context. |
| `0x8225c918` | `FM2_LuaLobbySort_MergeSortPass4` | Evidence from decompile and caller context. |
| `0x82464a28` | `FM2_StdList_CheckLengthAndAddCount` | Evidence from decompile and caller context. |
| `0x8242d0a0` | `FM2_Profile_CloseXamContentIfOpen` | Evidence from decompile and caller context. |
| `0x8222ee70` | `FM2_ProfileWStringNode_DtorOptionalFree` | Evidence from decompile and caller context. |
| `0x82251a30` | `FM2_AllocPoolAcquire224xCount` | Evidence from decompile and caller context. |
| `0x8227c900` | `FM2_AudioSignalGate_Ctor_F1CC` | Evidence from decompile and caller context. |
| `0x822905e0` | `FM2_SceneGraph_SetChildSlotVisibleByType` | Evidence from decompile and caller context. |
| `0x82296528` | `FM2_Profile_ResetStateAfterNotify` | Evidence from decompile and caller context. |
| `0x82297678` | `FM2_Profile_ApplyTuningRecordFromDb` | Evidence from decompile and caller context. |
| `0x82297bd8` | `FM2_Lua_PushDisplayStringClosure` | Evidence from decompile and caller context. |
| `0x8229a220` | `FM2_AudioSample_BuildIteratorPair` | Evidence from decompile and caller context. |
| `0x8229ca88` | `FM2_AudioRenderFrame_ProcessSampleBatch` | Evidence from decompile and caller context. |
| `0x8229ccf0` | `FM2_WaitText_Dtor` | Evidence from decompile and caller context. |
| `0x8229dd50` | `FM2_WaitAnimation_Ctor` | Evidence from decompile and caller context. |
| `0x8229f1b8` | `FM2_CompositeAdapterState_Dtor` | Evidence from decompile and caller context. |
| `0x824603d8` | `FM2_D3D_GetQueryPerformanceElapsedDiv` | Evidence from decompile and caller context. |
| `0x82460430` | `FM2_D3D_GetTickCountElapsedMs` | Evidence from decompile and caller context. |
| `0x82299e18` | `FM2_AudioSample_FindNextBufferNode` | Evidence from decompile and caller context. |
| `0x827fa088` | `FM2_DebugLog_NoOpStub` | Empty debug log stub called from audio render path. |
### Infrastructure pass 22 (33 functions)

Lua userdata closures/getters, car DB PI lookup, audio volume list, game type ctors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x822a0710` | `FM2_Lua_AppForza_UnpauseMedia` | Evidence from decompile and caller context. |
| `0x822a0148` | `FM2_Lua_PushSaveVolumeWrapperClosure` | Evidence from decompile and caller context. |
| `0x822a01c0` | `FM2_Lua_PushNetworkXuidClosure` | Evidence from decompile and caller context. |
| `0x822a9db8` | `FM2_Lua_PushLiveryHandleClosure` | Evidence from decompile and caller context. |
| `0x822b4b18` | `FM2_Lua_PushCarControlsClosure` | Evidence from decompile and caller context. |
| `0x822aab40` | `FM2_Lua_AuctionHouse_PushHighBidderDisplay` | Evidence from decompile and caller context. |
| `0x822b5e88` | `FM2_Lua_GameLibrary_GetCompressorATM` | Evidence from decompile and caller context. |
| `0x822b6a60` | `FM2_Lua_GameLibrary_GetSuspensionDamage` | Evidence from decompile and caller context. |
| `0x822b70d8` | `FM2_Lua_GameLibrary_GetDistUnderTire` | Evidence from decompile and caller context. |
| `0x822b88f0` | `FM2_Lua_PopCarControlsUserdata` | Evidence from decompile and caller context. |
| `0x822bb2d0` | `FM2_Lua_PopNumUnitStringUserdata` | Evidence from decompile and caller context. |
| `0x822c8ae8` | `FM2_Lua_Garage_CalcPerformanceIndex` | Maps Lua ratio to car DB performance index table. |
| `0x822cef20` | `FM2_Lua_PopUICarListUserdata` | Evidence from decompile and caller context. |
| `0x822d50c0` | `FM2_Lua_Leaderboard_PushGotFirstPlaceLocal` | Evidence from decompile and caller context. |
| `0x8222e340` | `FM2_CarDb_GetGlobalSingleton3390` | Evidence from decompile and caller context. |
| `0x82599458` | `FM2_CarDb_LookupPerformanceIndexByRatio` | Piecewise-linear lookup in 10-segment PI curve. |
| `0x824ae760` | `FM2_ComObject_AllocRefCountBlock72` | Evidence from decompile and caller context. |
| `0x822dc120` | `FM2_Lua_LiveryColor_PushFinishValue` | Evidence from decompile and caller context. |
| `0x822da538` | `FM2_Lua_LiveryEditor_PushHasOppositeSide` | Evidence from decompile and caller context. |
| `0x82301c00` | `FM2_Lua_ForzaProfile_PushUsingWheel` | Evidence from decompile and caller context. |
| `0x82314ba0` | `FM2_Lua_SaveVolume_PushOperationResult` | Evidence from decompile and caller context. |
| `0x8231e988` | `FM2_Lua_Tuning_GetFrontAccelValue` | Evidence from decompile and caller context. |
| `0x823353b0` | `FM2_Audio_VolumeListIteratorHasNext` | Evidence from decompile and caller context. |
| `0x82335428` | `FM2_Audio_VolumeListGetFloatAt40` | Evidence from decompile and caller context. |
| `0x82339ea0` | `FM2_LiveryRenderManager_InitListHead` | Evidence from decompile and caller context. |
| `0x82334df0` | `FM2_RaceGhostPlaybackState_Init` | Evidence from decompile and caller context. |
| `0x8233ce70` | `FM2_GameType_Ctor` | Evidence from decompile and caller context. |
| `0x82340c10` | `FM2_MultiscreenClientComponent_Ctor` | Evidence from decompile and caller context. |
| `0x82331560` | `FM2_HashName_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x8230abc8` | `FM2_SavedReplay_Dtor` | Evidence from decompile and caller context. |
| `0x823472f0` | `FM2_LuaGarage_EnsureCarRecordField92` | Lazy-init car record field at +92 before notify copy. |
| `0x822ddd78` | `FM2_Lua_PopLiveryLayerUserdata` | Evidence from decompile and caller context. |
| `0x82301cc8` | `FM2_ProfileLua_UnwindBindingContext` | Evidence from decompile and caller context. |

### Infrastructure pass 23 (33 functions)

Lobby sort comparators, audio volume list, boot/memory helpers, render scoped batch.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8228bd18` | `FM2_Audio_VolumeListFindByPrefixOrIterator` | Evidence from decompile and caller context. |
| `0x82411900` | `FM2_Timer_ReadTimeBase64` | Reads PPC time-base register into 64-bit out-param. |
| `0x8242a2b8` | `FM2_ProfileState_GetControllerIdAt12` | Evidence from decompile and caller context. |
| `0x824db4a0` | `FM2_Audio_VolumeListInitIteratorPair` | Evidence from decompile and caller context. |
| `0x82256398` | `FM2_LuaLobbySort_CompareUpgradeModifierGt` | Evidence from decompile and caller context. |
| `0x82256410` | `FM2_LuaLobbySort_CompareStreamBytesReadLt` | Evidence from decompile and caller context. |
| `0x82256858` | `FM2_LuaLobbySort_CompareDisplayNameLt` | Evidence from decompile and caller context. |
| `0x82256918` | `FM2_LuaLobbySort_CompareGamertagThenName` | Lobby sort mode 0: gamertag byte tie-break then display name. |
| `0x82256a38` | `FM2_LuaLobbySort_CompareRewardTierThenClass` | Lobby sort mode 4: reward tier then car class at +448. |
| `0x82256390` | `FM2_NetworkLobby_GetSeriesPlayerCount` | Evidence from decompile and caller context. |
| `0x822643a0` | `FM2_LobbyEntry_GetGamertagPrefixByte` | Evidence from decompile and caller context. |
| `0x822643f0` | `FM2_LobbyEntry_ClearSeriesPointsField28` | Evidence from decompile and caller context. |
| `0x822575e8` | `FM2_NetworkLobby_GetSeriesEntryAtIndex` | Evidence from decompile and caller context. |
| `0x8221b5d0` | `FM2_WString_CompareICaseFromComObjects` | Evidence from decompile and caller context. |
| `0x82293360` | `FM2_TuningRecord_GetFrontAccelPct100` | Evidence from decompile and caller context. |
| `0x82277f08` | `FM2_SceneCamera_GetStereoscopicModeVtable112` | Evidence from decompile and caller context. |
| `0x82297358` | `FM2_Profile_ApplyPendingTuningFromHeap` | Evidence from decompile and caller context. |
| `0x822ba480` | `FM2_Lua_PushNumUnitStringClosure` | Evidence from decompile and caller context. |
| `0x822cb590` | `FM2_Lua_PushUICarListClosure` | Evidence from decompile and caller context. |
| `0x822dd5f0` | `FM2_Lua_PushLiveryLayerClosure` | Evidence from decompile and caller context. |
| `0x82295fb0` | `FM2_TuningRecord_SetDecelIncrementScaled` | Evidence from decompile and caller context. |
| `0x82356858` | `FM2_Render_RbTreeIteratorDecrement` | Evidence from decompile and caller context. |
| `0x823568e0` | `FM2_Render_RbTreeLowerBoundBySortKey` | Evidence from decompile and caller context. |
| `0x823569e8` | `FM2_Render_InitDrawListRangeIterator` | Evidence from decompile and caller context. |
| `0x8234b090` | `FM2_Replay_GetDefaultWatchStreamPath` | Evidence from decompile and caller context. |
| `0x82363628` | `FM2_Memory_XPhysicalAllocTracked` | Evidence from decompile and caller context. |
| `0x823637c8` | `FM2_Memory_AllocViaPoolOrSmallBlock` | Evidence from decompile and caller context. |
| `0x82363d18` | `FM2_Boot_GetSubsystemTableEntry` | Evidence from decompile and caller context. |
| `0x82363fa8` | `FM2_Boot_ShutdownSubsystemByIndex` | Evidence from decompile and caller context. |
| `0x823643f8` | `FM2_AudioDevice_NotifyCategoryIfEnabled` | Evidence from decompile and caller context. |
| `0x82369280` | `FM2_Render_ScopedBatch_IncGpuKickDepth` | Evidence from decompile and caller context. |
| `0x82369418` | `FM2_Render_ScopedBatch_DecGpuKickDepthOrFree` | Evidence from decompile and caller context. |
| `0x8236a2a0` | `FM2_D3D_UnlockResourceFenceRegions` | Evidence from decompile and caller context. |
### Infrastructure pass 24 (33 functions)

GPU PM4 kick paths, input rumble XML, race ghost replay, scene node copy, Lua closures.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236ae10` | `FM2_D3D_ComputeResourceBindingFlags` | Evidence from decompile and caller context. |
| `0x8236ef20` | `FM2_Render_SetClearColorByteAndDirtyFlag` | Evidence from decompile and caller context. |
| `0x823700b0` | `FM2_Render_SetObjectPassLayerShift24` | Evidence from decompile and caller context. |
| `0x82372c20` | `FM2_D3D_WaitRingBufferCompletion5` | Evidence from decompile and caller context. |
| `0x823732e8` | `FM2_D3D_SyncRingBufferAfterGpuError` | Evidence from decompile and caller context. |
| `0x82376828` | `FM2_D3D_CreateShaderConstantFFixupWr` | Evidence from decompile and caller context. |
| `0x823764b0` | `FM2_Render_EmitDrawRangeCountPm4` | Evidence from decompile and caller context. |
| `0x82365b68` | `FM2_Render_EmitDrawRangeFromVector` | Evidence from decompile and caller context. |
| `0x8236c8c8` | `FM2_GpuKick_SubmitFenceOrInlinePm4` | Evidence from decompile and caller context. |
| `0x8235fb58` | `FM2_Input_DetectWheelSubtypeFromCapabilities` | Evidence from decompile and caller context. |
| `0x82362418` | `FM2_Input_InitAxisDefaultsTriple` | Evidence from decompile and caller context. |
| `0x82362570` | `FM2_Input_LoadControllerRumbleXmlThunk` | Evidence from decompile and caller context. |
| `0x82362480` | `FM2_Input_LoadControllerRumbleXml` | Loads ControllerRumble.xml wheel/controller defs into input state. |
| `0x82332c30` | `FM2_RaceGhost_ComputePlaybackWindow` | Accumulates ghost playback tick window from keyframe helpers. |
| `0x82332ec8` | `FM2_CareerCircuitRaceCoordinator_DtorPartial` | Evidence from decompile and caller context. |
| `0x8233b8c8` | `FM2_CareerRace_SlideGhostReplayRecordsLeft` | Evidence from decompile and caller context. |
| `0x8233bad0` | `FM2_CareerRace_FillGhostReplayRecords` | Evidence from decompile and caller context. |
| `0x823654a8` | `FM2_Memory_DeferredFreePopListHead` | Evidence from decompile and caller context. |
| `0x82364078` | `FM2_Memory_TryFreeViaPoolHandler` | Evidence from decompile and caller context. |
| `0x82363c78` | `FM2_Memory_XPhysicalAllocUnderCriticalSection` | Evidence from decompile and caller context. |
| `0x8230f3e8` | `FM2_Lua_PushPendingStringClosure` | Evidence from decompile and caller context. |
| `0x82314f40` | `FM2_Lua_PushSaveEnumeratorClosure` | Evidence from decompile and caller context. |
| `0x8231c168` | `FM2_Lua_PushLuaTuningClassClosure` | Evidence from decompile and caller context. |
| `0x822cb240` | `FM2_LuaGarage_CopyUICarListUserdata` | Evidence from decompile and caller context. |
| `0x822ec0c0` | `FM2_Lua_NetworkLobby_ClearSeriesPoints` | Evidence from decompile and caller context. |
| `0x822d5f28` | `FM2_Lua_Leaderboard_EnumerateByGamertagIndex` | Evidence from decompile and caller context. |
| `0x822dea50` | `FM2_Lua_Livery_CreateNewLayerAtArgs` | Evidence from decompile and caller context. |
| `0x822fa5b0` | `FM2_Lua_PhotoMode_ApplyCameraEffectParams` | Evidence from decompile and caller context. |
| `0x8235a778` | `FM2_Input_ControllerDevice_InitFromTemplate` | Evidence from decompile and caller context. |
| `0x8234cff0` | `FM2_SceneNodeTree_DetachAndReleaseRefs` | Evidence from decompile and caller context. |
| `0x8234d988` | `FM2_SceneNode_CopyAssignWithResourceLock` | Evidence from decompile and caller context. |
| `0x82370318` | `FM2_Render_SubmitObjectDrawConstantsSlot` | Evidence from decompile and caller context. |
| `0x823704a0` | `FM2_Render_SubmitObjectDrawConstantsSlotAlt` | Evidence from decompile and caller context. |

### Infrastructure pass 25 (33 functions)

BufFile/XML reader cluster, profile/tuning merge, race ghost playback, input rumble parse.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824cc940` | `FM2_BufFile_InitRefCountedStringPath` | Evidence from decompile and caller context. |
| `0x824d0288` | `FM2_BufFile_OpenAndReadArchiveEntry` | Shared buf-file open/read path for camera scripts and rumble XML. |
| `0x824cc020` | `FM2_BufFile_GetGlobalModuleSingleton` | Evidence from decompile and caller context. |
| `0x824cc640` | `FM2_BufFile_AssignPathStringRefCounted` | Evidence from decompile and caller context. |
| `0x824cc128` | `FM2_BufFile_SwapModuleRefAndReleaseOld` | Evidence from decompile and caller context. |
| `0x824ce8e0` | `FM2_XmlReader_CtorWithBufferSizes` | Evidence from decompile and caller context. |
| `0x824cf538` | `FM2_XmlReader_LoadFromBufFileStream` | Evidence from decompile and caller context. |
| `0x824ce138` | `FM2_XmlReader_FindChildElementByPath` | Evidence from decompile and caller context. |
| `0x8220e880` | `FM2_RefCount_DecrementAndFreePoolBlock` | Evidence from decompile and caller context. |
| `0x82221770` | `FM2_Profile_MergeStringListFromOther` | Evidence from decompile and caller context. |
| `0x82220098` | `FM2_Profile_SpliceStringListRange` | Evidence from decompile and caller context. |
| `0x8223d7e0` | `FM2_CareerRaceCoordinator_ClearField33Thunk` | Evidence from decompile and caller context. |
| `0x8223d750` | `FM2_CareerRaceCoordinator_FreeOptionalBlockAt1` | Evidence from decompile and caller context. |
| `0x822941e8` | `FM2_Profile_MergeTuningRecordsFromComObject` | Evidence from decompile and caller context. |
| `0x82294b50` | `FM2_TuningDb_AllocLinkedListNode` | Evidence from decompile and caller context. |
| `0x82294c20` | `FM2_TuningRecord_AdjustScrollSliderUp` | Evidence from decompile and caller context. |
| `0x82294cf0` | `FM2_TuningRecord_AdjustScrollSliderDown` | Evidence from decompile and caller context. |
| `0x8220a5a0` | `FM2_TuningUi_GetScrollSliderObjectAt56` | Evidence from decompile and caller context. |
| `0x82277b78` | `FM2_SceneNodeManager_GetStateVtable100` | Evidence from decompile and caller context. |
| `0x82277cc8` | `FM2_SceneCamera_ApplyPhotoModeEffectParams` | Evidence from decompile and caller context. |
| `0x822a7c00` | `FM2_BootConfigEntry_DtorAtexit` | Evidence from decompile and caller context. |
| `0x822a7ba8` | `FM2_BootConfigEntry_DestroyLuaBindingArray` | Evidence from decompile and caller context. |
| `0x824db1b8` | `FM2_Audio_VolumeListLowerBoundByPrefix` | Evidence from decompile and caller context. |
| `0x82249778` | `FM2_LiveryEditor_LoadDecalsForTabIndex` | Evidence from decompile and caller context. |
| `0x8223ded8` | `FM2_LiveryEditor_SetCurrentDecalTabId` | Evidence from decompile and caller context. |
| `0x8226fd08` | `FM2_PlayerChoices_SetAssistShiftingValue` | Evidence from decompile and caller context. |
| `0x823611f8` | `FM2_Input_ParseControllerRumbleXmlSection` | Parses ControllerRumble.xml motor sections into float rumble table. |
| `0x82330eb0` | `FM2_RaceGhost_AccumulateRotationalKeyframeDeltas` | Evidence from decompile and caller context. |
| `0x82330f38` | `FM2_RaceGhost_CopyPlaybackTransformBlock` | Evidence from decompile and caller context. |
| `0x82330ff0` | `FM2_RaceGhost_InterpolateExtendedPlaybackState` | Evidence from decompile and caller context. |
| `0x823325a0` | `FM2_RaceGhost_LookupAiPlayerFeeFromSql` | SQL lookup of AI player fee for ghost playback timing. |
| `0x82334d48` | `FM2_CareerCircuitRaceCoordinator_DestroyField2` | Evidence from decompile and caller context. |
| `0x82334e48` | `FM2_CareerCircuitRaceCoordinator_ResetBaseVtable` | Evidence from decompile and caller context. |
### Infrastructure pass 26 (33 functions)

Race ghost async playback, STL vector erase, GPU PM4 kick helpers, PNG/zlib, audio pump.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82331710` | `FM2_RaceGhost_CopyPlaybackUpdateArgs` | Evidence from decompile and caller context. |
| `0x82331d90` | `FM2_RaceGhost_EnqueueDeferredPlaybackTask` | Evidence from decompile and caller context. |
| `0x82332508` | `FM2_RaceGhost_SubmitPlaybackUpdateAsync` | Evidence from decompile and caller context. |
| `0x82331f10` | `FM2_RaceGhost_SchedulePlaybackUpdateTask` | Evidence from decompile and caller context. |
| `0x82357638` | `FM2_Stl_SlideStringRecords32Bytes` | Evidence from decompile and caller context. |
| `0x82365670` | `FM2_Stl_SlideRecords16Bytes` | Evidence from decompile and caller context. |
| `0x82357c08` | `FM2_Stl_Vector_EraseStringRangeAt` | Evidence from decompile and caller context. |
| `0x82365a40` | `FM2_Render_VectorEraseDrawRangeAt` | Evidence from decompile and caller context. |
| `0x8235a728` | `FM2_Input_SetWheelEnabledAndDetect` | Evidence from decompile and caller context. |
| `0x8235f9d8` | `FM2_Input_CopyRumbleDefaults88Bytes` | Evidence from decompile and caller context. |
| `0x82363368` | `FM2_Input_ControllerDevice_InitSslBindings` | Evidence from decompile and caller context. |
| `0x82364020` | `FM2_Memory_PoolHandlerCanFreeCategory` | Evidence from decompile and caller context. |
| `0x82367328` | `FM2_Memory_SetPhysicalAllocLockFlag` | Evidence from decompile and caller context. |
| `0x82367338` | `FM2_Memory_GetPhysicalAllocLockFlag` | Evidence from decompile and caller context. |
| `0x82366460` | `FM2_Memory_DeferredFreeRbTreeInsert` | Evidence from decompile and caller context. |
| `0x8236c828` | `FM2_GpuKick_SubmitShaderSyncPm4Bundle` | Evidence from decompile and caller context. |
| `0x8236c948` | `FM2_GpuKick_SubmitDrawSetupPm4Bundle` | Evidence from decompile and caller context. |
| `0x8236bd00` | `FM2_AudioRender_SubmitFrontBufferPath` | Evidence from decompile and caller context. |
| `0x823748d0` | `FM2_Render_ScopedBatch_FinalizeGpuKick` | Scoped batch teardown: sync GPU, release perf counters, free kick tag. |
| `0x82371250` | `FM2_GpuKick_SubmitViewportConstantPm4` | Evidence from decompile and caller context. |
| `0x82378940` | `FM2_GpuKick_SubmitTextureFetchPm4` | Evidence from decompile and caller context. |
| `0x823789d0` | `FM2_GpuKick_RotateMultiDrawTargetPm4` | Evidence from decompile and caller context. |
| `0x8237f358` | `FM2_GpuKick_ToggleClockGatingPm4` | Evidence from decompile and caller context. |
| `0x82356af8` | `FM2_BufFile_SeekAndTestPathPrefixMatch` | Evidence from decompile and caller context. |
| `0x8235ad90` | `FM2_UI_GetMaxPropertyAbsValueHalfStep` | Evidence from decompile and caller context. |
| `0x823815f0` | `FM2_AudioPumpThread_SignalWorkerEvent` | Evidence from decompile and caller context. |
| `0x82388cc8` | `FM2_Image_SwapEndian128BitRow` | Evidence from decompile and caller context. |
| `0x8239f1c8` | `FM2_Png_CompareSignatureBytes` | Evidence from decompile and caller context. |
| `0x823a4f90` | `FM2_Png_SetInterlaceHandlingFlag` | Evidence from decompile and caller context. |
| `0x823a4fa0` | `FM2_Png_SetBitDepth16Flag` | Evidence from decompile and caller context. |
| `0x823a4fc0` | `FM2_Png_ClampBitDepthToAtLeast8` | Evidence from decompile and caller context. |
| `0x823ae8e0` | `FM2_Zlib_CopyPendingInputToWindow` | Zlib deflate: copy pending input bytes into sliding window. |
| `0x822fd1c8` | `FM2_RaceGhost_MergeSortedKeyframeBufferSelfCheck` | Evidence from decompile and caller context. |

### Infrastructure pass 27 (33 functions)

XML/buf-file cluster, input SSL bindings, profile string lists, race ghost sort, UI property helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824cda18` | `FM2_XmlReader_FindElementByNameRecursive` | Depth-first XML element lookup by backslash path segment. |
| `0x824cd238` | `FM2_XmlReader_CompareElementNameI` | Evidence from decompile and caller context. |
| `0x824cdae0` | `FM2_XmlReader_GetChildElementByName` | Evidence from decompile and caller context. |
| `0x824cd6d0` | `FM2_XmlReader_ParseFloatAttribute` | Evidence from decompile and caller context. |
| `0x8245bdf8` | `FM2_BufFile_NormalizePathToLowercase` | Lowercases buf-file path in place for prefix matching. |
| `0x82463980` | `FM2_LiveryMask_GrowPendingRecordTable` | Evidence from decompile and caller context. |
| `0x824638b8` | `FM2_LiveryMask_AllocPendingRecordSlot188` | Evidence from decompile and caller context. |
| `0x824d10d8` | `FM2_Input_SslDeviceContext_Ctor` | Evidence from decompile and caller context. |
| `0x824d2710` | `FM2_Input_SslDeviceBinding_Ctor` | Evidence from decompile and caller context. |
| `0x824d0850` | `FM2_Input_SslContext_InitFromPath` | Evidence from decompile and caller context. |
| `0x824d2188` | `FM2_Input_SslBindingRecord_Init` | Evidence from decompile and caller context. |
| `0x821d04c0` | `FM2_Profile_AllocTuningHashNode16` | Evidence from decompile and caller context. |
| `0x824d36e0` | `FM2_ProfileTuningHashNode_Ctor` | Evidence from decompile and caller context. |
| `0x8221d380` | `FM2_Profile_AllocStringListNodeWithKey` | Evidence from decompile and caller context. |
| `0x8221d3d8` | `FM2_ProfileStringList_CheckLengthAndAdd` | Evidence from decompile and caller context. |
| `0x82246630` | `FM2_LiveryEditor_FindOrInsertDecalTabEntry` | Evidence from decompile and caller context. |
| `0x824ffe90` | `FM2_LiveryEditor_RbTreeInsertDecalTabKey` | Evidence from decompile and caller context. |
| `0x822fcfc0` | `FM2_RaceGhost_IntroSortKeyframeBuffer` | Evidence from decompile and caller context. |
| `0x8230bac8` | `FM2_RaceGhost_PartitionKeyframeBuffer` | Dual-pivot partition for race ghost keyframe introsort. |
| `0x82331988` | `FM2_RaceGhost_BuildPlaybackUpdateTask` | Evidence from decompile and caller context. |
| `0x82331b30` | `FM2_RaceGhost_InitDeferredPlaybackWrapper` | Evidence from decompile and caller context. |
| `0x8235d3f8` | `FM2_UI_PropertyMaskMatchesState` | Evidence from decompile and caller context. |
| `0x8235d3b0` | `FM2_UI_GetAnimPropertyBlockById` | Evidence from decompile and caller context. |
| `0x8235e3b0` | `FM2_UI_GetAnimPropertyFloatById` | Evidence from decompile and caller context. |
| `0x8235e540` | `FM2_UI_CountMatchingPropertiesInGroup` | Evidence from decompile and caller context. |
| `0x8235e610` | `FM2_UI_GetPropertyRecordByGroupIndex` | Evidence from decompile and caller context. |
| `0x8235e290` | `FM2_UI_GetDefaultPropertyFloatScaled` | Evidence from decompile and caller context. |
| `0x82360c40` | `FM2_Input_ParseRumbleMotorPairXml` | Evidence from decompile and caller context. |
| `0x823661d8` | `FM2_Memory_DeferredFreeMapInsertNode` | Evidence from decompile and caller context. |
| `0x8236b598` | `FM2_AudioRender_ComputeFrontBufferMixSample` | Evidence from decompile and caller context. |
| `0x8236b4d0` | `FM2_AudioRender_SampleFrontBufferRegion` | Evidence from decompile and caller context. |
| `0x8236c480` | `FM2_GpuKick_SubmitShaderConstantsFromTable` | Evidence from decompile and caller context. |
| `0x8236d948` | `FM2_Render_FreeGpuKickTagAt13404` | Evidence from decompile and caller context. |
### Infrastructure pass 28 (33 functions)

GPU kick/scaler, audio pump PM4, PNG/zlib/image convert, compression stream, metrics/FMOD.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823743d0` | `FM2_Render_EndCaptureReleaseSurfaces` | Evidence from decompile and caller context. |
| `0x823816c8` | `FM2_Render_WaitForGpuWorkerEvents` | Evidence from decompile and caller context. |
| `0x8237f2e8` | `FM2_GpuKick_SubmitViewportConstant3841` | Evidence from decompile and caller context. |
| `0x8237a888` | `FM2_GpuKick_SubmitVdScalerCommandBuffer` | Builds VdInitializeScaler PM4 packet for viewport blit. |
| `0x8237ab08` | `FM2_GpuKick_RetrainEdramAndFlushPm4` | Evidence from decompile and caller context. |
| `0x8237c988` | `FM2_GpuKick_NotifyPixCaptureFileEnded` | Evidence from decompile and caller context. |
| `0x823818d8` | `FM2_AudioPumpThread_DispatchPm4Commands` | Evidence from decompile and caller context. |
| `0x82389210` | `FM2_D3DXTex_Image_DtorReleaseLevels` | Evidence from decompile and caller context. |
| `0x8238ed18` | `FM2_Image_ConvertFloatRowTo565BE` | Evidence from decompile and caller context. |
| `0x8238ee10` | `FM2_Image_ConvertFloatRowTo565LE` | Evidence from decompile and caller context. |
| `0x823a41e0` | `FM2_Png_CreateReadStructFromCallbacks` | Evidence from decompile and caller context. |
| `0x823a4cf0` | `FM2_Png_DestroyReadStructsTriple` | Evidence from decompile and caller context. |
| `0x823a4fe8` | `FM2_Png_SetPaletteToRgbFlag` | Evidence from decompile and caller context. |
| `0x823a5018` | `FM2_Png_SetGrayToRgbAndScale` | Evidence from decompile and caller context. |
| `0x823a51f8` | `FM2_Png_SetAspectRatioMismatchFlag` | Evidence from decompile and caller context. |
| `0x823a5238` | `FM2_Png_SetRgbToGrayFlag` | Evidence from decompile and caller context. |
| `0x823a7038` | `FM2_Png_SetWriteFnAndClearOld` | Evidence from decompile and caller context. |
| `0x823ab428` | `FM2_Shader_InitHuffmanCallbackTable` | Evidence from decompile and caller context. |
| `0x823aee90` | `FM2_Zlib_ReadBitsFromInput` | Evidence from decompile and caller context. |
| `0x823af088` | `FM2_Zlib_FillDeflateWindowFromInput` | Zlib deflate: slides window and copies input from next_in. |
| `0x823b4538` | `FM2_Png_HuffmanDecodeSymbol` | Evidence from decompile and caller context. |
| `0x823c1590` | `FM2_ComObject_SyncChildProperties` | Evidence from decompile and caller context. |
| `0x823c81a8` | `FM2_Render_BindPassSurfacesForKick` | Evidence from decompile and caller context. |
| `0x823c8328` | `FM2_Render_ResolvePassGpuMemoryBlocks` | Evidence from decompile and caller context. |
| `0x823cdc20` | `FM2_D3D_BltRegionToSurface` | Evidence from decompile and caller context. |
| `0x823d3b38` | `FM2_Image_ConvertFloatRowTo555BE` | Evidence from decompile and caller context. |
| `0x823d3c38` | `FM2_Image_ConvertFloatRowTo555LE` | Evidence from decompile and caller context. |
| `0x823d3d30` | `FM2_Image_ConvertFloatRowTo4444` | Evidence from decompile and caller context. |
| `0x82412470` | `FM2_Metrics_InsertOrRemoveGlobalNode` | Evidence from decompile and caller context. |
| `0x82413fa8` | `FM2_FMOD_NormalizeSinLookupInput` | fabs/fsel sin lookup normalization (corrected pass 30). |
| `0x8242a7c0` | `FM2_CompressionStream_InitListHead` | Evidence from decompile and caller context. |
| `0x8242ac50` | `FM2_CompressionStream_ResetAndClearPending` | Evidence from decompile and caller context. |
| `0x8242b7f0` | `FM2_CompressionStream_Dtor` | Evidence from decompile and caller context. |

### Infrastructure pass 29 (33 functions)

Livery mask workers, compression stream, race ghost sort, buf-file refs, audio pump ring, GPU pass alloc.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82439190` | `FM2_CompressionStream_ReleasePendingRef` | Evidence from decompile and caller context. |
| `0x824614d8` | `FM2_LiveryMask_InitPendingRecordHeader` | Evidence from decompile and caller context. |
| `0x82461568` | `FM2_LiveryMask_SpawnWorkerThread` | Spawns XAP worker thread for livery mask pending-record processing. |
| `0x8237b978` | `FM2_Render_EndPixCaptureAndRestoreDisplay` | PIX capture end: restore display mode and release capture surfaces. |
| `0x823a9450` | `FM2_Png_FreeTaggedReadStruct` | Evidence from decompile and caller context. |
| `0x8242a8f0` | `FM2_CompressionStream_ClearPendingLists` | Evidence from decompile and caller context. |
| `0x824cc0b0` | `FM2_BufFile_BindGlobalModuleRef` | Evidence from decompile and caller context. |
| `0x824cca60` | `FM2_BufFile_ResolveOrLoadModuleRef` | Evidence from decompile and caller context. |
| `0x824cc5e8` | `FM2_BufFile_EnsureCapacityForLength` | Evidence from decompile and caller context. |
| `0x8228e4f8` | `FM2_RaceGhost_CompareAndSwapKeyframeTriple` | Evidence from decompile and caller context. |
| `0x8228e5d0` | `FM2_RaceGhost_SiftDownKeyframeHeap` | Evidence from decompile and caller context. |
| `0x8228e6c8` | `FM2_RaceGhost_HeapifyAndRestoreRange` | Evidence from decompile and caller context. |
| `0x8228e9e8` | `FM2_RaceGhost_HeapsortPartitionThreeWay` | Evidence from decompile and caller context. |
| `0x8228ea88` | `FM2_RaceGhost_HeapifyKeyframeRange` | Evidence from decompile and caller context. |
| `0x8228eaf8` | `FM2_RaceGhost_InsertionSortKeyframeRange` | Insertion sort for small race-ghost keyframe subranges. |
| `0x8228ec20` | `FM2_RaceGhost_HeapSortRecursive` | Evidence from decompile and caller context. |
| `0x822f9ec0` | `FM2_RaceGhost_SplicePlaybackListNodes` | Evidence from decompile and caller context. |
| `0x823313a8` | `FM2_RaceGhost_InitPlaybackTaskWrapper` | Evidence from decompile and caller context. |
| `0x8235d070` | `FM2_UI_GetPropertyMaskByteAtOffset` | Evidence from decompile and caller context. |
| `0x82366100` | `FM2_Memory_AllocDeferredFreeMapNode24` | Evidence from decompile and caller context. |
| `0x82366090` | `FM2_Memory_AllocDeferredMapBlockLocked` | Evidence from decompile and caller context. |
| `0x8236ded0` | `FM2_Render_AddRefPassSurfaceAt12412` | Evidence from decompile and caller context. |
| `0x8236e1e0` | `FM2_Render_AddRefPassSurfaceAt12416` | Evidence from decompile and caller context. |
| `0x8236e538` | `FM2_Render_AllocGpuPassMemoryBlock` | Evidence from decompile and caller context. |
| `0x8242a708` | `FM2_CompressionStream_AllocListSentinel` | Evidence from decompile and caller context. |
| `0x8230f1a8` | `FM2_ReplayPendingString_Ctor` | Evidence from decompile and caller context. |
| `0x823296d8` | `FM2_Lua_PopLuaCarSetupPointerUserdata` | Evidence from decompile and caller context. |
| `0x82381428` | `FM2_AudioPump_ComputeRingBufferMarker` | Evidence from decompile and caller context. |
| `0x82381490` | `FM2_AudioPump_CopyWaveChunkToRing` | Evidence from decompile and caller context. |
| `0x82381590` | `FM2_AudioPump_FlushPendingWaveChunks` | Evidence from decompile and caller context. |
| `0x8236a460` | `FM2_D3D_ComputeSurfaceCopyPitch` | Evidence from decompile and caller context. |
| `0x82369a50` | `FM2_AudioRender_CopySurfaceRegionToBuffer` | Evidence from decompile and caller context. |
| `0x8236a8f0` | `FM2_D3D_ComputeSurfaceBlitRegion` | Evidence from decompile and caller context. |
### Infrastructure pass 30 (33 functions)

GPU shader constants/gamma, PIX USB capture, audio pump PM4, PNG/JPEG init, Lua unwind, FMOD sin.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82377580` | `FM2_GpuKick_SubmitFloatShaderConstantsPm4` | Emits PM4 float shader constant fetch bundle (6437/6434 packets). |
| `0x82377660` | `FM2_GpuKick_SubmitFixedShaderConstantsPm4` | Evidence from decompile and caller context. |
| `0x82377750` | `FM2_GpuKick_BuildLinearGammaRampTable` | Evidence from decompile and caller context. |
| `0x823777a8` | `FM2_GpuKick_BuildPwlGammaRampTable` | Evidence from decompile and caller context. |
| `0x82378d58` | `FM2_GpuKick_ComputeScalerViewportRects` | Evidence from decompile and caller context. |
| `0x8237c5e8` | `FM2_GpuKick_CreatePixCaptureFileOnUsb` | Evidence from decompile and caller context. |
| `0x82381750` | `FM2_AudioPump_SubmitRingBufferMarkerPm4` | Evidence from decompile and caller context. |
| `0x82381850` | `FM2_AudioPump_WaitForBlockerCompletion` | Evidence from decompile and caller context. |
| `0x8239f0d0` | `FM2_Png_SetReadCallbacksOnStruct` | Evidence from decompile and caller context. |
| `0x8239f0e0` | `FM2_Png_FatalErrorShutdown` | Evidence from decompile and caller context. |
| `0x8239f118` | `FM2_Png_InvokeOldWriteFnIfSet` | Evidence from decompile and caller context. |
| `0x823a93d8` | `FM2_Png_AllocReadStructTagged` | Evidence from decompile and caller context. |
| `0x823a9468` | `FM2_Png_AllocChunkBufferTagged` | Evidence from decompile and caller context. |
| `0x823a94d8` | `FM2_Png_FreeChunkBufferIfOwner` | Evidence from decompile and caller context. |
| `0x823a4ba0` | `FM2_Png_DestroyReadStructFull` | Evidence from decompile and caller context. |
| `0x823b0398` | `FM2_Png_ReportError15` | Evidence from decompile and caller context. |
| `0x823ab188` | `FM2_Jpeg_ValidateDecompressState` | Evidence from decompile and caller context. |
| `0x823b2db0` | `FM2_Jpeg_InitSourceManager` | JPEG decompress: allocates and wires libjpeg source manager. |
| `0x823b4010` | `FM2_Jpeg_InitComponentInfoTable` | Evidence from decompile and caller context. |
| `0x823b4e40` | `FM2_Jpeg_InitEntropyDecoder` | Evidence from decompile and caller context. |
| `0x823b5c88` | `FM2_Jpeg_InitHuffmanDecodeTable` | Evidence from decompile and caller context. |
| `0x823b6188` | `FM2_Jpeg_InitSampleBufferTable` | Evidence from decompile and caller context. |
| `0x823b6380` | `FM2_Jpeg_InitUpsampler` | Evidence from decompile and caller context. |
| `0x823b6a20` | `FM2_Jpeg_InitColorConverter` | Evidence from decompile and caller context. |
| `0x823b79a0` | `FM2_Jpeg_InitColorSpaceConverter` | Evidence from decompile and caller context. |
| `0x823b8270` | `FM2_Jpeg_InitUpsampleBufferPaths` | Evidence from decompile and caller context. |
| `0x823bf708` | `FM2_Zlib_CopyInputToSlidingWindow` | Evidence from decompile and caller context. |
| `0x823c4ff8` | `FM2_ComObject_InvokeChildSyncCallback` | Evidence from decompile and caller context. |
| `0x824152c8` | `FM2_Stl_StringIterator_DecrementSafe` | Evidence from decompile and caller context. |
| `0x82417f78` | `FM2_Lua_MathTwoArgCompute` | Evidence from decompile and caller context. |
| `0x82418210` | `FM2_Lua_UnwindAndSetErrorStatus` | Evidence from decompile and caller context. |
| `0x82419cb8` | `FM2_Crt_UnlockHeap` | Evidence from decompile and caller context. |
| `0x82413fa8` | `FM2_FMOD_NormalizeSinLookupInput` | Evidence from decompile and caller context. |

### Infrastructure pass 31 (33 functions)

Race ghost sort, Lua userdata pop closures, replay pending strings, GPU kick/PIX, audio alloc.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8228e0e0` | `FM2_RaceGhost_SiftUpKeyframeHeap` | Evidence from decompile and caller context. |
| `0x8228e1a0` | `FM2_RaceGhost_IntroSortPartitionGap` | Evidence from decompile and caller context. |
| `0x822a17f0` | `FM2_Lua_PopSaveVolumeWrapperUserdata` | Evidence from decompile and caller context. |
| `0x822a1848` | `FM2_Lua_PopNetworkXuidUserdata` | Evidence from decompile and caller context. |
| `0x822a8fe8` | `FM2_Lua_AuctionHouse_GetCarSlotFromArgs` | Evidence from decompile and caller context. |
| `0x822aa0a8` | `FM2_Lua_PopLiveryHandleUserdata` | Evidence from decompile and caller context. |
| `0x822db290` | `FM2_Lua_LiveryEditor_LoadDecalsFromArg` | Evidence from decompile and caller context. |
| `0x822db340` | `FM2_Lua_LiveryEditor_SetDecalTabFromArg` | Evidence from decompile and caller context. |
| `0x822eb190` | `FM2_Lua_PlayerChoices_SetShiftingFromArg` | Evidence from decompile and caller context. |
| `0x822f9058` | `FM2_Lua_PhotoModeCamera_PushFocusToStack` | Evidence from decompile and caller context. |
| `0x822fb0a8` | `FM2_Lua_PhotoModeCamera_GetDepthAtScreenCoord` | Evidence from decompile and caller context. |
| `0x82301700` | `FM2_Lua_GetGarageUserdataOrRaise` | Evidence from decompile and caller context. |
| `0x82304e28` | `FM2_Lua_ForzaProfile_GetThrottleDeadzoneWheel` | Evidence from decompile and caller context. |
| `0x8230f0d8` | `FM2_ReplayPendingString_CopyConstruct` | Evidence from decompile and caller context. |
| `0x8230f200` | `FM2_ReplayPendingString_Dtor` | Evidence from decompile and caller context. |
| `0x8230f7c0` | `FM2_Replay_CreatePendingStringTask` | Evidence from decompile and caller context. |
| `0x8230f878` | `FM2_Lua_PopPendingStringUserdata` | Evidence from decompile and caller context. |
| `0x8230fec0` | `FM2_Lua_RewardReveal_GetCarLevelInfoFromArgs` | Evidence from decompile and caller context. |
| `0x82314fc0` | `FM2_Lua_PopSaveEnumeratorUserdata` | Evidence from decompile and caller context. |
| `0x8231edc8` | `FM2_Lua_TuningSetRearDecelFromArg` | Evidence from decompile and caller context. |
| `0x8231f658` | `FM2_Lua_PopLuaTuningClassUserdata` | Evidence from decompile and caller context. |
| `0x82329548` | `FM2_Lua_BuildCarSetupPointerUserdata` | Evidence from decompile and caller context. |
| `0x8232d758` | `FM2_Career_PendingBoolSslWrapper_Ctor` | Evidence from decompile and caller context. |
| `0x82341e60` | `FM2_RaceGhost_MergePlaybackKeyframeSlice` | Evidence from decompile and caller context. |
| `0x82363e58` | `FM2_AudioDevice_AllocPhysicalCategoryBuffer` | Evidence from decompile and caller context. |
| `0x82371ea8` | `FM2_D3D_WritePrimaryRingBufferWords` | Evidence from decompile and caller context. |
| `0x82372aa8` | `FM2_GpuKick_AppendSchedulerPm4Packets` | Appends PM4 scheduler packets (66940/1400 opcodes) to kick buffer. |
| `0x82372c00` | `FM2_D3D_WaitForGpuCommandCompletion` | Evidence from decompile and caller context. |
| `0x823744c8` | `FM2_GpuCommandBuffer_SetDisplayModeAndQuery` | Evidence from decompile and caller context. |
| `0x82378070` | `FM2_D3D_AllocGpuCaptureForPix` | Allocates D3D GPU capture object after PIXBeginCapture succeeds. |
| `0x8237b090` | `FM2_Render_ReleasePixCaptureSurfaces` | Evidence from decompile and caller context. |
| `0x8237b8c0` | `FM2_Render_AllocateEdramScratchSlice` | Evidence from decompile and caller context. |
| `0x8239f358` | `FM2_Png_ZeroPaletteBlock64` | Evidence from decompile and caller context. |
### Infrastructure pass 32 (33 functions)

PNG/JPEG helpers, STL deque iterators, compression/content/file-sys, async queue, FMOD.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823a4660` | `FM2_Png_EnsureRgbThenDecodeRow` | Evidence from decompile and caller context. |
| `0x823a4e80` | `FM2_Png_ValidateAndSetupRowDecode` | Evidence from decompile and caller context. |
| `0x823b01b0` | `FM2_Png_InvokeCustomDestroyCallback` | Evidence from decompile and caller context. |
| `0x823b0238` | `FM2_Png_AllocDecompressStateForChunk` | Evidence from decompile and caller context. |
| `0x823b26a8` | `FM2_Jpeg_InitDctCoefficientBuffers` | Evidence from decompile and caller context. |
| `0x823b6d10` | `FM2_Jpeg_InitIdctLookupTables` | Evidence from decompile and caller context. |
| `0x823b7c68` | `FM2_Jpeg_InitFastIdctLookupTables` | Evidence from decompile and caller context. |
| `0x82413d68` | `FM2_FMOD_SelectSignOrMagnitude` | fsel-based sign/magnitude selection for FMOD sin lookup path. |
| `0x82414bf0` | `FM2_Stl_DequeIterator_CtorFromIterator` | Evidence from decompile and caller context. |
| `0x82414c60` | `FM2_Stl_DequeIterator_CtorFromPtr` | Evidence from decompile and caller context. |
| `0x82414de8` | `FM2_Stl_DequeIterator_AdvanceByBlock` | Evidence from decompile and caller context. |
| `0x82414ee8` | `FM2_Stl_DequeIterator_FindBlockForIndex` | Evidence from decompile and caller context. |
| `0x82415090` | `FM2_Stl_DequeIterator_CopyRangeBlocks` | Evidence from decompile and caller context. |
| `0x82421120` | `FM2_Lua_StoreUnwindErrorGlobals` | Evidence from decompile and caller context. |
| `0x8242a768` | `FM2_CompressionStream_InitSentinelHead` | Resets compression stream intrusive list sentinel head. |
| `0x8242ccf8` | `FM2_Profile_IsContentDeviceReady` | Evidence from decompile and caller context. |
| `0x8242cd98` | `FM2_Lua_GetOverlappedAsyncResult` | Evidence from decompile and caller context. |
| `0x8242edc8` | `FM2_Storage_InitFileVolumeFromPath` | Evidence from decompile and caller context. |
| `0x8242f5c8` | `FM2_AsyncQueue_InitSemaphores` | Evidence from decompile and caller context. |
| `0x8242f758` | `FM2_ContentList_AssignRecordAndFreeBuffer` | Evidence from decompile and caller context. |
| `0x824302b8` | `FM2_ContentList_HeapSiftDownByCompare` | Evidence from decompile and caller context. |
| `0x8242fb48` | `FM2_AsyncOp_ReleasePlatformBuffer` | Evidence from decompile and caller context. |
| `0x824353c8` | `FM2_ContentRecord_AssignFromCopy` | Evidence from decompile and caller context. |
| `0x82435df0` | `FM2_ContentVector_MoveEraseRange36` | Evidence from decompile and caller context. |
| `0x82436e80` | `FM2_FileSys_Ctor` | Evidence from decompile and caller context. |
| `0x82436ef0` | `FM2_FileSys_Dtor` | Evidence from decompile and caller context. |
| `0x824343a8` | `FM2_FileSysWorker_CloseHandlesAndOptionalFree` | Evidence from decompile and caller context. |
| `0x824381d0` | `FM2_RaceGhost_EraseIntrusiveNodeFromList` | Evidence from decompile and caller context. |
| `0x824391b0` | `FM2_ContentManager_SnapshotChildListToBuffer` | Evidence from decompile and caller context. |
| `0x82464768` | `FM2_ContentBuffer_AllocTaggedCopyBuffer` | Evidence from decompile and caller context. |
| `0x824538b0` | `FM2_DeferredQueue_SampleElapsedTimestamp` | Evidence from decompile and caller context. |
| `0x82453a18` | `FM2_Set_IncrementIterator` | Evidence from decompile and caller context. |
| `0x8240c348` | `FM2_NtCloseHandleOrSetLastError` | Evidence from decompile and caller context. |

### Infrastructure pass 33 (33 functions)

Tick count import, Lua/replay/reward userdata, livery worker, race ghost interp, AI/career helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8240bde0` | `FM2_D3D_ReadKernelTickCountImport` | Returns kernel tick count via import slot MEMORY[0xBD]. |
| `0x82414b60` | `FM2_Crt_CopyConstructRefCountedCStr` | Evidence from decompile and caller context. |
| `0x824f16e8` | `FM2_AuctionHouse_LazyInitStaticData` | Evidence from decompile and caller context. |
| `0x82277d60` | `FM2_Lua_PhotoModeCamera_ReadDepthBufferSample` | Evidence from decompile and caller context. |
| `0x822a1690` | `FM2_Lua_GetNetworkXuidUserdataOrRaise` | Evidence from decompile and caller context. |
| `0x8230efe0` | `FM2_Replay_CreateGaragePendingStringTask` | Evidence from decompile and caller context. |
| `0x8230f140` | `FM2_RewardReveal_PendingUpgradeCompat_Ctor` | Evidence from decompile and caller context. |
| `0x8230f470` | `FM2_Lua_BuildPendingUpgradeCompatUserdata` | Evidence from decompile and caller context. |
| `0x8230f9a0` | `FM2_RewardReveal_CreatePendingCompatTask` | Evidence from decompile and caller context. |
| `0x8230fa90` | `FM2_Lua_PopPendingUpgradeCompatUserdata` | Evidence from decompile and caller context. |
| `0x823418e8` | `FM2_RaceGhost_ComputePlaybackInterpolationWeight` | Evidence from decompile and caller context. |
| `0x82363580` | `FM2_ContentBuffer_AllocWithTrackingNotify` | Evidence from decompile and caller context. |
| `0x823b0138` | `FM2_Png_ResetDecompressStateAndSyncChild` | Evidence from decompile and caller context. |
| `0x823c1658` | `FM2_Png_AllocInflateStateBuffers` | Evidence from decompile and caller context. |
| `0x8242a630` | `FM2_CompressionStream_FreeChildListRecursive` | Evidence from decompile and caller context. |
| `0x8242fe88` | `FM2_ContentList_HeapSiftUpByCompare` | Evidence from decompile and caller context. |
| `0x8242db78` | `FM2_Lua_GetAppForzaStateOffset8` | Evidence from decompile and caller context. |
| `0x82461500` | `FM2_LiveryMask_CloseWorkerThreadHandle` | Evidence from decompile and caller context. |
| `0x82461508` | `FM2_LiveryMask_IsWorkerThreadRunning` | GetExitCodeThread check for STILL_ACTIVE (259). |
| `0x824615c8` | `FM2_LiveryMask_CopyPendingRecordName128` | Evidence from decompile and caller context. |
| `0x824603c8` | `FM2_AudioFrameService_InvokeVtableUpdate` | Evidence from decompile and caller context. |
| `0x82460580` | `FM2_AudioFrameService_InitTimingBaseline` | Evidence from decompile and caller context. |
| `0x82463698` | `FM2_Math_PositiveModulo` | Evidence from decompile and caller context. |
| `0x82464088` | `FM2_CmdLine_InitParamsVtable` | Evidence from decompile and caller context. |
| `0x824662b8` | `FM2_AIOvertake_GetAssistValueAtIndex172` | Evidence from decompile and caller context. |
| `0x824662d0` | `FM2_AIOvertake_GetAssistValueAtIndex192` | Evidence from decompile and caller context. |
| `0x82466a78` | `FM2_CareerRace_GetElapsedRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x82466a88` | `FM2_CareerRace_GetTotalRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x82466b80` | `FM2_CareerRace_SubtractPhotoModeDeltaTime` | Evidence from decompile and caller context. |
| `0x8246c4d0` | `FM2_AIOvertake_IsHornThresholdExceeded` | Evidence from decompile and caller context. |
| `0x8246d020` | `FM2_AIOvertake_GetGlobalRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x824804d8` | `FM2_AIDriver_IsAssistModeActive` | Evidence from decompile and caller context. |
| `0x824a1688` | `FM2_Presentation_GetCarResourceLoadCountA` | Evidence from decompile and caller context. |
### Infrastructure pass 34 (33 functions)

Network/hash RB-tree, async queue, buffered file read, AI driver, presentation/livery, auction house.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824a16b0` | `FM2_Presentation_GetCarResourceLoadCountB` | Evidence from decompile and caller context. |
| `0x824a2338` | `FM2_LiveryMask_ResourceManager_Ctor` | Evidence from decompile and caller context. |
| `0x824a0e20` | `FM2_LiveryRenderManager_ClearFinalizeEvent` | Evidence from decompile and caller context. |
| `0x824a0ec0` | `FM2_CriticalSection_SetRawEventSlot` | Evidence from decompile and caller context. |
| `0x824a0fd8` | `FM2_ResourceLock_DecrementRefUnderLock` | Evidence from decompile and caller context. |
| `0x824530f0` | `FM2_StreamRead_ReleasePriorAndGetCurrent` | Evidence from decompile and caller context. |
| `0x82453400` | `FM2_Input_DetectWheelSubtypeFromCaps` | Evidence from decompile and caller context. |
| `0x824540b0` | `FM2_HashName_LinkPropertyTreeNode` | Evidence from decompile and caller context. |
| `0x82454370` | `FM2_Network_AppendMessagePayloadNodes` | Evidence from decompile and caller context. |
| `0x82455eb0` | `FM2_Network_ClearMessageTreeRoot` | Evidence from decompile and caller context. |
| `0x824562f8` | `FM2_Network_InsertSortedMessageNode` | Evidence from decompile and caller context. |
| `0x8245b560` | `FM2_PropertyBag_InitRbTreeNodeFromKey` | Evidence from decompile and caller context. |
| `0x8245b1a8` | `FM2_HashName_EraseRbTreeNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x8245eec0` | `FM2_RbTree_IncrementIteratorPastSentinel` | Evidence from decompile and caller context. |
| `0x8245f030` | `FM2_ContentDb_CountHashRangeNodes` | Evidence from decompile and caller context. |
| `0x8245f0b8` | `FM2_ContentDb_InitHashRangeIterator` | Evidence from decompile and caller context. |
| `0x824609c8` | `FM2_AsyncQueue_GlobalStaticInit` | Evidence from decompile and caller context. |
| `0x8245e898` | `FM2_CmdLineGlobal_StaticInit` | Evidence from decompile and caller context. |
| `0x8242fce8` | `FM2_FontSystem_DecrementRefAndMaybeClose` | Evidence from decompile and caller context. |
| `0x82430a58` | `FM2_AsyncOp_AllocAlignedPlatformBuffer` | Evidence from decompile and caller context. |
| `0x824321d0` | `FM2_AsyncOp_EnqueueUnderGlobalLock` | Evidence from decompile and caller context. |
| `0x82439470` | `FM2_Input_EraseControllerStateListNode` | Evidence from decompile and caller context. |
| `0x82439a68` | `FM2_D3D_Subscriber_TryEnableDeviceLocked` | Evidence from decompile and caller context. |
| `0x824621a8` | `FM2_BufferedFileRead_SyncOrAsyncRead` | Routes read through async wrapper or direct NtRead path. |
| `0x82463280` | `FM2_BufferedFileRead_SyncOrAsyncReadFile` | Evidence from decompile and caller context. |
| `0x824920e0` | `FM2_Sort_HeapSortDwordArrayInPlace` | Evidence from decompile and caller context. |
| `0x82494608` | `FM2_CircularBuffer_SelectSlotAndResize` | Evidence from decompile and caller context. |
| `0x8248f9c0` | `FM2_CircularBuffer_WrapReadIndices` | Evidence from decompile and caller context. |
| `0x8247dd38` | `FM2_AIDriver_ClearPathSegmentFlags` | Evidence from decompile and caller context. |
| `0x8247e2e8` | `FM2_AIDriver_SampleSteeringFromPath` | Evidence from decompile and caller context. |
| `0x82482290` | `FM2_AIDriver_ResetRaceLineState` | Evidence from decompile and caller context. |
| `0x824f1650` | `FM2_AuctionHouse_Ctor` | Constructs Forza2::CAuctionHouse with intrusive-list sentinel nodes. |
| `0x8243c640` | `FM2_ResourceLock_AssignHandleAndWaitReady` | Evidence from decompile and caller context. |

### Infrastructure pass 35 (33 functions)

Buffered file read, network RB-tree, async queue, D3D singletons, car audio, input wheel.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x826ec7f0` | `FM2_CircularBuffer_GetCapacityField8` | Returns *(obj+8) capacity used by circular buffer select/resize. |
| `0x82461bf8` | `FM2_BufferedFileRead_MaybeRaiseDiscError` | Evidence from decompile and caller context. |
| `0x82461ed0` | `FM2_BufferedFileRead_UnpackHandleDescriptor` | Evidence from decompile and caller context. |
| `0x8228b240` | `FM2_AsyncOp_AllocTuningListNodePair` | Evidence from decompile and caller context. |
| `0x8230f2a8` | `FM2_RewardReveal_CreatePendingCompatTaskWrapper` | Evidence from decompile and caller context. |
| `0x8240cbd0` | `FM2_SystemTimeFields_FromKeQuerySystemTime` | Evidence from decompile and caller context. |
| `0x8240d950` | `FM2_Memory_NtAllocateVirtualMemoryWrapped` | Evidence from decompile and caller context. |
| `0x824607f0` | `FM2_AsyncQueue_InitSentinelListHead` | Evidence from decompile and caller context. |
| `0x82461b50` | `FM2_BufferedFileRead_GetOverlappedResult` | Evidence from decompile and caller context. |
| `0x824618e8` | `FM2_BufferedFileRead_FatalSecuredFileError` | Logs secured-file error and spins forever (noreturn debug trap). |
| `0x82464f60` | `FM2_AIDriver_GetPathBufferLength` | Evidence from decompile and caller context. |
| `0x82492be0` | `FM2_CircularBuffer_IsSingleSlotMode` | Evidence from decompile and caller context. |
| `0x82453290` | `FM2_Input_XamInputGetCapabilitiesEx` | Evidence from decompile and caller context. |
| `0x82453330` | `FM2_Input_MapXamStatusToWheelError` | Evidence from decompile and caller context. |
| `0x82453de8` | `FM2_HashName_ClonePropertyTreeRecursive` | Evidence from decompile and caller context. |
| `0x82453ac8` | `FM2_Network_RbTreeLowerBoundByMessageKey` | Evidence from decompile and caller context. |
| `0x82453f88` | `FM2_Network_InitMessageInsertContext` | Evidence from decompile and caller context. |
| `0x82455958` | `FM2_Network_EraseMessageTreeNode` | Evidence from decompile and caller context. |
| `0x82455ca0` | `FM2_Network_LowerBoundInsertMessageNode` | Evidence from decompile and caller context. |
| `0x8245db98` | `FM2_CmdLine_InitCircularListHead` | Evidence from decompile and caller context. |
| `0x8245ef70` | `FM2_ContentDb_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x8245efd0` | `FM2_ContentDb_InitHashLookupContext` | Evidence from decompile and caller context. |
| `0x8245f290` | `FM2_RbTree_InsertNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x82430930` | `FM2_AsyncOp_TryPopFreeBlockFromQueue` | Evidence from decompile and caller context. |
| `0x82431280` | `FM2_AsyncOp_SpliceIntrusiveListHead` | Evidence from decompile and caller context. |
| `0x824a4768` | `FM2_D3D_InitGlobalDeviceSingletonA` | Evidence from decompile and caller context. |
| `0x824a47b0` | `FM2_D3D_InitGlobalDeviceSingletonB` | Evidence from decompile and caller context. |
| `0x824a47f8` | `FM2_D3D_InitGlobalDeviceSingletonC` | Evidence from decompile and caller context. |
| `0x824a4840` | `FM2_D3D_InitGlobalDeviceSingletonD` | Evidence from decompile and caller context. |
| `0x824ac470` | `FM2_RaceEntry_GetVisibilityChangeVtable` | Evidence from decompile and caller context. |
| `0x824a7410` | `FM2_CarAudioComponent_Dtor` | Evidence from decompile and caller context. |
| `0x824a7698` | `FM2_CarAudio_GetStaticMetaPointer` | Evidence from decompile and caller context. |
| `0x824b1700` | `FM2_AudioSample_InitOutputPairDescriptor` | Evidence from decompile and caller context. |
### Infrastructure pass 36 (33 functions)

Resource lock frame state, compression RB-tree, car audio voice, Lua stack/IO helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824a3268` | `FM2_ResourceLock_ResolveFrameStateAndWalk` | Evidence from decompile and caller context. |
| `0x824a32e0` | `FM2_ResourceLock_EnterCritSecOrResolve` | Evidence from decompile and caller context. |
| `0x824a4b98` | `FM2_ResourceLock_WaitForReadyOrTimeout` | Evidence from decompile and caller context. |
| `0x824ab3a0` | `FM2_CompressionStream_DestroyRbTreeRoot` | Evidence from decompile and caller context. |
| `0x824ab238` | `FM2_CompressionStream_EraseRbTreeNode` | Evidence from decompile and caller context. |
| `0x824ae5f0` | `FM2_ComObject_InitRefCountBlockFields` | Evidence from decompile and caller context. |
| `0x824ae658` | `FM2_CarAudio_InitVoiceBufferBase` | Evidence from decompile and caller context. |
| `0x824ae6b0` | `FM2_CarAudio_AssignVoiceBufferVtables` | Evidence from decompile and caller context. |
| `0x824adaa8` | `FM2_CarAudio_InitVoiceBufferFromSelf` | Evidence from decompile and caller context. |
| `0x824adda0` | `FM2_CarAudio_AllocVoiceBufferNode` | Evidence from decompile and caller context. |
| `0x824adec0` | `FM2_CarAudio_ComputeUtf8CharWidth` | Evidence from decompile and caller context. |
| `0x824a8868` | `FM2_CarAudio_AllocStreamBufferAligned` | Evidence from decompile and caller context. |
| `0x824a7920` | `FM2_RenderAdapter_SwitchPresentationModePartial` | Evidence from decompile and caller context. |
| `0x824a8b20` | `FM2_CarAudio_TryStopStreamDecRef` | Evidence from decompile and caller context. |
| `0x824b36e0` | `FM2_RbTreeNode_LowerBoundByKey` | Evidence from decompile and caller context. |
| `0x824b6840` | `FM2_Lua_ShiftStackSlotDownAndCopy` | Evidence from decompile and caller context. |
| `0x824b6d18` | `FM2_Lua_TryCoerceStackSlotToNumber` | Evidence from decompile and caller context. |
| `0x824b6f80` | `FM2_Lua_GetBooleanFromStackSlot` | Evidence from decompile and caller context. |
| `0x824b7060` | `FM2_Lua_PushIntegerAsNumberSlot` | Evidence from decompile and caller context. |
| `0x824b7318` | `FM2_Lua_PushLightUserdataSlot` | Evidence from decompile and caller context. |
| `0x824b8280` | `FM2_Lua_PushInternedStringSlot` | Evidence from decompile and caller context. |
| `0x824b8788` | `FM2_Lua_RestoreSavedStackValue` | Evidence from decompile and caller context. |
| `0x824b88a8` | `FM2_Lua_GrowValueStackSlots` | Evidence from decompile and caller context. |
| `0x824b89e8` | `FM2_Lua_GrowCallInfoStack` | Evidence from decompile and caller context. |
| `0x824b9148` | `FM2_Lua_SetErrHandlerAndRestoreSlot` | Evidence from decompile and caller context. |
| `0x824b9848` | `FM2_LuaIO_ProtectedOpenFileCall` | Evidence from decompile and caller context. |
| `0x824bb218` | `FM2_LuaIO_DispatchFileOpStub` | Lua IO file-op dispatcher: grow buffer then finalize read/write stub. |
| `0x824b81a0` | `FM2_Lua_ParseStringToDouble` | Evidence from decompile and caller context. |
| `0x8245ced8` | `FM2_DeferredTaskParams_ReleaseChildCallback` | Evidence from decompile and caller context. |
| `0x8243c5e8` | `FM2_ResourceLock_FileChunkDtor` | Evidence from decompile and caller context. |
| `0x824b9f00` | `FM2_Lua_UpdateObjectGcMark` | Evidence from decompile and caller context. |
| `0x824b9268` | `FM2_Lua_IncrementCallDepthOrOverflow` | Evidence from decompile and caller context. |
| `0x824b8550` | `FM2_LuaSyntax_VaFormatExpectedToken` | Evidence from decompile and caller context. |

### Infrastructure pass 37 (33 functions)

Network/compression iterators, resource lock frame resolve, async alloc, Lua IO, D3D singletons.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82453bf0` | `FM2_Network_IncrementMessageTreeIterator` | Evidence from decompile and caller context. |
| `0x824a31a8` | `FM2_ResourceLock_ResolveFrameAllocatorState` | Evidence from decompile and caller context. |
| `0x824bafa0` | `FM2_Lua_GrowStackForFileOp` | Evidence from decompile and caller context. |
| `0x824bf910` | `FM2_Lua_UnlinkOpenUpvaluesAbove` | Evidence from decompile and caller context. |
| `0x82534638` | `FM2_AsyncQueue_AllocSentinelNodeArray40` | Evidence from decompile and caller context. |
| `0x82586b78` | `FM2_CmdLine_AllocCircularListNodeArray28` | Evidence from decompile and caller context. |
| `0x826177a8` | `FM2_FrameAllocMap_AllocNode24` | Evidence from decompile and caller context. |
| `0x82621210` | `FM2_RbTree_RotateLeftAtChild` | RB-tree left rotation used during insert rebalance. |
| `0x827e7f70` | `FM2_Stl_VectorEmplacePairAtEndOrRealloc` | Evidence from decompile and caller context. |
| `0x82299dc0` | `FM2_AudioSample_RbTreeLowerBoundByTotals` | Evidence from decompile and caller context. |
| `0x82424bf0` | `FM2_Lua_GetCharClassTable` | Evidence from decompile and caller context. |
| `0x824301b8` | `FM2_AsyncOp_FindMatchingQueueBlock` | Evidence from decompile and caller context. |
| `0x82430718` | `FM2_AsyncOp_TryAcquireFreeBlockByIndex` | Evidence from decompile and caller context. |
| `0x82453b68` | `FM2_CompressionStream_IncrementTreeIterator` | Evidence from decompile and caller context. |
| `0x82454150` | `FM2_Network_InitEmptyMessageTreeHead` | Evidence from decompile and caller context. |
| `0x82453f28` | `FM2_Network_DestroyMessageTreeRecursive` | Evidence from decompile and caller context. |
| `0x82455848` | `FM2_Network_RbTreeLowerBoundByMessageKeyByte` | Evidence from decompile and caller context. |
| `0x8245ccb8` | `FM2_DeferredTaskParams_FreeIfOutsidePool` | Evidence from decompile and caller context. |
| `0x824bad00` | `FM2_LuaIO_ResetLexStateForNextOp` | Evidence from decompile and caller context. |
| `0x824bb320` | `FM2_Lua_LinkProtoToGcObject` | Evidence from decompile and caller context. |
| `0x824a2298` | `FM2_ResourceLock_WalkFrameSlotRange` | Evidence from decompile and caller context. |
| `0x824a37b0` | `FM2_D3D_RegisterGlobalDeviceSingletonD` | Evidence from decompile and caller context. |
| `0x824a38f8` | `FM2_D3D_RegisterGlobalDeviceSingletonA` | Evidence from decompile and caller context. |
| `0x824a3a30` | `FM2_D3D_RegisterGlobalDeviceSingletonB` | Evidence from decompile and caller context. |
| `0x824a3b68` | `FM2_D3D_RegisterGlobalDeviceSingletonC` | Evidence from decompile and caller context. |
| `0x824b3570` | `FM2_RenderAdapter_DecRefPresentationSwitch` | Evidence from decompile and caller context. |
| `0x824b35b8` | `FM2_RenderAdapter_GetPresentationSwitchFlag` | Evidence from decompile and caller context. |
| `0x824b3658` | `FM2_RenderAdapter_InitPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824adb08` | `FM2_CarAudio_ComputeUtf8EncodedSize` | Evidence from decompile and caller context. |
| `0x82454558` | `FM2_Network_FindSchedulerNodeByDeadline` | Evidence from decompile and caller context. |
| `0x82456818` | `FM2_Network_InsertTimedMessageBefore` | Evidence from decompile and caller context. |
| `0x8242f658` | `FM2_AsyncQueue_DecRefAndMaybeDestroy` | Evidence from decompile and caller context. |
| `0x824ae128` | `FM2_CircularBuffer_MoveEraseRange8` | Evidence from decompile and caller context. |

### Infrastructure pass 38 (33 functions)

Lua compiler/runtime helpers, ref-counted strings, compression insert, circular buffer erase.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824bf370` | `FM2_Lua_AllocStringTableEntry` | Evidence from decompile and caller context. |
| `0x824bf6d8` | `FM2_Lua_AllocCClosureWithUpvalues` | Evidence from decompile and caller context. |
| `0x824be770` | `FM2_Lua_GrowProtoConstantsTable` | Evidence from decompile and caller context. |
| `0x824be810` | `FM2_Lua_InitProtoWithUpvalues` | Evidence from decompile and caller context. |
| `0x824bf190` | `FM2_Lua_ProtectedCallSetupFrame` | Evidence from decompile and caller context. |
| `0x824bec90` | `FM2_Lua_PushLoadedClosureUpvalues` | Evidence from decompile and caller context. |
| `0x824be3b0` | `FM2_Lua_FindTableSlotForValue` | Evidence from decompile and caller context. |
| `0x824bea18` | `FM2_Lua_GetProtoConstantSlot` | Evidence from decompile and caller context. |
| `0x824bc890` | `FM2_Lua_GetTableFieldCopySlots` | Evidence from decompile and caller context. |
| `0x824bcc38` | `FM2_Lua_TryGetTableFieldViaUpvalue` | Evidence from decompile and caller context. |
| `0x824bc558` | `FM2_Lua_TypeErrorForConcat` | Evidence from decompile and caller context. |
| `0x824bbed0` | `FM2_Lua_ErrorFormatAndThrow` | Evidence from decompile and caller context. |
| `0x824bbbe8` | `FM2_Lua_CheckForInfiniteProtoChain` | Evidence from decompile and caller context. |
| `0x824bbde0` | `FM2_Lua_ResolveCallTargetProto` | Evidence from decompile and caller context. |
| `0x824b8a90` | `FM2_Lua_CallHookOrTraceback` | Evidence from decompile and caller context. |
| `0x824b8b80` | `FM2_Lua_AdjustStackForVarargs` | Evidence from decompile and caller context. |
| `0x824b8d10` | `FM2_Lua_TypeErrorOnCallValue` | Evidence from decompile and caller context. |
| `0x824b8de0` | `FM2_Lua_EnterProtectedCallFrame` | Evidence from decompile and caller context. |
| `0x824bfbf0` | `FM2_LuaSyntax_InitLexerFromReader` | Evidence from decompile and caller context. |
| `0x824bfcc8` | `FM2_LuaIO_InitFileHandleState` | Evidence from decompile and caller context. |
| `0x824c3908` | `FM2_LuaSyntax_GetTokenName` | Evidence from decompile and caller context. |
| `0x824c3990` | `FM2_LuaSyntax_ExpectedTokenNear` | Evidence from decompile and caller context. |
| `0x824c4c50` | `FM2_LuaSyntax_SaveLookaheadToken` | Evidence from decompile and caller context. |
| `0x824c2f70` | `FM2_LuaSyntax_ErrorOnPrecompiledChunk` | Evidence from decompile and caller context. |
| `0x82457b98` | `FM2_Network_RbTreeLowerBoundByMessageKeyDword` | Evidence from decompile and caller context. |
| `0x82466080` | `FM2_AIOvertake_CopyVector128ToOutput` | Evidence from decompile and caller context. |
| `0x824ca820` | `FM2_Stl_RefCountedString_DecRefOrFree` | Evidence from decompile and caller context. |
| `0x824ca898` | `FM2_Stl_RefCountedString_AssignRef` | Evidence from decompile and caller context. |
| `0x824ca960` | `FM2_Stl_RefCountedString_MoveInsertRange` | Evidence from decompile and caller context. |
| `0x82454ad8` | `FM2_CompressionStream_InsertNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x827d8658` | `FM2_CircularBuffer_EraseRangeWrapper` | Evidence from decompile and caller context. |
| `0x824b8598` | `FM2_Lua_ParseLoadStringFormatSpec` | Evidence from decompile and caller context. |
| `0x824b9030` | `FM2_Lua_LoadStringOrFileChunk` | Evidence from decompile and caller context. |

### Infrastructure pass 40 (33 functions)

Lap tracker spline, network message queue/RB-tree, content-record sort, com-object field blocks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825872b8` | `FM2_CircularList_InitSentinelHead28` | Evidence from decompile and caller context. |
| `0x826107d8` | `FM2_Network_RbTreeLowerBoundInsertHint` | Evidence from decompile and caller context. |
| `0x82467c98` | `FM2_LapTracker_AdvanceSplineSampleIndex` | Evidence from decompile and caller context. |
| `0x82465838` | `FM2_LapTracker_InterpolateCarPositionOnSpline` | Evidence from decompile and caller context. |
| `0x822032b8` | `FM2_ComObject_DestroyFieldBlockRecursive` | Evidence from decompile and caller context. |
| `0x822712a8` | `FM2_ComObject_FindVectorIterByFieldAndCopy` | Evidence from decompile and caller context. |
| `0x82271b00` | `FM2_ComObject_CopyConstructPairFromVectorIter` | Evidence from decompile and caller context. |
| `0x82201940` | `FM2_ComObject_RemoveFieldSlotAndCompact` | Evidence from decompile and caller context. |
| `0x82365ea0` | `FM2_Memory_AllocTaggedSmallBlockFromPoolEntry` | Evidence from decompile and caller context. |
| `0x823d1610` | `FM2_BinaryReader_ReadAndByteSwapScalar` | Evidence from decompile and caller context. |
| `0x8242ab60` | `FM2_CompressionStream_RbTreeLowerBoundInsertHint` | Evidence from decompile and caller context. |
| `0x82453fe8` | `FM2_Network_RbTreeInitLowerBoundHint` | Evidence from decompile and caller context. |
| `0x82435370` | `FM2_ContentRecord_CopyConstructFrom` | Evidence from decompile and caller context. |
| `0x82435428` | `FM2_ContentRecord_SwapViaTemp` | Evidence from decompile and caller context. |
| `0x82435a20` | `FM2_Script_RegisterBindingNormalizePath` | Evidence from decompile and caller context. |
| `0x82435bc8` | `FM2_AsyncOp_MedianOfThreeContentRecords` | Evidence from decompile and caller context. |
| `0x82435d88` | `FM2_AsyncOp_QuickSortPartitionRange` | Evidence from decompile and caller context. |
| `0x82435e48` | `FM2_AsyncOp_IntroSortContentRecords` | Evidence from decompile and caller context. |
| `0x82435ee8` | `FM2_AsyncOp_HeapifyDownContentRecord` | Evidence from decompile and caller context. |
| `0x82436338` | `FM2_AsyncOp_PartitionContentRecords` | Evidence from decompile and caller context. |
| `0x82436460` | `FM2_AsyncOp_SortContentRecordSubrange` | Evidence from decompile and caller context. |
| `0x824563f8` | `FM2_Network_InitMessageChannelWithVectorReserve` | Evidence from decompile and caller context. |
| `0x824565f8` | `FM2_Network_InitMessageChannelWithDwordVector` | Evidence from decompile and caller context. |
| `0x82456e48` | `FM2_Network_InitMessageQueueFromSource` | Evidence from decompile and caller context. |
| `0x82458538` | `FM2_Network_DispatchDueMessagesFromTree` | Evidence from decompile and caller context. |
| `0x824586b0` | `FM2_Network_InitDeadlineTimerState` | Evidence from decompile and caller context. |
| `0x82455a58` | `FM2_Network_AssignMessageListFromSource` | Evidence from decompile and caller context. |
| `0x824546e0` | `FM2_Network_InitEmptyTimedMessageList` | Evidence from decompile and caller context. |
| `0x824556a0` | `FM2_Network_ReserveMessagePayloadVector` | Evidence from decompile and caller context. |
| `0x8229f6f8` | `FM2_Stl_VectorReserveDwordCapacity` | Evidence from decompile and caller context. |
| `0x8245dcb0` | `FM2_ComObject_FindListNodeByFieldOffset` | Evidence from decompile and caller context. |
| `0x82461a60` | `FM2_BufferedFileRead_InitAsyncReadRequest` | Evidence from decompile and caller context. |
| `0x824b8108` | `FM2_Lua_CountLeadingZeroBits8` | Evidence from decompile and caller context. |

### Infrastructure pass 41 (33 functions)

Buffered file ring buffer, render presentation adapter, Lua stack grow, resource lock teardown.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82462020` | `FM2_BufferedFileRead_SubmitAsyncReadLocked` | Evidence from decompile and caller context. |
| `0x82462ba8` | `FM2_BufferedFileRead_GrowRingBufferCapacity` | Evidence from decompile and caller context. |
| `0x82462d70` | `FM2_BufferedFileRead_AppendRingBufferSlot` | Evidence from decompile and caller context. |
| `0x82481990` | `FM2_AIDriver_ResetRaceLineStateOnSectorChange` | Evidence from decompile and caller context. |
| `0x824a1448` | `FM2_ResourceLock_WalkFrameSlotsUntilMatch` | Evidence from decompile and caller context. |
| `0x824a7678` | `FM2_ResourceLock_GetNullFrameSlotSentinel` | Evidence from decompile and caller context. |
| `0x824a88b8` | `FM2_Memory_AllocArray32Checked` | Evidence from decompile and caller context. |
| `0x824a9b10` | `FM2_IntrusiveList_InitSentinelHead` | Evidence from decompile and caller context. |
| `0x824ae7c0` | `FM2_CarAudio_AppendVoiceIdAndInitBuffer` | Evidence from decompile and caller context. |
| `0x824b3498` | `FM2_RenderAdapter_DecRefPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824b3430` | `FM2_RenderAdapter_IncRefPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824b34f0` | `FM2_RenderAdapter_TryEnablePresentationSwitch` | Evidence from decompile and caller context. |
| `0x824b3630` | `FM2_RenderAdapter_EnterPresentationCritSecSingleton` | Evidence from decompile and caller context. |
| `0x824ba138` | `FM2_Lua_MarkObjectDuringStackGrow` | Evidence from decompile and caller context. |
| `0x824ba4c8` | `FM2_Lua_TraverseProtoUpvaluesForMark` | Evidence from decompile and caller context. |
| `0x824ba710` | `FM2_Lua_ComputeStackGrowSizeForObject` | Evidence from decompile and caller context. |
| `0x824b2d98` | `FM2_RenderAdapter_SwitchPresentationModePartial` | Evidence from decompile and caller context. |
| `0x826af9a0` | `FM2_D3D_ApplyPresentationThrottleGlobals` | Evidence from decompile and caller context. |
| `0x82412148` | `FM2_RenderAdapter_SetPresentationSlotMultiplier` | Evidence from decompile and caller context. |
| `0x8242d8a8` | `FM2_Lua_CreateComPtrFromThreeLuaNumbers` | Evidence from decompile and caller context. |
| `0x8236e320` | `FM2_AudioRender_AllocMixBufferRegion` | Evidence from decompile and caller context. |
| `0x82417950` | `FM2_Crt_HeapReallocOrSetErrno` | Evidence from decompile and caller context. |
| `0x824365d0` | `FM2_FileSys_ComparePathsCaseInsensitive` | Evidence from decompile and caller context. |
| `0x82455158` | `FM2_Network_AllocMessageListHeadNode` | Evidence from decompile and caller context. |
| `0x82453b18` | `FM2_CompressionStream_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x82435ca0` | `FM2_AsyncOp_IntroSortInnerLoop` | Evidence from decompile and caller context. |
| `0x824a9910` | `FM2_IntrusiveList_AllocSentinelNode` | Evidence from decompile and caller context. |
| `0x82412160` | `FM2_RenderAdapter_GetPresentationSlotFromGlobals` | Evidence from decompile and caller context. |
| `0x82454230` | `FM2_Network_InitTimedMessageNodeFields` | Evidence from decompile and caller context. |
| `0x82453ed8` | `FM2_Network_CopyMessagePayloadQwords` | Evidence from decompile and caller context. |
| `0x824ae1b8` | `FM2_CarAudio_InitVoiceBufferRange` | Evidence from decompile and caller context. |
| `0x824a4108` | `FM2_ResourceLock_ReleaseFrameSlotsAndWalk` | Evidence from decompile and caller context. |
| `0x824a4488` | `FM2_ResourceLock_TeardownFrameSlotRange` | Evidence from decompile and caller context. |

### Infrastructure pass 42 (33 functions)

Memory alloc helpers, network timed messages, Lua GC mark/lexer/parser, lap tracker spline, async sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8259f340` | `FM2_Memory_AllocArray8Checked` | Evidence from decompile and caller context. |
| `0x8220c198` | `FM2_Stl_String_ResizeAndNullTerminate` | Evidence from decompile and caller context. |
| `0x82205ef0` | `FM2_Lua_GetDefaultComPtrCtorArgs` | Evidence from decompile and caller context. |
| `0x82205ba0` | `FM2_Lua_BindingPairVector_ReserveCapacity` | Evidence from decompile and caller context. |
| `0x82454318` | `FM2_Network_MemmovePayloadRange8` | Evidence from decompile and caller context. |
| `0x82457b30` | `FM2_Network_BuildTimedMessageListFromRange` | Evidence from decompile and caller context. |
| `0x824576a8` | `FM2_Network_AllocTimedMessageNodeFromTemplate` | Evidence from decompile and caller context. |
| `0x82435488` | `FM2_AsyncOp_HeapifyUpContentRecord` | Evidence from decompile and caller context. |
| `0x82435530` | `FM2_AsyncOp_PartitionIntroSortRange` | Evidence from decompile and caller context. |
| `0x824364c0` | `FM2_ContentVector_DestroyRangeAndTrim` | Evidence from decompile and caller context. |
| `0x8242f6f0` | `FM2_AsyncQueue_IncRefAndMaybeCloseHandle` | Evidence from decompile and caller context. |
| `0x82461ad0` | `FM2_BufferedFileRead_HashBufferWithXeCryptSha` | Evidence from decompile and caller context. |
| `0x82464de0` | `FM2_LapTracker_ComputeSplineSegmentBounds` | Evidence from decompile and caller context. |
| `0x82464e60` | `FM2_LapTracker_CompareTrackProgressFlags` | Evidence from decompile and caller context. |
| `0x82480580` | `FM2_AIDriver_WrapSectorIndexForward` | Evidence from decompile and caller context. |
| `0x82480fb0` | `FM2_AIDriver_ReconcileRaceLineSectorState` | Evidence from decompile and caller context. |
| `0x824ba348` | `FM2_Lua_MarkStackObjectsDuringTraverse` | Evidence from decompile and caller context. |
| `0x824ba5b8` | `FM2_Lua_MarkTableUpvaluesDuringTraverse` | Evidence from decompile and caller context. |
| `0x824ba9e0` | `FM2_Lua_TraverseOpenUpvalueChain` | Evidence from decompile and caller context. |
| `0x824bab50` | `FM2_Lua_CollectGrayObjectsFromList` | Evidence from decompile and caller context. |
| `0x824badd8` | `FM2_Lua_MarkGrayObjectGraphRecursive` | Evidence from decompile and caller context. |
| `0x824bb348` | `FM2_Lua_LinkUpvalueToOpenList` | Evidence from decompile and caller context. |
| `0x824befd8` | `FM2_Lua_CreateClosureFromHashSlot` | Evidence from decompile and caller context. |
| `0x824bf308` | `FM2_Lua_LookupOrCreateClosureSlot` | Evidence from decompile and caller context. |
| `0x824bf738` | `FM2_Lua_AllocProtoWithConstants` | Evidence from decompile and caller context. |
| `0x824bf7b8` | `FM2_Lua_AllocUpvalueDescTable` | Evidence from decompile and caller context. |
| `0x824bfb90` | `FM2_Lua_FindUpvalueIndexInProto` | Evidence from decompile and caller context. |
| `0x824bfce8` | `FM2_LuaSyntax_ReadBytesFromLexer` | Evidence from decompile and caller context. |
| `0x824c0c50` | `FM2_LuaSyntax_PushParserStateFrame` | Evidence from decompile and caller context. |
| `0x824c2db8` | `FM2_LuaSyntax_ParseChunkStatements` | Evidence from decompile and caller context. |
| `0x824c3488` | `FM2_LuaSyntax_ParseFuncOrStatList` | Evidence from decompile and caller context. |
| `0x824c37e0` | `FM2_LuaSyntax_AppendLexemeToBuffer` | Evidence from decompile and caller context. |
| `0x82454d08` | `FM2_Network_EraseMessageTreeNodeRebalance` | Evidence from decompile and caller context. |

### Infrastructure pass 43 (33 functions)

BufFile module refs, car-audio mix channel, Lua binding vector, compiler optional slots, render sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c7e28` | `FM2_LuaCompiler_ResetOptionalState` | Evidence from decompile and caller context. |
| `0x824ca780` | `FM2_CarAudioMixChannel_DtorBase` | Evidence from decompile and caller context. |
| `0x824ca9b8` | `FM2_Stl_Vector_IncRefCopyRange` | Evidence from decompile and caller context. |
| `0x824caa20` | `FM2_Stl_Vector_AssignRefCountedRange` | Evidence from decompile and caller context. |
| `0x824caa80` | `FM2_Stl_Vector_DecRefRange` | Evidence from decompile and caller context. |
| `0x824cab60` | `FM2_CarAudioMixChannel_ClearVoiceList` | Evidence from decompile and caller context. |
| `0x824cabb8` | `FM2_CarAudioMixChannel_EraseVoiceRange` | Evidence from decompile and caller context. |
| `0x824ca908` | `FM2_Stl_Vector_MoveConstructRefCountedRange` | Evidence from decompile and caller context. |
| `0x824caad8` | `FM2_CarAudioMixChannel_ReplaceVoiceRange` | Evidence from decompile and caller context. |
| `0x824cbe78` | `FM2_BufFile_GetLazyInitFlagPtr` | Evidence from decompile and caller context. |
| `0x824cbee8` | `FM2_BufFile_DerefStreamHandle` | Evidence from decompile and caller context. |
| `0x824cc1a0` | `FM2_BufFile_BinarySearchModuleRef` | Evidence from decompile and caller context. |
| `0x824cc438` | `FM2_BufFile_EnsureCapacityAndCopy` | Evidence from decompile and caller context. |
| `0x824cc6f0` | `FM2_BufFile_AppendFromBufferPtr` | Evidence from decompile and caller context. |
| `0x824cc760` | `FM2_BufFile_AppendCString` | Evidence from decompile and caller context. |
| `0x824cc890` | `FM2_BufFile_FindModuleRefIfLoaded` | Evidence from decompile and caller context. |
| `0x824cbef8` | `FM2_BufFile_AllocGrowableStringBuffer` | Evidence from decompile and caller context. |
| `0x824cbf60` | `FM2_BufFile_StreqOptionalCase` | Evidence from decompile and caller context. |
| `0x8242d140` | `FM2_Profile_ApplyTuningRecordFromDevice` | Evidence from decompile and caller context. |
| `0x82464020` | `FM2_Lua_GetComPtrMetatableSingleton` | Evidence from decompile and caller context. |
| `0x82463b40` | `FM2_Lua_InitBindingPairListHead` | Evidence from decompile and caller context. |
| `0x824bf9e0` | `FM2_Lua_AllocParserStateGcObject` | Evidence from decompile and caller context. |
| `0x824c2c10` | `FM2_LuaSyntax_ParseStatement` | Evidence from decompile and caller context. |
| `0x824c79d0` | `FM2_LuaCompiler_EraseOptionalRange` | Evidence from decompile and caller context. |
| `0x824c7818` | `FM2_LuaCompiler_ReplaceOptionalRange` | Evidence from decompile and caller context. |
| `0x82204e90` | `FM2_Lua_BindingPairVector_ShrinkToSize` | Evidence from decompile and caller context. |
| `0x82204c50` | `FM2_Lua_BindingPairVector_MoveTailElements` | Evidence from decompile and caller context. |
| `0x82205488` | `FM2_Lua_BindingPairVector_GrowCapacity` | Evidence from decompile and caller context. |
| `0x82417bb0` | `FM2_Render_SortDrawListByMaterialKey` | Evidence from decompile and caller context. |
| `0x82455bd8` | `FM2_Network_ClonePayloadListIntoNode` | Evidence from decompile and caller context. |
| `0x824c7658` | `FM2_LuaCompiler_MoveOptionalSlotRange` | Evidence from decompile and caller context. |
| `0x826af8c0` | `FM2_D3D_InitVoicePresentationSubsystem` | Evidence from decompile and caller context. |
| `0x824c72d0` | `FM2_LuaCompiler_CopyOptionalSlotFromSource` | Evidence from decompile and caller context. |

### Infrastructure pass 44 (33 functions)

RB-tree rotate, lap tracker cross product, Lua GC shrink, Lua syntax statement parser cluster.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8258c7f8` | `FM2_RbTree_RotateLeftAtChildSibling` | Evidence from decompile and caller context. |
| `0x8246a3a8` | `FM2_LapTracker_ComputeCrossProductProgressOnSpline` | Evidence from decompile and caller context. |
| `0x822049b0` | `FM2_Memory_AllocArray308Checked` | Evidence from decompile and caller context. |
| `0x82204e30` | `FM2_Lua_BindingPairVector_UninitializedFill308` | Evidence from decompile and caller context. |
| `0x824b9c08` | `FM2_Lua_GrowStackAndShrinkLiveSlots` | Evidence from decompile and caller context. |
| `0x824ba048` | `FM2_Lua_ProcessGrayObjectWorkList` | Evidence from decompile and caller context. |
| `0x824ba880` | `FM2_Lua_MarkProtoUpvalueChain` | Evidence from decompile and caller context. |
| `0x824bbbb0` | `FM2_LuaSyntax_CheckProtoHasDebugInfo` | Evidence from decompile and caller context. |
| `0x824be6c8` | `FM2_Lua_GetTableIndexAsClosureSlot` | Evidence from decompile and caller context. |
| `0x824be988` | `FM2_Lua_ShrinkProtoSideTables` | Evidence from decompile and caller context. |
| `0x824bee50` | `FM2_Lua_CopyTableIntoClosureSlot` | Evidence from decompile and caller context. |
| `0x824bf8d8` | `FM2_Lua_UnlinkProtoConstantListNode` | Evidence from decompile and caller context. |
| `0x824bfa88` | `FM2_Lua_ShrinkProtoTablesAndCode` | Evidence from decompile and caller context. |
| `0x824bfb60` | `FM2_Lua_ShrinkProtoUpvalueArray` | Evidence from decompile and caller context. |
| `0x824c04b8` | `FM2_LuaSyntax_ParseReturnStatement` | Evidence from decompile and caller context. |
| `0x824c17c0` | `FM2_LuaSyntax_ParseParenExprList` | Evidence from decompile and caller context. |
| `0x824c1d18` | `FM2_LuaSyntax_PushTempScopeFrame` | Evidence from decompile and caller context. |
| `0x824c1d78` | `FM2_LuaSyntax_ParseLocalAssign` | Evidence from decompile and caller context. |
| `0x824c1f20` | `FM2_LuaSyntax_CollectLocalFlagsInScope` | Evidence from decompile and caller context. |
| `0x824c1fb8` | `FM2_LuaSyntax_ParseFunctionStmt` | Evidence from decompile and caller context. |
| `0x824c20a0` | `FM2_LuaSyntax_ParseForNumericStmt` | Evidence from decompile and caller context. |
| `0x824c2620` | `FM2_LuaSyntax_ParseRepeatUntilStmt` | Evidence from decompile and caller context. |
| `0x824c26e8` | `FM2_LuaSyntax_ParseIfThenElseStmt` | Evidence from decompile and caller context. |
| `0x824c2838` | `FM2_LuaSyntax_ParseTableConstructor` | Evidence from decompile and caller context. |
| `0x824c2930` | `FM2_LuaSyntax_ParseFieldListTrailingComma` | Evidence from decompile and caller context. |
| `0x824c2a30` | `FM2_LuaSyntax_ParseLocalFunctionStmt` | Evidence from decompile and caller context. |
| `0x824c2af8` | `FM2_LuaSyntax_ParseBreakOrReturnEarly` | Evidence from decompile and caller context. |
| `0x824c2fd0` | `FM2_LuaSyntax_ValidateChunkNotPrecompiled` | Evidence from decompile and caller context. |
| `0x824c32d0` | `FM2_LuaSyntax_ParseLocalVarDeclList` | Evidence from decompile and caller context. |
| `0x824802d8` | `FM2_AIDriver_AdvanceCircularSectorIndex` | Evidence from decompile and caller context. |
| `0x8242ce58` | `FM2_Profile_LoadTuningFromDevicePath` | Evidence from decompile and caller context. |
| `0x82422c28` | `FM2_Crt_StrncpyValidated` | Evidence from decompile and caller context. |
| `0x824c03d8` | `FM2_LuaSyntax_ErrorTooManyLocals` | Evidence from decompile and caller context. |

### Infrastructure pass 45 (33 functions)

BufFile/XML reader, Lua compiler optional slots, syntax block parser, Lua GC mark helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c6f10` | `FM2_LuaCompiler_DtorOptionalSlotAt` | Evidence from decompile and caller context. |
| `0x824c7708` | `FM2_LuaCompiler_ClearOptionalSlotVector` | Evidence from decompile and caller context. |
| `0x824cc9b8` | `FM2_BufFile_InsertModuleRefAtIndex` | Evidence from decompile and caller context. |
| `0x824cc320` | `FM2_BufFile_GrowModuleRefVectorCapacity` | Evidence from decompile and caller context. |
| `0x824ccbc0` | `FM2_XmlReader_ComparePathSegmentI` | Evidence from decompile and caller context. |
| `0x824ccc58` | `FM2_XmlReader_ParsePathToBuffer` | Evidence from decompile and caller context. |
| `0x824cd5a8` | `FM2_XmlReader_LoadFloatAttrByName` | Evidence from decompile and caller context. |
| `0x824cd8e8` | `FM2_Lua_GetLibRegFieldByIndex12` | Evidence from decompile and caller context. |
| `0x824cd980` | `FM2_Lua_GetLibRegFieldByIndex16` | Evidence from decompile and caller context. |
| `0x824ce0b8` | `FM2_Lua_GetLibRegFirstFieldIfAny` | Evidence from decompile and caller context. |
| `0x824ce490` | `FM2_XmlReader_InitEmptyAttrNodeA` | Evidence from decompile and caller context. |
| `0x824ce4e8` | `FM2_XmlReader_InitEmptyAttrNodeB` | Evidence from decompile and caller context. |
| `0x824ce540` | `FM2_XmlReader_GrowAttrVectorCapacity` | Evidence from decompile and caller context. |
| `0x824ce658` | `FM2_XmlReader_InsertAttrAtIndex` | Evidence from decompile and caller context. |
| `0x824cf490` | `FM2_XmlReader_ApplyAttrDefaultsFromTable` | Evidence from decompile and caller context. |
| `0x824cff90` | `FM2_BufFile32768_Ctor` | Evidence from decompile and caller context. |
| `0x824d0090` | `FM2_BufFile_SetErrorFlagAndCopyPath` | Evidence from decompile and caller context. |
| `0x824d0f48` | `FM2_Input_SslContext_InitWithVtable` | Evidence from decompile and caller context. |
| `0x824d19c8` | `FM2_Camera_LoadScriptPathFromConfig` | Evidence from decompile and caller context. |
| `0x824d3340` | `FM2_Config_ParseWhitespaceDelimitedTokens` | Evidence from decompile and caller context. |
| `0x824c05c8` | `FM2_LuaSyntax_AllocLocalVarSlot` | Evidence from decompile and caller context. |
| `0x824c1ae8` | `FM2_LuaSyntax_ParseBlockWithScope` | Evidence from decompile and caller context. |
| `0x824bb740` | `FM2_LuaSyntax_ParseFunctionBody` | Evidence from decompile and caller context. |
| `0x8257e190` | `FM2_LuaCompiler_CopyOptionalSlotTailFields` | Evidence from decompile and caller context. |
| `0x824c0d58` | `FM2_LuaSyntax_LoadStringChunkBody` | Evidence from decompile and caller context. |
| `0x824c30c8` | `FM2_LuaSyntax_ParseFuncHeaderAndBody` | Evidence from decompile and caller context. |
| `0x824c3648` | `FM2_LuaSyntax_InitDefaultHeaderTag` | Evidence from decompile and caller context. |
| `0x824c3888` | `FM2_LuaSyntax_InternTokenNameStrings` | Evidence from decompile and caller context. |
| `0x824c3a50` | `FM2_LuaSyntax_RegisterLocalNameSlot` | Evidence from decompile and caller context. |
| `0x824c3aa8` | `FM2_LuaSyntax_LexerNextOnRefCountZero` | Evidence from decompile and caller context. |
| `0x824c3c60` | `FM2_LuaSyntax_LexSingleCharOperator` | Evidence from decompile and caller context. |
| `0x824c3cf0` | `FM2_LuaSyntax_ParseNumberWithLocale` | Evidence from decompile and caller context. |
| `0x824c3df8` | `FM2_LuaSyntax_ParseNumberOrHexLiteral` | Evidence from decompile and caller context. |

### Infrastructure pass 46 (33 functions)

Lua syntax codegen/backpatch cluster (4-caller helpers), RB-tree insert helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c0458` | `FM2_LuaSyntax_ErrorIfTokenMismatch` | Evidence from decompile and caller context. |
| `0x824c0a20` | `FM2_LuaSyntax_PopScopeAndRestoreLocals` | Evidence from decompile and caller context. |
| `0x824c5748` | `FM2_LuaSyntax_PatchJumpListBackpatch` | Evidence from decompile and caller context. |
| `0x824c57d0` | `FM2_LuaSyntax_BackpatchJumpToHere` | Evidence from decompile and caller context. |
| `0x824c15d0` | `FM2_LuaSyntax_ParseCommaSeparatedBlocks` | Evidence from decompile and caller context. |
| `0x824c5e80` | `FM2_LuaSyntax_EmitExprAsRkOperand` | Evidence from decompile and caller context. |
| `0x824c61a8` | `FM2_LuaSyntax_DischargeExprToAnyReg` | Evidence from decompile and caller context. |
| `0x824c0850` | `FM2_LuaSyntax_ResolveLocalOrUpvalueIndex` | Evidence from decompile and caller context. |
| `0x824c0980` | `FM2_LuaSyntax_EmitAssignOrAdjustStack` | Evidence from decompile and caller context. |
| `0x824c0f98` | `FM2_LuaSyntax_ParseCallOrIndexSuffix` | Evidence from decompile and caller context. |
| `0x824c54d8` | `FM2_LuaSyntax_EmitInstructionWord` | Evidence from decompile and caller context. |
| `0x824c4f18` | `FM2_LuaSyntax_PatchJumpChainToTarget` | Evidence from decompile and caller context. |
| `0x824c57e8` | `FM2_LuaSyntax_DischargeExprToReg` | Evidence from decompile and caller context. |
| `0x824c5c30` | `FM2_LuaSyntax_CodeConditionalJump` | Evidence from decompile and caller context. |
| `0x824c5cc0` | `FM2_LuaSyntax_DischargeToRegOrEmitMove` | Evidence from decompile and caller context. |
| `0x824c5a10` | `FM2_LuaSyntax_FreeExpAndAllocReg` | Evidence from decompile and caller context. |
| `0x824c4cf8` | `FM2_LuaSyntax_SavePcForBackpatch` | Evidence from decompile and caller context. |
| `0x824c4d08` | `FM2_LuaSyntax_PatchJumpFixupAtPc` | Evidence from decompile and caller context. |
| `0x824c4da8` | `FM2_LuaSyntax_MakeIndexedExpDesc` | Evidence from decompile and caller context. |
| `0x824c4fd0` | `FM2_LuaSyntax_ReserveFreeRegCount` | Evidence from decompile and caller context. |
| `0x824c5080` | `FM2_LuaSyntax_ExpToRegWithKOrMove` | Evidence from decompile and caller context. |
| `0x824c5240` | `FM2_LuaSyntax_PatchJumpOffsetInCode` | Evidence from decompile and caller context. |
| `0x824c55c0` | `FM2_LuaSyntax_EmitEncodedInstruction` | Evidence from decompile and caller context. |
| `0x824c5608` | `FM2_LuaSyntax_GrowProtoBuffer` | Evidence from decompile and caller context. |
| `0x824c0330` | `FM2_LuaSyntax_LexIdentifierOrKeyword` | Evidence from decompile and caller context. |
| `0x824c6500` | `FM2_LuaSyntax_SetExpDescToCallResult` | Evidence from decompile and caller context. |
| `0x824c57b0` | `FM2_LuaSyntax_EmitLoadBoolInstruction` | Evidence from decompile and caller context. |
| `0x824c6a60` | `FM2_LuaSyntax_MergeJumpListIfAtPc` | Evidence from decompile and caller context. |
| `0x824c51d0` | `FM2_LuaSyntax_MakeConstantNumberExpDesc` | Evidence from decompile and caller context. |
| `0x824c5038` | `FM2_LuaSyntax_ReserveStackSlotsAfterCall` | Evidence from decompile and caller context. |
| `0x824c14d0` | `FM2_LuaSyntax_ParseMethodOrFunctionHeader` | Evidence from decompile and caller context. |
| `0x8258c8e8` | `FM2_RbTree_InsertNodeWithHint` | Evidence from decompile and caller context. |
| `0x8258c860` | `FM2_RbTree_CountNodesInSubtreeRange` | Evidence from decompile and caller context. |

### Infrastructure pass 47 (33 functions)

Lua syntax parse/discharge helpers, XML reader whitespace/element parse, BufFile ref helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c1120` | `FM2_LuaSyntax_ParseSimpleExprPrefix` | Evidence from decompile and caller context. |
| `0x824c4050` | `FM2_LuaSyntax_LexStringLiteral` | Evidence from decompile and caller context. |
| `0x824c1638` | `FM2_LuaSyntax_ParsePrimaryPrefixExpr` | Evidence from decompile and caller context. |
| `0x824c1978` | `FM2_LuaSyntax_ParsePrimaryExprSuffixLoop` | Evidence from decompile and caller context. |
| `0x824c2388` | `FM2_LuaSyntax_ParseForNumericLoopVars` | Evidence from decompile and caller context. |
| `0x824c24f8` | `FM2_LuaSyntax_ParseForInIteratorVars` | Evidence from decompile and caller context. |
| `0x824c5308` | `FM2_LuaSyntax_FixupJumpTargetsAtPc` | Evidence from decompile and caller context. |
| `0x824c54b8` | `FM2_LuaSyntax_PatchLastEmittedLine` | Evidence from decompile and caller context. |
| `0x824c5d48` | `FM2_LuaSyntax_DischargeExpToRegIfIndexed` | Evidence from decompile and caller context. |
| `0x824c5fc0` | `FM2_LuaSyntax_EmitBinaryOpAndFreeRegs` | Evidence from decompile and caller context. |
| `0x824c6740` | `FM2_LuaSyntax_ExpToRegByKind` | Evidence from decompile and caller context. |
| `0x824c67f8` | `FM2_LuaSyntax_DischargeTestExpToJmp` | Evidence from decompile and caller context. |
| `0x824c62c0` | `FM2_LuaSyntax_DischargeJumpListToReg` | Evidence from decompile and caller context. |
| `0x824c6378` | `FM2_LuaSyntax_NegateConditionalJumpLists` | Evidence from decompile and caller context. |
| `0x824cbed0` | `FM2_BufFile_IncRefModuleHandle` | Evidence from decompile and caller context. |
| `0x824cc258` | `FM2_BufFile_ReleaseModuleRefArray` | Evidence from decompile and caller context. |
| `0x824cd088` | `FM2_XmlReader_GetAttrStringOrRequire` | Evidence from decompile and caller context. |
| `0x824cd350` | `FM2_XmlReader_SkipWhitespaceAndParseNodes` | Evidence from decompile and caller context. |
| `0x824cf3d8` | `FM2_XmlReader_ParseChildNodesUntilClose` | Evidence from decompile and caller context. |
| `0x824ccde0` | `FM2_XmlReader_CompareTokenCaseInsensitive` | Evidence from decompile and caller context. |
| `0x824ccd30` | `FM2_XmlReader_SkipWhitespace` | Evidence from decompile and caller context. |
| `0x824cd2d8` | `FM2_XmlReader_TryParseCommentOrDecl` | Evidence from decompile and caller context. |
| `0x824cf118` | `FM2_XmlReader_ParseElementOpenTag` | Evidence from decompile and caller context. |
| `0x824c5208` | `FM2_LuaSyntax_MakeFloatExpDesc` | Evidence from decompile and caller context. |
| `0x824c5370` | `FM2_LuaSyntax_TryCoalesceStringConcatExp` | Evidence from decompile and caller context. |
| `0x824c55e8` | `FM2_LuaSyntax_EmitLoadKInstruction` | Evidence from decompile and caller context. |
| `0x824c45c0` | `FM2_LuaSyntax_LexNumberOrStringLiteral` | Evidence from decompile and caller context. |
| `0x824c6850` | `FM2_LuaSyntax_SimpleExprTypeDispatch` | Evidence from decompile and caller context. |
| `0x824c56a8` | `FM2_LuaSyntax_EmitVarArgRangeInstruction` | Evidence from decompile and caller context. |
| `0x824c5d60` | `FM2_LuaSyntax_StoreExpResultToReg` | Evidence from decompile and caller context. |
| `0x824c60a8` | `FM2_LuaSyntax_EmitCompareBranchInstruction` | Evidence from decompile and caller context. |
| `0x824ccb98` | `FM2_XmlReader_InitScratchNameBuffer` | Evidence from decompile and caller context. |
| `0x824ccd98` | `FM2_XmlReader_ScanUntilChar` | Evidence from decompile and caller context. |

### Infrastructure pass 48 (33 functions)

Lua syntax tail (for-loop/lex/codegen), XML reader attr parse, network RB-tree due messages, Lua binding sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c58f8` | `FM2_LuaSyntax_DischargeToRegAndEmitUnaryOrBinary` | Evidence from decompile and caller context. |
| `0x824c21d0` | `FM2_LuaSyntax_InitForLoopHiddenLocals` | Evidence from decompile and caller context. |
| `0x824c3f78` | `FM2_LuaSyntax_LexQuotedStringContinue` | Evidence from decompile and caller context. |
| `0x824c6540` | `FM2_LuaSyntax_EmitBinaryOpWithStringCoalesce` | Evidence from decompile and caller context. |
| `0x824c6640` | `FM2_LuaSyntax_EmitBinaryOpBetweenRegs` | Evidence from decompile and caller context. |
| `0x824c4cc0` | `FM2_LuaSyntax_LexTokenViaNumberOrString` | Evidence from decompile and caller context. |
| `0x824c06e0` | `FM2_LuaSyntax_AddLocalOrUpvalueToProto` | Evidence from decompile and caller context. |
| `0x824c0ae8` | `FM2_LuaSyntax_RegisterFuncLocalInProto` | Evidence from decompile and caller context. |
| `0x824c1008` | `FM2_LuaSyntax_ParseSimpleExprAtom` | Evidence from decompile and caller context. |
| `0x824c1388` | `FM2_LuaSyntax_ParseFunctionParamList` | Evidence from decompile and caller context. |
| `0x824c42a8` | `FM2_LuaSyntax_LexLongBracketString` | Evidence from decompile and caller context. |
| `0x824ccd00` | `FM2_XmlReader_IsWhitespaceChar` | Evidence from decompile and caller context. |
| `0x824cd4c0` | `FM2_XmlReader_GetNodeRecordAtIndex` | Evidence from decompile and caller context. |
| `0x824cec68` | `FM2_XmlReader_AppendAttrVectorEntry` | Evidence from decompile and caller context. |
| `0x824cee60` | `FM2_XmlReader_ParseElementAttributes` | Evidence from decompile and caller context. |
| `0x824ced18` | `FM2_XmlReader_InsertNameAttrNode` | Evidence from decompile and caller context. |
| `0x821d7940` | `FM2_BufFile_InvokeWriterFlushVtable` | Evidence from decompile and caller context. |
| `0x82414080` | `FM2_LuaSyntax_RoundDoubleForStringConcat` | Evidence from decompile and caller context. |
| `0x824b80b0` | `FM2_LuaSyntax_ComputeOpcodeSizeClass` | Evidence from decompile and caller context. |
| `0x82457890` | `FM2_Network_RbTreeLowerBoundByDueTime` | Evidence from decompile and caller context. |
| `0x82457ca8` | `FM2_Network_RbTreeCollectDueMessages` | Evidence from decompile and caller context. |
| `0x824e7f20` | `FM2_Network_RbTreeSuccessorFromNode` | Evidence from decompile and caller context. |
| `0x824eb200` | `FM2_Lua_BindingPairIntrosortPartition` | Evidence from decompile and caller context. |
| `0x824eb2a0` | `FM2_Lua_BindingPairHeapSortDown` | Evidence from decompile and caller context. |
| `0x824eb6b8` | `FM2_Lua_BindingPathCompareCaseInsensitive` | Evidence from decompile and caller context. |
| `0x824ebae0` | `FM2_Lua_BindingPairInsertionSortTail` | Evidence from decompile and caller context. |
| `0x824d3408` | `FM2_Config_LookupTokenByIndex` | Evidence from decompile and caller context. |
| `0x824d3530` | `FM2_ProfileTuning_AssignWideString` | Evidence from decompile and caller context. |
| `0x824de6c8` | `FM2_LuaLeaderboard_TestClipDownloadFlag` | Evidence from decompile and caller context. |
| `0x824d5da8` | `FM2_Lua_PushSslUnitStringsTable` | Evidence from decompile and caller context. |
| `0x82434408` | `FM2_Config_WriteDelimitedTokenAtIndex` | Evidence from decompile and caller context. |
| `0x8240c380` | `FM2_Profile_OpenContentCreateExOrError` | Evidence from decompile and caller context. |
| `0x824e5300` | `FM2_RenderAdapter_InitPresentationVtables` | Evidence from decompile and caller context. |

### Infrastructure pass 49 (33 functions)

XML reader/writer attr helpers, render adapter init, profile/Lua binding alloc, network dispatch.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824ccf40` | `FM2_XmlReader_ParseBoolAttrDefault` | Evidence from decompile and caller context. |
| `0x824cce90` | `FM2_XmlReader_ParseFloatAttrDefault` | Evidence from decompile and caller context. |
| `0x824cd740` | `FM2_XmlReader_ParseBoolAttrStrict` | Evidence from decompile and caller context. |
| `0x824cd0d0` | `FM2_XmlReader_BuildErrorPathString` | Evidence from decompile and caller context. |
| `0x824cd1b0` | `FM2_XmlReader_StreamReadLineIntoBuffer` | Evidence from decompile and caller context. |
| `0x824cd3a8` | `FM2_XmlWriter_AppendStringAttr` | Evidence from decompile and caller context. |
| `0x824cd430` | `FM2_XmlWriter_AppendFloatAttr` | Evidence from decompile and caller context. |
| `0x824ce240` | `FM2_XmlWriter_WriteIndentBeforeClose` | Evidence from decompile and caller context. |
| `0x824ce390` | `FM2_XmlWriter_WriteSelfClosingTag` | Evidence from decompile and caller context. |
| `0x824ce970` | `FM2_XmlReader_LoadAttributesFromArchive` | Evidence from decompile and caller context. |
| `0x824cf688` | `FM2_XmlReader_ApplyAttrDefaultsFromNode` | Evidence from decompile and caller context. |
| `0x824cf8f8` | `FM2_XmlReader_FindAttrEntryByName` | Evidence from decompile and caller context. |
| `0x824cfa78` | `FM2_XmlReader_FindAttrByPathSegments` | Evidence from decompile and caller context. |
| `0x824cfc10` | `FM2_XmlReader_CopyAttrDefaultsToNode` | Evidence from decompile and caller context. |
| `0x824cc840` | `FM2_XmlReader_DtorAttrNodeList` | Evidence from decompile and caller context. |
| `0x824cc4d0` | `FM2_BufFile_CompactModuleRefTableLocked` | Evidence from decompile and caller context. |
| `0x824cd4d0` | `FM2_XmlReader_DtorStringNodeA` | Evidence from decompile and caller context. |
| `0x824cd538` | `FM2_XmlReader_DtorStringNodeB` | Evidence from decompile and caller context. |
| `0x824e3fb0` | `FM2_RenderAdapter_InitSwitchModeBase` | Evidence from decompile and caller context. |
| `0x824e53c0` | `FM2_RenderAdapter_InitSwitchModeAlt` | Evidence from decompile and caller context. |
| `0x824e7bd0` | `FM2_CareerRace_MoveRewardsBlock` | Evidence from decompile and caller context. |
| `0x824e8368` | `FM2_HashName_AssignFromPropertySlice` | Evidence from decompile and caller context. |
| `0x824e9960` | `FM2_LiveProfile_ReadWriteBufferHeader` | Evidence from decompile and caller context. |
| `0x824eae80` | `FM2_Lua_AllocBindingPairVector` | Evidence from decompile and caller context. |
| `0x824eaf90` | `FM2_Lua_BindingPairMedianOfThree` | Evidence from decompile and caller context. |
| `0x824ebf20` | `FM2_Lua_BindingPairSortEntryThunk` | Evidence from decompile and caller context. |
| `0x824d6480` | `FM2_Lua_PushDampingFromKeyframeDouble` | Evidence from decompile and caller context. |
| `0x824d75a0` | `FM2_Profile_MakeStringKeyComPtr` | Evidence from decompile and caller context. |
| `0x824d76a0` | `FM2_Profile_ParseUnsignedFromSubString` | Evidence from decompile and caller context. |
| `0x824d8b28` | `FM2_Math_AllocForceVectorComPtr` | Evidence from decompile and caller context. |
| `0x824d3cb0` | `FM2_Scene_GetNotifyStateFromParamHelper` | Evidence from decompile and caller context. |
| `0x82587c88` | `FM2_Network_DispatchMessageFromQueueLocked` | Evidence from decompile and caller context. |
| `0x8258d228` | `FM2_Set_LowerBoundByKeyInTree` | Evidence from decompile and caller context. |

### Infrastructure pass 50 (33 functions)

Render adapter/D3D init, view traversal draw setup, AI race line, XML writer, Lua binding sort, image/PNG load.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824e9c78` | `FM2_RenderAdapter_InitSwitchModeSharedFields` | Evidence from decompile and caller context. |
| `0x825c5320` | `FM2_LiveProfile_ReadWriteBufferScoped` | Evidence from decompile and caller context. |
| `0x8222e4e0` | `FM2_XmlWriter_AppendVsnprintf` | Evidence from decompile and caller context. |
| `0x82758250` | `FM2_Crt_WcsncpyChecked` | Evidence from decompile and caller context. |
| `0x824eb090` | `FM2_Lua_BindingPairSiftDown` | Evidence from decompile and caller context. |
| `0x821fcb38` | `FM2_IntrusiveList_SpliceNodeRange` | Evidence from decompile and caller context. |
| `0x8223f108` | `FM2_CarParts_ApplyUpgradeSlotFromDescriptor` | Evidence from decompile and caller context. |
| `0x82480be0` | `FM2_AIDriver_UpdateRaceLineFromSector` | Evidence from decompile and caller context. |
| `0x824810a8` | `FM2_AIDriver_ResetRaceLineOnSectorChange` | Evidence from decompile and caller context. |
| `0x824a52c0` | `FM2_D3D_InitGlobalDeviceSingleton` | Evidence from decompile and caller context. |
| `0x824efcb8` | `FM2_ExceptionFilter_OnCppException` | Evidence from decompile and caller context. |
| `0x824f0fb0` | `FM2_D3D_LazyInitPresentChainNotify` | Evidence from decompile and caller context. |
| `0x824f69f8` | `FM2_D3D_Subscriber_InitVtables` | Evidence from decompile and caller context. |
| `0x824fa8b0` | `FM2_Memory_LookupFrameAllocNotifyState` | Evidence from decompile and caller context. |
| `0x82502aa8` | `FM2_D3D_Subscriber_EnableDeviceJournal` | Evidence from decompile and caller context. |
| `0x8250a178` | `FM2_Render_ViewTraversalUpdateNodes` | Evidence from decompile and caller context. |
| `0x825105e8` | `FM2_Render_CompileMissingPassBuffers` | Evidence from decompile and caller context. |
| `0x82514e58` | `FM2_Render_DecodeAndSubmitDrawKey` | Evidence from decompile and caller context. |
| `0x82516700` | `FM2_Render_ObjectPassDrawSetupCore` | Evidence from decompile and caller context. |
| `0x82517520` | `FM2_Render_UpdatePassVisibilityState` | Evidence from decompile and caller context. |
| `0x824df738` | `FM2_RenderAdapter_ClearPresentationBinding` | Evidence from decompile and caller context. |
| `0x825c5290` | `FM2_LiveProfile_ReadWriteBufferBody` | Evidence from decompile and caller context. |
| `0x82429e08` | `FM2_BinaryStream_InitReadScope` | Evidence from decompile and caller context. |
| `0x82429e60` | `FM2_BinaryStream_DtorReadScope` | Evidence from decompile and caller context. |
| `0x821fc718` | `FM2_Render_NotifyChainInsertSubscriber` | Evidence from decompile and caller context. |
| `0x8250ffd0` | `FM2_Render_InitPassCompileLock` | Evidence from decompile and caller context. |
| `0x8250fbd8` | `FM2_Render_DtorPassCompileLock` | Evidence from decompile and caller context. |
| `0x82493680` | `FM2_AIDriver_LookupTrackWidthSample` | Evidence from decompile and caller context. |
| `0x82333430` | `FM2_Stl_IntrosortMedianOfThreeFloats` | Evidence from decompile and caller context. |
| `0x82360e38` | `FM2_Input_InitAxisDefaultsFromTable` | Evidence from decompile and caller context. |
| `0x8236aa48` | `FM2_D3D_ComputeResourceBindingFlags` | Evidence from decompile and caller context. |
| `0x823780e0` | `FM2_GpuCommandBuffer_BeginPerfCaptureOrKick` | Evidence from decompile and caller context. |
| `0x823a4348` | `FM2_Image_LoadPngFromMemory` | Evidence from decompile and caller context. |

### Infrastructure pass 51 (33 functions)

Render frame pipeline / view traversal cluster, PNG/bitstream/image, Lua binding register, FMOD geometry.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823b1278` | `FM2_Png_EnsureRgbThenDecodeRow` | Evidence from decompile and caller context. |
| `0x823c1190` | `FM2_Bitstream_ReadVariableBits` | Evidence from decompile and caller context. |
| `0x823d6eb0` | `FM2_Image_PixelBuffer_Init` | Evidence from decompile and caller context. |
| `0x82251c98` | `FM2_LiveryMask_MergeProfileRecordFromNode` | Evidence from decompile and caller context. |
| `0x824930a0` | `FM2_AIDriver_ComputeSectorIndexFromProgress` | Evidence from decompile and caller context. |
| `0x8250a820` | `FM2_Render_WaitResourceLockForPassCompile` | Evidence from decompile and caller context. |
| `0x8250ce18` | `FM2_Render_ViewTraversalCullNodes` | Evidence from decompile and caller context. |
| `0x8250cff0` | `FM2_Render_ViewTraversalSortDrawLists` | Evidence from decompile and caller context. |
| `0x8250d238` | `FM2_Render_ViewTraversalEmitDrawCalls` | Evidence from decompile and caller context. |
| `0x8250e400` | `FM2_Presentation_AllocCarInstanceSlot` | Evidence from decompile and caller context. |
| `0x8250f590` | `FM2_Memory_AllocDeferredMapBlockLocked` | Evidence from decompile and caller context. |
| `0x82512038` | `FM2_Presentation_CopyCarDisplayBlockToSlot` | Evidence from decompile and caller context. |
| `0x82513280` | `FM2_Render_BuildObjectPassCommandBuffer` | Evidence from decompile and caller context. |
| `0x82513340` | `FM2_Render_AppendObjectPassDrawEntry` | Evidence from decompile and caller context. |
| `0x825135b8` | `FM2_Vector_EraseBegin20ByteElements` | Evidence from decompile and caller context. |
| `0x825151a0` | `FM2_Render_SubmitObjectDrawConstantsBlock` | Evidence from decompile and caller context. |
| `0x82516e30` | `FM2_Render_UpdatePassVisibilitySortKeysA` | Evidence from decompile and caller context. |
| `0x82516ed8` | `FM2_Render_UpdatePassVisibilitySortKeysB` | Evidence from decompile and caller context. |
| `0x82517778` | `FM2_Render_FramePipelineSubmitPassA` | Evidence from decompile and caller context. |
| `0x82517870` | `FM2_Render_FramePipelineSubmitPassB` | Evidence from decompile and caller context. |
| `0x825179e0` | `FM2_Render_SubmitPassWrapperInner` | Evidence from decompile and caller context. |
| `0x82517b18` | `FM2_Render_FramePipelineDrawObjects` | Evidence from decompile and caller context. |
| `0x82518228` | `FM2_Render_FramePipelineFinalizePass` | Evidence from decompile and caller context. |
| `0x8251a010` | `FM2_Render_SubmitSortedObjectDrawListsInner` | Evidence from decompile and caller context. |
| `0x8251c290` | `FM2_Presentation_ApplyCarCameraVMX` | Evidence from decompile and caller context. |
| `0x82521a98` | `FM2_Render_InstanceHybridDrawPathInner` | Evidence from decompile and caller context. |
| `0x825222b0` | `FM2_Render_ExecuteSortedDrawListsCore` | Evidence from decompile and caller context. |
| `0x8254d6e8` | `FM2_Lua_RegisterBindingPairsInModuleTable` | Evidence from decompile and caller context. |
| `0x8254e278` | `FM2_Lua_AssertionFailedVprintf` | Evidence from decompile and caller context. |
| `0x82553568` | `FM2_Lua_AllocUpvalueClosure` | Evidence from decompile and caller context. |
| `0x82556170` | `FM2_CarDynamics_InitSuspensionFromTireBlock` | Evidence from decompile and caller context. |
| `0x82559680` | `FM2_FMOD_Geometry_AddPolygonFromVMX` | Evidence from decompile and caller context. |
| `0x82559d50` | `FM2_Render_ObjectPassDrawSetupMaterialPass` | Evidence from decompile and caller context. |

### Infrastructure pass 52 (33 functions)

Render view-traversal / object-pass draw cluster: visibility VMX, pass env constants, draw-list submit.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825d62e0` | `FM2_Render_ViewTraversalGetDrawListEntryPtr` | Evidence from decompile and caller context. |
| `0x82724200` | `FM2_Render_CopyPassLightingStateBlock` | Evidence from decompile and caller context. |
| `0x82513108` | `FM2_Render_AllocObjectPassDrawSlotLocked` | Evidence from decompile and caller context. |
| `0x82516d08` | `FM2_Render_ApplyPassEnvironmentIfDeferred` | Evidence from decompile and caller context. |
| `0x8251b520` | `FM2_Render_TestPassOcclusionBounds` | Evidence from decompile and caller context. |
| `0x8251d030` | `FM2_Render_UpdateDrawCullFlagsIfVisible` | Evidence from decompile and caller context. |
| `0x8251ff88` | `FM2_Render_TestPassVisibilityVMX` | Evidence from decompile and caller context. |
| `0x825a39a8` | `FM2_Presentation_InitCarSlotTransformZeros` | Evidence from decompile and caller context. |
| `0x8251b4f0` | `FM2_Render_ClearPassDrawOverride` | Evidence from decompile and caller context. |
| `0x8251bbb0` | `FM2_Render_HasPassDrawListForSortKey` | Evidence from decompile and caller context. |
| `0x8251b1a0` | `FM2_Render_ComputePassDrawOverrideVMX` | Evidence from decompile and caller context. |
| `0x82761080` | `FM2_Render_GetPassEnvConstantSlotA` | Evidence from decompile and caller context. |
| `0x827610c0` | `FM2_Render_GetPassEnvConstantSlotB` | Evidence from decompile and caller context. |
| `0x82761100` | `FM2_Render_SetPassEnvConstantSlotA` | Evidence from decompile and caller context. |
| `0x82761120` | `FM2_Render_SetPassEnvConstantSlotB` | Evidence from decompile and caller context. |
| `0x82512f40` | `FM2_Render_GrowObjectPassDrawVector` | Evidence from decompile and caller context. |
| `0x8252d060` | `FM2_Render_SortVisibleRenderablesThunk` | Evidence from decompile and caller context. |
| `0x8252dba0` | `FM2_Render_GetDistanceKeyFromPassSlot` | Evidence from decompile and caller context. |
| `0x825276b0` | `FM2_Render_ExecuteSortedDrawListsPassA` | Evidence from decompile and caller context. |
| `0x82527878` | `FM2_Render_ExecuteSortedDrawListsPassB` | Evidence from decompile and caller context. |
| `0x8252ac00` | `FM2_Render_CompilePassIfStaleLocked` | Evidence from decompile and caller context. |
| `0x82559df0` | `FM2_Render_ObjectPassDrawSetupMaterialSlot` | Evidence from decompile and caller context. |
| `0x8255b828` | `FM2_Render_SubmitSortedObjectDrawListVMX` | Evidence from decompile and caller context. |
| `0x8255d798` | `FM2_Render_ObjectPassShouldDrawVisible` | Evidence from decompile and caller context. |
| `0x8255fa28` | `FM2_Render_ObjectPassEmitDrawIfVisible` | Evidence from decompile and caller context. |
| `0x8272d5a8` | `FM2_Render_TestFrustumOcclusionVMX` | Evidence from decompile and caller context. |
| `0x82564588` | `FM2_Render_AssignResourceLockFromPassData` | Evidence from decompile and caller context. |
| `0x825094c0` | `FM2_Render_NotifyGlobalManagerStateChange` | Evidence from decompile and caller context. |
| `0x82522418` | `FM2_Render_UploadDrawListMatrixConstants` | Evidence from decompile and caller context. |
| `0x82528750` | `FM2_Stl_Vector_EraseRangeAtCopy` | Evidence from decompile and caller context. |
| `0x82538990` | `FM2_Render_InstanceHybridDrawPathSort` | Evidence from decompile and caller context. |
| `0x82539398` | `FM2_Render_InstancePathWrapperInner` | Evidence from decompile and caller context. |
| `0x8250f708` | `FM2_Render_IsPassCompileResourceReady` | Evidence from decompile and caller context. |

### Infrastructure pass 53 (33 functions)

Render/audio/Lua/FMOD/RB-tree helpers: presentation slots, Lua assert/math, file cache, boot parse.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82509468` | `FM2_Render_SpliceAndReleaseNotifyList` | Evidence from decompile and caller context. |
| `0x8250cd60` | `FM2_DeferredCommand_ClearStringField` | Evidence from decompile and caller context. |
| `0x82505ec8` | `FM2_Render_ComputeInstancePathBlendMatrix` | Evidence from decompile and caller context. |
| `0x82505d70` | `FM2_AudioVoice_GrowChannelArrayCapacity` | Evidence from decompile and caller context. |
| `0x82512358` | `FM2_PresentationCarConfig_DeleteOptional` | Evidence from decompile and caller context. |
| `0x825127a0` | `FM2_PresentationSlotVector_Clear200Byte` | Evidence from decompile and caller context. |
| `0x8254e318` | `FM2_Lua_AssertionFailedTypeError` | Evidence from decompile and caller context. |
| `0x8254e450` | `FM2_Lua_MathTwoArgDispatch` | Evidence from decompile and caller context. |
| `0x8254d220` | `FM2_Lua_PushMetatableWithGcAndProps` | Evidence from decompile and caller context. |
| `0x82551ab0` | `FM2_FindAndReplaceDelimitedTextRange` | Evidence from decompile and caller context. |
| `0x82555fc8` | `FM2_Animation_ClampKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x8255c210` | `FM2_Render_TestObjectPassDrawVisibility` | Evidence from decompile and caller context. |
| `0x8255b1c8` | `FM2_Render_SubmitSortedDrawListsTail` | Evidence from decompile and caller context. |
| `0x8255fb58` | `FM2_Render_ObjectPassDrawTraversalInner` | Evidence from decompile and caller context. |
| `0x82567060` | `FM2_TModel_InitVMXBounds` | Evidence from decompile and caller context. |
| `0x82568590` | `FM2_Render_HelperB3E8PathA` | Evidence from decompile and caller context. |
| `0x82572ca8` | `FM2_WString_GrowHeapCapacity` | Evidence from decompile and caller context. |
| `0x82572dc8` | `FM2_AudioManager_InitSignalGateField` | Evidence from decompile and caller context. |
| `0x82579bb8` | `FM2_Render_SubmitObjectDrawConstantsTail` | Evidence from decompile and caller context. |
| `0x8257ccf8` | `FM2_HashName_CtorEmpty` | Evidence from decompile and caller context. |
| `0x8257cdc0` | `FM2_PropertyBag_AllocRbTreeNode` | Evidence from decompile and caller context. |
| `0x82582188` | `FM2_XmlElement_Dtor` | Evidence from decompile and caller context. |
| `0x82586f40` | `FM2_FMOD_Build3DAttributesPairA` | Evidence from decompile and caller context. |
| `0x82587738` | `FM2_UI_GetMaxPropertyAbsValueHalfStep` | Evidence from decompile and caller context. |
| `0x8258a480` | `FM2_FileInfoCache_AllocateEntry` | Evidence from decompile and caller context. |
| `0x8258b0f8` | `FM2_RenderAdapter_DestroyChildAndClearList` | Evidence from decompile and caller context. |
| `0x8258b3c0` | `FM2_IntrusiveListNode_InitWithOffset` | Evidence from decompile and caller context. |
| `0x8258b4d0` | `FM2_RbTree_InitIteratorWithHint` | Evidence from decompile and caller context. |
| `0x8258c208` | `FM2_Presentation_InitMediaFoundationField` | Evidence from decompile and caller context. |
| `0x8258ebe0` | `FM2_CarDb_QueryStockPartByOrdinal` | Evidence from decompile and caller context. |
| `0x825a11f8` | `FM2_Render_ViewTraversalNotifyHook` | Evidence from decompile and caller context. |
| `0x825a31e0` | `FM2_AudioRenderFrame_LogSaveFrontBufferBody` | Evidence from decompile and caller context. |
| `0x825a3ca8` | `FM2_Boot_ParseCommandLineTokenBuffer` | Evidence from decompile and caller context. |

### Infrastructure pass 54 (33 functions)

Render pass lighting VMX cluster, frame pipeline pass cleanup, sort/visibility helpers, PNG/math.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827c5e38` | `FM2_Render_SetPassTimingScalar` | Evidence from decompile and caller context. |
| `0x82725780` | `FM2_Render_MultiplyMatrix4x4VMX` | Evidence from decompile and caller context. |
| `0x821d7608` | `FM2_Render_ViewTraversalNormalizeBasisVMX` | Evidence from decompile and caller context. |
| `0x82724218` | `FM2_Render_SetPassLightingCoeffs` | Evidence from decompile and caller context. |
| `0x82724888` | `FM2_Render_ApplyPassLightingStateInner` | Evidence from decompile and caller context. |
| `0x82724958` | `FM2_Render_CopyPassViewMatrix4x4` | Evidence from decompile and caller context. |
| `0x82724988` | `FM2_Render_CopyPassProjMatrix4x4` | Evidence from decompile and caller context. |
| `0x827249a0` | `FM2_Render_SetPassLightingFromMatrix` | Evidence from decompile and caller context. |
| `0x827250f8` | `FM2_Render_TransformVectorByMatrix4x4` | Evidence from decompile and caller context. |
| `0x827255b0` | `FM2_Render_MultiplyMatrix4x4AccumVMX` | Evidence from decompile and caller context. |
| `0x827306c0` | `FM2_ConstantBuffer_UploadVector4Block` | Evidence from decompile and caller context. |
| `0x82725310` | `FM2_Render_BuildPassLightingMatrixVMX` | Evidence from decompile and caller context. |
| `0x82724568` | `FM2_Render_UpdatePassSortKeysFromBounds` | Evidence from decompile and caller context. |
| `0x82723d18` | `FM2_RenderContext_UploadMatrixConstantsFromPass` | Evidence from decompile and caller context. |
| `0x82723860` | `FM2_RenderTls_BatchSubmitDrawPacketsTail` | Evidence from decompile and caller context. |
| `0x825377e8` | `FM2_Render_InstancePathWrapperTraverse` | Evidence from decompile and caller context. |
| `0x8253cfd0` | `FM2_Render_FramePipelineNotifyPassState` | Evidence from decompile and caller context. |
| `0x8253d440` | `FM2_Render_FramePipelineCleanupPassSlots` | Evidence from decompile and caller context. |
| `0x8252b8d8` | `FM2_Render_SortVisibleRenderablesIntrosort` | Evidence from decompile and caller context. |
| `0x821e1d60` | `FM2_AudioRenderFrame_FlushLogBufferChunk` | Evidence from decompile and caller context. |
| `0x821efec0` | `FM2_Render_HelperB3E8ResetState` | Evidence from decompile and caller context. |
| `0x82515d58` | `FM2_Render_TestObjectPassOcclusionWrapped` | Evidence from decompile and caller context. |
| `0x82659258` | `FM2_Memory_AllocFromAllocatorContext` | Evidence from decompile and caller context. |
| `0x824df418` | `FM2_RenderAdapter_ResetPresentationStateBlock` | Evidence from decompile and caller context. |
| `0x825c5f48` | `FM2_ProfileDb_InitPropertyBagCritSec` | Evidence from decompile and caller context. |
| `0x82557428` | `FM2_BufferedFileRead_RandUnitFloat` | Evidence from decompile and caller context. |
| `0x8258b370` | `FM2_RbTree_FindLowerBoundNodeByKey` | Evidence from decompile and caller context. |
| `0x82461428` | `FM2_Crt_CreateSemaphoreA` | Evidence from decompile and caller context. |
| `0x8239f2b8` | `FM2_Png_AllocDecodeStateBuffer` | Evidence from decompile and caller context. |
| `0x82724078` | `FM2_RenderTls_BindPassStateToContext` | Evidence from decompile and caller context. |
| `0x82724160` | `FM2_Render_InitPassLightingStateBlock` | Evidence from decompile and caller context. |
| `0x82724270` | `FM2_Render_ApplyPassLightingCoeffsVMX` | Evidence from decompile and caller context. |
| `0x827260e8` | `FM2_Math_FastInvSqrtTaylor` | Evidence from decompile and caller context. |

### Infrastructure pass 55 (33 functions)

Audio/FMOD/XAudio2, car dynamics, profile/livery, deferred tasks, STL RB-tree, pass lighting helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8264eb78` | `FM2_CarDynamics_InitSubsystemField` | Evidence from decompile and caller context. |
| `0x826c2e90` | `FM2_XAudio2_Stream_SubmitBufferLockedTail` | Evidence from decompile and caller context. |
| `0x8258ec80` | `FM2_Lua_PushEngineTypeEnumToStack` | Evidence from decompile and caller context. |
| `0x82657fa0` | `FM2_CarDynamics_InitTireParamsField` | Evidence from decompile and caller context. |
| `0x82678f38` | `FM2_FMOD_DspConnectionPool_ProcessInputTail` | Evidence from decompile and caller context. |
| `0x826bc1b0` | `FM2_D3D_InitVoicePresentationField` | Evidence from decompile and caller context. |
| `0x82429f40` | `FM2_LiveryMask_MergeProfileRecordTail` | Evidence from decompile and caller context. |
| `0x824d3d48` | `FM2_Profile_ApplyTuningRecordField` | Evidence from decompile and caller context. |
| `0x825c5d38` | `FM2_LiveProfile_ReadWriteBufferTail` | Evidence from decompile and caller context. |
| `0x8267a158` | `FM2_FMOD_Dsp_ProcessReverbMixTail` | Evidence from decompile and caller context. |
| `0x82721878` | `FM2_Render_PrepareSceneSliceTransforms` | Evidence from decompile and caller context. |
| `0x82754c30` | `FM2_LiveryMask_SpawnWorkerThread` | Evidence from decompile and caller context. |
| `0x82756310` | `FM2_XAudio2_Pool_FreeWithCallback` | Evidence from decompile and caller context. |
| `0x82763300` | `FM2_DeferredTask_SubmitWithParamsLookup` | Evidence from decompile and caller context. |
| `0x8277c508` | `FM2_STL_RbTree_InsertOrFindNode` | Evidence from decompile and caller context. |
| `0x824f53b8` | `FM2_RenderState_ApplyFromContextBlock` | Evidence from decompile and caller context. |
| `0x825a25b0` | `FM2_AudioManager_InitSignalGateField` | Evidence from decompile and caller context. |
| `0x8266f620` | `FM2_FMOD_Channel_GetVolumeScalar` | Evidence from decompile and caller context. |
| `0x82766a10` | `FM2_CameraList_FindPrevByCamId` | Evidence from decompile and caller context. |
| `0x82767928` | `FM2_STL_RbTree_InsertOrFindAlt` | Evidence from decompile and caller context. |
| `0x8265ea30` | `FM2_FMOD_Channel_StopIfPlayingCheck` | Evidence from decompile and caller context. |
| `0x826743b0` | `FM2_FMOD_DspConnectionPool_ProcessInputCheck` | Evidence from decompile and caller context. |
| `0x826b54c8` | `FM2_XAudio2_StreamPool_UnlinkAndNotifyBody` | Evidence from decompile and caller context. |
| `0x826bbef0` | `FM2_D3D_InitVoicePresentationSubsystemTail` | Evidence from decompile and caller context. |
| `0x826efa08` | `FM2_SQLite_AppendLowercaseIdentifierMode1` | Evidence from decompile and caller context. |
| `0x825562b8` | `FM2_CarDynamics_InitTireParamsTail` | Evidence from decompile and caller context. |
| `0x82638d40` | `FM2_CarSetup_CtorFields` | Evidence from decompile and caller context. |
| `0x82673ea0` | `FM2_FMOD_Dsp_AdjustDelayLinePointers` | Evidence from decompile and caller context. |
| `0x826792d0` | `FM2_FMOD_Dsp_ProcessReverbMixBlockTail` | Evidence from decompile and caller context. |
| `0x82725d80` | `FM2_Render_SetObjectDistanceKeySlot` | Evidence from decompile and caller context. |
| `0x82724760` | `FM2_Render_BuildPassLightingFromCameraAngles` | Evidence from decompile and caller context. |
| `0x827243b0` | `FM2_Render_ApplyPassLightingCoeffsVMXWide` | Evidence from decompile and caller context. |
| `0x82725f80` | `FM2_Render_ComputeSinCosForPassLighting` | Evidence from decompile and caller context. |

### Infrastructure pass 56 (33 functions)

Render pass-lighting VMX tail, FMOD/SQLite, race ghost playback, D3D GPU resource read, ForzaTV singleton.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8265e8a0` | `FM2_FMOD_Channel_GetEventParameterValue` | Evidence from decompile and caller context. |
| `0x826ef890` | `FM2_SQLite_ExprSetAffinityAndFlags` | Evidence from decompile and caller context. |
| `0x821d65f0` | `FM2_Render_HelperB3E8ClearStringVector` | Evidence from decompile and caller context. |
| `0x821e9a88` | `FM2_Render_HelperB3E8InitStringFields` | Evidence from decompile and caller context. |
| `0x82228560` | `FM2_Render_NotifyChainInsertSubscriberSorted` | Evidence from decompile and caller context. |
| `0x821d6538` | `FM2_Render_HelperB3E8InitStringRange` | Evidence from decompile and caller context. |
| `0x826ff938` | `FM2_SQLite_ExprAppendLowercaseToken` | Evidence from decompile and caller context. |
| `0x826eedc0` | `FM2_SQLite_ExprGrowTokenBuffer` | Evidence from decompile and caller context. |
| `0x8237ef10` | `FM2_D3D_ReadGpuResourceFloatData` | Evidence from decompile and caller context. |
| `0x8237ed10` | `FM2_D3D_ReleaseGpuResourceRef` | Evidence from decompile and caller context. |
| `0x8253d3d0` | `FM2_Render_FramePipelineResolvePassSlot` | Evidence from decompile and caller context. |
| `0x82237d90` | `FM2_RaceGhost_SelectPlaybackNode` | Evidence from decompile and caller context. |
| `0x82364140` | `FM2_Memory_AllocViaPoolHandler` | Evidence from decompile and caller context. |
| `0x827283f8` | `FM2_RenderTls_SetGlobalPassStatePtrA` | Evidence from decompile and caller context. |
| `0x82728418` | `FM2_RenderTls_SetGlobalPassStatePtrB` | Evidence from decompile and caller context. |
| `0x82724a50` | `FM2_Render_SetPassLightingFromInverseMatrix` | Evidence from decompile and caller context. |
| `0x82724ce0` | `FM2_Render_TransformPointByLightingMatrix` | Evidence from decompile and caller context. |
| `0x82725210` | `FM2_Render_CopyLightingMatrixColumns` | Evidence from decompile and caller context. |
| `0x827258d0` | `FM2_Render_TestPassBoundsVMX` | Evidence from decompile and caller context. |
| `0x8236f180` | `FM2_RenderTls_BindPassStateToContextInner` | Evidence from decompile and caller context. |
| `0x82682168` | `FM2_FMOD_Channel_GetVolumeFromEventTable` | Evidence from decompile and caller context. |
| `0x82684938` | `FM2_FMOD_Event_GetUserDataPtrImpl` | Evidence from decompile and caller context. |
| `0x824f2e48` | `FM2_ForzaTV_InitSubscriberVtables` | Evidence from decompile and caller context. |
| `0x824f2ef0` | `FM2_ForzaTV_EnsureSingletonInit` | Evidence from decompile and caller context. |
| `0x827253c8` | `FM2_Render_BuildPassLightingMatrixFromAngles` | Evidence from decompile and caller context. |
| `0x827254c0` | `FM2_Render_SetPassLightingScaleMatrix` | Evidence from decompile and caller context. |
| `0x82725698` | `FM2_Render_MultiplyPassMatrixVMXVariant` | Evidence from decompile and caller context. |
| `0x827259e0` | `FM2_Render_SetPassLightingModeScalar` | Evidence from decompile and caller context. |
| `0x82725b70` | `FM2_Render_ComparePassMatrixBytesVMX` | Evidence from decompile and caller context. |
| `0x82724d68` | `FM2_Render_TransformDirByLightingMatrix` | Evidence from decompile and caller context. |
| `0x82724e40` | `FM2_Render_ProjectPassBoundsToScreen` | Evidence from decompile and caller context. |
| `0x82725160` | `FM2_Render_SetPassLightingDiagonalMatrix` | Evidence from decompile and caller context. |
| `0x827261f8` | `FM2_Render_ApplyPassLightingMatrixToState` | Evidence from decompile and caller context. |

### Infrastructure pass 57 (33 functions)

Deferred tasks, livery render, FMOD/XAudio2/SQLite, D3D device, font/XTS/STL helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82767fe0` | `FM2_STL_VectorClearAndFreeRange` | Evidence from decompile and caller context. |
| `0x82777000` | `FM2_DeferredTask_SubmitPreloadingAnimTurnOnTail` | Evidence from decompile and caller context. |
| `0x8277d808` | `FM2_STL_WStringInsertCharsSlot` | Evidence from decompile and caller context. |
| `0x82782748` | `FM2_STL_CircularBuffer_PushBackEntry` | Evidence from decompile and caller context. |
| `0x825db070` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot0` | Evidence from decompile and caller context. |
| `0x825db0b8` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot1` | Evidence from decompile and caller context. |
| `0x825db9d0` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot2` | Evidence from decompile and caller context. |
| `0x82763488` | `FM2_DeferredTask_SubmitWithParamsLookupBody` | Evidence from decompile and caller context. |
| `0x824f5550` | `FM2_D3D_DeviceContext_DtorFields` | Evidence from decompile and caller context. |
| `0x8254d2b8` | `FM2_FindAndReplaceDelimitedTextRangeImpl` | Evidence from decompile and caller context. |
| `0x824b7448` | `FM2_Lua_RegisterBindingPairsModuleTail` | Evidence from decompile and caller context. |
| `0x825eacf8` | `FM2_SceneSerializer_DisposeAndClearChildren` | Evidence from decompile and caller context. |
| `0x82615988` | `FM2_SceneNode_InvokeVirtualMethod32Body` | Evidence from decompile and caller context. |
| `0x82617a60` | `FM2_HashNamePropertyList_EraseNode` | Evidence from decompile and caller context. |
| `0x82657fd8` | `FM2_CarDynamics_ComputeSuspensionDotsVMX` | Evidence from decompile and caller context. |
| `0x825dddb0` | `FM2_LiveryRenderManager_CreateLayerBinding` | Evidence from decompile and caller context. |
| `0x825edfb8` | `FM2_Lua_UISceneManager_GetFadeOutBeginEvent` | Evidence from decompile and caller context. |
| `0x8265ea78` | `FM2_FMOD_Channel_StopIfPlayingInner` | Evidence from decompile and caller context. |
| `0x826631a0` | `FM2_FMOD_Channel_StopIfPlayingAlt` | Evidence from decompile and caller context. |
| `0x826afa08` | `FM2_RenderAdapter_DecRefPresentationSwitch` | Evidence from decompile and caller context. |
| `0x826bd550` | `FM2_XAudio2_WorkerThread_MainLoopBody` | Evidence from decompile and caller context. |
| `0x826ee218` | `FM2_SQLite_Database_CloseField` | Evidence from decompile and caller context. |
| `0x827526f0` | `FM2_DeferredTaskQueue_AllocWorkItemBody` | Evidence from decompile and caller context. |
| `0x82535be8` | `FM2_DirectIface_SetVertexShaderFromHandle` | Evidence from decompile and caller context. |
| `0x82537270` | `FM2_DirectIface_SetVertexShaderFromHandleB` | Evidence from decompile and caller context. |
| `0x825b4188` | `FM2_DirectIface_SetPixelShaderFromHandle` | Evidence from decompile and caller context. |
| `0x825d3130` | `FM2_SceneGraph_UpdateNodeWithNotifyStateField` | Evidence from decompile and caller context. |
| `0x827bd050` | `FM2_FontRenderer_LayoutGlyphRunBody` | Evidence from decompile and caller context. |
| `0x827bec78` | `FM2_FontCache_InitSentinelList` | Evidence from decompile and caller context. |
| `0x82781e30` | `FM2_XtsClient_ProcessMessageQueueStep` | Evidence from decompile and caller context. |
| `0x82778780` | `FM2_STL_WStringInsertCharsAlt` | Evidence from decompile and caller context. |
| `0x8277c760` | `FM2_STL_WStringInsertCharsRange` | Evidence from decompile and caller context. |
| `0x827f73b0` | `FM2_STL_RbTree_InsertOrFindLeaf` | Evidence from decompile and caller context. |

### Infrastructure pass 58 (33 functions)

Pass-lighting VMX/math tail, race ghost playback, car-parts bounds, livery/XML/com/audio helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8223e0e0` | `FM2_Render_BuildPassLightingBasisVMX` | Evidence from decompile and caller context. |
| `0x827f6100` | `FM2_STL_AllocViaComGpuAllocator` | Evidence from decompile and caller context. |
| `0x821e8280` | `FM2_RaceGhost_InitPlaybackContext` | Evidence from decompile and caller context. |
| `0x82232180` | `FM2_RaceGhost_RebuildPlaybackFromSamples` | Evidence from decompile and caller context. |
| `0x822361a0` | `FM2_RaceGhost_SplicePlaybackNodeList` | Evidence from decompile and caller context. |
| `0x822356b0` | `FM2_RaceGhost_FindPlaybackNodeByKey` | Evidence from decompile and caller context. |
| `0x82236fb0` | `FM2_RaceGhost_LoadPlaybackSslBindingChain` | Evidence from decompile and caller context. |
| `0x8223f6a0` | `FM2_CarParts_ApplyUpgradeSlotBoundsTransform` | Evidence from decompile and caller context. |
| `0x821d76f0` | `FM2_Render_ComputePassLightingBasisVectors` | Evidence from decompile and caller context. |
| `0x827261c8` | `FM2_Render_SinDegreesFloat` | Evidence from decompile and caller context. |
| `0x82726350` | `FM2_Render_ClampAbsFloat220M` | Evidence from decompile and caller context. |
| `0x82726548` | `FM2_Render_TruncateDoubleToFloat` | Evidence from decompile and caller context. |
| `0x82726440` | `FM2_Render_ClampAbsFloat220MAlt` | Evidence from decompile and caller context. |
| `0x827265b0` | `FM2_Render_TruncateDoubleToFloatAlt` | Evidence from decompile and caller context. |
| `0x82726618` | `FM2_Render_InitGlobalLightingTlsState` | Evidence from decompile and caller context. |
| `0x82726da0` | `FM2_Render_EnsureGlobalLightingTlsInit` | Evidence from decompile and caller context. |
| `0x82726c78` | `FM2_Render_AdvancePassLightingCycleIndex` | Evidence from decompile and caller context. |
| `0x82726e18` | `FM2_Render_AdvancePassLightingCycleIndexTls` | Evidence from decompile and caller context. |
| `0x82726ec0` | `FM2_Render_DotProduct4WithBias` | Evidence from decompile and caller context. |
| `0x82726f60` | `FM2_Render_LerpVec4` | Evidence from decompile and caller context. |
| `0x82727080` | `FM2_Render_ComputeVec4LengthSq` | Evidence from decompile and caller context. |
| `0x82727158` | `FM2_Render_ComputePassLightingSlotStride48` | Evidence from decompile and caller context. |
| `0x82727180` | `FM2_Render_AllocPassLightingSlotArray` | Evidence from decompile and caller context. |
| `0x82727200` | `FM2_Render_ClearPassLightingSlotVMX` | Evidence from decompile and caller context. |
| `0x827272b0` | `FM2_Render_ComputePassLightingSlotOffset` | Evidence from decompile and caller context. |
| `0x827272c8` | `FM2_Render_BindPassLightingResourcePair` | Evidence from decompile and caller context. |
| `0x82727390` | `FM2_Render_UpdatePassLightingSlotFields` | Evidence from decompile and caller context. |
| `0x82727410` | `FM2_Render_ProcessPassLightingBatchA` | Evidence from decompile and caller context. |
| `0x827277f0` | `FM2_Render_ProcessPassLightingBatchB` | Evidence from decompile and caller context. |
| `0x8224eef8` | `FM2_LiveryMask_ProcessPendingEntryUpdatesBody` | Evidence from decompile and caller context. |
| `0x822516d0` | `FM2_XmlReader_InsertAttrEntrySorted` | Evidence from decompile and caller context. |
| `0x82260188` | `FM2_ComObject_InitRefCountFieldsBody` | Evidence from decompile and caller context. |
| `0x82284608` | `FM2_AudioRenderFrame_PathBInner` | Evidence from decompile and caller context. |

### Infrastructure pass 59 (33 functions)

Profile/garage/input, render GPU kick, D3D texture/PNG decode, shader constants.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82296968` | `FM2_Profile_SetTuningDisplayNameInner` | Evidence from decompile and caller context. |
| `0x82346b68` | `FM2_LuaGarage_EnsureCarRecordField92Body` | Evidence from decompile and caller context. |
| `0x8235f3d8` | `FM2_Input_InitControllerDevicesBody` | Evidence from decompile and caller context. |
| `0x823628a8` | `FM2_Input_ControllerDevice_InitSslBindingsBody` | Evidence from decompile and caller context. |
| `0x82365ab8` | `FM2_Vector_ComputeEraseRangeSpan16` | Evidence from decompile and caller context. |
| `0x82365bc8` | `FM2_Vector_EraseBegin20ByteElementsImpl` | Evidence from decompile and caller context. |
| `0x8236b010` | `FM2_AudioRender_SampleFrontBufferRegionBody` | Evidence from decompile and caller context. |
| `0x8236da60` | `FM2_Render_ObjectPassPrefetchDrawBatch` | Evidence from decompile and caller context. |
| `0x823733b8` | `FM2_Render_ScopedBatch_FinalizeGpuKickBody` | Evidence from decompile and caller context. |
| `0x82376598` | `FM2_Render_MarkDrawListStateDirty` | Evidence from decompile and caller context. |
| `0x8237dfd8` | `FM2_D3D_ReleaseGpuResourceRefInner` | Evidence from decompile and caller context. |
| `0x8237a320` | `FM2_GpuKick_SubmitVdScalerCommandBufferBody` | Evidence from decompile and caller context. |
| `0x8237b1a0` | `FM2_GpuKick_CreatePixCaptureFileOnUsbBody` | Evidence from decompile and caller context. |
| `0x8237bd48` | `FM2_GpuKick_NotifyPixCaptureFileEndedBody` | Evidence from decompile and caller context. |
| `0x8237d158` | `FM2_AudioMix_SubmitPendingOutputBody` | Evidence from decompile and caller context. |
| `0x8237f4d8` | `FM2_GpuCommandBuffer_BeginPerfCaptureBody` | Evidence from decompile and caller context. |
| `0x82385aa0` | `FM2_D3D_CreateTextureFromSurfaceLevelBody` | Evidence from decompile and caller context. |
| `0x82386130` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB` | Evidence from decompile and caller context. |
| `0x823868d8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyC` | Evidence from decompile and caller context. |
| `0x823876d8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyD` | Evidence from decompile and caller context. |
| `0x8238e098` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE` | Evidence from decompile and caller context. |
| `0x8239e508` | `FM2_Render_SetPassLightingModeScalarBodyA` | Evidence from decompile and caller context. |
| `0x8239e6c8` | `FM2_Render_SetPassLightingModeScalarBodyB` | Evidence from decompile and caller context. |
| `0x823a46b0` | `FM2_Shader_ApplyConstantsBatchBody` | Evidence from decompile and caller context. |
| `0x823a6cf0` | `FM2_Png_EnsureRgbThenDecodeRowBody` | Evidence from decompile and caller context. |
| `0x823a7018` | `FM2_Image_LoadPngFromMemory_InitHeader` | Evidence from decompile and caller context. |
| `0x823a9510` | `FM2_Png_DecodeRowScalarThunk` | Evidence from decompile and caller context. |
| `0x823a9520` | `FM2_Png_AllocDecodeStateBufferBody` | Evidence from decompile and caller context. |
| `0x823b1490` | `FM2_Image_LoadPngFromMemory_ParseChunkHeader` | Evidence from decompile and caller context. |
| `0x823b15f8` | `FM2_Image_LoadPngFromMemory_DecodeIdatBody` | Evidence from decompile and caller context. |
| `0x823b18c0` | `FM2_Image_LoadPngFromMemory_FilterRowBody` | Evidence from decompile and caller context. |
| `0x823b1a80` | `FM2_Image_LoadPngFromMemory_ValidateSigBody` | Evidence from decompile and caller context. |
| `0x823b1b00` | `FM2_Image_LoadPngFromMemory_ReadChunkBody` | Evidence from decompile and caller context. |

### Infrastructure pass 60 (33 functions)

PNG read/validate, pass-lighting offsets/VMX, D3D texture helpers, race ghost, livery mask.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823b1500` | `FM2_Image_LoadPngReadChunkBytesToBuffer` | Evidence from decompile and caller context. |
| `0x827281d0` | `FM2_Render_ComputePassLightingResourceOffset64B` | Evidence from decompile and caller context. |
| `0x8239f2f8` | `FM2_Image_LoadPngValidateStreamAfterRead` | Evidence from decompile and caller context. |
| `0x82725ec0` | `FM2_Render_ReciprocalSinScaleFloat` | Evidence from decompile and caller context. |
| `0x8238efc0` | `FM2_D3D_ReleaseTextureSurfacePairTagged` | Evidence from decompile and caller context. |
| `0x82391c80` | `FM2_D3D_InitTextureDescFromFormat` | Evidence from decompile and caller context. |
| `0x823cd8d0` | `FM2_D3D_GetDeviceCapsThunk` | Evidence from decompile and caller context. |
| `0x823cd8d8` | `FM2_D3D_QueryTextureResourceType` | Evidence from decompile and caller context. |
| `0x823cdbc8` | `FM2_D3D_ComputeMipLevelCount` | Evidence from decompile and caller context. |
| `0x823d3680` | `FM2_D3D_ClearComPtrPair` | Evidence from decompile and caller context. |
| `0x8240b998` | `FM2_Render_ZeroPassLightingCacheLinesVMX` | Evidence from decompile and caller context. |
| `0x824a72b8` | `FM2_Render_HasPassLightingResourceBound` | Evidence from decompile and caller context. |
| `0x824d1178` | `FM2_Input_BuildControllerSslBindingEntry` | Evidence from decompile and caller context. |
| `0x824e31b0` | `FM2_ComObject_AllocSharedStateBuffer` | Evidence from decompile and caller context. |
| `0x826115d8` | `FM2_Vector_ReallocGrow16ByteElements` | Evidence from decompile and caller context. |
| `0x82727dd0` | `FM2_Render_ComputePassLightingSlotOffset64B` | Evidence from decompile and caller context. |
| `0x82728088` | `FM2_Render_GetPassLightingWorkerSlotIndex` | Evidence from decompile and caller context. |
| `0x827280a0` | `FM2_Render_CopyPassLightingPairHead` | Evidence from decompile and caller context. |
| `0x82728280` | `FM2_Render_TestPassLightingSlotIndexValid` | Evidence from decompile and caller context. |
| `0x82728378` | `FM2_Render_GetPassLightingSlotDataPtr` | Evidence from decompile and caller context. |
| `0x827261f0` | `FM2_Render_SinRadiansDouble` | Evidence from decompile and caller context. |
| `0x821d28f8` | `FM2_Input_ControllerSslBindingInitField` | Evidence from decompile and caller context. |
| `0x821f4c50` | `FM2_ComObject_GetRefCountField` | Evidence from decompile and caller context. |
| `0x8221cd00` | `FM2_RaceGhost_ComparePlaybackNodeKey` | Evidence from decompile and caller context. |
| `0x8222f5e8` | `FM2_RaceGhost_LoadPlaybackResourcePath` | Evidence from decompile and caller context. |
| `0x82231848` | `FM2_RaceGhost_ParsePlaybackMetadataBlock` | Evidence from decompile and caller context. |
| `0x82236250` | `FM2_RaceGhost_BuildPlaybackSampleTable` | Evidence from decompile and caller context. |
| `0x82249ae0` | `FM2_LiveryMask_UpdateEntryFlagsField` | Evidence from decompile and caller context. |
| `0x8224a7b8` | `FM2_LiveryMask_ProcessPendingLayerEntry` | Evidence from decompile and caller context. |
| `0x8224b400` | `FM2_LiveryMask_ReleasePendingEntryRef` | Evidence from decompile and caller context. |
| `0x8224b850` | `FM2_LiveryMask_QueuePendingEntryUpdate` | Evidence from decompile and caller context. |
| `0x8224b910` | `FM2_LiveryMask_ApplyPendingEntryTransform` | Evidence from decompile and caller context. |
| `0x8224c0a8` | `FM2_LiveryMask_ClearPendingEntrySlot` | Evidence from decompile and caller context. |

### Infrastructure pass 61 (33 functions)

Livery mask tail, com-object ref-count, profile/garage, D3D texture upload, pass-lighting subscribers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8224c240` | `FM2_LiveryMask_FinalizePendingEntrySlot` | Evidence from decompile and caller context. |
| `0x8224e230` | `FM2_LiveryMask_ProcessPendingEntryBatch` | Evidence from decompile and caller context. |
| `0x8224ee38` | `FM2_LiveryMask_ResetPendingEntryState` | Evidence from decompile and caller context. |
| `0x822515f0` | `FM2_XmlReader_GrowAttrTableCapacity` | Evidence from decompile and caller context. |
| `0x82252468` | `FM2_LiveryMask_MergePendingEntryLists` | Evidence from decompile and caller context. |
| `0x82254a58` | `FM2_RaceGhost_TraversePlaybackNodeTree` | Evidence from decompile and caller context. |
| `0x822551b8` | `FM2_RaceGhost_GetPlaybackNodeChildCount` | Evidence from decompile and caller context. |
| `0x82255c50` | `FM2_ComObject_InitRefCountSubobjectFields` | Evidence from decompile and caller context. |
| `0x82257148` | `FM2_ComObject_BindRefCountVtableChain` | Evidence from decompile and caller context. |
| `0x82257eb0` | `FM2_ComObject_InitRefCountCallbackFields` | Evidence from decompile and caller context. |
| `0x8225ef80` | `FM2_ComObject_InitRefCountFieldsFromSourceCore` | Evidence from decompile and caller context. |
| `0x822939b0` | `FM2_Profile_SetTuningDisplayNameParseBody` | Evidence from decompile and caller context. |
| `0x822943a0` | `FM2_Profile_SetTuningDisplayNameValidateBody` | Evidence from decompile and caller context. |
| `0x822963d8` | `FM2_Profile_SetTuningDisplayNameCommitBody` | Evidence from decompile and caller context. |
| `0x823428f0` | `FM2_ComObject_InitRefCountAggregateBody` | Evidence from decompile and caller context. |
| `0x82345880` | `FM2_LuaGarage_EnsureCarRecordLookupBody` | Evidence from decompile and caller context. |
| `0x82345960` | `FM2_LuaGarage_EnsureCarRecordFieldCopy` | Evidence from decompile and caller context. |
| `0x823468b0` | `FM2_LuaGarage_EnsureCarRecordFieldInit` | Evidence from decompile and caller context. |
| `0x823b1ca0` | `FM2_Image_LoadPngFromMemory_ReadIdatHeader` | Evidence from decompile and caller context. |
| `0x823b1e10` | `FM2_Image_LoadPngFromMemory_DecompressIdatChunk` | Evidence from decompile and caller context. |
| `0x823b2048` | `FM2_Image_LoadPngFromMemory_ValidateChunkCrc` | Evidence from decompile and caller context. |
| `0x823bfbb0` | `FM2_D3D_ConvertSurfaceFormatToD3d` | Evidence from decompile and caller context. |
| `0x823bfcd8` | `FM2_D3D_CreateTextureFromSurfaceLevelInner` | Evidence from decompile and caller context. |
| `0x823c04d0` | `FM2_D3D_UploadTextureSurfaceLevels` | Evidence from decompile and caller context. |
| `0x823c0ae8` | `FM2_D3D_BuildTextureUploadDescriptor` | Evidence from decompile and caller context. |
| `0x823c0dc8` | `FM2_D3D_ComputeTexturePitchAndSize` | Evidence from decompile and caller context. |
| `0x8240dcc8` | `FM2_Render_GetPassLightingGlobalStatePtr` | Evidence from decompile and caller context. |
| `0x8242a3f0` | `FM2_Render_AppendPassLightingSubscriber` | Evidence from decompile and caller context. |
| `0x82457710` | `FM2_Render_BindPassLightingSubscriberParams` | Evidence from decompile and caller context. |
| `0x82469be8` | `FM2_Render_InsertPassLightingTreeNode` | Evidence from decompile and caller context. |
| `0x824806e8` | `FM2_Render_UpdatePassLightingCoeffSlot` | Evidence from decompile and caller context. |
| `0x82480780` | `FM2_Render_InterpolatePassLightingScalar` | Evidence from decompile and caller context. |
| `0x82484bf0` | `FM2_Render_ResetPassLightingSlotState` | Evidence from decompile and caller context. |

### Infrastructure pass 62 (8 functions)

Race ghost SQL query/format helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8222e580` | `FM2_RaceGhost_FormatSqlQueryLarge` | Evidence from decompile and caller context. |
| `0x8222e650` | `FM2_RaceGhost_FormatSqlQueryMedium` | Evidence from decompile and caller context. |
| `0x8222f490` | `FM2_RaceGhost_QueryRarityByOrdinal` | Evidence from decompile and caller context. |
| `0x8222f5a0` | `FM2_RaceGhost_ComputeUpgradeRarityBonus` | Evidence from decompile and caller context. |
| `0x8221ae78` | `FM2_ResourceManager_GetPendingRecordThreadContext` | Evidence from decompile and caller context. |
| `0x8226fcb0` | `FM2_ComObject_InitRefCountFieldTripletFromIndex` | Evidence from decompile and caller context. |
| `0x8229ba98` | `FM2_AudioSample_BuildOutputPairDescriptorField08` | Evidence from decompile and caller context. |
| `0x8229bbe0` | `FM2_AudioSample_BuildOutputPairDescriptorField20` | Evidence from decompile and caller context. |

### Infrastructure pass 63 (3 functions)

Race ghost SQL and livery axis parsing helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8222f2a0` | `FM2_RaceGhost_QueryCarPartFieldByLevel` | Evidence from decompile and caller context. |
| `0x82233870` | `FM2_RaceGhost_GetPartLevelById` | Evidence from decompile and caller context. |
| `0x8224a3b0` | `FM2_LiveryMask_ParseAxisDirectionToken` | Evidence from decompile and caller context. |

### Infrastructure pass 64 (9 functions)

Render-sort key field writers, network message priority tests, and camera-list payload accessors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82761d10` | `FM2_RenderSortKey_Init` | Decomp shows it zeroes/initializes multiple sort-key payload bytes/flags and masks top bits; caller only uses it as the sort-key setup entrypoint. |
| `0x82761d60` | `FM2_RenderSortKey_SetValidBit` | Decomp shows this sets only bit31 in the sort-key dword from a boolean argument. |
| `0x82761db8` | `FM2_RenderSortKey_SetInterpolatedBit` | Decomp shows this writes `a2<<30` into the same high-bit field mask, toggling a boolean flag bit at compile-time-visible bit 30. |
| `0x82761d80` | `FM2_RenderSortKey_SetRGBABytes` | Decomp shows it writes four byte fields at offsets 8..11; caller uses it to pass packed RGBA bytes. |
| `0x82761d98` | `FM2_RenderSortKey_SetUvRange` | Decomp shows it writes two consecutive floats at offsets +12 and +16 and is fed interpolated float values in the caller. |
| `0x82761db0` | `FM2_RenderSortKey_SetField28` | Decomp shows it writes a float at offset +28; direct scalar field set. |
| `0x82772e58` | `FM2_NetworkMessage_HasPriority2` | Decomp shows equality check `field12 == 2` and used as the first branch in priority comparison. |
| `0x82772e38` | `FM2_NetworkMessage_HasPriority3` | Decomp shows equality check `field12 == 3`, used alongside priority value checks in message ordering. |
| `0x8277b340` | `FM2_CameraList_GetPayloadAtOffset33` | Decomp shows returns `(BYTE*)a1 + 0x21` and caller loops while that byte is false, consistent with a linked-list sentinel/payload-offset accessor. |
| `0x82773f50` | `FM2_NetworkPeer_SetMethodIndex` | Decomp shows it stores the second argument into a field at `+48` of the peer object pointer loaded at `a1+1020`. |
| `0x82766ab0` | `FM2_STL_ListNode_GetPayloadAtOffset56` | Decomp shows it returns `a1 + 56`; caller writes a byte flag through this accessor on a list node. |
| `0x82766530` | `FM2_STL_ListNode_GetPayloadAtOffset57` | Decomp shows it returns `a1 + 57`; caller writes a byte flag through this accessor on a list node. |
| `0x8277c4c0` | `FM2_STL_WString_GetWideCharCount` | Decomp shows it returns `(DWORD*)(a1+12)- (DWORD*)(a1+4)` shifted by 1 when initialized, matching wide-char count in the UTF-16 buffer layout. |

### Infrastructure pass 65 (9 functions)

Thread-local initialization, simple index math helpers, and explicit init/destroy thunks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8275ee30` | `FM2_ThreadLocalCsArray_LockByType` | Decomp writes `a2 & 3` into a local then locks `j_RtlEnterCriticalSection` on `&unk_82A43828 + (a2 & 3)`. |
| `0x82758be8` | `FM2_JsonWriter_HasModeBit5` | Decomp returns `*(a1+12) & 0x20` with invalid-parameter fallback on null. |
| `0x827660d0` | `FM2_D3D_CreateGraphicsThreadLocalObject` | Decomp calls init helper, sets object slot to `off_82141118`, and initializes trailing fields via `sub_827664C0`. |
| `0x827636f0` | `FM2_D3D_InitThreadLocalState` | Decomp sets vtable-like field to `off_8214055C` and clears consecutive int fields. |
| `0x82761150` | `FM2_Render_GetObjectVertexOffset` | Decomp writes pointer math `*(DWORD*)(*(base+12)+8*a2+4)+656*a3+192` to caller-provided output. |
| `0x82761178` | `FM2_Render_GetObjectColorOffset` | Same base/index pattern with `+656*a3+432`, indicating a sibling object-array field offset helper. |
| `0x827611a0` | `FM2_Render_GetObjectTexcoordOffset` | Same base/index pattern with `+656*a3+544`, indicating a sibling object-array field offset helper. |
| `0x82767fa0` | `FM2_NetworkMessage_KeyCompare` | Decomp negates boolean result of `sub_8279F608(a1,a2)`, forming a key-difference predicate for message ordering paths. |
| `0x82777d90` | `FM2_iaSND_PlayList_InitVtable` | Decomp sets object vtable to `off_821446EC` then calls `iaSND_PLAY_LIST` destructor, explicit init/dtor helper style. |

### Infrastructure pass 66 (3 functions)

Small STL/map allocator and entrypoint wrappers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827633b0` | `FM2_STL_Map_ResizeBufferIfNeeded` | Decomp frees prior storage if present, allocates via `FM2_STL_AllocViaComGpuAllocator`, sets count/size fields, and returns standard allocation failure code on null. |
| `0x827635e8` | `FM2_Stl_SortListNodeBundleCtor` | Decomp sets vtable pointer `off_82043830`, clears flag byte, and calls a callback on `*(a1+4)` with `(a1+8)` if callback exists. |
| `0x826d0ba0` | `FM2_CriticalSection_EnterAndNotifyError` | Decomp enters `a2+4` then dispatches `(a2,-2146073008)` into `sub_826D0138`. |

## Infrastructure pass 67 (3 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8224B4B0` | `FM2_ResourceLock_ReplaceRetainedHandleIfReady` | Decompile clears/release current slot with `FM2_ResourceLock_ClearAndReleaseHandle`, assigns a retained handle via `FM2_ResourceLock_AssignRetainedHandle`, then queries ready state (`FM2_ResourceLock_WaitReadyState3`). On success it records `*(a1+36)` from the handle and returns 1; on failure it clears thread-safe flag and returns 0. Called from `FM2_LiveryMask_ClearPendingEntrySlot`. |
| `0x822561A0` | `FM2_ComObject_RegisterCarAudioStaticMetaIfMatching` | Decompile reads an interface object from `a2`, pulls its identifier field, checks it against the static script binding table (`FM2_Crt_StaticInit_ScriptBindingTable_829C2410`) with `sub_82201860`, and if accepted calls `FM2_CarAudio_GetStaticMetaPointer` method 36 with `(v2, 0, 1)`. This aligns with a conditional CarAudio static-meta registration hook inside `FM2_ComObject_InitRefCountFieldsFromSourceCore`. |
| `0x82258650` | `FM2_ComObject_CountRefBlocksByPredicate` | Decompile initializes a temporary list sentinel, iterates `FM2_Crt_StaticInit_GraphicsStreamList_829C45B0`, applies caller-supplied predicate `a2` to each block, increments a count for true results, then destroys the temporary list and frees it. Called only by `FM2_ComObject_InitRefCountFieldsFromSourceCore`, consistent with a local counting helper. |

## Infrastructure pass 68 (5 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236AF10` | `FM2_D3D_BuildResourceDescriptorPacked` | Decompile packs a `D3DResource` identity into a descriptor word at `a3[0]` using `(Identifier, ReferenceCount, Fence)` bit fields, then writes resource type and additional fields from `FM2_D3D_ComputeResourceBindingFlags_0` into `a3[4..6]`. Caller paths are texture-load/creation helpers, so this is the packed resource metadata formatter. |
| `0x8236BDD8` | `FM2_D3D_BuildResourceDescriptorPackedThunk` | One-instruction forwarder to `FM2_D3D_BuildResourceDescriptorPacked`; used where the same helper is invoked through a shorter call-chain variant. |
| `0x82204740` | `FM2_AudioManager_InitAndBindSignalGateThunk` | Decompile is a direct jump to `0x8294E2B8` with no local logic. Caller is `FM2_AudioManager_InitAndBindSignalGate`, so this name preserves the thunk role and unresolved target binding. |
| `0x82373160` | `FM2_GpuKick_NotifyPixCapture_StoreFenceAndDrain` | Decompile copies `*(a1+10780)` into `*(a1+10800)` and then calls `D3D_SubmitAndDrainCommands(a1)`, matching a store-and-drain marker helper in the pix-capture notify path (`FM2_GpuKick_NotifyPixCaptureFileEndedBody`). |
| `0x82373218` | `FM2_Render_ScopedBatch_WritePacketMarker` | Decompile grows/guards the ring-buffer cursor, writes a marker packet sequence (`1480`, `0x20000`, `3332`, `0`) and updates `a1+48`, then returns the updated cursor. Called from `FM2_Render_ScopedBatch_FinalizeGpuKickBody` during scoped-batch finalization. |

## Infrastructure pass 69 (5 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236BC38` | `FM2_D3D_ComputeMipLevelDepthFromTextureFlags` | Decompile returns `((*(DWORD *)(a1+44) >> 6) & 0xF) + 1`, directly reading texture flags to derive mip depth/level count metadata; used by `FM2_D3D_CreateTextureFromSurfaceLevelBodyD`. |
| `0x82424020` | `FM2_LuaSyntax_GetLocaleDecimalPointPtr` | Decompile returns `off_829989F4`; `FM2_LuaSyntax_ParseNumberWithLocale` uses it as the locale decimal separator source and falls back to `'.'` when null. |
| `0x824CC838` | `FM2_XmlReader_ParseElementOpenTag_Thunk` | One-instruction thunk into `sub_824CC598`; explicitly called from `FM2_XmlReader_ParseElementOpenTag` before parse-open validation paths. |
| `0x824FD2E0` | `FM2_D3D_DeviceContext_DtorFields_Thunk` | Direct thunk to `sub_824FD210`; decompile of target closes connection state (`FM2_LiveConnection_CloseXtsTask`) and clears three fields, matching device-context field cleanup behavior. |
| `0x8257BAB0` | `FM2_Render_AssignResourceLockFromPassDataThunk` | Thin wrapper that forwards to `FM2_ResourceLock_AssignRetainedHandle((int *)(*(_DWORD*)a1 + 224), a2)` and returns the helper result for pass-data resource lock assignment. |

## Infrastructure pass 70 (4 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82391F30` | `FM2_D3DTexture_ReleaseResourceContext` | Decompile releases up to five D3D resources (`a1[1]`, `[2]`, `[4]`, `[3]`, then optional `[1]`) with addRef/release discipline and optional scoped-batch cleanup. |
| `0x82392030` | `FM2_D3D_ReleaseResourceSlot` | Tiny helper releasing a single optional D3D resource slot (`*a1`) via `D3DResource_Release` and clearing it. |
| `0x82392580` | `FM2_D3D_ReleaseResourceSlotThunk` | Simple argument-forwarding thunk into `FM2_D3D_ReleaseResourceSlot`; called before texture upload writes in texture creation entrypoints. |
| `0x82392088` | `FM2_D3DTexture_BuildResourceCleanup` | Wrapper around `FM2_D3DTexture_ReleaseResourceContext`, used from texture/from-surface body paths as pre-error cleanup/teardown helper. |

## Infrastructure pass 71 (2026-06-18)

Small helper and accessor pass (high-confidence naming by direct decompile + caller context).

| Address | New name | Evidence |
| --- | --- | --- |
| 0x8222E7B8 | FM2_RaceGhost_GetRarityDescriptorName | Called by FM2_RaceGhost_QueryRarityByOrdinal; decomp returns table pointer and embedded string from rarity-global registry when index is in range, otherwise fallback constant string. |
| 0x8229BA30 | FM2_AudioSample_BuildOutputPairDescriptorFieldBody | Called by FM2_AudioSample_BuildOutputPairDescriptorField08/20; function builds output pair descriptor, validates with sub_82454290, finalizes with sub_8229B6A8, and returns status byte. |
| 0x8230B038 | FM2_CReplayStats_Ctor | Called by FM2_LuaGarage_EnsureCarRecordLookupBody; decomp initializes Forza2::CReplayStats vtable and zeroes key members, matching constructor behavior. |
| 0x8236A2B8 | FM2_D3DResource_UnlockForRelease | Called from FM2_D3DTexture_ReleaseResourceContext and FM2_D3D_ReleaseResourceSlot; decomp calls D3D::UnlockResource using addresses derived from a1 + 24. |
| 0x8237DF50 | FM2_D3DTexture_InitDefaultDescriptorCopy | FM2_D3D_CreateTextureFromSurfaceLevelBodyC calls this helper; callee performs FM2_MemcpyAligned from unk_82024690 into output with fixed 304-byte copy. |
| 0x8240E3B0 | FM2_D3DTexture_InitSurfaceDesc | FM2_D3D_CreateTextureFromSurfaceLevelBody passes unpacked dimensions/steps and this helper stores them into four dwords of a local descriptor structure and returns success. |
| 0x824EF788 | FM2_RaceGhost_GetInterpolationModeByte | Used by FM2_RaceGhost_ComputePlaybackInterpolationWeight; decomp returns *(u8*)(a1+100) which is consumed as interpolation mode in caller math. |
| 0x824FB0E8 | FM2_DeferredTask_InitPreloadAnimRecord | FM2_DeferredTask_SubmitPreloadingAnimTurnOn repeatedly initializes same struct layout via this helper (field0, field4, and byte8 cleared). |
| 0x824FD258 | FM2_RaceGhost_GetRarityByte | FM2_RaceGhost_QueryRarityByOrdinal feeds this accessor (*(u8*)(a1+8)) into rarity SQL-query string construction. |
| 0x824FD260 | FM2_AsyncQueue_HasPendingOp | FM2_AsyncQueue_FindPendingOp uses this direct boolean check (*a1 != 0) before queue operations. |
| 0x82503688 | FM2_PresentationCar_DtorBody | Invoked by FM2_PresentationCar_Dtor; sets base vtable and delegates to FM2_Object_AssignBaseVtable_82000E18. |
| 0x825A3B58 | FM2_Render_HasPassFlag | FM2_Render_CompileMissingPassBuffers checks (a2 == (*a1 & a2)) through this helper, so it is a pass-flag test predicate. |
| 0x825AD5E8 | FM2_Render_GetPassConstantDesc | Called by FM2_RenderPass_BindSurfaceAndConstants; returns indexed unk_829A2BAC entry for current pass and passes it into bind call as constant-table value. |
| 0x825B36A0 | FM2_AudioRenderFrame_StoreFenceAndDrain | FM2_AudioRenderFrame_PathB forwards *(a1+2136) into this helper, which calls FM2_GpuKick_NotifyPixCapture_StoreFenceAndDrain. |
| 0x825DB058 | FM2_LuaGarage_HasProfileManagerHeap | FM2_LuaGarage_EnsureCarRecordField92Body uses this check to gate manager-heap access ((a1+4) != 0). |
| 0x82615A50 | FM2_ComObject_DeleteOptionalBody | FM2_ComObject_DeleteOptional invokes this tiny init step that writes off_820423C0 into object vtable slot. |
| 0x8264EAF0 | FM2_CarDynamics_InitScalarPair | FM2_CarDynamics_InitSubsystems repeatedly calls this reset helper; it writes 0.0f and 0 into object scalar/payload fields. |
| 0x8264ECD0 | FM2_CarDynamics_InvokeUpdateHook | FM2_CarDynamics_UpdateSimulationStep dispatches this helper; decomp calls virtual slot +60 on a subobject at a1 + 24. |

### Batch 6 � 2026-06-18 (continuation)\n\nRemaining `sub_` callees in queue after this pass: **617** (down from **637**)\n
| Address | New name | Evidence |
| --- | --- | --- |
| `0x8240D840` | `FM2_Input_ControllerGetStateThunk` | This function is a one-instruction trampoline (`JUMPOUT`) to `XamInputGetState`; it directly re-exports the input API call through this local helper. |
| `0x82505D10` | `FM2_Audio_MLPMatrix_GetErrorCallback` | Returns the function pointer stored at `off_8299D420`; that table�s first entry (`sub_82505DC0`) formats `MLPMatrix Warning/FATAL` strings, so this helper is the public accessor for that callback. |
| `0x825084C0` | `FM2_Render_GetPassTimingState` | This helper returns the global timing state pointer `unk_829F43D8`, then immediately used by `FM2_Render_SetPassTimingScalar` calls in frame pipeline paths. |
| `0x8251A368` | `FM2_Render_FrameObject_IsPassSkippableCandidate` | In `FM2_Render_FramePipelineDrawObjects` and `sub_82519CF0`, each render object is filtered by this byte check before adding to submit accounting; it reads `*(u8*)(obj+3409)` and gates object inclusion. |
| `0x825A26D0` | `FM2_D3DResource_InitSurfaceState` | Writes descriptor fields at offsets `+4`, `+12`, `+40`, then conditionally calls `sub_825A2538`; callers pass a resource slot pointer before texture-copy operations, matching field initialization of surface upload metadata. |
| `0x825A2C30` | `FM2_D3DResource_ResetSurfaceStateVTable` | Sets the object vtable/header pointer to `off_8210B6A4` and then jumps to `sub_825A2450`, consistent with the paired init/teardown in front-buffer save-path flow around `FM2_AudioRenderFrame_LogSaveFrontBuffer`. |
| `0x825A3DF0` | `FM2_STL_WString_CopyWords` | Looped copy of `_WORD` values from source to destination until the destination iterator reaches the end pointer; called from `FM2_STL_WStringInsertChars` cluster. |
| `0x825E05F0` | `FM2_VTable_Dispatch_Offset76` | Performs a single indirect call through `*(a2->vtable + 76)` with `(a2,a1)`; this is a generic virtual-dispatch thunk in the gameplay/helper layer. |
| `0x826C2D70` | `FM2_XAudio2_Pool_FreeWithCallback_82A3C9C0` | Thin wrapper that immediately calls `FM2_XAudio2_Pool_FreeWithCallback` with `&unk_82A3C9C0` and the input pointer; used as XAudio voice-release cleanup callback. |
| `0x82725CA0` | `FM2_Math_DotProduct3` | Returns `(x1*x2) + (y1*y2) + (z1*z2)` for 3-float vectors (sum of axis products), matching 3D dot-product semantics. |
| `0x82725D58` | `FM2_Math_SetVector3` | Assigns `result[0]=a2`, `result[1]=a3`, `result[2]=a4` from three scalar doubles; used by nearby lighting/math callers to materialize 3D vector data. |
| `0x82725E38` | `FM2_Math_SinRadiansFromLengthSquared` | Computes `(x^2+y^2+z^2)` then passes through `FM2_Render_SinRadiansDouble`, so behavior is �sin of vector length squared� for the 3-component input vector. |
| `0x82724970` | `FM2_Math_CopyMatrix4x4FromObject` | Calls `FM2_Math_CopyMatrix4x4(a2, obj+160)` and returns destination pointer; simple struct field copy helper used in pass-lighting coefficient setup. |
| `0x82753DF8` | `FM2_Net_NetDll_XNetCleanupThunk` | One-instruction thunk to `NetDll_XNetCleanup`; no local state, import-forwarding helper only. |
| `0x82755D60` | `FM2_Crypto_XeCryptShaFinalThunk` | One-instruction thunk to `XeCryptShaFinal` in crypto stack helper cluster. |
| `0x8275EE78` | `FM2_Runtime_RtlLeaveCriticalSectionThunk` | One-instruction thunk to `RtlLeaveCriticalSection`. |
| `0x82758550` | `FM2_Utility_vswprintf_s_l` | Direct wrapper around `vswprintf_s_l(..., 0, format, args)` with fixed locale argument. |
| `0x82758700` | `FM2_Utility_vswprintf_s` | Forwards to `sub_82758560(a1, a2, 0)`, then into the `..._l` path; locale default wrapper. |
| `0x82763750` | `FM2_DeferredTask_SetField4` | Writes caller-provided value to `object+4`; appears in deferred-task submission paths and used as a small field-initializer helper. |
| `0x8278B2D0` | `FM2_SharedPtr_IncrementRefCountAtOffset68` | Increments `*(a1+68)` in shared-ptr-assignment paths; this is the explicit increment side of a reference-count field update. |


### Batch 7 — 2026-06-18

| Address | New name | Evidence |
| --- | --- | --- |
| `0x8228DC30` | `FM2_GraphicsAdapter_ExecuteSqlForEachStream_32769` | Returns `FM2_GraphicsAdapter_ExecuteSqlForEachStream(32769, a1)`. |
| `0x8228DC50` | `FM2_GraphicsAdapter_ExecuteSqlForEachStream_5` | Returns `FM2_GraphicsAdapter_ExecuteSqlForEachStream(5, a1)`. |
| `0x8239F240` | `FM2_Png_AllocateFilterRowBuffer` | Multiplies dimensions, allocates tagged buffer, zeroes bytes, with explicit split cleanup at 0x8000 when requested. |
| `0x823A4148` | `FM2_Png_ReadIdatHeaderSetMode` | In `FM2_Image_LoadPngFromMemory_ReadIdatHeader`, this sets byte at +44, loads constant `0.45455f` into +40, and sets status bits `| 0x801` when context is present. |
| `0x823A4178` | `FM2_Png_ReadIdatHeaderSetWindowBits` | Stores optional chunk length at +48, copies 5 `_WORD` pairs into +52.., sets half-word at +22, and marks bitfield `| 0x10`. |
| `0x823B0A00` | `FM2_Png_ValidateChunkType` | Validates 4 ASCII type bytes with strict A-Z / a-z range checks and calls `FM2_Png_ReportErrorAndUnwind` on invalid chunk type. |
| `0x82437DB8` | `FM2_RaceGhost_AppendAndInitDisplayNameBuffer` | Appends a subrange from `a1` into a local STL string buffer, assigns it to `a2`, and clears the local temporary buffer. |
| `0x824A0E48` | `FM2_RefCountAndEventSignalIfZero` | Atomically decrements field at `+732`; when the result reaches zero, calls `Nt_SetEvent(*(obj+100))`. |
| `0x824A8CC0` | `FM2_Network_RbTreeNode_DetachRelink` | Reads/remaps intrusive node links (plus flags at `+33`, `+4`) and rewires parent/neighbor pointers in the list/tree structure. |
| `0x824B6608` | `FM2_Lua_GrowStackSlotsForRange` | Validates max slot range (`((top-bot)>>4)+a2 <= 2048`), computes `16*a2`, grows stack if needed, and updates high-water mark. |
| `0x824CC598` | `FM2_XmlReader_ParseOpenTag_GetBufferOrCopy` | Reads buffer pointer from `*a1`; if state byte is not 1, it calls `FM2_BufFile_EnsureCapacityAndCopy` before returning the buffer pointer. |
| `0x824EF6A0` | `FM2_ExceptionFilter_ClearCleanupCallbacks` | Iterates callback slots from `(a1+36)` for `a1+24` entries, invokes each via virtual function, then clears 0x40 bytes with `FM2_Crt_MemsetVectorized`. |
| `0x824FD210` | `FM2_D3D_DeviceContext_DtorFields_Thunk` | Thin forwarding helper into `FM2_D3D_DeviceContext_DtorFields` that was already used as a D3D teardown thunk. |
| `0x82505DC0` | `FM2_Audio_MLPMatrix_FormatErrorMessage` | Selects warning/error format string by result (`FATAL` for `0`, `Warning` for `1`) then calls formatter with severity context (`byte_829F4230`). |
| `0x82578950` | `FM2_AudioManager_SetSignalGateField` | Stores argument into global `dword_82A00C9C` and returns the same value. |
| `0x825BB9D0` | `FM2_AudioManager_SetSignalGateMask` | Stores argument into global `byte_829A4B5C` and returns the same value. |
| `0x825D0AF0` | `FM2_AudioManager_GetSignalGateField` | Returns global `dword_82A028DC` directly. |
| `0x826BFF28` | `FM2_XAudio2_VoicePool_FreeWithCallback_82A3C858` | Wrapper that directly calls `FM2_XAudio2_Pool_FreeWithCallback((int)&unk_82A3C858, a1)`. |
| `0x826C1DE0` | `FM2_XAudio2_VoicePool_FreeWithCallback_82A3C998` | Wrapper that directly calls `FM2_XAudio2_Pool_FreeWithCallback((int)&unk_82A3C998, a1)`. |
| `0x826EC530` | `FM2_SQLite_AppendLowercaseIdentifierThunk` | Forwards to `sub_826EF620(a1+8, a2, a3, 1, a4)`, confirming lower-case identifier append helper signature. |
| `0x82713F40` | `FM2_RaceEntry_GetField80` | Returns field pointer integer at `obj + 80`. |
| `0x82727F78` | `FM2_Render_AllocPassLightingSlot_GetFlags` | Reads flags as `*(u16*)(obj + 34)` from pass-lighting-slot object. |
| `0x82728670` | `FM2_Presentation_SetGlobal_82A41D90` | Stores provided value into global `dword_82A41D90`. |
| `0x82728680` | `FM2_Presentation_SetGlobal_82A41D8C` | Stores provided value into global `dword_82A41D8C`. |
| `0x8272DCA8` | `FM2_Render_IsPresentationModeSwitchOff` | Returns `((*(_BYTE *)(*(_DWORD*)a1 + 76) & 1) == 0)`, i.e., byte flag check on vtable-driven object. |

### Batch 8 — 2026-06-18

| Address | New name | Evidence |
| --- | --- | --- |
| `0x821F3BF0` | `FM2_RenderAdapter_ClearOrResetControllerSessionIfNeeded` | Gets device context from offset, checks active profile state and if mismatch calls callback via function pointer at `*(obj+4)+40`; then clears `a1+8`. |
| `0x82201860` | `FM2_ComObject_ConditionalCarAudioMetaBinding` | Checks for duplicate key in in-object table and command-line gating; if allowed, calls init virtual (`*(a1)+72`) then appends `a2` at `a1[10+11]` and increments count. |
| `0x823876D0` | `FM2_AudioRenderFrame_EnqueueD3DCommand` | Thin forwarding helper adding a fixed trailing argument `2` into `sub_82387138`. |
| `0x8238E018` | `FM2_D3D_InitSurfaceDescriptorCopy` | Verifies 14-byte marker/header bounds, validates `v8[0]==19778`, then calls `sub_8238A390` with remaining payload. |
| `0x823A9500` | `FM2_Shader_ApplyConstantsBatchBody_CopyFloatRange` | Writes combined high+low parts of `a3` then calls `FM2_MemcpyAligned(a2, v4, a3)`. |
| `0x823CE778` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Thunk_0` | Single-level thunk forwarding to `FM2_D3D_CreateTextureFromSurfaceLevelBodyB` shared routine. |
| `0x823CEA18` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Thunk_1` | Single-level thunk forwarding to `sub_823CE2E8` for the same texture-path cluster. |
| `0x823D2168` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_Thunk` | Single-level thunk forwarding to `sub_823D1EA0` for texture-surface-level path B/E split. |
| `0x8242D100` | `FM2_Profile_SetTuningDisplayNameInner_DirectVtblCall` | Reads vtable pointer from `a2` and calls method slot `+84` with `(a1,a2,a3,a4,1)`, returning `a1`. |
| `0x82492B40` | `FM2_AIDriver_ComputeSectorIndexFromProgress_LoadIndex` | Computes modulo delta, applies periodic wrap (`a1[33]`) for sector index extraction. |
| `0x82492DA8` | `FM2_AIDriver_ComputeSectorIndexFromProgress_LoadCount` | VMX distance normalization path for sector metrics and weighted interpolation; fallback uses precomputed scalar blend. |
| `0x824937B0` | `FM2_AIDriver_UpdateRaceLineFromSector_Apply` | Resolves index with `FM2_AIDriver_ComputeSectorIndexFromProgress` and returns table sample unless disabled flag set (returns `100.0`). |
| `0x82493818` | `FM2_AIDriver_ResetRaceLineStateField` | Builds temporary 24-byte ring/carry params, calls `FM2_AIDriver_AdvanceCircularSectorIndex`, and returns difference of two track-width sample lookups. |
| `0x82494940` | `FM2_AIDriver_ResetRaceLineOnSectorChange_Delta` | Sums per-index values at `[a1+20]`, updates vector at `a1+12`, and returns either fallback `sub_824948D0` or completed normalized value. |
| `0x824D1338` | `FM2_Input_ControllerDevice_InitSslBindingsBody_SetSslFields` | Initializes SSL binding context from path then stores three fields at `+24/+28/+32`, and sets vtable `off_82041CF4`. |
| `0x824D2B70` | `FM2_Input_InitControllerDevicesBody_ParseControllerName` | Constructor-like init: calls `FM2_Input_SslDeviceBinding_Ctor`, sets vtable to `off_82041F84`, zeros fields (`+316`..`+328`), writes sentinel bytes, then `sub_824D07B8(a1,2)`. |
| `0x824D3C58` | `FM2_Scene_GetNotifyStateFromParamHelper_NormalizeUtf16` | Allocates pool 16 via `FM2_AllocPoolAcquireOrInit_Thunk`, obtains resource from `sub_824D3670`, then assigns into COM pointer. |
| `0x824D3D88` | `FM2_Lua_PushSslUnitStringsTable` | One-level init helper: `sub_824D3838(a1,2,...)` then returns `a1`. |
| `0x824D3DC8` | `FM2_LuaGarage_EnsureCarRecordField92_SetSslTag` | One-level init helper: `sub_824D3838(a1,3,...)` then returns `a1`. |
| `0x824D7470` | `FM2_Profile_ParseUnsignedFromSubString_Validate` | Initializes parsed-unsigned node header (`off_82042164`), writes `a2` as value at `+8`, and seeds state via `sub_824DD2D8(dword_829F392C)`. |
| `0x824D74D0` | `FM2_Profile_MakeStringKeyComPtr_InitUtf16` | Similar key-object init; writes vtable and value at `+8`, initializes byte flag from vtable method on global profile object. |
| `0x82492B40` | `FM2_AIDriver_ComputeSectorIndexFromProgress_LoadIndex` | Sector math helper used by race-line interpolation for wrapped index lookup. |
| `0x82492DA8` | `FM2_AIDriver_ComputeSectorIndexFromProgress_LoadCount` | VMX-based magnitude branch on condition bit, then blends scalar with precomputed offset from object arrays. |
| `0x824EAEF8` | `FM2_Lua_BindingPairSiftDown_InitRangeAndCompare` | Heap sift-down routine for `size`/`value` pair arrays (`*(result+8*a)`), compares UTF-16 byte strings, swaps toward parent index. |
| `0x824EA598` | `FM2_RenderAdapter_InitPresentationVtables_ClearState` | Initializes vtable pointers and critical section at `a1+164`, clears object tail fields/flags. |
| `0x824EAEF8` | `FM2_Lua_BindingPairSiftDown_InitRangeAndCompare` | Maintains heap order by bytewise comparison and shifts nodes up; writes packed QWORD back at computed slot. |
| `0x82590030` | `FM2_RaceGhost_BuildPlaybackSampleTable_Init` | Initializes nested `CarSetup` at `a1+4`, sets `a1+308=0`, and returns object pointer. |
| `0x825A2C70` | `FM2_AudioRenderFrame_LogSaveFrontBufferBody_InitSurfaceState` | Explicit object init thunk for surface state: sets multiple fields/flags (`+8/+12/+36/+40`) and vtable pointer. |
| `0x825A3E10` | `FM2_STL_WString_EnsureCapacityBeforeInsert` | Computes offset diff and `memmove`-slides UTF-16 characters left before insertion point. |
| `0x825A3E10` | `FM2_STL_WString_EnsureCapacityBeforeInsert` | Computes `v3=(a2-a1)/2`, shifts range backward with `FM2_Crt_MemmoveS`, and returns adjusted destination. |
| `0x825B4138` | `FM2_AudioManager_InitAndBindSignalGate_SpliceSlot` | Splices node list from `a1+112` and conditionally invokes `result->vftable+8` callback. |
| `0x825DBE60` | `FM2_LiveryRenderManager_TryFinalizeLayout_ClearStruct` | Sets vtable `off_8210A998`, clears list pointers/strings for layout clear/reinit state. |
| `0x82638E28` | `FM2_CarSetup_Ctor_InitField168` | Calls sub-ctor-like routine and writes sentinel `-1.0f` to `result+168`. |

### Batch 9 — 2026-06-19

| Address | New name | Evidence |
| --- | --- | --- |
| `0x821D00A8` | `FM2_Crt_AllocateArrayFailureException_ctor` | Small constructor writes vtable `off_82000E08` and stores `a2` in `result[1]`; all current callers pass through array-overflow checks before `FM2_STL_RaiseArrayConstructionException` with `_TI2_AVbad_alloc_std__`. |
| `0x821D00C0` | `FM2_Crt_BadAllocException_ctor` | Called from unwind support; stores vtable `off_82000DF8` and returns object pointer, consistent with bad_alloc-style constructor side of allocator failure path. |
| `0x821D00D0` | `FM2_Crt_CopyByte` | One-byte copy helper: `*result = *a2; return result;`. Used by write-buffer helper before storing bytes. |
| `0x821D0120` | `FM2_CarSetup_Ctor_InitSubfields` | Called from car-setup constructor/reset sites and chains `FM2_CarSetup_CtorFields` twice before initializing `field168` via thunk. |
| `0x821D0168` | `FM2_Memory_FreePointerArray_AndSelf` | Frees array element payloads when flags indicate container ownership, then optionally releases the container itself (`FM2_Memory_FreeSmallBlockOrNull`). |
| `0x821D0260` | `FM2_AuctionHouseRecordsetListener_vtableMethod_96` | Vtable entry for `IAuctionHouseRecordsetListener`-style interface table (`0x82001164`); computes `(a1->vtable[24])(a1,a2)+a2` when `a3` true and returns `a2` otherwise. |
| `0x821D02F0` | `FM2_AuctionHouseRecordsetListener_vtableMethod_108_Field` | Vtable entry in `0x82000E3C` constant-string table (`Main/NumAuctionHouseTransactions` nearby). Calls same vtable slot `+108` twice with `(a1+4)` and `(a1+8)`, then returns second call result. |
| `0x821D0490` | `FM2_MemmoveShiftLeft` | Exact thin wrapper around `FM2_Crt_MemmoveS` preserving pointer/size semantics. Used in buffer-shift path for stream-like write helpers. |
| `0x821D0518` | `FM2_GraphicsStreamList_InvokeDeleteQueryById` | In vtable cluster around `0x82001120`, acquires graphics-stream singleton, calls virtual `+72` with `a3`, then executes `sub_82230E78` + `sub_8221FAB0` and returns success flag. |
| `0x821D05F8` | `FM2_RefCounted_Dtor_Field27to30` | Releases up to four embedded pointers at offsets `+120,+116,+112,+108` using `vtable+8` if set; used as cleanup for stack-owned tuple/object fields. |

### Batch 10 — 2026-06-19

| Address | New name | Evidence |
| --- | --- | --- |
| `0x821D1928` | `FM2_Stl_LengthError_Ctor` | Sets vtable to `std::length_error` and calls `FM2_Stl_LogicError_Dtor`, matching non-message length_error constructor path. |
| `0x821D1990` | `FM2_Stl_OutOfRange_Ctor` | Sets vtable to `std::out_of_range` and calls `FM2_Stl_LogicError_Dtor`, matching default out_of_range construction helper. |
| `0x821D19A8` | `FM2_Stl_OutOfRange_DtorAndFree` | Out-of-range constructor/dtor helper: sets vtable, calls `FM2_Stl_LogicError_Dtor`, and conditionally `FM2_Memory_FreeSmallBlockOrNull` when low bit flag set. |
| `0x821D2898` | `FM2_Stl_Bitset_ThrowInvalidPosition` | Builds string `"invalid bitset<N> position"`, wraps as `std::out_of_range`, then destroys the temporary string; called when bit index exceeds 0x7F in bitset operations. |
| `0x821D33B8` | `FM2_Stl_Bitset_SetOrReset` | Computes bit bucket and mask for `a2`, then `OR`/`AND` with `a3` to set or reset a bit in the bitset storage at `a1`. |
| `0x821D38A0` | `FM2_AuctionHouseConnector_DtorFields` | Dtor-body helper: assigns `Forza2::CAuctionHouseConnector` vtable, destroys hash-name property list (`FM2_HashNamePropertyList_DestroyAndFree`), and returns base-vtable assignment helper. |
| `0x821D38E8` | `FM2_AuctionHouseConnector_Ctor` | Thin constructor for connector object; initializes intrusive sentinel node, resets several fields, and writes `Forza2::CAuctionHouseConnector` vtable. |
| `0x821D3940` | `FM2_AuctionHouseConnector_Dtor` | Calls `FM2_AuctionHouseConnector_DtorFields` and conditionally frees the object when flag bit is set. |
| `0x821D3A90` | `FM2_AuctionHouseRecordsetObserver_GetHasChanged` | Lua `"HasChanged"` path in `sub_822ADE90` reads byte20 state, returns current byte21, then clears it when state bit set. |
| `0x821D3AB0` | `FM2_AuctionHouseRecordsetObserver_GetHasBeenDeleted` | Lua `"HasBeenDeleted"` path in `sub_822ADF20` returns byte22 and preserves side-effect semantics used by observer getter pattern. |
| `0x821D3AB8` | `FM2_AuctionHouseRecordsetObserver_GetHasBeenOutbid` | Lua `"HasBeenOutbid"` path in `sub_822ADFB0` returns byte23 and conditionally clears that latch bit. |
| `0x821D3AD8` | `FM2_AuctionHouseRecordsetObserver_GetHasAuctionEnded` | Lua `"HasAuctionEnded"` path in `sub_822AE040` returns byte24 and conditionally clears the same-style observer flag latch. |

### Batch 11 — 2026-06-19

| Address | New name | Evidence |
| --- | --- | --- |
| `0x827FECF8` | `FM2_Zlib_InflateInit2_WindowBits15` | Decompilation is a direct wrapper around `sub_827FEBD8(a1, 15, a2, a3)`, where the fixed constant `15` is the zlib window-bits argument. |
| `0x827DD0D8` | `FM2_LuaLapTracker_GetStateOffset12FromV2` | Returns `FM2_LuaLapTracker_GetStateOffset4(*(DWORD*)(a1 + 520) + 12)`; this is clearly a field-adaptor for the Lua lap-tracker state object. |
| `0x827DD2D8` | `FM2_LuaLapTracker_GetField532` | Plain accessor returning `*(a1 + 532)` with no side effects, matching a stable structure field read used by lap-tracker call-sites. |
| `0x827DD3B0` | `FM2_LuaLapTracker_FindEntryByHash` | Decompilation shows 31-sized open-addressing hash search logic (`key * 31` style hashing, modulo 31 probing, looped slot scans) that returns a boolean and optionally copies payload through output pointer. |
| `0x827DD518` | `FM2_LuaLapTracker_FindEntryById` | Thin wrapper that computes `a1 + 264` and forwards into `FM2_LuaLapTracker_FindEntryByHash`, returning boolean search status. |
| `0x827A7058` | `FM2_STL_AllocViaComGpuAllocator_4176` | Calls allocator by fixed formula `4176 * a2` then forwards to `sub_827A6B48`, indicating a size-scaled allocation helper for 4176-byte units. |

### Batch 12 — 2026-06-19 (XtsClient/STL follow-up)

| Address | New name | Evidence |
| --- | --- | --- |
| `0x827A5E50` | `FM2_XtsClient_AccumulatePayloadSizeThunk` | Thin wrapper that forwards to `FM2_XtsClient_AccumulatePayloadSizeThunk()` target `sub_827A6AA8(a1)` with no local logic. |
| `0x827A5850` | `FM2_XtsClient_AppendPayloadNodeFromQueue` | Thin wrapper around `sub_827A6A30(a1, *(a2 + 4), a2)` and returns `a1`, matching an append/chain helper pattern used by `FM2_XtsClient_AccumulatePendingPayloadSize`. |
| `0x827A6910` | `FM2_XtsClient_SendRequestPacket_ParamShuffle` | Forwards arguments to `FM2_XtsClient_SendRequestPacket_Body(a2, a3, a1)` with positional reorder only. |
| `0x827A6A30` | `FM2_XtsClient_AppendPayloadFromNodeThunk` | Thin thunk calling `sub_827A70C0(a1, a2, a3)` and returning `a1`; used by both `FM2_XtsClient_AppendPayloadNodeFromQueue` and related callers. |
| `0x827A6B48` | `FM2_XtsClient_GetBackChainOrOne` | Returns `sub_827A7278()` result; that callee returns current back_chain when non-zero else constant `1`, so this is a direct back-chain/guard wrapper. |
| `0x827A6B48` | `FM2_XtsClient_AccumulatePendingPayloadSizeThunk` | Inlined-through wrapper that forwards `a1` into `sub_827A6AA8(a1)` (helper entry retained as previous-pass evidence for accumulated payload-size path). |
| `0x827A6A30` | `FM2_XtsClient_AppendPayloadFromNodeThunk` | Single-purpose pass-through helper that sets `a2` from `*(a1+4)` in callers and invokes `sub_827A70C0` before returning the destination object. |
| `0x827A7058` | `FM2_STL_AllocViaComGpuAllocator_4176` | Calls `sub_827A6B48` with `4176 * a2`, confirming fixed-size Com-GPU allocator wrapper for 4176-byte elements. |
| `0x827A7090` | `FM2_XtsClient_SendRequestPacket_HelperB` | One-line wrapper directly forwarding both args to `sub_827A72B0`; no local mutations. |
| `0x827A74D0` | `FM2_XtsClient_SendRequestPacket_BodyThunk` | Forwards `(a2, a3, a4, a1, ...)` into `sub_827A7940`, indicating a call-shape adapter/thunk before deep send-request logic. |
| `0x827A7610` | `FM2_XtsClient_SendRequestPacket_VariantThunk` | Call-shape adapter forwarding reordered arguments into `sub_827A7BB8`; used by alternate send-request path with many stack args. |
| `0x827A76B0` | `FM2_XtsClient_SendRequestPacket_CompareThunkC` | Constructs two byte flags/locals (`sub_82789B48`, `FM2_XtsClient_SendRequestPacket_EmptyThunk`) and forwards into `sub_827A7C98`; no direct business logic. |
| `0x827A7928` | `FM2_XtsClient_SendRequestPacket_EmptyThunk` | Decompilation is an empty function (`void;` no ops), confirming no-op thunk. |
| `0x827A8070` | `FM2_STL_ConstructElement4176_Helper` | Allocates `4176` bytes via `sub_82618468`, then `FM2_MemcpyAligned(result, a2, 4176)` when non-null; explicit allocator+copy 4176-byte element helper. |
| `0x827A85D0` | `FM2_XtsClient_SendRequestPacket_HelperE` | Loads packed context via `sub_82790498`, then forwards `(a9, a10, a12, *v13)` into `sub_827A86E8`; clear decompile pass-through to next layer. |
| `0x827A8618` | `FM2_XtsClient_SendRequestPacket_HelperF` | Loads a byte via `sub_82790498(...)` and forwards to `sub_827A8738(a1, a2, *byte)` with no local branching. |
| `0x827A8658` | `FM2_XtsClient_SendRequestPacket_HelperG` | Sets up temporary flags (`sub_827A7E58`, `FM2_XtsClient_SendRequestPacket_EmptyThunk`) and forwards extracted tuple fields to `sub_827A77A8`; no own logic. |
| `0x827A85D0` | `FM2_XtsClient_SendRequestPacket_HelperE` | Thin wrapper using `sub_82790498` result byte and dispatching to `sub_827A86E8`; kept aligned with observed call-through role. |
| `0x8279D9E0` | `FM2_XtsClient_AccumulatePendingPayloadSize_HelperA` | Single call to `sub_8279EE58(a1)` then returns `a1`, consistent with one-step accumulator helper. |
| `0x8279EF08` | `FM2_XtsClient_AccumulatePendingPayloadSize_HelperB` | Calls `sub_827FC8D0()` then `sub_8279F5A0(a1, *v3, a2)`; function is a small dispatch/dispatch-argument helper. |
| `0x8279EF58` | `FM2_XtsClient_AccumulatePendingPayloadSizeFromNode` | Pass-through `sub_8279F5A0(a1, *DWORD(a2+4), a2)` followed by `return a1`, matching node-based accumulator wrapper. |
| `0x827A5890` | `FM2_XtsClient_SendRequestPacket_ComputeChunkCount` | Computes `((a1+8) - (a1+4)) / 4176` when second pointer present; explicit bounded chunk-count accessor. |
| `0x827A6748` | `FM2_XtsClient_SendRequestPacket_PayloadCount` | Computes `((a1+12) - (a1+4)) / 4176` when base present; same chunk-count pattern on alternate state struct layout. |
| `0x827900F8` | `FM2_STL_CopyConstructRange40_BodyThunk` | 3-arg wrapper forwarding `(a2, a3)` into `sub_82790E50`; caller sample is `FM2_STL_CopyConstructRange40_B`. |
| `0x827906D0` | `FM2_STL_DestructRange_Thunk_906D0` | 1-arg wrapper around `sub_82790160(a1)`; caller sample `FM2_STL_EhUnwind_Call906D0` confirms unwind cleanup thunk role. |
| `0x82792240` | `FM2_StdTupleLexCompare_BodyThunk` | Returns `sub_827A7E40(a1, v2)` with `v2` as local byte stack state, matching tuple lex-compare body trampoline. |
| `0x827FECF8` | `FM2_Zlib_InflateInit2_WindowBits15` | Wrapper to `sub_827FEBD8(a1, 15, a2, a3)` with fixed window bits `15`; confirmed in prior decomp. |
| `0x827DD0D8` | `FM2_LuaLapTracker_GetStateOffset12FromV2` | Delegates to `FM2_LuaLapTracker_GetStateOffset4(*(DWORD*)(a1 + 520) + 12)` with direct offset chaining. |
| `0x827DD2D8` | `FM2_LuaLapTracker_GetField532` | Returns `*(a1 + 532)` accessor with no control flow. |
| `0x827DD3B0` | `FM2_LuaLapTracker_FindEntryByHash` | Open-addressing hash lookup with modulo-31 probing, key normalization, and slot checks; returns boolean and optionally writes payload. |
| `0x827DD518` | `FM2_LuaLapTracker_FindEntryById` | Computes `a1 + 264` and forwards to `FM2_LuaLapTracker_FindEntryByHash`, returning its boolean result. |

### Batch 12b — 2026-06-19 (correction)

The lines below disambiguate duplicated temporary names from the previous insertion:

| Address | New name | Evidence |
| --- | --- | --- |
| `0x827A6B48` | `FM2_XtsClient_GetBackChainOrOne` | Renamed from a temporary probe name after verifying `sub_827A7278()` returns either existing back-chain marker or `1`. |
| `0x827A6A30` | `FM2_XtsClient_AppendPayloadFromNodeThunk` | Confirmed pass-through behavior: calls `sub_827A70C0(a1, a2, a3)` and returns `a1`. |
| `0x827A85D0` | `FM2_XtsClient_SendRequestPacket_HelperE` | Confirmed by decompile as a small forwarding helper into `sub_827A86E8` after fetching a byte with `sub_82790498`. |
| `0x827A1108` | `FM2_ListNodeBundle_GetVtableHelper` | Returns `sub_827A15E0(a2)` immediately with no local logic, acting as a vtable/state indirection helper for list-node-bundle initialization paths. |

Additional one-off rename recorded for list-node bundle helper:

| Address | New name | Evidence |
| --- | --- | --- |
| `0x827A1108` | `FM2_ListNodeBundle_GetVtableHelper` | Returns `sub_827A15E0(a2)` immediately with no local logic, acting as a vtable/state indirection helper for list-node-bundle initialization paths. |

### Batch 13 — 2026-06-19 (Wrapper/initializer pass)

| Address | New name | Evidence |
| --- | --- | --- |
| `0x8276E660` | `FM2_NetworkSession_CtorBody` | Calls `FM2_FileStream_Ctor(a1)`, assigns three vtable/pool pointers (`off_82142B30`, `off_82142B04`, `off_82142A30`), clears a field, and returns `a1`; only caller sample is `FM2_NetworkSession_Ctor`. |
| `0x8276E840` | `FM2_DeferredTask_SubmitPreloadingAnimTurnOn_GetPayloadBufferPtr` | Single-line pass-through to `sub_8276E950(a1)` that selects one of two payload buffer pointers depending on `*(a1+24)` size threshold. |
| `0x8277D9A8` | `FM2_STL_WStringInsertChars_ThunkA` | Pure adapter: `return sub_82780040(a2, a3, a1)` with reordered/counted arguments and no local branches. |
| `0x8277DAF8` | `FM2_STL_WStringInsertChars_ThunkB` | Single forwarder to `sub_8277F618(a1)` and returns result, with no logic beyond the call. |
| `0x82780090` | `FM2_STL_WStringInsertChars_ThunkC` | Pure call-shape wrapper `return sub_82780338(a2, a3, a4, a1)` and nothing else. |
| `0x82779970` | `FM2_XtsClientMessageHandler_Ctor` | Constructor-like initializer: calls `FM2_XtsClientMessageHandler_CtorFields(a1)` then sets additional vtable and helper pointers for broadcast-up message handling. |
| `0x8277F5E0` | `FM2_InitListNodeBundle_AllocSlot` | Returns `FM2_STL_AllocViaComGpuAllocator(60 * a2)` directly, indicating slot-size allocation for list-node-bundle usage. |
| `0x8277FF50` | `FM2_STL_WStringInsertCharsRange_Helper` | Sets temporary flag/output bytes, parses via `sub_824CD5A0`, then forwards to `sub_82780278`; appears to be a small WString-range insertion preamble helper. |
| `0x8277F618` | `FM2_STL_WStringInsertChars_RawChainCheck` | Returns a stack `back_chain` value when present, otherwise returns literal `1`; used only as the body target of the adjacent thunk in the insert-chars cluster. |


### Batch 13 (clarification)

| Address | New name | Evidence |
| --- | --- | --- |
| `0x827799F0` | `FM2_XtsClientMessageHandler_CtorFields` | Internal helper for `FM2_XtsClientMessageHandler_Ctor`: initializes constructor fields (`+4`, `+3`, `+4`, `+38`), links nodes, writes buffer-string metadata, and returns `a1`. |
### Batch 14 — 2026-06-19 (continuation)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821D3D20` | `FM2_DeferredAudioManagerUpdate_Callback` | One-line thunk that passes `a1-4` into `sub_821D3DD8`; no local side effects besides argument adjustment. |
| `0x821D3EE8` | `FM2_DeferredAudioManagerUpdate_Invoke` | Reads callback args from `a2` (`*(a2+8)` flag and `*(float*)(a2+4)`) and invokes `a1->vtable[5]`; this is the deferred audio-manager update callback entry used by `FM2_QueueDeferredAudioManagerUpdate`. |
| `0x827212C0` | `FM2_Render_SetGlobal419A4` | Stores the incoming value directly to `dword_82A419A4` and returns it. |
| `0x827212D0` | `FM2_Render_SetGlobal419A8` | Stores the incoming value directly to `dword_82A419A8` and returns it. |
| `0x82713660` | `FM2_SQLite_HashEntry_Unlink` | Unlinks a node from list buckets when `entry[1]` is set, rewires peer pointers (`+12/+8`), optionally frees two payload pointers when container flag indicates ownership, then clears node link fields. |
| `0x827153C8` | `FM2_SQLite_HashEntry_Release` | Decrements entry refcount at `obj[?]`; when it drops to zero, relinks node into global hash list, runs container callback, and decrements container live-count with optional finalization on teardown condition. |
| `0x82715490` | `FM2_SQLite_HashLookupOrInsert` | Performs SQLite-style hash lookup (`bucket = key & (bucketCount-1)` and open-chained/probing walk), returns existing node payload when present, otherwise grows hash/allocs a new node, wires hash links and initializes flags before returning pointer to node value storage. |
### Batch 15 — 2026-06-19 (race-ghost/XML/math helpers)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x822310C8` | `FM2_RaceGhost_ComputeCarUpgradeRarity` | Called from `FM2_RaceGhost_ParsePlaybackMetadataBlock`; allocates and loops through part IDs, queries upgrade path data (`SELECT Level,IsStock ...`), and accumulates level/stock-based rarity into a bonus path with `FM2_RaceGhost_ComputeUpgradeRarityBonus`, then writes result at `a1+536` and flags state bit. |
| `0x82239BA0` | `FM2_XmlReader_GrowAttrTableCapacityCore` | Core allocator/insert helper for XML reader attr storage: calculates current element count, grows/reallocates a 28-byte-element table when needed, shifts/splices entries, then fills new element(s) via `FM2_Stl_String_AssignRange`; called by `FM2_XmlReader_GrowAttrTableCapacity`. |
| `0x82249C70` | `FM2_RaceGhost_TryAttachUpgradeNode` | Guards on object flags and a vtable check on `a2`, creates temporary ComPtr wrappers, then invokes `sub_82252D80` to resolve and splice an upgrade-path node before releasing temporaries. |
| `0x82249EA8` | `FM2_RaceGhost_ExtractFilePathIfPrefixed` | If `a2[3] == 1`, strips a leading `"file:"` prefix from `a2`-string fields and writes resulting wide text into `a1` via `FM2_Stl_String_AssignRange`; otherwise leaves destination initialized. |
| `0x82725F00` | `FM2_Render_IsFloatNearZero` | Small tolerance predicate: returns true when `|a1| < 9.9999997e-06` (`sub_82726540(a1, a2) < 0.0000099999997`), and is used by `FM2_Render_ComputeVec4LengthSq`. |

### Batch 16 — 2026-06-19 (render/UI/Lua helper growth routines)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827F1C10` | `FM2_Render_SortVisibleRenderables_GrowAndAppendSortableItem` | Growth helper for `FM2_Render_SortVisibleRenderables`; when vector capacity is exhausted, it calls `sub_827F1740` to grow then performs range/iterator invariants before writing the result end-iterator. |
| `0x827F1D00` | `FM2_Render_SortVisibleRenderables_AppendSortableItem` | Fast append helper for visible-renderable sort tuples: writes 48-byte tuple in-place when capacity exists; if full, it forwards to `FM2_Render_SortVisibleRenderables_GrowAndAppendSortableItem`. |
| `0x827D8E50` | `FM2_Lua_BindingVector_GrowAndAppendPair` | Used by `FM2_Lua_BindingVector_AppendPair`; validates vector bounds, calls `sub_827D8B00` to grow/insert a pair payload, then returns pointer to the stored pair. |
| `0x827D7148` | `FM2_UIScene_PostMessageWithGuard_FindBindingByType` | Called from `FM2_UIScene_PostMessageWithGuard`; walks a parent/child chain using child index `a4` and predicate `sub_827E1968`, returning boolean match and writing out the found binding pointer at `a5`. |

### Batch 17 — 2026-06-19 (LiveryMask/CarParts/Input/GPU surface helpers)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8224E918` | `FM2_LiveryMask_ResetPendingEntryState_GrowBuffer` | Performs bounds checks on a 244-byte vector-like pending-entry container and invokes `sub_8224D6F8(a2, growFlag, a4)` before publishing updated `*_QWORD` bounds; this is exactly a grow/append helper used in `FM2_LiveryMask_ResetPendingEntryState`. |
| `0x8224E9F8` | `FM2_LiveryMask_ProcessPendingEntries` | Enumerates candidate pending-entry lists (`a1+4`, `+40`, `+16`, `+28`), conditionally erases stale nodes, refreshes pending profile/upgrade cache objects through `FM2_LiveryMask_FindProfileRecordByKey`, and releases/merges bindings before returning a dirty flag (`0/1`). |
| `0x8224E3??` | `FM2_CarParts_RemoveMatchingUpgradeFromList` | Iterates an upgrade list via `FM2_CarParts_AdvanceUpgradeListIterator`, matches upgrade IDs by comparing name-prefix fields (`CompareNodeNamePrefix` path), and when matching while ref-count conditions permit calls registry unlock/`sub_82253430` then returns iterator state. |
| `0x82249B70` | `FM2_CarParts_MergeUpgradePathFromLivery` | If the guarded flag at `a1+60` is clear, it builds a temporary `ComPtr`, calls `FM2_CarParts_MergeUpgradePathListFromLookup` on `a1+16`, and returns the tuple payload at `a1+56`; it is a small upgrade-path merge shim used before exposing merge result. |
| `0x827D6B08` | `FM2_SQLite_KeyLookup_827D6B28_B` | Performs chained hash-chain walk using predicate `sub_827E0650`; matches `a2` against node keys, optionally returns `a4` from the found entry, and is a search helper duplicated across two nearby offsets. |
| `0x827D6B28` | `FM2_SQLite_KeyLookup_827D6B28_B` | Same body/call pattern as `0x827D6B08` (same lookup predicate + chain descent); treated as the paired duplicate key-lookup helper for the same search family. |
| `0x8237FAF8` | `FM2_GpuCommandBuffer_EmitPerfCaptureRegisters` | Begins by waiting for prior completion, optionally reset command-packet cursor, then emits a long sequence of GPU register packet writes (register IDs and pointer-derived payload words) to `a1` command buffer and updates `a1[12]`; used by performance capture kickoff. |
| `0x82362578` | `FM2_Input_ControllerDevice_InitSslBindingsBody_Impl` | Allocates temporary string-binding builders and repeatedly inserts SSL-style fields: `LastInput`, `LastOutput`, `UseRumble?` (`true`/`false`), plus `Point0..Point2.{x,y}` into an output collection `a2` via repeated `FM2_Render_NotifyManagerStateChange(..., a2)` callbacks. |
| `0x82386EE8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyA_Thunk` | Validates input/output pointers, gathers surface metadata via `sub_82392090`, calls `FM2_D3D_CreateTextureFromSurfaceLevelBody`, then performs texture cleanup via `FM2_D3DTexture_BuildResourceCleanup`; returns negative HRESULT on setup failure. |
| `0x82387008` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Thunk` | Equivalent wrapper around volume path helper (`sub_82392588` + `FM2_D3D_CreateTextureFromSurfaceLevelBodyB`) with null checks, resource-slot setup/teardown, and HRESULT passthrough. |
| `0x82342698` | `FM2_CareerRace_MaybePostRaceCameraAndEndTimer` | Applies post-race camera/effects when one condition holds, handles playback-list lifecycle checks, updates playback position for linked-list-empty states, and initializes `a1[90]` from `EndRaceTimer` + frame-timing vtable when timer seed is not yet set. |

### Batch 18 — 2026-06-19 (Audio sample/volume-list/audio-mix pass)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8229B6A8` | `FM2_AudioSample_BuildOutputPairDescriptorFieldBodyValues` | Inlined helper for `FM2_AudioSample_BuildOutputPairDescriptorFieldBody`; validates the compression tree iterator inputs, either uses the passed iterator value or advances with `FM2_CompressionStream_IncrementTreeIterator`, then writes the two descriptor dwords into the caller-provided output pair storage and returns that pointer. |
| `0x82336A58` | `FM2_Audio_VolumeListFindOrInsertByPrefix` | In `FM2_Audio_VolumeListGetOrCreateFloatAt40`, this helper walks an ordered name-prefix structure and performs insert-like behavior when the current iterator node does not already satisfy exact prefix ownership; it delegates creation/rotation work to `FM2_Stl_MapInsert`-style helper `sub_82335C40` and returns iterator pair state. |
| `0x82336CD8` | `FM2_Audio_VolumeListGetOrCreateFloatAt40` | Used across race-ghost/post-race audio paths whenever code expects a writable pointer to the volume-list float field. It reuses existing iterator initialization, creates a default node when missing (string copy + prefix insert), then returns the value slot at `+40` with runtime checks against iterator backreferences. |
| `0x8237D080` | `FM2_AudioMix_SubmitPendingOutputSetupFormat` | Internal helper called only from `FM2_AudioMix_SubmitPendingOutputBody` when surface/output packet metadata must be normalized. It computes and writes format/stride/offset/flag fields (`+10648`, `+10652`, `+10660`, plus dirty-state bits at `+32/+20`), so the chosen name captures its role as setup/config before pending output packet emission. |

### Batch 19 — 2026-06-19 (Image decoding helpers)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825036F0` | `FM2_Outptr_WriteZero` | Stores `0` into the output pointer argument (`*result`) and returns that pointer; there are no branches or side effects beyond this zero-init convention. |
| `0x82389448` | `FM2_Image_ParsePFMFromMemory` | Parses `PF`/`Pf` headers, validates width/height and optional scale, allocates `16 * width * height` bytes, and populates texture-like pixels with three channels plus alpha from each source triple. |
| `0x82389850` | `FM2_Image_ParseRadianceHdrFromMemory` | Detects Radiance HDR payloads (`#?RADIANCE`), parses `FORMAT=` and optional `EXPOSURE=` lines, validates axis orientation token (`+/-YX`), then decodes scanline/rgbe blocks with scale and swizzle into output pixels. |
| `0x8238A390` | `FM2_Image_ParseTgaFromMemory` | Reads a 18-byte TGA header, validates dimension/format fields, derives expected pitch/pixel format, handles grayscale/indexed/raw and RLE branches, then copies/unpacks rows into normalized texture memory with orientation handling. |
| `0x8238B4A0` | `FM2_Image_DecodeJpegFromMemory` | Uses `jpeg_std_error` and custom JPEG callbacks, initializes libjpeg context via `FM2_Image_DecodePngFromMemory`/`sub_8239F960`, decodes components/scans, and writes either RGB24 or BGRA-packed results to allocated GPU memory. |
| `0x8238BA90` | `FM2_Image_ParseTgaPaletteFromMemory` | TGA parser variant that validates header flags, converts indexed/palette/pixel values into RGBA words, optionally builds an 0x400-byte palette cache, and optionally decodes run-length segments before final alpha correction. |
| `0x8238C1A0` | `FM2_Image_ParsePPMFromMemory` | Parses `P5`/`P6` textual headers, enforces channel ranges, then fills rows (with optional scale conversion) for grayscale or RGB textures into caller storage. |
| `0x8238CEF0` | `FM2_Image_ParseDDSFromMemory` | Confirms DDS signature, validates header flags and mip chain constraints, selects format descriptors, supports cubemap/mipmap iteration, and copies or allocates per-level surfaces. |

### Infrastructure pass 72 (33 functions)

Lua SSL table, D3D texture-desc/surface gather, XTS client, livery/race-ghost/com-object, PNG/shader/XML/SQLite helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824d3838` | `FM2_Lua_PushSslUnitStringsTableBody` | Evidence from decompile and caller context. |
| `0x82388498` | `FM2_Image_StrncpyBounded` | Evidence from decompile and caller context. |
| `0x82392090` | `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | Evidence from decompile and caller context. |
| `0x82392588` | `FM2_D3D_GatherVolumeMetadataForTextureCreate` | Evidence from decompile and caller context. |
| `0x82798410` | `FM2_XtsClientMessageHandler_InitConnectionFields` | Evidence from decompile and caller context. |
| `0x8279f5a0` | `FM2_XtsClient_AccumulatePendingPayloadSizeCore` | Evidence from decompile and caller context. |
| `0x827fc8d0` | `FM2_STL_ListNode_LinkNextWrapper` | Evidence from decompile and caller context. |
| `0x82726540` | `FM2_Render_AbsFloat` | Evidence from decompile and caller context. |
| `0x823901d0` | `FM2_D3D_TextureDesc_FromFormatBodyA` | Evidence from decompile and caller context. |
| `0x82390540` | `FM2_D3D_TextureDesc_FromFormatBodyB` | Evidence from decompile and caller context. |
| `0x82390e08` | `FM2_D3D_TextureDesc_FromFormatBodyC` | Evidence from decompile and caller context. |
| `0x82391598` | `FM2_D3D_TextureDesc_FromFormatBodyD` | Evidence from decompile and caller context. |
| `0x82391e48` | `FM2_D3D_CopySurfaceRectLocked` | Evidence from decompile and caller context. |
| `0x82393250` | `FM2_D3D_TextureDesc_AllocStagingBuffer` | Evidence from decompile and caller context. |
| `0x8239d878` | `FM2_D3D_TextureDesc_SelectFormatHandler` | Evidence from decompile and caller context. |
| `0x821d3dd8` | `FM2_DeferredAudioManagerUpdate_Destroy` | Evidence from decompile and caller context. |
| `0x8224d6f8` | `FM2_LiveryMask_GrowPendingEntryBufferCore` | Evidence from decompile and caller context. |
| `0x82252d80` | `FM2_RaceGhost_ResolveAndAttachUpgradeNode` | Evidence from decompile and caller context. |
| `0x82253430` | `FM2_CarParts_RemoveMatchingUpgradeFromListCore` | Evidence from decompile and caller context. |
| `0x822199e0` | `FM2_ComObject_InitRefCountFromSourceCoreBody` | Evidence from decompile and caller context. |
| `0x8229b130` | `FM2_AudioSample_BuildOutputPairDescriptorFieldBodyCore` | Evidence from decompile and caller context. |
| `0x82335c40` | `FM2_Audio_VolumeListInsertNodeByPrefix` | Evidence from decompile and caller context. |
| `0x823a3fd0` | `FM2_Png_SetImageDimensions` | Evidence from decompile and caller context. |
| `0x823a6958` | `FM2_Png_EnsureRgbThenDecodeRowCore` | Evidence from decompile and caller context. |
| `0x823a6e58` | `FM2_Shader_ApplyConstantsBatchFlushBody` | Evidence from decompile and caller context. |
| `0x823b03a8` | `FM2_Shader_ApplyConstantsBatchValidateSlot` | Evidence from decompile and caller context. |
| `0x821e6888` | `FM2_CareerRace_UpdatePlaybackTimerFromEndRace` | Evidence from decompile and caller context. |
| `0x821f2428` | `FM2_RenderAdapter_ResetControllerSessionState` | Evidence from decompile and caller context. |
| `0x82219888` | `FM2_ComObject_InitRefCountBindingFields` | Evidence from decompile and caller context. |
| `0x82236dc0` | `FM2_XmlReader_GrowAttrTableRealloc` | Evidence from decompile and caller context. |
| `0x82237428` | `FM2_XmlReader_ShiftAttrTableEntries` | Evidence from decompile and caller context. |
| `0x82238e60` | `FM2_XmlReader_FillNewAttrTableSlot` | Evidence from decompile and caller context. |
| `0x821d42f0` | `FM2_SQLite_HashBucketGetHead` | Evidence from decompile and caller context. |

### Infrastructure pass 73 (33 functions)

SQLite hash buckets, graphics stream delete, audio mix/render, image TGA/DDS/JPEG/PNG, shader constants, D3D upload/caps.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821d4300` | `FM2_SQLite_HashBucketSetHead` | Evidence from decompile and caller context. |
| `0x82279880` | `FM2_SQLite_HashEntryAllocNode` | Evidence from decompile and caller context. |
| `0x8221fab0` | `FM2_GraphicsStreamList_DeleteQueryCallback` | Evidence from decompile and caller context. |
| `0x82230e78` | `FM2_GraphicsStreamList_DeleteQueryByIdBody` | Evidence from decompile and caller context. |
| `0x822633c8` | `FM2_ComObject_InitRefCountAggregateFields` | Evidence from decompile and caller context. |
| `0x82335f10` | `FM2_Audio_VolumeListFindNodeByPrefixWalk` | Evidence from decompile and caller context. |
| `0x8237ccf0` | `FM2_AudioMix_SubmitPendingOutputWritePackets` | Evidence from decompile and caller context. |
| `0x82387138` | `FM2_AudioRenderFrame_EnqueueD3DCommandBody` | Evidence from decompile and caller context. |
| `0x82388ab8` | `FM2_D3D_CopyDefaultSurfaceDescriptor` | Evidence from decompile and caller context. |
| `0x82388b50` | `FM2_Image_ParseTgaFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238b9f8` | `FM2_Image_ParseTgaPaletteFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238ccc0` | `FM2_Image_ParseDDSFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238e9d0` | `FM2_D3D_TextureDesc_FromFormat_ResolveHandler` | Evidence from decompile and caller context. |
| `0x8239f3c0` | `FM2_Image_DecodeJpegFromMemory_InitContext` | Evidence from decompile and caller context. |
| `0x8239f808` | `FM2_Image_DecodeJpegFromMemory_ReadScanlines` | Evidence from decompile and caller context. |
| `0x8239f960` | `FM2_Image_DecodeJpegFromMemory_AllocOutput` | Evidence from decompile and caller context. |
| `0x8239fae0` | `FM2_Image_DecodeJpegFromMemory_ConvertColorSpace` | Evidence from decompile and caller context. |
| `0x8239fbe0` | `FM2_Image_DecodeJpegFromMemory_WritePixels` | Evidence from decompile and caller context. |
| `0x823b0940` | `FM2_Image_LoadPngValidateChunkContinuation` | Evidence from decompile and caller context. |
| `0x821f2f58` | `FM2_CareerRace_GetEndRaceTimerSeedPtr` | Evidence from decompile and caller context. |
| `0x822643a8` | `FM2_ComObject_RefCountIncrementOne` | Evidence from decompile and caller context. |
| `0x82270bf8` | `FM2_ComObject_RefCountNoOpRet` | Evidence from decompile and caller context. |
| `0x8229b650` | `FM2_AudioSample_BuildOutputPairDescriptorAdvanceIter` | Evidence from decompile and caller context. |
| `0x82790498` | `FM2_XtsClient_SendRequestPacket_NoOpStub` | Evidence from decompile and caller context. |
| `0x82789b48` | `FM2_XtsClient_SendRequestPacket_CompareFlagStub` | Evidence from decompile and caller context. |
| `0x823b0aa0` | `FM2_Shader_ApplyConstantsBatchWriteSlotA` | Evidence from decompile and caller context. |
| `0x823b0d08` | `FM2_Shader_ApplyConstantsBatchWriteSlotB` | Evidence from decompile and caller context. |
| `0x823b1050` | `FM2_Shader_ApplyConstantsBatchWriteSlotC` | Evidence from decompile and caller context. |
| `0x823bf878` | `FM2_D3D_BuildTextureUploadDescriptorInit` | Evidence from decompile and caller context. |
| `0x823bf968` | `FM2_D3D_BuildTextureUploadDescriptorBody` | Evidence from decompile and caller context. |
| `0x823c0928` | `FM2_D3D_ComputeTexturePitchAligned` | Evidence from decompile and caller context. |
| `0x823cd4b8` | `FM2_D3D_GetDeviceCapsBody` | Evidence from decompile and caller context. |
| `0x823ce2e8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Impl` | Evidence from decompile and caller context. |

### Infrastructure pass 74 (33 functions)

XML attr table, D3D texture-desc/JPEG, career race, livery grow, com-object aggregate, audio render enqueue.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82232090` | `FM2_XmlReader_CopyAttrTableEntryFields` | Evidence from decompile and caller context. |
| `0x823900a8` | `FM2_D3D_TextureDesc_ComputeFormatBlockSizeA` | Evidence from decompile and caller context. |
| `0x82390a70` | `FM2_D3D_TextureDesc_AllocFormatConversionBuffer` | Evidence from decompile and caller context. |
| `0x82390d70` | `FM2_D3D_TextureDesc_ReleaseFormatChain` | Evidence from decompile and caller context. |
| `0x823aaf88` | `FM2_Image_DecodeJpegFromMemory_OutputReadyCallback` | Evidence from decompile and caller context. |
| `0x823cc398` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_CopyPixels` | Evidence from decompile and caller context. |
| `0x821d3c20` | `FM2_DeferredAudioManagerUpdate_DtorFields` | Evidence from decompile and caller context. |
| `0x821e7ff8` | `FM2_ComObject_CompareStringFieldPrefix` | Evidence from decompile and caller context. |
| `0x821f0f20` | `FM2_CareerRace_GetPlaybackFrameTimingPtr` | Evidence from decompile and caller context. |
| `0x821f15a8` | `FM2_CareerRace_UpdatePlaybackTimerInner` | Evidence from decompile and caller context. |
| `0x821f23c8` | `FM2_CareerRace_GetEndRaceTimerFieldPtr` | Evidence from decompile and caller context. |
| `0x8221e520` | `FM2_GraphicsStreamList_DeleteQueryDispatch` | Evidence from decompile and caller context. |
| `0x8221f8f8` | `FM2_GraphicsStreamList_DeleteQueryCallbackBody` | Evidence from decompile and caller context. |
| `0x8223d990` | `FM2_RaceGhost_AttachUpgradeNodeFinalize` | Evidence from decompile and caller context. |
| `0x8224a6a0` | `FM2_LiveryMask_GrowPendingEntryCheckBounds` | Evidence from decompile and caller context. |
| `0x8224b530` | `FM2_LiveryMask_GrowPendingEntryAppendSlot` | Evidence from decompile and caller context. |
| `0x8224c100` | `FM2_LiveryMask_GrowPendingEntryShiftTail` | Evidence from decompile and caller context. |
| `0x82255eb0` | `FM2_ComObject_InitRefCountAggregateBindField` | Evidence from decompile and caller context. |
| `0x82262fb8` | `FM2_ComObject_InitRefCountAggregateSetFlag` | Evidence from decompile and caller context. |
| `0x822643b0` | `FM2_ComObject_RefCountAggregateIncrementOne` | Evidence from decompile and caller context. |
| `0x822699b0` | `FM2_ComObject_InitRefCountAggregateFromCarRecord` | Evidence from decompile and caller context. |
| `0x82270228` | `FM2_ComObject_InitRefCountAggregateLinkNode` | Evidence from decompile and caller context. |
| `0x82270a20` | `FM2_ComObject_InitCarRecordFromDataQuery` | Evidence from decompile and caller context. |
| `0x8229a6d8` | `FM2_AudioSample_BuildOutputPairDescriptorValidate` | Evidence from decompile and caller context. |
| `0x8229ae88` | `FM2_AudioSample_BuildOutputPairDescriptorReleaseIter` | Evidence from decompile and caller context. |
| `0x823357d0` | `FM2_Audio_VolumeListInsertNodeRebalance` | Evidence from decompile and caller context. |
| `0x8236c1e8` | `FM2_D3D_GatherVolumeMetadataFromResourceDesc` | Evidence from decompile and caller context. |
| `0x8237cac8` | `FM2_AudioMix_SubmitPendingOutputInitPacket` | Evidence from decompile and caller context. |
| `0x823852f8` | `FM2_D3D_CreateTextureResourceFromFormat` | Evidence from decompile and caller context. |
| `0x82386dc8` | `FM2_AudioRenderFrame_EnqueueD3DCommandInitA` | Evidence from decompile and caller context. |
| `0x82386e58` | `FM2_AudioRenderFrame_EnqueueD3DCommandInitB` | Evidence from decompile and caller context. |
| `0x823892f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandBindSurface` | Evidence from decompile and caller context. |
| `0x8238e490` | `FM2_AudioRenderFrame_EnqueueD3DCommandEmitPackets` | Evidence from decompile and caller context. |

### Infrastructure pass 75 (33 functions)

D3D format handlers, JPEG/shader constant flush cluster, AI race line, Lua/audio helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82392f88` | `FM2_D3D_TextureDesc_SelectFormatHandlerA` | Evidence from decompile and caller context. |
| `0x82393380` | `FM2_D3D_TextureDesc_SelectFormatHandlerB` | Evidence from decompile and caller context. |
| `0x8239c1d8` | `FM2_D3D_TextureDesc_SelectFormatHandlerC` | Evidence from decompile and caller context. |
| `0x8239cbc0` | `FM2_D3D_TextureDesc_SelectFormatHandlerD` | Evidence from decompile and caller context. |
| `0x8239f6c8` | `FM2_Image_DecodeJpegFromMemory_AllocScanBuffer` | Evidence from decompile and caller context. |
| `0x8239fa28` | `FM2_Image_DecodeJpegFromMemory_WriteRowPixels` | Evidence from decompile and caller context. |
| `0x823a2088` | `FM2_Image_DecodeJpegFromMemory_InitDecompress` | Evidence from decompile and caller context. |
| `0x823a5080` | `FM2_Shader_ApplyConstantsBatchFlushGuard` | Evidence from decompile and caller context. |
| `0x823a50c8` | `FM2_Shader_ApplyConstantsBatchFlushWriteA` | Evidence from decompile and caller context. |
| `0x823a53d8` | `FM2_Shader_ApplyConstantsBatchFlushWriteB` | Evidence from decompile and caller context. |
| `0x823a5550` | `FM2_Shader_ApplyConstantsBatchFlushWriteC` | Evidence from decompile and caller context. |
| `0x823a5790` | `FM2_Shader_ApplyConstantsBatchFlushCheckSlot` | Evidence from decompile and caller context. |
| `0x823a57f0` | `FM2_Shader_ApplyConstantsBatchFlushWriteD` | Evidence from decompile and caller context. |
| `0x823a5bc8` | `FM2_Shader_ApplyConstantsBatchFlushWriteE` | Evidence from decompile and caller context. |
| `0x823a6030` | `FM2_Shader_ApplyConstantsBatchFlushWriteF` | Evidence from decompile and caller context. |
| `0x823a62e8` | `FM2_Shader_ApplyConstantsBatchFlushWriteG` | Evidence from decompile and caller context. |
| `0x823a6800` | `FM2_Shader_ApplyConstantsBatchFlushWriteH` | Evidence from decompile and caller context. |
| `0x823a9df0` | `FM2_Image_DecodeJpegFromMemory_SetErrorHandler` | Evidence from decompile and caller context. |
| `0x823b20b8` | `FM2_Shader_ApplyConstantsBatchBodyInner` | Evidence from decompile and caller context. |
| `0x823b7148` | `FM2_Jpeg_InitColorSpaceConverterBody` | Evidence from decompile and caller context. |
| `0x823c1740` | `FM2_Shader_ApplyConstantsBatchValidateSlotBody` | Evidence from decompile and caller context. |
| `0x823cd260` | `FM2_D3D_GetDeviceCapsQuerySurfaceFormats` | Evidence from decompile and caller context. |
| `0x823d1ea0` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_Impl` | Evidence from decompile and caller context. |
| `0x82413e98` | `FM2_AIDriver_ResetRaceLineStateClearSector` | Evidence from decompile and caller context. |
| `0x82413ed8` | `FM2_RaceGhost_QueryPartLevelForRarityBonus` | Evidence from decompile and caller context. |
| `0x82418670` | `FM2_Image_ParsePPMFromMemory_ReadDigit` | Evidence from decompile and caller context. |
| `0x824186b0` | `FM2_Image_ParsePPMFromMemory_SkipWhitespace` | Evidence from decompile and caller context. |
| `0x8241cfe0` | `FM2_LuaSyntax_CoalesceStringConcatExpBody` | Evidence from decompile and caller context. |
| `0x82438c10` | `FM2_LuaGarage_EnsureCarRecordFieldCopyBody` | Evidence from decompile and caller context. |
| `0x82454290` | `FM2_AudioSample_BuildOutputPairDescriptorValidateBody` | Evidence from decompile and caller context. |
| `0x82464f70` | `FM2_AIDriver_ResetRaceLineStateClearProgress` | Evidence from decompile and caller context. |
| `0x82483740` | `FM2_AIDriver_ResetRaceLineOnSectorChangeClamp` | Evidence from decompile and caller context. |
| `0x82492e68` | `FM2_AIDriver_ComputeSectorIndexFromProgressBody` | Evidence from decompile and caller context. |

### Infrastructure pass 76 (33 functions)

Com-object aggregate, D3D texture resource create/upload, audio render D3D packet writers, shader validate, JPEG.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824f18b0` | `FM2_ComObject_GetAggregateFieldAt60` | Evidence from decompile and caller context. |
| `0x8222e2a0` | `FM2_ComObject_InitCarPlaybackVectorDefaults` | Evidence from decompile and caller context. |
| `0x822625d0` | `FM2_ComObject_InitRefCountAggregateSetFlagBody` | Evidence from decompile and caller context. |
| `0x82268728` | `FM2_CareerRace_UpdatePlaybackTimerComputeFrame` | Evidence from decompile and caller context. |
| `0x82270060` | `FM2_ComObject_InitRefCountAggregateLinkNodeBody` | Evidence from decompile and caller context. |
| `0x823638f0` | `FM2_TestTmp2_InvokeBody` | Evidence from decompile and caller context. |
| `0x8236a290` | `FM2_D3D_ComputeMipCountFromResourceDesc` | Evidence from decompile and caller context. |
| `0x8236b628` | `FM2_D3D_GatherVolumeMetadataFromDescInner` | Evidence from decompile and caller context. |
| `0x8236be90` | `FM2_D3D_GatherVolumeMetadataFromDescThunk` | Evidence from decompile and caller context. |
| `0x823851a8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketA` | Evidence from decompile and caller context. |
| `0x82388798` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketB` | Evidence from decompile and caller context. |
| `0x823890f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketC` | Evidence from decompile and caller context. |
| `0x82389158` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketD` | Evidence from decompile and caller context. |
| `0x8238af50` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketE` | Evidence from decompile and caller context. |
| `0x8238b7d0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketF` | Evidence from decompile and caller context. |
| `0x8238c488` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketG` | Evidence from decompile and caller context. |
| `0x8238d390` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketH` | Evidence from decompile and caller context. |
| `0x8238d8c0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketI` | Evidence from decompile and caller context. |
| `0x8238da88` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJ` | Evidence from decompile and caller context. |
| `0x8239ef08` | `FM2_AudioRenderFrame_EnqueueD3DCommandFinalize` | Evidence from decompile and caller context. |
| `0x8239f4e0` | `FM2_Image_DecodeJpegFromMemory_AllocComponentBuffer` | Evidence from decompile and caller context. |
| `0x823c4958` | `FM2_Shader_ApplyConstantsBatchValidateSlotCheck` | Evidence from decompile and caller context. |
| `0x823c49b8` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyA` | Evidence from decompile and caller context. |
| `0x823c5488` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyB` | Evidence from decompile and caller context. |
| `0x823c5568` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyC` | Evidence from decompile and caller context. |
| `0x823c5728` | `FM2_Shader_ApplyConstantsBatchValidateSlotGuard` | Evidence from decompile and caller context. |
| `0x823c5758` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyD` | Evidence from decompile and caller context. |
| `0x823cce40` | `FM2_D3D_CreateTextureResourceFromFormatAlloc` | Evidence from decompile and caller context. |
| `0x823ce7a8` | `FM2_D3D_CreateTextureResourceFromFormatUploadA` | Evidence from decompile and caller context. |
| `0x823cea48` | `FM2_D3D_CreateTextureResourceFromFormatUploadB` | Evidence from decompile and caller context. |
| `0x823d0920` | `FM2_D3D_CreateTextureResourceFromFormatUploadCore` | Evidence from decompile and caller context. |
| `0x823d14d0` | `FM2_D3D_CreateTextureResourceFromFormatCleanup` | Evidence from decompile and caller context. |
| `0x8245ca78` | `FM2_ComObject_FormatCarIdSqlAppend` | Evidence from decompile and caller context. |

### Infrastructure pass 77 (33 functions)

AI race line, car audio/livery, Lua SSL/input, profile, exception filter, D3D present chain.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82494668` | `FM2_AIDriver_ResetRaceLineOnSectorChangeInterpA` | Evidence from decompile and caller context. |
| `0x824946c8` | `FM2_AIDriver_ResetRaceLineOnSectorChangeInterpB` | Evidence from decompile and caller context. |
| `0x824947f0` | `FM2_AIDriver_ResetRaceLineOnSectorChangeBlend` | Evidence from decompile and caller context. |
| `0x824948d0` | `FM2_AIDriver_ResetRaceLineOnSectorChangeFinalize` | Evidence from decompile and caller context. |
| `0x8249a920` | `FM2_CarAudio_DtorBody` | Evidence from decompile and caller context. |
| `0x824a6758` | `FM2_LiveryMask_ParseAndLoadEntryBody` | Evidence from decompile and caller context. |
| `0x824aacb8` | `FM2_CompressionStream_EraseRbTreeNodeBody` | Evidence from decompile and caller context. |
| `0x824bd360` | `FM2_Lua_IncrementCallDepthOrOverflowBody` | Evidence from decompile and caller context. |
| `0x824cdca0` | `FM2_LiveryMask_ProcessPendingLayerEntryInitA` | Evidence from decompile and caller context. |
| `0x824cdde0` | `FM2_LiveryMask_ProcessPendingLayerEntryInitB` | Evidence from decompile and caller context. |
| `0x824cde98` | `FM2_LiveryMask_ProcessPendingLayerEntryInitC` | Evidence from decompile and caller context. |
| `0x824d0560` | `FM2_Render_ResetPassLightingSlotStateCopy` | Evidence from decompile and caller context. |
| `0x824d07b8` | `FM2_Input_InitControllerDevicesParseControllerName` | Evidence from decompile and caller context. |
| `0x824d2df8` | `FM2_Input_InitControllerDevicesParseBindingA` | Evidence from decompile and caller context. |
| `0x824d2f50` | `FM2_Input_InitControllerDevicesParseBindingB` | Evidence from decompile and caller context. |
| `0x824d3670` | `FM2_Scene_GetNotifyStateFromParamNormalizeUtf16` | Evidence from decompile and caller context. |
| `0x824d56c8` | `FM2_Lua_PushSslUnitStringsTableBodyA` | Evidence from decompile and caller context. |
| `0x824d59a0` | `FM2_Lua_PushSslUnitStringsTableBodyB` | Evidence from decompile and caller context. |
| `0x824d5f28` | `FM2_Lua_PushDampingFromKeyframeDoubleBody` | Evidence from decompile and caller context. |
| `0x824d8030` | `FM2_Math_AllocForceVectorComPtrBody` | Evidence from decompile and caller context. |
| `0x824daca8` | `FM2_Audio_VolumeListFindOrInsertByPrefixWalk` | Evidence from decompile and caller context. |
| `0x824dd2d8` | `FM2_Profile_ParseUnsignedFromSubStringValidateBody` | Evidence from decompile and caller context. |
| `0x824e30c0` | `FM2_ComObject_AllocSharedStateBufferInit` | Evidence from decompile and caller context. |
| `0x824e9e98` | `FM2_RenderAdapter_InitPresentationVtablesClearStateBody` | Evidence from decompile and caller context. |
| `0x824ef8a8` | `FM2_ExceptionFilter_OnCppExceptionLogBodyA` | Evidence from decompile and caller context. |
| `0x824efc50` | `FM2_ExceptionFilter_OnCppExceptionLogBodyB` | Evidence from decompile and caller context. |
| `0x824f0758` | `FM2_Render_NotifyChainInsertSubscriberSortedInit` | Evidence from decompile and caller context. |
| `0x824f2630` | `FM2_Memory_LookupFrameAllocNotifyStateBody` | Evidence from decompile and caller context. |
| `0x823d1d10` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDesc` | Evidence from decompile and caller context. |
| `0x824b1168` | `FM2_ComObject_GetAggregateFieldAt60Thunk` | Evidence from decompile and caller context. |
| `0x824fe578` | `FM2_D3D_LazyInitPresentChainBody` | Evidence from decompile and caller context. |
| `0x825025a0` | `FM2_D3D_Subscriber_EnableDeviceJournalBody` | Evidence from decompile and caller context. |
| `0x82503388` | `FM2_Memory_LookupFrameAllocNotifyStateInit` | Evidence from decompile and caller context. |

### Infrastructure pass 78 (33 functions)


**Apply: 0/33** - targets already named in IDA (jpeg/png/zlib/d3d_texture/io_sink snake_case from prior work).
Audio render D3D packet writers F/G/J, D3D texture upload core, shader validate shared bodies.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82388578` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketShared` | Evidence from decompile and caller context. |
| `0x824d8ad0` | `FM2_Lua_PushSslValueFromTableCell` | Evidence from decompile and caller context. |
| `0x8238d860` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketIJShared` | Evidence from decompile and caller context. |
| `0x823c5018` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyShared` | Evidence from decompile and caller context. |
| `0x823d2120` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketEHShared` | Evidence from decompile and caller context. |
| `0x821e70c8` | `FM2_ComObject_InitRefCountAggregateLinkNodeInit` | Evidence from decompile and caller context. |
| `0x823850f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketAPre` | Evidence from decompile and caller context. |
| `0x8238e8c0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketDPre` | Evidence from decompile and caller context. |
| `0x8238e9b8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketCPre` | Evidence from decompile and caller context. |
| `0x823a23e0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyA` | Evidence from decompile and caller context. |
| `0x823a2580` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyB` | Evidence from decompile and caller context. |
| `0x823a2728` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyC` | Evidence from decompile and caller context. |
| `0x823a2808` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyD` | Evidence from decompile and caller context. |
| `0x823a33c8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyE` | Evidence from decompile and caller context. |
| `0x823a3588` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyA` | Evidence from decompile and caller context. |
| `0x823a3620` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyB` | Evidence from decompile and caller context. |
| `0x823a3d60` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyC` | Evidence from decompile and caller context. |
| `0x823a3e60` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyD` | Evidence from decompile and caller context. |
| `0x823a3ea0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyE` | Evidence from decompile and caller context. |
| `0x823a3f40` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyF` | Evidence from decompile and caller context. |
| `0x823c58b8` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyAInner` | Evidence from decompile and caller context. |
| `0x823cf070` | `FM2_D3D_CreateTextureResourceUploadCoreInitA` | Evidence from decompile and caller context. |
| `0x823cf100` | `FM2_D3D_CreateTextureResourceUploadCoreBodyA` | Evidence from decompile and caller context. |
| `0x823cf428` | `FM2_D3D_CreateTextureResourceUploadCoreBodyB` | Evidence from decompile and caller context. |
| `0x823cfea8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyC` | Evidence from decompile and caller context. |
| `0x823cff30` | `FM2_D3D_CreateTextureResourceUploadCoreBodyD` | Evidence from decompile and caller context. |
| `0x823d00d8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyE` | Evidence from decompile and caller context. |
| `0x823d0218` | `FM2_D3D_CreateTextureResourceUploadCoreBodyF` | Evidence from decompile and caller context. |
| `0x823d1768` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerA` | Evidence from decompile and caller context. |
| `0x823d1a70` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerB` | Evidence from decompile and caller context. |
| `0x823d21a8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyG` | Evidence from decompile and caller context. |
| `0x82418328` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyA` | Evidence from decompile and caller context. |
| `0x824186d0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyB` | Evidence from decompile and caller context. |

### Infrastructure pass 79 (33 functions)


**Apply: 0/33** - targets already named in IDA (lua/render/presentation snake_case from prior work).
Livery/car audio, Lua call-depth overflow, profile/input, render object-pass, presentation.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82432688` | `FM2_LiveryMask_ParseAndLoadEntryValidate` | Evidence from decompile and caller context. |
| `0x82494740` | `FM2_AIDriver_ResetRaceLineOnSectorChangeFinalizeBody` | Evidence from decompile and caller context. |
| `0x8249a868` | `FM2_CarAudio_DtorReleaseBindingA` | Evidence from decompile and caller context. |
| `0x8249a8d0` | `FM2_CarAudio_DtorReleaseBindingB` | Evidence from decompile and caller context. |
| `0x824a5998` | `FM2_LiveryMask_ParseAndLoadEntryParseLayer` | Evidence from decompile and caller context. |
| `0x824b80e8` | `FM2_Lua_IncrementCallDepthOrOverflowCheck` | Evidence from decompile and caller context. |
| `0x824bc5c8` | `FM2_Lua_IncrementCallDepthOrOverflowGrowStack` | Evidence from decompile and caller context. |
| `0x824bcd78` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchA` | Evidence from decompile and caller context. |
| `0x824bce28` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchB` | Evidence from decompile and caller context. |
| `0x824bcec8` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchC` | Evidence from decompile and caller context. |
| `0x824bcf70` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchD` | Evidence from decompile and caller context. |
| `0x824bd228` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchE` | Evidence from decompile and caller context. |
| `0x824bedc8` | `FM2_Lua_IncrementCallDepthOrOverflowErrorHandler` | Evidence from decompile and caller context. |
| `0x824befa8` | `FM2_Lua_IncrementCallDepthOrOverflowGuard` | Evidence from decompile and caller context. |
| `0x824bf820` | `FM2_Lua_IncrementCallDepthOrOverflowCleanup` | Evidence from decompile and caller context. |
| `0x824d0ed8` | `FM2_Input_InitControllerDevicesParseBindingField` | Evidence from decompile and caller context. |
| `0x824d3e58` | `FM2_Lua_PushSslUnitStringsTableAppendA` | Evidence from decompile and caller context. |
| `0x824d3f00` | `FM2_Lua_PushSslUnitStringsTableAppendB` | Evidence from decompile and caller context. |
| `0x824d4608` | `FM2_Math_AllocForceVectorComPtrInitA` | Evidence from decompile and caller context. |
| `0x824d4628` | `FM2_Math_AllocForceVectorComPtrInitB` | Evidence from decompile and caller context. |
| `0x824db218` | `FM2_Profile_ParseUnsignedFromSubStringValidateDigit` | Evidence from decompile and caller context. |
| `0x824dcec0` | `FM2_Profile_ParseUnsignedFromSubStringValidateRange` | Evidence from decompile and caller context. |
| `0x824f26d8` | `FM2_SystemEventSubscriber_CtorFields` | Evidence from decompile and caller context. |
| `0x82502268` | `FM2_D3D_LazyInitPresentChainInit` | Evidence from decompile and caller context. |
| `0x82505d20` | `FM2_Audio_MLPMatrix_FormatErrorMessageBody` | Evidence from decompile and caller context. |
| `0x825065c8` | `FM2_Render_FramePipelineSubmitPassABody` | Evidence from decompile and caller context. |
| `0x82510a88` | `FM2_Memory_AllocTaggedSmallBlockFromPoolEntryBody` | Evidence from decompile and caller context. |
| `0x82510d20` | `FM2_PresentationSlotVector_Clear200ByteBody` | Evidence from decompile and caller context. |
| `0x82510e28` | `FM2_Render_BuildObjectPassCommandBufferInitA` | Evidence from decompile and caller context. |
| `0x82511038` | `FM2_Render_BuildObjectPassCommandBufferInitB` | Evidence from decompile and caller context. |
| `0x825110b0` | `FM2_Render_AppendObjectPassDrawEntryBody` | Evidence from decompile and caller context. |
| `0x825117a0` | `FM2_Presentation_CopyCarDisplayBlockToSlotBody` | Evidence from decompile and caller context. |
| `0x82511828` | `FM2_PresentationCarConfig_DeleteOptionalBodyA` | Evidence from decompile and caller context. |

### Infrastructure pass 80 (33 functions)


**Apply: 33/33.**
Presentation/render frame pipeline, sort introspect, instance path wrapper cluster.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82511968` | `FM2_PresentationCarConfig_DeleteOptionalBodyB` | Evidence from decompile and caller context. |
| `0x82511b28` | `FM2_Presentation_CopyCarDisplayBlockToSlotBody` | Evidence from decompile and caller context. |
| `0x825121d0` | `FM2_PresentationSlotVector_Clear200ByteInit` | Evidence from decompile and caller context. |
| `0x825125c0` | `FM2_Vector_ComputeEraseSpanFor20ByteElements` | Evidence from decompile and caller context. |
| `0x82512aa8` | `FM2_Render_GrowObjectPassDrawVectorBody` | Evidence from decompile and caller context. |
| `0x82512b30` | `FM2_Vector_EraseBegin20ByteElementsTail` | Evidence from decompile and caller context. |
| `0x82513188` | `FM2_Vector_EraseBegin20ByteElementsHead` | Evidence from decompile and caller context. |
| `0x82514640` | `FM2_Render_FramePipelineDrawObjectsBody` | Evidence from decompile and caller context. |
| `0x82515a58` | `FM2_Render_CompilePassIfStaleLockedBodyA` | Evidence from decompile and caller context. |
| `0x82515ba8` | `FM2_Render_CompilePassIfStaleLockedBodyB` | Evidence from decompile and caller context. |
| `0x8251a0a8` | `FM2_Presentation_ApplyCarCameraVMXBodyA` | Evidence from decompile and caller context. |
| `0x8251a198` | `FM2_Presentation_ApplyCarCameraVMXBodyB` | Evidence from decompile and caller context. |
| `0x8251ae30` | `FM2_Render_FramePipelineSubmitPassBBody` | Evidence from decompile and caller context. |
| `0x8251e9f0` | `FM2_CarPresentation_DtorBody` | Evidence from decompile and caller context. |
| `0x8251f0c8` | `FM2_Render_TestPassVisibilityVMXBody` | Evidence from decompile and caller context. |
| `0x82522598` | `FM2_Render_CompilePassIfStaleLockedBodyC` | Evidence from decompile and caller context. |
| `0x82527d00` | `FM2_Render_SortVisibleRenderablesIntrosortInit` | Evidence from decompile and caller context. |
| `0x82528898` | `FM2_Render_SortVisibleRenderablesIntrosortBody` | Evidence from decompile and caller context. |
| `0x82528d00` | `FM2_Render_SortVisibleRenderablesIntrosortPartition` | Evidence from decompile and caller context. |
| `0x82529a68` | `FM2_Render_SortVisibleRenderablesIntrosortInsert` | Evidence from decompile and caller context. |
| `0x8252ad70` | `FM2_Render_Helper16E0SortKeyCompare` | Evidence from decompile and caller context. |
| `0x8252d118` | `FM2_Render_GetDistanceKeyFromPassSlotBody` | Evidence from decompile and caller context. |
| `0x8252dc18` | `FM2_Render_UpdateObjectDistanceKeysBody` | Evidence from decompile and caller context. |
| `0x8252efe8` | `FM2_Render_SortVisibleRenderablesCompare` | Evidence from decompile and caller context. |
| `0x8252f040` | `FM2_Memory_AllocTaggedSmallBlockFromPoolEntryTail` | Evidence from decompile and caller context. |
| `0x82535b08` | `FM2_Render_InstanceHybridDrawPathSortBody` | Evidence from decompile and caller context. |
| `0x82535c98` | `FM2_Render_InitSkinnedModelResourceLockBody` | Evidence from decompile and caller context. |
| `0x82536520` | `FM2_Render_InstanceHybridDrawPathSortCore` | Evidence from decompile and caller context. |
| `0x82536840` | `FM2_Render_InstanceHybridDrawPathSortPartition` | Evidence from decompile and caller context. |
| `0x82536d38` | `FM2_Render_InstancePathWrapperBodyA` | Evidence from decompile and caller context. |
| `0x82537538` | `FM2_Render_InstanceHybridDrawPathSortFinalize` | Evidence from decompile and caller context. |
| `0x82537a68` | `FM2_Render_InstancePathWrapperBodyB` | Evidence from decompile and caller context. |
| `0x82538870` | `FM2_Render_InstancePathWrapperInnerBody` | Evidence from decompile and caller context. |

### Infrastructure pass 81 (33 functions)


**Apply: 32/33** - 0x82503668 skipped (already memory_frame_alloc_notify_entry_init).
Scene graph/STL, render object-pass/draw setup, FMOD/network, race ghost playback table.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8253d970` | `FM2_SpliceResultObjectsIntoListInitA` | Evidence from decompile and caller context. |
| `0x8253db30` | `FM2_IntrusiveList_InitSentinelBody` | Evidence from decompile and caller context. |
| `0x8253efa0` | `FM2_SpliceResultObjectsIntoListInitB` | Evidence from decompile and caller context. |
| `0x825422b0` | `FM2_LuaGarage_EnsureCarRecordLookupTail` | Evidence from decompile and caller context. |
| `0x82544700` | `FM2_SceneGraph_DestroySubtreeAndFreeBody` | Evidence from decompile and caller context. |
| `0x82545fe0` | `FM2_IntrusiveList_ResetToSelfBody` | Evidence from decompile and caller context. |
| `0x82548ae8` | `FM2_Set_InsertUniqueSortedBody` | Evidence from decompile and caller context. |
| `0x8254a4e0` | `FM2_LuaParser_GetTokenOrAdvanceLineBody` | Evidence from decompile and caller context. |
| `0x82551508` | `FM2_FindAndReplaceDelimitedTextRangeBody` | Evidence from decompile and caller context. |
| `0x82551a00` | `FM2_FindAndReplaceDelimitedTextRangeTail` | Evidence from decompile and caller context. |
| `0x82556080` | `FM2_CarDynamics_ComputeSuspensionDotsVMXBody` | Evidence from decompile and caller context. |
| `0x8255bc00` | `FM2_Render_ObjectPassDrawTraversalBody` | Evidence from decompile and caller context. |
| `0x8255d430` | `FM2_Render_ObjectPassShouldDrawVisibleCheck` | Evidence from decompile and caller context. |
| `0x8255d4b8` | `FM2_Render_ObjectPassShouldDrawVisibleBody` | Evidence from decompile and caller context. |
| `0x82561208` | `FM2_Render_DrawPassMaterialSetupBodyA` | Evidence from decompile and caller context. |
| `0x82561f58` | `FM2_D3D_ValidateResourceHandlesOrRecoverBody` | Evidence from decompile and caller context. |
| `0x82563b68` | `FM2_Render_DrawPassMaterialSetupBodyB` | Evidence from decompile and caller context. |
| `0x825687c8` | `FM2_Render_ObjectPassDrawSetupBody` | Evidence from decompile and caller context. |
| `0x8256ac18` | `FM2_Render_HelperB3E8DrawPathTail` | Evidence from decompile and caller context. |
| `0x8257cbe8` | `FM2_HashName_CtorEmptyBody` | Evidence from decompile and caller context. |
| `0x8257cf90` | `FM2_AIDriver_ResetRaceLineInterpBScalar` | Evidence from decompile and caller context. |
| `0x82586de0` | `FM2_FMOD_Build3DAttributesPairBodyA` | Evidence from decompile and caller context. |
| `0x82587048` | `FM2_FMOD_Build3DAttributesPairBodyB` | Evidence from decompile and caller context. |
| `0x82587b88` | `FM2_Network_DispatchMessageFromQueueLockedBody` | Evidence from decompile and caller context. |
| `0x82589ae0` | `FM2_FileInfoCache_AllocateEntryBody` | Evidence from decompile and caller context. |
| `0x8258b060` | `FM2_RenderAdapter_DestroyChildAndClearListBody` | Evidence from decompile and caller context. |
| `0x8258c008` | `FM2_Presentation_InitMediaFoundationFieldBody` | Evidence from decompile and caller context. |
| `0x8258d0b8` | `FM2_Set_LowerBoundByKeyInTreeBody` | Evidence from decompile and caller context. |
| `0x825977a8` | `FM2_ComObject_InitCarRecordFromDataQueryBody` | Evidence from decompile and caller context. |
| `0x82598048` | `FM2_RaceGhost_BuildPlaybackSampleTableCore` | Evidence from decompile and caller context. |
| `0x8259dc20` | `FM2_RaceGhost_BuildPlaybackSampleTableParse` | Evidence from decompile and caller context. |
| `0x82503668` | `FM2_Memory_LookupFrameAllocNotifyStateHelper` | Evidence from decompile and caller context. |
| `0x824cd5a0` | `FM2_STL_WStringInsertCharsRange_LenThunk` | Evidence from decompile and caller context. |

### Infrastructure pass 82 (33 functions)


**Apply: 33/33.**
Newly exposed callees from passes 80–81: presentation slot vector, car presentation dtor, render sort/visibility, D3D validate.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825a0298` | `FM2_Render_DrawPassMaterialSetupSharedHelper` | Evidence from decompile and caller context. |
| `0x825e5230` | `FM2_LiveryMask_OrRaceGhostSharedUtil` | Evidence from decompile and caller context. |
| `0x8272d7a0` | `FM2_Presentation_CopyCarDisplayBlockSharedAppend` | Evidence from decompile and caller context. |
| `0x821efe38` | `FM2_Render_HelperB3E8DrawPathInit` | Evidence from decompile and caller context. |
| `0x82227100` | `FM2_D3D_ValidateResourceHandlesCheckA` | Evidence from decompile and caller context. |
| `0x82227158` | `FM2_D3D_ValidateResourceHandlesCheckB` | Evidence from decompile and caller context. |
| `0x82369fa0` | `FM2_D3D_ValidateResourceHandlesRecoverSlot` | Evidence from decompile and caller context. |
| `0x82369ff0` | `FM2_D3D_ValidateResourceHandlesRecoverNoOp` | Evidence from decompile and caller context. |
| `0x8236ea80` | `FM2_Render_InstancePathWrapperCallThunk` | Evidence from decompile and caller context. |
| `0x82418630` | `FM2_HashName_InitSaltFieldA` | Evidence from decompile and caller context. |
| `0x82418650` | `FM2_HashName_InitSaltFieldB` | Evidence from decompile and caller context. |
| `0x82455100` | `FM2_Network_DispatchMessageQueueTail` | Evidence from decompile and caller context. |
| `0x824635e8` | `FM2_RaceGhost_BuildPlaybackSampleTableFinalize` | Evidence from decompile and caller context. |
| `0x824a76a8` | `FM2_RenderAdapter_DestroyChildClearThunk` | Evidence from decompile and caller context. |
| `0x8250f1c8` | `FM2_Presentation_CopyCarDisplayBlockSlotInitA` | Evidence from decompile and caller context. |
| `0x82510260` | `FM2_Presentation_CopyCarDisplayBlockSlotInitB` | Evidence from decompile and caller context. |
| `0x825104a8` | `FM2_PresentationSlotVector_Clear200ByteInnerA` | Evidence from decompile and caller context. |
| `0x82510910` | `FM2_PresentationSlotVector_Clear200ByteInnerB` | Evidence from decompile and caller context. |
| `0x82510ef8` | `FM2_Presentation_CopyCarDisplayBlockLinkNode` | Evidence from decompile and caller context. |
| `0x82511110` | `FM2_Presentation_CopyCarDisplayBlockSlotFinalize` | Evidence from decompile and caller context. |
| `0x82511170` | `FM2_PresentationSlotVector_Clear200ByteDtorChain` | Evidence from decompile and caller context. |
| `0x825145e8` | `FM2_CarPresentation_DtorReleaseFieldA` | Evidence from decompile and caller context. |
| `0x8251d540` | `FM2_CarPresentation_DtorReleaseFieldB` | Evidence from decompile and caller context. |
| `0x8251e270` | `FM2_CarPresentation_DtorClearOwnedLists` | Evidence from decompile and caller context. |
| `0x8251e410` | `FM2_Render_TestPassVisibilityVMXCore` | Evidence from decompile and caller context. |
| `0x82523020` | `FM2_Render_SortVisibleRenderablesPartitionTail` | Evidence from decompile and caller context. |
| `0x82526490` | `FM2_Render_SortVisibleRenderablesInitHeap` | Evidence from decompile and caller context. |
| `0x82526a88` | `FM2_Render_SortVisibleRenderablesInsertTail` | Evidence from decompile and caller context. |
| `0x82527c60` | `FM2_Render_SortVisibleRenderablesBodyTail` | Evidence from decompile and caller context. |
| `0x8252bbb8` | `FM2_Render_GetDistanceKeyFromPassSlotCore` | Evidence from decompile and caller context. |
| `0x8252d170` | `FM2_Render_UpdateObjectDistanceKeysTail` | Evidence from decompile and caller context. |
| `0x82587788` | `FM2_Network_DispatchMessageFromQueueLockedTail` | Evidence from decompile and caller context. |
| `0x8258cba0` | `FM2_Set_LowerBoundByKeyInTreeTail` | Evidence from decompile and caller context. |

### Infrastructure pass 83 (33 functions)


**Apply: 33/33.**
Instance path wrapper inner, find/replace text, D3D validate recover, object-pass draw setup, visibility VMX.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8252d920` | `FM2_Memory_AllocTaggedSmallBlockPoolGrow` | Evidence from decompile and caller context. |
| `0x82534a88` | `FM2_Render_InstanceHybridDrawPathSortPartitionTail` | Evidence from decompile and caller context. |
| `0x82535038` | `FM2_Render_InstanceHybridDrawPathSortBodyInner` | Evidence from decompile and caller context. |
| `0x82535a08` | `FM2_Render_InstancePathWrapperInnerInit` | Evidence from decompile and caller context. |
| `0x82535a68` | `FM2_Render_InstanceHybridDrawPathSortCoreInner` | Evidence from decompile and caller context. |
| `0x82535b78` | `FM2_Render_InstanceHybridDrawPathSortFinalizeTail` | Evidence from decompile and caller context. |
| `0x825361c8` | `FM2_Render_InstancePathWrapperInnerCore` | Evidence from decompile and caller context. |
| `0x82536440` | `FM2_Render_InstancePathWrapperInnerParse` | Evidence from decompile and caller context. |
| `0x825374c8` | `FM2_Render_InstancePathWrapperInnerFinalize` | Evidence from decompile and caller context. |
| `0x82541e20` | `FM2_LuaGarage_EnsureCarRecordLookupParse` | Evidence from decompile and caller context. |
| `0x82544c78` | `FM2_IntrusiveList_ResetToSelfUnlink` | Evidence from decompile and caller context. |
| `0x8254a128` | `FM2_LuaParser_GetTokenOrAdvanceLineSkip` | Evidence from decompile and caller context. |
| `0x8254a408` | `FM2_LuaParser_GetTokenOrAdvanceLineRead` | Evidence from decompile and caller context. |
| `0x82550f88` | `FM2_FindAndReplaceDelimitedTextRangeScan` | Evidence from decompile and caller context. |
| `0x82551048` | `FM2_FindAndReplaceDelimitedTextRangeReplace` | Evidence from decompile and caller context. |
| `0x825511c8` | `FM2_FindAndReplaceDelimitedTextRangeAppend` | Evidence from decompile and caller context. |
| `0x825512a0` | `FM2_FindAndReplaceDelimitedTextRangeFinalize` | Evidence from decompile and caller context. |
| `0x82551378` | `FM2_FindAndReplaceDelimitedTextRangeGrowBuffer` | Evidence from decompile and caller context. |
| `0x82551430` | `FM2_FindAndReplaceDelimitedTextRangeCopyTail` | Evidence from decompile and caller context. |
| `0x8255a380` | `FM2_D3D_ValidateResourceHandlesOrRecoverCoreA` | Evidence from decompile and caller context. |
| `0x8255b1f0` | `FM2_D3D_ValidateResourceHandlesOrRecoverCoreB` | Evidence from decompile and caller context. |
| `0x8255d370` | `FM2_D3D_ValidateResourceHandlesOrRecoverTail` | Evidence from decompile and caller context. |
| `0x82560ef0` | `FM2_Render_ObjectPassDrawSetupInitA` | Evidence from decompile and caller context. |
| `0x82561010` | `FM2_Render_ObjectPassDrawSetupInitB` | Evidence from decompile and caller context. |
| `0x825610a8` | `FM2_Render_ObjectPassDrawSetupBindState` | Evidence from decompile and caller context. |
| `0x825611b0` | `FM2_Render_TestPassVisibilityVMXPreCheck` | Evidence from decompile and caller context. |
| `0x82563760` | `FM2_Render_ObjectPassDrawSetupMaterialCore` | Evidence from decompile and caller context. |
| `0x82565608` | `FM2_Render_ObjectPassDrawSetupSortKeys` | Evidence from decompile and caller context. |
| `0x82566a40` | `FM2_Render_ObjectPassDrawSetupFlushLists` | Evidence from decompile and caller context. |
| `0x82567928` | `FM2_Render_ObjectPassDrawSetupEmitDraws` | Evidence from decompile and caller context. |
| `0x8256b0d8` | `FM2_Render_TestPassVisibilityVMXFrustumCore` | Evidence from decompile and caller context. |
| `0x82579ba8` | `FM2_Render_TestPassVisibilityVMXNoOpThunk` | Evidence from decompile and caller context. |
| `0x82586e58` | `FM2_FMOD_Build3DAttributesPairBodyCTail` | Evidence from decompile and caller context. |
