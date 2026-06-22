import json

RENAMES = [
    ("0x824c0458", "FM2_LuaSyntax_ErrorIfTokenMismatch"),
    ("0x824c0a20", "FM2_LuaSyntax_PopScopeAndRestoreLocals"),
    ("0x824c5748", "FM2_LuaSyntax_PatchJumpListBackpatch"),
    ("0x824c57d0", "FM2_LuaSyntax_BackpatchJumpToHere"),
    ("0x824c15d0", "FM2_LuaSyntax_ParseCommaSeparatedBlocks"),
    ("0x824c5e80", "FM2_LuaSyntax_EmitExprAsRkOperand"),
    ("0x824c61a8", "FM2_LuaSyntax_DischargeExprToAnyReg"),
    ("0x824c0850", "FM2_LuaSyntax_ResolveLocalOrUpvalueIndex"),
    ("0x824c0980", "FM2_LuaSyntax_EmitAssignOrAdjustStack"),
    ("0x824c0f98", "FM2_LuaSyntax_ParseCallOrIndexSuffix"),
    ("0x824c54d8", "FM2_LuaSyntax_EmitInstructionWord"),
    ("0x824c4f18", "FM2_LuaSyntax_PatchJumpChainToTarget"),
    ("0x824c57e8", "FM2_LuaSyntax_DischargeExprToReg"),
    ("0x824c5c30", "FM2_LuaSyntax_CodeConditionalJump"),
    ("0x824c5cc0", "FM2_LuaSyntax_DischargeToRegOrEmitMove"),
    ("0x824c5a10", "FM2_LuaSyntax_FreeExpAndAllocReg"),
    ("0x824c4cf8", "FM2_LuaSyntax_SavePcForBackpatch"),
    ("0x824c4d08", "FM2_LuaSyntax_PatchJumpFixupAtPc"),
    ("0x824c4da8", "FM2_LuaSyntax_MakeIndexedExpDesc"),
    ("0x824c4fd0", "FM2_LuaSyntax_ReserveFreeRegCount"),
    ("0x824c5080", "FM2_LuaSyntax_ExpToRegWithKOrMove"),
    ("0x824c5240", "FM2_LuaSyntax_PatchJumpOffsetInCode"),
    ("0x824c55c0", "FM2_LuaSyntax_EmitEncodedInstruction"),
    ("0x824c5608", "FM2_LuaSyntax_GrowProtoBuffer"),
    ("0x824c0330", "FM2_LuaSyntax_LexIdentifierOrKeyword"),
    ("0x824c6500", "FM2_LuaSyntax_SetExpDescToCallResult"),
    ("0x824c57b0", "FM2_LuaSyntax_EmitLoadBoolInstruction"),
    ("0x824c6a60", "FM2_LuaSyntax_MergeJumpListIfAtPc"),
    ("0x824c51d0", "FM2_LuaSyntax_MakeConstantNumberExpDesc"),
    ("0x824c5038", "FM2_LuaSyntax_ReserveStackSlotsAfterCall"),
    ("0x824c14d0", "FM2_LuaSyntax_ParseMethodOrFunctionHeader"),
    ("0x8258c8e8", "FM2_RbTree_InsertNodeWithHint"),
    ("0x8258c860", "FM2_RbTree_CountNodesInSubtreeRange"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass46.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 46 (33 functions)\n",
    "Lua syntax codegen/backpatch cluster (4-caller helpers), RB-tree insert helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass46.md", "w", encoding="utf-8").write("\n".join(md))
