### Infrastructure pass 45 (33 functions)

BufFile/XML reader, Lua compiler optional slots, syntax block parser, Lua GC mark helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c6f10` | `FM2_LuaCompiler_DtorOptionalSlotAt` | Evidence from decompile and caller context. |
| `0x824c7708` | `FM2_LuaCompiler_ClearOptionalSlotVector` | Evidence from decompile and caller context. |
| `0x824cc9b8` | `FM2_BufFile_InsertModuleRefAtIndex` | Evidence from decompile and caller context. |
| `0x824cc320` | `FM2_BufFile_GrowModuleRefVectorCapacity` | Evidence from decompile and caller context. |
| `0x824ccbc0` | `FM2_XmlReader_ComparePathSegmentI` | Evidence from decompile and caller context. |
| `0x824ccc58` | `FM2_XmlReader_ParsePathToBuffer` | Evidence from decompile and caller context. |
| `0x824cd5a8` | `FM2_XmlReader_LoadFloatAttrByName` | Evidence from decompile and caller context. |
| `0x824cd8e8` | `FM2_Lua_GetLibRegFieldByIndex12` | Evidence from decompile and caller context. |
| `0x824cd980` | `FM2_Lua_GetLibRegFieldByIndex16` | Evidence from decompile and caller context. |
| `0x824ce0b8` | `FM2_Lua_GetLibRegFirstFieldIfAny` | Evidence from decompile and caller context. |
| `0x824ce490` | `FM2_XmlReader_InitEmptyAttrNodeA` | Evidence from decompile and caller context. |
| `0x824ce4e8` | `FM2_XmlReader_InitEmptyAttrNodeB` | Evidence from decompile and caller context. |
| `0x824ce540` | `FM2_XmlReader_GrowAttrVectorCapacity` | Evidence from decompile and caller context. |
| `0x824ce658` | `FM2_XmlReader_InsertAttrAtIndex` | Evidence from decompile and caller context. |
| `0x824cf490` | `FM2_XmlReader_ApplyAttrDefaultsFromTable` | Evidence from decompile and caller context. |
| `0x824cff90` | `FM2_BufFile32768_Ctor` | Evidence from decompile and caller context. |
| `0x824d0090` | `FM2_BufFile_SetErrorFlagAndCopyPath` | Evidence from decompile and caller context. |
| `0x824d0f48` | `FM2_Input_SslContext_InitWithVtable` | Evidence from decompile and caller context. |
| `0x824d19c8` | `FM2_Camera_LoadScriptPathFromConfig` | Evidence from decompile and caller context. |
| `0x824d3340` | `FM2_Config_ParseWhitespaceDelimitedTokens` | Evidence from decompile and caller context. |
| `0x824c05c8` | `FM2_LuaSyntax_AllocLocalVarSlot` | Evidence from decompile and caller context. |
| `0x824c1ae8` | `FM2_LuaSyntax_ParseBlockWithScope` | Evidence from decompile and caller context. |
| `0x824bb740` | `FM2_LuaSyntax_ParseFunctionBody` | Evidence from decompile and caller context. |
| `0x8257e190` | `FM2_LuaCompiler_CopyOptionalSlotTailFields` | Evidence from decompile and caller context. |
| `0x824c0d58` | `FM2_LuaSyntax_LoadStringChunkBody` | Evidence from decompile and caller context. |
| `0x824c30c8` | `FM2_LuaSyntax_ParseFuncHeaderAndBody` | Evidence from decompile and caller context. |
| `0x824c3648` | `FM2_LuaSyntax_InitDefaultHeaderTag` | Evidence from decompile and caller context. |
| `0x824c3888` | `FM2_LuaSyntax_InternTokenNameStrings` | Evidence from decompile and caller context. |
| `0x824c3a50` | `FM2_LuaSyntax_RegisterLocalNameSlot` | Evidence from decompile and caller context. |
| `0x824c3aa8` | `FM2_LuaSyntax_LexerNextOnRefCountZero` | Evidence from decompile and caller context. |
| `0x824c3c60` | `FM2_LuaSyntax_LexSingleCharOperator` | Evidence from decompile and caller context. |
| `0x824c3cf0` | `FM2_LuaSyntax_ParseNumberWithLocale` | Evidence from decompile and caller context. |
| `0x824c3df8` | `FM2_LuaSyntax_ParseNumberOrHexLiteral` | Evidence from decompile and caller context. |