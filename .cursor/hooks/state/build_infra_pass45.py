import json

RENAMES = [
    ("0x824c6f10", "FM2_LuaCompiler_DtorOptionalSlotAt"),
    ("0x824c7708", "FM2_LuaCompiler_ClearOptionalSlotVector"),
    ("0x824cc9b8", "FM2_BufFile_InsertModuleRefAtIndex"),
    ("0x824cc320", "FM2_BufFile_GrowModuleRefVectorCapacity"),
    ("0x824ccbc0", "FM2_XmlReader_ComparePathSegmentI"),
    ("0x824ccc58", "FM2_XmlReader_ParsePathToBuffer"),
    ("0x824cd5a8", "FM2_XmlReader_LoadFloatAttrByName"),
    ("0x824cd8e8", "FM2_Lua_GetLibRegFieldByIndex12"),
    ("0x824cd980", "FM2_Lua_GetLibRegFieldByIndex16"),
    ("0x824ce0b8", "FM2_Lua_GetLibRegFirstFieldIfAny"),
    ("0x824ce490", "FM2_XmlReader_InitEmptyAttrNodeA"),
    ("0x824ce4e8", "FM2_XmlReader_InitEmptyAttrNodeB"),
    ("0x824ce540", "FM2_XmlReader_GrowAttrVectorCapacity"),
    ("0x824ce658", "FM2_XmlReader_InsertAttrAtIndex"),
    ("0x824cf490", "FM2_XmlReader_ApplyAttrDefaultsFromTable"),
    ("0x824cff90", "FM2_BufFile32768_Ctor"),
    ("0x824d0090", "FM2_BufFile_SetErrorFlagAndCopyPath"),
    ("0x824d0f48", "FM2_Input_SslContext_InitWithVtable"),
    ("0x824d19c8", "FM2_Camera_LoadScriptPathFromConfig"),
    ("0x824d3340", "FM2_Config_ParseWhitespaceDelimitedTokens"),
    ("0x824c05c8", "FM2_LuaSyntax_AllocLocalVarSlot"),
    ("0x824c1ae8", "FM2_LuaSyntax_ParseBlockWithScope"),
    ("0x824bb740", "FM2_LuaSyntax_ParseFunctionBody"),
    ("0x8257e190", "FM2_LuaCompiler_CopyOptionalSlotTailFields"),
    ("0x824c0d58", "FM2_LuaSyntax_LoadStringChunkBody"),
    ("0x824c30c8", "FM2_LuaSyntax_ParseFuncHeaderAndBody"),
    ("0x824c3648", "FM2_LuaSyntax_InitDefaultHeaderTag"),
    ("0x824c3888", "FM2_LuaSyntax_InternTokenNameStrings"),
    ("0x824c3a50", "FM2_LuaSyntax_RegisterLocalNameSlot"),
    ("0x824c3aa8", "FM2_LuaSyntax_LexerNextOnRefCountZero"),
    ("0x824c3c60", "FM2_LuaSyntax_LexSingleCharOperator"),
    ("0x824c3cf0", "FM2_LuaSyntax_ParseNumberWithLocale"),
    ("0x824c3df8", "FM2_LuaSyntax_ParseNumberOrHexLiteral"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass45.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 45 (33 functions)\n",
    "BufFile/XML reader, Lua compiler optional slots, syntax block parser, Lua GC mark helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass45.md", "w", encoding="utf-8").write("\n".join(md))
