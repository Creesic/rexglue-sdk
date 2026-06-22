## Iteration 252

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826278B8 | sub_826278B8 | FM2_Lua_CMatrixGetProperty_43 | 0.92 | Pushes float at offset 56; error `invalid 'self' in accessing variable '_43'`; registered in `FM2_Lua_RegisterCMatrixToluaBindings`. |
| 0x82627920 | sub_82627920 | FM2_Lua_CMatrixSetProperty_43 | 0.92 | Sets float at offset 56; `_43` variable assignment errors. |
| 0x826279C8 | sub_826279C8 | FM2_Lua_CMatrixGetProperty_44 | 0.92 | Pushes float at offset 60; error `invalid 'self' in accessing variable '_44'`. |
| 0x82627A30 | sub_82627A30 | FM2_Lua_CMatrixSetProperty_44 | 0.92 | Sets float at offset 60; `_44` variable assignment errors. |
| 0x82635510 | sub_82635510 | FM2_SceneGraph_AttachBindingElements | 0.91 | Sole callee from `FM2_Lua_CLuaObjectElementAttach`; extracts node ids at `+12`; calls `FM2_SceneGraph_LinkSubtreeBySlotSelector`. |
| 0x82635570 | sub_82635570 | FM2_SceneGraph_DetachBindingElements | 0.91 | Sole callee from `FM2_Lua_CLuaObjectElementDetach`; calls `FM2_SceneGraph_DetachNodeIfParentMatches`. |
| 0x826355D0 | sub_826355D0 | FM2_SceneGraph_GetParentBindingElement | 0.90 | Sole callee from `FM2_Lua_CLuaObjectElementParent`; lookup via `FM2_Render_LookupUnitStringChildIdBySlotSelector`. |
| 0x8262FB10 | sub_8262FB10 | FM2_Lua_AkSceneGraphAttach | 0.93 | Registered as `attach` in `FM2_Lua_RegisterAkSceneGraphToluaBindings`; calls `FM2_SceneGraph_CompareNodeFields_0`. |
| 0x8262FD10 | sub_8262FD10 | FM2_Lua_CLuaObjectElementAttachBefore | 0.93 | Registered as `attachBefore`; three `CLuaObjectElement` args; `#ferror in function 'attachBefore'`. |
| 0x8260FDC0 | sub_8260FDC0 | FM2_SceneGraph_LinkSubtreeBySlotSelector | 0.90 | Wraps `FM2_Render_LinkUnitStringSubtreeBySlotSelector`; attach backend used by binding-element helpers. |
| 0x8260FDF0 | sub_8260FDF0 | FM2_SceneGraph_DetachNodeIfParentMatches | 0.89 | Verifies parent id via `sub_827D6938`; detaches via `sub_827DD1D0`; detach backend. |
| 0x8260FEF0 | sub_8260FEF0 | FM2_SceneGraph_GetBindingElementByChildSlot | 0.89 | Child slot lookup; returns `FM2_Lua_GetOrCreateObjectElementForBinding` on success. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826356D8 | sub_826356D8 | `attachBefore` backend; defer with `FM2_SceneGraph_CompareNodeFields_0` misname cleanup. |
| 0x82635618 | FM2_SceneGraph_CompareNodeFields_0 | Misnamed attach wrapper; defer dedicated misname-fix pass. |
