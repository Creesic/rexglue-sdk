## Iteration 240

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82624B60 | sub_82624B60 | FM2_Lua_CColorSetFromOtherColor | 0.93 | Registered as `setColor` in `FM2_Lua_RegisterCColorToluaBindings`; copies `const CColor` arg via `sub_827EB280`. |
| 0x82624C70 | sub_82624C70 | FM2_Lua_CColorToString | 0.93 | Registered as `tostring`; error string `CLuaBindings::ColorToString`; formats via `sub_82635190`. |
| 0x82624D38 | sub_82624D38 | FM2_Lua_CColorCopy | 0.92 | Registered as `copy`; calls `FM2_Lua_Color_Copy` and registers GC userdata. |
| 0x82624DF8 | sub_82624DF8 | FM2_Lua_CColorEquals | 0.92 | Registered as `.eq`; compares two `CColor` via `sub_827EB040`; pushes bool. |
| 0x82624F00 | sub_82624F00 | FM2_Lua_CColorGetRed | 0.94 | Property getter `r`; reads byte at offset 0; error `accessing variable 'r'`. |
| 0x82624F78 | sub_82624F78 | FM2_Lua_CColorSetRed | 0.94 | Property setter `r`; writes via `sub_827EB060`. |
| 0x82625030 | sub_82625030 | FM2_Lua_CColorGetGreen | 0.94 | Property getter `g`; reads byte at offset 1. |
| 0x826250A8 | sub_826250A8 | FM2_Lua_CColorSetGreen | 0.94 | Property setter `g`; writes via `sub_827EB0B0`. |
| 0x82625160 | sub_82625160 | FM2_Lua_CColorGetBlue | 0.94 | Property getter `b`; reads byte at offset 2. |
| 0x826251D8 | sub_826251D8 | FM2_Lua_CColorSetBlue | 0.94 | Property setter `b`; writes via `sub_827EB100`. |
| 0x82625290 | sub_82625290 | FM2_Lua_CColorGetAlpha | 0.94 | Property getter `a`; reads byte at offset 3. |
| 0x82625308 | sub_82625308 | FM2_Lua_CColorSetAlpha | 0.94 | Property setter `a`; writes via `sub_827EB150`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262BB68 | sub_8262BB68 | `CLuaObjectMaterial` module registration; defer material property cluster. |
| 0x8262AEA0 | sub_8262AEA0 | `CLuaTimeContext` module registration; defer time-context cluster. |
| 0x82632F40 | sub_82632F40 | Thin `FM2_Lua_SetStackTop(a1, -2)` after module pop. |
| 0x82631DA8 | sub_82631DA8 | 4-byte thunk to ambient-color lazy init. |
| 0x82631C50 | sub_82631C50 | Lazy ambient CColor allocator; defer with material getters. |
