### Infrastructure pass 50 (33 functions)

Render adapter/D3D init, view traversal draw setup, AI race line, XML writer, Lua binding sort, image/PNG load.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824e9c78` | `FM2_RenderAdapter_InitSwitchModeSharedFields` | Evidence from decompile and caller context. |
| `0x825c5320` | `FM2_LiveProfile_ReadWriteBufferScoped` | Evidence from decompile and caller context. |
| `0x8222e4e0` | `FM2_XmlWriter_AppendVsnprintf` | Evidence from decompile and caller context. |
| `0x82758250` | `FM2_Crt_WcsncpyChecked` | Evidence from decompile and caller context. |
| `0x824eb090` | `FM2_Lua_BindingPairSiftDown` | Evidence from decompile and caller context. |
| `0x821fcb38` | `FM2_IntrusiveList_SpliceNodeRange` | Evidence from decompile and caller context. |
| `0x8223f108` | `FM2_CarParts_ApplyUpgradeSlotFromDescriptor` | Evidence from decompile and caller context. |
| `0x82480be0` | `FM2_AIDriver_UpdateRaceLineFromSector` | Evidence from decompile and caller context. |
| `0x824810a8` | `FM2_AIDriver_ResetRaceLineOnSectorChange` | Evidence from decompile and caller context. |
| `0x824a52c0` | `FM2_D3D_InitGlobalDeviceSingleton` | Evidence from decompile and caller context. |
| `0x824efcb8` | `FM2_ExceptionFilter_OnCppException` | Evidence from decompile and caller context. |
| `0x824f0fb0` | `FM2_D3D_LazyInitPresentChainNotify` | Evidence from decompile and caller context. |
| `0x824f69f8` | `FM2_D3D_Subscriber_InitVtables` | Evidence from decompile and caller context. |
| `0x824fa8b0` | `FM2_Memory_LookupFrameAllocNotifyState` | Evidence from decompile and caller context. |
| `0x82502aa8` | `FM2_D3D_Subscriber_EnableDeviceJournal` | Evidence from decompile and caller context. |
| `0x8250a178` | `FM2_Render_ViewTraversalUpdateNodes` | Evidence from decompile and caller context. |
| `0x825105e8` | `FM2_Render_CompileMissingPassBuffers` | Evidence from decompile and caller context. |
| `0x82514e58` | `FM2_Render_DecodeAndSubmitDrawKey` | Evidence from decompile and caller context. |
| `0x82516700` | `FM2_Render_ObjectPassDrawSetupCore` | Evidence from decompile and caller context. |
| `0x82517520` | `FM2_Render_UpdatePassVisibilityState` | Evidence from decompile and caller context. |
| `0x824df738` | `FM2_RenderAdapter_ClearPresentationBinding` | Evidence from decompile and caller context. |
| `0x825c5290` | `FM2_LiveProfile_ReadWriteBufferBody` | Evidence from decompile and caller context. |
| `0x82429e08` | `FM2_BinaryStream_InitReadScope` | Evidence from decompile and caller context. |
| `0x82429e60` | `FM2_BinaryStream_DtorReadScope` | Evidence from decompile and caller context. |
| `0x821fc718` | `FM2_Render_NotifyChainInsertSubscriber` | Evidence from decompile and caller context. |
| `0x8250ffd0` | `FM2_Render_InitPassCompileLock` | Evidence from decompile and caller context. |
| `0x8250fbd8` | `FM2_Render_DtorPassCompileLock` | Evidence from decompile and caller context. |
| `0x82493680` | `FM2_AIDriver_LookupTrackWidthSample` | Evidence from decompile and caller context. |
| `0x82333430` | `FM2_Stl_IntrosortMedianOfThreeFloats` | Evidence from decompile and caller context. |
| `0x82360e38` | `FM2_Input_InitAxisDefaultsFromTable` | Evidence from decompile and caller context. |
| `0x8236aa48` | `FM2_D3D_ComputeResourceBindingFlags` | Evidence from decompile and caller context. |
| `0x823780e0` | `FM2_GpuCommandBuffer_BeginPerfCaptureOrKick` | Evidence from decompile and caller context. |
| `0x823a4348` | `FM2_Image_LoadPngFromMemory` | Evidence from decompile and caller context. |