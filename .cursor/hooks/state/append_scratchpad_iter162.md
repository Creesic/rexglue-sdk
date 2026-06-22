## Iteration 162

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82749320 | sub_82749320 | FM2_AtiSSM_LogDebugVprintf | 0.94 | ATI SSM debug logger: prefixes `"AtiSSM:"`, optional `ssmdpf.txt` file sink, level table `dword_829A9C18`; 151 xrefs across compiled-shader state compiler. |
| 0x827497E8 | sub_827497E8 | FM2_AtiSSM_GetCompiledStateDword | 0.93 | Bounds-checks `eState < uNumStates` (`Internal_AS.h:309`); returns `ASMap[eState]` dword. |
| 0x82749B80 | sub_82749B80 | FM2_AtiSSM_GetCompiledStateFloat | 0.93 | Same bounds check as dword getter (`Internal_AS.h:338`); returns float from state map. |
| 0x82749858 | sub_82749858 | FM2_AtiSSM_GetArrayStateCompiledIndex | 0.92 | Validates array-state index/count (`Internal_AS.h:634`); computes compiled state index via `GetCompiledStateDword`. |
| 0x82749908 | sub_82749908 | FM2_AtiSSM_AllocRequiredRenderStateStruct | 0.92 | Allocates 16-byte render-state struct via pool callback; fills type/fields; appends to linked list (`compiledshader.cpp:1177`). |
| 0x82749758 | sub_82749758 | FM2_AtiSSM_LinkedListAppendPayload | 0.91 | Allocates list node, stores payload at `node[3]`, inserts via `LinkedListInsertItem` (`linkedlist.cpp:3144`). |
| 0x82749530 | sub_82749530 | FM2_AtiSSM_LinkedListAllocNode | 0.91 | Pops node from mem pool; initializes `head/next/prev` fields (`linkedlist.cpp:73`). |
| 0x82749690 | sub_82749690 | FM2_AtiSSM_LinkedListInsertItem | 0.91 | Inserts item before sentinel; increments list count (`linkedlist.cpp:3105-3107`). |
| 0x82749A80 | sub_82749A80 | FM2_AtiSSM_MemPoolAllocFromFreelist | 0.90 | Freelist allocator from `mempriv.h`; grows pool via `sub_827499C0` when empty. |
| 0x82749F70 | sub_82749F70 | FM2_AtiSSM_FormatFloatForShaderDump | 0.91 | Formats float as `±int.9digits` via `sprintf_0`; used in shader dump emitters. |
| 0x82749228 | sub_82749228 | FM2_AtiSSM_WalkCompiledShaderTokens | 0.89 | Decodes packed shader token bitfields; dispatches to `sub_82748E38`; called from render pass bind paths. |
| 0x82749CA0 | sub_82749CA0 | FM2_DeferredTaskQueue_BumpAllocAligned | 0.92 | 4-byte aligned bump alloc on queue buffer; grows via `sub_8274E858`; caller `FM2_DeferredTaskQueue_AllocWorkItem`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F2628 | sub_826F2628 | Sets single byte on nested parse object; too thin alone. |
| 0x826F6300 | sub_826F6300 | Thin pager callback setter; callee unknown. |
| 0x826F6360 | sub_826F6360 | Thin wrapper → `Pager_SetJournalModeFlags` only. |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826F6440 | sub_826F6440 | Too trivial alone. |
| 0x826F6458 | sub_826F6458 | Too trivial alone. |
| 0x826F72F0 | sub_826F72F0 | Thin wrapper only. |
| 0x826F7310 | sub_826F7310 | Thin wrapper only. |
| 0x826FA1D0 | sub_826FA1D0 | Reads single byte from mem cell; too trivial alone. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x82748E38 | sub_82748E38 | Large shader microcode emitter (~1KB); needs dedicated pass. |
| 0x827464A8 | sub_827464A8 | Large cube-map SH convolution (~4.2KB); defer env-map cluster. |
| 0x82745DC8 | sub_82745DC8 | SH basis scale-by-channel helper; defer with `sub_827464A8`. |
| 0x82745F00 | sub_82745F00 | Large SH visibility matrix (~1.4KB); defer env-map cluster. |
