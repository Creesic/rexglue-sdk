### Infrastructure pass 46 (33 functions)

Lua syntax codegen/backpatch cluster (4-caller helpers), RB-tree insert helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c0458` | `FM2_LuaSyntax_ErrorIfTokenMismatch` | Evidence from decompile and caller context. |
| `0x824c0a20` | `FM2_LuaSyntax_PopScopeAndRestoreLocals` | Evidence from decompile and caller context. |
| `0x824c5748` | `FM2_LuaSyntax_PatchJumpListBackpatch` | Evidence from decompile and caller context. |
| `0x824c57d0` | `FM2_LuaSyntax_BackpatchJumpToHere` | Evidence from decompile and caller context. |
| `0x824c15d0` | `FM2_LuaSyntax_ParseCommaSeparatedBlocks` | Evidence from decompile and caller context. |
| `0x824c5e80` | `FM2_LuaSyntax_EmitExprAsRkOperand` | Evidence from decompile and caller context. |
| `0x824c61a8` | `FM2_LuaSyntax_DischargeExprToAnyReg` | Evidence from decompile and caller context. |
| `0x824c0850` | `FM2_LuaSyntax_ResolveLocalOrUpvalueIndex` | Evidence from decompile and caller context. |
| `0x824c0980` | `FM2_LuaSyntax_EmitAssignOrAdjustStack` | Evidence from decompile and caller context. |
| `0x824c0f98` | `FM2_LuaSyntax_ParseCallOrIndexSuffix` | Evidence from decompile and caller context. |
| `0x824c54d8` | `FM2_LuaSyntax_EmitInstructionWord` | Evidence from decompile and caller context. |
| `0x824c4f18` | `FM2_LuaSyntax_PatchJumpChainToTarget` | Evidence from decompile and caller context. |
| `0x824c57e8` | `FM2_LuaSyntax_DischargeExprToReg` | Evidence from decompile and caller context. |
| `0x824c5c30` | `FM2_LuaSyntax_CodeConditionalJump` | Evidence from decompile and caller context. |
| `0x824c5cc0` | `FM2_LuaSyntax_DischargeToRegOrEmitMove` | Evidence from decompile and caller context. |
| `0x824c5a10` | `FM2_LuaSyntax_FreeExpAndAllocReg` | Evidence from decompile and caller context. |
| `0x824c4cf8` | `FM2_LuaSyntax_SavePcForBackpatch` | Evidence from decompile and caller context. |
| `0x824c4d08` | `FM2_LuaSyntax_PatchJumpFixupAtPc` | Evidence from decompile and caller context. |
| `0x824c4da8` | `FM2_LuaSyntax_MakeIndexedExpDesc` | Evidence from decompile and caller context. |
| `0x824c4fd0` | `FM2_LuaSyntax_ReserveFreeRegCount` | Evidence from decompile and caller context. |
| `0x824c5080` | `FM2_LuaSyntax_ExpToRegWithKOrMove` | Evidence from decompile and caller context. |
| `0x824c5240` | `FM2_LuaSyntax_PatchJumpOffsetInCode` | Evidence from decompile and caller context. |
| `0x824c55c0` | `FM2_LuaSyntax_EmitEncodedInstruction` | Evidence from decompile and caller context. |
| `0x824c5608` | `FM2_LuaSyntax_GrowProtoBuffer` | Evidence from decompile and caller context. |
| `0x824c0330` | `FM2_LuaSyntax_LexIdentifierOrKeyword` | Evidence from decompile and caller context. |
| `0x824c6500` | `FM2_LuaSyntax_SetExpDescToCallResult` | Evidence from decompile and caller context. |
| `0x824c57b0` | `FM2_LuaSyntax_EmitLoadBoolInstruction` | Evidence from decompile and caller context. |
| `0x824c6a60` | `FM2_LuaSyntax_MergeJumpListIfAtPc` | Evidence from decompile and caller context. |
| `0x824c51d0` | `FM2_LuaSyntax_MakeConstantNumberExpDesc` | Evidence from decompile and caller context. |
| `0x824c5038` | `FM2_LuaSyntax_ReserveStackSlotsAfterCall` | Evidence from decompile and caller context. |
| `0x824c14d0` | `FM2_LuaSyntax_ParseMethodOrFunctionHeader` | Evidence from decompile and caller context. |
| `0x8258c8e8` | `FM2_RbTree_InsertNodeWithHint` | Evidence from decompile and caller context. |
| `0x8258c860` | `FM2_RbTree_CountNodesInSubtreeRange` | Evidence from decompile and caller context. |