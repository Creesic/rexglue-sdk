### Infrastructure pass 38 (33 functions)

Lua compiler/runtime helpers, ref-counted strings, compression insert, circular buffer erase.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824bf370` | `FM2_Lua_AllocStringTableEntry` | Evidence from decompile and caller context. |
| `0x824bf6d8` | `FM2_Lua_AllocCClosureWithUpvalues` | Evidence from decompile and caller context. |
| `0x824be770` | `FM2_Lua_GrowProtoConstantsTable` | Evidence from decompile and caller context. |
| `0x824be810` | `FM2_Lua_InitProtoWithUpvalues` | Evidence from decompile and caller context. |
| `0x824bf190` | `FM2_Lua_ProtectedCallSetupFrame` | Evidence from decompile and caller context. |
| `0x824bec90` | `FM2_Lua_PushLoadedClosureUpvalues` | Evidence from decompile and caller context. |
| `0x824be3b0` | `FM2_Lua_FindTableSlotForValue` | Evidence from decompile and caller context. |
| `0x824bea18` | `FM2_Lua_GetProtoConstantSlot` | Evidence from decompile and caller context. |
| `0x824bc890` | `FM2_Lua_GetTableFieldCopySlots` | Evidence from decompile and caller context. |
| `0x824bcc38` | `FM2_Lua_TryGetTableFieldViaUpvalue` | Evidence from decompile and caller context. |
| `0x824bc558` | `FM2_Lua_TypeErrorForConcat` | Evidence from decompile and caller context. |
| `0x824bbed0` | `FM2_Lua_ErrorFormatAndThrow` | Evidence from decompile and caller context. |
| `0x824bbbe8` | `FM2_Lua_CheckForInfiniteProtoChain` | Evidence from decompile and caller context. |
| `0x824bbde0` | `FM2_Lua_ResolveCallTargetProto` | Evidence from decompile and caller context. |
| `0x824b8a90` | `FM2_Lua_CallHookOrTraceback` | Evidence from decompile and caller context. |
| `0x824b8b80` | `FM2_Lua_AdjustStackForVarargs` | Evidence from decompile and caller context. |
| `0x824b8d10` | `FM2_Lua_TypeErrorOnCallValue` | Evidence from decompile and caller context. |
| `0x824b8de0` | `FM2_Lua_EnterProtectedCallFrame` | Evidence from decompile and caller context. |
| `0x824bfbf0` | `FM2_LuaSyntax_InitLexerFromReader` | Evidence from decompile and caller context. |
| `0x824bfcc8` | `FM2_LuaIO_InitFileHandleState` | Evidence from decompile and caller context. |
| `0x824c3908` | `FM2_LuaSyntax_GetTokenName` | Evidence from decompile and caller context. |
| `0x824c3990` | `FM2_LuaSyntax_ExpectedTokenNear` | Evidence from decompile and caller context. |
| `0x824c4c50` | `FM2_LuaSyntax_SaveLookaheadToken` | Evidence from decompile and caller context. |
| `0x824c2f70` | `FM2_LuaSyntax_ErrorOnPrecompiledChunk` | Evidence from decompile and caller context. |
| `0x82457b98` | `FM2_Network_RbTreeLowerBoundByMessageKeyDword` | Evidence from decompile and caller context. |
| `0x82466080` | `FM2_AIOvertake_CopyVector128ToOutput` | Evidence from decompile and caller context. |
| `0x824ca820` | `FM2_Stl_RefCountedString_DecRefOrFree` | Evidence from decompile and caller context. |
| `0x824ca898` | `FM2_Stl_RefCountedString_AssignRef` | Evidence from decompile and caller context. |
| `0x824ca960` | `FM2_Stl_RefCountedString_MoveInsertRange` | Evidence from decompile and caller context. |
| `0x82454ad8` | `FM2_CompressionStream_InsertNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x827d8658` | `FM2_CircularBuffer_EraseRangeWrapper` | Evidence from decompile and caller context. |
| `0x824b8598` | `FM2_Lua_ParseLoadStringFormatSpec` | Evidence from decompile and caller context. |
| `0x824b9030` | `FM2_Lua_LoadStringOrFileChunk` | Evidence from decompile and caller context. |