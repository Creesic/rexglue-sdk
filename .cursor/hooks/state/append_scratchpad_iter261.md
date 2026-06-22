## Iteration 261

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82631858 | sub_82631858 | FM2_Lua_CMaterialReadBlendModeField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetBlendMode`; binding slot 17 byte enum. |
| 0x826318A0 | sub_826318A0 | FM2_Lua_CMaterialReadFillModeField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetFillMode`; binding slot 21 byte enum. |
| 0x826318E8 | sub_826318E8 | FM2_Lua_CMaterialReadCullingField | 0.92 | Sole callee from `FM2_Lua_CMaterialGetCulling`; binding slot 24 byte enum. |
| 0x82631930 | sub_82631930 | FM2_Lua_CMaterialWriteSpecularEnableField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetSpecularEnable`; slot 18 via `DispatchMaterialPassByNegativeSlot`. |
| 0x82631970 | sub_82631970 | FM2_Lua_CMaterialWriteSpecularPowerField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetSpecularPower`; slot 16 float via `ForEachResourceCacheByPassFlag`. |
| 0x826319B0 | sub_826319B0 | FM2_Lua_CMaterialWriteEmissivePowerField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetEmissivePower`; slot 20 float. |
| 0x826319F0 | sub_826319F0 | FM2_Lua_CMaterialWriteShadeTypeField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetShadeType`; slot 12 byte via `sub_827D6EE8`. |
| 0x82631A30 | sub_82631A30 | FM2_Lua_CMaterialWriteBlendModeField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetBlendMode`; slot 17 byte. |
| 0x82631A70 | sub_82631A70 | FM2_Lua_CMaterialWriteFillModeField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetFillMode`; slot 21 byte. |
| 0x82631AB0 | sub_82631AB0 | FM2_Lua_CMaterialWriteCullingField | 0.91 | Sole callee from `FM2_Lua_CMaterialSetCulling`; slot 24 byte. |
| 0x82631F90 | sub_82631F90 | FM2_Lua_CMaterialSyncBindingFieldNode | 0.89 | Shared by color lazy-init paths; `XmlNavigator_FindMatchingNode` with field slot id. |
| 0x82631AF0 | sub_82631AF0 | FM2_Lua_InitElementAssetBindingObject | 0.88 | Called from `FM2_Lua_GetOrCreateObjectElementForBinding`; sets vtable `off_82113400` element/asset layout. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82631EC0 | sub_82631EC0 | Material binding dtor variant; defer until vtable/type cluster reviewed. |
| 0x82631FF0 | sub_82631FF0 | Additional material float reader; defer next material pass. |
