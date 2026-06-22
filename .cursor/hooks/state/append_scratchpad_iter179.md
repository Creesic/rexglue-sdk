## Iteration 179

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824D8278 | sub_824D8278 | FM2_Profile_InitUnitFromPoolObject | 0.91 | Vtable `off_82042184` near `"size"`/`"fontmap"`; `"UnitStrings"` keys; `NotifyManagerStateChange`; unit lookup. |
| 0x824D8C00 | sub_824D8C00 | FM2_Profile_AllocUnitComPtrFromPool | 0.90 | 56-byte pool alloc; calls unit init; `ComPtr_ResetAndAssign`; 42 callers. |
| 0x82504EA0 | sub_82504EA0 | FM2_TextureFormatTable_VerifyGpuUploadByteCount | 0.91 | `GetPixelFormatByteSizeFromName` × w×h; compares GPU upload bytes; optional pre-upload hook; 35 callers. |
| 0x827E3A28 | sub_827E3A28 | FM2_Profile_InitMemoryCategoryNode | 0.90 | Vtable `off_82159510` near `"global"`/`"memory"`/`"performance"`; zeros counter qwords; clears vector at `+32`. |
| 0x82785B38 | sub_82785B38 | FM2_Render_InitElementSliceHandleFields | 0.89 | Stores qword handle + byte flag at `+8`; 36 callers paired with slice compare/init. |
| 0x82792650 | sub_82792650 | FM2_Render_AssignElementBaseFrom | 0.88 | Assignment operator wrapping `CopyElementBaseFrom`; returns `this`; 68 vtable xrefs. |
| 0x827A5E18 | sub_827A5E18 | FM2_Render_InitSliceHandleBase | 0.89 | Sets vtable `off_82144D30`; zeros `+4`; clears `+8` byte; 46 callers. |
| 0x827FC2D8 | sub_827FC2D8 | FM2_Render_InitSliceHandleWithNotifier | 0.90 | Same slice-handle vtable; stores notifier at `+4`; invokes callback on `+8` flag; 41 callers. |
| 0x8258AB18 | sub_8258AB18 | FM2_Stl_DwordVector_PopBackAndShrink | 0.89 | `MemmoveS` shrink after pop; decrements end by 4; returns popped dword; 45 callers. |
| 0x82464BF8 | sub_82464BF8 | FM2_Math_NormalizeVec4InPlaceOptional | 0.91 | VMX `vrsqrtefp`/`vnmsubfp` normalize path when flag set; copies xy first; 33 callers. |
| 0x822B8B80 | sub_822B8B80 | FM2_ProfileLua_InvokeBindingProtectedCall | 0.90 | `ProfileLua_InitStackMarker`; copies stack slots; `InvokeProtectedCall32`; 32 callers. |
| 0x82788F98 | sub_82788F98 | FM2_Render_InitElementSliceViewFromSource | 0.89 | Passes source `+12` offset into slice-view init chain; 48 vtable callers. |

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
| 0x827E0440 | sub_827E0440 | Binary search category mapping byte; defer profile lookup cluster. |
| 0x822AE8D8 | sub_822AE8D8 | ProfileLua move stack slot; defer next ProfileLua pass. |
| 0x82633B40 | sub_82633B40 | Lua stack index clamp; defer tolua/Lua cluster. |
| 0x8277CCF8 | sub_8277CCF8 | 16B field accessor only. |
| 0x82785028 | sub_82785028 | 16B field accessor only. |
