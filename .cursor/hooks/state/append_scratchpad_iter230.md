## Iteration 230

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825C6D98 | sub_825C6D98 | FM2_HashNamePropertyList_InitIteratorAtLowerBound | 0.90 | Builds `{tree,lower_bound_node}` iterator via `FM2_HashNamePropertyList_FindLowerBoundByDwordKey`; paired with upper-bound init in `sub_827D8088`. |
| 0x82616678 | sub_82616678 | FM2_BindingScript_Streambuf_AdjustNestedVtableOffsets | 0.89 | Restores nested vtables `off_8210EEB0`, `off_8210EEA8`, `off_8210EEAC` using `this` offset arithmetic before facet teardown. |
| 0x82617028 | sub_82617028 | FM2_BindingScript_Streambuf_Dtor | 0.92 | Vtable slot 0 at `off_8210EEF0`; clears buffer areas, destroys body, optional free. |
| 0x826173D8 | sub_826173D8 | FM2_BindingScript_Streambuf_DestroyNestedBody | 0.90 | Nested streambuf teardown: restores `off_8210EF28`, sets `off_8210EEF0`, clear/destroy, then vtable offset fixup. |
| 0x82617870 | sub_82617870 | FM2_BindingScript_Streambuf_DtorWithLocaleFacet | 0.91 | Vtable slot 0 at `off_8210EF28`; calls nested destroy at offset -21; facet dec-ref; optional free. |
| 0x82616F88 | sub_82616F88 | FM2_BindingScript_StreambufBase_Dtor | 0.90 | Vtable slot 0 at `off_8210EEB8`; calls `FM2_BindingScript_Streambuf_DestroyBody`; optional free. |
| 0x82616758 | sub_82616758 | FM2_LocaleFacet_Dtor | 0.89 | Vtable slot 0 at `off_8210EEB4`; calls `FM2_LocaleFacet_DecRefAndMaybeDestroy`; optional free. |
| 0x8275F698 | sub_8275F698 | FM2_LocaleFacet_DecRefAndMaybeDestroy | 0.91 | Decrements `byte_82A438FC[id]`; on zero destroys notification chains and releases ref-counted facet ptr. |
| 0x8275F750 | sub_8275F750 | FM2_BindingScript_Streambuf_FreeOwnedLocalePtr | 0.88 | Called from streambuf destroy body; frees `*a1` owned locale pointer after `FM2_Noop`. |
| 0x82616E98 | sub_82616E98 | FM2_LocaleFacet_RestoreVtableMinus4AndDestroy | 0.88 | At offset -4 restores `off_8210EEA8`; sets facet vtable `off_8210EEB4`; dec-ref destroy. |
| 0x82616F10 | sub_82616F10 | FM2_LocaleFacet_RestoreVtableMinus8AndDestroy | 0.88 | At offset -8 restores `off_8210EEAC`; sets facet vtable `off_8210EEB4`; dec-ref destroy. |
| 0x82616FD8 | sub_82616FD8 | FM2_LocaleFacet_RestoreNestedVtablesAndDestroy | 0.89 | Calls nested vtable fixup at offset -3; facet vtable assign; dec-ref destroy; optional free. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8275F568 | sub_8275F568 | Locale facet notification-chain teardown; defer with register helper. |
| 0x8275F5F8 | sub_8275F5F8 | Registers facet in thread-local table `dword_82A438D4`; defer locale cluster. |
| 0x8275F788 | sub_8275F788 | Thin `RtlEnterCriticalSection` wrapper on facet CS ptr. |
| 0x82617AB8 | sub_82617AB8 | `std::runtime_error` destroy body; defer STL exception cluster. |
| 0x82617BD0 | sub_82617BD0 | `runtime_error` dtor vtable `off_8210EF38` slot 0. |
| 0x82617B08 | sub_82617B08 | Thin wrapper calling runtime-error destroy body. |
| 0x82617EF0 | sub_82617EF0 | Runtime-error ctor from message string with `"unknown"` default. |
| 0x82616838 | sub_82616838 | Locale-streambuf ctor with optional skip-init; defer iostream cluster. |
| 0x826168B8 | sub_826168B8 | Assigns streambuf from source; locks facet critical section. |
| 0x827D8088 | sub_827D8088 | Combines lower/upper bound iterators into range pair; needs twin naming context. |
| 0x825C6DF8 | sub_825C6DF8 | Upper-bound iterator init twin of `825C6D98`; defer paired pass. |
