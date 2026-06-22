### Infrastructure pass 30 (33 functions)

GPU shader constants/gamma, PIX USB capture, audio pump PM4, PNG/JPEG init, Lua unwind, FMOD sin.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82377580` | `FM2_GpuKick_SubmitFloatShaderConstantsPm4` | Emits PM4 float shader constant fetch bundle (6437/6434 packets). |
| `0x82377660` | `FM2_GpuKick_SubmitFixedShaderConstantsPm4` | Evidence from decompile and caller context. |
| `0x82377750` | `FM2_GpuKick_BuildLinearGammaRampTable` | Evidence from decompile and caller context. |
| `0x823777a8` | `FM2_GpuKick_BuildPwlGammaRampTable` | Evidence from decompile and caller context. |
| `0x82378d58` | `FM2_GpuKick_ComputeScalerViewportRects` | Evidence from decompile and caller context. |
| `0x8237c5e8` | `FM2_GpuKick_CreatePixCaptureFileOnUsb` | Evidence from decompile and caller context. |
| `0x82381750` | `FM2_AudioPump_SubmitRingBufferMarkerPm4` | Evidence from decompile and caller context. |
| `0x82381850` | `FM2_AudioPump_WaitForBlockerCompletion` | Evidence from decompile and caller context. |
| `0x8239f0d0` | `FM2_Png_SetReadCallbacksOnStruct` | Evidence from decompile and caller context. |
| `0x8239f0e0` | `FM2_Png_FatalErrorShutdown` | Evidence from decompile and caller context. |
| `0x8239f118` | `FM2_Png_InvokeOldWriteFnIfSet` | Evidence from decompile and caller context. |
| `0x823a93d8` | `FM2_Png_AllocReadStructTagged` | Evidence from decompile and caller context. |
| `0x823a9468` | `FM2_Png_AllocChunkBufferTagged` | Evidence from decompile and caller context. |
| `0x823a94d8` | `FM2_Png_FreeChunkBufferIfOwner` | Evidence from decompile and caller context. |
| `0x823a4ba0` | `FM2_Png_DestroyReadStructFull` | Evidence from decompile and caller context. |
| `0x823b0398` | `FM2_Png_ReportError15` | Evidence from decompile and caller context. |
| `0x823ab188` | `FM2_Jpeg_ValidateDecompressState` | Evidence from decompile and caller context. |
| `0x823b2db0` | `FM2_Jpeg_InitSourceManager` | JPEG decompress: allocates and wires libjpeg source manager. |
| `0x823b4010` | `FM2_Jpeg_InitComponentInfoTable` | Evidence from decompile and caller context. |
| `0x823b4e40` | `FM2_Jpeg_InitEntropyDecoder` | Evidence from decompile and caller context. |
| `0x823b5c88` | `FM2_Jpeg_InitHuffmanDecodeTable` | Evidence from decompile and caller context. |
| `0x823b6188` | `FM2_Jpeg_InitSampleBufferTable` | Evidence from decompile and caller context. |
| `0x823b6380` | `FM2_Jpeg_InitUpsampler` | Evidence from decompile and caller context. |
| `0x823b6a20` | `FM2_Jpeg_InitColorConverter` | Evidence from decompile and caller context. |
| `0x823b79a0` | `FM2_Jpeg_InitColorSpaceConverter` | Evidence from decompile and caller context. |
| `0x823b8270` | `FM2_Jpeg_InitUpsampleBufferPaths` | Evidence from decompile and caller context. |
| `0x823bf708` | `FM2_Zlib_CopyInputToSlidingWindow` | Evidence from decompile and caller context. |
| `0x823c4ff8` | `FM2_ComObject_InvokeChildSyncCallback` | Evidence from decompile and caller context. |
| `0x824152c8` | `FM2_Stl_StringIterator_DecrementSafe` | Evidence from decompile and caller context. |
| `0x82417f78` | `FM2_Lua_MathTwoArgCompute` | Evidence from decompile and caller context. |
| `0x82418210` | `FM2_Lua_UnwindAndSetErrorStatus` | Evidence from decompile and caller context. |
| `0x82419cb8` | `FM2_Crt_UnlockHeap` | Evidence from decompile and caller context. |
| `0x82413fa8` | `FM2_FMOD_NormalizeSinLookupInput` | Evidence from decompile and caller context. |