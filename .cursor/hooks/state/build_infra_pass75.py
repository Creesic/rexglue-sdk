import json

RENAMES = [
    ("0x82392f88", "FM2_D3D_TextureDesc_SelectFormatHandlerA"),
    ("0x82393380", "FM2_D3D_TextureDesc_SelectFormatHandlerB"),
    ("0x8239c1d8", "FM2_D3D_TextureDesc_SelectFormatHandlerC"),
    ("0x8239cbc0", "FM2_D3D_TextureDesc_SelectFormatHandlerD"),
    ("0x8239f6c8", "FM2_Image_DecodeJpegFromMemory_AllocScanBuffer"),
    ("0x8239fa28", "FM2_Image_DecodeJpegFromMemory_WriteRowPixels"),
    ("0x823a2088", "FM2_Image_DecodeJpegFromMemory_InitDecompress"),
    ("0x823a5080", "FM2_Shader_ApplyConstantsBatchFlushGuard"),
    ("0x823a50c8", "FM2_Shader_ApplyConstantsBatchFlushWriteA"),
    ("0x823a53d8", "FM2_Shader_ApplyConstantsBatchFlushWriteB"),
    ("0x823a5550", "FM2_Shader_ApplyConstantsBatchFlushWriteC"),
    ("0x823a5790", "FM2_Shader_ApplyConstantsBatchFlushCheckSlot"),
    ("0x823a57f0", "FM2_Shader_ApplyConstantsBatchFlushWriteD"),
    ("0x823a5bc8", "FM2_Shader_ApplyConstantsBatchFlushWriteE"),
    ("0x823a6030", "FM2_Shader_ApplyConstantsBatchFlushWriteF"),
    ("0x823a62e8", "FM2_Shader_ApplyConstantsBatchFlushWriteG"),
    ("0x823a6800", "FM2_Shader_ApplyConstantsBatchFlushWriteH"),
    ("0x823a9df0", "FM2_Image_DecodeJpegFromMemory_SetErrorHandler"),
    ("0x823b20b8", "FM2_Shader_ApplyConstantsBatchBodyInner"),
    ("0x823b7148", "FM2_Jpeg_InitColorSpaceConverterBody"),
    ("0x823c1740", "FM2_Shader_ApplyConstantsBatchValidateSlotBody"),
    ("0x823cd260", "FM2_D3D_GetDeviceCapsQuerySurfaceFormats"),
    ("0x823d1ea0", "FM2_D3D_CreateTextureFromSurfaceLevelBodyE_Impl"),
    ("0x82413e98", "FM2_AIDriver_ResetRaceLineStateClearSector"),
    ("0x82413ed8", "FM2_RaceGhost_QueryPartLevelForRarityBonus"),
    ("0x82418670", "FM2_Image_ParsePPMFromMemory_ReadDigit"),
    ("0x824186b0", "FM2_Image_ParsePPMFromMemory_SkipWhitespace"),
    ("0x8241cfe0", "FM2_LuaSyntax_CoalesceStringConcatExpBody"),
    ("0x82438c10", "FM2_LuaGarage_EnsureCarRecordFieldCopyBody"),
    ("0x82454290", "FM2_AudioSample_BuildOutputPairDescriptorValidateBody"),
    ("0x82464f70", "FM2_AIDriver_ResetRaceLineStateClearProgress"),
    ("0x82483740", "FM2_AIDriver_ResetRaceLineOnSectorChangeClamp"),
    ("0x82492e68", "FM2_AIDriver_ComputeSectorIndexFromProgressBody"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass75.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 75 (33 functions)\n",
    "D3D format handlers, JPEG/shader constant flush cluster, AI race line, Lua/audio helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass75.md", "w", encoding="utf-8").write("\n".join(md))
