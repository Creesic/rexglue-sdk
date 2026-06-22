### Infrastructure pass 42 (33 functions)

Memory alloc helpers, network timed messages, Lua GC mark/lexer/parser, lap tracker spline, async sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8259f340` | `FM2_Memory_AllocArray8Checked` | Evidence from decompile and caller context. |
| `0x8220c198` | `FM2_Stl_String_ResizeAndNullTerminate` | Evidence from decompile and caller context. |
| `0x82205ef0` | `FM2_Lua_GetDefaultComPtrCtorArgs` | Evidence from decompile and caller context. |
| `0x82205ba0` | `FM2_Lua_BindingPairVector_ReserveCapacity` | Evidence from decompile and caller context. |
| `0x82454318` | `FM2_Network_MemmovePayloadRange8` | Evidence from decompile and caller context. |
| `0x82457b30` | `FM2_Network_BuildTimedMessageListFromRange` | Evidence from decompile and caller context. |
| `0x824576a8` | `FM2_Network_AllocTimedMessageNodeFromTemplate` | Evidence from decompile and caller context. |
| `0x82435488` | `FM2_AsyncOp_HeapifyUpContentRecord` | Evidence from decompile and caller context. |
| `0x82435530` | `FM2_AsyncOp_PartitionIntroSortRange` | Evidence from decompile and caller context. |
| `0x824364c0` | `FM2_ContentVector_DestroyRangeAndTrim` | Evidence from decompile and caller context. |
| `0x8242f6f0` | `FM2_AsyncQueue_IncRefAndMaybeCloseHandle` | Evidence from decompile and caller context. |
| `0x82461ad0` | `FM2_BufferedFileRead_HashBufferWithXeCryptSha` | Evidence from decompile and caller context. |
| `0x82464de0` | `FM2_LapTracker_ComputeSplineSegmentBounds` | Evidence from decompile and caller context. |
| `0x82464e60` | `FM2_LapTracker_CompareTrackProgressFlags` | Evidence from decompile and caller context. |
| `0x82480580` | `FM2_AIDriver_WrapSectorIndexForward` | Evidence from decompile and caller context. |
| `0x82480fb0` | `FM2_AIDriver_ReconcileRaceLineSectorState` | Evidence from decompile and caller context. |
| `0x824ba348` | `FM2_Lua_MarkStackObjectsDuringTraverse` | Evidence from decompile and caller context. |
| `0x824ba5b8` | `FM2_Lua_MarkTableUpvaluesDuringTraverse` | Evidence from decompile and caller context. |
| `0x824ba9e0` | `FM2_Lua_TraverseOpenUpvalueChain` | Evidence from decompile and caller context. |
| `0x824bab50` | `FM2_Lua_CollectGrayObjectsFromList` | Evidence from decompile and caller context. |
| `0x824badd8` | `FM2_Lua_MarkGrayObjectGraphRecursive` | Evidence from decompile and caller context. |
| `0x824bb348` | `FM2_Lua_LinkUpvalueToOpenList` | Evidence from decompile and caller context. |
| `0x824befd8` | `FM2_Lua_CreateClosureFromHashSlot` | Evidence from decompile and caller context. |
| `0x824bf308` | `FM2_Lua_LookupOrCreateClosureSlot` | Evidence from decompile and caller context. |
| `0x824bf738` | `FM2_Lua_AllocProtoWithConstants` | Evidence from decompile and caller context. |
| `0x824bf7b8` | `FM2_Lua_AllocUpvalueDescTable` | Evidence from decompile and caller context. |
| `0x824bfb90` | `FM2_Lua_FindUpvalueIndexInProto` | Evidence from decompile and caller context. |
| `0x824bfce8` | `FM2_LuaSyntax_ReadBytesFromLexer` | Evidence from decompile and caller context. |
| `0x824c0c50` | `FM2_LuaSyntax_PushParserStateFrame` | Evidence from decompile and caller context. |
| `0x824c2db8` | `FM2_LuaSyntax_ParseChunkStatements` | Evidence from decompile and caller context. |
| `0x824c3488` | `FM2_LuaSyntax_ParseFuncOrStatList` | Evidence from decompile and caller context. |
| `0x824c37e0` | `FM2_LuaSyntax_AppendLexemeToBuffer` | Evidence from decompile and caller context. |
| `0x82454d08` | `FM2_Network_EraseMessageTreeNodeRebalance` | Evidence from decompile and caller context. |