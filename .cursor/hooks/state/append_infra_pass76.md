### Infrastructure pass 76 (33 functions)

Com-object aggregate, D3D texture resource create/upload, audio render D3D packet writers, shader validate, JPEG.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824f18b0` | `FM2_ComObject_GetAggregateFieldAt60` | Evidence from decompile and caller context. |
| `0x8222e2a0` | `FM2_ComObject_InitCarPlaybackVectorDefaults` | Evidence from decompile and caller context. |
| `0x822625d0` | `FM2_ComObject_InitRefCountAggregateSetFlagBody` | Evidence from decompile and caller context. |
| `0x82268728` | `FM2_CareerRace_UpdatePlaybackTimerComputeFrame` | Evidence from decompile and caller context. |
| `0x82270060` | `FM2_ComObject_InitRefCountAggregateLinkNodeBody` | Evidence from decompile and caller context. |
| `0x823638f0` | `FM2_TestTmp2_InvokeBody` | Evidence from decompile and caller context. |
| `0x8236a290` | `FM2_D3D_ComputeMipCountFromResourceDesc` | Evidence from decompile and caller context. |
| `0x8236b628` | `FM2_D3D_GatherVolumeMetadataFromDescInner` | Evidence from decompile and caller context. |
| `0x8236be90` | `FM2_D3D_GatherVolumeMetadataFromDescThunk` | Evidence from decompile and caller context. |
| `0x823851a8` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketA` | Evidence from decompile and caller context. |
| `0x82388798` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketB` | Evidence from decompile and caller context. |
| `0x823890f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketC` | Evidence from decompile and caller context. |
| `0x82389158` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketD` | Evidence from decompile and caller context. |
| `0x8238af50` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketE` | Evidence from decompile and caller context. |
| `0x8238b7d0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketF` | Evidence from decompile and caller context. |
| `0x8238c488` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketG` | Evidence from decompile and caller context. |
| `0x8238d390` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketH` | Evidence from decompile and caller context. |
| `0x8238d8c0` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketI` | Evidence from decompile and caller context. |
| `0x8238da88` | `FM2_AudioRenderFrame_EnqueueD3DCommandWritePacketJ` | Evidence from decompile and caller context. |
| `0x8239ef08` | `FM2_AudioRenderFrame_EnqueueD3DCommandFinalize` | Evidence from decompile and caller context. |
| `0x8239f4e0` | `FM2_Image_DecodeJpegFromMemory_AllocComponentBuffer` | Evidence from decompile and caller context. |
| `0x823c4958` | `FM2_Shader_ApplyConstantsBatchValidateSlotCheck` | Evidence from decompile and caller context. |
| `0x823c49b8` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyA` | Evidence from decompile and caller context. |
| `0x823c5488` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyB` | Evidence from decompile and caller context. |
| `0x823c5568` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyC` | Evidence from decompile and caller context. |
| `0x823c5728` | `FM2_Shader_ApplyConstantsBatchValidateSlotGuard` | Evidence from decompile and caller context. |
| `0x823c5758` | `FM2_Shader_ApplyConstantsBatchValidateSlotBodyD` | Evidence from decompile and caller context. |
| `0x823cce40` | `FM2_D3D_CreateTextureResourceFromFormatAlloc` | Evidence from decompile and caller context. |
| `0x823ce7a8` | `FM2_D3D_CreateTextureResourceFromFormatUploadA` | Evidence from decompile and caller context. |
| `0x823cea48` | `FM2_D3D_CreateTextureResourceFromFormatUploadB` | Evidence from decompile and caller context. |
| `0x823d0920` | `FM2_D3D_CreateTextureResourceFromFormatUploadCore` | Evidence from decompile and caller context. |
| `0x823d14d0` | `FM2_D3D_CreateTextureResourceFromFormatCleanup` | Evidence from decompile and caller context. |
| `0x8245ca78` | `FM2_ComObject_FormatCarIdSqlAppend` | Evidence from decompile and caller context. |