## Iteration 257

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262CC70 | sub_8262CC70 | FM2_Lua_CLuaObjectNodeGetPosition | 0.92 | Property getter `position`; returns `CVector3` via `sub_82631640`. |
| 0x8262CCD8 | sub_8262CCD8 | FM2_Lua_CLuaObjectNodeSetPosition | 0.91 | Property setter `position`; expects `CVector3`; calls `FM2_Lua_SceneNode_GetChildByIndex_0`. |
| 0x8262CD88 | sub_8262CD88 | FM2_Lua_CLuaObjectNodeGetRotation | 0.92 | Property getter `rotation`; returns `CRotation` via `sub_82631648`. |
| 0x8262CDF0 | sub_8262CDF0 | FM2_Lua_CLuaObjectNodeSetRotation | 0.91 | Property setter `rotation`; expects `CRotation`; calls `sub_82631488`. |
| 0x8262CEA0 | sub_8262CEA0 | FM2_Lua_CLuaObjectNodeGetScale | 0.92 | Property getter `scale`; returns `CVector3` via `sub_82631650`. |
| 0x8262CF08 | sub_8262CF08 | FM2_Lua_CLuaObjectNodeSetScale | 0.91 | Property setter `scale`; expects `CVector3`; calls `sub_826314E0`. |
| 0x8262CFB8 | sub_8262CFB8 | FM2_Lua_CLuaObjectNodeGetPivot | 0.92 | Property getter `pivot`; returns `CVector3` via `sub_82631658`. |
| 0x8262D020 | sub_8262D020 | FM2_Lua_CLuaObjectNodeSetPivot | 0.91 | Property setter `pivot`; expects `CVector3`; calls `sub_82631538`. |
| 0x8262D0D0 | sub_8262D0D0 | FM2_Lua_CLuaObjectNodeGetPositionVelocity | 0.92 | Property getter `positionVelocity`; returns `CVector3` via `sub_82631660`. |
| 0x8262D138 | sub_8262D138 | FM2_Lua_CLuaObjectNodeSetPositionVelocity | 0.91 | Property setter `positionVelocity`; expects `CVector3`; calls `sub_82631590`. |
| 0x8262D1E8 | sub_8262D1E8 | FM2_Lua_CLuaObjectNodeGetRotationVelocity | 0.91 | Property getter `rotationVelocity`; returns `CRotation` via `FM2_Thunk_10`. |
| 0x8262D250 | sub_8262D250 | FM2_Lua_CLuaObjectNodeSetRotationVelocity | 0.91 | Property setter `rotationVelocity`; expects `CRotation`; calls `sub_826315E8`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262D368 | sub_8262D368 | `localTransform` getter; defer with transform thunk cluster. |
| 0x8262D5D0 | sub_8262D5D0 | `CLuaObjectEvent.new`; defer event binding cluster. |
