### Infrastructure pass 19 (33 functions)

Profile RB-tree, race ghost/entry, livery mask, lobby sort modes, D3D wait, render adapter.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82253888` | `FM2_ProfileDb_RbTreeInsertNodeImpl` | Evidence from decompile and caller context. |
| `0x8228ef18` | `FM2_RaceGhost_MergeSortedKeyframeBuffer` | Merges sorted dword keyframe ranges in ghost playback buffer. |
| `0x824a4888` | `FM2_D3D_WaitGpuFrameSlotSpinLoop` | Spins/waits on GPU frame slot with timeout (D3D resource lock path). |
| `0x82540420` | `FM2_RenderAdapter_InitChildListFromRange` | Evidence from decompile and caller context. |
| `0x821d9aa8` | `FM2_LiveryMask_SetPendingRecordField12` | Evidence from decompile and caller context. |
| `0x821d9db0` | `FM2_RaceEntry_GetGhostTableEntryAtIndex` | Evidence from decompile and caller context. |
| `0x821da240` | `FM2_LiveryMask_SetPendingRecordField8` | Evidence from decompile and caller context. |
| `0x821e9ae0` | `FM2_ContentEntry_CopyCarSetupHead304` | Evidence from decompile and caller context. |
| `0x8220a9a8` | `FM2_SQLite_FormatQueryVprintfShort` | Evidence from decompile and caller context. |
| `0x8221bd30` | `FM2_RaceEntry_ShouldProcessGhostForClass` | Evidence from decompile and caller context. |
| `0x82224e18` | `FM2_RaceEntry_SpawnGhostSceneNodeAtSlot` | Evidence from decompile and caller context. |
| `0x82249f80` | `FM2_LiveryMask_GenerateUniqueColorKeyString` | Evidence from decompile and caller context. |
| `0x8224a290` | `FM2_LiveryMask_DtorReleaseColorKeyNode` | Evidence from decompile and caller context. |
| `0x8224a718` | `FM2_LiveryMask_BinarySearchRecordById` | Evidence from decompile and caller context. |
| `0x822531a8` | `FM2_Lua_LiveryEditor_ApplyColorFromRegistry` | Evidence from decompile and caller context. |
| `0x8225e3f8` | `FM2_LuaLobbySort_SortMode0` | Lobby sort dispatch for context mode 0 at profile +760. |
| `0x8225e610` | `FM2_LuaLobbySort_SortMode1` | Evidence from decompile and caller context. |
| `0x8225e828` | `FM2_LuaLobbySort_SortMode2` | Evidence from decompile and caller context. |
| `0x8225ea40` | `FM2_LuaLobbySort_SortMode3` | Evidence from decompile and caller context. |
| `0x8225ec58` | `FM2_LuaLobbySort_SortMode4` | Evidence from decompile and caller context. |
| `0x82266888` | `FM2_Vector48Record_ReleaseRefFields` | Evidence from decompile and caller context. |
| `0x822669a0` | `FM2_Vector48Record_MoveConstruct` | Evidence from decompile and caller context. |
| `0x82266d10` | `FM2_Vector48Iterator_ShiftRecordsBackward` | Evidence from decompile and caller context. |
| `0x82266dc0` | `FM2_Vector48Iterator_FillFromRecord` | Evidence from decompile and caller context. |
| `0x822695d0` | `FM2_RaceEntry_UpdateGhostVisibilityFlag` | Evidence from decompile and caller context. |
| `0x8226b390` | `FM2_RaceGhostWorldState_Ctor` | Evidence from decompile and caller context. |
| `0x8226b840` | `FM2_CarAudioStreamDefaults_Ctor` | Initializes car-audio stream defaults with wide `Default` name. |
| `0x82277c98` | `FM2_SceneCamera_CallVfunc12` | Evidence from decompile and caller context. |
| `0x8227b618` | `FM2_AudioSignalGate_Ctor_F0A4` | Evidence from decompile and caller context. |
| `0x8227b780` | `FM2_AudioSignalGate_Ctor_F0F0` | Evidence from decompile and caller context. |
| `0x825489c8` | `FM2_RenderAdapter_CopyChildListFromRange` | Evidence from decompile and caller context. |
| `0x8242a258` | `FM2_FileStream_Ctor36714` | Evidence from decompile and caller context. |
| `0x82460670` | `FM2_D3D_InitGpuWaitTimerState` | Evidence from decompile and caller context. |