## Iteration 180

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827E0440 | sub_827E0440 | FM2_Profile_LookupSortedCategoryByteById | 0.90 | Binary search sorted dword table at `+348`; returns mapping byte at `+508`; negative id input; 37 profile callers. |
| 0x82633B40 | sub_82633B40 | FM2_Lua_ClampStackIndexOrDefault | 0.91 | Returns index if `GetStackDepth` ≥ `abs(index)` else default; 31 tolua callers. |
| 0x822AE8D8 | sub_822AE8D8 | FM2_ProfileLua_MoveStackSlotToBinding | 0.89 | `CopyStackSlotToTop` then `PopStackSlot` into binding index; 33 ProfileLua callers. |
| 0x824DABF8 | sub_824DABF8 | FM2_Profile_NotifyUnitStringStateChange | 0.90 | `"UnitStrings"` key from unit table index; `NotifyManagerStateChange`; 30 callers. |
| 0x827E3A90 | sub_827E3A90 | FM2_Profile_MergeMemoryCategoryFromSource | 0.89 | Merges bitmask counters and 48-byte records from source category node; 39 memory-profiler callers. |
| 0x827EAA80 | sub_827EAA80 | FM2_Profile_SetVariantTypeAndAlloc | 0.90 | Switches variant type; type 14 allocates 28-byte string object; frees prior payload; 35 callers. |
| 0x82465F50 | sub_82465F50 | FM2_Physics_ComputeWrappedAngleDelta | 0.89 | Wraps index delta to ±half period then `(delta+from)-to`; 31 physics callers. |
| 0x8278B3D0 | sub_8278B3D0 | FM2_NetworkMessage_SendIfPriority2 | 0.90 | `HasPriority2` gate; builds message and sends; else `0x804B0005`; thread assert; 30 callers. |
| 0x824516A8 | sub_824516A8 | FM2_Zlib_InputStream_ReadByte | 0.91 | Zlib inflater byte reader with refill via `sub_82453130`; EOF returns 255; 27 callers. |
| 0x823934F8 | sub_823934F8 | FM2_Image_ResampleKernel_ProcessTexelScoped | 0.90 | Temporarily rewires resampler fields; `ResampleKernel_ApplyTexelTransform`; vtable process hooks; 33 vtable xrefs. |
| 0x822080B0 | sub_822080B0 | FM2_BinaryStream_ReadOrWriteFloatXmlAttribute | 0.91 | Read: `CXMLNode::GetNumericValue`; write: builds `attr="value"` string; endian flag at `+48`; 27 callers. |
| 0x8245AE98 | sub_8245AE98 | FM2_HashName_InitIntPropertyNode | 0.89 | Vtable `off_8203CEA4`; `HashName_AssignPropertyByTypeId(7)`; stores int at `+8`; 30 codegen callers. |

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
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x82790AF8 | sub_82790AF8 | Render assign-with-transform; defer with `82782848` cluster. |
| 0x82798528 | sub_82798528 | Multi-vtable render dtor; defer slice-handle cluster. |
