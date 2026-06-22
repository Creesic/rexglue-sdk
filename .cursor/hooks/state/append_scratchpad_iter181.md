## Iteration 181

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825B5768 | sub_825B5768 | FM2_Render_ResolveShaderPassKeywordCacheEntry | 0.91 | Cached dword vector lookup; `MatchShaderPassKeyword`; `render_pass_decode_material_slot_descriptor`; 50 callers. |
| 0x8245FFA8 | sub_8245FFA8 | FM2_HashName_AssignFromSlashPath | 0.92 | Splits path on `"/"`; recursive property-bag walk; `HashName_AssignFromPropertyNode`; 48 callers. |
| 0x825D3E48 | sub_825D3E48 | FM2_Render_AllocStateChangePairFromPool | 0.90 | 12-byte pool node; stores two dwords; `NotifyManagerStateChange`; 30 callers. |
| 0x82776F10 | sub_82776F10 | FM2_Render_InitSliceHandleFromSource | 0.90 | Slice-handle vtable `off_82144D30`; copies notifier from source; invokes callback; 27 callers. |
| 0x82768BE8 | sub_82768BE8 | FM2_NetworkMessage_SendScoped | 0.89 | Stack message context init/send/teardown; thread assert; 27 network callers. |
| 0x82790A60 | sub_82790A60 | FM2_Render_AdvanceElementSliceOffset | 0.91 | Bounds-checks parent buffer range; adds delta to offset at `+8`; trap on overflow. |
| 0x82790AF8 | sub_82790AF8 | FM2_Render_AssignElementWithSliceOffsetAdjust | 0.90 | `AssignElementBaseFrom` + `AdvanceElementSliceOffset` + reassign; 31 vtable callers. |
| 0x822C98B8 | sub_822C98B8 | FM2_Lua_SetUserInterfaceUserStringOnBinding | 0.90 | Pushes `"UserInterface::UserString"` userdata env; pops into binding slot; 26 callers. |
| 0x823931F8 | sub_823931F8 | FM2_Image_ResampleKernel_DtorFreeTaggedBuffers | 0.91 | Image resampler vtable `off_82028738`; frees tagged buffers at `+56`/`+88`/`+92`. |
| 0x8239D7D8 | sub_8239D7D8 | FM2_Image_ResampleKernel_DtorMaybeFree | 0.90 | Calls resampler dtor; optional tagged free of object; 34 vtable xrefs. |
| 0x827AD1D8 | sub_827AD1D8 | FM2_Stl_DwordVector_GetCount | 0.92 | Returns `(end-begin)>>2` or 0; 27 callers across render/network. |
| 0x824E6408 | sub_824E6408 | FM2_Xam_ReadProfileSettingsOverlappedFetch | 0.91 | `XUserReadProfileSettings` size query + overlapped fetch; friends context on `997`; 29 callers. |

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
| 0x827DDA98 | sub_827DDA98 | 24B vector push guard; too thin alone. |
| 0x824F0160 | sub_824F0160 | 8-byte jump thunk only. |
| 0x827897C0 | sub_827897C0 | 60B wrapper → `InitElementSliceView` only. |
| 0x827893A8 | sub_827893A8 | 44B `Outptr_WriteZero` only; too thin alone. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate to `sub_827AF060`; defer render base cluster. |
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → `sub_82334768` vector push. |
| 0x827EB600 | sub_827EB600 | 12B thunk only. |
| 0x8277CCF8 | sub_8277CCF8 | 16B field accessor only. |
| 0x82785028 | sub_82785028 | 16B field accessor only. |
| 0x82785038 | sub_82785038 | 16B field accessor only. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x82798528 | sub_82798528 | Multi-vtable render dtor; defer slice-handle cluster. |
| 0x82782848 | sub_82782848 | 52B wrapper → `AdvanceElementSliceOffset` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x827DE060 | sub_827DE060 | 60B thin wrapper → `ElementLinkVector_PushCopy12`. |
| 0x8254E230 | sub_8254E230 | Lua type-check helper; defer next Lua cluster. |
