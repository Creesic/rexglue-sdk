## Iteration 259

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82631040 | sub_82631040 | FM2_SceneNode_GetOrCachePositionVector | 0.90 | Caches `CVector3` at node `+56`; binding slot 19; callers include `FM2_Lua_CLuaObjectNodeGetPosition`. |
| 0x826310E8 | sub_826310E8 | FM2_SceneNode_GetOrCacheRotation | 0.90 | Caches `CRotation` at `+60`; slot 14; caller `FM2_Lua_CLuaObjectNodeGetRotation`. |
| 0x82631190 | sub_82631190 | FM2_SceneNode_GetOrCacheScaleVector | 0.90 | Caches `CVector3` at `+64`; slot 16; caller `FM2_Lua_CLuaObjectNodeGetScale`. |
| 0x82631238 | sub_82631238 | FM2_SceneNode_GetOrCachePivotVector | 0.90 | Caches `CVector3` at `+68`; slot 12; caller `FM2_Lua_CLuaObjectNodeGetPivot`. |
| 0x826312E0 | sub_826312E0 | FM2_SceneNode_GetOrCachePositionVelocityVector | 0.90 | Caches `CVector3` at `+72`; slot 15; caller `FM2_Lua_CLuaObjectNodeGetPositionVelocity`. |
| 0x82631388 | sub_82631388 | FM2_SceneNode_GetOrCacheRotationVelocity | 0.90 | Caches `CRotation` at `+76`; slot 18; used by rotationVelocity setter via `sub_826315E8`. |
| 0x82631488 | sub_82631488 | FM2_SceneNode_AssignRotationFromUserdata | 0.91 | Sole callee from `FM2_Lua_CLuaObjectNodeSetRotation`; `InvokeElementCallbackWithVec128` on cached rotation. |
| 0x826314E0 | sub_826314E0 | FM2_SceneNode_AssignScaleFromUserdata | 0.91 | Sole callee from `FM2_Lua_CLuaObjectNodeSetScale`; assigns cached scale vector. |
| 0x82631538 | sub_82631538 | FM2_SceneNode_AssignPivotFromUserdata | 0.91 | Sole callee from `FM2_Lua_CLuaObjectNodeSetPivot`; assigns cached pivot vector. |
| 0x82631590 | sub_82631590 | FM2_SceneNode_AssignPositionVelocityFromUserdata | 0.91 | Sole callee from `FM2_Lua_CLuaObjectNodeSetPositionVelocity`. |
| 0x82630ED8 | sub_82630ED8 | FM2_SceneNode_SyncCachedVectorFromBindingProperty | 0.89 | Shared by all vector caches; `FindBindingByType` then writes xyz into cached userdata. |
| 0x82634C40 | sub_82634C40 | FM2_Lua_DispatchAkOutputProfileFields | 0.90 | Sole callee from `FM2_Lua_AkOutput`; iterates stack slots calling `ForEachProfileIntVectorInvokeVtableMethod4`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826315E8 | sub_826315E8 | Rotation-velocity setter; defer with remaining setter thunk cluster. |
| 0x82631640 | sub_82631640 | Thin thunk to position cache; insufficient standalone evidence. |
