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
