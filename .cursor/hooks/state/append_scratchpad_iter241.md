## Iteration 241

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262BB68 | sub_8262BB68 | FM2_Lua_RegisterCLuaObjectMaterialToluaBindings | 0.93 | Registers `Material`/`CLuaObjectMaterial` module with specular/emissive/blend/culling/fill/shade/ambient/diffuse/specular color property accessors. |
| 0x8262C838 | sub_8262C838 | FM2_Lua_CLuaObjectMaterialToluaGcCollector | 0.92 | Tolua GC for `CLuaObjectMaterial`; invokes vtable dtor then frees userdata block. |
| 0x8262AFD0 | sub_8262AFD0 | FM2_Lua_CMaterialGetSpecularEnable | 0.93 | Property getter `specularEnable`; reads bool via `sub_82631740`. |
| 0x8262B048 | sub_8262B048 | FM2_Lua_CMaterialSetSpecularEnable | 0.92 | Property setter `specularEnable`; bool from stack arg 2. |
| 0x8262B100 | sub_8262B100 | FM2_Lua_CMaterialGetSpecularPower | 0.93 | Property getter `specularPower`; pushes float from `FM2_Helper_1770_2`. |
| 0x8262B170 | sub_8262B170 | FM2_Lua_CMaterialSetSpecularPower | 0.92 | Property setter `specularPower`; writes float via `sub_82631970`. |
| 0x8262B220 | sub_8262B220 | FM2_Lua_CMaterialGetEmissivePower | 0.93 | Property getter `emissivePower`; reads via `sub_826317C0`. |
| 0x8262B290 | sub_8262B290 | FM2_Lua_CMaterialSetEmissivePower | 0.92 | Property setter `emissivePower`; writes via `sub_826319B0`. |
| 0x8262B888 | sub_8262B888 | FM2_Lua_CMaterialSetAmbientColor | 0.92 | Property setter `ambient`; type-checks `CColor` arg; assigns via `FM2_Lua_SceneNode_GetChildByIndex`. |
| 0x8262B938 | sub_8262B938 | FM2_Lua_CMaterialGetDiffuseColor | 0.92 | Property getter `diffuse`; pushes lazy `CColor` userdata. |
| 0x8262B9A0 | sub_8262B9A0 | FM2_Lua_CMaterialSetDiffuseColor | 0.92 | Property setter `diffuse`; copies `CColor` via `sub_82631DB8`. |
| 0x82631C50 | sub_82631C50 | FM2_Lua_CMaterialGetOrCreateAmbientColor | 0.90 | Lazy-inits ambient `CColor` at `a1+60` from material field `a1+52`; used by ambient getter thunk. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262BA50 | sub_8262BA50 | Specular color getter; defer with remaining material color accessors. |
| 0x8262BAB8 | sub_8262BAB8 | Specular color setter; defer same cluster. |
| 0x8262B340 | sub_8262B340 | `blendMode` getter; defer enum property cluster. |
| 0x8262B3C0 | sub_8262B3C0 | `blendMode` setter; defer enum property cluster. |
| 0x82631DA0 | sub_82631DA0 | 4-byte thunk to `sub_82631BA8` (diffuse lazy init). |
| 0x82631DA8 | sub_82631DA8 | 4-byte thunk to ambient lazy init. |
| 0x8262AEA0 | sub_8262AEA0 | `CLuaTimeContext` module registration; defer time-context cluster. |
