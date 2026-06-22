import json

RENAMES = [
    ("0x825a0298", "FM2_Render_DrawPassMaterialSetupSharedHelper"),
    ("0x825e5230", "FM2_LiveryMask_OrRaceGhostSharedUtil"),
    ("0x8272d7a0", "FM2_Presentation_CopyCarDisplayBlockSharedAppend"),
    ("0x821efe38", "FM2_Render_HelperB3E8DrawPathInit"),
    ("0x82227100", "FM2_D3D_ValidateResourceHandlesCheckA"),
    ("0x82227158", "FM2_D3D_ValidateResourceHandlesCheckB"),
    ("0x82369fa0", "FM2_D3D_ValidateResourceHandlesRecoverSlot"),
    ("0x82369ff0", "FM2_D3D_ValidateResourceHandlesRecoverNoOp"),
    ("0x8236ea80", "FM2_Render_InstancePathWrapperCallThunk"),
    ("0x82418630", "FM2_HashName_InitSaltFieldA"),
    ("0x82418650", "FM2_HashName_InitSaltFieldB"),
    ("0x82455100", "FM2_Network_DispatchMessageQueueTail"),
    ("0x824635e8", "FM2_RaceGhost_BuildPlaybackSampleTableFinalize"),
    ("0x824a76a8", "FM2_RenderAdapter_DestroyChildClearThunk"),
    ("0x8250f1c8", "FM2_Presentation_CopyCarDisplayBlockSlotInitA"),
    ("0x82510260", "FM2_Presentation_CopyCarDisplayBlockSlotInitB"),
    ("0x825104a8", "FM2_PresentationSlotVector_Clear200ByteInnerA"),
    ("0x82510910", "FM2_PresentationSlotVector_Clear200ByteInnerB"),
    ("0x82510ef8", "FM2_Presentation_CopyCarDisplayBlockLinkNode"),
    ("0x82511110", "FM2_Presentation_CopyCarDisplayBlockSlotFinalize"),
    ("0x82511170", "FM2_PresentationSlotVector_Clear200ByteDtorChain"),
    ("0x825145e8", "FM2_CarPresentation_DtorReleaseFieldA"),
    ("0x8251d540", "FM2_CarPresentation_DtorReleaseFieldB"),
    ("0x8251e270", "FM2_CarPresentation_DtorClearOwnedLists"),
    ("0x8251e410", "FM2_Render_TestPassVisibilityVMXCore"),
    ("0x82523020", "FM2_Render_SortVisibleRenderablesPartitionTail"),
    ("0x82526490", "FM2_Render_SortVisibleRenderablesInitHeap"),
    ("0x82526a88", "FM2_Render_SortVisibleRenderablesInsertTail"),
    ("0x82527c60", "FM2_Render_SortVisibleRenderablesBodyTail"),
    ("0x8252bbb8", "FM2_Render_GetDistanceKeyFromPassSlotCore"),
    ("0x8252d170", "FM2_Render_UpdateObjectDistanceKeysTail"),
    ("0x82587788", "FM2_Network_DispatchMessageFromQueueLockedTail"),
    ("0x8258cba0", "FM2_Set_LowerBoundByKeyInTreeTail"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass82.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 82 (33 functions)\n",
    "Newly exposed callees from passes 80–81: presentation slot vector, car presentation dtor, render sort/visibility, D3D validate.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass82.md", "w", encoding="utf-8").write("\n".join(md))
