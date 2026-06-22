## Iteration 242

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262BA50 | sub_8262BA50 | FM2_Lua_CMaterialGetSpecularColor | 0.92 | Property getter `specular`; pushes lazy `CColor` via `sub_82631DB0`. |
| 0x8262BAB8 | sub_8262BAB8 | FM2_Lua_CMaterialSetSpecularColor | 0.92 | Property setter `specular`; type-checks `CColor`; assigns via `sub_82631E68`. |
| 0x8262B340 | sub_8262B340 | FM2_Lua_CMaterialGetBlendMode | 0.93 | Property getter `blendMode`; reads dword enum via `sub_82631858`. |
| 0x8262B3C0 | sub_8262B3C0 | FM2_Lua_CMaterialSetBlendMode | 0.92 | Property setter `blendMode`; writes via `sub_82631A30`. |
| 0x8262B478 | sub_8262B478 | FM2_Lua_CMaterialGetCulling | 0.93 | Property getter `culling`; reads via `sub_826318E8`. |
| 0x8262B4F8 | sub_8262B4F8 | FM2_Lua_CMaterialSetCulling | 0.92 | Property setter `culling`; writes via `sub_82631AB0`. |
| 0x8262B5B0 | sub_8262B5B0 | FM2_Lua_CMaterialGetFillMode | 0.93 | Property getter `fillMode`; reads via `sub_826318A0`. |
| 0x8262B630 | sub_8262B630 | FM2_Lua_CMaterialSetFillMode | 0.92 | Property setter `fillMode`; writes via `sub_82631A70`. |
| 0x8262B6E8 | sub_8262B6E8 | FM2_Lua_CMaterialGetShadeType | 0.93 | Property getter `shadeType`; reads via `sub_82631810`. |
| 0x8262B768 | sub_8262B768 | FM2_Lua_CMaterialSetShadeType | 0.92 | Property setter `shadeType`; writes via `sub_826319F0`. |
| 0x8262AEA0 | sub_8262AEA0 | FM2_Lua_RegisterCLuaTimeContextToluaBindings | 0.93 | Registers `TimeContext`/`CLuaTimeContext` with paused/time props and play/pause/goToTime/tostring. |
| 0x8262AE48 | sub_8262AE48 | FM2_Lua_CLuaTimeContextToluaGcCollector | 0.92 | Tolua GC for `CLuaTimeContext`; clears string field then frees userdata. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262AA20 | sub_8262AA20 | `paused` getter; defer with remaining TimeContext methods. |
| 0x8262AA98 | sub_8262AA98 | `time` getter; defer TimeContext cluster. |
| 0x8262AB08 | sub_8262AB08 | `tostring` method; defer TimeContext cluster. |
| 0x8262ABD0 | sub_8262ABD0 | `play` method; defer TimeContext cluster. |
| 0x82631BA8 | sub_82631BA8 | Lazy diffuse CColor init; defer color-field helper pass. |
| 0x82631DB0 | sub_82631DB0 | 4-byte thunk to specular lazy-init. |
| 0x82631DB8 | sub_82631DB8 | Diffuse color assign helper; defer with `82631BA8`. |
