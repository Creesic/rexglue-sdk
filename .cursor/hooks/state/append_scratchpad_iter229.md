## Iteration 229

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826171B8 | sub_826171B8 | FM2_HashNamePropertyList_FindLowerBoundByDwordKey | 0.91 | RB-tree lower_bound walk on dword key at node `+12`; sentinel flag at `+17`; used by `sub_825C6D98` iterator init. |
| 0x826176F0 | sub_826176F0 | FM2_HashNamePropertyList_FindUpperBoundByDwordKey | 0.91 | RB-tree upper_bound walk on dword key at `+12`; sentinel flag at `+45`; feeds iterator init helper. |
| 0x82617748 | sub_82617748 | FM2_HashNamePropertyList_AllocSentinelNode24 | 0.90 | Allocates 24-byte sentinel node via `stl_vector_20byte_allocate_n_elements`; sets header flags `+16=1`, `+17=0`. |
| 0x82617810 | sub_82617810 | FM2_HashNamePropertyList_DestroySubtreePostOrder | 0.90 | Post-order recursive free while node flag `+21` clear; used by `FM2_HashNamePropertyList_EraseNode`. |
| 0x826178C0 | sub_826178C0 | FM2_HashNamePropertyList_InitIteratorAtUpperBound | 0.90 | Builds `{tree,upper_bound_node}` iterator via upper-bound helper; used by insert-hint logic. |
| 0x82617D88 | sub_82617D88 | FM2_HashNamePropertyList_InitInsertIteratorHint | 0.89 | Combines upper-bound iterator with key compare at `node+12` to choose insert-before/after hint pair. |
| 0x82617E28 | sub_82617E28 | FM2_HashNamePropertyList_LookupPropertyNameByKey | 0.91 | Finds hash-name entry in list at `a1+84` by dword key; returns property name C-string pointer via insert-hint walk. |
| 0x82617C28 | sub_82617C28 | FM2_BindingScript_ImportXmlPropertyTreeRecursive | 0.92 | Recursively walks XML/lua children; calls lexer vtable name getter; links units or registers profile nodes; recurses into subtree. |
| 0x82617D00 | sub_82617D00 | FM2_BindingScript_WalkXmlChildrenInvokeNameGetter | 0.88 | Iterates XML child nodes; invokes binding-script lexer vtable `+20` to populate temp string per child. |
| 0x826167B0 | sub_826167B0 | FM2_BindingScript_Streambuf_ClearBufferAreas | 0.91 | Frees owned buffer when flag bit0 set; zeroes six get/put area pointers; clears owned-buffer flag. |
| 0x82616908 | sub_82616908 | FM2_BindingScript_Streambuf_DestroyBody | 0.90 | Sets base vtable `off_8210EEB8`; releases ref-counted locale facet at `+14`; frees owned locale ptr. |
| 0x826166E0 | sub_826166E0 | FM2_RefCountedPtr_ReleaseAndMaybeFree | 0.90 | Decrements refcount; invokes destructor vtable when zero; optional `FM2_Memory_FreeSmallBlockOrNull` on wrapper. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82616678 | sub_82616678 | Nested streambuf vtable offset fixup (`off_8210EEB0/EEA8/EEAC`); defer multi-level dtor cluster. |
| 0x8275F698 | sub_8275F698 | Locale facet refcount table `byte_82A438FC`; defer facet teardown cluster. |
| 0x8275F750 | sub_8275F750 | Thin free of streambuf owned locale pointer after `FM2_Noop`. |
| 0x82617028 | sub_82617028 | Streambuf dtor vtable slot 0; defer with nested dtor helpers. |
| 0x826173D8 | sub_826173D8 | Multi-level streambuf dtor calling clear/destroy/vtable fixup chain. |
| 0x82617870 | sub_82617870 | Deep nested streambuf dtor wrapper; defer dtor cluster. |
| 0x82616E98 | sub_82616E98 | Locale facet nested dtor at offset -4; thin wrapper. |
| 0x82616F10 | sub_82616F10 | Locale facet nested dtor at offset -8; thin wrapper. |
| 0x82616FD8 | sub_82616FD8 | Combines vtable fixup + facet dtor; defer cluster. |
| 0x825C6D98 | sub_825C6D98 | Iterator init wrapper using lower_bound; defer with `826178C0` lower-bound twin. |
| 0x82617AB8 | sub_82617AB8 | STL runtime-error object dtor; defer exception-object cluster. |
| 0x82617BD0 | sub_82617BD0 | Runtime-error object dtor with optional free; defer exception cluster. |
