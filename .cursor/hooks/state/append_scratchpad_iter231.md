## Iteration 231

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8275F568 | sub_8275F568 | FM2_LocaleFacet_DestroyNotificationChains | 0.91 | Invokes facet notification callbacks at `a1+32`; frees singly-linked chains at `+28` and `+32`; called from `FM2_LocaleFacet_DecRefAndMaybeDestroy`. |
| 0x8275F5F8 | sub_8275F5F8 | FM2_LocaleFacet_RegisterInThreadLocalTable | 0.90 | Under CS lock, finds free slot in `dword_82A438D4[1..7]`; registers facet; increments `byte_82A438FC[id]`. |
| 0x827D7EB8 | sub_827D7EB8 | FM2_RbTree24_FindUpperBoundByDwordKey | 0.90 | Upper_bound walk on 24-byte RB-tree nodes; dword key at `+12`; sentinel flag at `+17`; paired with lower-bound init. |
| 0x825C6DF8 | sub_825C6DF8 | FM2_RbTree24_InitIteratorAtUpperBound | 0.90 | Builds `{tree,upper_bound_node}` iterator via `FM2_RbTree24_FindUpperBoundByDwordKey`. |
| 0x827D8088 | sub_827D8088 | FM2_RbTree24_BuildEqualRangeIteratorPair | 0.91 | Combines lower/upper bound iterators into four-field equal-range pair; used by profile/hash-name lookup callers. |
| 0x82616838 | sub_82616838 | FM2_LocaleStreambuf_CtorInitFacet | 0.89 | Sets locale-streambuf vtables `unk_8210F054`/`off_8210EEA8`; optionally calls facet registration on ctor. |
| 0x826168B8 | sub_826168B8 | FM2_LocaleStreambuf_AssignFromAndLockFacet | 0.88 | Copies streambuf pointer; locks source facet critical section at embedded offset `+40`. |
| 0x82617AB8 | sub_82617AB8 | FM2_Stl_RuntimeError_DestroyBody | 0.90 | Destroys `runtime_error` message string; restores base vtable `off_82000DF8`. |
| 0x82617BD0 | sub_82617BD0 | FM2_Stl_RuntimeError_Dtor | 0.90 | Vtable slot 0 at `off_8210EF38`; calls destroy body; optional free. |
| 0x82617EF0 | sub_82617EF0 | FM2_Stl_RuntimeError_CtorFromMessageString | 0.91 | Constructs `runtime_error` with default `"unknown"` then copies message string; sets vtable `off_8210EF38`. |
| 0x82618540 | sub_82618540 | FM2_Lua_CtypeFacet_DestroyBody | 0.90 | Binding-script `ctype` facet dtor body: frees char-class table via `FM2_Crt_Free`/`FM2_Memory_FreeSmallBlockOrNull`; restores `off_820423C0`. |
| 0x826185A8 | sub_826185A8 | FM2_Lua_CtypeFacet_Dtor | 0.90 | Vtable slot 0 at `off_8210EF54`; calls ctype facet destroy body; optional free. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper (8 bytes). |
| 0x82617B08 | sub_82617B08 | Thin runtime-error dtor wrapper calling destroy body. |
| 0x826183A8 | sub_826183A8 | `ctype` facet `Tolower` range; defer remaining vtable slots. |
| 0x826183F0 | sub_826183F0 | `ctype` facet `Toupper` single char (vtable+4). |
| 0x82618420 | sub_82618420 | `ctype` facet `Toupper` range (vtable+3). |
| 0x82618468 | sub_82618468 | Identity `do_narrow` stub returning input unchanged. |
| 0x82618488 | sub_82618488 | `ctype` facet `widen` range via `memcpy_s`. |
| 0x826184F0 | sub_826184F0 | Locale `codecvt`/`mbrtowc` style narrow copy helper. |
| 0x82619B90 | sub_82619B90 | Locale-streambuf open/init with facet register; defer iostream cluster. |
| 0x82619B08 | sub_82619B08 | Streambuf assign wrapper calling `FM2_LocaleStreambuf_AssignFromAndLockFacet`. |
| 0x8261CC70 | sub_8261CC70 | Wide streambuf ctor chaining locale facet init; defer iostream cluster. |
