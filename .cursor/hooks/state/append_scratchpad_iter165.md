## Iteration 165

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82747668 | sub_82747668 | FM2_Render_GetShBasisKernelTokenByteSize | 0.91 | Maps SH kernel token IDs (e.g. `0x1A2286`, `0x2C83A4`) to dump stride 4/8/12/16; `__trap` on unknown; callee of `AppendShBasisKernelDumpEntry`. |
| 0x82747778 | sub_82747778 | FM2_Render_LookupShBasisKernelTokenByOrder | 0.90 | Dispatches order 0–17 via `byte_82120650` jump table at `0x827477AC`; returns token ID in r3; caller `BuildShBasisKernelDumpTable`. |
| 0x82747880 | sub_82747880 | FM2_Render_AppendShBasisKernelDumpEntry | 0.92 | Writes 11-byte dump entry (type, token, flags); advances offset via `GetShBasisKernelTokenByteSize`; 8 callers in SH dump builder. |
| 0x82747540 | sub_82747540 | FM2_Render_InitShBasisKernelLayoutFromFlags | 0.90 | Parses SH basis flag word; computes per-channel kernel offsets using `dword_82120630`; first step of `BuildShBasisKernelDumpTable`. |
| 0x8274E4C8 | sub_8274E4C8 | FM2_AtiSSM_IRInstBindVirtualRegisters | 0.91 | `cfg.cpp:649` vReg assert; `VRegInfo::BumpDefs`/`BumpUses`; binds operands via `IRInstBindArgumentOperand`; handles alloc slot types 26/27. |
| 0x82750240 | sub_82750240 | FM2_AtiSSM_LookupVRegTableEntryFromOperandSlots | 0.88 | Copies indexed operand slot fields then `HashTableFindFirstMatchingEntry`; used during IR VReg binding. |
| 0x827508E0 | sub_827508E0 | FM2_AtiSSM_HashTableFindFirstMatchingEntry | 0.89 | Hash-bucket walk with compare callback; returns first matching entry or 0; shared by VReg table lookups. |
| 0x8274E8C8 | sub_8274E8C8 | FM2_AtiSSM_AssertAsmOperandIndexInRange | 0.87 | `asm.cpp:2894` assert when index >7; otherwise `bctr` jump-dispatch to encode handlers; 4 encode callers. |
| 0x8274F858 | sub_8274F858 | FM2_AtiSSM_EncodeShaderAsmAluOperandPacket | 0.90 | `asm.cpp:2470–2480` modifier asserts; calls `EncodeShaderAsmOperand`; packs ALU dword from `dword_8213B078`; 8 xrefs. |
| 0x82751358 | sub_82751358 | FM2_AtiSSM_AllocAddrIndexedRegisterSetWithId | 0.89 | Wraps `InitAddrIndexedRegisterSet`; assigns vtable `off_8213B5B0`; bumps register-set id at `ctx+388`. |
| 0x827528B8 | sub_827528B8 | FM2_AtiSSM_AllocShaderCompileWorkItem | 0.88 | `AllocWorkItemBody(118)`; sets vtable `off_8213BF38`; links CFG/IR inst; marks operand resolved. |
| 0x82751EF0 | sub_82751EF0 | FM2_AtiSSM_VectorDtor_ResetBumpOnDelete | 0.87 | Vector dtor wrapper; calls inner dtor then `ResetBumpRegionOnRewind` when delete flag set; 8 vtable xrefs. |

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
| 0x827477AC | sub_827477AC | Jump-table stub cluster (18×12B token-id loaders); covered by dispatcher rename. |
| 0x8274E480 | sub_8274E480 | Thin wrapper → `GetOrCreateVRegTableEntry(18)` only. |
| 0x8274EFB0 | sub_8274EFB0 | Large asm modifier encoder (~892B); defer asm encode cluster. |
| 0x82752990 | sub_82752990 | Returns constant 1; vtable predicate stub (8B). |
| 0x82752EF0 | sub_82752EF0 | Bump-alloc wrapper → `sub_82752740`; defer work-item cluster. |
| 0x827511C8 | sub_827511C8 | Inner vector dtor; defer with `VectorDtor_ResetBumpOnDelete`. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
