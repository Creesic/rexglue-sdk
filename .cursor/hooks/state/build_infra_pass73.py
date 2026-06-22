import json

RENAMES = [
    ("0x821d4300", "FM2_SQLite_HashBucketSetHead"),
    ("0x82279880", "FM2_SQLite_HashEntryAllocNode"),
    ("0x8221fab0", "FM2_GraphicsStreamList_DeleteQueryCallback"),
    ("0x82230e78", "FM2_GraphicsStreamList_DeleteQueryByIdBody"),
    ("0x822633c8", "FM2_ComObject_InitRefCountAggregateFields"),
    ("0x82335f10", "FM2_Audio_VolumeListFindNodeByPrefixWalk"),
    ("0x8237ccf0", "FM2_AudioMix_SubmitPendingOutputWritePackets"),
    ("0x82387138", "FM2_AudioRenderFrame_EnqueueD3DCommandBody"),
    ("0x82388ab8", "FM2_D3D_CopyDefaultSurfaceDescriptor"),
    ("0x82388b50", "FM2_Image_ParseTgaFromMemory_ReadHeader"),
    ("0x8238b9f8", "FM2_Image_ParseTgaPaletteFromMemory_ReadHeader"),
    ("0x8238ccc0", "FM2_Image_ParseDDSFromMemory_ReadHeader"),
    ("0x8238e9d0", "FM2_D3D_TextureDesc_FromFormat_ResolveHandler"),
    ("0x8239f3c0", "FM2_Image_DecodeJpegFromMemory_InitContext"),
    ("0x8239f808", "FM2_Image_DecodeJpegFromMemory_ReadScanlines"),
    ("0x8239f960", "FM2_Image_DecodeJpegFromMemory_AllocOutput"),
    ("0x8239fae0", "FM2_Image_DecodeJpegFromMemory_ConvertColorSpace"),
    ("0x8239fbe0", "FM2_Image_DecodeJpegFromMemory_WritePixels"),
    ("0x823b0940", "FM2_Image_LoadPngValidateChunkContinuation"),
    ("0x821f2f58", "FM2_CareerRace_GetEndRaceTimerSeedPtr"),
    ("0x822643a8", "FM2_ComObject_RefCountIncrementOne"),
    ("0x82270bf8", "FM2_ComObject_RefCountNoOpRet"),
    ("0x8229b650", "FM2_AudioSample_BuildOutputPairDescriptorAdvanceIter"),
    ("0x82790498", "FM2_XtsClient_SendRequestPacket_NoOpStub"),
    ("0x82789b48", "FM2_XtsClient_SendRequestPacket_CompareFlagStub"),
    ("0x823b0aa0", "FM2_Shader_ApplyConstantsBatchWriteSlotA"),
    ("0x823b0d08", "FM2_Shader_ApplyConstantsBatchWriteSlotB"),
    ("0x823b1050", "FM2_Shader_ApplyConstantsBatchWriteSlotC"),
    ("0x823bf878", "FM2_D3D_BuildTextureUploadDescriptorInit"),
    ("0x823bf968", "FM2_D3D_BuildTextureUploadDescriptorBody"),
    ("0x823c0928", "FM2_D3D_ComputeTexturePitchAligned"),
    ("0x823cd4b8", "FM2_D3D_GetDeviceCapsBody"),
    ("0x823ce2e8", "FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Impl"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass73.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 73 (33 functions)\n",
    "SQLite hash buckets, graphics stream delete, audio mix/render, image TGA/DDS/JPEG/PNG, shader constants, D3D upload/caps.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass73.md", "w", encoding="utf-8").write("\n".join(md))
