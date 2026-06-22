## Iteration 232

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826183A8 | sub_826183A8 | FM2_Lua_CtypeFacet_TolowerRange | 0.92 | Vtable slot 1 at `off_8210EF54`; applies `Tolower` per byte over `[a2,a3)` using char-class table at `a1+8`. |
| 0x826183F0 | sub_826183F0 | FM2_Lua_CtypeFacet_ToupperChar | 0.91 | Vtable slot 4; single-char `Toupper` using facet char-class state. |
| 0x82618420 | sub_82618420 | FM2_Lua_CtypeFacet_ToupperRange | 0.92 | Vtable slot 3; applies `Toupper` per byte over `[a2,a3)`. |
| 0x82618468 | sub_82618468 | FM2_Lua_CtypeFacet_DoNarrowIdentity | 0.88 | Vtable slots 6/9; identity `do_narrow` returning input char unchanged. |
| 0x82618488 | sub_82618488 | FM2_Lua_CtypeFacet_WidenRangeMemcpy | 0.90 | Vtable slot 7; `do_widen` range copy via `memcpy_s` without translation. |
| 0x826184F0 | sub_826184F0 | FM2_Lua_CtypeFacet_MbrtowcMemcpy | 0.89 | Vtable slot 10; locale conversion helper copying narrow bytes with `memcpy_s` bounds check. |
| 0x82617B08 | sub_82617B08 | FM2_Stl_RuntimeErrorBase_Dtor | 0.90 | Vtable slot 0 at `off_8210EF2C`; calls `FM2_Stl_RuntimeError_DestroyBody`; optional free. |
| 0x82618E30 | sub_82618E30 | FM2_Lua_GetOrCreateBindingScriptLexerForThread | 0.91 | Thread-local CS; returns cached `dword_82A06A78` or creates lexer singleton and registers via `sub_8275F120`. |
| 0x826187A0 | sub_826187A0 | FM2_Stl_IosBase_InitBindingScriptStream | 0.90 | Inits ios fields/tie state; allocates ref-counted locale holder via `sub_8275F390`; used by streambuf open path. |
| 0x82619868 | sub_82619868 | FM2_LocaleStreambuf_ImbueLocale | 0.90 | Resolves thread lexer locale; calls locale facet vtable `+24` with open-mode mask (e.g. 32). |
| 0x82619B90 | sub_82619B90 | FM2_LocaleStreambuf_OpenInitFacet | 0.91 | Streambuf open: ios init, imbue locale, sets fail bit if no locale; optional `FM2_LocaleFacet_RegisterInThreadLocalTable`. |
| 0x82619B08 | sub_82619B08 | FM2_LocaleStreambuf_AssignFromSourceStream | 0.89 | Assigns from source via `FM2_LocaleStreambuf_AssignFromAndLockFacet`; copies ios fail state; may sync via `sub_826199B8`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper (8 bytes). |
| 0x826199B8 | sub_826199B8 | Propagates eof/fail ios state from tied stream; defer ios cluster. |
| 0x8261B0F0 | sub_8261B0F0 | Embedded iostream ctor chaining streambuf open; defer with `8261CC70`. |
| 0x8261CC70 | sub_8261CC70 | Wide iostream ctor wiring locale streambuf vtables. |
| 0x82618178 | sub_82618178 | Extracts string from streambuf get/put areas; defer stream string helpers. |
| 0x82618818 | sub_82618818 | Splits binding script path into dir/base strings; defer presentation loader cluster. |
| 0x8261D6E0 | sub_8261D6E0 | Large presentation XML import dispatcher; defer dedicated pass. |
| 0x826195C0 | sub_826195C0 | Frame-alloc map insert-or-lookup for binding int vectors. |
| 0x8261DB18 | sub_8261DB18 | Variant-to-string serializer for binding property types. |
| 0x8275F120 | sub_8275F120 | Registers lexer in global facet chain `dword_82A4389C`. |
| 0x8275F390 | sub_8275F390 | Lazy global `"C"` locale singleton allocator. |
