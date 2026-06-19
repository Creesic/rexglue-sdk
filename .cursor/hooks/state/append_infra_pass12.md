### Infrastructure pass 12 (33 functions)

Small getters/setters, FileSys, FMOD/SQLite thunks, render TLS.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8220a5b0` | `FM2_SceneProp_GetManagerBindingOffset172` | Returns scene-prop manager binding at `this+172`. |
| `0x8221a8a8` | `FM2_LuaLapTracker_GetStateOffset4` | Small helper from decompile/caller context. |
| `0x82224578` | `FM2_RaceGhost_GetFieldAt592` | Read race-ghost dword at offset +592. |
| `0x8222e1a8` | `FM2_LuaLapTracker_GetFieldAt528` | Small helper from decompile/caller context. |
| `0x82255480` | `FM2_Profile_GetSettingsBlockOffset124` | Small helper from decompile/caller context. |
| `0x8225ef78` | `FM2_LuaLobbySort_SetContextAndRun` | Store lobby-sort context at +96; run sort helper. |
| `0x82264380` | `FM2_GraphicsStream_GetLinkedFlagAt17` | Small helper from decompile/caller context. |
| `0x82264398` | `FM2_GraphicsStream_SetNotifyFlagAt4` | Small helper from decompile/caller context. |
| `0x82360298` | `FM2_GraphicsAdapter_GetNotifyFlagAt4` | Small helper from decompile/caller context. |
| `0x82466ab0` | `FM2_CareerRace_GetFieldAt64` | Small helper from decompile/caller context. |
| `0x82474868` | `FM2_AIDriver_ForwardToAssistCompute` | Forward AI-driver assist compute using field at +8. |
| `0x824c3a48` | `FM2_LuaSyntax_ExpectedTokenAtOffset16` | Small helper from decompile/caller context. |
| `0x824f30d0` | `FM2_LuaTournament_GetQualifyingEntryCount` | Tournament qualifying entry count at +25560. |
| `0x82506658` | `FM2_RenderFramePipeline_SetField3348` | Small helper from decompile/caller context. |
| `0x8257cf60` | `FM2_CarDynamics_SetBasePointer` | Small helper from decompile/caller context. |
| `0x82581910` | `FM2_RewardsQuery_GetRecordOffset24` | Small helper from decompile/caller context. |
| `0x82581e78` | `FM2_SQLiteToken_GetFlagAt26` | Small helper from decompile/caller context. |
| `0x82603b30` | `FM2_SceneGraph_GetCompareFieldAt36` | Small helper from decompile/caller context. |
| `0x82728078` | `FM2_RenderTls_GetWorkerSlotMask16` | Small helper from decompile/caller context. |
| `0x8243c030` | `FM2_SceneProp_GetFieldAt48` | Small helper from decompile/caller context. |
| `0x8245cd58` | `FM2_AudioManager_SetFieldAt132` | Small helper from decompile/caller context. |
| `0x8240c600` | `FM2_CarAudio_AllocStreamBufferZeroed` | Small helper from decompile/caller context. |
| `0x825fa868` | `FM2_ComPtr_AssignRefAtOffset216` | Small helper from decompile/caller context. |
| `0x82685838` | `FM2_FMOD_Event_SetParameter2DFlag1` | Small helper from decompile/caller context. |
| `0x826ec460` | `FM2_SQLite_AppendLowercaseIdentifierMode1` | Small helper from decompile/caller context. |
| `0x826ec4c0` | `FM2_SQLite_AppendLowercaseIdentifierAlt` | Small helper from decompile/caller context. |
| `0x82758ec8` | `FM2_Crt_Fopen` | Thin CRT `fopen` wrapper. |
| `0x82761da8` | `FM2_RenderSortable_SetSortKeyFloat24` | Small helper from decompile/caller context. |
| `0x82206c48` | `FM2_FileSys_Ctor` | Construct `CFileSys` object; zero child fields. |
| `0x82206cb8` | `FM2_FileSys_Dtor` | Destroy `CFileSys` and child allocators. |
| `0x82208d68` | `FM2_FileSysEntry_Dtor` | Small helper from decompile/caller context. |
| `0x8220aa70` | `FM2_AllocPoolAcquire2320xCount` | Pool alloc `2320 * count` bytes with overflow guard. |
| `0x8220b260` | `FM2_ComPtr_MakeFromPoolAlloc16` | Small helper from decompile/caller context. |