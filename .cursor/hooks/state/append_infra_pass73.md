### Infrastructure pass 73 (33 functions)

SQLite hash buckets, graphics stream delete, audio mix/render, image TGA/DDS/JPEG/PNG, shader constants, D3D upload/caps.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821d4300` | `FM2_SQLite_HashBucketSetHead` | Evidence from decompile and caller context. |
| `0x82279880` | `FM2_SQLite_HashEntryAllocNode` | Evidence from decompile and caller context. |
| `0x8221fab0` | `FM2_GraphicsStreamList_DeleteQueryCallback` | Evidence from decompile and caller context. |
| `0x82230e78` | `FM2_GraphicsStreamList_DeleteQueryByIdBody` | Evidence from decompile and caller context. |
| `0x822633c8` | `FM2_ComObject_InitRefCountAggregateFields` | Evidence from decompile and caller context. |
| `0x82335f10` | `FM2_Audio_VolumeListFindNodeByPrefixWalk` | Evidence from decompile and caller context. |
| `0x8237ccf0` | `FM2_AudioMix_SubmitPendingOutputWritePackets` | Evidence from decompile and caller context. |
| `0x82387138` | `FM2_AudioRenderFrame_EnqueueD3DCommandBody` | Evidence from decompile and caller context. |
| `0x82388ab8` | `FM2_D3D_CopyDefaultSurfaceDescriptor` | Evidence from decompile and caller context. |
| `0x82388b50` | `FM2_Image_ParseTgaFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238b9f8` | `FM2_Image_ParseTgaPaletteFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238ccc0` | `FM2_Image_ParseDDSFromMemory_ReadHeader` | Evidence from decompile and caller context. |
| `0x8238e9d0` | `FM2_D3D_TextureDesc_FromFormat_ResolveHandler` | Evidence from decompile and caller context. |
| `0x8239f3c0` | `FM2_Image_DecodeJpegFromMemory_InitContext` | Evidence from decompile and caller context. |
| `0x8239f808` | `FM2_Image_DecodeJpegFromMemory_ReadScanlines` | Evidence from decompile and caller context. |
| `0x8239f960` | `FM2_Image_DecodeJpegFromMemory_AllocOutput` | Evidence from decompile and caller context. |
| `0x8239fae0` | `FM2_Image_DecodeJpegFromMemory_ConvertColorSpace` | Evidence from decompile and caller context. |
| `0x8239fbe0` | `FM2_Image_DecodeJpegFromMemory_WritePixels` | Evidence from decompile and caller context. |
| `0x823b0940` | `FM2_Image_LoadPngValidateChunkContinuation` | Evidence from decompile and caller context. |
| `0x821f2f58` | `FM2_CareerRace_GetEndRaceTimerSeedPtr` | Evidence from decompile and caller context. |
| `0x822643a8` | `FM2_ComObject_RefCountIncrementOne` | Evidence from decompile and caller context. |
| `0x82270bf8` | `FM2_ComObject_RefCountNoOpRet` | Evidence from decompile and caller context. |
| `0x8229b650` | `FM2_AudioSample_BuildOutputPairDescriptorAdvanceIter` | Evidence from decompile and caller context. |
| `0x82790498` | `FM2_XtsClient_SendRequestPacket_NoOpStub` | Evidence from decompile and caller context. |
| `0x82789b48` | `FM2_XtsClient_SendRequestPacket_CompareFlagStub` | Evidence from decompile and caller context. |
| `0x823b0aa0` | `FM2_Shader_ApplyConstantsBatchWriteSlotA` | Evidence from decompile and caller context. |
| `0x823b0d08` | `FM2_Shader_ApplyConstantsBatchWriteSlotB` | Evidence from decompile and caller context. |
| `0x823b1050` | `FM2_Shader_ApplyConstantsBatchWriteSlotC` | Evidence from decompile and caller context. |
| `0x823bf878` | `FM2_D3D_BuildTextureUploadDescriptorInit` | Evidence from decompile and caller context. |
| `0x823bf968` | `FM2_D3D_BuildTextureUploadDescriptorBody` | Evidence from decompile and caller context. |
| `0x823c0928` | `FM2_D3D_ComputeTexturePitchAligned` | Evidence from decompile and caller context. |
| `0x823cd4b8` | `FM2_D3D_GetDeviceCapsBody` | Evidence from decompile and caller context. |
| `0x823ce2e8` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_Impl` | Evidence from decompile and caller context. |