### Infrastructure pass 33 (33 functions)

Tick count import, Lua/replay/reward userdata, livery worker, race ghost interp, AI/career helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8240bde0` | `FM2_D3D_ReadKernelTickCountImport` | Returns kernel tick count via import slot MEMORY[0xBD]. |
| `0x82414b60` | `FM2_Crt_CopyConstructRefCountedCStr` | Evidence from decompile and caller context. |
| `0x824f16e8` | `FM2_AuctionHouse_LazyInitStaticData` | Evidence from decompile and caller context. |
| `0x82277d60` | `FM2_Lua_PhotoModeCamera_ReadDepthBufferSample` | Evidence from decompile and caller context. |
| `0x822a1690` | `FM2_Lua_GetNetworkXuidUserdataOrRaise` | Evidence from decompile and caller context. |
| `0x8230efe0` | `FM2_Replay_CreateGaragePendingStringTask` | Evidence from decompile and caller context. |
| `0x8230f140` | `FM2_RewardReveal_PendingUpgradeCompat_Ctor` | Evidence from decompile and caller context. |
| `0x8230f470` | `FM2_Lua_BuildPendingUpgradeCompatUserdata` | Evidence from decompile and caller context. |
| `0x8230f9a0` | `FM2_RewardReveal_CreatePendingCompatTask` | Evidence from decompile and caller context. |
| `0x8230fa90` | `FM2_Lua_PopPendingUpgradeCompatUserdata` | Evidence from decompile and caller context. |
| `0x823418e8` | `FM2_RaceGhost_ComputePlaybackInterpolationWeight` | Evidence from decompile and caller context. |
| `0x82363580` | `FM2_ContentBuffer_AllocWithTrackingNotify` | Evidence from decompile and caller context. |
| `0x823b0138` | `FM2_Png_ResetDecompressStateAndSyncChild` | Evidence from decompile and caller context. |
| `0x823c1658` | `FM2_Png_AllocInflateStateBuffers` | Evidence from decompile and caller context. |
| `0x8242a630` | `FM2_CompressionStream_FreeChildListRecursive` | Evidence from decompile and caller context. |
| `0x8242fe88` | `FM2_ContentList_HeapSiftUpByCompare` | Evidence from decompile and caller context. |
| `0x8242db78` | `FM2_Lua_GetAppForzaStateOffset8` | Evidence from decompile and caller context. |
| `0x82461500` | `FM2_LiveryMask_CloseWorkerThreadHandle` | Evidence from decompile and caller context. |
| `0x82461508` | `FM2_LiveryMask_IsWorkerThreadRunning` | GetExitCodeThread check for STILL_ACTIVE (259). |
| `0x824615c8` | `FM2_LiveryMask_CopyPendingRecordName128` | Evidence from decompile and caller context. |
| `0x824603c8` | `FM2_AudioFrameService_InvokeVtableUpdate` | Evidence from decompile and caller context. |
| `0x82460580` | `FM2_AudioFrameService_InitTimingBaseline` | Evidence from decompile and caller context. |
| `0x82463698` | `FM2_Math_PositiveModulo` | Evidence from decompile and caller context. |
| `0x82464088` | `FM2_CmdLine_InitParamsVtable` | Evidence from decompile and caller context. |
| `0x824662b8` | `FM2_AIOvertake_GetAssistValueAtIndex172` | Evidence from decompile and caller context. |
| `0x824662d0` | `FM2_AIOvertake_GetAssistValueAtIndex192` | Evidence from decompile and caller context. |
| `0x82466a78` | `FM2_CareerRace_GetElapsedRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x82466a88` | `FM2_CareerRace_GetTotalRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x82466b80` | `FM2_CareerRace_SubtractPhotoModeDeltaTime` | Evidence from decompile and caller context. |
| `0x8246c4d0` | `FM2_AIOvertake_IsHornThresholdExceeded` | Evidence from decompile and caller context. |
| `0x8246d020` | `FM2_AIOvertake_GetGlobalRaceTimeFloat` | Evidence from decompile and caller context. |
| `0x824804d8` | `FM2_AIDriver_IsAssistModeActive` | Evidence from decompile and caller context. |
| `0x824a1688` | `FM2_Presentation_GetCarResourceLoadCountA` | Evidence from decompile and caller context. |