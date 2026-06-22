import json

RENAMES = [
    ("0x824ccf40", "FM2_XmlReader_ParseBoolAttrDefault"),
    ("0x824cce90", "FM2_XmlReader_ParseFloatAttrDefault"),
    ("0x824cd740", "FM2_XmlReader_ParseBoolAttrStrict"),
    ("0x824cd0d0", "FM2_XmlReader_BuildErrorPathString"),
    ("0x824cd1b0", "FM2_XmlReader_StreamReadLineIntoBuffer"),
    ("0x824cd3a8", "FM2_XmlWriter_AppendStringAttr"),
    ("0x824cd430", "FM2_XmlWriter_AppendFloatAttr"),
    ("0x824ce240", "FM2_XmlWriter_WriteIndentBeforeClose"),
    ("0x824ce390", "FM2_XmlWriter_WriteSelfClosingTag"),
    ("0x824ce970", "FM2_XmlReader_LoadAttributesFromArchive"),
    ("0x824cf688", "FM2_XmlReader_ApplyAttrDefaultsFromNode"),
    ("0x824cf8f8", "FM2_XmlReader_FindAttrEntryByName"),
    ("0x824cfa78", "FM2_XmlReader_FindAttrByPathSegments"),
    ("0x824cfc10", "FM2_XmlReader_CopyAttrDefaultsToNode"),
    ("0x824cc840", "FM2_XmlReader_DtorAttrNodeList"),
    ("0x824cc4d0", "FM2_BufFile_CompactModuleRefTableLocked"),
    ("0x824cd4d0", "FM2_XmlReader_DtorStringNodeA"),
    ("0x824cd538", "FM2_XmlReader_DtorStringNodeB"),
    ("0x824e3fb0", "FM2_RenderAdapter_InitSwitchModeBase"),
    ("0x824e53c0", "FM2_RenderAdapter_InitSwitchModeAlt"),
    ("0x824e7bd0", "FM2_CareerRace_MoveRewardsBlock"),
    ("0x824e8368", "FM2_HashName_AssignFromPropertySlice"),
    ("0x824e9960", "FM2_LiveProfile_ReadWriteBufferHeader"),
    ("0x824eae80", "FM2_Lua_AllocBindingPairVector"),
    ("0x824eaf90", "FM2_Lua_BindingPairMedianOfThree"),
    ("0x824ebf20", "FM2_Lua_BindingPairSortEntryThunk"),
    ("0x824d6480", "FM2_Lua_PushDampingFromKeyframeDouble"),
    ("0x824d75a0", "FM2_Profile_MakeStringKeyComPtr"),
    ("0x824d76a0", "FM2_Profile_ParseUnsignedFromSubString"),
    ("0x824d8b28", "FM2_Math_AllocForceVectorComPtr"),
    ("0x824d3cb0", "FM2_Scene_GetNotifyStateFromParamHelper"),
    ("0x82587c88", "FM2_Network_DispatchMessageFromQueueLocked"),
    ("0x8258d228", "FM2_Set_LowerBoundByKeyInTree"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass49.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 49 (33 functions)\n",
    "XML reader/writer attr helpers, render adapter init, profile/Lua binding alloc, network dispatch.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass49.md", "w", encoding="utf-8").write("\n".join(md))
