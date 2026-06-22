### Infrastructure pass 77 (33 functions)

AI race line, car audio/livery, Lua SSL/input, profile, exception filter, D3D present chain.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82494668` | `FM2_AIDriver_ResetRaceLineOnSectorChangeInterpA` | Evidence from decompile and caller context. |
| `0x824946c8` | `FM2_AIDriver_ResetRaceLineOnSectorChangeInterpB` | Evidence from decompile and caller context. |
| `0x824947f0` | `FM2_AIDriver_ResetRaceLineOnSectorChangeBlend` | Evidence from decompile and caller context. |
| `0x824948d0` | `FM2_AIDriver_ResetRaceLineOnSectorChangeFinalize` | Evidence from decompile and caller context. |
| `0x8249a920` | `FM2_CarAudio_DtorBody` | Evidence from decompile and caller context. |
| `0x824a6758` | `FM2_LiveryMask_ParseAndLoadEntryBody` | Evidence from decompile and caller context. |
| `0x824aacb8` | `FM2_CompressionStream_EraseRbTreeNodeBody` | Evidence from decompile and caller context. |
| `0x824bd360` | `FM2_Lua_IncrementCallDepthOrOverflowBody` | Evidence from decompile and caller context. |
| `0x824cdca0` | `FM2_LiveryMask_ProcessPendingLayerEntryInitA` | Evidence from decompile and caller context. |
| `0x824cdde0` | `FM2_LiveryMask_ProcessPendingLayerEntryInitB` | Evidence from decompile and caller context. |
| `0x824cde98` | `FM2_LiveryMask_ProcessPendingLayerEntryInitC` | Evidence from decompile and caller context. |
| `0x824d0560` | `FM2_Render_ResetPassLightingSlotStateCopy` | Evidence from decompile and caller context. |
| `0x824d07b8` | `FM2_Input_InitControllerDevicesParseControllerName` | Evidence from decompile and caller context. |
| `0x824d2df8` | `FM2_Input_InitControllerDevicesParseBindingA` | Evidence from decompile and caller context. |
| `0x824d2f50` | `FM2_Input_InitControllerDevicesParseBindingB` | Evidence from decompile and caller context. |
| `0x824d3670` | `FM2_Scene_GetNotifyStateFromParamNormalizeUtf16` | Evidence from decompile and caller context. |
| `0x824d56c8` | `FM2_Lua_PushSslUnitStringsTableBodyA` | Evidence from decompile and caller context. |
| `0x824d59a0` | `FM2_Lua_PushSslUnitStringsTableBodyB` | Evidence from decompile and caller context. |
| `0x824d5f28` | `FM2_Lua_PushDampingFromKeyframeDoubleBody` | Evidence from decompile and caller context. |
| `0x824d8030` | `FM2_Math_AllocForceVectorComPtrBody` | Evidence from decompile and caller context. |
| `0x824daca8` | `FM2_Audio_VolumeListFindOrInsertByPrefixWalk` | Evidence from decompile and caller context. |
| `0x824dd2d8` | `FM2_Profile_ParseUnsignedFromSubStringValidateBody` | Evidence from decompile and caller context. |
| `0x824e30c0` | `FM2_ComObject_AllocSharedStateBufferInit` | Evidence from decompile and caller context. |
| `0x824e9e98` | `FM2_RenderAdapter_InitPresentationVtablesClearStateBody` | Evidence from decompile and caller context. |
| `0x824ef8a8` | `FM2_ExceptionFilter_OnCppExceptionLogBodyA` | Evidence from decompile and caller context. |
| `0x824efc50` | `FM2_ExceptionFilter_OnCppExceptionLogBodyB` | Evidence from decompile and caller context. |
| `0x824f0758` | `FM2_Render_NotifyChainInsertSubscriberSortedInit` | Evidence from decompile and caller context. |
| `0x824f2630` | `FM2_Memory_LookupFrameAllocNotifyStateBody` | Evidence from decompile and caller context. |
| `0x823d1d10` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDesc` | Evidence from decompile and caller context. |
| `0x824b1168` | `FM2_ComObject_GetAggregateFieldAt60Thunk` | Evidence from decompile and caller context. |
| `0x824fe578` | `FM2_D3D_LazyInitPresentChainBody` | Evidence from decompile and caller context. |
| `0x825025a0` | `FM2_D3D_Subscriber_EnableDeviceJournalBody` | Evidence from decompile and caller context. |
| `0x82503388` | `FM2_Memory_LookupFrameAllocNotifyStateInit` | Evidence from decompile and caller context. |