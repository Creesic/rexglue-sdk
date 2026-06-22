### Infrastructure pass 59 (33 functions)

Profile/garage/input, render GPU kick, D3D texture/PNG decode, shader constants.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82296968` | `FM2_Profile_SetTuningDisplayNameInner` | Evidence from decompile and caller context. |
| `0x82346b68` | `FM2_LuaGarage_EnsureCarRecordField92Body` | Evidence from decompile and caller context. |
| `0x8235f3d8` | `FM2_Input_InitControllerDevicesBody` | Evidence from decompile and caller context. |
| `0x823628a8` | `FM2_Input_ControllerDevice_InitSslBindingsBody` | Evidence from decompile and caller context. |
| `0x82365ab8` | `FM2_Vector_ComputeEraseRangeSpan16` | Evidence from decompile and caller context. |
| `0x82365bc8` | `FM2_Vector_EraseBegin20ByteElementsImpl` | Evidence from decompile and caller context. |
| `0x8236b010` | `FM2_AudioRender_SampleFrontBufferRegionBody` | Evidence from decompile and caller context. |
| `0x8236da60` | `FM2_Render_ObjectPassPrefetchDrawBatch` | Evidence from decompile and caller context. |
| `0x823733b8` | `FM2_Render_ScopedBatch_FinalizeGpuKickBody` | Evidence from decompile and caller context. |
| `0x82376598` | `FM2_Render_MarkDrawListStateDirty` | Evidence from decompile and caller context. |
| `0x8237dfd8` | `FM2_D3D_ReleaseGpuResourceRefInner` | Evidence from decompile and caller context. |
| `0x8237a320` | `FM2_GpuKick_SubmitVdScalerCommandBufferBody` | Evidence from decompile and caller context. |
| `0x8237b1a0` | `FM2_GpuKick_CreatePixCaptureFileOnUsbBody` | Evidence from decompile and caller context. |
| `0x8237bd48` | `FM2_GpuKick_NotifyPixCaptureFileEndedBody` | Evidence from decompile and caller context. |
| `0x8237d158` | `FM2_AudioMix_SubmitPendingOutputBody` | Evidence from decompile and caller context. |
| `0x8237f4d8` | `FM2_GpuCommandBuffer_BeginPerfCaptureBody` | Evidence from decompile and caller context. |
| `0x82385aa0` | `FM2_D3D_CreateTextureFromSurfaceLevelBody` | Evidence from decompile and caller context. |
| `0x82386130` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB` | Evidence from decompile and caller context. |
| `0x823868d8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyC` | Evidence from decompile and caller context. |
| `0x823876d8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyD` | Evidence from decompile and caller context. |
| `0x8238e098` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE` | Evidence from decompile and caller context. |
| `0x8239e508` | `FM2_Render_SetPassLightingModeScalarBodyA` | Evidence from decompile and caller context. |
| `0x8239e6c8` | `FM2_Render_SetPassLightingModeScalarBodyB` | Evidence from decompile and caller context. |
| `0x823a46b0` | `FM2_Shader_ApplyConstantsBatchBody` | Evidence from decompile and caller context. |
| `0x823a6cf0` | `FM2_Png_EnsureRgbThenDecodeRowBody` | Evidence from decompile and caller context. |
| `0x823a7018` | `FM2_Image_LoadPngFromMemory_InitHeader` | Evidence from decompile and caller context. |
| `0x823a9510` | `FM2_Png_DecodeRowScalarThunk` | Evidence from decompile and caller context. |
| `0x823a9520` | `FM2_Png_AllocDecodeStateBufferBody` | Evidence from decompile and caller context. |
| `0x823b1490` | `FM2_Image_LoadPngFromMemory_ParseChunkHeader` | Evidence from decompile and caller context. |
| `0x823b15f8` | `FM2_Image_LoadPngFromMemory_DecodeIdatBody` | Evidence from decompile and caller context. |
| `0x823b18c0` | `FM2_Image_LoadPngFromMemory_FilterRowBody` | Evidence from decompile and caller context. |
| `0x823b1a80` | `FM2_Image_LoadPngFromMemory_ValidateSigBody` | Evidence from decompile and caller context. |
| `0x823b1b00` | `FM2_Image_LoadPngFromMemory_ReadChunkBody` | Evidence from decompile and caller context. |