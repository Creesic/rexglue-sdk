## Iteration 256

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262FF10 | sub_8262FF10 | FM2_Lua_AkOutput | 0.91 | Registered in `FM2_Lua_RegisterAkToluaBindings` as `output`; 10 stack args; calls `sub_82634C40`. |
| 0x82630100 | sub_82630100 | FM2_Lua_AkCreateElement | 0.93 | Registered as `createElement`; type string; returns `CLuaObjectElement` via `sub_82634BC8`. |
| 0x826301A8 | sub_826301A8 | FM2_Lua_AkDeleteElement | 0.92 | Registered as `deleteElement`; takes `CLuaObjectElement`; `#ferror in function 'deleteElement'`. |
| 0x82630348 | sub_82630348 | FM2_Lua_CPresentationGetRoot | 0.92 | Property getter `root`; pushes `CElement` userdata. |
| 0x826303B0 | sub_826303B0 | FM2_Lua_CPresentationToString | 0.93 | Registered as `tostring` on `CPresentation`; `ToString` error string. |
| 0x82630478 | sub_82630478 | FM2_Lua_CPresentationGetRunTime | 0.92 | Property getter `runTime`; pushes float via `sub_82634E30`. |
| 0x826304E8 | sub_826304E8 | FM2_Lua_CPresentationGetScene | 0.92 | Property getter `scene`; returns `CLuaObjectScene` userdata. |
| 0x82630550 | sub_82630550 | FM2_Lua_CPresentationGetByID | 0.92 | Registered as `getByID`; numeric id; returns `CLuaObjectElement`. |
| 0x82630668 | sub_82630668 | FM2_Lua_CPresentationGetById | 0.92 | Registered as `getById`; same backend as `getByID`; `#ferror in function 'getById'`. |
| 0x8262C9F0 | sub_8262C9F0 | FM2_Lua_CLuaObjectNodeCalcBBox | 0.92 | Registered as `calcBBox`; two `CVector3` args; `CLuaObjectNode` binding. |
| 0x8262CB30 | sub_8262CB30 | FM2_Lua_CLuaObjectNodeCalcBBoxSelf | 0.92 | Registered as `calcBBoxSelf`; self bbox variant on `CLuaObjectNode`. |
| 0x8262D300 | sub_8262D300 | FM2_Lua_CLuaObjectNodeGetLastGlobalTransform | 0.92 | Property getter `lastGlobalTransform`; returns `CMatrix` userdata. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262D368 | sub_8262D368 | Node `localTransform` getter; defer with transform backend cluster. |
| 0x82634BC8 | sub_82634BC8 | Ak create-element backend; no strings; defer helper cluster. |
