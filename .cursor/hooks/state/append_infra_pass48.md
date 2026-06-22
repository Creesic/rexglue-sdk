### Infrastructure pass 48 (33 functions)

Lua syntax tail (for-loop/lex/codegen), XML reader attr parse, network RB-tree due messages, Lua binding sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c58f8` | `FM2_LuaSyntax_DischargeToRegAndEmitUnaryOrBinary` | Evidence from decompile and caller context. |
| `0x824c21d0` | `FM2_LuaSyntax_InitForLoopHiddenLocals` | Evidence from decompile and caller context. |
| `0x824c3f78` | `FM2_LuaSyntax_LexQuotedStringContinue` | Evidence from decompile and caller context. |
| `0x824c6540` | `FM2_LuaSyntax_EmitBinaryOpWithStringCoalesce` | Evidence from decompile and caller context. |
| `0x824c6640` | `FM2_LuaSyntax_EmitBinaryOpBetweenRegs` | Evidence from decompile and caller context. |
| `0x824c4cc0` | `FM2_LuaSyntax_LexTokenViaNumberOrString` | Evidence from decompile and caller context. |
| `0x824c06e0` | `FM2_LuaSyntax_AddLocalOrUpvalueToProto` | Evidence from decompile and caller context. |
| `0x824c0ae8` | `FM2_LuaSyntax_RegisterFuncLocalInProto` | Evidence from decompile and caller context. |
| `0x824c1008` | `FM2_LuaSyntax_ParseSimpleExprAtom` | Evidence from decompile and caller context. |
| `0x824c1388` | `FM2_LuaSyntax_ParseFunctionParamList` | Evidence from decompile and caller context. |
| `0x824c42a8` | `FM2_LuaSyntax_LexLongBracketString` | Evidence from decompile and caller context. |
| `0x824ccd00` | `FM2_XmlReader_IsWhitespaceChar` | Evidence from decompile and caller context. |
| `0x824cd4c0` | `FM2_XmlReader_GetNodeRecordAtIndex` | Evidence from decompile and caller context. |
| `0x824cec68` | `FM2_XmlReader_AppendAttrVectorEntry` | Evidence from decompile and caller context. |
| `0x824cee60` | `FM2_XmlReader_ParseElementAttributes` | Evidence from decompile and caller context. |
| `0x824ced18` | `FM2_XmlReader_InsertNameAttrNode` | Evidence from decompile and caller context. |
| `0x821d7940` | `FM2_BufFile_InvokeWriterFlushVtable` | Evidence from decompile and caller context. |
| `0x82414080` | `FM2_LuaSyntax_RoundDoubleForStringConcat` | Evidence from decompile and caller context. |
| `0x824b80b0` | `FM2_LuaSyntax_ComputeOpcodeSizeClass` | Evidence from decompile and caller context. |
| `0x82457890` | `FM2_Network_RbTreeLowerBoundByDueTime` | Evidence from decompile and caller context. |
| `0x82457ca8` | `FM2_Network_RbTreeCollectDueMessages` | Evidence from decompile and caller context. |
| `0x824e7f20` | `FM2_Network_RbTreeSuccessorFromNode` | Evidence from decompile and caller context. |
| `0x824eb200` | `FM2_Lua_BindingPairIntrosortPartition` | Evidence from decompile and caller context. |
| `0x824eb2a0` | `FM2_Lua_BindingPairHeapSortDown` | Evidence from decompile and caller context. |
| `0x824eb6b8` | `FM2_Lua_BindingPathCompareCaseInsensitive` | Evidence from decompile and caller context. |
| `0x824ebae0` | `FM2_Lua_BindingPairInsertionSortTail` | Evidence from decompile and caller context. |
| `0x824d3408` | `FM2_Config_LookupTokenByIndex` | Evidence from decompile and caller context. |
| `0x824d3530` | `FM2_ProfileTuning_AssignWideString` | Evidence from decompile and caller context. |
| `0x824de6c8` | `FM2_LuaLeaderboard_TestClipDownloadFlag` | Evidence from decompile and caller context. |
| `0x824d5da8` | `FM2_Lua_PushSslUnitStringsTable` | Evidence from decompile and caller context. |
| `0x82434408` | `FM2_Config_WriteDelimitedTokenAtIndex` | Evidence from decompile and caller context. |
| `0x8240c380` | `FM2_Profile_OpenContentCreateExOrError` | Evidence from decompile and caller context. |
| `0x824e5300` | `FM2_RenderAdapter_InitPresentationVtables` | Evidence from decompile and caller context. |