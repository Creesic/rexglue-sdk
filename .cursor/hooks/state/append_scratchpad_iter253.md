## Iteration 253

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82635618 | FM2_SceneGraph_CompareNodeFields_0 | FM2_SceneGraph_AttachBindingElementsFromAkLua | 0.91 | Misnamed; sole caller `FM2_Lua_AkSceneGraphAttach`; identical attach path via `FM2_SceneGraph_LinkSubtreeBySlotSelector`. |
| 0x82635678 | FM2_SceneGraph_CompareNodeFields | FM2_SceneGraph_DetachBindingElementsFromAkLua | 0.91 | Misnamed; sole caller `sub_8262FC10`; calls `FM2_SceneGraph_DetachNodeIfParentMatches`. |
| 0x826356D8 | sub_826356D8 | FM2_SceneGraph_AttachBindingElementsBefore | 0.90 | Sole caller `FM2_Lua_CLuaObjectElementAttachBefore`; optional sibling id `-1`; calls `FM2_SceneGraph_LinkOrInsertSubtreeBeforeSibling`. |
| 0x8260FE50 | sub_8260FE50 | FM2_SceneGraph_LinkOrInsertSubtreeBeforeSibling | 0.89 | Insert-before-sibling attach; validates parent via `sub_827D6938`; `sub_827DD1A0` or `FM2_Render_LinkUnitStringSubtreeBySlotSelector`. |
| 0x8262FC10 | sub_8262FC10 | FM2_Lua_AkSceneGraphDetach | 0.93 | Registered as `detach` in `FM2_Lua_RegisterAkSceneGraphToluaBindings`; `#ferror in function 'detach'`. |
| 0x8262D958 | sub_8262D958 | FM2_Lua_CLuaObjectElementGetType | 0.92 | Property getter `type`; error `invalid 'self' in accessing variable 'type'`. |
| 0x8262D9C8 | sub_8262D9C8 | FM2_Lua_CLuaObjectElementGetId | 0.92 | Property getter `id`; pushes controller id via `FM2_ProfileState_GetControllerIdAt12`. |
| 0x8262DA48 | sub_8262DA48 | FM2_Lua_CLuaObjectElementGetActive | 0.92 | Property getter `active`; `FM2_Lua_GetBindingDeferredBoolFromXmlChain1`. |
| 0x8262DAC0 | sub_8262DAC0 | FM2_Lua_CLuaObjectElementSetActive | 0.91 | Property setter `active`; dispatches `FM2_Lua_DispatchBindingDeferredMaterialPass1`. |
| 0x8262DB78 | sub_8262DB78 | FM2_Lua_CLuaObjectElementGetGlobalActive | 0.92 | Property getter `globalactive`; recursive pass0 bool. |
| 0x8262DBF0 | sub_8262DBF0 | FM2_Lua_CLuaObjectElementGetOpacity | 0.92 | Property getter `opacity`; `FM2_Lua_GetBindingDeferredFloatFromXmlChain11`. |
| 0x8262DC60 | sub_8262DC60 | FM2_Lua_CLuaObjectElementSetOpacity | 0.91 | Property setter `opacity`; expects float arg; opacity variable assignment errors. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262DD10 | sub_8262DD10 | Element `parent` getter; defer with tostring/.eq cluster. |
| 0x8262DD78 | sub_8262DD78 | Element `tostring`; defer method cluster. |
