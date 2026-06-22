## Iteration 208

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826203F8 | sub_826203F8 | FM2_Lua_InvokeProtectedBindingEventWithVariants | 0.90 | Registers error handler; pushes binding context + variant vector; `FM2_Lua_ProtectedCallWithTraceback`; on success captures up to 4 return variants; on error appends via loader callback. |
| 0x826208D8 | sub_826208D8 | FM2_Lua_PushBindingVariantVectorOntoStack | 0.91 | Iterates 16-byte binding-variant vector `[begin,end)`; `FM2_Lua_PushValueFromBindingVariant` per element; callee of protected event invoke. |
| 0x82610E48 | sub_82610E48 | FM2_Lua_InvokeBindingEventAndCollectReturnStrings | 0.89 | Calls protected event invoke; foreach return slot gets metatable `tostring` and appends to STL string; error path assigns loader error string. |
| 0x8260FAC0 | sub_8260FAC0 | FM2_Lua_RaiseBindingErrorByMessageIndex | 0.91 | `FM2_Lua_GetBindingErrorMessageByIndex` → `sub_8261F990`/`FM2_Lua_ErrorVprintf`; 12 binding validation callers. |
| 0x8254DDD0 | sub_8254DDD0 | FM2_LuaIO_CopyReaderBufferChunkToOutput | 0.90 | Lua reader callback; copies `a2[1]` bytes from `*a2` to `*a3`; clears remaining count; used by `FM2_LuaIO_LoadScriptBufferWithMode`. |
| 0x826202C0 | sub_826202C0 | FM2_Lua_AppendBindingEventErrorAndInvokeLoaderCallback | 0.89 | Appends error C string to loader `std::string` at `a1+9`; invokes optional callback at `(*a1)+44`. |
| 0x82620340 | sub_82620340 | FM2_Lua_GetBindingVariantVectorElemPtrAt | 0.90 | Bounds-checked index into 16-byte-element vector; `stl_throw_invalid_vector_subscript` on OOB; used storing event return variants. |
| 0x82620990 | sub_82620990 | FM2_Lua_InitBindingSlotWithNilValue | 0.90 | Binding-slot ctor: type tag `0`, pushes nil, records stack depth; 2 Lua binding callers. |
| 0x827E0650 | sub_827E0650 | FM2_Profile_IsMaterialPassFlagBitSet | 0.89 | Non-negative pass: tests `(1<<pass) & dword_+20`; negative pass: sorted dword table or category-byte bit test; gates pass-10 dispatch. |
| 0x827E06F0 | sub_827E06F0 | FM2_Render_ClearMaterialPassFlagAndReleaseVariant | 0.88 | Clears pass flag bit at material `+16`; resolves variant payload; `sub_827EAB38` release; used before cache invalidation walk. |
| 0x827D77D8 | sub_827D77D8 | FM2_Render_RecursivelyInvalidateMaterialPassSubtree | 0.89 | If pass enabled calls `FM2_Render_ResourceCache_ForEachMatchingEntry`; recurses child material nodes at pass slot `a3`; self-recursive on child ids. |
| 0x827D78E8 | sub_827D78E8 | FM2_Render_InvalidateMaterialPassByNegativeSlotId | 0.90 | `FM2_Render_LookupMaterialPassFlagByteByNegativeId` then `FM2_Render_RecursivelyInvalidateMaterialPassSubtree`; 15 material-update callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261F990 | sub_8261F990 | Thin wrapper → `FM2_Lua_ErrorVprintf` only. |
| 0x82611DC0 | sub_82611DC0 | Generic 16-byte vector grow/append helper. |
| 0x82620A48 | sub_82620A48 | Defer pairing with nil-slot init (`InitBindingSlotWithNumberValue`). |
| 0x827EAB38 | sub_827EAB38 | Defer with variant-release cluster (`sub_827E06F0` callee). |
| 0x827DD170 | sub_827DD170 | 3-way dispatch to `sub_827EF3D0`; needs unit-string link naming. |
| 0x827EF3D0 | sub_827EF3D0 | Unit-string table chain linker; defer with render subtree cluster. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
| 0x8261F988 | sub_8261F988 | Thin `FM2_Lua_GetStackDepth` wrapper. |
| 0x827DD108 | sub_827DD108 | 8B deref `return *a2` only. |
