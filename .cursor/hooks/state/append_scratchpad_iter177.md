## Iteration 177

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8242BC80 | sub_8242BC80 | FM2_BinaryStream_ReadOrWriteByteByEndianFlag | 0.90 | Vtable `+48` endian flag; `+32`/`+36` read/write 1 byte; 29 vtable xrefs. |
| 0x8242BD70 | sub_8242BD70 | FM2_BinaryStream_ReadOrWriteQwordByEndianFlag | 0.90 | Same endian dispatch pattern for 8-byte fields; 43 vtable xrefs. |
| 0x8242BEC8 | sub_8242BEC8 | FM2_BinaryStream_ReadOrWriteSimd8xDwordByEndianFlag | 0.89 | Endian-dispatched read/write of eight 4-byte fields (32 bytes); 15 vtable xrefs. |
| 0x824CDD60 | sub_824CDD60 | FM2_XmlReader_GetChildAttributeDwordOrError | 0.91 | `GetChildElementByName`; parse dword or `"XML Attribute not found"` error; 33 callers. |
| 0x824CCEE8 | sub_824CCEE8 | FM2_XmlReader_ParseAttributeDwordOrDefault | 0.90 | `sscanf(attr, "%u")` on child `+4` string; returns default on miss. |
| 0x827D7918 | sub_827D7918 | FM2_Render_ElementNodeDtor | 0.91 | Element vtable `off_82157BBC`; unlinks parent; releases two child refs; frees `+36` buffer. |
| 0x827DD648 | sub_827DD648 | FM2_Render_UnlinkElementFromParentQueue | 0.88 | Removes element `+8` id from parent queue vector at `+548`; swap-pop; called from element dtor. |
| 0x82659378 | sub_82659378 | FM2_FMOD_HeapAllocDelayLineScoped | 0.92 | `fmod_dsp_delay_line_register/unregister`; `HeapAllocMaybeZero`; `fmod_eventi.cpp:108-110`. |
| 0x82632DE8 | sub_82632DE8 | FM2_Lua_ToluaRegisterConstAndTypeMetatables | 0.91 | Builds `"const "` + name; registers const and type metatables via `PushMetatableWithGcAndProps`. |
| 0x826365C8 | sub_826365C8 | FM2_Lua_ToluaSetupConstMetatableMetamethods | 0.92 | Sets `__index`/`__newindex`/`__add`…/`__gc` with `tolua_gc_event`; const metatable setup. |
| 0x822DC950 | sub_822DC950 | FM2_Lua_GetUserInterfaceDisplayStringMethod | 0.91 | Resolves `"UserInterface::DisplayString"` userdata method or raises bad-arg; 69 callers. |
| 0x827806C0 | sub_827806C0 | FM2_ProtocolQueue_InitWithLock | 0.90 | Vtable near `"ProtocolQueue.m_pConnection"`; stores lock at `+4`; calls refcount helper; 46 callers. |

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
| 0x82780768 | sub_82780768 | 64B refcount increment on `ProtocolQueue`; defer with queue cluster. |
| 0x82788F98 | sub_82788F98 | 60B wrapper → `sub_827897C0` only. |
| 0x827897C0 | sub_827897C0 | 60B wrapper → `sub_82789868` only. |
| 0x827893A8 | sub_827893A8 | 44B `Outptr_WriteZero` only; too thin alone. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate to `sub_827AF060`; defer render base cluster. |
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → `sub_82334768` vector push. |
| 0x8235F258 | sub_8235F258 | Event-binding vector emplace; defer with `8235EBB8`/`8235F110` cluster. |
| 0x8235EBB8 | sub_8235EBB8 | 28-byte-record vector insert; defer event-binding cluster. |
| 0x822BB210 | sub_822BB210 | ProfileLua binding init; defer profile/Lua cluster. |
| 0x82504E20 | sub_82504E20 | `lstrcmpA` 20-byte entry lookup; defer Win32 string-table cluster. |
