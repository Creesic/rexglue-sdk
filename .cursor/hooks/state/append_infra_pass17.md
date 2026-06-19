### Infrastructure pass 17 (33 functions)

Stream/Lua/livery helpers, career assist getters, render pass setup, audio/SQLite.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8242bc48` | `FM2_BinaryStream_InitBaseVtable` | Evidence from decompile and caller context. |
| `0x8222e490` | `FM2_WString_IsPointerInsideBuffer` | Evidence from decompile and caller context. |
| `0x821e9bf0` | `FM2_ContentEntry_CopyHeadFields384` | Evidence from decompile and caller context. |
| `0x8226ab58` | `FM2_LapTrackerInit_CopyAssign` | Evidence from decompile and caller context. |
| `0x822196f8` | `FM2_SQLite_FormatQueryVprintf` | Varargs format into query buffer (lap-tracker/car-db SQL). |
| `0x8224bcb8` | `FM2_LiveryMask_FindPendingEntryInList` | Evidence from decompile and caller context. |
| `0x8224ccc0` | `FM2_LiveryMask_QueryMediaNameByCarId` | SQL `SELECT MediaName FROM Data_Car WHERE id = ...`. |
| `0x8224c748` | `FM2_Lua_LiveryEditor_BuildLayerMaterialList` | Evidence from decompile and caller context. |
| `0x8223fc00` | `FM2_Lua_LiveryEditor_IsSlotIndexValid` | Evidence from decompile and caller context. |
| `0x82251b88` | `FM2_ProfileDb_RbTreeLowerBound` | Evidence from decompile and caller context. |
| `0x82251fc0` | `FM2_LiveryMask_MergeProfileRecordFromNode` | Evidence from decompile and caller context. |
| `0x82254548` | `FM2_LiveryMask_FindOrInsertColorKey` | Evidence from decompile and caller context. |
| `0x8242bb88` | `FM2_CompressionStream_CtorFromSource` | Evidence from decompile and caller context. |
| `0x824bec00` | `FM2_Lua_ProtectedCallSetupFrame` | Evidence from decompile and caller context. |
| `0x824d3190` | `FM2_WideString_ReleaseHeapBuffer` | Evidence from decompile and caller context. |
| `0x822540f8` | `FM2_ProfileDb_RbTreeInsertOrFind` | Evidence from decompile and caller context. |
| `0x82254eb0` | `FM2_Lua_LiveryEditor_ApplyLayerFromArgs` | Evidence from decompile and caller context. |
| `0x82267428` | `FM2_Vector48Iterator_InsertRangeFromSource` | Evidence from decompile and caller context. |
| `0x8226b7e0` | `FM2_RaceGhost_GetWorldStateSingleton` | Evidence from decompile and caller context. |
| `0x8226b8c0` | `FM2_CarAudio_GetStreamBufferSingleton` | Evidence from decompile and caller context. |
| `0x8226d360` | `FM2_RenderAdapter_GetDeviceContextFromOffset` | Evidence from decompile and caller context. |
| `0x8226fd20` | `FM2_CareerRace_GetAssistSuggestLineEnabled` | Returns field +416 unless profile forces `ForceOffSuggLine`. |
| `0x8226fd78` | `FM2_CareerRace_GetAssistAbsEnabled` | Evidence from decompile and caller context. |
| `0x8226fdd0` | `FM2_CareerRace_GetAssistTcsEnabled` | Evidence from decompile and caller context. |
| `0x8226fe28` | `FM2_CareerRace_GetAssistStmEnabled` | Evidence from decompile and caller context. |
| `0x8226fe80` | `FM2_CareerRace_GetAssistManualTransEnabled` | Evidence from decompile and caller context. |
| `0x822708d8` | `FM2_CareerRace_GetUpgradeModifierOrStockTune` | Stock-tune override via `ForceStockUpgradesAndTuning` XML flag. |
| `0x82272010` | `FM2_GraphicsStreamList_CtorInit` | Evidence from decompile and caller context. |
| `0x822737a8` | `FM2_Render_SetFramePipelineGlobalPtr` | Evidence from decompile and caller context. |
| `0x82276570` | `FM2_Render_MatchShaderPassKeyword` | Evidence from decompile and caller context. |
| `0x822766c8` | `FM2_Render_WritePassConstantSlot` | Writes pass-constant float into PM4 bitfield slot. |
| `0x822768d0` | `FM2_AudioSignalGate_Ctor_E734` | Evidence from decompile and caller context. |
| `0x82279000` | `FM2_SQLite_VfsReadSchemaCallback` | Evidence from decompile and caller context. |