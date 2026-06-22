import json

RENAMES = [
    ("0x82296968", "FM2_Profile_SetTuningDisplayNameInner"),
    ("0x82346b68", "FM2_LuaGarage_EnsureCarRecordField92Body"),
    ("0x8235f3d8", "FM2_Input_InitControllerDevicesBody"),
    ("0x823628a8", "FM2_Input_ControllerDevice_InitSslBindingsBody"),
    ("0x82365ab8", "FM2_Vector_ComputeEraseRangeSpan16"),
    ("0x82365bc8", "FM2_Vector_EraseBegin20ByteElementsImpl"),
    ("0x8236b010", "FM2_AudioRender_SampleFrontBufferRegionBody"),
    ("0x8236da60", "FM2_Render_ObjectPassPrefetchDrawBatch"),
    ("0x823733b8", "FM2_Render_ScopedBatch_FinalizeGpuKickBody"),
    ("0x82376598", "FM2_Render_MarkDrawListStateDirty"),
    ("0x8237dfd8", "FM2_D3D_ReleaseGpuResourceRefInner"),
    ("0x8237a320", "FM2_GpuKick_SubmitVdScalerCommandBufferBody"),
    ("0x8237b1a0", "FM2_GpuKick_CreatePixCaptureFileOnUsbBody"),
    ("0x8237bd48", "FM2_GpuKick_NotifyPixCaptureFileEndedBody"),
    ("0x8237d158", "FM2_AudioMix_SubmitPendingOutputBody"),
    ("0x8237f4d8", "FM2_GpuCommandBuffer_BeginPerfCaptureBody"),
    ("0x82385aa0", "FM2_D3D_CreateTextureFromSurfaceLevelBody"),
    ("0x82386130", "FM2_D3D_CreateTextureFromSurfaceLevelBodyB"),
    ("0x823868d8", "FM2_D3D_CreateTextureFromSurfaceLevelBodyC"),
    ("0x823876d8", "FM2_D3D_CreateTextureFromSurfaceLevelBodyD"),
    ("0x8238e098", "FM2_D3D_CreateTextureFromSurfaceLevelBodyE"),
    ("0x8239e508", "FM2_Render_SetPassLightingModeScalarBodyA"),
    ("0x8239e6c8", "FM2_Render_SetPassLightingModeScalarBodyB"),
    ("0x823a46b0", "FM2_Shader_ApplyConstantsBatchBody"),
    ("0x823a6cf0", "FM2_Png_EnsureRgbThenDecodeRowBody"),
    ("0x823a7018", "FM2_Image_LoadPngFromMemory_InitHeader"),
    ("0x823a9510", "FM2_Png_DecodeRowScalarThunk"),
    ("0x823a9520", "FM2_Png_AllocDecodeStateBufferBody"),
    ("0x823b1490", "FM2_Image_LoadPngFromMemory_ParseChunkHeader"),
    ("0x823b15f8", "FM2_Image_LoadPngFromMemory_DecodeIdatBody"),
    ("0x823b18c0", "FM2_Image_LoadPngFromMemory_FilterRowBody"),
    ("0x823b1a80", "FM2_Image_LoadPngFromMemory_ValidateSigBody"),
    ("0x823b1b00", "FM2_Image_LoadPngFromMemory_ReadChunkBody"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass59.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 59 (33 functions)\n",
    "Profile/garage/input, render GPU kick, D3D texture/PNG decode, shader constants.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass59.md", "w", encoding="utf-8").write("\n".join(md))
