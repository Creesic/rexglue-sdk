## Iteration 235

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82619080 | sub_82619080 | FM2_Lua_AllocCStringCopy | 0.90 | Measures C-string length; pool-allocates and byte-copies including terminator; used for moneypunct string fields. |
| 0x82619160 | sub_82619160 | FM2_Lua_MoneypunctFacet_LoadCLocaleFields | 0.91 | Loads locale decimal/thousands chars and `"false"`/`"true"` name strings into moneypunct facet via `Getcvt`. |
| 0x82618BA8 | sub_82618BA8 | FM2_Stl_String_InsertFillCharAtOffset | 0.90 | Inserts `a3` fill chars at offset `a2` in SSO string; grows buffer with memmove when capacity insufficient. |
| 0x82619798 | sub_82619798 | FM2_Stl_String_InsertCharAndRefreshIterator | 0.89 | Inserts one char via fill helper at iterator position; refreshes `{ptr,begin,end}` iterator pair. |
| 0x82619A70 | sub_82619A70 | FM2_Stl_StringIterator_PushBackChar | 0.89 | Appends char at string end if capacity; otherwise grows via insert-char-and-refresh path. |
| 0x8261C010 | sub_8261C010 | FM2_BindingScript_ExportUnitStringTableToXml | 0.92 | Ios vtable slot 6; walks unit-string table, builds frame-alloc map, exports each entry via vtable `+8`. |
| 0x8261E0D0 | sub_8261E0D0 | FM2_BindingScript_ExportPresentationNodePropertiesToXml | 0.93 | Ios vtable slot 7; exports node `type`/properties/content-type fields and gator attributes to XML writer. |
| 0x8261D3B8 | sub_8261D3B8 | FM2_BindingScript_ProcessDeferredFontRegistryBindings | 0.90 | Ios vtable slot 8; routes unresolved font-registry keys to deferred circular buffers by content-type id; flushes via vtable `+12`. |
| 0x824DABB0 | sub_824DABB0 | FM2_Lua_NumpunctFacet_Dtor | 0.89 | Numpunct facet dtor; restores base vtable `off_820423C0`; optional free. |
| 0x8261CD08 | sub_8261CD08 | FM2_Lua_NumpunctFacet_FormatIntegerToOstream | 0.91 | Numpunct vtable slot 7; builds integer printf spec; `sprintf_s`; writes via `sub_8261B770`. |
| 0x8261CF28 | sub_8261CF28 | FM2_Lua_NumpunctFacet_FormatDoubleToOstream | 0.92 | Numpunct vtable slot 3; scales double per ios precision/flags; `FM2_BindingScript_BuildFloatPrintfFormatSpec`; writes formatted output. |
| 0x82619D50 | sub_82619D50 | FM2_Profile_SetVariantFromDelimitedString | 0.91 | Parses whitespace/comma-delimited string by binding type id; uploads float/int/uchar vectors via profile variant setters. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper (8 bytes). |
| 0x8275F790 | sub_8275F790 | Thin jump thunk to `RtlLeaveCriticalSection`. |
| 0x8261D0B8 | sub_8261D0B8 | Near-duplicate of `FM2_Lua_NumpunctFacet_FormatDoubleToOstream` (vtable slot 2). |
| 0x8261D248 | sub_8261D248 | Formats pointer as `%p` to ostream; defer numpunct slot cluster. |
| 0x8261CD90 | sub_8261CD90 | Additional numpunct format slot; defer remaining vtable slots. |
| 0x8261CE18 | sub_8261CE18 | Additional numpunct format slot; defer cluster. |
| 0x8261CEA0 | sub_8261CEA0 | Additional numpunct format slot; defer cluster. |
| 0x8261B770 | sub_8261B770 | Core ostream write-with-locale helper; large shared callee. |
| 0x8261C390 | sub_8261C390 | Circular-buffer finalize helper called from deferred binding path. |
