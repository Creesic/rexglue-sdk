### Infrastructure pass 44 (33 functions)

RB-tree rotate, lap tracker cross product, Lua GC shrink, Lua syntax statement parser cluster.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8258c7f8` | `FM2_RbTree_RotateLeftAtChildSibling` | Evidence from decompile and caller context. |
| `0x8246a3a8` | `FM2_LapTracker_ComputeCrossProductProgressOnSpline` | Evidence from decompile and caller context. |
| `0x822049b0` | `FM2_Memory_AllocArray308Checked` | Evidence from decompile and caller context. |
| `0x82204e30` | `FM2_Lua_BindingPairVector_UninitializedFill308` | Evidence from decompile and caller context. |
| `0x824b9c08` | `FM2_Lua_GrowStackAndShrinkLiveSlots` | Evidence from decompile and caller context. |
| `0x824ba048` | `FM2_Lua_ProcessGrayObjectWorkList` | Evidence from decompile and caller context. |
| `0x824ba880` | `FM2_Lua_MarkProtoUpvalueChain` | Evidence from decompile and caller context. |
| `0x824bbbb0` | `FM2_LuaSyntax_CheckProtoHasDebugInfo` | Evidence from decompile and caller context. |
| `0x824be6c8` | `FM2_Lua_GetTableIndexAsClosureSlot` | Evidence from decompile and caller context. |
| `0x824be988` | `FM2_Lua_ShrinkProtoSideTables` | Evidence from decompile and caller context. |
| `0x824bee50` | `FM2_Lua_CopyTableIntoClosureSlot` | Evidence from decompile and caller context. |
| `0x824bf8d8` | `FM2_Lua_UnlinkProtoConstantListNode` | Evidence from decompile and caller context. |
| `0x824bfa88` | `FM2_Lua_ShrinkProtoTablesAndCode` | Evidence from decompile and caller context. |
| `0x824bfb60` | `FM2_Lua_ShrinkProtoUpvalueArray` | Evidence from decompile and caller context. |
| `0x824c04b8` | `FM2_LuaSyntax_ParseReturnStatement` | Evidence from decompile and caller context. |
| `0x824c17c0` | `FM2_LuaSyntax_ParseParenExprList` | Evidence from decompile and caller context. |
| `0x824c1d18` | `FM2_LuaSyntax_PushTempScopeFrame` | Evidence from decompile and caller context. |
| `0x824c1d78` | `FM2_LuaSyntax_ParseLocalAssign` | Evidence from decompile and caller context. |
| `0x824c1f20` | `FM2_LuaSyntax_CollectLocalFlagsInScope` | Evidence from decompile and caller context. |
| `0x824c1fb8` | `FM2_LuaSyntax_ParseFunctionStmt` | Evidence from decompile and caller context. |
| `0x824c20a0` | `FM2_LuaSyntax_ParseForNumericStmt` | Evidence from decompile and caller context. |
| `0x824c2620` | `FM2_LuaSyntax_ParseRepeatUntilStmt` | Evidence from decompile and caller context. |
| `0x824c26e8` | `FM2_LuaSyntax_ParseIfThenElseStmt` | Evidence from decompile and caller context. |
| `0x824c2838` | `FM2_LuaSyntax_ParseTableConstructor` | Evidence from decompile and caller context. |
| `0x824c2930` | `FM2_LuaSyntax_ParseFieldListTrailingComma` | Evidence from decompile and caller context. |
| `0x824c2a30` | `FM2_LuaSyntax_ParseLocalFunctionStmt` | Evidence from decompile and caller context. |
| `0x824c2af8` | `FM2_LuaSyntax_ParseBreakOrReturnEarly` | Evidence from decompile and caller context. |
| `0x824c2fd0` | `FM2_LuaSyntax_ValidateChunkNotPrecompiled` | Evidence from decompile and caller context. |
| `0x824c32d0` | `FM2_LuaSyntax_ParseLocalVarDeclList` | Evidence from decompile and caller context. |
| `0x824802d8` | `FM2_AIDriver_AdvanceCircularSectorIndex` | Evidence from decompile and caller context. |
| `0x8242ce58` | `FM2_Profile_LoadTuningFromDevicePath` | Evidence from decompile and caller context. |
| `0x82422c28` | `FM2_Crt_StrncpyValidated` | Evidence from decompile and caller context. |
| `0x824c03d8` | `FM2_LuaSyntax_ErrorTooManyLocals` | Evidence from decompile and caller context. |