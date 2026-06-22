import json

RENAMES = [
    ("0x8258c7f8", "FM2_RbTree_RotateLeftAtChildSibling"),
    ("0x8246a3a8", "FM2_LapTracker_ComputeCrossProductProgressOnSpline"),
    ("0x822049b0", "FM2_Memory_AllocArray308Checked"),
    ("0x82204e30", "FM2_Lua_BindingPairVector_UninitializedFill308"),
    ("0x824b9c08", "FM2_Lua_GrowStackAndShrinkLiveSlots"),
    ("0x824ba048", "FM2_Lua_ProcessGrayObjectWorkList"),
    ("0x824ba880", "FM2_Lua_MarkProtoUpvalueChain"),
    ("0x824bbbb0", "FM2_LuaSyntax_CheckProtoHasDebugInfo"),
    ("0x824be6c8", "FM2_Lua_GetTableIndexAsClosureSlot"),
    ("0x824be988", "FM2_Lua_ShrinkProtoSideTables"),
    ("0x824bee50", "FM2_Lua_CopyTableIntoClosureSlot"),
    ("0x824bf8d8", "FM2_Lua_UnlinkProtoConstantListNode"),
    ("0x824bfa88", "FM2_Lua_ShrinkProtoTablesAndCode"),
    ("0x824bfb60", "FM2_Lua_ShrinkProtoUpvalueArray"),
    ("0x824c04b8", "FM2_LuaSyntax_ParseReturnStatement"),
    ("0x824c17c0", "FM2_LuaSyntax_ParseParenExprList"),
    ("0x824c1d18", "FM2_LuaSyntax_PushTempScopeFrame"),
    ("0x824c1d78", "FM2_LuaSyntax_ParseLocalAssign"),
    ("0x824c1f20", "FM2_LuaSyntax_CollectLocalFlagsInScope"),
    ("0x824c1fb8", "FM2_LuaSyntax_ParseFunctionStmt"),
    ("0x824c20a0", "FM2_LuaSyntax_ParseForNumericStmt"),
    ("0x824c2620", "FM2_LuaSyntax_ParseRepeatUntilStmt"),
    ("0x824c26e8", "FM2_LuaSyntax_ParseIfThenElseStmt"),
    ("0x824c2838", "FM2_LuaSyntax_ParseTableConstructor"),
    ("0x824c2930", "FM2_LuaSyntax_ParseFieldListTrailingComma"),
    ("0x824c2a30", "FM2_LuaSyntax_ParseLocalFunctionStmt"),
    ("0x824c2af8", "FM2_LuaSyntax_ParseBreakOrReturnEarly"),
    ("0x824c2fd0", "FM2_LuaSyntax_ValidateChunkNotPrecompiled"),
    ("0x824c32d0", "FM2_LuaSyntax_ParseLocalVarDeclList"),
    ("0x824802d8", "FM2_AIDriver_AdvanceCircularSectorIndex"),
    ("0x8242ce58", "FM2_Profile_LoadTuningFromDevicePath"),
    ("0x82422c28", "FM2_Crt_StrncpyValidated"),
    ("0x824c03d8", "FM2_LuaSyntax_ErrorTooManyLocals"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass44.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 44 (33 functions)\n",
    "RB-tree rotate, lap tracker cross product, Lua GC shrink, Lua syntax statement parser cluster.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass44.md", "w", encoding="utf-8").write("\n".join(md))
