### Infrastructure pass 22 (33 functions)

Lua userdata closures/getters, car DB PI lookup, audio volume list, game type ctors.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x822a0710` | `FM2_Lua_AppForza_UnpauseMedia` | Evidence from decompile and caller context. |
| `0x822a0148` | `FM2_Lua_PushSaveVolumeWrapperClosure` | Evidence from decompile and caller context. |
| `0x822a01c0` | `FM2_Lua_PushNetworkXuidClosure` | Evidence from decompile and caller context. |
| `0x822a9db8` | `FM2_Lua_PushLiveryHandleClosure` | Evidence from decompile and caller context. |
| `0x822b4b18` | `FM2_Lua_PushCarControlsClosure` | Evidence from decompile and caller context. |
| `0x822aab40` | `FM2_Lua_AuctionHouse_PushHighBidderDisplay` | Evidence from decompile and caller context. |
| `0x822b5e88` | `FM2_Lua_GameLibrary_GetCompressorATM` | Evidence from decompile and caller context. |
| `0x822b6a60` | `FM2_Lua_GameLibrary_GetSuspensionDamage` | Evidence from decompile and caller context. |
| `0x822b70d8` | `FM2_Lua_GameLibrary_GetDistUnderTire` | Evidence from decompile and caller context. |
| `0x822b88f0` | `FM2_Lua_PopCarControlsUserdata` | Evidence from decompile and caller context. |
| `0x822bb2d0` | `FM2_Lua_PopNumUnitStringUserdata` | Evidence from decompile and caller context. |
| `0x822c8ae8` | `FM2_Lua_Garage_CalcPerformanceIndex` | Maps Lua ratio to car DB performance index table. |
| `0x822cef20` | `FM2_Lua_PopUICarListUserdata` | Evidence from decompile and caller context. |
| `0x822d50c0` | `FM2_Lua_Leaderboard_PushGotFirstPlaceLocal` | Evidence from decompile and caller context. |
| `0x8222e340` | `FM2_CarDb_GetGlobalSingleton3390` | Evidence from decompile and caller context. |
| `0x82599458` | `FM2_CarDb_LookupPerformanceIndexByRatio` | Piecewise-linear lookup in 10-segment PI curve. |
| `0x824ae760` | `FM2_ComObject_AllocRefCountBlock72` | Evidence from decompile and caller context. |
| `0x822dc120` | `FM2_Lua_LiveryColor_PushFinishValue` | Evidence from decompile and caller context. |
| `0x822da538` | `FM2_Lua_LiveryEditor_PushHasOppositeSide` | Evidence from decompile and caller context. |
| `0x82301c00` | `FM2_Lua_ForzaProfile_PushUsingWheel` | Evidence from decompile and caller context. |
| `0x82314ba0` | `FM2_Lua_SaveVolume_PushOperationResult` | Evidence from decompile and caller context. |
| `0x8231e988` | `FM2_Lua_Tuning_GetFrontAccelValue` | Evidence from decompile and caller context. |
| `0x823353b0` | `FM2_Audio_VolumeListIteratorHasNext` | Evidence from decompile and caller context. |
| `0x82335428` | `FM2_Audio_VolumeListGetFloatAt40` | Evidence from decompile and caller context. |
| `0x82339ea0` | `FM2_LiveryRenderManager_InitListHead` | Evidence from decompile and caller context. |
| `0x82334df0` | `FM2_RaceGhostPlaybackState_Init` | Evidence from decompile and caller context. |
| `0x8233ce70` | `FM2_GameType_Ctor` | Evidence from decompile and caller context. |
| `0x82340c10` | `FM2_MultiscreenClientComponent_Ctor` | Evidence from decompile and caller context. |
| `0x82331560` | `FM2_HashName_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x8230abc8` | `FM2_SavedReplay_Dtor` | Evidence from decompile and caller context. |
| `0x823472f0` | `FM2_LuaGarage_EnsureCarRecordField92` | Lazy-init car record field at +92 before notify copy. |
| `0x822ddd78` | `FM2_Lua_PopLiveryLayerUserdata` | Evidence from decompile and caller context. |
| `0x82301cc8` | `FM2_ProfileLua_UnwindBindingContext` | Evidence from decompile and caller context. |