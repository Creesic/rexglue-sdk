import json

RENAMES = [
    ("0x824c1120", "FM2_LuaSyntax_ParseSimpleExprPrefix"),
    ("0x824c4050", "FM2_LuaSyntax_LexStringLiteral"),
    ("0x824c1638", "FM2_LuaSyntax_ParsePrimaryPrefixExpr"),
    ("0x824c1978", "FM2_LuaSyntax_ParsePrimaryExprSuffixLoop"),
    ("0x824c2388", "FM2_LuaSyntax_ParseForNumericLoopVars"),
    ("0x824c24f8", "FM2_LuaSyntax_ParseForInIteratorVars"),
    ("0x824c5308", "FM2_LuaSyntax_FixupJumpTargetsAtPc"),
    ("0x824c54b8", "FM2_LuaSyntax_PatchLastEmittedLine"),
    ("0x824c5d48", "FM2_LuaSyntax_DischargeExpToRegIfIndexed"),
    ("0x824c5fc0", "FM2_LuaSyntax_EmitBinaryOpAndFreeRegs"),
    ("0x824c6740", "FM2_LuaSyntax_ExpToRegByKind"),
    ("0x824c67f8", "FM2_LuaSyntax_DischargeTestExpToJmp"),
    ("0x824c62c0", "FM2_LuaSyntax_DischargeJumpListToReg"),
    ("0x824c6378", "FM2_LuaSyntax_NegateConditionalJumpLists"),
    ("0x824cbed0", "FM2_BufFile_IncRefModuleHandle"),
    ("0x824cc258", "FM2_BufFile_ReleaseModuleRefArray"),
    ("0x824cd088", "FM2_XmlReader_GetAttrStringOrRequire"),
    ("0x824cd350", "FM2_XmlReader_SkipWhitespaceAndParseNodes"),
    ("0x824cf3d8", "FM2_XmlReader_ParseChildNodesUntilClose"),
    ("0x824ccde0", "FM2_XmlReader_CompareTokenCaseInsensitive"),
    ("0x824ccd30", "FM2_XmlReader_SkipWhitespace"),
    ("0x824cd2d8", "FM2_XmlReader_TryParseCommentOrDecl"),
    ("0x824cf118", "FM2_XmlReader_ParseElementOpenTag"),
    ("0x824c5208", "FM2_LuaSyntax_MakeFloatExpDesc"),
    ("0x824c5370", "FM2_LuaSyntax_TryCoalesceStringConcatExp"),
    ("0x824c55e8", "FM2_LuaSyntax_EmitLoadKInstruction"),
    ("0x824c45c0", "FM2_LuaSyntax_LexNumberOrStringLiteral"),
    ("0x824c6850", "FM2_LuaSyntax_SimpleExprTypeDispatch"),
    ("0x824c56a8", "FM2_LuaSyntax_EmitVarArgRangeInstruction"),
    ("0x824c5d60", "FM2_LuaSyntax_StoreExpResultToReg"),
    ("0x824c60a8", "FM2_LuaSyntax_EmitCompareBranchInstruction"),
    ("0x824ccb98", "FM2_XmlReader_InitScratchNameBuffer"),
    ("0x824ccd98", "FM2_XmlReader_ScanUntilChar"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass47.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 47 (33 functions)\n",
    "Lua syntax parse/discharge helpers, XML reader whitespace/element parse, BufFile ref helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass47.md", "w", encoding="utf-8").write("\n".join(md))
