import json

RENAMES = [
    ("0x8259f340", "FM2_Memory_AllocArray8Checked"),
    ("0x8220c198", "FM2_Stl_String_ResizeAndNullTerminate"),
    ("0x82205ef0", "FM2_Lua_GetDefaultComPtrCtorArgs"),
    ("0x82205ba0", "FM2_Lua_BindingPairVector_ReserveCapacity"),
    ("0x82454318", "FM2_Network_MemmovePayloadRange8"),
    ("0x82457b30", "FM2_Network_BuildTimedMessageListFromRange"),
    ("0x824576a8", "FM2_Network_AllocTimedMessageNodeFromTemplate"),
    ("0x82435488", "FM2_AsyncOp_HeapifyUpContentRecord"),
    ("0x82435530", "FM2_AsyncOp_PartitionIntroSortRange"),
    ("0x824364c0", "FM2_ContentVector_DestroyRangeAndTrim"),
    ("0x8242f6f0", "FM2_AsyncQueue_IncRefAndMaybeCloseHandle"),
    ("0x82461ad0", "FM2_BufferedFileRead_HashBufferWithXeCryptSha"),
    ("0x82464de0", "FM2_LapTracker_ComputeSplineSegmentBounds"),
    ("0x82464e60", "FM2_LapTracker_CompareTrackProgressFlags"),
    ("0x82480580", "FM2_AIDriver_WrapSectorIndexForward"),
    ("0x82480fb0", "FM2_AIDriver_ReconcileRaceLineSectorState"),
    ("0x824ba348", "FM2_Lua_MarkStackObjectsDuringTraverse"),
    ("0x824ba5b8", "FM2_Lua_MarkTableUpvaluesDuringTraverse"),
    ("0x824ba9e0", "FM2_Lua_TraverseOpenUpvalueChain"),
    ("0x824bab50", "FM2_Lua_CollectGrayObjectsFromList"),
    ("0x824badd8", "FM2_Lua_MarkGrayObjectGraphRecursive"),
    ("0x824bb348", "FM2_Lua_LinkUpvalueToOpenList"),
    ("0x824befd8", "FM2_Lua_CreateClosureFromHashSlot"),
    ("0x824bf308", "FM2_Lua_LookupOrCreateClosureSlot"),
    ("0x824bf738", "FM2_Lua_AllocProtoWithConstants"),
    ("0x824bf7b8", "FM2_Lua_AllocUpvalueDescTable"),
    ("0x824bfb90", "FM2_Lua_FindUpvalueIndexInProto"),
    ("0x824bfce8", "FM2_LuaSyntax_ReadBytesFromLexer"),
    ("0x824c0c50", "FM2_LuaSyntax_PushParserStateFrame"),
    ("0x824c2db8", "FM2_LuaSyntax_ParseChunkStatements"),
    ("0x824c3488", "FM2_LuaSyntax_ParseFuncOrStatList"),
    ("0x824c37e0", "FM2_LuaSyntax_AppendLexemeToBuffer"),
    ("0x82454d08", "FM2_Network_EraseMessageTreeNodeRebalance"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass42.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 42 (33 functions)\n",
    "Memory alloc helpers, network timed messages, Lua GC mark/lexer/parser, lap tracker spline, async sort.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass42.md", "w", encoding="utf-8").write("\n".join(md))
