## Iteration 260

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82631430 | FM2_Lua_SceneNode_GetChildByIndex_0 | FM2_SceneNode_AssignPositionFromUserdata | 0.92 | Misname fix; sole callee from `FM2_Lua_CLuaObjectNodeSetPosition`; same pattern as other `Assign*FromUserdata` helpers. |
| 0x826315E8 | sub_826315E8 | FM2_SceneNode_AssignRotationVelocityFromUserdata | 0.91 | Sole callee from `FM2_Lua_CLuaObjectNodeSetRotationVelocity`; caches rotation velocity then `InvokeElementCallbackWithVec128`. |
| 0x8260FAB8 | sub_8260FAB8 | FM2_Lua_GetOrCreateBindingElementFromProfile | 0.90 | Thin wrapper: `GetOrCreateObjectElementForBinding(*(profile+20))`; used by presentation lookup, event target, scene element. |
| 0x8260FCB0 | sub_8260FCB0 | FM2_SceneGraph_CreateBindingElementFromXmlTypeName | 0.91 | Backend of `FM2_Lua_CreateBindingElementFromTypeName`; `Xml_GetTypeHandleFromNameBuffer` + `GetMaterialNodeByDwordKeyAndNotify`. |
| 0x82633828 | sub_82633828 | FM2_Lua_PushObjectEventTargetBindingElement | 0.89 | Sole backend for `FM2_Lua_CLuaObjectEventGetTarget`; resolves target id then `GetOrCreateBindingElementFromProfile`. |
| 0x82633910 | sub_82633910 | FM2_Lua_GetObjectEventNameCString | 0.88 | Sole backend for `FM2_Lua_CLuaObjectEventGetName`; returns `std::string`-like name buffer as C string. |
| 0x82632F40 | sub_82632F40 | FM2_Lua_PopToluaModuleStack | 0.90 | Wrapper `SetStackTop(-2)`; called twice after every tolua module registration. |
| 0x826342D0 | sub_826342D0 | FM2_Lua_ToluaValidateBooleanArg | 0.91 | Type-check helper; fills error with `"boolean"`; used by `active`/`opacity` setters. |
| 0x82633B98 | sub_82633B98 | FM2_Lua_GetStackSlotTruthyOrDefault | 0.90 | Reads stack slot via `IsStackSlotTruthy` or returns default; paired with boolean validator. |
| 0x82631740 | sub_82631740 | FM2_Lua_CMaterialReadSpecularEnableField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetSpecularEnable`; binding slot 18 bool via `XmlTree_ResolveIndexedChildChain_0`. |
| 0x826317C0 | sub_826317C0 | FM2_Lua_CMaterialReadEmissivePowerField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetEmissivePower`; binding slot 20 float. |
| 0x82631810 | sub_82631810 | FM2_Lua_CMaterialReadShadeTypeField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetShadeType`; binding slot 12 byte enum. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82631640 | j_FM2_SceneNode_GetOrCachePositionVector | 4-byte thunk; IDA already reflects target name. |
| 0x82631970 | sub_82631970 | Material float writer cluster; defer with blend/cull/fill setters. |
