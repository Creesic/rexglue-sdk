### Infrastructure pass 23 (33 functions)

Lobby sort comparators, audio volume list, boot/memory helpers, render scoped batch.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8228bd18` | `FM2_Audio_VolumeListFindByPrefixOrIterator` | Evidence from decompile and caller context. |
| `0x82411900` | `FM2_Timer_ReadTimeBase64` | Reads PPC time-base register into 64-bit out-param. |
| `0x8242a2b8` | `FM2_ProfileState_GetControllerIdAt12` | Evidence from decompile and caller context. |
| `0x824db4a0` | `FM2_Audio_VolumeListInitIteratorPair` | Evidence from decompile and caller context. |
| `0x82256398` | `FM2_LuaLobbySort_CompareUpgradeModifierGt` | Evidence from decompile and caller context. |
| `0x82256410` | `FM2_LuaLobbySort_CompareStreamBytesReadLt` | Evidence from decompile and caller context. |
| `0x82256858` | `FM2_LuaLobbySort_CompareDisplayNameLt` | Evidence from decompile and caller context. |
| `0x82256918` | `FM2_LuaLobbySort_CompareGamertagThenName` | Lobby sort mode 0: gamertag byte tie-break then display name. |
| `0x82256a38` | `FM2_LuaLobbySort_CompareRewardTierThenClass` | Lobby sort mode 4: reward tier then car class at +448. |
| `0x82256390` | `FM2_NetworkLobby_GetSeriesPlayerCount` | Evidence from decompile and caller context. |
| `0x822643a0` | `FM2_LobbyEntry_GetGamertagPrefixByte` | Evidence from decompile and caller context. |
| `0x822643f0` | `FM2_LobbyEntry_ClearSeriesPointsField28` | Evidence from decompile and caller context. |
| `0x822575e8` | `FM2_NetworkLobby_GetSeriesEntryAtIndex` | Evidence from decompile and caller context. |
| `0x8221b5d0` | `FM2_WString_CompareICaseFromComObjects` | Evidence from decompile and caller context. |
| `0x82293360` | `FM2_TuningRecord_GetFrontAccelPct100` | Evidence from decompile and caller context. |
| `0x82277f08` | `FM2_SceneCamera_GetStereoscopicModeVtable112` | Evidence from decompile and caller context. |
| `0x82297358` | `FM2_Profile_ApplyPendingTuningFromHeap` | Evidence from decompile and caller context. |
| `0x822ba480` | `FM2_Lua_PushNumUnitStringClosure` | Evidence from decompile and caller context. |
| `0x822cb590` | `FM2_Lua_PushUICarListClosure` | Evidence from decompile and caller context. |
| `0x822dd5f0` | `FM2_Lua_PushLiveryLayerClosure` | Evidence from decompile and caller context. |
| `0x82295fb0` | `FM2_TuningRecord_SetDecelIncrementScaled` | Evidence from decompile and caller context. |
| `0x82356858` | `FM2_Render_RbTreeIteratorDecrement` | Evidence from decompile and caller context. |
| `0x823568e0` | `FM2_Render_RbTreeLowerBoundBySortKey` | Evidence from decompile and caller context. |
| `0x823569e8` | `FM2_Render_InitDrawListRangeIterator` | Evidence from decompile and caller context. |
| `0x8234b090` | `FM2_Replay_GetDefaultWatchStreamPath` | Evidence from decompile and caller context. |
| `0x82363628` | `FM2_Memory_XPhysicalAllocTracked` | Evidence from decompile and caller context. |
| `0x823637c8` | `FM2_Memory_AllocViaPoolOrSmallBlock` | Evidence from decompile and caller context. |
| `0x82363d18` | `FM2_Boot_GetSubsystemTableEntry` | Evidence from decompile and caller context. |
| `0x82363fa8` | `FM2_Boot_ShutdownSubsystemByIndex` | Evidence from decompile and caller context. |
| `0x823643f8` | `FM2_AudioDevice_NotifyCategoryIfEnabled` | Evidence from decompile and caller context. |
| `0x82369280` | `FM2_Render_ScopedBatch_IncGpuKickDepth` | Evidence from decompile and caller context. |
| `0x82369418` | `FM2_Render_ScopedBatch_DecGpuKickDepthOrFree` | Evidence from decompile and caller context. |
| `0x8236a2a0` | `FM2_D3D_UnlockResourceFenceRegions` | Evidence from decompile and caller context. |