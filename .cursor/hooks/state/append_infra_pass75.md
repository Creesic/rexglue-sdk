### Infrastructure pass 75 (33 functions)

D3D format handlers, JPEG/shader constant flush cluster, AI race line, Lua/audio helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82392f88` | `FM2_D3D_TextureDesc_SelectFormatHandlerA` | Evidence from decompile and caller context. |
| `0x82393380` | `FM2_D3D_TextureDesc_SelectFormatHandlerB` | Evidence from decompile and caller context. |
| `0x8239c1d8` | `FM2_D3D_TextureDesc_SelectFormatHandlerC` | Evidence from decompile and caller context. |
| `0x8239cbc0` | `FM2_D3D_TextureDesc_SelectFormatHandlerD` | Evidence from decompile and caller context. |
| `0x8239f6c8` | `FM2_Image_DecodeJpegFromMemory_AllocScanBuffer` | Evidence from decompile and caller context. |
| `0x8239fa28` | `FM2_Image_DecodeJpegFromMemory_WriteRowPixels` | Evidence from decompile and caller context. |
| `0x823a2088` | `FM2_Image_DecodeJpegFromMemory_InitDecompress` | Evidence from decompile and caller context. |
| `0x823a5080` | `FM2_Shader_ApplyConstantsBatchFlushGuard` | Evidence from decompile and caller context. |
| `0x823a50c8` | `FM2_Shader_ApplyConstantsBatchFlushWriteA` | Evidence from decompile and caller context. |
| `0x823a53d8` | `FM2_Shader_ApplyConstantsBatchFlushWriteB` | Evidence from decompile and caller context. |
| `0x823a5550` | `FM2_Shader_ApplyConstantsBatchFlushWriteC` | Evidence from decompile and caller context. |
| `0x823a5790` | `FM2_Shader_ApplyConstantsBatchFlushCheckSlot` | Evidence from decompile and caller context. |
| `0x823a57f0` | `FM2_Shader_ApplyConstantsBatchFlushWriteD` | Evidence from decompile and caller context. |
| `0x823a5bc8` | `FM2_Shader_ApplyConstantsBatchFlushWriteE` | Evidence from decompile and caller context. |
| `0x823a6030` | `FM2_Shader_ApplyConstantsBatchFlushWriteF` | Evidence from decompile and caller context. |
| `0x823a62e8` | `FM2_Shader_ApplyConstantsBatchFlushWriteG` | Evidence from decompile and caller context. |
| `0x823a6800` | `FM2_Shader_ApplyConstantsBatchFlushWriteH` | Evidence from decompile and caller context. |
| `0x823a9df0` | `FM2_Image_DecodeJpegFromMemory_SetErrorHandler` | Evidence from decompile and caller context. |
| `0x823b20b8` | `FM2_Shader_ApplyConstantsBatchBodyInner` | Evidence from decompile and caller context. |
| `0x823b7148` | `FM2_Jpeg_InitColorSpaceConverterBody` | Evidence from decompile and caller context. |
| `0x823c1740` | `FM2_Shader_ApplyConstantsBatchValidateSlotBody` | Evidence from decompile and caller context. |
| `0x823cd260` | `FM2_D3D_GetDeviceCapsQuerySurfaceFormats` | Evidence from decompile and caller context. |
| `0x823d1ea0` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_Impl` | Evidence from decompile and caller context. |
| `0x82413e98` | `FM2_AIDriver_ResetRaceLineStateClearSector` | Evidence from decompile and caller context. |
| `0x82413ed8` | `FM2_RaceGhost_QueryPartLevelForRarityBonus` | Evidence from decompile and caller context. |
| `0x82418670` | `FM2_Image_ParsePPMFromMemory_ReadDigit` | Evidence from decompile and caller context. |
| `0x824186b0` | `FM2_Image_ParsePPMFromMemory_SkipWhitespace` | Evidence from decompile and caller context. |
| `0x8241cfe0` | `FM2_LuaSyntax_CoalesceStringConcatExpBody` | Evidence from decompile and caller context. |
| `0x82438c10` | `FM2_LuaGarage_EnsureCarRecordFieldCopyBody` | Evidence from decompile and caller context. |
| `0x82454290` | `FM2_AudioSample_BuildOutputPairDescriptorValidateBody` | Evidence from decompile and caller context. |
| `0x82464f70` | `FM2_AIDriver_ResetRaceLineStateClearProgress` | Evidence from decompile and caller context. |
| `0x82483740` | `FM2_AIDriver_ResetRaceLineOnSectorChangeClamp` | Evidence from decompile and caller context. |
| `0x82492e68` | `FM2_AIDriver_ComputeSectorIndexFromProgressBody` | Evidence from decompile and caller context. |