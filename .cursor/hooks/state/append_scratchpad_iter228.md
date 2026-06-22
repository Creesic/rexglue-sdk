## Iteration 228

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8275EC78 | sub_8275EC78 | FM2_Lua_BuildLocaleCharClassTableState | 0.90 | Builds 4-field locale state: facet id, locale name, calloc'd 512-byte char-class copy from `FM2_Lua_GetCharClassTable`, ownership flag. |
| 0x826158C0 | sub_826158C0 | FM2_Lua_CopyLocaleCharClassTableState | 0.89 | Copies locale char-class state from build helper into caller buffer; used by binding script lexer init. |
| 0x82615AE8 | sub_82615AE8 | FM2_Lua_InitBindingScriptLexerCharClassState | 0.90 | Stores copied char-class fields into binding-script lexer object at `+2..+5`; called from singleton ctor. |
| 0x826185F8 | sub_826185F8 | FM2_Lua_GetOrCreateBindingScriptLexerSingleton | 0.91 | Lazy-allocates 24-byte lexer (`off_8210EF54`); inits `"C"` locale facet; wires char-class state. |
| 0x82618290 | sub_82618290 | FM2_Lua_InitLocaleFacetHolderUnderCritSec | 0.89 | Locks thread-local CS; clears four locale strings; calls `sub_8275F498`; throws on null locale name. |
| 0x82617B58 | sub_82617B58 | FM2_Lua_DestroyLocaleFacetHolderAndUnlock | 0.88 | Destroys four locale strings under facet holder; leaves critical section. |
| 0x826170F8 | sub_826170F8 | FM2_BindingScript_Streambuf_InitBase | 0.91 | Inits `off_8210EEB8` streambuf: mutex, owned-buffer flag, six get/put area pointer slots, ref-counted locale ptr. |
| 0x82617980 | sub_82617980 | FM2_BindingScript_Streambuf_CtorWithOpenMode | 0.90 | Calls streambuf base init; sets vtable `off_8210EEF0`; maps open-mode bits into `a1[16]` flags. |
| 0x82616970 | sub_82616970 | FM2_BindingScript_Streambuf_UnderflowGetChar | 0.92 | Vtable slot 5 (`off_8210EEF0`); underflow then decrements get-area count and returns next byte. |
| 0x82617208 | sub_82617208 | FM2_BindingScript_Streambuf_ReadChars | 0.92 | Vtable slot 7; `xsgetn`-style read via buffered get-area or virtual underflow fallback. |
| 0x826172F0 | sub_826172F0 | FM2_BindingScript_Streambuf_WriteChars | 0.92 | Vtable slot 8; `xsputn`-style write via buffered put-area or virtual overflow per char. |
| 0x82617440 | sub_82617440 | FM2_BindingScript_Streambuf_PutCharGrowBuffer | 0.93 | Vtable slot 1; grows put buffer when full; writes char; updates get/put pointer triples and owned-buffer flag. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826171B8 | sub_826171B8 | Hash-name property RB-tree lower_bound; defer with `826176F0` cluster. |
| 0x826176F0 | sub_826176F0 | Hash-name property RB-tree upper_bound; defer insert/erase cluster. |
| 0x82617748 | sub_82617748 | Allocates 24-byte RB-tree sentinel node; defer tree cluster. |
| 0x82617810 | sub_82617810 | Post-order destroy of hash-name property tree nodes. |
| 0x826178C0 | sub_826178C0 | Init iterator at lower_bound; defer with `82617D88`. |
| 0x82617C28 | sub_82617C28 | Recursive binding XML property import; needs vtable+20 naming first. |
| 0x82617D00 | sub_82617D00 | Walks XML children invoking lexer vtable name getter. |
| 0x82617D88 | sub_82617D88 | Insert-hint iterator pair for hash-name property list. |
| 0x82617E28 | sub_82617E28 | Lookup property name string by key in hash-name list. |
| 0x82616678 | sub_82616678 | Nested streambuf vtable offset fixup; defer dtor cluster. |
| 0x826166E0 | sub_826166E0 | Refcount release dtor; defer locale-facet cleanup cluster. |
| 0x826167B0 | sub_826167B0 | Clears streambuf buffer pointer areas; defer dtor cluster. |
| 0x82616908 | sub_82616908 | Streambuf destroy body; defer with `8275F750`. |
| 0x8275F698 | sub_8275F698 | Locale facet dec-ref; defer facet cluster. |
| 0x8275F750 | sub_8275F750 | Frees streambuf owned locale ptr after noop. |
