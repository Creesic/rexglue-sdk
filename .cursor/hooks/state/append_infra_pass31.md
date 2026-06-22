### Infrastructure pass 31 (33 functions)

Race ghost sort, Lua userdata pop closures, replay pending strings, GPU kick/PIX, audio alloc.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8228e0e0` | `FM2_RaceGhost_SiftUpKeyframeHeap` | Evidence from decompile and caller context. |
| `0x8228e1a0` | `FM2_RaceGhost_IntroSortPartitionGap` | Evidence from decompile and caller context. |
| `0x822a17f0` | `FM2_Lua_PopSaveVolumeWrapperUserdata` | Evidence from decompile and caller context. |
| `0x822a1848` | `FM2_Lua_PopNetworkXuidUserdata` | Evidence from decompile and caller context. |
| `0x822a8fe8` | `FM2_Lua_AuctionHouse_GetCarSlotFromArgs` | Evidence from decompile and caller context. |
| `0x822aa0a8` | `FM2_Lua_PopLiveryHandleUserdata` | Evidence from decompile and caller context. |
| `0x822db290` | `FM2_Lua_LiveryEditor_LoadDecalsFromArg` | Evidence from decompile and caller context. |
| `0x822db340` | `FM2_Lua_LiveryEditor_SetDecalTabFromArg` | Evidence from decompile and caller context. |
| `0x822eb190` | `FM2_Lua_PlayerChoices_SetShiftingFromArg` | Evidence from decompile and caller context. |
| `0x822f9058` | `FM2_Lua_PhotoModeCamera_PushFocusToStack` | Evidence from decompile and caller context. |
| `0x822fb0a8` | `FM2_Lua_PhotoModeCamera_GetDepthAtScreenCoord` | Evidence from decompile and caller context. |
| `0x82301700` | `FM2_Lua_GetGarageUserdataOrRaise` | Evidence from decompile and caller context. |
| `0x82304e28` | `FM2_Lua_ForzaProfile_GetThrottleDeadzoneWheel` | Evidence from decompile and caller context. |
| `0x8230f0d8` | `FM2_ReplayPendingString_CopyConstruct` | Evidence from decompile and caller context. |
| `0x8230f200` | `FM2_ReplayPendingString_Dtor` | Evidence from decompile and caller context. |
| `0x8230f7c0` | `FM2_Replay_CreatePendingStringTask` | Evidence from decompile and caller context. |
| `0x8230f878` | `FM2_Lua_PopPendingStringUserdata` | Evidence from decompile and caller context. |
| `0x8230fec0` | `FM2_Lua_RewardReveal_GetCarLevelInfoFromArgs` | Evidence from decompile and caller context. |
| `0x82314fc0` | `FM2_Lua_PopSaveEnumeratorUserdata` | Evidence from decompile and caller context. |
| `0x8231edc8` | `FM2_Lua_TuningSetRearDecelFromArg` | Evidence from decompile and caller context. |
| `0x8231f658` | `FM2_Lua_PopLuaTuningClassUserdata` | Evidence from decompile and caller context. |
| `0x82329548` | `FM2_Lua_BuildCarSetupPointerUserdata` | Evidence from decompile and caller context. |
| `0x8232d758` | `FM2_Career_PendingBoolSslWrapper_Ctor` | Evidence from decompile and caller context. |
| `0x82341e60` | `FM2_RaceGhost_MergePlaybackKeyframeSlice` | Evidence from decompile and caller context. |
| `0x82363e58` | `FM2_AudioDevice_AllocPhysicalCategoryBuffer` | Evidence from decompile and caller context. |
| `0x82371ea8` | `FM2_D3D_WritePrimaryRingBufferWords` | Evidence from decompile and caller context. |
| `0x82372aa8` | `FM2_GpuKick_AppendSchedulerPm4Packets` | Appends PM4 scheduler packets (66940/1400 opcodes) to kick buffer. |
| `0x82372c00` | `FM2_D3D_WaitForGpuCommandCompletion` | Evidence from decompile and caller context. |
| `0x823744c8` | `FM2_GpuCommandBuffer_SetDisplayModeAndQuery` | Evidence from decompile and caller context. |
| `0x82378070` | `FM2_D3D_AllocGpuCaptureForPix` | Allocates D3D GPU capture object after PIXBeginCapture succeeds. |
| `0x8237b090` | `FM2_Render_ReleasePixCaptureSurfaces` | Evidence from decompile and caller context. |
| `0x8237b8c0` | `FM2_Render_AllocateEdramScratchSlice` | Evidence from decompile and caller context. |
| `0x8239f358` | `FM2_Png_ZeroPaletteBlock64` | Evidence from decompile and caller context. |