## Iteration 164

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8274EA38 | sub_8274EA38 | FM2_AtiSSM_EncodeShaderAsmOperand | 0.91 | XGRAPHICS `asm.cpp:2188`; encodes ALU operand from IR inst via `CFG::Number`; 29+ compiler xrefs. |
| 0x82750630 | sub_82750630 | FM2_DeferredTaskQueue_GrowVectorCopyElements | 0.90 | Doubles vector capacity; `BumpAllocAligned` + `MemcpyAligned` + `ResetBumpRegionOnRewind`; callee of append helpers. |
| 0x82750CD0 | sub_82750CD0 | FM2_AtiSSM_InitAddrIndexedRegisterSet | 0.89 | Constructs AddrIndexedSet in deferred queue; XGRAPHICS AddrIndexedSet ctor xref; sets vtable + bump fields. |
| 0x8274E3F0 | sub_8274E3F0 | FM2_DeferredTaskQueue_InitBumpVectorCapacity2 | 0.90 | Init vector struct (cap=2, size=0) in bump allocator region; used by SSM deferred emitters. |
| 0x82750DD0 | sub_82750DD0 | FM2_AtiSSM_AppendDeferredVectorElement | 0.91 | Calls `GrowVectorCopyElements` then stores element and increments count at `a1+24`; 12 compiler xrefs. |
| 0x82752A28 | sub_82752A28 | FM2_AtiSSM_DeferredObjectDtor | 0.89 | Sets vtable `off_82138958`; calls `ResetBumpRegionOnRewind` on destroy. |
| 0x82752508 | sub_82752508 | FM2_AtiSSM_IRInstDtor_RemoveFromCfgRootSet | 0.90 | IR inst dtor; unlinks from CFG root set via `DoublyLinkedListUnlinkNode` + `VectorRemoveElementAtIndex`. |
| 0x827505D8 | sub_827505D8 | FM2_AtiSSM_DoublyLinkedListUnlinkNode | 0.92 | Splices prev/next pointers (28B node); shared list primitive in SSM compiler. |
| 0x827505F8 | sub_827505F8 | FM2_AtiSSM_VectorRemoveElementAtIndex | 0.91 | Decrements count and `MemcpyAligned` shifts dword array when index in range. |
| 0x827526C0 | sub_827526C0 | FM2_AtiSSM_IRInstInit_AddToCfgRootSet | 0.90 | Sets IR inst flags/fields; calls `XGRAPHICS::CFG::AddToRootSet`; 10 vtable ctor xrefs. |
| 0x82750340 | sub_82750340 | FM2_AtiSSM_GetOrCreateVRegTableEntry | 0.88 | Sets VReg slot fields then `VRegTable::Create` if lookup fails; callee of temp-register alloc. |
| 0x827478F0 | sub_827478F0 | FM2_Render_BuildShBasisKernelDumpTable | 0.90 | Parses SH basis flags; dispatches kernel token IDs via jump table; appends dump entries with `AppendShBasisKernelDumpEntry`. |

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
| 0x82747778 | sub_82747778 | SH kernel dispatch jump table; defer paired with `sub_82747668`/`sub_82747880`. |
| 0x827477AC | sub_827477AC | Returns constant `0x2C83A4` only (12B); defer with dispatch table cluster. |
| 0x82747668 | sub_82747668 | Maps SH kernel token IDs to byte sizes; defer with dump-table cluster. |
| 0x82747880 | sub_82747880 | Appends single SH dump entry; defer with `BuildShBasisKernelDumpTable` pass. |
| 0x8274E480 | sub_8274E480 | Thin wrapper → `GetOrCreateVRegTableEntry(18)`; too thin alone. |
| 0x8274E4C8 | sub_8274E4C8 | Large IR compiler (~908B); needs dedicated pass. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
