## Iteration 178

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8235F258 | sub_8235F258 | FM2_EventBindingVector_SetRecordAtIndex | 0.90 | Vtable `0x82184918`; ensures capacity then copies 28-byte record at 16-byte index; 130 codegen dispatch callers. |
| 0x8235EBB8 | sub_8235EBB8 | FM2_EventBindingVector_InsertCopyAtEndOrGrow | 0.89 | Vtable `0x821848E8`; 28-byte `MemcpyAligned` insert or grow path; callee of set-record. |
| 0x8235F110 | sub_8235F110 | FM2_EventBindingVector_EnsureCapacityForIndex | 0.88 | Grows 16-byte-record vector to `index+1`; erase/truncate path; called before set-record. |
| 0x82504E20 | sub_82504E20 | FM2_TextureFormatTable_FindEntryByName | 0.91 | `lstrcmpA` linear search over 20-byte name entries; 51 texture/GPU callers. |
| 0x82504CE8 | sub_82504CE8 | FM2_TextureFormatTable_GetPixelFormatByteSizeFromName | 0.92 | Maps `"float"`/`"uint8"`/`"int32"`/`"double"` etc. to byte sizes 1/2/4/8; 7 callers. |
| 0x826592F8 | sub_826592F8 | FM2_FMOD_HeapAllocFromPoolDelayLineScoped | 0.92 | `fmod_dsp_delay_line_register/unregister`; `HeapAllocFromPoolLocked`; `fmod_eventi.cpp:99-101`. |
| 0x82469238 | sub_82469238 | FM2_CMLPArrayInt_DtorMaybeFreeBuffer | 0.91 | `CMLPArray<int>` vftable; frees `+8` buffer when `+5` ownership flag clear; 41 callers. |
| 0x82780768 | sub_82780768 | FM2_ProtocolQueue_IncrementRefAfterUnlock | 0.89 | `LeaveCriticalSectionOrNull` on lock at `+4`; increments refcount at `+8`; called from `InitWithLock`. |
| 0x82789868 | sub_82789868 | FM2_Render_InitElementSliceView | 0.90 | Bounds-checks parent buffer `+12`/`+16`; stores parent ptr and byte offset; 3 callers + vtable. |
| 0x822BB210 | sub_822BB210 | FM2_ProfileLua_InitBindingWithIntArg | 0.90 | `ProfileLua_InitBindingNilMarker`; pushes int stack arg; links marker; 57 callers. |
| 0x822A0318 | sub_822A0318 | FM2_ProfileLua_InitBindingWithCFunction | 0.89 | Init binding marker then `PushCFunction` + `PopStackSlot`; 45 callers. |
| 0x8279F608 | sub_8279F608 | FM2_Render_CompareElementSliceHandles | 0.90 | Traps if buffer ptr mismatch; compares dword offset at `+4`; 40 callers. |

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
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
| 0x82753ED0 | sub_82753ED0 | Jump thunk to `0x8294F0D8` (28B). |
| 0x82758B70 | sub_82758B70 | 24B wrapper → `BuildUrlPathFromComponents(a1,-1,...)`. |
| 0x8275A088 | sub_8275A088 | 8B thunk → `AtofParseFloat` only. |
| 0x8275A30C | sub_8275A30C | 56B unlock trampoline inside `FgetsLocked`; too thin alone. |
| 0x8275B6B8 | sub_8275B6B8 | CRT SEH frame-handler/unwind helper; defer exception cluster. |
| 0x8275C8A8 | sub_8275C8A8 | 8B thunk → `StrSpanIncludingCharsBackward`. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine (~4KB); defer stdio cluster. |
| 0x82792650 | sub_82792650 | 52B wrapper → `CopyElementBaseFrom` only. |
| 0x827DDA98 | sub_827DDA98 | 24B vector push guard; too thin alone. |
| 0x824F0160 | sub_824F0160 | 8-byte jump thunk only. |
| 0x82788F98 | sub_82788F98 | 60B wrapper → `sub_827897C0` only. |
| 0x827897C0 | sub_827897C0 | 60B wrapper → `InitElementSliceView` only. |
| 0x827893A8 | sub_827893A8 | 44B `Outptr_WriteZero` only; too thin alone. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate to `sub_827AF060`; defer render base cluster. |
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → `sub_82334768` vector push. |
| 0x824D8C00 | sub_824D8C00 | Profile unit pool alloc wrapper; defer with `824D8278` cluster. |
| 0x824D8278 | sub_824D8278 | Profile unit init with `UnitStrings`; defer profile cluster. |
| 0x82504EA0 | sub_82504EA0 | Texture upload verify; defer with format-table cluster. |
| 0x827E3A28 | sub_827E3A28 | Init near `"global"`/`"memory"` strings; defer profile/input cluster. |
| 0x827A5E18 | sub_827A5E18 | 52B vtable init only; defer render/input cluster. |
| 0x82785B38 | sub_82785B38 | 52B slice-handle field copy; defer with slice cluster. |
| 0x827EB600 | sub_827EB600 | 12B thunk only. |
