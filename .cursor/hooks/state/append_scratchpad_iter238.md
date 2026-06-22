## Iteration 238

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82619470 | sub_82619470 | FM2_Lua_NumpunctFacet_DestroyBody | 0.90 | Frees numpunct facet string buffers; resets vtable to `off_820423C0`. |
| 0x826194C8 | sub_826194C8 | FM2_Lua_NumpunctFacet_Delete | 0.91 | Numpunct vtable slot 9; calls destroy body then optional `FM2_Memory_FreeSmallBlockOrNull`. |
| 0x82619518 | sub_82619518 | FM2_Stl_String_CtorFromNestedCStrPtr | 0.85 | Copies C string from `*(a2+8)`; facet/locale string field ctor helper. |
| 0x826195C0 | sub_826195C0 | FM2_Memory_GetOrInsertFrameAllocMapValue | 0.91 | Frame-alloc map lookup/insert at `a1+88`; used when exporting presentation `xmlref` attrs. |
| 0x8261B170 | sub_8261B170 | FM2_Stl_Ostream_WriteLocaleFormattedNumericString | 0.90 | Core locale-aware numeric string writer; called from numpunct `FormatDouble`/`FormatLongDouble`. |
| 0x8261BF38 | sub_8261BF38 | FM2_BindingScript_PresentationLoader_Dtor | 0.92 | Destroys presentation loader (`off_8210F028`); frees XML tree, hash lists, strings, font cache. |
| 0x8261CA08 | sub_8261CA08 | FM2_Stl_Ostream_WriteInt | 0.90 | Numpunct facet vtable `+24` with `int` value; variant export integer path. |
| 0x826206E8 | sub_826206E8 | FM2_Lua_SetStackTopRelative | 0.92 | Thin wrapper: `FM2_Lua_SetStackTop(*a1, -1 - a2)` from binding init/finalize paths. |
| 0x826308E0 | sub_826308E0 | FM2_BindingScript_ClearDeferredPresentationXmlBuffers | 0.89 | Frees per-slot deferred XML pointer vectors and clears circular buffer at `a1+48`. |
| 0x82630970 | sub_82630970 | FM2_BindingScript_DestroyDeferredPresentationXmlTree | 0.90 | Clears deferred buffers then frees root XML vector at `a1+36`. |
| 0x827D2EE8 | sub_827D2EE8 | FM2_XgfSerializer_Ctor | 0.91 | Calls `PresentationLoader_Ctor` then sets `CForzaXGFSerializer` vftable and extra fields. |
| 0x827D2F48 | sub_827D2F48 | FM2_BindingScript_PresentationLoader_Delete | 0.90 | Deletes extra string field then `PresentationLoader_Dtor`; optional heap free. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82619468 | sub_82619468 | Single-byte accessor; only SQLite xref — low confidence / incidental. |
| 0x827D47C8 | sub_827D47C8 | Large XGF presentation import; defer dedicated pass. |
| 0x827D76C8 | sub_827D76C8 | XML child navigator helper; defer with `0x827Dxxxx` cluster. |
