### Infrastructure pass 21 (33 functions)

Lobby sort splice/merge, profile DB RB-tree, D3D timer, audio render, profile tuning.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824e83e0` | `FM2_StdList_SpliceAndAdjustCount` | Circular-list splice with count adjustment (lobby sort). |
| `0x824e8990` | `FM2_StdList_SpliceFrontIfNonEmpty` | Evidence from decompile and caller context. |
| `0x8236be98` | `FM2_AudioMix_SetupRenderTargetTexturesThunk` | Evidence from decompile and caller context. |
| `0x82295ff0` | `FM2_Profile_DtorReleaseSubobjects` | Evidence from decompile and caller context. |
| `0x82460498` | `FM2_D3D_GetGpuWaitElapsedSeconds` | Evidence from decompile and caller context. |
| `0x82205828` | `FM2_Profile_DestroyStringListAt28` | Evidence from decompile and caller context. |
| `0x8221cd60` | `FM2_Profile_DestroyWStringListAt16` | Evidence from decompile and caller context. |
| `0x82251890` | `FM2_ProfileDb_RbTreeRotateLeft` | Evidence from decompile and caller context. |
| `0x82251aa8` | `FM2_ProfileDb_RbTreeRotateRight` | Evidence from decompile and caller context. |
| `0x82253130` | `FM2_ProfileDb_AllocRbTreeNode220` | Evidence from decompile and caller context. |
| `0x8225c278` | `FM2_LuaLobbySort_MergeSortPass0` | Stable merge sort pass for lobby list (mode 0). |
| `0x8225c420` | `FM2_LuaLobbySort_MergeSortPass1` | Evidence from decompile and caller context. |
| `0x8225c5c8` | `FM2_LuaLobbySort_MergeSortPass2` | Evidence from decompile and caller context. |
| `0x8225c770` | `FM2_LuaLobbySort_MergeSortPass3` | Evidence from decompile and caller context. |
| `0x8225c918` | `FM2_LuaLobbySort_MergeSortPass4` | Evidence from decompile and caller context. |
| `0x82464a28` | `FM2_StdList_CheckLengthAndAddCount` | Evidence from decompile and caller context. |
| `0x8242d0a0` | `FM2_Profile_CloseXamContentIfOpen` | Evidence from decompile and caller context. |
| `0x8222ee70` | `FM2_ProfileWStringNode_DtorOptionalFree` | Evidence from decompile and caller context. |
| `0x82251a30` | `FM2_AllocPoolAcquire224xCount` | Evidence from decompile and caller context. |
| `0x8227c900` | `FM2_AudioSignalGate_Ctor_F1CC` | Evidence from decompile and caller context. |
| `0x822905e0` | `FM2_SceneGraph_SetChildSlotVisibleByType` | Evidence from decompile and caller context. |
| `0x82296528` | `FM2_Profile_ResetStateAfterNotify` | Evidence from decompile and caller context. |
| `0x82297678` | `FM2_Profile_ApplyTuningRecordFromDb` | Evidence from decompile and caller context. |
| `0x82297bd8` | `FM2_Lua_PushDisplayStringClosure` | Evidence from decompile and caller context. |
| `0x8229a220` | `FM2_AudioSample_BuildIteratorPair` | Evidence from decompile and caller context. |
| `0x8229ca88` | `FM2_AudioRenderFrame_ProcessSampleBatch` | Evidence from decompile and caller context. |
| `0x8229ccf0` | `FM2_WaitText_Dtor` | Evidence from decompile and caller context. |
| `0x8229dd50` | `FM2_WaitAnimation_Ctor` | Evidence from decompile and caller context. |
| `0x8229f1b8` | `FM2_CompositeAdapterState_Dtor` | Evidence from decompile and caller context. |
| `0x824603d8` | `FM2_D3D_GetQueryPerformanceElapsedDiv` | Evidence from decompile and caller context. |
| `0x82460430` | `FM2_D3D_GetTickCountElapsedMs` | Evidence from decompile and caller context. |
| `0x82299e18` | `FM2_AudioSample_FindNextBufferNode` | Evidence from decompile and caller context. |
| `0x827fa088` | `FM2_DebugLog_NoOpStub` | Empty debug log stub called from audio render path. |