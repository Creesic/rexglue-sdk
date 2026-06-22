import json

RENAMES = [
    ("0x824c58f8", "FM2_LuaSyntax_DischargeToRegAndEmitUnaryOrBinary"),
    ("0x824c21d0", "FM2_LuaSyntax_InitForLoopHiddenLocals"),
    ("0x824c3f78", "FM2_LuaSyntax_LexQuotedStringContinue"),
    ("0x824c6540", "FM2_LuaSyntax_EmitBinaryOpWithStringCoalesce"),
    ("0x824c6640", "FM2_LuaSyntax_EmitBinaryOpBetweenRegs"),
    ("0x824c4cc0", "FM2_LuaSyntax_LexTokenViaNumberOrString"),
    ("0x824c06e0", "FM2_LuaSyntax_AddLocalOrUpvalueToProto"),
    ("0x824c0ae8", "FM2_LuaSyntax_RegisterFuncLocalInProto"),
    ("0x824c1008", "FM2_LuaSyntax_ParseSimpleExprAtom"),
    ("0x824c1388", "FM2_LuaSyntax_ParseFunctionParamList"),
    ("0x824c42a8", "FM2_LuaSyntax_LexLongBracketString"),
    ("0x824ccd00", "FM2_XmlReader_IsWhitespaceChar"),
    ("0x824cd4c0", "FM2_XmlReader_GetNodeRecordAtIndex"),
    ("0x824cec68", "FM2_XmlReader_AppendAttrVectorEntry"),
    ("0x824cee60", "FM2_XmlReader_ParseElementAttributes"),
    ("0x824ced18", "FM2_XmlReader_InsertNameAttrNode"),
    ("0x821d7940", "FM2_BufFile_InvokeWriterFlushVtable"),
    ("0x82414080", "FM2_LuaSyntax_RoundDoubleForStringConcat"),
    ("0x824b80b0", "FM2_LuaSyntax_ComputeOpcodeSizeClass"),
    ("0x82457890", "FM2_Network_RbTreeLowerBoundByDueTime"),
    ("0x82457ca8", "FM2_Network_RbTreeCollectDueMessages"),
    ("0x824e7f20", "FM2_Network_RbTreeSuccessorFromNode"),
    ("0x824eb200", "FM2_Lua_BindingPairIntrosortPartition"),
    ("0x824eb2a0", "FM2_Lua_BindingPairHeapSortDown"),
    ("0x824eb6b8", "FM2_Lua_BindingPathCompareCaseInsensitive"),
    ("0x824ebae0", "FM2_Lua_BindingPairInsertionSortTail"),
    ("0x824d3408", "FM2_Config_LookupTokenByIndex"),
    ("0x824d3530", "FM2_ProfileTuning_AssignWideString"),
    ("0x824de6c8", "FM2_LuaLeaderboard_TestClipDownloadFlag"),
    ("0x824d5da8", "FM2_Lua_PushSslUnitStringsTable"),
    ("0x82434408", "FM2_Config_WriteDelimitedTokenAtIndex"),
    ("0x8240c380", "FM2_Profile_OpenContentCreateExOrError"),
    ("0x824e5300", "FM2_RenderAdapter_InitPresentationVtables"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass48.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 48 (33 functions)\n",
    "Lua syntax tail (for-loop/lex/codegen), XML reader attr parse, network RB-tree due messages, Lua binding sort.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass48.md", "w", encoding="utf-8").write("\n".join(md))
