### Infrastructure pass 18 (33 functions)

Career XML/Lua, race ghost, livery mask, profile RB-tree, audio resource hooks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825c5d78` | `FM2_CareerRace_LookupXmlIntByRewardId` | Evidence from decompile and caller context. |
| `0x825c5958` | `FM2_CareerRace_QueryGameOptionValueInt` | SQL `SELECT Value FROM GameOptionValues WHERE Id=%u`. |
| `0x82255ff8` | `FM2_CareerRace_IsAssistOverrideRaceMode` | True when profile race mode at +80 is 1, 7, or 8. |
| `0x8240d918` | `FM2_Thread_NtClearEventOrFail` | Evidence from decompile and caller context. |
| `0x8245ce10` | `FM2_ComObject_InitBaseVtable423C0` | Evidence from decompile and caller context. |
| `0x824b9550` | `FM2_Lua_IncrementCallDepthOrOverflow` | Increments Lua call depth; throws at 200 frames. |
| `0x824bc470` | `FM2_Lua_TypeErrorCorruptValue` | Evidence from decompile and caller context. |
| `0x824bc6b8` | `FM2_Lua_ProtectedCallMarkYieldable` | Evidence from decompile and caller context. |
| `0x824bc718` | `FM2_Lua_ResolveUpvalueOrConstant` | Evidence from decompile and caller context. |
| `0x824beb38` | `FM2_Lua_FindTableSlotForValue` | Evidence from decompile and caller context. |
| `0x824beae0` | `FM2_Lua_HashLookupClosureSlot` | Evidence from decompile and caller context. |
| `0x82277cb0` | `FM2_SceneCamera_CallVfunc20` | Evidence from decompile and caller context. |
| `0x8222e350` | `FM2_RaceGhost_GetCareerCarPropertyTable` | Evidence from decompile and caller context. |
| `0x8222e838` | `FM2_RaceGhost_TableExistsQuery` | Evidence from decompile and caller context. |
| `0x8222f400` | `FM2_RaceGhost_GetOrBuildMainCareerNode` | Evidence from decompile and caller context. |
| `0x82253ef8` | `FM2_ProfileDb_RbTreeInsertNode` | Evidence from decompile and caller context. |
| `0x82251980` | `FM2_ProfileDb_RbTreeIncrementIterator` | Evidence from decompile and caller context. |
| `0x82249fe0` | `FM2_LiveryMask_ParseColorKeyString` | Evidence from decompile and caller context. |
| `0x8224a628` | `FM2_AllocPoolAcquire292xCount` | Evidence from decompile and caller context. |
| `0x8224c160` | `FM2_LiveryMask_CopyPendingUpdateNode` | Evidence from decompile and caller context. |
| `0x8224b6a8` | `FM2_LiveryMask_ReleasePendingUpdateRefs` | Evidence from decompile and caller context. |
| `0x82266908` | `FM2_Vector48Record_CopyAssign` | Evidence from decompile and caller context. |
| `0x8226a0f8` | `FM2_Crt_MemmoveDwordRange` | Evidence from decompile and caller context. |
| `0x8245b828` | `FM2_Crt_SnprintfBufferVa` | Evidence from decompile and caller context. |
| `0x8221a8b0` | `FM2_HashName_LookupAltModuleProperty` | Evidence from decompile and caller context. |
| `0x824a3398` | `FM2_ResourceLock_AppendWaiterEntry` | Evidence from decompile and caller context. |
| `0x82277b38` | `FM2_LiveryMask_GetFieldAt2156` | Evidence from decompile and caller context. |
| `0x82278e00` | `FM2_AudioSignalGate_Ctor_EC5C` | Evidence from decompile and caller context. |
| `0x82279e18` | `FM2_AudioSignalGate_Ctor_EEBC` | Evidence from decompile and caller context. |
| `0x8227d5b0` | `FM2_AudioResource_RegisterHook_EB94` | Static audio resource hook: alloc vtable + register with resource manager. |
| `0x8227d618` | `FM2_AudioResource_RegisterHook_EBB4` | Evidence from decompile and caller context. |
| `0x8227d680` | `FM2_AudioResource_RegisterHook_EBD4` | Evidence from decompile and caller context. |
| `0x8227d6e8` | `FM2_AudioResource_RegisterHook_EBF4` | Evidence from decompile and caller context. |