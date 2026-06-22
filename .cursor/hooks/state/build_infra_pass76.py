import json

RENAMES = [
    ("0x824f18b0", "FM2_ComObject_GetAggregateFieldAt60"),
    ("0x8222e2a0", "FM2_ComObject_InitCarPlaybackVectorDefaults"),
    ("0x822625d0", "FM2_ComObject_InitRefCountAggregateSetFlagBody"),
    ("0x82268728", "FM2_CareerRace_UpdatePlaybackTimerComputeFrame"),
    ("0x82270060", "FM2_ComObject_InitRefCountAggregateLinkNodeBody"),
    ("0x823638f0", "FM2_TestTmp2_InvokeBody"),
    ("0x8236a290", "FM2_D3D_ComputeMipCountFromResourceDesc"),
    ("0x8236b628", "FM2_D3D_GatherVolumeMetadataFromDescInner"),
    ("0x8236be90", "FM2_D3D_GatherVolumeMetadataFromDescThunk"),
    ("0x823851a8", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketA"),
    ("0x82388798", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketB"),
    ("0x823890f0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketC"),
    ("0x82389158", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketD"),
    ("0x8238af50", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketE"),
    ("0x8238b7d0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketF"),
    ("0x8238c488", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketG"),
    ("0x8238d390", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketH"),
    ("0x8238d8c0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketI"),
    ("0x8238da88", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJ"),
    ("0x8239ef08", "FM2_AudioRenderFrame_EnqueueD3DCommandFinalize"),
    ("0x8239f4e0", "FM2_Image_DecodeJpegFromMemory_AllocComponentBuffer"),
    ("0x823c4958", "FM2_Shader_ApplyConstantsBatchValidateSlotCheck"),
    ("0x823c49b8", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyA"),
    ("0x823c5488", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyB"),
    ("0x823c5568", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyC"),
    ("0x823c5728", "FM2_Shader_ApplyConstantsBatchValidateSlotGuard"),
    ("0x823c5758", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyD"),
    ("0x823cce40", "FM2_D3D_CreateTextureResourceFromFormatAlloc"),
    ("0x823ce7a8", "FM2_D3D_CreateTextureResourceFromFormatUploadA"),
    ("0x823cea48", "FM2_D3D_CreateTextureResourceFromFormatUploadB"),
    ("0x823d0920", "FM2_D3D_CreateTextureResourceFromFormatUploadCore"),
    ("0x823d14d0", "FM2_D3D_CreateTextureResourceFromFormatCleanup"),
    ("0x8245ca78", "FM2_ComObject_FormatCarIdSqlAppend"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass76.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 76 (33 functions)\n",
    "Com-object aggregate, D3D texture resource create/upload, audio render D3D packet writers, shader validate, JPEG.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass76.md", "w", encoding="utf-8").write("\n".join(md))
