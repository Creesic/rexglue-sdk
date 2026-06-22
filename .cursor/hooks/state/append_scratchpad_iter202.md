## Iteration 202

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82632960 | sub_82632960 | FM2_Lua_ToluaRegisterCommonClassMetatable | 0.90 | Pushes registry table; assigns C closure or `tolua_commonclass` via `FM2_Lua_ToluaEnsureUboxWeakTable`; used by const-binding registration. |
| 0x826326F8 | sub_826326F8 | FM2_Lua_ToluaRegisterSuperclassFlags | 0.91 | Sets `tolua_super` entries to true; copies subclass method table via `FM2_Lua_TableTraverseNextEntry`; called from const/type metatable setup. |
| 0x82633048 | sub_82633048 | FM2_Lua_ToluaRegisterConstBindingWithCollector | 0.90 | Builds `"const "` name variants; registers common-class + super flags; optional `.collector` lightuserdata field; 14 Lua binding callers. |
| 0x82632848 | sub_82632848 | FM2_Lua_ToluaEnsureUboxWeakTable | 0.91 | Ensures `tolua_ubox` weak table (`__mode="v"`) in registry; string refs `tolua_ubox`, `__mode`, `v`. |
| 0x824F48D8 | sub_824F48D8 | FM2_RaceGhost_IsReplayPlaybackFrameValid | 0.89 | Compares replay byte `+25664` vs manager `+8336`; frame index in `[1,capacity]` or timed-event controller set. |
| 0x824F30A0 | sub_824F30A0 | FM2_RaceGhost_GetCurrentFrameIndexFromReplayBuffer | 0.92 | Returns `*(replayBuffer+25536)`; used by playback validator and Lua ghost queries. |
| 0x824F30D8 | sub_824F30D8 | FM2_RaceGhost_GetKeyframeCapacityFromReplayBuffer | 0.91 | Returns `*(int16*)(buf+25526) * *(u8*)(buf+25524)`; keyframe capacity product for range check. |
| 0x824B7CF0 | sub_824B7CF0 | FM2_Lua_TableTraverseNextEntry | 0.90 | Lua table `next` traversal; `FM2_Lua_GetStackSlotPointer` + hash walk; advances stack ±16; used in tolua super registration. |
| 0x827DF140 | sub_827DF140 | FM2_Render_GetOrBuildCachedMaterialWorldMatrix | 0.88 | Returns identity `qword_82A73200` if material flag unset; else recursive `GMatrix3D` multiply + `FM2_Render_DispatchMaterialPassByNegativeSlot(17)`. |
| 0x824897B0 | sub_824897B0 | FM2_AIDriver_TestPathSegmentBlockingClearance | 0.89 | `FM2_AIDriver_LerpPathSegmentHalf` pair; VMX normalize/dot; compares lateral delta vs thresholds at `a1+688/+692`; 11 AI callers. |
| 0x82633740 | sub_82633740 | FM2_Lua_CtorObjectElementWithLightUserdata | 0.90 | `FM2_Lua_GetLightUserdataAndRestoreStack`; `FM2_Lua_InitObjectElementFromBinding`; vtable `off_82113B5C`; negative binding path. |
| 0x82621EA0 | sub_82621EA0 | FM2_Lua_InitObjectElementFromBinding | 0.91 | Sets vtable `off_8210F5D0`; copies binding dword; `FM2_SceneGraph_GetCompareFieldAt36`; clears string at `+16`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x824F0160 | sub_824F0160 | 8B offset helper `a1+12` only. |
| 0x82785028 | sub_82785028 | 16B offset helper `a1+308` only. |
| 0x8261F3E8 | sub_8261F3E8 | Large Lua object-element factory; defer full binding lifecycle cluster. |
| 0x827DECF8 | sub_827DECF8 | Large SIMD matrix build; defer with material matrix cluster siblings. |
| 0x827D6910 | sub_827D6910 | Thin child-handle fetch `*(a1+4*(idx+4))`; defer render accessor cluster. |
| 0x824BE5A0 | sub_824BE5A0 | Lua table hash traversal core; defer with `sub_824BE490` cluster. |
| 0x821F1B70 | sub_821F1B70 | Pure VMX128 math kernel; no callees/strings for semantic name. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine; defer until sub-callees named. |
