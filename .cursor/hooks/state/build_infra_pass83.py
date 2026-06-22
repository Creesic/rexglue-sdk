import json

RENAMES = [
    ("0x8252d920", "FM2_Memory_AllocTaggedSmallBlockPoolGrow"),
    ("0x82534a88", "FM2_Render_InstanceHybridDrawPathSortPartitionTail"),
    ("0x82535038", "FM2_Render_InstanceHybridDrawPathSortBodyInner"),
    ("0x82535a08", "FM2_Render_InstancePathWrapperInnerInit"),
    ("0x82535a68", "FM2_Render_InstanceHybridDrawPathSortCoreInner"),
    ("0x82535b78", "FM2_Render_InstanceHybridDrawPathSortFinalizeTail"),
    ("0x825361c8", "FM2_Render_InstancePathWrapperInnerCore"),
    ("0x82536440", "FM2_Render_InstancePathWrapperInnerParse"),
    ("0x825374c8", "FM2_Render_InstancePathWrapperInnerFinalize"),
    ("0x82541e20", "FM2_LuaGarage_EnsureCarRecordLookupParse"),
    ("0x82544c78", "FM2_IntrusiveList_ResetToSelfUnlink"),
    ("0x8254a128", "FM2_LuaParser_GetTokenOrAdvanceLineSkip"),
    ("0x8254a408", "FM2_LuaParser_GetTokenOrAdvanceLineRead"),
    ("0x82550f88", "FM2_FindAndReplaceDelimitedTextRangeScan"),
    ("0x82551048", "FM2_FindAndReplaceDelimitedTextRangeReplace"),
    ("0x825511c8", "FM2_FindAndReplaceDelimitedTextRangeAppend"),
    ("0x825512a0", "FM2_FindAndReplaceDelimitedTextRangeFinalize"),
    ("0x82551378", "FM2_FindAndReplaceDelimitedTextRangeGrowBuffer"),
    ("0x82551430", "FM2_FindAndReplaceDelimitedTextRangeCopyTail"),
    ("0x8255a380", "FM2_D3D_ValidateResourceHandlesOrRecoverCoreA"),
    ("0x8255b1f0", "FM2_D3D_ValidateResourceHandlesOrRecoverCoreB"),
    ("0x8255d370", "FM2_D3D_ValidateResourceHandlesOrRecoverTail"),
    ("0x82560ef0", "FM2_Render_ObjectPassDrawSetupInitA"),
    ("0x82561010", "FM2_Render_ObjectPassDrawSetupInitB"),
    ("0x825610a8", "FM2_Render_ObjectPassDrawSetupBindState"),
    ("0x825611b0", "FM2_Render_TestPassVisibilityVMXPreCheck"),
    ("0x82563760", "FM2_Render_ObjectPassDrawSetupMaterialCore"),
    ("0x82565608", "FM2_Render_ObjectPassDrawSetupSortKeys"),
    ("0x82566a40", "FM2_Render_ObjectPassDrawSetupFlushLists"),
    ("0x82567928", "FM2_Render_ObjectPassDrawSetupEmitDraws"),
    ("0x8256b0d8", "FM2_Render_TestPassVisibilityVMXFrustumCore"),
    ("0x82579ba8", "FM2_Render_TestPassVisibilityVMXNoOpThunk"),
    ("0x82586e58", "FM2_FMOD_Build3DAttributesPairBodyCTail"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass83.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 83 (33 functions)\n",
    "Instance path wrapper inner, find/replace text, D3D validate recover, object-pass draw setup, visibility VMX.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass83.md", "w", encoding="utf-8").write("\n".join(md))
