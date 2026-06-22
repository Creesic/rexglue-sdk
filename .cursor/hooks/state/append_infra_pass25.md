### Infrastructure pass 25 (33 functions)

BufFile/XML reader cluster, profile/tuning merge, race ghost playback, input rumble parse.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824cc940` | `FM2_BufFile_InitRefCountedStringPath` | Evidence from decompile and caller context. |
| `0x824d0288` | `FM2_BufFile_OpenAndReadArchiveEntry` | Shared buf-file open/read path for camera scripts and rumble XML. |
| `0x824cc020` | `FM2_BufFile_GetGlobalModuleSingleton` | Evidence from decompile and caller context. |
| `0x824cc640` | `FM2_BufFile_AssignPathStringRefCounted` | Evidence from decompile and caller context. |
| `0x824cc128` | `FM2_BufFile_SwapModuleRefAndReleaseOld` | Evidence from decompile and caller context. |
| `0x824ce8e0` | `FM2_XmlReader_CtorWithBufferSizes` | Evidence from decompile and caller context. |
| `0x824cf538` | `FM2_XmlReader_LoadFromBufFileStream` | Evidence from decompile and caller context. |
| `0x824ce138` | `FM2_XmlReader_FindChildElementByPath` | Evidence from decompile and caller context. |
| `0x8220e880` | `FM2_RefCount_DecrementAndFreePoolBlock` | Evidence from decompile and caller context. |
| `0x82221770` | `FM2_Profile_MergeStringListFromOther` | Evidence from decompile and caller context. |
| `0x82220098` | `FM2_Profile_SpliceStringListRange` | Evidence from decompile and caller context. |
| `0x8223d7e0` | `FM2_CareerRaceCoordinator_ClearField33Thunk` | Evidence from decompile and caller context. |
| `0x8223d750` | `FM2_CareerRaceCoordinator_FreeOptionalBlockAt1` | Evidence from decompile and caller context. |
| `0x822941e8` | `FM2_Profile_MergeTuningRecordsFromComObject` | Evidence from decompile and caller context. |
| `0x82294b50` | `FM2_TuningDb_AllocLinkedListNode` | Evidence from decompile and caller context. |
| `0x82294c20` | `FM2_TuningRecord_AdjustScrollSliderUp` | Evidence from decompile and caller context. |
| `0x82294cf0` | `FM2_TuningRecord_AdjustScrollSliderDown` | Evidence from decompile and caller context. |
| `0x8220a5a0` | `FM2_TuningUi_GetScrollSliderObjectAt56` | Evidence from decompile and caller context. |
| `0x82277b78` | `FM2_SceneNodeManager_GetStateVtable100` | Evidence from decompile and caller context. |
| `0x82277cc8` | `FM2_SceneCamera_ApplyPhotoModeEffectParams` | Evidence from decompile and caller context. |
| `0x822a7c00` | `FM2_BootConfigEntry_DtorAtexit` | Evidence from decompile and caller context. |
| `0x822a7ba8` | `FM2_BootConfigEntry_DestroyLuaBindingArray` | Evidence from decompile and caller context. |
| `0x824db1b8` | `FM2_Audio_VolumeListLowerBoundByPrefix` | Evidence from decompile and caller context. |
| `0x82249778` | `FM2_LiveryEditor_LoadDecalsForTabIndex` | Evidence from decompile and caller context. |
| `0x8223ded8` | `FM2_LiveryEditor_SetCurrentDecalTabId` | Evidence from decompile and caller context. |
| `0x8226fd08` | `FM2_PlayerChoices_SetAssistShiftingValue` | Evidence from decompile and caller context. |
| `0x823611f8` | `FM2_Input_ParseControllerRumbleXmlSection` | Parses ControllerRumble.xml motor sections into float rumble table. |
| `0x82330eb0` | `FM2_RaceGhost_AccumulateRotationalKeyframeDeltas` | Evidence from decompile and caller context. |
| `0x82330f38` | `FM2_RaceGhost_CopyPlaybackTransformBlock` | Evidence from decompile and caller context. |
| `0x82330ff0` | `FM2_RaceGhost_InterpolateExtendedPlaybackState` | Evidence from decompile and caller context. |
| `0x823325a0` | `FM2_RaceGhost_LookupAiPlayerFeeFromSql` | SQL lookup of AI player fee for ghost playback timing. |
| `0x82334d48` | `FM2_CareerCircuitRaceCoordinator_DestroyField2` | Evidence from decompile and caller context. |
| `0x82334e48` | `FM2_CareerCircuitRaceCoordinator_ResetBaseVtable` | Evidence from decompile and caller context. |