import json

RENAMES = [
    ("0x82511968", "FM2_PresentationCarConfig_DeleteOptionalBodyB"),
    ("0x82511b28", "FM2_Presentation_CopyCarDisplayBlockToSlotBody"),
    ("0x825121d0", "FM2_PresentationSlotVector_Clear200ByteInit"),
    ("0x825125c0", "FM2_Vector_ComputeEraseSpanFor20ByteElements"),
    ("0x82512aa8", "FM2_Render_GrowObjectPassDrawVectorBody"),
    ("0x82512b30", "FM2_Vector_EraseBegin20ByteElementsTail"),
    ("0x82513188", "FM2_Vector_EraseBegin20ByteElementsHead"),
    ("0x82514640", "FM2_Render_FramePipelineDrawObjectsBody"),
    ("0x82515a58", "FM2_Render_CompilePassIfStaleLockedBodyA"),
    ("0x82515ba8", "FM2_Render_CompilePassIfStaleLockedBodyB"),
    ("0x8251a0a8", "FM2_Presentation_ApplyCarCameraVMXBodyA"),
    ("0x8251a198", "FM2_Presentation_ApplyCarCameraVMXBodyB"),
    ("0x8251ae30", "FM2_Render_FramePipelineSubmitPassBBody"),
    ("0x8251e9f0", "FM2_CarPresentation_DtorBody"),
    ("0x8251f0c8", "FM2_Render_TestPassVisibilityVMXBody"),
    ("0x82522598", "FM2_Render_CompilePassIfStaleLockedBodyC"),
    ("0x82527d00", "FM2_Render_SortVisibleRenderablesIntrosortInit"),
    ("0x82528898", "FM2_Render_SortVisibleRenderablesIntrosortBody"),
    ("0x82528d00", "FM2_Render_SortVisibleRenderablesIntrosortPartition"),
    ("0x82529a68", "FM2_Render_SortVisibleRenderablesIntrosortInsert"),
    ("0x8252ad70", "FM2_Render_Helper16E0SortKeyCompare"),
    ("0x8252d118", "FM2_Render_GetDistanceKeyFromPassSlotBody"),
    ("0x8252dc18", "FM2_Render_UpdateObjectDistanceKeysBody"),
    ("0x8252efe8", "FM2_Render_SortVisibleRenderablesCompare"),
    ("0x8252f040", "FM2_Memory_AllocTaggedSmallBlockFromPoolEntryTail"),
    ("0x82535b08", "FM2_Render_InstanceHybridDrawPathSortBody"),
    ("0x82535c98", "FM2_Render_InitSkinnedModelResourceLockBody"),
    ("0x82536520", "FM2_Render_InstanceHybridDrawPathSortCore"),
    ("0x82536840", "FM2_Render_InstanceHybridDrawPathSortPartition"),
    ("0x82536d38", "FM2_Render_InstancePathWrapperBodyA"),
    ("0x82537538", "FM2_Render_InstanceHybridDrawPathSortFinalize"),
    ("0x82537a68", "FM2_Render_InstancePathWrapperBodyB"),
    ("0x82538870", "FM2_Render_InstancePathWrapperInnerBody"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass80.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 80 (33 functions)\n",
    "Presentation/render frame pipeline, sort introspect, instance path wrapper cluster.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass80.md", "w", encoding="utf-8").write("\n".join(md))
