## Iteration 233

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826199B8 | sub_826199B8 | FM2_Stl_IosBase_PropagateFailFromTiedStream | 0.90 | If tied streambuf underflow returns -1, sets ios fail bit via `FM2_Stl_IosBase_SetStateFlagsOrThrow`; used after stream assign. |
| 0x8261B0F0 | sub_8261B0F0 | FM2_Stl_Iostream_CtorInitEmbeddedStreambuf | 0.91 | Embedded iostream ctor: sets `unk_8210F018`/`off_8210EEAC` vtables; calls `FM2_LocaleStreambuf_OpenInitFacet`. |
| 0x8261CC70 | sub_8261CC70 | FM2_Stl_WideIostream_CtorInitLocaleStreambuf | 0.90 | Wide iostream ctor chains embedded streambuf ctor + `FM2_LocaleStreambuf_CtorInitFacet`; sets `off_8210EEB0`. |
| 0x8275F120 | sub_8275F120 | FM2_Lua_RegisterBindingScriptLexerInGlobalChain | 0.89 | Prepends 8-byte node `{next,lexer}` onto global facet chain `dword_82A4389C`; called after lexer creation. |
| 0x8275F390 | sub_8275F390 | FM2_Locale_GetOrCreateGlobalCLocale | 0.91 | Lazy-allocates 52-byte global locale with name `"C"` at `dword_82A438A0`; used by ios/stream init paths. |
| 0x82618178 | sub_82618178 | FM2_BindingScript_Streambuf_ExtractStringContents | 0.91 | Reads put/get buffer ranges from streambuf flag bits into `std::string` via `FM2_Stl_String_AppendCStr`. |
| 0x82618818 | sub_82618818 | FM2_BindingScript_SetPresentationPathAndDirectory | 0.90 | Stores full presentation path at `a1+104`; splits on last `\\` into directory field `a1+132` with trailing slash. |
| 0x8261D6E0 | sub_8261D6E0 | FM2_BindingScript_LoadPresentationXmlV2 | 0.93 | Ios vtable slot 5 (`off_8210F018`); loads `presentation` XML version 2; dispatches scenegraph/library/logic/animation/etc. importers. |
| 0x826189D8 | sub_826189D8 | FM2_FrameAllocMap_FindOrInsertIteratorByKey | 0.90 | Lower_bound walk on frame-alloc map; returns iterator or inserts via `FM2_FrameAllocMap_InsertOrAssign`. |
| 0x82618AF8 | sub_82618AF8 | FM2_FrameAllocMap_InsertOrAssignIterator | 0.89 | Inserts/assigns frame-alloc map node; returns `{tree,node,inserted}` iterator triple. |
| 0x82618ED0 | sub_82618ED0 | FM2_Lua_NumpunctFacet_InitWithCLocale | 0.91 | Numpunct facet ctor `off_8210EFC8`; loads `"C"` locale facet holder; stores `Getcvt` result at `+8`. |
| 0x826198E8 | sub_826198E8 | FM2_Lua_GetOrCreateNumpunctFacetSingleton | 0.90 | Lazy-allocates 16-byte numpunct facet via `FM2_Lua_NumpunctFacet_InitWithCLocale`; returns 4 on success. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper (8 bytes). |
| 0x82618F38 | sub_82618F38 | Builds float printf format spec string; defer format/ostream cluster. |
| 0x82618BA8 | sub_82618BA8 | Large `std::string` insert-fill helper; defer string iterator cluster. |
| 0x82619108 | sub_82619108 | Ostream put-byte-or-fail; defer with `82619218` write cluster. |
| 0x82619218 | sub_82619218 | Writes byte range to ostream updating sentry state. |
| 0x826192E0 | sub_826192E0 | Ostream write C-string with optional delimiter between null-separated segments. |
| 0x82619798 | sub_82619798 | String insert char then refresh iterator; thin helper. |
| 0x82619C10 | sub_82619C10 | Thread-local numpunct facet getter twin of `82618E30` pattern. |
| 0x82619CB0 | sub_82619CB0 | Thread-local moneypunct-like facet getter; needs `82619950` naming first. |
| 0x82619950 | sub_82619950 | Lazy facet singleton via `sub_826193E0`; defer facet cluster. |
| 0x8261AE68 | sub_8261AE68 | Ostream flush/sync propagating fail and locking facet CS. |
| 0x8275F1D8 | sub_8275F1D8 | Locale name facet ctor with wildcard `"*"` name. |
