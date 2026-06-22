import json

RENAMES = [
    ("0x82388578", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketShared"),
    ("0x824d8ad0", "FM2_Lua_PushSslValueFromTableCell"),
    ("0x8238d860", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketIJShared"),
    ("0x823c5018", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyShared"),
    ("0x823d2120", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketEHShared"),
    ("0x821e70c8", "FM2_ComObject_InitRefCountAggregateLinkNodeInit"),
    ("0x823850f0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketAPre"),
    ("0x8238e8c0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketDPre"),
    ("0x8238e9b8", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketCPre"),
    ("0x823a23e0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyA"),
    ("0x823a2580", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyB"),
    ("0x823a2728", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyC"),
    ("0x823a2808", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyD"),
    ("0x823a33c8", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyE"),
    ("0x823a3588", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyA"),
    ("0x823a3620", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyB"),
    ("0x823a3d60", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyC"),
    ("0x823a3e60", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyD"),
    ("0x823a3ea0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyE"),
    ("0x823a3f40", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyF"),
    ("0x823c58b8", "FM2_Shader_ApplyConstantsBatchValidateSlotBodyAInner"),
    ("0x823cf070", "FM2_D3D_CreateTextureResourceUploadCoreInitA"),
    ("0x823cf100", "FM2_D3D_CreateTextureResourceUploadCoreBodyA"),
    ("0x823cf428", "FM2_D3D_CreateTextureResourceUploadCoreBodyB"),
    ("0x823cfea8", "FM2_D3D_CreateTextureResourceUploadCoreBodyC"),
    ("0x823cff30", "FM2_D3D_CreateTextureResourceUploadCoreBodyD"),
    ("0x823d00d8", "FM2_D3D_CreateTextureResourceUploadCoreBodyE"),
    ("0x823d0218", "FM2_D3D_CreateTextureResourceUploadCoreBodyF"),
    ("0x823d1768", "FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerA"),
    ("0x823d1a70", "FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerB"),
    ("0x823d21a8", "FM2_D3D_CreateTextureResourceUploadCoreBodyG"),
    ("0x82418328", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyA"),
    ("0x824186d0", "FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyB"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass78.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 78 (33 functions)\n",
    "Audio render D3D packet writers F/G/J, D3D texture upload core, shader validate shared bodies.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass78.md", "w", encoding="utf-8").write("\n".join(md))
