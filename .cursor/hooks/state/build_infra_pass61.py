import json

RENAMES = [
    ("0x8224c240", "FM2_LiveryMask_FinalizePendingEntrySlot"),
    ("0x8224e230", "FM2_LiveryMask_ProcessPendingEntryBatch"),
    ("0x8224ee38", "FM2_LiveryMask_ResetPendingEntryState"),
    ("0x822515f0", "FM2_XmlReader_GrowAttrTableCapacity"),
    ("0x82252468", "FM2_LiveryMask_MergePendingEntryLists"),
    ("0x82254a58", "FM2_RaceGhost_TraversePlaybackNodeTree"),
    ("0x822551b8", "FM2_RaceGhost_GetPlaybackNodeChildCount"),
    ("0x82255c50", "FM2_ComObject_InitRefCountSubobjectFields"),
    ("0x82257148", "FM2_ComObject_BindRefCountVtableChain"),
    ("0x82257eb0", "FM2_ComObject_InitRefCountCallbackFields"),
    ("0x8225ef80", "FM2_ComObject_InitRefCountFieldsFromSourceCore"),
    ("0x822939b0", "FM2_Profile_SetTuningDisplayNameParseBody"),
    ("0x822943a0", "FM2_Profile_SetTuningDisplayNameValidateBody"),
    ("0x822963d8", "FM2_Profile_SetTuningDisplayNameCommitBody"),
    ("0x823428f0", "FM2_ComObject_InitRefCountAggregateBody"),
    ("0x82345880", "FM2_LuaGarage_EnsureCarRecordLookupBody"),
    ("0x82345960", "FM2_LuaGarage_EnsureCarRecordFieldCopy"),
    ("0x823468b0", "FM2_LuaGarage_EnsureCarRecordFieldInit"),
    ("0x823b1ca0", "FM2_Image_LoadPngFromMemory_ReadIdatHeader"),
    ("0x823b1e10", "FM2_Image_LoadPngFromMemory_DecompressIdatChunk"),
    ("0x823b2048", "FM2_Image_LoadPngFromMemory_ValidateChunkCrc"),
    ("0x823bfbb0", "FM2_D3D_ConvertSurfaceFormatToD3d"),
    ("0x823bfcd8", "FM2_D3D_CreateTextureFromSurfaceLevelInner"),
    ("0x823c04d0", "FM2_D3D_UploadTextureSurfaceLevels"),
    ("0x823c0ae8", "FM2_D3D_BuildTextureUploadDescriptor"),
    ("0x823c0dc8", "FM2_D3D_ComputeTexturePitchAndSize"),
    ("0x8240dcc8", "FM2_Render_GetPassLightingGlobalStatePtr"),
    ("0x8242a3f0", "FM2_Render_AppendPassLightingSubscriber"),
    ("0x82457710", "FM2_Render_BindPassLightingSubscriberParams"),
    ("0x82469be8", "FM2_Render_InsertPassLightingTreeNode"),
    ("0x824806e8", "FM2_Render_UpdatePassLightingCoeffSlot"),
    ("0x82480780", "FM2_Render_InterpolatePassLightingScalar"),
    ("0x82484bf0", "FM2_Render_ResetPassLightingSlotState"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass61.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 61 (33 functions)\n",
    "Livery mask tail, com-object ref-count, profile/garage, D3D texture upload, pass-lighting subscribers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass61.md", "w", encoding="utf-8").write("\n".join(md))
