### Infrastructure pass 47 (33 functions)

Lua syntax parse/discharge helpers, XML reader whitespace/element parse, BufFile ref helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c1120` | `FM2_LuaSyntax_ParseSimpleExprPrefix` | Evidence from decompile and caller context. |
| `0x824c4050` | `FM2_LuaSyntax_LexStringLiteral` | Evidence from decompile and caller context. |
| `0x824c1638` | `FM2_LuaSyntax_ParsePrimaryPrefixExpr` | Evidence from decompile and caller context. |
| `0x824c1978` | `FM2_LuaSyntax_ParsePrimaryExprSuffixLoop` | Evidence from decompile and caller context. |
| `0x824c2388` | `FM2_LuaSyntax_ParseForNumericLoopVars` | Evidence from decompile and caller context. |
| `0x824c24f8` | `FM2_LuaSyntax_ParseForInIteratorVars` | Evidence from decompile and caller context. |
| `0x824c5308` | `FM2_LuaSyntax_FixupJumpTargetsAtPc` | Evidence from decompile and caller context. |
| `0x824c54b8` | `FM2_LuaSyntax_PatchLastEmittedLine` | Evidence from decompile and caller context. |
| `0x824c5d48` | `FM2_LuaSyntax_DischargeExpToRegIfIndexed` | Evidence from decompile and caller context. |
| `0x824c5fc0` | `FM2_LuaSyntax_EmitBinaryOpAndFreeRegs` | Evidence from decompile and caller context. |
| `0x824c6740` | `FM2_LuaSyntax_ExpToRegByKind` | Evidence from decompile and caller context. |
| `0x824c67f8` | `FM2_LuaSyntax_DischargeTestExpToJmp` | Evidence from decompile and caller context. |
| `0x824c62c0` | `FM2_LuaSyntax_DischargeJumpListToReg` | Evidence from decompile and caller context. |
| `0x824c6378` | `FM2_LuaSyntax_NegateConditionalJumpLists` | Evidence from decompile and caller context. |
| `0x824cbed0` | `FM2_BufFile_IncRefModuleHandle` | Evidence from decompile and caller context. |
| `0x824cc258` | `FM2_BufFile_ReleaseModuleRefArray` | Evidence from decompile and caller context. |
| `0x824cd088` | `FM2_XmlReader_GetAttrStringOrRequire` | Evidence from decompile and caller context. |
| `0x824cd350` | `FM2_XmlReader_SkipWhitespaceAndParseNodes` | Evidence from decompile and caller context. |
| `0x824cf3d8` | `FM2_XmlReader_ParseChildNodesUntilClose` | Evidence from decompile and caller context. |
| `0x824ccde0` | `FM2_XmlReader_CompareTokenCaseInsensitive` | Evidence from decompile and caller context. |
| `0x824ccd30` | `FM2_XmlReader_SkipWhitespace` | Evidence from decompile and caller context. |
| `0x824cd2d8` | `FM2_XmlReader_TryParseCommentOrDecl` | Evidence from decompile and caller context. |
| `0x824cf118` | `FM2_XmlReader_ParseElementOpenTag` | Evidence from decompile and caller context. |
| `0x824c5208` | `FM2_LuaSyntax_MakeFloatExpDesc` | Evidence from decompile and caller context. |
| `0x824c5370` | `FM2_LuaSyntax_TryCoalesceStringConcatExp` | Evidence from decompile and caller context. |
| `0x824c55e8` | `FM2_LuaSyntax_EmitLoadKInstruction` | Evidence from decompile and caller context. |
| `0x824c45c0` | `FM2_LuaSyntax_LexNumberOrStringLiteral` | Evidence from decompile and caller context. |
| `0x824c6850` | `FM2_LuaSyntax_SimpleExprTypeDispatch` | Evidence from decompile and caller context. |
| `0x824c56a8` | `FM2_LuaSyntax_EmitVarArgRangeInstruction` | Evidence from decompile and caller context. |
| `0x824c5d60` | `FM2_LuaSyntax_StoreExpResultToReg` | Evidence from decompile and caller context. |
| `0x824c60a8` | `FM2_LuaSyntax_EmitCompareBranchInstruction` | Evidence from decompile and caller context. |
| `0x824ccb98` | `FM2_XmlReader_InitScratchNameBuffer` | Evidence from decompile and caller context. |
| `0x824ccd98` | `FM2_XmlReader_ScanUntilChar` | Evidence from decompile and caller context. |