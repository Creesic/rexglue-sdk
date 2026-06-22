### Infrastructure pass 78 (33 functions)

Audio render D3D packet writers F/G/J, D3D texture upload core, shader validate shared bodies.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82388578` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketShared` | Evidence from decompile and caller context. |
| `0x824d8ad0` | `FM2_Lua_PushSslValueFromTableCell` | Evidence from decompile and caller context. |
| `0x8238d860` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketIJShared` | Evidence from decompile and caller context. |
| `0x823c5018` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyShared` | Evidence from decompile and caller context. |
| `0x823d2120` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketEHShared` | Evidence from decompile and caller context. |
| `0x821e70c8` | `FM2_ComObject_InitRefCountAggregateLinkNodeInit` | Evidence from decompile and caller context. |
| `0x823850f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketAPre` | Evidence from decompile and caller context. |
| `0x8238e8c0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketDPre` | Evidence from decompile and caller context. |
| `0x8238e9b8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketCPre` | Evidence from decompile and caller context. |
| `0x823a23e0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyA` | Evidence from decompile and caller context. |
| `0x823a2580` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyB` | Evidence from decompile and caller context. |
| `0x823a2728` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyC` | Evidence from decompile and caller context. |
| `0x823a2808` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyD` | Evidence from decompile and caller context. |
| `0x823a33c8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketFBodyE` | Evidence from decompile and caller context. |
| `0x823a3588` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyA` | Evidence from decompile and caller context. |
| `0x823a3620` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyB` | Evidence from decompile and caller context. |
| `0x823a3d60` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyC` | Evidence from decompile and caller context. |
| `0x823a3e60` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyD` | Evidence from decompile and caller context. |
| `0x823a3ea0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyE` | Evidence from decompile and caller context. |
| `0x823a3f40` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketGBodyF` | Evidence from decompile and caller context. |
| `0x823c58b8` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyAInner` | Evidence from decompile and caller context. |
| `0x823cf070` | `FM2_D3D_CreateTextureResourceUploadCoreInitA` | Evidence from decompile and caller context. |
| `0x823cf100` | `FM2_D3D_CreateTextureResourceUploadCoreBodyA` | Evidence from decompile and caller context. |
| `0x823cf428` | `FM2_D3D_CreateTextureResourceUploadCoreBodyB` | Evidence from decompile and caller context. |
| `0x823cfea8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyC` | Evidence from decompile and caller context. |
| `0x823cff30` | `FM2_D3D_CreateTextureResourceUploadCoreBodyD` | Evidence from decompile and caller context. |
| `0x823d00d8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyE` | Evidence from decompile and caller context. |
| `0x823d0218` | `FM2_D3D_CreateTextureResourceUploadCoreBodyF` | Evidence from decompile and caller context. |
| `0x823d1768` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerA` | Evidence from decompile and caller context. |
| `0x823d1a70` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDescInnerB` | Evidence from decompile and caller context. |
| `0x823d21a8` | `FM2_D3D_CreateTextureResourceUploadCoreBodyG` | Evidence from decompile and caller context. |
| `0x82418328` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyA` | Evidence from decompile and caller context. |
| `0x824186d0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJBodyB` | Evidence from decompile and caller context. |