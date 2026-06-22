### Infrastructure pass 28 (33 functions)

GPU kick/scaler, audio pump PM4, PNG/zlib/image convert, compression stream, metrics/FMOD.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823743d0` | `FM2_Render_EndCaptureReleaseSurfaces` | Evidence from decompile and caller context. |
| `0x823816c8` | `FM2_Render_WaitForGpuWorkerEvents` | Evidence from decompile and caller context. |
| `0x8237f2e8` | `FM2_GpuKick_SubmitViewportConstant3841` | Evidence from decompile and caller context. |
| `0x8237a888` | `FM2_GpuKick_SubmitVdScalerCommandBuffer` | Builds VdInitializeScaler PM4 packet for viewport blit. |
| `0x8237ab08` | `FM2_GpuKick_RetrainEdramAndFlushPm4` | Evidence from decompile and caller context. |
| `0x8237c988` | `FM2_GpuKick_NotifyPixCaptureFileEnded` | Evidence from decompile and caller context. |
| `0x823818d8` | `FM2_AudioPumpThread_DispatchPm4Commands` | Evidence from decompile and caller context. |
| `0x82389210` | `FM2_D3DXTex_Image_DtorReleaseLevels` | Evidence from decompile and caller context. |
| `0x8238ed18` | `FM2_Image_ConvertFloatRowTo565BE` | Evidence from decompile and caller context. |
| `0x8238ee10` | `FM2_Image_ConvertFloatRowTo565LE` | Evidence from decompile and caller context. |
| `0x823a41e0` | `FM2_Png_CreateReadStructFromCallbacks` | Evidence from decompile and caller context. |
| `0x823a4cf0` | `FM2_Png_DestroyReadStructsTriple` | Evidence from decompile and caller context. |
| `0x823a4fe8` | `FM2_Png_SetPaletteToRgbFlag` | Evidence from decompile and caller context. |
| `0x823a5018` | `FM2_Png_SetGrayToRgbAndScale` | Evidence from decompile and caller context. |
| `0x823a51f8` | `FM2_Png_SetAspectRatioMismatchFlag` | Evidence from decompile and caller context. |
| `0x823a5238` | `FM2_Png_SetRgbToGrayFlag` | Evidence from decompile and caller context. |
| `0x823a7038` | `FM2_Png_SetWriteFnAndClearOld` | Evidence from decompile and caller context. |
| `0x823ab428` | `FM2_Shader_InitHuffmanCallbackTable` | Evidence from decompile and caller context. |
| `0x823aee90` | `FM2_Zlib_ReadBitsFromInput` | Evidence from decompile and caller context. |
| `0x823af088` | `FM2_Zlib_FillDeflateWindowFromInput` | Zlib deflate: slides window and copies input from next_in. |
| `0x823b4538` | `FM2_Png_HuffmanDecodeSymbol` | Evidence from decompile and caller context. |
| `0x823c1590` | `FM2_ComObject_SyncChildProperties` | Evidence from decompile and caller context. |
| `0x823c81a8` | `FM2_Render_BindPassSurfacesForKick` | Evidence from decompile and caller context. |
| `0x823c8328` | `FM2_Render_ResolvePassGpuMemoryBlocks` | Evidence from decompile and caller context. |
| `0x823cdc20` | `FM2_D3D_BltRegionToSurface` | Evidence from decompile and caller context. |
| `0x823d3b38` | `FM2_Image_ConvertFloatRowTo555BE` | Evidence from decompile and caller context. |
| `0x823d3c38` | `FM2_Image_ConvertFloatRowTo555LE` | Evidence from decompile and caller context. |
| `0x823d3d30` | `FM2_Image_ConvertFloatRowTo4444` | Evidence from decompile and caller context. |
| `0x82412470` | `FM2_Metrics_InsertOrRemoveGlobalNode` | Evidence from decompile and caller context. |
| `0x82413fa8` | `FM2_FMOD_InitSinLookupTable` | Evidence from decompile and caller context. |
| `0x8242a7c0` | `FM2_CompressionStream_InitListHead` | Evidence from decompile and caller context. |
| `0x8242ac50` | `FM2_CompressionStream_ResetAndClearPending` | Evidence from decompile and caller context. |
| `0x8242b7f0` | `FM2_CompressionStream_Dtor` | Evidence from decompile and caller context. |