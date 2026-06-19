### Infrastructure pass 15 (33 functions)

Profile Lua stack markers, wstring, intrusive-list RB-tree, profile DB, career race.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8220c6f8` | `FM2_ProfileLua_UnwindBindingStackSlot` | Evidence from decompile and caller context. |
| `0x8240c618` | `FM2_Thread_YieldExecution` | Thin `NtYieldExecution` wrapper. |
| `0x8220c758` | `FM2_ProfileLua_PushStackMarkerLink` | Evidence from decompile and caller context. |
| `0x8220c7c0` | `FM2_ProfileLua_InitStackMarker` | Evidence from decompile and caller context. |
| `0x8220c890` | `FM2_ProfileLua_PushBindingKeyAndPop` | Evidence from decompile and caller context. |
| `0x8221bf40` | `FM2_WString_GrowHeapCapacity` | Evidence from decompile and caller context. |
| `0x8221cf10` | `FM2_IntrusiveList_EraseNodeRebalance122` | RB-tree erase for 122-byte intrusive-list nodes. |
| `0x8221d328` | `FM2_IntrusiveList_ClearSentinelLinks122` | Evidence from decompile and caller context. |
| `0x822246c0` | `FM2_Boot_VsprintfBuffer260` | Boot path `vsprintf_s` into 0x104-byte buffer. |
| `0x82225578` | `FM2_IntrusiveList_AllocSentinelNode24` | Evidence from decompile and caller context. |
| `0x82437508` | `FM2_BufferedStream_InitCore` | Evidence from decompile and caller context. |
| `0x824b7668` | `FM2_Lua_InvokeProtectedCall32` | Evidence from decompile and caller context. |
| `0x824f51d8` | `FM2_Profile_GetFieldAt40` | Evidence from decompile and caller context. |
| `0x82572948` | `FM2_AudioManager_GetAltSingleton24` | Evidence from decompile and caller context. |
| `0x82509400` | `FM2_Presentation_GetManagerSingleton8` | Evidence from decompile and caller context. |
| `0x822dc9a8` | `FM2_ConfigEntry_ReleaseRefOptionalFree` | Evidence from decompile and caller context. |
| `0x827e4ca0` | `FM2_AllocPoolAcquire24xCount` | Evidence from decompile and caller context. |
| `0x824d3580` | `FM2_ComObject_SetUtf8NameWide` | Evidence from decompile and caller context. |
| `0x824a51a0` | `FM2_ResourceLock_EnterCritSecOrResolve` | Evidence from decompile and caller context. |
| `0x824a4f68` | `FM2_D3D_WaitGpuFrameSlotWithTimeout` | Evidence from decompile and caller context. |
| `0x822272f0` | `FM2_DeferredCommand_DtorReleaseRef` | Evidence from decompile and caller context. |
| `0x82228478` | `FM2_DeferredCommand_CopyAssign` | Evidence from decompile and caller context. |
| `0x8222ed38` | `FM2_WString_EraseSubrangeInPlace` | Evidence from decompile and caller context. |
| `0x8223db98` | `FM2_SceneProp_GetWideCharAtIndex` | Evidence from decompile and caller context. |
| `0x8224c5d0` | `FM2_LiveryMask_CheckListLengthLimit` | Evidence from decompile and caller context. |
| `0x82251b10` | `FM2_ProfileDb_CompareStringRecordsLess` | Evidence from decompile and caller context. |
| `0x82251c48` | `FM2_ProfileDb_InitBindingContexts` | Evidence from decompile and caller context. |
| `0x822520f0` | `FM2_ProfileDb_ReleaseBindingContexts` | Evidence from decompile and caller context. |
| `0x82252170` | `FM2_ProfileDb_DtorReleaseAll` | Evidence from decompile and caller context. |
| `0x822529b0` | `FM2_ProfileDb_CopyAssignRecord` | Evidence from decompile and caller context. |
| `0x82255488` | `FM2_Profile_ClearOptionsChangedFlag` | Clears profile options-changed bit at +744. |
| `0x82256028` | `FM2_CareerRace_IsRaceModeType2` | Evidence from decompile and caller context. |
| `0x82256040` | `FM2_CareerRace_IsRaceModeType6` | Evidence from decompile and caller context. |