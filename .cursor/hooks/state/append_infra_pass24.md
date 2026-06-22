### Infrastructure pass 24 (33 functions)

GPU PM4 kick paths, input rumble XML, race ghost replay, scene node copy, Lua closures.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8236ae10` | `FM2_D3D_ComputeResourceBindingFlags` | Evidence from decompile and caller context. |
| `0x8236ef20` | `FM2_Render_SetClearColorByteAndDirtyFlag` | Evidence from decompile and caller context. |
| `0x823700b0` | `FM2_Render_SetObjectPassLayerShift24` | Evidence from decompile and caller context. |
| `0x82372c20` | `FM2_D3D_WaitRingBufferCompletion5` | Evidence from decompile and caller context. |
| `0x823732e8` | `FM2_D3D_SyncRingBufferAfterGpuError` | Evidence from decompile and caller context. |
| `0x82376828` | `FM2_D3D_CreateShaderConstantFFixupWr` | Evidence from decompile and caller context. |
| `0x823764b0` | `FM2_Render_EmitDrawRangeCountPm4` | Evidence from decompile and caller context. |
| `0x82365b68` | `FM2_Render_EmitDrawRangeFromVector` | Evidence from decompile and caller context. |
| `0x8236c8c8` | `FM2_GpuKick_SubmitFenceOrInlinePm4` | Evidence from decompile and caller context. |
| `0x8235fb58` | `FM2_Input_DetectWheelSubtypeFromCapabilities` | Evidence from decompile and caller context. |
| `0x82362418` | `FM2_Input_InitAxisDefaultsTriple` | Evidence from decompile and caller context. |
| `0x82362570` | `FM2_Input_LoadControllerRumbleXmlThunk` | Evidence from decompile and caller context. |
| `0x82362480` | `FM2_Input_LoadControllerRumbleXml` | Loads ControllerRumble.xml wheel/controller defs into input state. |
| `0x82332c30` | `FM2_RaceGhost_ComputePlaybackWindow` | Accumulates ghost playback tick window from keyframe helpers. |
| `0x82332ec8` | `FM2_CareerCircuitRaceCoordinator_DtorPartial` | Evidence from decompile and caller context. |
| `0x8233b8c8` | `FM2_CareerRace_SlideGhostReplayRecordsLeft` | Evidence from decompile and caller context. |
| `0x8233bad0` | `FM2_CareerRace_FillGhostReplayRecords` | Evidence from decompile and caller context. |
| `0x823654a8` | `FM2_Memory_DeferredFreePopListHead` | Evidence from decompile and caller context. |
| `0x82364078` | `FM2_Memory_TryFreeViaPoolHandler` | Evidence from decompile and caller context. |
| `0x82363c78` | `FM2_Memory_XPhysicalAllocUnderCriticalSection` | Evidence from decompile and caller context. |
| `0x8230f3e8` | `FM2_Lua_PushPendingStringClosure` | Evidence from decompile and caller context. |
| `0x82314f40` | `FM2_Lua_PushSaveEnumeratorClosure` | Evidence from decompile and caller context. |
| `0x8231c168` | `FM2_Lua_PushLuaTuningClassClosure` | Evidence from decompile and caller context. |
| `0x822cb240` | `FM2_LuaGarage_CopyUICarListUserdata` | Evidence from decompile and caller context. |
| `0x822ec0c0` | `FM2_Lua_NetworkLobby_ClearSeriesPoints` | Evidence from decompile and caller context. |
| `0x822d5f28` | `FM2_Lua_Leaderboard_EnumerateByGamertagIndex` | Evidence from decompile and caller context. |
| `0x822dea50` | `FM2_Lua_Livery_CreateNewLayerAtArgs` | Evidence from decompile and caller context. |
| `0x822fa5b0` | `FM2_Lua_PhotoMode_ApplyCameraEffectParams` | Evidence from decompile and caller context. |
| `0x8235a778` | `FM2_Input_ControllerDevice_InitFromTemplate` | Evidence from decompile and caller context. |
| `0x8234cff0` | `FM2_SceneNodeTree_DetachAndReleaseRefs` | Evidence from decompile and caller context. |
| `0x8234d988` | `FM2_SceneNode_CopyAssignWithResourceLock` | Evidence from decompile and caller context. |
| `0x82370318` | `FM2_Render_SubmitObjectDrawConstantsSlot` | Evidence from decompile and caller context. |
| `0x823704a0` | `FM2_Render_SubmitObjectDrawConstantsSlotAlt` | Evidence from decompile and caller context. |