import json

RENAMES = [
    ("0x823b1278", "FM2_Png_EnsureRgbThenDecodeRow"),
    ("0x823c1190", "FM2_Bitstream_ReadVariableBits"),
    ("0x823d6eb0", "FM2_Image_PixelBuffer_Init"),
    ("0x82251c98", "FM2_LiveryMask_MergeProfileRecordFromNode"),
    ("0x824930a0", "FM2_AIDriver_ComputeSectorIndexFromProgress"),
    ("0x8250a820", "FM2_Render_WaitResourceLockForPassCompile"),
    ("0x8250ce18", "FM2_Render_ViewTraversalCullNodes"),
    ("0x8250cff0", "FM2_Render_ViewTraversalSortDrawLists"),
    ("0x8250d238", "FM2_Render_ViewTraversalEmitDrawCalls"),
    ("0x8250e400", "FM2_Presentation_AllocCarInstanceSlot"),
    ("0x8250f590", "FM2_Memory_AllocDeferredMapBlockLocked"),
    ("0x82512038", "FM2_Presentation_CopyCarDisplayBlockToSlot"),
    ("0x82513280", "FM2_Render_BuildObjectPassCommandBuffer"),
    ("0x82513340", "FM2_Render_AppendObjectPassDrawEntry"),
    ("0x825135b8", "FM2_Vector_EraseBegin20ByteElements"),
    ("0x825151a0", "FM2_Render_SubmitObjectDrawConstantsBlock"),
    ("0x82516e30", "FM2_Render_UpdatePassVisibilitySortKeysA"),
    ("0x82516ed8", "FM2_Render_UpdatePassVisibilitySortKeysB"),
    ("0x82517778", "FM2_Render_FramePipelineSubmitPassA"),
    ("0x82517870", "FM2_Render_FramePipelineSubmitPassB"),
    ("0x825179e0", "FM2_Render_SubmitPassWrapperInner"),
    ("0x82517b18", "FM2_Render_FramePipelineDrawObjects"),
    ("0x82518228", "FM2_Render_FramePipelineFinalizePass"),
    ("0x8251a010", "FM2_Render_SubmitSortedObjectDrawListsInner"),
    ("0x8251c290", "FM2_Presentation_ApplyCarCameraVMX"),
    ("0x82521a98", "FM2_Render_InstanceHybridDrawPathInner"),
    ("0x825222b0", "FM2_Render_ExecuteSortedDrawListsCore"),
    ("0x8254d6e8", "FM2_Lua_RegisterBindingPairsInModuleTable"),
    ("0x8254e278", "FM2_Lua_AssertionFailedVprintf"),
    ("0x82553568", "FM2_Lua_AllocUpvalueClosure"),
    ("0x82556170", "FM2_CarDynamics_InitSuspensionFromTireBlock"),
    ("0x82559680", "FM2_FMOD_Geometry_AddPolygonFromVMX"),
    ("0x82559d50", "FM2_Render_ObjectPassDrawSetupMaterialPass"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass51.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 51 (33 functions)\n",
    "Render frame pipeline / view traversal cluster, PNG/bitstream/image, Lua binding register, FMOD geometry.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass51.md", "w", encoding="utf-8").write("\n".join(md))
