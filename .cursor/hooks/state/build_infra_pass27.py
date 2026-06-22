import json

RENAMES = [
    ("0x824cda18", "FM2_XmlReader_FindElementByNameRecursive"),
    ("0x824cd238", "FM2_XmlReader_CompareElementNameI"),
    ("0x824cdae0", "FM2_XmlReader_GetChildElementByName"),
    ("0x824cd6d0", "FM2_XmlReader_ParseFloatAttribute"),
    ("0x8245bdf8", "FM2_BufFile_NormalizePathToLowercase"),
    ("0x82463980", "FM2_LiveryMask_GrowPendingRecordTable"),
    ("0x824638b8", "FM2_LiveryMask_AllocPendingRecordSlot188"),
    ("0x824d10d8", "FM2_Input_SslDeviceContext_Ctor"),
    ("0x824d2710", "FM2_Input_SslDeviceBinding_Ctor"),
    ("0x824d0850", "FM2_Input_SslContext_InitFromPath"),
    ("0x824d2188", "FM2_Input_SslBindingRecord_Init"),
    ("0x821d04c0", "FM2_Profile_AllocTuningHashNode16"),
    ("0x824d36e0", "FM2_ProfileTuningHashNode_Ctor"),
    ("0x8221d380", "FM2_Profile_AllocStringListNodeWithKey"),
    ("0x8221d3d8", "FM2_ProfileStringList_CheckLengthAndAdd"),
    ("0x82246630", "FM2_LiveryEditor_FindOrInsertDecalTabEntry"),
    ("0x824ffe90", "FM2_LiveryEditor_RbTreeInsertDecalTabKey"),
    ("0x822fcfc0", "FM2_RaceGhost_IntroSortKeyframeBuffer"),
    ("0x8230bac8", "FM2_RaceGhost_PartitionKeyframeBuffer"),
    ("0x82331988", "FM2_RaceGhost_BuildPlaybackUpdateTask"),
    ("0x82331b30", "FM2_RaceGhost_InitDeferredPlaybackWrapper"),
    ("0x8235d3f8", "FM2_UI_PropertyMaskMatchesState"),
    ("0x8235d3b0", "FM2_UI_GetAnimPropertyBlockById"),
    ("0x8235e3b0", "FM2_UI_GetAnimPropertyFloatById"),
    ("0x8235e540", "FM2_UI_CountMatchingPropertiesInGroup"),
    ("0x8235e610", "FM2_UI_GetPropertyRecordByGroupIndex"),
    ("0x8235e290", "FM2_UI_GetDefaultPropertyFloatScaled"),
    ("0x82360c40", "FM2_Input_ParseRumbleMotorPairXml"),
    ("0x823661d8", "FM2_Memory_DeferredFreeMapInsertNode"),
    ("0x8236b598", "FM2_AudioRender_ComputeFrontBufferMixSample"),
    ("0x8236b4d0", "FM2_AudioRender_SampleFrontBufferRegion"),
    ("0x8236c480", "FM2_GpuKick_SubmitShaderConstantsFromTable"),
    ("0x8236d948", "FM2_Render_FreeGpuKickTagAt13404"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824cda18": "Depth-first XML element lookup by backslash path segment.",
    "0x8245bdf8": "Lowercases buf-file path in place for prefix matching.",
    "0x8230bac8": "Dual-pivot partition for race ghost keyframe introsort.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass27.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 27 (33 functions)\n",
    "XML/buf-file cluster, input SSL bindings, profile string lists, race ghost sort, UI property helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass27.md", "w", encoding="utf-8").write("\n".join(md))
