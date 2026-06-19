### Infrastructure pass 16 (33 functions)

IO streams, Lua protected call, livery mask, race ghost, career race, render adapter.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8242cc38` | `FM2_RedirectStream_CtorFromSource` | Evidence from decompile and caller context. |
| `0x824a4d20` | `FM2_ResourceLock_ResolveFrameAllocatorState` | Evidence from decompile and caller context. |
| `0x824bca78` | `FM2_Lua_ProtectedCallDispatchLoop` | Evidence from decompile and caller context. |
| `0x824d32d8` | `FM2_WideString_ReserveCapacityChars` | Evidence from decompile and caller context. |
| `0x82610330` | `FM2_IntrusiveList_IncrementIteratorPastErase122` | Evidence from decompile and caller context. |
| `0x8220c688` | `FM2_ProfileLua_InitBindingNilMarker` | Evidence from decompile and caller context. |
| `0x8221cc88` | `FM2_IntrusiveList_DestroySubtreeNodes122` | Evidence from decompile and caller context. |
| `0x82226958` | `FM2_RaceEntry_OnFinishedNotifyGhostReplay` | Evidence from decompile and caller context. |
| `0x82230368` | `FM2_CarDb_QueryBaseCostByCarId` | SQLite `SELECT BaseCost FROM Data_Car WHERE Id=%u`. |
| `0x82231740` | `FM2_WString_AssignFromWideCStrChecked` | Evidence from decompile and caller context. |
| `0x82238928` | `FM2_RaceGhost_SelectOrRebuildPlaybackNode` | Evidence from decompile and caller context. |
| `0x8223d910` | `FM2_Render_PackDrawMatrixVMX128` | Evidence from decompile and caller context. |
| `0x82241830` | `FM2_Lua_LiveryEditor_RevertLayerApplyUpgrade` | Evidence from decompile and caller context. |
| `0x82242458` | `FM2_CarParts_ApplyUpgradeSlotGroupImpl` | Evidence from decompile and caller context. |
| `0x82242b88` | `FM2_Lua_LiveryEditor_GetCurrentLayerRecord` | Evidence from decompile and caller context. |
| `0x8224c428` | `FM2_LiveryMask_UpdatePendingEntryAt16` | Evidence from decompile and caller context. |
| `0x8224c4d8` | `FM2_LiveryMask_UpdatePendingEntryAt28` | Evidence from decompile and caller context. |
| `0x8224cdf8` | `FM2_LiveryMask_SetActiveCarMediaPath` | Builds `GAME:\Media\cars\%s` path for active livery car. |
| `0x8224d168` | `FM2_LiveryMask_BuildPendingUpdateRecord96` | Evidence from decompile and caller context. |
| `0x8224d6a0` | `FM2_LiveryMask_AllocPendingUpdateNode` | Evidence from decompile and caller context. |
| `0x8224e878` | `FM2_LiveryMask_EraseListNodeAndIter` | Evidence from decompile and caller context. |
| `0x822521c0` | `FM2_ProfileDb_InitRbTreeIterator` | Evidence from decompile and caller context. |
| `0x82252718` | `FM2_LiveryMask_FindProfileRecordByKey` | Evidence from decompile and caller context. |
| `0x82252928` | `FM2_LiveryMask_InsertOrUpdateProfileRecord` | Evidence from decompile and caller context. |
| `0x82252bf8` | `FM2_CarParts_LookupUpgradePathByName` | Evidence from decompile and caller context. |
| `0x82254c80` | `FM2_Lua_LiveryEditor_SetColorKeyValue` | Evidence from decompile and caller context. |
| `0x8225ae70` | `FM2_RaceGhost_MergeSortedKeyframeRanges` | Evidence from decompile and caller context. |
| `0x8225e330` | `FM2_CareerRace_CopyRewardsBlockAt760` | Evidence from decompile and caller context. |
| `0x8225ee70` | `FM2_LuaLobbySort_RunSortByContextMode` | Evidence from decompile and caller context. |
| `0x822624f8` | `FM2_RenderAdapter_DestroyChildAndClearList` | Evidence from decompile and caller context. |
| `0x82264450` | `FM2_GraphicsStream_IsLinkedListEmpty` | Returns true when graphics-stream linked list at +12 is empty. |
| `0x8226a8a0` | `FM2_SceneGraph_CopyContentEntryDwordVector` | Evidence from decompile and caller context. |
| `0x8226b1e0` | `FM2_CareerRace_CopyGhostReplayRecord` | Deep copy of ghost replay record with VMX128 matrix blocks. |