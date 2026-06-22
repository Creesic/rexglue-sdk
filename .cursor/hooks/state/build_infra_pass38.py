import json

RENAMES = [
    ("0x824bf370", "FM2_Lua_AllocStringTableEntry"),
    ("0x824bf6d8", "FM2_Lua_AllocCClosureWithUpvalues"),
    ("0x824be770", "FM2_Lua_GrowProtoConstantsTable"),
    ("0x824be810", "FM2_Lua_InitProtoWithUpvalues"),
    ("0x824bf190", "FM2_Lua_ProtectedCallSetupFrame"),
    ("0x824bec90", "FM2_Lua_PushLoadedClosureUpvalues"),
    ("0x824be3b0", "FM2_Lua_FindTableSlotForValue"),
    ("0x824bea18", "FM2_Lua_GetProtoConstantSlot"),
    ("0x824bc890", "FM2_Lua_GetTableFieldCopySlots"),
    ("0x824bcc38", "FM2_Lua_TryGetTableFieldViaUpvalue"),
    ("0x824bc558", "FM2_Lua_TypeErrorForConcat"),
    ("0x824bbed0", "FM2_Lua_ErrorFormatAndThrow"),
    ("0x824bbbe8", "FM2_Lua_CheckForInfiniteProtoChain"),
    ("0x824bbde0", "FM2_Lua_ResolveCallTargetProto"),
    ("0x824b8a90", "FM2_Lua_CallHookOrTraceback"),
    ("0x824b8b80", "FM2_Lua_AdjustStackForVarargs"),
    ("0x824b8d10", "FM2_Lua_TypeErrorOnCallValue"),
    ("0x824b8de0", "FM2_Lua_EnterProtectedCallFrame"),
    ("0x824bfbf0", "FM2_LuaSyntax_InitLexerFromReader"),
    ("0x824bfcc8", "FM2_LuaIO_InitFileHandleState"),
    ("0x824c3908", "FM2_LuaSyntax_GetTokenName"),
    ("0x824c3990", "FM2_LuaSyntax_ExpectedTokenNear"),
    ("0x824c4c50", "FM2_LuaSyntax_SaveLookaheadToken"),
    ("0x824c2f70", "FM2_LuaSyntax_ErrorOnPrecompiledChunk"),
    ("0x82457b98", "FM2_Network_RbTreeLowerBoundByMessageKeyDword"),
    ("0x82466080", "FM2_AIOvertake_CopyVector128ToOutput"),
    ("0x824ca820", "FM2_Stl_RefCountedString_DecRefOrFree"),
    ("0x824ca898", "FM2_Stl_RefCountedString_AssignRef"),
    ("0x824ca960", "FM2_Stl_RefCountedString_MoveInsertRange"),
    ("0x82454ad8", "FM2_CompressionStream_InsertNodeAndRebalance"),
    ("0x827d8658", "FM2_CircularBuffer_EraseRangeWrapper"),
    ("0x824b8598", "FM2_Lua_ParseLoadStringFormatSpec"),
    ("0x824b9030", "FM2_Lua_LoadStringOrFileChunk"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass38.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 38 (33 functions)\n",
    "Lua compiler/runtime helpers, ref-counted strings, compression insert, circular buffer erase.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass38.md", "w", encoding="utf-8").write("\n".join(md))
