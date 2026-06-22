## Iteration 244

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82627AD8 | sub_82627AD8 | FM2_Lua_RegisterCMatrixToluaBindings | 0.94 | Registers `Matrix`/`CMatrix` module with multiply/transpose/scale/translate/rotate and m11–m44 accessors. |
| 0x82628BD0 | sub_82628BD0 | FM2_Lua_RegisterCRotationToluaBindings | 0.94 | Registers `Rotation`/`CRotation` with lookAt/add/subtract/scale and x/y/z properties. |
| 0x8262A6F8 | sub_8262A6F8 | FM2_Lua_RegisterCVector3ToluaBindings | 0.94 | Registers `Vector`/`CVector3` with distance/dot/cross/normalize/transform methods. |
| 0x8262D3D0 | sub_8262D3D0 | FM2_Lua_RegisterCLuaObjectNodeToluaBindings | 0.93 | Registers `Node`/`CLuaObjectNode` with position/rotation/scale/pivot and calcBBox methods. |
| 0x8262C680 | sub_8262C680 | FM2_Lua_RegisterCLuaObjectTextToluaBindings | 0.93 | Registers `Text`/`CLuaObjectText` with textString/scroll/box/renderStyle/textColor properties. |
| 0x8262C948 | sub_8262C948 | FM2_Lua_RegisterCLuaObjectModelToluaBindings | 0.92 | Registers `Model`/`CLuaObjectModel` extending `CLuaObjectNode`. |
| 0x8262C8A0 | sub_8262C8A0 | FM2_Lua_RegisterCLuaObjectSceneToluaBindings | 0.92 | Registers `Scene`/`CLuaObjectScene` extending `CLuaObjectNode`. |
| 0x8262D860 | sub_8262D860 | FM2_Lua_RegisterCLuaObjectEventToluaBindings | 0.92 | Registers `Event`/`CLuaObjectEvent` with `new`, `name`, and `target` properties. |
| 0x8262F3E8 | sub_8262F3E8 | FM2_Lua_RegisterCLuaObjectElementToluaBindings | 0.93 | Registers `Element`/`CLuaObjectElement` with type/id/active/opacity/parent and get/find methods. |
| 0x82630780 | sub_82630780 | FM2_Lua_RegisterCPresentationToluaBindings | 0.93 | Registers `Presentation`/`CPresentation` with root/runTime/scene and getByID methods. |
| 0x82630240 | sub_82630240 | FM2_Lua_RegisterAkToluaBindings | 0.92 | Registers `ak` module with output/fireEvent/createElement/deleteElement bindings. |
| 0x827E8E10 | sub_827E8E10 | FM2_CTimeContext_PlayTimeline | 0.90 | Resumes timeline: invalidates pass 22, sets pass flag 26; called from `FM2_Lua_CTimeContextPlay`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x827E8EB8 | sub_827E8EB8 | Pause timeline backend; defer paired pass with `827E8F88`. |
| 0x827E8F88 | sub_827E8F88 | Go-to-time pass-flag backend; defer timeline cluster completion. |
| 0x82634830 | sub_82634830 | 8-byte forwarder to `FM2_CTimeContext_PlayTimeline`. |
| 0x82634838 | sub_82634838 | 8-byte forwarder to pause backend. |
| 0x82634840 | sub_82634840 | Thin seconds-to-ms wrapper around go-to-time backend. |
| 0x82634888 | sub_82634888 | 8-byte forwarder to `FM2_Diag_LogEvent22` paused probe. |
| 0x82632F40 | sub_82632F40 | Thin `FM2_Lua_SetStackTop(a1, -2)` after module registration. |
| 0x82634588 | sub_82634588 | Thin push C-string-or-nil helper; defer shared tolua utils pass. |
