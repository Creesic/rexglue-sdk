### Infrastructure pass 72 (33 functions)

Lua SSL table, D3D texture-desc/surface gather, XTS client, livery/race-ghost/com-object, PNG/shader/XML/SQLite helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824d3838` | `FM2_Lua_PushSslUnitStringsTableBody` | Evidence from decompile and caller context. |
| `0x82388498` | `FM2_Image_StrncpyBounded` | Evidence from decompile and caller context. |
| `0x82392090` | `FM2_D3D_GatherSurfaceMetadataForTextureCreate` | Evidence from decompile and caller context. |
| `0x82392588` | `FM2_D3D_GatherVolumeMetadataForTextureCreate` | Evidence from decompile and caller context. |
| `0x82798410` | `FM2_XtsClientMessageHandler_InitConnectionFields` | Evidence from decompile and caller context. |
| `0x8279f5a0` | `FM2_XtsClient_AccumulatePendingPayloadSizeCore` | Evidence from decompile and caller context. |
| `0x827fc8d0` | `FM2_STL_ListNode_LinkNextWrapper` | Evidence from decompile and caller context. |
| `0x82726540` | `FM2_Render_AbsFloat` | Evidence from decompile and caller context. |
| `0x823901d0` | `FM2_D3D_TextureDesc_FromFormatBodyA` | Evidence from decompile and caller context. |
| `0x82390540` | `FM2_D3D_TextureDesc_FromFormatBodyB` | Evidence from decompile and caller context. |
| `0x82390e08` | `FM2_D3D_TextureDesc_FromFormatBodyC` | Evidence from decompile and caller context. |
| `0x82391598` | `FM2_D3D_TextureDesc_FromFormatBodyD` | Evidence from decompile and caller context. |
| `0x82391e48` | `FM2_D3D_CopySurfaceRectLocked` | Evidence from decompile and caller context. |
| `0x82393250` | `FM2_D3D_TextureDesc_AllocStagingBuffer` | Evidence from decompile and caller context. |
| `0x8239d878` | `FM2_D3D_TextureDesc_SelectFormatHandler` | Evidence from decompile and caller context. |
| `0x821d3dd8` | `FM2_DeferredAudioManagerUpdate_Destroy` | Evidence from decompile and caller context. |
| `0x8224d6f8` | `FM2_LiveryMask_GrowPendingEntryBufferCore` | Evidence from decompile and caller context. |
| `0x82252d80` | `FM2_RaceGhost_ResolveAndAttachUpgradeNode` | Evidence from decompile and caller context. |
| `0x82253430` | `FM2_CarParts_RemoveMatchingUpgradeFromListCore` | Evidence from decompile and caller context. |
| `0x822199e0` | `FM2_ComObject_InitRefCountFromSourceCoreBody` | Evidence from decompile and caller context. |
| `0x8229b130` | `FM2_AudioSample_BuildOutputPairDescriptorFieldBodyCore` | Evidence from decompile and caller context. |
| `0x82335c40` | `FM2_Audio_VolumeListInsertNodeByPrefix` | Evidence from decompile and caller context. |
| `0x823a3fd0` | `FM2_Png_SetImageDimensions` | Evidence from decompile and caller context. |
| `0x823a6958` | `FM2_Png_EnsureRgbThenDecodeRowCore` | Evidence from decompile and caller context. |
| `0x823a6e58` | `FM2_Shader_ApplyConstantsBatchFlushBody` | Evidence from decompile and caller context. |
| `0x823b03a8` | `FM2_Shader_ApplyConstantsBatchValidateSlot` | Evidence from decompile and caller context. |
| `0x821e6888` | `FM2_CareerRace_UpdatePlaybackTimerFromEndRace` | Evidence from decompile and caller context. |
| `0x821f2428` | `FM2_RenderAdapter_ResetControllerSessionState` | Evidence from decompile and caller context. |
| `0x82219888` | `FM2_ComObject_InitRefCountBindingFields` | Evidence from decompile and caller context. |
| `0x82236dc0` | `FM2_XmlReader_GrowAttrTableRealloc` | Evidence from decompile and caller context. |
| `0x82237428` | `FM2_XmlReader_ShiftAttrTableEntries` | Evidence from decompile and caller context. |
| `0x82238e60` | `FM2_XmlReader_FillNewAttrTableSlot` | Evidence from decompile and caller context. |
| `0x821d42f0` | `FM2_SQLite_HashBucketGetHead` | Evidence from decompile and caller context. |