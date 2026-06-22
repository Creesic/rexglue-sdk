## Iteration 176

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82633BE8 | sub_82633BE8 | FM2_Lua_ToluaGetUserdataPointerFromStack | 0.91 | Returns full userdata ptr or lightuserdata dword; used by `ToluaCastBinding`. |
| 0x826593F8 | sub_826593F8 | FM2_FMOD_Dsp_ReverbProcessDelayLineScoped | 0.92 | Registers delay-line slot; `ReverbProcessDelayLine`; unregisters; `fmod_eventi.cpp:126-128`. |
| 0x827D79F8 | sub_827D79F8 | FM2_Render_InitElementNode | 0.90 | Vtable `off_82157BBC` (`"element"`); links parent via `sub_827DDA98`; 77 xrefs. |
| 0x827D79A8 | sub_827D79A8 | FM2_Render_ElementNodeDtorMaybeFree | 0.89 | Calls element dtor `sub_827D7918`; optional `Memory_FreeSmallBlockOrNull`. |
| 0x8242BCF8 | sub_8242BCF8 | FM2_BinaryStream_ReadOrWriteDwordByEndianFlag | 0.90 | Vtable `+48` endian flag; `+32`/`+36` read/write 4 bytes; 58 vtable xrefs. |
| 0x8242BE50 | sub_8242BE50 | FM2_BinaryStream_ReadOrWriteWordByEndianFlag | 0.90 | Same endian dispatch pattern for 2-byte fields. |
| 0x824CDFB0 | sub_824CDFB0 | FM2_XmlReader_GetChildElementAtIndex | 0.91 | Walks 24-byte sibling nodes by index from `+12` child link; 62 callers. |
| 0x824CDC08 | sub_824CDC08 | FM2_XmlReader_GetChildElementValueDwordOrDefault | 0.90 | `GetChildElementByName`; returns child `+4` dword or default. |
| 0x827BE870 | sub_827BE870 | FM2_Stl_RedBlackTree_RotateLeft | 0.91 | Classic left rotation on `+8`/`+4` child links; 59 callers in STL insert. |
| 0x827BE808 | sub_827BE808 | FM2_Stl_RedBlackTree_RotateRight | 0.91 | Classic right rotation mirror of rotate-left. |
| 0x827929C0 | sub_827929C0 | FM2_Render_CopyElementBaseFrom | 0.89 | Copies fields `+4`/`+8` from source element after base init. |
| 0x826361B0 | sub_826361B0 | FM2_Lua_ToluaGcEventCallback | 0.92 | `tolua_gc_event` handler; `.collector` field; clears ubox entry on GC. |

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
| 0x827D7918 | sub_827D7918 | Element node dtor body; defer with `ElementNodeDtorMaybeFree` cluster. |
| 0x827DDA98 | sub_827DDA98 | 24B vector push guard; too thin alone. |
| 0x824F0160 | sub_824F0160 | 8-byte jump thunk only. |
