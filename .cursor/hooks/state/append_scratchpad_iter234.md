## Iteration 234

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82619108 | sub_82619108 | FM2_Stl_Ostream_PutByteOrSetFail | 0.91 | Ostream sentry put-byte; calls `FM2_Stl_Streambuf_PutByteOrBuffer`; sets fail flag at `+4` on -1. |
| 0x82619218 | sub_82619218 | FM2_Stl_Ostream_WriteByteRange | 0.90 | Writes `[a4,a4+a5)` via repeated `FM2_Stl_Ostream_PutByteOrSetFail`; returns updated sentry/ios state triple. |
| 0x826192E0 | sub_826192E0 | FM2_Stl_Ostream_WriteCStringWithDelimiter | 0.91 | Scans C-string with `memchr` for `\\0` segments; writes each via byte-range helper; optional delimiter byte between segments. |
| 0x82619280 | sub_82619280 | FM2_Stl_Ostream_WriteFillByteCount | 0.89 | Writes same byte `a5` times through ostream sentry; returns updated state triple. |
| 0x82618F38 | sub_82618F38 | FM2_BindingScript_BuildFloatPrintfFormatSpec | 0.92 | Builds printf spec (`%`, optional `+`/`#`, `.*`, `f`/`e`/`g`) from ios format flags `a4`. |
| 0x82618FD0 | sub_82618FD0 | FM2_BindingScript_BuildIntegerPrintfFormatSpec | 0.92 | Builds integer printf spec (`%`, `+`/`#`, length modifier, `d`/`o`/`x`/`I64`); used by variant serializers. |
| 0x826193E0 | sub_826193E0 | FM2_Lua_MoneypunctFacet_InitWithCLocale | 0.91 | Moneypunct facet ctor `off_8210EFEC`; loads `"C"` locale; initializes separators/bool names via `sub_82619160`. |
| 0x82619950 | sub_82619950 | FM2_Lua_GetOrCreateMoneypunctFacetSingleton | 0.90 | Lazy-allocates 24-byte moneypunct facet; calls `FM2_Lua_MoneypunctFacet_InitWithCLocale`; returns 4. |
| 0x82619C10 | sub_82619C10 | FM2_Lua_GetOrCreateNumpunctFacetForThread | 0.90 | Thread-local CS; caches numpunct facet in `dword_82A06A7C`; mirrors `FM2_Lua_GetOrCreateBindingScriptLexerForThread` pattern. |
| 0x82619CB0 | sub_82619CB0 | FM2_Lua_GetOrCreateMoneypunctFacetForThread | 0.90 | Thread-local CS; caches moneypunct facet in `dword_82A06A80`; registers in global lexer chain. |
| 0x8261AE68 | sub_8261AE68 | FM2_Stl_Ostream_FlushAndUnlockFacet | 0.89 | If tied stream needs flush, propagates fail; then unlocks facet critical section via `sub_8275F790`. |
| 0x8275F1D8 | sub_8275F1D8 | FM2_Locale_NameFacet_Ctor | 0.90 | Locale name facet ctor `off_8213F848`; sets category id 1; initializes name string to `"*"`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper (8 bytes). |
| 0x8275F790 | sub_8275F790 | Thin jump thunk to `RtlLeaveCriticalSection`. |
| 0x82618BA8 | sub_82618BA8 | Large `std::string` insert-fill-at-offset; defer string iterator cluster. |
| 0x82619798 | sub_82619798 | Inserts char via fill helper then refreshes iterator; defer with `82618BA8`. |
| 0x82619A70 | sub_82619A70 | String iterator push-back char with growth fallback. |
| 0x82619080 | sub_82619080 | Pool alloc copy of C string for moneypunct facet fields. |
| 0x82619160 | sub_82619160 | Loads locale decimal/thousands and bool name strings into moneypunct facet. |
| 0x8261C010 | sub_8261C010 | Ios vtable slot 6 binding handler; large defer. |
| 0x8261D3B8 | sub_8261D3B8 | Ios vtable slot 8; large defer. |
| 0x8261E0D0 | sub_8261E0D0 | Ios vtable slot 7 export handler; large defer. |
