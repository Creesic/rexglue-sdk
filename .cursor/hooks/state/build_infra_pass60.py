import json

RENAMES = [
    ("0x823b1500", "FM2_Image_LoadPngReadChunkBytesToBuffer"),
    ("0x827281d0", "FM2_Render_ComputePassLightingResourceOffset64B"),
    ("0x8239f2f8", "FM2_Image_LoadPngValidateStreamAfterRead"),
    ("0x82725ec0", "FM2_Render_ReciprocalSinScaleFloat"),
    ("0x8238efc0", "FM2_D3D_ReleaseTextureSurfacePairTagged"),
    ("0x82391c80", "FM2_D3D_InitTextureDescFromFormat"),
    ("0x823cd8d0", "FM2_D3D_GetDeviceCapsThunk"),
    ("0x823cd8d8", "FM2_D3D_QueryTextureResourceType"),
    ("0x823cdbc8", "FM2_D3D_ComputeMipLevelCount"),
    ("0x823d3680", "FM2_D3D_ClearComPtrPair"),
    ("0x8240b998", "FM2_Render_ZeroPassLightingCacheLinesVMX"),
    ("0x824a72b8", "FM2_Render_HasPassLightingResourceBound"),
    ("0x824d1178", "FM2_Input_BuildControllerSslBindingEntry"),
    ("0x824e31b0", "FM2_ComObject_AllocSharedStateBuffer"),
    ("0x826115d8", "FM2_Vector_ReallocGrow16ByteElements"),
    ("0x82727dd0", "FM2_Render_ComputePassLightingSlotOffset64B"),
    ("0x82728088", "FM2_Render_GetPassLightingWorkerSlotIndex"),
    ("0x827280a0", "FM2_Render_CopyPassLightingPairHead"),
    ("0x82728280", "FM2_Render_TestPassLightingSlotIndexValid"),
    ("0x82728378", "FM2_Render_GetPassLightingSlotDataPtr"),
    ("0x827261f0", "FM2_Render_SinRadiansDouble"),
    ("0x821d28f8", "FM2_Input_ControllerSslBindingInitField"),
    ("0x821f4c50", "FM2_ComObject_GetRefCountField"),
    ("0x8221cd00", "FM2_RaceGhost_ComparePlaybackNodeKey"),
    ("0x8222f5e8", "FM2_RaceGhost_LoadPlaybackResourcePath"),
    ("0x82231848", "FM2_RaceGhost_ParsePlaybackMetadataBlock"),
    ("0x82236250", "FM2_RaceGhost_BuildPlaybackSampleTable"),
    ("0x82249ae0", "FM2_LiveryMask_UpdateEntryFlagsField"),
    ("0x8224a7b8", "FM2_LiveryMask_ProcessPendingLayerEntry"),
    ("0x8224b400", "FM2_LiveryMask_ReleasePendingEntryRef"),
    ("0x8224b850", "FM2_LiveryMask_QueuePendingEntryUpdate"),
    ("0x8224b910", "FM2_LiveryMask_ApplyPendingEntryTransform"),
    ("0x8224c0a8", "FM2_LiveryMask_ClearPendingEntrySlot"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass60.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 60 (33 functions)\n",
    "PNG read/validate, pass-lighting offsets/VMX, D3D texture helpers, race ghost, livery mask.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass60.md", "w", encoding="utf-8").write("\n".join(md))
