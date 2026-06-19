# FM2 IDA Renames — 2026-06-18

Session log for unnamed `sub_` functions renamed in IDA (`default.xex.i64`) by
walking outward from already-named `FM2_` functions and cross-checking repo docs
(`docs/FM2-ida-toml-function-notes.md`, `docs/FM2-native-renderer-generator-notes.md`,
`docs/FM2-performance-notes.md`, `docs/FM2-audio-fmod-decode-cadence.md`).

Method: enumerate `sub_` callees of named `FM2_` functions, decompile the
highest-traffic clusters (render/D3D, allocator, audio, STL/EH), name from
behavior and caller context.

**From batch 2 onward, each entry includes explicit rename reasoning.**

External references: repo docs plus `D:\Emulation\Xbox360techdocs` (Xbox 360
system PDFs — notably `system_xbox_360_memory_copy_functions.pdf` and
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
names. **978/978** batch-4 placeholder functions re-named (manual passes 1–27). Post batch-4:
high-traffic `sub_` infrastructure naming in progress (**570** renamed: passes 1–18; **1010** remaining `sub_` from named `FM2_` callers).

**Render emit cluster BFS (7 roots → 317 functions) is fully exhausted:** 0 unnamed
`sub_` remain in the transitive closure of
`FM2_Render_EmitPassDrawWork`, `FM2_D3D_EmitDirtyStateAndDrawList`,
`FM2_D3D_EmitDrawListStatePackets`, `FM2_D3D_EmitScissorRegionPackets`,
`FM2_D3D_EmitSurfaceResolvePackets`, `FM2_D3D_BeginCommandBufferBatch`, and
`FM2_D3D_FinalizeCommandBufferBatch`.

~106 unnamed `sub_` callees of `FM2_` functions remain globally outside the
emit cluster (**~1201** total `sub_` callees from any `FM2_` function; prioritize
by caller count — see `.cursor/hooks/state/unnamed-sub-callees.json`). **~44,696** unnamed `sub_` remain in the binary overall after
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
| `0x8236D958` | `FM2_RenderContext_UploadMatrixConstants` | Uploads 4×4 matrix block + related constants; used by instance/UI draw paths. |
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

- `0x82537A68` / `0x82539398` / `0x82536D38` — instance path camera/constants setup around `FM2_Render_InstancePathWrapper`
- `0x825B0348` — very large function on audio render path B
- `0x8267FA90` — callee of `FM2_AudioVoiceApplyOutputMatrix_826A8628`
- `0x8237D158` / `0x827328E8` — constant-buffer upload helpers on audio mix path
- Remaining `0x8236ECxx` / `0x8236EDxx` D3D dirty-state emit helpers inside `FM2_Render_EmitPassDrawWork`
- `0x8255D880` — large object-pass constant submit helper

Consider mirroring stable gameplay-facing names into `FM2/fm2_manifest.toml` when codegen hooks or crash logs need them.

---

## Batch 2 — 2026-06-18 (with reasoning)

### Render / D3D / command buffer

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825AD688` | `FM2_Render_CopyPassConstantBlock` | Called from scoped-batch and pass-compile paths; indexes global pass table at `unk_829A2BB0` with stride `1232` and copies `16 * count` bytes via `FM2_MemcpyAligned`. Name reflects pass-constant block copy, not generic memcpy. |
| `0x825AD650` | `FM2_Render_GetPassConstantBaseOffset` | Returns `unk_829A2BB0[308 * passIndex]` — base dword offset into the pass-constant table. Paired with slot-index accessor below. |
| `0x825AD628` | `FM2_Render_GetPassConstantSlotIndex` | Returns adjacent table entry `unk_829A2BA8[308 * passIndex]`; used with base offset to locate pass constant storage. |
| `0x8236F1C0` | `FM2_RenderContext_SetZEnableBit` | Same dirty-mask pattern as other `FM2_RenderContext_Set*` helpers: writes `(4*a2)&4` into `ctx+10420`, sets dirty bit `0x800`. Bit 2 corresponds to Z-enable in the packed render-state dword. |
| `0x8236F1F0` | `FM2_RenderContext_SetAlphaBlendEnableBits` | Sets `(16*a2)&0x70` in `ctx+10420`; marks dirty `0x800` and `0x20000`. Called alongside other blend/alpha helpers inside `FM2_Render_EmitPassDrawWork`. |
| `0x82370080` | `FM2_RenderContext_SetVertexFetchModeBit` | Modifies `ctx+10428` nibble at bit 4 (`(16*a2)&0x10`); dirty bit `0x200`. Distinct field from `10420` state dword used by raster/blend helpers. |
| `0x8236FC88` | `FM2_RenderContext_SetBoundSurfaceEDRAMMode` | Reads bound surface at `ctx+12160`, rewrites tiled/EDRAM-related bits in surface field `+28` based on format nibble and `a2`. Much more complex than a simple dirty-bit setter — name captures surface/EDRAM mode update. |
| `0x8236EAC0` | `FM2_RenderContext_SetIndexBufferModeBit` | Sets bit 3 in `ctx+10428` (`(8*a2)&8`); dirty bits `0x200` and `0x40000`. |
| `0x8236E228` | `FM2_RenderContext_SetActivePassId` | Stores pass id at `ctx+11540`; dirty bit `0x80000`. Used by indexed/instance draw paths before constant upload. |
| `0x82721190` | `FM2_Render_SetGlobalFillMode` | Maps caller values `0→0`, `100→2`, `101→6` into global `dword_82A4198C`; other values stored verbatim. Called from pass draw emit when `a6` requests alternate fill mode. |
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
| `0x82273338` | `FM2_AudioMix_InitDefaultCoefficients` | Initializes default float coefficients/matrix blocks at object base using VMX stores; called during `FM2_AudioManager_InitAndBindSignalGate`. No external inputs — pure init table write. |

### STL / list / gameplay

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827A7D10` | `FM2_STL_ConstructElement4176` | Thin wrapper to `sub_827A8070`; called from both `FM2_STL_ConstructArray4176` and `CopyConstructRange4176`. Matches existing `FM2_STL_ConstructArray4176` naming pattern. |
| `0x82766EC0` | `FM2_STL_GetNodeDataPtr16` | Returns `a1+16`; used in list-node bundle init sequences after sentinel setup. |
| `0x82766ED0` | `FM2_STL_GetNodeDataPtr17` | Returns `a1+17`; adjacent node accessor in same init bundle cluster — likely char-sized link flag field. |
| `0x8224FF00` | `FM2_LiveryMask_ProcessPendingEntryUpdates` | Sole caller context is list-entry notification path, but **internal strings** name the domain: `LiveryMasks\\Masks.xml`, `-BaseUncompTemp`, `-DamageUncompTemp`, `(Base)`, `(Damage)`. Renamed away from generic "ListEntryManager" after string evidence. |

### GPU kick / perf

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82378AB0` | `FM2_GpuCommandBuffer_BeginPerfCaptureOrKick` | Called from `FM2_GpuCommandBuffer_BuildAndSubmit`; handles perf capture file `e:\xbperfview.cap`, `mftb` timing fields, allocates `D3D::P_CPC` slots, transitions kick state machine at `ctx+21272`. Name reflects perf-capture + kick setup, not generic submit. |

---

## Batch 3 — Render emit cluster (167 renames, 2026-06-18)

BFS from emit roots listed in Summary. Every remaining `sub_` in the 317-function
closure was renamed (including CRT/STL helpers reached only through emit paths).

### Render context state setters (sampler / depth / texture fetch)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236EAF8` | `FM2_RenderContext_SetDepthStencilEnableState` | Sets MSB of `ctx+11572`, recomputes packed sampler dword at `ctx+11568`, mirrors to `ctx+10424/10456/10460/10464`, sets dirty qword bits — same pattern as named `FM2_RenderContext_Set*State` helpers. |
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
| `0x82724BF8` | `FM2_Math_CopyMatrix4x4` | Copies 16 floats (4×4) with column-major layout. |
| `0x82724C88` | `FM2_Math_SetIdentityMatrix4x4` | Writes identity to 4×4 float matrix. |
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

### CRT runtime (emit closure — Microsoft CRT, prefixed for IDA clarity)

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

### STL / EH (emit closure — reached via hang dump strings)

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

## Batch 4 — Random sample (1000 renames, 2026-06-18)

**Method:** `random.seed(20260618)` → `random.sample(sub_*, 1000)` from **45,689**
unnamed functions. Each function was decompiled and renamed via heuristics in
`scripts/ida_fm2_random_rename_batch.py` (IDA MCP `py_exec_file`).

**Result:** 1000/1000 renamed successfully (0 skipped, 0 failed).

### Naming heuristics (reasoning by category)

| Pattern | Example name | Reasoning |
| --- | --- | --- |
| Decompile references `std::` | `FM2_Stl_StringHelper` | MSVC STL method body detected in pseudocode. |
| Decompile references `D3D::` / `D3D_` | `FM2_D3D_ShaderHelper` | Xbox D3D runtime helper reached from game code. |
| Calls existing `FM2_*` symbol | `FM2_Render_NotifyManagerStateChange_Caller` | Named for dominant already-named callee in decompile. |
| Size ≤ 8 bytes | `FM2_Thunk` / `FM2_JumpTail` | Branch/tail stub, not a semantic function body. |
| String literal in decompile | `FM2_Str_<slug>` | Domain hint from embedded string (when ≥4 chars). |
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
and later refinement — unlike batches 1–3 which used manual evidence-based
naming. Re-run targeted manual passes on hot paths as needed.

---

## Batch 4 — Manual re-pass (replacing heuristic placeholders)

The automated heuristic batch (1000 `FM2_*Helper` / `*_Caller` names) is being
**replaced** with manual decompile-based names and explicit reasoning, same
standard as batches 1–3.

**Progress:** 700 / 978 placeholders corrected (passes 1–19 below). Remaining
placeholders tracked in `.cursor/hooks/state/batch4-placeholders.json`.

**Workflow:** `scripts/ida_fm2_decompile_placeholder_slice.py` (decompile slice)
→ manual name from strings/RTTI/vtable/callees → IDA MCP `rename` → log here.

### Manual re-pass 1 (35 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821D4648` | `FM2_Object_VirtualDispatch_Offset12` | Single-instruction vtable dispatch at offset +12. |
| `0x821D61A8` | `FM2_ForzaCmdLine_InitStartupList` | Uses `FM2_GetForzaCommandLineParamsSingleton` + `FM2_IntrusiveList_SpliceNodes` to build startup list. |
| `0x821D7AC8` | `FM2_Math_VmxTransformVectorBlock` | VMX128 `lvlx`/`stvx` vector transform with float inputs. |
| `0x821D8240` | `FM2_Keyframe_GetFloatAtOffset100` | Returns `*(float*)(a1+100)` — keyframe/scalar field accessor. |
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
| `0x8221EBE8` | `FM2_Settings_GetDegreesHiddenAsRadians` | Reads `"DegreesHidden"` hash; converts deg→rad. |
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
| `0x822BAFA8` | `FM2_Lua_PushDampingFromKeyframeFloat` | Keyframe float → damping lookup → Lua push. |
| `0x822BBC70` | `FM2_Math_VmxNormalizeAndPushForceVector` | VMX128 normalize (vrsqrtefp) then force vector push. |
| `0x822C36D0` | `FM2_Lua_GameLibraryDynamics_GetCompressorATM` | Lua `"compressorATM"` on `"GameLibrary::Dynamics"`. |
| `0x822C4748` | `FM2_Lua_GameLibraryTire_GetSuspensionDmg` | Lua `"suspensionDmg"` on `"GameLibrary::Tire"`. |
| `0x822C4D20` | `FM2_Lua_GameLibraryTire_GetDistUnder` | Lua `"DistUnder"` on `"GameLibrary::Tire"`. |
| `0x822C6060` | `FM2_Lua_GameLibraryLapTracker_GetMaxSegments` | Lua `"MAX_SEGMENTS"` constant 17 on `"GameLibrary::LapTracker"`. |
| `0x822C7FB8` | `FM2_Lua_PushEngineTypeEnumToStack` | Engine type index ≤0x1B → enum string push. |
| `0x822C92F0` | `FM2_Lua_LapTracker_GetSegmentTimeDelta` | Lap segment index → time delta push. |
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
| `0x822E7F68` | `FM2_Lua_GetObjectPropertyAsInt` | Property lookup vtable+108 → int push. |
| `0x822E8138` | `FM2_Lua_PushSslObjectToStack` | Wraps SSL object ref for Lua stack. |
| `0x822E8C28` | `FM2_Lua_MessageCenter_GetCarName` | Lua `"CarName"` on `"MessageCenter::Message"`. |
| `0x822E93A0` | `FM2_Lua_PushBoolFromFilenameMatch` | Filename substring match → bool push. |
| `0x822EC410` | `FM2_Lua_PushQwordPairToStack` | Object vtable+28 qword pair push. |
| `0x822ECC10` | `FM2_Lua_PushStringFromObjectVtable20` | Object vtable+20 string push. |
| `0x822EFC00` | `FM2_Lua_NetworkLobby_SetSystemLink` | Lua `"SetSystemLink"` → `FM2_RenderAdapter_SwitchPresentationMode(*,3)`. |
| `0x822F0110` | `FM2_Lua_NetworkLobby_SetMultiscreenClient` | Lua `"SetMultiscreenClient"` → presentation mode 9. |
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
| `0x823047C8` | `FM2_Lua_PushDegreesBetweenScreensFromHash` | Hash lookup `"DegreesBetweenScreens"` → float push. |
| `0x82307D58` | `FM2_Lua_ProfileManager_GetFpInitNeedStorageDevice` | Enum 1 for `"FP_INIT_NEED_VALID_STORAGE_DEVICE"`. |
| `0x82309A90` | `FM2_Lua_ForzaProfile_GetThrottleDeadzoneInsideWheel` | Profile float getter. |
| `0x8230A158` | `FM2_Lua_ForzaProfile_GetControllerType` | Profile controller type getter. |
| `0x8230ADF0` | `FM2_Lua_PushCarIdFromCarRecord` | Car record → car id push. |
| `0x8230B540` | `FM2_SavedReplay_Dtor` | Dtor for `CSavedReplay` vftable. |
| `0x8230E890` | `FM2_Lua_ReplayRecorder_SaveVolumeWrapper` | Lua `"saveVolumeWrapper"` on `"ReplayTheater::ReplayRecorder"`. |
| `0x8230EA30` | `FM2_ReplayTheater_RegisterCallbackThunk` | Thin thunk to replay theater callback register. |
| `0x8230F258` | `FM2_ReplayBuffer_DeleteOptional` | Replay buffer cleanup with optional free. |
| `0x823114A8` | `FM2_Replay_QueueSslReadWithArgs` | Queues SSL read with float/time args. |
| `0x82312060` | `FM2_Lua_RewardReveal_GetCarLevelRewardCompatInfo` | Lua on `"RewardReveals::RewardReveal"`. |
| `0x823126E0` | `FM2_Lua_RaceWinnings_GetDamagePenaltyValue` | Reads damage penalty from winnings object +160. |
| `0x82315128` | `FM2_Lua_CreateComPtrFromThreeLuaNumbers` | Three Lua numbers → COM object construct. |
| `0x823162F0` | `FM2_Lua_SavedGameContentItem_GetAsyncOperationResult` | Async op result enum push. |

### Manual re-pass 4 (35 functions)

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823165C0` | `FM2_Lua_SaveVolumeWrapper_GetOperationResult` | Lua `"operationResult"` on `"SavedGame::SaveVolumeWrapper"`. |
| `0x8231A420` | `FM2_Lua_Tournament_GetNumQualifyingEntries` | Lua on `"TournamentLua::Tournament"`. |
| `0x8231A880` | `FM2_Lua_Tournament_GetHasBranding` | Reads branding flag at object +25676. |
| `0x8231C7C8` | `FM2_Lua_PushFloatFromObjectField2` | Object field index 2 → float Lua push. |
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
| `0x82388E18` | `FM2_Image_SwapEndianPixelRow` | Per-pixel byte/word swap (big↔little) on RGBA row. |
| `0x8238C680` | `FM2_Image_LoadPngFromMemory` | D3DX `png_struct`/`png_info` decode + GPU alloc. |
| `0x8238EF08` | `FM2_Image_Downsample2x2BytesAverage` | 2×2 byte pixel box filter average. |
| `0x82391E28` | `FM2_Struct_ClearFiveDwords` | Zeroes five consecutive DWORD fields. |
| `0x823A4B38` | `FM2_Shader_ApplyConstantsBatch` | Batch-applies shader constants in nested loops. |
| `0x823AB9D8` | `FM2_Shader_InitCallbackTable` | Installs three shader callback fn ptrs. |
| `0x823AF460` | `FM2_Bitstream_ReadVariableBits` | Variable-width bit extraction from stream. |
| `0x823B46A0` | `FM2_Huffman_DecodeSymbol` | Huffman tree walk; error code 118 on overflow. |
| `0x823C0230` | `FM2_NetworkPacket_EncodeHeaderFields` | Encodes packet header fields at object +5812. |
| `0x823C1F88` | `FM2_ComObject_SyncChildProperties` | Syncs child COM property block via vtable+36. |
| `0x823CD040` | `FM2_D3D_BltRegionToSurface` | D3D blit with `tagRECT`/`tagPOINT` region args. |
| `0x823CF9F8` | `FM2_Math_VmxNormalizeVectorArray16` | VMX128 normalize 16 vectors using `unk_82030260` permute. |
| `0x823D3A40` | `FM2_Image_Downsample2x2Rgb565Average` | 2×2 RGB565 box filter with 0x7E0/0xF81F masks. |
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
| `0x8242F818` | `FM2_Stream_IsOpenAsBool` | Returns ±1 from stream vtable+60 open check. |
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
| `0x824C9520` | `FM2_CarAudio_ResetMixMatrixAndLoadStream` | Clears 10×4 mix matrix; loads stream object. |
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
| `0x825850E0` | `FM2_FMOD_Geometry_AddPolygonFromVMX` | VMX128 verts → `FMOD::Geometry::addPolygon`. |
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
| `0x826163C8` | `FM2_SceneGraph_GetNodeTypeName` | Maps node type id → name (`animationtrack`, etc.). |
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
| `0x826AF3A8` | `FM2_XAudio2_QueryInterface` | COM QueryInterface → `E_POINTER`. |
| `0x826B5170` | `FM2_XAudio2_VoicePool_StopAndRelease` | Stops voice pool; Rtl critsec. |
| `0x826B7390` | `FM2_XAudio2_Voice_SubmitFormatBuffer` | Submit buffer with wchar path. |
| `0x826BCFA0` | `FM2_XAudio2_WorkerThread_Main_WKTD` | Worker thread TLS tag `"WKTD"`. |
| `0x826C12B8` | `FM2_XAudio2_VoiceCallback_PostMessage` | Voice callback posts msg type 16. |
| `0x826C3000` | `FM2_XAudio2_StreamPool_UnlinkAndNotify` | Unlinks stream node from pool. |
| `0x826C30F0` | `FM2_XAudio2_Voice_ReleaseRef` | Atomic dec ref; `Nt_SetEvent`. |
| `0x826CC978` | `FM2_XAudio2_Stream_EndSubmitPacket` | End submit packet; critsec+524. |
| `0x826CCAA8` | `FM2_XAudio2_Stream_ResetSubmitState` | Resets stream submit state flags. |
| `0x826CE900` | `FM2_XAudio2_Stream_SubmitBufferLocked` | Locked buffer submit path. |
| `0x826CEFA0` | `FM2_XAudio2_Voice_DispatchPropertyMessage` | Switch on property ids 198–219. |
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
| `0x828052D0` | `FM2_Crt_AtexitRegisterRestartString_829C32A4` | CRT init `"Restart"` → `dword_829C32A4`. |
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
| `0x8281C480` | `FM2_Crt_AtexitRegisterRestartString_829DAC64` | CRT init `"Restart"` → `dword_829DAC64`. |
| `0x8281CE80` | `FM2_Crt_AtexitRegisterStartDrivingString` | CRT init `"StartDriving"` + `atexit`. |
| `0x8281CFC0` | `FM2_Crt_AtexitRegisterRestartString_829DB184` | CRT init `"Restart"` → `dword_829DB184`. |
| `0x8281E7B0` | `FM2_Crt_AtexitRegisterNullSub` | CRT `atexit(nullsub_3)`. |
| `0x8281F018` | `FM2_Crt_InitAllocatorCriticalSection` | Init critsec `stru_82A00E64` spin 0x100. |
| `0x8281FF80` | `FM2_XmlStaticInit_CacheTypeHandle_82A02BC4` | Static init caches XML type → global. |
| `0x82820C28` | `FM2_XmlStaticInit_CacheTypeHandle_82A02B9C` | Static init caches XML type → global. |
| `0x828212E8` | `FM2_XmlStaticInit_CacheTypeHandle_82A029D0` | Static init caches XML type → global. |
| `0x82824A70` | `FM2_XmlStaticInit_CacheTypeHandle_82A030B8` | Static init caches XML type → global. |
| `0x82825490` | `FM2_XmlStaticInit_CacheTypeHandle_82A02DC8` | Static init caches XML type → global. |
| `0x82825D90` | `FM2_XmlStaticInit_CacheTypeHandle_82A02F3C` | Static init caches XML type → global. |
| `0x82826D50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0302C` | Static init caches XML type → global. |
| `0x82827D60` | `FM2_XmlStaticInit_CacheTypeHandle_82A03354` | Static init caches XML type → global. |
| `0x82828198` | `FM2_XmlStaticInit_CacheTypeHandle_82A03284` | Static init caches XML type → global. |
| `0x828285D0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0344C` | Static init caches XML type → global. |
| `0x82828E88` | `FM2_XmlStaticInit_CacheTypeHandle_82A032E8` | Static init caches XML type → global. |
| `0x8282B5A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03404` | Static init caches XML type → global. |
| `0x8282CB10` | `FM2_XmlStaticInit_CacheTypeHandle_82A03544` | Static init caches XML type → global. |
| `0x8282DBA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03600` | Static init caches XML type → global. |
| `0x8282DE78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0376C` | Static init caches XML type → global. |
| `0x8282F3D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03744` | Static init caches XML type → global. |
| `0x8282F780` | `FM2_XmlStaticInit_CacheTypeHandle_82A03554` | Static init caches XML type → global. |
| `0x8282FD80` | `FM2_XmlStaticInit_CacheTypeHandle_82A03B48` | Static init caches XML type → global. |
| `0x82830950` | `FM2_XmlStaticInit_CacheTypeHandle_82A03C4C` | Static init caches XML type → global. |
| `0x828329A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A03CCC` | Static init caches XML type → global. |
| `0x82836350` | `FM2_XmlStaticInit_CacheTypeHandle_82A04088` | Static init caches XML type → global. |
| `0x82836AA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03FF4` | Static init caches XML type → global. |
| `0x82836C08` | `FM2_XmlStaticInit_CacheTypeHandle_82A03E00` | Static init caches XML type → global. |
| `0x82836D70` | `FM2_XmlStaticInit_CacheTypeHandle_82A0412C` | Static init caches XML type → global. |

### Manual re-pass 18 (35 functions)

Lua UI/tuning bindings + XML type-handle static init (continued).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828372C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A040AC` | Static init caches XML type → `dword_82A040AC`. |
| `0x82837CA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A03FB8` | Static init caches XML type → `dword_82A03FB8`. |
| `0x82838EA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A04478` | Static init caches XML type → `dword_82A04478`. |
| `0x82839178` | `FM2_XmlStaticInit_CacheTypeHandle_82A0429C` | Static init caches XML type → `dword_82A0429C`. |
| `0x8283B410` | `FM2_XmlStaticInit_CacheTypeHandle_82A043F8` | Static init caches XML type → `dword_82A043F8`. |
| `0x8283BBF0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04404` | Static init caches XML type → `dword_82A04404`. |
| `0x8283C028` | `FM2_XmlStaticInit_CacheTypeHandle_82A04494` | Static init caches XML type → `dword_82A04494`. |
| `0x8283C198` | `FM2_XmlStaticInit_CacheTypeHandle_82A047D8` | Static init caches XML type → `dword_82A047D8`. |
| `0x8283DB30` | `FM2_XmlStaticInit_CacheTypeHandle_82A04728` | Static init caches XML type → `dword_82A04728`. |
| `0x8283DFB0` | `FM2_XmlStaticInit_CacheTypeHandle_82A048C4` | Static init caches XML type → `dword_82A048C4`. |
| `0x8283E790` | `FM2_XmlStaticInit_CacheTypeHandle_82A045D8` | Static init caches XML type → `dword_82A045D8`. |
| `0x8283ECA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04904` | Static init caches XML type → `dword_82A04904`. |
| `0x8283F3A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A045B4` | Static init caches XML type → `dword_82A045B4`. |
| `0x8283F4C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A047E8` | Static init caches XML type → `dword_82A047E8`. |
| `0x8283F750` | `FM2_XmlStaticInit_CacheTypeHandle_82A04764` | Static init caches XML type → `dword_82A04764`. |
| `0x82840FC0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04B6C` | Static init caches XML type → `dword_82A04B6C`. |
| `0x82841830` | `FM2_XmlStaticInit_CacheTypeHandle_82A04B24` | Static init caches XML type → `dword_82A04B24`. |
| `0x82843EB8` | `FM2_XmlStaticInit_CacheTypeHandle_82A04CCC` | Static init caches XML type → `dword_82A04CCC`. |
| `0x82846230` | `FM2_XmlStaticInit_CacheTypeHandle_82A051B4` | Static init caches XML type → `dword_82A051B4`. |
| `0x82846D70` | `FM2_XmlStaticInit_CacheTypeHandle_82A04E78` | Static init caches XML type → `dword_82A04E78`. |
| `0x82847CA0` | `FM2_XmlStaticInit_CacheTypeHandle_82A04E84` | Static init caches XML type → `dword_82A04E84`. |
| `0x828481F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A05020` | Static init caches XML type → `dword_82A05020`. |
| `0x828482D0` | `FM2_XmlStaticInit_CacheTypeHandle_82A050DC` | Static init caches XML type → `dword_82A050DC`. |
| `0x828489B0` | `FM2_Lua_RegisterSetOverlay` | Registers Lua UI `"setOverlay"`. |
| `0x82848DA0` | `FM2_Lua_RegisterGuideResult` | Registers Lua UI `"GuideResult"`. |
| `0x82848DC0` | `FM2_Lua_RegisterSetEvent` | Registers Lua UI `"setEvent"`. |
| `0x82848F20` | `FM2_Lua_RegisterGoToFlow` | Registers Lua UI `"goToFlow"`. |
| `0x82849120` | `FM2_Lua_RegisterFadeInBeginEventName` | Registers Lua scene `"FadeInBeginEventName"`. |
| `0x828493D0` | `FM2_Lua_RegisterDisplacement` | Registers Lua tuning `"Displacement"`. |
| `0x82849550` | `FM2_Lua_RegisterTuningTirePressure` | Registers Lua `"TuningTirePressure"`. |
| `0x828497E0` | `FM2_Lua_RegisterDisplacement_82A04D20` | Second `"Displacement"` on table `82A04D20`. |
| `0x8284A110` | `FM2_Lua_RegisterMessage` | Registers Lua `"message"`. |
| `0x8284A578` | `FM2_XmlStaticInit_CacheTypeHandle_82A053AC` | Static init caches XML type → `dword_82A053AC`. |
| `0x8284B2F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A05374` | Static init caches XML type → `dword_82A05374`. |
| `0x8284BD18` | `FM2_XmlStaticInit_CacheTypeHandle_82A05288` | Static init caches XML type → `dword_82A05288`. |

### Manual re-pass 19 (35 functions)

XML type-handle static init (continued) + CRT atexit hook.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8284C198` | `FM2_XmlStaticInit_CacheTypeHandle_82A05384` | Static init caches XML type → `dword_82A05384`. |
| `0x8284C660` | `FM2_XmlStaticInit_CacheTypeHandle_82A052AC` | Static init caches XML type → `dword_82A052AC`. |
| `0x8284C738` | `FM2_XmlStaticInit_CacheTypeHandle_82A05564` | Static init caches XML type → `dword_82A05564`. |
| `0x8284D038` | `FM2_XmlStaticInit_CacheTypeHandle_82A05278` | Static init caches XML type → `dword_82A05278`. |
| `0x8284DD28` | `FM2_XmlStaticInit_CacheTypeHandle_82A05484` | Static init caches XML type → `dword_82A05484`. |
| `0x8284DE48` | `FM2_XmlStaticInit_CacheTypeHandle_82A05408` | Static init caches XML type → `dword_82A05408`. |
| `0x8284E088` | `FM2_XmlStaticInit_CacheTypeHandle_82A05494` | Static init caches XML type → `dword_82A05494`. |
| `0x8284E358` | `FM2_XmlStaticInit_CacheTypeHandle_82A055B4` | Static init caches XML type → `dword_82A055B4`. |
| `0x8284E630` | `FM2_Crt_AtexitRegisterSub_8294C858` | CRT `atexit(sub_8294C858)`. |
| `0x82850D58` | `FM2_XmlStaticInit_CacheTypeHandle_82A059C0` | Static init caches XML type → `dword_82A059C0`. |
| `0x828524F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A058D8` | Static init caches XML type → `dword_82A058D8`. |
| `0x82852A10` | `FM2_XmlStaticInit_CacheTypeHandle_82A05D30` | Static init caches XML type → `dword_82A05D30`. |
| `0x82852B30` | `FM2_XmlStaticInit_CacheTypeHandle_82A05BD0` | Static init caches XML type → `dword_82A05BD0`. |
| `0x82854900` | `FM2_XmlStaticInit_CacheTypeHandle_82A05C1C` | Static init caches XML type → `dword_82A05C1C`. |
| `0x82856058` | `FM2_XmlStaticInit_CacheTypeHandle_82A05D10` | Static init caches XML type → `dword_82A05D10`. |
| `0x828569A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06078` | Static init caches XML type → `dword_82A06078`. |
| `0x82856B10` | `FM2_XmlStaticInit_CacheTypeHandle_82A060D8` | Static init caches XML type → `dword_82A060D8`. |
| `0x828570B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A06254` | Static init caches XML type → `dword_82A06254`. |
| `0x82857218` | `FM2_XmlStaticInit_CacheTypeHandle_82A06210` | Static init caches XML type → `dword_82A06210`. |
| `0x828572A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0627C` | Static init caches XML type → `dword_82A0627C`. |
| `0x82859930` | `FM2_XmlStaticInit_CacheTypeHandle_82A05F80` | Static init caches XML type → `dword_82A05F80`. |
| `0x82859C48` | `FM2_XmlStaticInit_CacheTypeHandle_82A0621C` | Static init caches XML type → `dword_82A0621C`. |
| `0x82859FA8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06148` | Static init caches XML type → `dword_82A06148`. |
| `0x8285ACD8` | `FM2_XmlStaticInit_CacheTypeHandle_82A064B8` | Static init caches XML type → `dword_82A064B8`. |
| `0x8285B080` | `FM2_XmlStaticInit_CacheTypeHandle_82A0643C` | Static init caches XML type → `dword_82A0643C`. |
| `0x8285B5D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06354` | Static init caches XML type → `dword_82A06354`. |
| `0x8285C1A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06534` | Static init caches XML type → `dword_82A06534`. |
| `0x8285C598` | `FM2_XmlStaticInit_CacheTypeHandle_82A064C0` | Static init caches XML type → `dword_82A064C0`. |
| `0x8285CD78` | `FM2_XmlStaticInit_CacheTypeHandle_82A063D8` | Static init caches XML type → `dword_82A063D8`. |
| `0x8285D6C0` | `FM2_XmlStaticInit_CacheTypeHandle_82A066A0` | Static init caches XML type → `dword_82A066A0`. |
| `0x82860770` | `FM2_XmlStaticInit_CacheTypeHandle_82A06898` | Static init caches XML type → `dword_82A06898`. |
| `0x82860F98` | `FM2_XmlStaticInit_CacheTypeHandle_82A06840` | Static init caches XML type → `dword_82A06840`. |
| `0x82861100` | `FM2_XmlStaticInit_CacheTypeHandle_82A06908` | Static init caches XML type → `dword_82A06908`. |
| `0x82861610` | `FM2_XmlStaticInit_CacheTypeHandle_82A06978` | Static init caches XML type → `dword_82A06978`. |
| `0x828616A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0677C` | Static init caches XML type → `dword_82A0677C`. |

### Manual re-pass 20 (35 functions)

XML type-handle static init hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828619B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A066B0` | Static init caches XML type → `dword_82A066B0`. |
| `0x82861D60` | `FM2_XmlStaticInit_CacheTypeHandle_82A06724` | Static init caches XML type → `dword_82A06724`. |
| `0x82862300` | `FM2_XmlStaticInit_CacheTypeHandle_82A067B0` | Static init caches XML type → `dword_82A067B0`. |
| `0x82862930` | `FM2_XmlStaticInit_CacheTypeHandle_82A06980` | Static init caches XML type → `dword_82A06980`. |
| `0x82862A50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0689C` | Static init caches XML type → `dword_82A0689C`. |
| `0x82862A98` | `FM2_XmlStaticInit_CacheTypeHandle_82A068F4` | Static init caches XML type → `dword_82A068F4`. |
| `0x82863940` | `FM2_XmlStaticInit_CacheTypeHandle_82A06C28` | Static init caches XML type → `dword_82A06C28`. |
| `0x82865128` | `FM2_XmlStaticInit_CacheTypeHandle_82A06B70` | Static init caches XML type → `dword_82A06B70`. |
| `0x828655F0` | `FM2_XmlStaticInit_CacheTypeHandle_82A06E10` | Static init caches XML type → `dword_82A06E10`. |
| `0x828668C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06DA4` | Static init caches XML type → `dword_82A06DA4`. |
| `0x82867F20` | `FM2_XmlStaticInit_CacheTypeHandle_82A071E8` | Static init caches XML type → `dword_82A071E8`. |
| `0x82868E98` | `FM2_XmlStaticInit_CacheTypeHandle_82A0720C` | Static init caches XML type → `dword_82A0720C`. |
| `0x8286A6C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A06EA4` | Static init caches XML type → `dword_82A06EA4`. |
| `0x8286B298` | `FM2_XmlStaticInit_CacheTypeHandle_82A06E60` | Static init caches XML type → `dword_82A06E60`. |
| `0x8286CF98` | `FM2_XmlStaticInit_CacheTypeHandle_82A07458` | Static init caches XML type → `dword_82A07458`. |
| `0x8286D4F0` | `FM2_XmlStaticInit_CacheTypeHandle_82A072A8` | Static init caches XML type → `dword_82A072A8`. |
| `0x8286DF58` | `FM2_XmlStaticInit_CacheTypeHandle_82A07400` | Static init caches XML type → `dword_82A07400`. |
| `0x8286EA98` | `FM2_XmlStaticInit_CacheTypeHandle_82A07404` | Static init caches XML type → `dword_82A07404`. |
| `0x8286F038` | `FM2_XmlStaticInit_CacheTypeHandle_82A07398` | Static init caches XML type → `dword_82A07398`. |
| `0x8286F620` | `FM2_XmlStaticInit_CacheTypeHandle_82A07450` | Static init caches XML type → `dword_82A07450`. |
| `0x8286FAF8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0798C` | Static init caches XML type → `dword_82A0798C`. |
| `0x82872C78` | `FM2_XmlStaticInit_CacheTypeHandle_82A07938` | Static init caches XML type → `dword_82A07938`. |
| `0x828753E8` | `FM2_XmlStaticInit_CacheTypeHandle_82A07C00` | Static init caches XML type → `dword_82A07C00`. |
| `0x828758B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A07AD0` | Static init caches XML type → `dword_82A07AD0`. |
| `0x82876A68` | `FM2_XmlStaticInit_CacheTypeHandle_82A07A14` | Static init caches XML type → `dword_82A07A14`. |
| `0x82876F78` | `FM2_XmlStaticInit_CacheTypeHandle_82A07BAC` | Static init caches XML type → `dword_82A07BAC`. |
| `0x82877E28` | `FM2_XmlStaticInit_CacheTypeHandle_82A0804C` | Static init caches XML type → `dword_82A0804C`. |
| `0x828789F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A07FD0` | Static init caches XML type → `dword_82A07FD0`. |
| `0x8287BEE8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08304` | Static init caches XML type → `dword_82A08304`. |
| `0x8287C248` | `FM2_XmlStaticInit_CacheTypeHandle_82A08464` | Static init caches XML type → `dword_82A08464`. |
| `0x8287C998` | `FM2_XmlStaticInit_CacheTypeHandle_82A08138` | Static init caches XML type → `dword_82A08138`. |
| `0x8287E7F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08170` | Static init caches XML type → `dword_82A08170`. |
| `0x8287EA80` | `FM2_XmlStaticInit_CacheTypeHandle_82A082B0` | Static init caches XML type → `dword_82A082B0`. |
| `0x8287ED98` | `FM2_XmlStaticInit_CacheTypeHandle_82A082E0` | Static init caches XML type → `dword_82A082E0`. |
| `0x82880038` | `FM2_XmlStaticInit_CacheTypeHandle_82A084AC` | Static init caches XML type → `dword_82A084AC`. |

### Manual re-pass 21 (35 functions)

XML type-handle static init hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82881550` | `FM2_XmlStaticInit_CacheTypeHandle_82A086EC` | Static init caches XML type → `dword_82A086EC`. |
| `0x82881598` | `FM2_XmlStaticInit_CacheTypeHandle_82A084C0` | Static init caches XML type → `dword_82A084C0`. |
| `0x828823A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08804` | Static init caches XML type → `dword_82A08804`. |
| `0x82882EE8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0877C` | Static init caches XML type → `dword_82A0877C`. |
| `0x82883440` | `FM2_XmlStaticInit_CacheTypeHandle_82A08738` | Static init caches XML type → `dword_82A08738`. |
| `0x82885B68` | `FM2_XmlStaticInit_CacheTypeHandle_82A08A44` | Static init caches XML type → `dword_82A08A44`. |
| `0x82886468` | `FM2_XmlStaticInit_CacheTypeHandle_82A08AA8` | Static init caches XML type → `dword_82A08AA8`. |
| `0x82887230` | `FM2_XmlStaticInit_CacheTypeHandle_82A089A4` | Static init caches XML type → `dword_82A089A4`. |
| `0x828879C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08AC0` | Static init caches XML type → `dword_82A08AC0`. |
| `0x82887F30` | `FM2_XmlStaticInit_CacheTypeHandle_82A08ED0` | Static init caches XML type → `dword_82A08ED0`. |
| `0x828881B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08CB0` | Static init caches XML type → `dword_82A08CB0`. |
| `0x828883F8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08E18` | Static init caches XML type → `dword_82A08E18`. |
| `0x828895B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A08F58` | Static init caches XML type → `dword_82A08F58`. |
| `0x828898C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A08C6C` | Static init caches XML type → `dword_82A08C6C`. |
| `0x8288AE70` | `FM2_XmlStaticInit_CacheTypeHandle_82A08DC8` | Static init caches XML type → `dword_82A08DC8`. |
| `0x8288BDF8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0920C` | Static init caches XML type → `dword_82A0920C`. |
| `0x8288C230` | `FM2_XmlStaticInit_CacheTypeHandle_82A09054` | Static init caches XML type → `dword_82A09054`. |
| `0x8288C5D8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09198` | Static init caches XML type → `dword_82A09198`. |
| `0x8288C620` | `FM2_XmlStaticInit_CacheTypeHandle_82A0925C` | Static init caches XML type → `dword_82A0925C`. |
| `0x8288C6B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A092C8` | Static init caches XML type → `dword_82A092C8`. |
| `0x8288C8A8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0907C` | Static init caches XML type → `dword_82A0907C`. |
| `0x8288ECF0` | `FM2_XmlStaticInit_CacheTypeHandle_82A08F94` | Static init caches XML type → `dword_82A08F94`. |
| `0x82892078` | `FM2_XmlStaticInit_CacheTypeHandle_82A093EC` | Static init caches XML type → `dword_82A093EC`. |
| `0x828922B8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09650` | Static init caches XML type → `dword_82A09650`. |
| `0x82892738` | `FM2_XmlStaticInit_CacheTypeHandle_82A095DC` | Static init caches XML type → `dword_82A095DC`. |
| `0x828931A0` | `FM2_XmlStaticInit_CacheTypeHandle_82A095C0` | Static init caches XML type → `dword_82A095C0`. |
| `0x82893590` | `FM2_XmlStaticInit_CacheTypeHandle_82A09524` | Static init caches XML type → `dword_82A09524`. |
| `0x828936B0` | `FM2_XmlStaticInit_CacheTypeHandle_82A09548` | Static init caches XML type → `dword_82A09548`. |
| `0x828939C8` | `FM2_XmlStaticInit_CacheTypeHandle_82A096B8` | Static init caches XML type → `dword_82A096B8`. |
| `0x82894050` | `FM2_XmlStaticInit_CacheTypeHandle_82A09818` | Static init caches XML type → `dword_82A09818`. |
| `0x82896A80` | `FM2_XmlStaticInit_CacheTypeHandle_82A096C0` | Static init caches XML type → `dword_82A096C0`. |
| `0x82896EB8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0977C` | Static init caches XML type → `dword_82A0977C`. |
| `0x82898278` | `FM2_XmlStaticInit_CacheTypeHandle_82A09CE8` | Static init caches XML type → `dword_82A09CE8`. |
| `0x82898590` | `FM2_XmlStaticInit_CacheTypeHandle_82A09DE0` | Static init caches XML type → `dword_82A09DE0`. |
| `0x82898620` | `FM2_XmlStaticInit_CacheTypeHandle_82A09D8C` | Static init caches XML type → `dword_82A09D8C`. |


### Manual re-pass 22 (35 functions)

Offset 770+: XML type-handle static init hooks (plus CRT string/ptr-pair inits in pass 23).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8289a8b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A09AF4` | Static init caches XML type → `dword_82A09AF4`. |
| `0x8289b320` | `FM2_XmlStaticInit_CacheTypeHandle_82A09DCC` | Static init caches XML type → `dword_82A09DCC`. |
| `0x8289cc38` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A060` | Static init caches XML type → `dword_82A0A060`. |
| `0x8289e030` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A0F0` | Static init caches XML type → `dword_82A0A0F0`. |
| `0x8289e780` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A154` | Static init caches XML type → `dword_82A0A154`. |
| `0x8289eb28` | `FM2_XmlStaticInit_CacheTypeHandle_82A09EAC` | Static init caches XML type → `dword_82A09EAC`. |
| `0x8289ecd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A1B0` | Static init caches XML type → `dword_82A0A1B0`. |
| `0x828a0bd0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A4DC` | Static init caches XML type → `dword_82A0A4DC`. |
| `0x828a3c78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A204` | Static init caches XML type → `dword_82A0A204`. |
| `0x828a43d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A8AC` | Static init caches XML type → `dword_82A0A8AC`. |
| `0x828a4580` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A664` | Static init caches XML type → `dword_82A0A664`. |
| `0x828a5cd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A6B4` | Static init caches XML type → `dword_82A0A6B4`. |
| `0x828a80d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A748` | Static init caches XML type → `dword_82A0A748`. |
| `0x828a8680` | `FM2_XmlStaticInit_CacheTypeHandle_82A0A9FC` | Static init caches XML type → `dword_82A0A9FC`. |
| `0x828a8bd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AAEC` | Static init caches XML type → `dword_82A0AAEC`. |
| `0x828a9328` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AC1C` | Static init caches XML type → `dword_82A0AC1C`. |
| `0x828a9640` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AAB0` | Static init caches XML type → `dword_82A0AAB0`. |
| `0x828a98c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AC8C` | Static init caches XML type → `dword_82A0AC8C`. |
| `0x828a9b98` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB48` | Static init caches XML type → `dword_82A0AB48`. |
| `0x828aa7b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB38` | Static init caches XML type → `dword_82A0AB38`. |
| `0x828ab650` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AB54` | Static init caches XML type → `dword_82A0AB54`. |
| `0x828acce0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AFA0` | Static init caches XML type → `dword_82A0AFA0`. |
| `0x828ad8b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AFA4` | Static init caches XML type → `dword_82A0AFA4`. |
| `0x828adb80` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AD20` | Static init caches XML type → `dword_82A0AD20`. |
| `0x828af368` | `FM2_XmlStaticInit_CacheTypeHandle_82A0AEE0` | Static init caches XML type → `dword_82A0AEE0`. |
| `0x828af518` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B004` | Static init caches XML type → `dword_82A0B004`. |
| `0x828b1d18` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B3E4` | Static init caches XML type → `dword_82A0B3E4`. |
| `0x828b27c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B09C` | Static init caches XML type → `dword_82A0B09C`. |
| `0x828b4f78` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B708` | Static init caches XML type → `dword_82A0B708`. |
| `0x828b5248` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B734` | Static init caches XML type → `dword_82A0B734`. |
| `0x828b6910` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B5AC` | Static init caches XML type → `dword_82A0B5AC`. |
| `0x828b6c70` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B72C` | Static init caches XML type → `dword_82A0B72C`. |
| `0x828b6f88` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B4D8` | Static init caches XML type → `dword_82A0B4D8`. |
| `0x828b75b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0B4DC` | Static init caches XML type → `dword_82A0B4DC`. |
| `0x828b85c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BB58` | Static init caches XML type → `dword_82A0BB58`. |

### Manual re-pass 23 (35 functions)

Offset 770+: XML type-handle static init hooks (plus CRT string/ptr-pair inits in pass 23).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828b9c90` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BDA0` | Static init caches XML type → `dword_82A0BDA0`. |
| `0x828ba080` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BBEC` | Static init caches XML type → `dword_82A0BBEC`. |
| `0x828ba1a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BCD8` | Static init caches XML type → `dword_82A0BCD8`. |
| `0x828ba818` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BC04` | Static init caches XML type → `dword_82A0BC04`. |
| `0x828baa10` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BBF0` | Static init caches XML type → `dword_82A0BBF0`. |
| `0x828baf68` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BDB0` | Static init caches XML type → `dword_82A0BDB0`. |
| `0x828bc288` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BCA4` | Static init caches XML type → `dword_82A0BCA4`. |
| `0x828bc438` | `FM2_Crt_InitStaticString_82A0BC84` | CRT static init: `FM2_Stl_String_InitOrClear(&unk_82A0BC84)` + `atexit(sub_8294D210)`. |
| `0x828bc5a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BF00` | Static init caches XML type → `dword_82A0BF00`. |
| `0x828bd298` | `FM2_XmlStaticInit_CacheTypeHandle_82A0BE10` | Static init caches XML type → `dword_82A0BE10`. |
| `0x828bdb50` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C094` | Static init caches XML type → `dword_82A0C094`. |
| `0x828c0070` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C178` | Static init caches XML type → `dword_82A0C178`. |
| `0x828c1308` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C2A8` | Static init caches XML type → `dword_82A0C2A8`. |
| `0x828c2f28` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C210` | Static init caches XML type → `dword_82A0C210`. |
| `0x828c3948` | `FM2_XmlStaticInit_CacheTypeHandle_82A0C358` | Static init caches XML type → `dword_82A0C358`. |
| `0x828c5408` | `FM2_Crt_StaticInitPtrPair_82A43B1C` | CRT static init: `sub_8276B730` stores ptr+count into `82a43b1c`. |
| `0x828c5cc8` | `FM2_Crt_StaticInitPtrPair_82A43C5C` | CRT static init: `sub_8276B730` stores ptr+count into `82a43c5c`. |
| `0x828c6160` | `FM2_Crt_StaticInitPtrPair_82A43D04` | CRT static init: `sub_8276B730` stores ptr+count into `82a43d04`. |
| `0x828c6cc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70E50` | Static init caches XML type → `dword_82A70E50`. |
| `0x828c7578` | `FM2_XmlStaticInit_CacheTypeHandle_82A70D08` | Static init caches XML type → `dword_82A70D08`. |
| `0x828c8388` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C30` | Static init caches XML type → `dword_82A70C30`. |
| `0x828c8580` | `FM2_XmlStaticInit_CacheTypeHandle_82A70E28` | Static init caches XML type → `dword_82A70E28`. |
| `0x828c8928` | `FM2_XmlStaticInit_CacheTypeHandle_82A70F00` | Static init caches XML type → `dword_82A70F00`. |
| `0x828c9270` | `FM2_XmlStaticInit_CacheTypeHandle_82A70FB4` | Static init caches XML type → `dword_82A70FB4`. |
| `0x828c99c0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C7C` | Static init caches XML type → `dword_82A70C7C`. |
| `0x828c9c48` | `FM2_XmlStaticInit_CacheTypeHandle_82A70DC4` | Static init caches XML type → `dword_82A70DC4`. |
| `0x828cb058` | `FM2_XmlStaticInit_CacheTypeHandle_82A70C64` | Static init caches XML type → `dword_82A70C64`. |
| `0x828cb1c0` | `FM2_XmlStaticInit_CacheTypeHandle_82A70CFC` | Static init caches XML type → `dword_82A70CFC`. |
| `0x828cb600` | `FM2_XmlStaticInit_CacheTypeHandle_82A7112C` | Static init caches XML type → `dword_82A7112C`. |
| `0x828ccb60` | `FM2_XmlStaticInit_CacheTypeHandle_82A71234` | Static init caches XML type → `dword_82A71234`. |
| `0x828cd1d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A71134` | Static init caches XML type → `dword_82A71134`. |
| `0x828cdd60` | `FM2_XmlStaticInit_CacheTypeHandle_82A712B0` | Static init caches XML type → `dword_82A712B0`. |
| `0x828ce8e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A711C4` | Static init caches XML type → `dword_82A711C4`. |
| `0x828cf278` | `FM2_XmlStaticInit_CacheTypeHandle_82A71128` | Static init caches XML type → `dword_82A71128`. |
| `0x828cf8b0` | `FM2_XmlStaticInit_CacheTypeHandle_82A716D4` | Static init caches XML type → `dword_82A716D4`. |


### Manual re-pass 24 (35 functions)

XML type-handle static init hooks (offset 840+).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828d05a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A713D8` | Static init caches XML type → `dword_82A713D8`. |
| `0x828d1e60` | `FM2_XmlStaticInit_CacheTypeHandle_82A7149C` | Static init caches XML type → `dword_82A7149C`. |
| `0x828d1fc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A714B4` | Static init caches XML type → `dword_82A714B4`. |
| `0x828d2178` | `FM2_XmlStaticInit_CacheTypeHandle_82A71450` | Static init caches XML type → `dword_82A71450`. |
| `0x828d3528` | `FM2_XmlStaticInit_CacheTypeHandle_82A7160C` | Static init caches XML type → `dword_82A7160C`. |
| `0x828d4148` | `FM2_XmlStaticInit_CacheTypeHandle_82A717DC` | Static init caches XML type → `dword_82A717DC`. |
| `0x828d4340` | `FM2_XmlStaticInit_CacheTypeHandle_82A717E0` | Static init caches XML type → `dword_82A717E0`. |
| `0x828d5858` | `FM2_XmlStaticInit_CacheTypeHandle_82A71AF0` | Static init caches XML type → `dword_82A71AF0`. |
| `0x828d6bc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A7181C` | Static init caches XML type → `dword_82A7181C`. |
| `0x828d8fe0` | `FM2_XmlStaticInit_CacheTypeHandle_82A71DF0` | Static init caches XML type → `dword_82A71DF0`. |
| `0x828da978` | `FM2_XmlStaticInit_CacheTypeHandle_82A71E10` | Static init caches XML type → `dword_82A71E10`. |
| `0x828dbe00` | `FM2_XmlStaticInit_CacheTypeHandle_82A71C08` | Static init caches XML type → `dword_82A71C08`. |
| `0x828dc2d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A720CC` | Static init caches XML type → `dword_82A720CC`. |
| `0x828dd638` | `FM2_XmlStaticInit_CacheTypeHandle_82A71FA4` | Static init caches XML type → `dword_82A71FA4`. |
| `0x828ddc20` | `FM2_XmlStaticInit_CacheTypeHandle_82A72248` | Static init caches XML type → `dword_82A72248`. |
| `0x828de688` | `FM2_XmlStaticInit_CacheTypeHandle_82A71F04` | Static init caches XML type → `dword_82A71F04`. |
| `0x828de880` | `FM2_XmlStaticInit_CacheTypeHandle_82A71FBC` | Static init caches XML type → `dword_82A71FBC`. |
| `0x828df600` | `FM2_XmlStaticInit_CacheTypeHandle_82A7210C` | Static init caches XML type → `dword_82A7210C`. |
| `0x828df6d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72074` | Static init caches XML type → `dword_82A72074`. |
| `0x828e0968` | `FM2_XmlStaticInit_CacheTypeHandle_82A72678` | Static init caches XML type → `dword_82A72678`. |
| `0x828e23d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A723D4` | Static init caches XML type → `dword_82A723D4`. |
| `0x828e5200` | `FM2_XmlStaticInit_CacheTypeHandle_82A72810` | Static init caches XML type → `dword_82A72810`. |
| `0x828e6b50` | `FM2_XmlStaticInit_CacheTypeHandle_82A72918` | Static init caches XML type → `dword_82A72918`. |
| `0x828e71c8` | `FM2_XmlStaticInit_CacheTypeHandle_82A726F4` | Static init caches XML type → `dword_82A726F4`. |
| `0x828e8500` | `FM2_XmlStaticInit_CacheTypeHandle_82A72E28` | Static init caches XML type → `dword_82A72E28`. |
| `0x828e8ff8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72D90` | Static init caches XML type → `dword_82A72D90`. |
| `0x828e9ce8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72E1C` | Static init caches XML type → `dword_82A72E1C`. |
| `0x828ea5e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72DF8` | Static init caches XML type → `dword_82A72DF8`. |
| `0x828eaa20` | `FM2_XmlStaticInit_CacheTypeHandle_82A72BD4` | Static init caches XML type → `dword_82A72BD4`. |
| `0x828eb290` | `FM2_XmlStaticInit_CacheTypeHandle_82A72ACC` | Static init caches XML type → `dword_82A72ACC`. |
| `0x828ebc68` | `FM2_XmlStaticInit_CacheTypeHandle_82A72D44` | Static init caches XML type → `dword_82A72D44`. |
| `0x828ec930` | `FM2_XmlStaticInit_CacheTypeHandle_82A73038` | Static init caches XML type → `dword_82A73038`. |
| `0x828ecc00` | `FM2_XmlStaticInit_CacheTypeHandle_82A731D4` | Static init caches XML type → `dword_82A731D4`. |
| `0x828ecfa8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72FE0` | Static init caches XML type → `dword_82A72FE0`. |
| `0x828ed428` | `FM2_XmlStaticInit_CacheTypeHandle_82A730E4` | Static init caches XML type → `dword_82A730E4`. |

### Manual re-pass 25 (35 functions)

XML type-handle static init hooks (offset 840+).

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x828ed980` | `FM2_XmlStaticInit_CacheTypeHandle_82A72F48` | Static init caches XML type → `dword_82A72F48`. |
| `0x828ef438` | `FM2_XmlStaticInit_CacheTypeHandle_82A73004` | Static init caches XML type → `dword_82A73004`. |
| `0x828efdc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A72FBC` | Static init caches XML type → `dword_82A72FBC`. |
| `0x828eff30` | `FM2_XmlStaticInit_CacheTypeHandle_82A73158` | Static init caches XML type → `dword_82A73158`. |
| `0x828f0db0` | `FM2_XmlStaticInit_CacheTypeHandle_82A73584` | Static init caches XML type → `dword_82A73584`. |
| `0x828f1b30` | `FM2_XmlStaticInit_CacheTypeHandle_82A733A8` | Static init caches XML type → `dword_82A733A8`. |
| `0x828f1e48` | `FM2_XmlStaticInit_CacheTypeHandle_82A735DC` | Static init caches XML type → `dword_82A735DC`. |
| `0x828f2a60` | `FM2_XmlStaticInit_CacheTypeHandle_82A735B4` | Static init caches XML type → `dword_82A735B4`. |
| `0x828f2e98` | `FM2_XmlStaticInit_CacheTypeHandle_82A7358C` | Static init caches XML type → `dword_82A7358C`. |
| `0x828f5138` | `FM2_XmlStaticInit_CacheTypeHandle_82A736E0` | Static init caches XML type → `dword_82A736E0`. |
| `0x828f55b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A73770` | Static init caches XML type → `dword_82A73770`. |
| `0x828f6068` | `FM2_XmlStaticInit_CacheTypeHandle_82A73860` | Static init caches XML type → `dword_82A73860`. |
| `0x828f6608` | `FM2_XmlStaticInit_CacheTypeHandle_82A73814` | Static init caches XML type → `dword_82A73814`. |
| `0x828f7028` | `FM2_XmlStaticInit_CacheTypeHandle_82A736CC` | Static init caches XML type → `dword_82A736CC`. |
| `0x828f74a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A739A8` | Static init caches XML type → `dword_82A739A8`. |
| `0x828f7a90` | `FM2_XmlStaticInit_CacheTypeHandle_82A73638` | Static init caches XML type → `dword_82A73638`. |
| `0x828f82b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A738CC` | Static init caches XML type → `dword_82A738CC`. |
| `0x828fa678` | `FM2_XmlStaticInit_CacheTypeHandle_82A73D50` | Static init caches XML type → `dword_82A73D50`. |
| `0x828fa9d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A73B4C` | Static init caches XML type → `dword_82A73B4C`. |
| `0x828fd020` | `FM2_XmlStaticInit_CacheTypeHandle_82A73EE8` | Static init caches XML type → `dword_82A73EE8`. |
| `0x828fdd58` | `FM2_XmlStaticInit_CacheTypeHandle_82A73F04` | Static init caches XML type → `dword_82A73F04`. |
| `0x828fe928` | `FM2_XmlStaticInit_CacheTypeHandle_82A73E68` | Static init caches XML type → `dword_82A73E68`. |
| `0x828fecd0` | `FM2_XmlStaticInit_CacheTypeHandle_82A73FA0` | Static init caches XML type → `dword_82A73FA0`. |
| `0x828ff738` | `FM2_XmlStaticInit_CacheTypeHandle_82A7411C` | Static init caches XML type → `dword_82A7411C`. |
| `0x828ffe88` | `FM2_XmlStaticInit_CacheTypeHandle_82A7407C` | Static init caches XML type → `dword_82A7407C`. |
| `0x82900f68` | `FM2_XmlStaticInit_CacheTypeHandle_82A74304` | Static init caches XML type → `dword_82A74304`. |
| `0x82901a60` | `FM2_XmlStaticInit_CacheTypeHandle_82A74454` | Static init caches XML type → `dword_82A74454`. |
| `0x82902750` | `FM2_XmlStaticInit_CacheTypeHandle_82A74434` | Static init caches XML type → `dword_82A74434`. |
| `0x82902948` | `FM2_XmlStaticInit_CacheTypeHandle_82A744E8` | Static init caches XML type → `dword_82A744E8`. |
| `0x82902b40` | `FM2_XmlStaticInit_CacheTypeHandle_82A744D8` | Static init caches XML type → `dword_82A744D8`. |
| `0x82903d88` | `FM2_XmlStaticInit_CacheTypeHandle_82A74334` | Static init caches XML type → `dword_82A74334`. |
| `0x82906928` | `FM2_XmlStaticInit_CacheTypeHandle_82A74730` | Static init caches XML type → `dword_82A74730`. |
| `0x829073d8` | `FM2_XmlStaticInit_CacheTypeHandle_82A7462C` | Static init caches XML type → `dword_82A7462C`. |
| `0x82907cd8` | `FM2_XmlStaticInit_CacheTypeHandle_82A745A8` | Static init caches XML type → `dword_82A745A8`. |
| `0x829097e0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74B34` | Static init caches XML type → `dword_82A74B34`. |


### Manual re-pass 26 (35 functions)

Final batch-4 placeholders (offset 910+): XML static init + CRT atexit dtors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8290a908` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A0C` | Static init caches XML type → `dword_82A74A0C`. |
| `0x8290aa70` | `FM2_XmlStaticInit_CacheTypeHandle_82A74C80` | Static init caches XML type → `dword_82A74C80`. |
| `0x8290b568` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A3C` | Static init caches XML type → `dword_82A74A3C`. |
| `0x8290b688` | `FM2_XmlStaticInit_CacheTypeHandle_82A74C8C` | Static init caches XML type → `dword_82A74C8C`. |
| `0x8290c258` | `FM2_XmlStaticInit_CacheTypeHandle_82A74A58` | Static init caches XML type → `dword_82A74A58`. |
| `0x8290de80` | `FM2_XmlStaticInit_CacheTypeHandle_82A74E20` | Static init caches XML type → `dword_82A74E20`. |
| `0x8290e8a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74DE4` | Static init caches XML type → `dword_82A74DE4`. |
| `0x82910598` | `FM2_XmlStaticInit_CacheTypeHandle_82A74FB8` | Static init caches XML type → `dword_82A74FB8`. |
| `0x82910af0` | `FM2_XmlStaticInit_CacheTypeHandle_82A74F90` | Static init caches XML type → `dword_82A74F90`. |
| `0x82910e20` | `FM2_XmlStaticInit_CacheTypeHandle_82A75238` | Static init caches XML type → `dword_82A75238`. |
| `0x82911180` | `FM2_XmlStaticInit_CacheTypeHandle_82A75120` | Static init caches XML type → `dword_82A75120`. |
| `0x82911570` | `FM2_XmlStaticInit_CacheTypeHandle_82A75218` | Static init caches XML type → `dword_82A75218`. |
| `0x82911a38` | `FM2_XmlStaticInit_CacheTypeHandle_82A75194` | Static init caches XML type → `dword_82A75194`. |
| `0x82912800` | `FM2_XmlStaticInit_CacheTypeHandle_82A75230` | Static init caches XML type → `dword_82A75230`. |
| `0x82913928` | `FM2_XmlStaticInit_CacheTypeHandle_82A75090` | Static init caches XML type → `dword_82A75090`. |
| `0x82915d30` | `FM2_XmlStaticInit_CacheTypeHandle_82A75458` | Static init caches XML type → `dword_82A75458`. |
| `0x82915fb8` | `FM2_XmlStaticInit_CacheTypeHandle_82A754F0` | Static init caches XML type → `dword_82A754F0`. |
| `0x82917ef0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75524` | Static init caches XML type → `dword_82A75524`. |
| `0x82919b18` | `FM2_XmlStaticInit_CacheTypeHandle_82A75B14` | Static init caches XML type → `dword_82A75B14`. |
| `0x8291a070` | `FM2_XmlStaticInit_CacheTypeHandle_82A758B0` | Static init caches XML type → `dword_82A758B0`. |
| `0x8291a220` | `FM2_XmlStaticInit_CacheTypeHandle_82A758F8` | Static init caches XML type → `dword_82A758F8`. |
| `0x8291a388` | `FM2_XmlStaticInit_CacheTypeHandle_82A75AF4` | Static init caches XML type → `dword_82A75AF4`. |
| `0x8291b468` | `FM2_XmlStaticInit_CacheTypeHandle_82A7589C` | Static init caches XML type → `dword_82A7589C`. |
| `0x8291b540` | `FM2_XmlStaticInit_CacheTypeHandle_82A758AC` | Static init caches XML type → `dword_82A758AC`. |
| `0x8291c500` | `FM2_XmlStaticInit_CacheTypeHandle_82A75974` | Static init caches XML type → `dword_82A75974`. |
| `0x8291d5a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75DF0` | Static init caches XML type → `dword_82A75DF0`. |
| `0x8291e1b8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F10` | Static init caches XML type → `dword_82A75F10`. |
| `0x8291e4d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F68` | Static init caches XML type → `dword_82A75F68`. |
| `0x8291ef38` | `FM2_XmlStaticInit_CacheTypeHandle_82A75C94` | Static init caches XML type → `dword_82A75C94`. |
| `0x8291efc8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75CA0` | Static init caches XML type → `dword_82A75CA0`. |
| `0x8291fd00` | `FM2_XmlStaticInit_CacheTypeHandle_82A75C00` | Static init caches XML type → `dword_82A75C00`. |
| `0x829202e8` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F30` | Static init caches XML type → `dword_82A75F30`. |
| `0x82921028` | `FM2_XmlStaticInit_CacheTypeHandle_82A76318` | Static init caches XML type → `dword_82A76318`. |
| `0x82921850` | `FM2_XmlStaticInit_CacheTypeHandle_82A76118` | Static init caches XML type → `dword_82A76118`. |
| `0x82922300` | `FM2_XmlStaticInit_CacheTypeHandle_82A760E4` | Static init caches XML type → `dword_82A760E4`. |

### Manual re-pass 27 (33 functions)

Final batch-4 placeholders (offset 910+): XML static init + CRT atexit dtors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82922780` | `FM2_XmlStaticInit_CacheTypeHandle_82A75F74` | Static init caches XML type → `dword_82A75F74`. |
| `0x82926910` | `FM2_XmlStaticInit_CacheTypeHandle_82A765AC` | Static init caches XML type → `dword_82A765AC`. |
| `0x82928338` | `FM2_XmlStaticInit_CacheTypeHandle_82A76660` | Static init caches XML type → `dword_82A76660`. |
| `0x82929300` | `FM2_XmlStaticInit_CacheTypeHandle_82A76A04` | Static init caches XML type → `dword_82A76A04`. |
| `0x82929930` | `FM2_XmlStaticInit_CacheTypeHandle_82A7690C` | Static init caches XML type → `dword_82A7690C`. |
| `0x82929bb8` | `FM2_XmlStaticInit_CacheTypeHandle_82A769BC` | Static init caches XML type → `dword_82A769BC`. |
| `0x8292a668` | `FM2_XmlStaticInit_CacheTypeHandle_82A769F4` | Static init caches XML type → `dword_82A769F4`. |
| `0x8292a7d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A76950` | Static init caches XML type → `dword_82A76950`. |
| `0x8292ba60` | `FM2_XmlStaticInit_CacheTypeHandle_82A767FC` | Static init caches XML type → `dword_82A767FC`. |
| `0x8292c2d0` | `FM2_XmlStaticInit_CacheTypeHandle_82A768E4` | Static init caches XML type → `dword_82A768E4`. |
| `0x8292e720` | `FM2_XmlStaticInit_CacheTypeHandle_82A76DB4` | Static init caches XML type → `dword_82A76DB4`. |
| `0x829304a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A76DE0` | Static init caches XML type → `dword_82A76DE0`. |
| `0x8293a920` | `FM2_XmlStaticInit_CacheTypeHandle_82A771BC` | Static init caches XML type → `dword_82A771BC`. |
| `0x8293b070` | `FM2_XmlStaticInit_CacheTypeHandle_82A76E6C` | Static init caches XML type → `dword_82A76E6C`. |
| `0x8293c108` | `FM2_XmlStaticInit_CacheTypeHandle_82A77054` | Static init caches XML type → `dword_82A77054`. |
| `0x8293cc18` | `FM2_XmlStaticInit_CacheTypeHandle_82A77480` | Static init caches XML type → `dword_82A77480`. |
| `0x8293e0a0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77400` | Static init caches XML type → `dword_82A77400`. |
| `0x8293f570` | `FM2_XmlStaticInit_CacheTypeHandle_82A772F8` | Static init caches XML type → `dword_82A772F8`. |
| `0x8293fde0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77468` | Static init caches XML type → `dword_82A77468`. |
| `0x82941660` | `FM2_XmlStaticInit_CacheTypeHandle_82A7784C` | Static init caches XML type → `dword_82A7784C`. |
| `0x82942bc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A77758` | Static init caches XML type → `dword_82A77758`. |
| `0x82943dc0` | `FM2_XmlStaticInit_CacheTypeHandle_82A7767C` | Static init caches XML type → `dword_82A7767C`. |
| `0x82944240` | `FM2_XmlStaticInit_CacheTypeHandle_82A77638` | Static init caches XML type → `dword_82A77638`. |
| `0x82945568` | `FM2_XmlStaticInit_CacheTypeHandle_82A77C64` | Static init caches XML type → `dword_82A77C64`. |
| `0x82946018` | `FM2_XmlStaticInit_CacheTypeHandle_82A779F0` | Static init caches XML type → `dword_82A779F0`. |
| `0x829460a8` | `FM2_XmlStaticInit_CacheTypeHandle_82A77BBC` | Static init caches XML type → `dword_82A77BBC`. |
| `0x82946378` | `FM2_XmlStaticInit_CacheTypeHandle_82A77B04` | Static init caches XML type → `dword_82A77B04`. |
| `0x82946b58` | `FM2_XmlStaticInit_CacheTypeHandle_82A77D0C` | Static init caches XML type → `dword_82A77D0C`. |
| `0x82948268` | `FM2_XmlStaticInit_CacheTypeHandle_82A77D74` | Static init caches XML type → `dword_82A77D74`. |
| `0x829495d0` | `FM2_Crt_AtexitDtor_Sub822A7C00_829D8228` | CRT atexit dtor thunk: `sub_822A7C00(&unk_829D8228)` frees block, clears fields, re-inits string. |
| `0x8294b6d8` | `FM2_Crt_AtexitDtor_Sub825A0430_829F2E90` | CRT atexit dtor thunk: `sub_825A0430(&unk_829F2E90)` frees small block + clears triple. |
| `0x8294bdd0` | `FM2_Crt_AtexitDtor_Sub825A0430_82A00C6C` | CRT atexit dtor thunk: `sub_825A0430(&unk_82A00C6C)` frees small block + clears triple. |
| `0x8294d228` | `FM2_Crt_AtexitFreeSmallBlock_82A0BA44` | CRT atexit: `FM2_Memory_FreeSmallBlockOrNull(dword_82A0BA44)` then zeroes `82A0BA44..4C`. |

---

## Post batch-4: high-traffic `sub_` infrastructure

Target list: `scripts/ida_fm2_list_unnamed_sub_callees.py` → `.cursor/hooks/state/unnamed-sub-callees.json` (sorted by FM2 caller count).

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
