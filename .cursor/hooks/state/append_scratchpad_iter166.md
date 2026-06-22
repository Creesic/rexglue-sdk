## Iteration 166

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8274EFB0 | sub_8274EFB0 | FM2_AtiSSM_EncodeShaderAsmOperandModifierBits | 0.90 | `asm.cpp:2361–2370`; packs swizzle/mask/zero bits into ALU dword from operand bytes at `+128`; callee of vector/scalar encode paths. |
| 0x8274FC38 | sub_8274FC38 | FM2_AtiSSM_EncodeShaderAsmVectorAluPacket | 0.89 | `asm.cpp:2621` modifier assert; vector ALU path (`0x14000000` opcode tag); calls `EncodeShaderAsmOperandModifierBits`; vtable xref `off_8213BE64`. |
| 0x8274FE80 | sub_8274FE80 | FM2_AtiSSM_EncodeShaderAsmScalarAluPacket | 0.89 | `asm.cpp:2755–2776` pred_sel asserts; scalar ALU encode; calls modifier helper + `CFG::Number`; vtable xref `off_8213BD8C`. |
| 0x82752608 | sub_82752608 | FM2_AtiSSM_MapSwizzleToDestRegIndex | 0.88 | Maps source swizzle index to dest reg (4/7/8/48) using PS compile flag at `ctx+2144`; callee of modifier encoder. |
| 0x82752EF0 | sub_82752EF0 | FM2_AtiSSM_BumpAllocShaderCompileContext960 | 0.90 | `BumpAllocAligned(ctx,964)` then `InitShaderCompileContextWorkItem`; 6 shader-compile ctor xrefs. |
| 0x82752740 | sub_82752740 | FM2_AtiSSM_InitShaderCompileContextWorkItem | 0.89 | `AllocWorkItemBody`; vtable `off_8213BD80`; init operand slots to -1/0; used by bump-alloc wrapper. |
| 0x827511C8 | sub_827511C8 | FM2_AtiSSM_VectorBaseDtor_FreeBumpChildren | 0.88 | Vector base dtor; calls `FreeDeferredBumpChildChain` + `ResetBumpRegionOnRewind`; callee of `VectorDtor_ResetBumpOnDelete`. |
| 0x82750C70 | sub_82750C70 | FM2_AtiSSM_FreeDeferredBumpChildChain | 0.89 | Walks bump-linked child chain at `+4`; resets bump region per node; used by vector base dtor. |
| 0x827520B0 | sub_827520B0 | FM2_AtiSSM_IRInstValidateOperandsAndEmit | 0.91 | `irinst.cpp:136–153` operand/reg asserts; loops dest/src operands; vtable emit callbacks at `+20`/`+24`. |
| 0x82752E00 | sub_82752E00 | FM2_AtiSSM_InitIRInstOpcode135_AddToCfgRootSet | 0.87 | `AllocWorkItemBody(135)`; vtable `off_8213C5A8`; `CFG::AddToRootSet`; flags `0x18` on IR inst. |
| 0x82752D78 | sub_82752D78 | FM2_AtiSSM_InitIRInstOpcode136_AddToCfgRootSet | 0.87 | `AllocWorkItemBody(136)`; vtable `off_8213C530`; `CFG::AddToRootSet`; flags `0x18` on IR inst. |
| 0x82754DB0 | sub_82754DB0 | FM2_Xam_AllocKernelStackWorkContext | 0.88 | `AllocGpuTagged(0xA50)` + `MmCreateKernelStack`; stores callback `sub_82756580`; caller thread setup path. |

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
| 0x827477AC | sub_827477AC | Jump-table stub cluster (18×12B); covered by dispatcher rename. |
| 0x8274E480 | ?SetUpParamGen@CFG@XGRAPHICS@@AAAPAVVRegInfo@2@XZ | Already has meaningful XGRAPHICS demangled name. |
| 0x827527B0 | sub_827527B0 | Init IR opcode 48; defer opcode-specific cluster. |
| 0x82752CF0 | sub_82752CF0 | Generic init by caller opcode; defer with bump wrappers. |
| 0x82752990 | sub_82752990 | Returns constant 1; vtable predicate stub (8B). |
| 0x82753658 | sub_82753658 | Thin scalar-const work-item alloc (76B). |
| 0x82753788 | sub_82753788 | Bump wrapper → `AllocAddrIndexedRegisterSetWithId`; too thin alone. |
| 0x82753D30 | sub_82753D30 | XNet startup shim; defer XAM cluster with session IO. |
| 0x82754028 | sub_82754028 | XAM session create IO (`0xB0010`); defer XAM cluster. |
| 0x827548B0 | sub_827548B0 | XAM session property IO (`0xB0025`); defer XAM cluster. |
| 0x82754D60 | sub_82754D60 | TLS work-context free helper; defer with `AllocKernelStackWorkContext`. |
| 0x82754E90 | sub_82754E90 | Kernel stack teardown; defer XAM work-context cluster. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
