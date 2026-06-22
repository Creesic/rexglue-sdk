## Iteration 258

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262D368 | sub_8262D368 | FM2_Lua_CLuaObjectNodeGetLocalTransform | 0.92 | Property getter `localTransform`; returns `CMatrix` via `sub_82631038`. |
| 0x8262D5D0 | sub_8262D5D0 | FM2_Lua_CLuaObjectEventNew | 0.93 | Registered as `new` on `CLuaObjectEvent`; two string args; `#ferror in function 'new'`. |
| 0x8262D6E0 | sub_8262D6E0 | FM2_Lua_CLuaObjectEventGetName | 0.92 | Property getter `name`; pushes C string via `sub_82633910`. |
| 0x8262D750 | sub_8262D750 | FM2_Lua_CLuaObjectEventSetName | 0.91 | Property setter `name`; validates string arg; `_name` assignment errors. |
| 0x8262D7F8 | sub_8262D7F8 | FM2_Lua_CLuaObjectEventGetTarget | 0.92 | Property getter `target`; returns `CLuaObjectElement` via `sub_82633828`. |
| 0x82630D80 | sub_82630D80 | FM2_SceneNode_GetOrCacheGlobalTransformMatrix | 0.90 | Caches matrix at node `+80`; `GetOrBuildCachedMaterialWorldMatrix`; caller of lastGlobalTransform thunk. |
| 0x82630E08 | sub_82630E08 | FM2_SceneNode_GetOrCacheLocalTransformMatrix | 0.90 | Caches matrix at node `+84`; `BuildMaterialLocalMatrixFromElementPasses`; localTransform backend. |
| 0x82634BC8 | sub_82634BC8 | FM2_Lua_CreateBindingElementFromTypeName | 0.91 | Sole callee from `FM2_Lua_AkCreateElement`; XML type name → `GetOrCreateObjectElementForBinding`. |
| 0x82634DF8 | sub_82634DF8 | FM2_Lua_GetPresentationRootElement | 0.90 | Sole callee from `FM2_Lua_CPresentationGetRoot`; `GetNodeFromHandle` on lap-tracker state. |
| 0x82634E30 | sub_82634E30 | FM2_Lua_GetPresentationRunTimeSeconds | 0.90 | Sole callee from `FM2_Lua_CPresentationGetRunTime`; ms→seconds `* 0.001`. |
| 0x82634E80 | sub_82634E80 | FM2_Lua_GetPresentationSceneElement | 0.90 | Sole callee from `FM2_Lua_CPresentationGetScene`; scene handle lookup via `sub_8260FAB8`. |
| 0x82634EE0 | sub_82634EE0 | FM2_Lua_LookupPresentationElementById | 0.91 | Callee from `getByID`/`getById`; numeric id lookup via `sub_8260FAB8`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82631030 | sub_82631030 | Thin thunk to global-transform cache; defer vector-cache cluster. |
| 0x82631040 | sub_82631040 | Position vector cache; defer with rotation/scale cache cluster. |
