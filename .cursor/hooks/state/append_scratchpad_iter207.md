## Iteration 207

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8261FF78 | sub_8261FF78 | FM2_Lua_CustomPrintStackViaMetatableTostring | 0.92 | Iterates stack calling metatable `tostring`; tab-separates into buffer; error string references `` `print' ``; invokes callback at `a1[1]`; registered as `"print"` gc target. |
| 0x826200D0 | sub_826200D0 | FM2_Lua_GcFinalizeBindingPrintContext | 0.91 | Lua gc finalizer; `FM2_Lua_GetLightUserdataAndRestoreStack` then `FM2_Lua_CustomPrintStackViaMetatableTostring`; set on globals `"print"` in `FM2_Lua_InitBindingScriptContext`. |
| 0x827EACC8 | sub_827EACC8 | FM2_Render_GetMaterialTypeHandleFromXmlName | 0.90 | Wraps `FM2_Xml_GetTypeHandleFromNameBuffer`; returns dword type handle for material hash lookup; 4 material-resolve callers. |
| 0x827E52A8 | sub_827E52A8 | FM2_Render_GetMaterialNodeByTypeNameAndNotify | 0.90 | Xml name → type handle → `FM2_Render_LookupMaterialHashBucketByDwordKey` → `sub_827E47C0`; optional profile callback. |
| 0x827E5368 | sub_827E5368 | FM2_Render_ResolveMaterialNodeByTypeNameWithPassAndNotify | 0.89 | Type-name hash path + `sub_827E47D0` with pass id; optional `FM2_Profile_InvokeManagerCallbacksUnderCritSec`. |
| 0x827E51C8 | sub_827E51C8 | FM2_Render_GetMaterialNodeFromSlotAndNotify | 0.88 | Resolves existing material slot via `sub_827E47C0` (no hash); optional profile manager callback; 3 callers. |
| 0x8261FA88 | sub_8261FA88 | FM2_Lua_LoadOrAppendScriptBufferToContext | 0.89 | Tries `FM2_LuaIO_LoadScriptBufferWithMode`; on success appends result string + optional callback `a1[11]`; else allocates `FM2_Lua_InitScriptLoaderBindingObject`. |
| 0x82620580 | sub_82620580 | FM2_Lua_InitScriptLoaderBindingObject | 0.90 | `FM2_Lua_InitBindingSlotFromStackIndex` + vector grow + clears string/callback fields at `+36/+44/+48`; 9 script-loader callers. |
| 0x8254DDF8 | sub_8254DDF8 | FM2_LuaIO_LoadScriptBufferWithMode | 0.91 | Packages buffer ptr/len; `FM2_LuaIO_OpenFileWithMode` with reader `sub_8254DDD0`. |
| 0x82620230 | sub_82620230 | FM2_Lua_GetBindingErrorMessageByIndex | 0.92 | Returns `off_829A5F88[index]` C string table (`E_LUAERROR_START`, event-param errors, etc.); used by `sub_8260FAC0` before `FM2_Lua_ErrorVprintf`. |
| 0x82620EF0 | sub_82620EF0 | FM2_Lua_InitBindingSlotWithStdStringLString | 0.90 | Pushes `std::string` contents as Lua string; records stack depth; type tag `4`; used in material pass-10 dispatch loop. |
| 0x826105F0 | sub_826105F0 | FM2_Render_DispatchMaterialPass10OnBindingObjectList | 0.88 | Skips if `sub_827E0650` pass-10 flag set; walks intrusive binding-object list; protected Lua calls per entry; `FM2_Render_DispatchMaterialPassByNegativeSlot(10)`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261F990 | sub_8261F990 | Thin wrapper → `FM2_Lua_ErrorVprintf` only. |
| 0x8260FAC0 | sub_8260FAC0 | 64B error-index thunk; covered by message table + ErrorVprintf. |
| 0x8254DDD0 | sub_8254DDD0 | 40B IO reader chunk copy; defer with LuaIO cluster. |
| 0x827E47C0 | sub_827E47C0 | 16B wrapper → `sub_827F5590`. |
| 0x827D78E8 | sub_827D78E8 | Thin wrapper → `sub_827D77D8` after flag lookup. |
| 0x827D77D8 | sub_827D77D8 | Recursive material pass-cache invalidation; defer with resource-cache cluster. |
| 0x827E0650 | sub_827E0650 | Pass-flag bit test helper; defer with profile flag naming cluster. |
| 0x82611DC0 | sub_82611DC0 | Generic 16-byte vector append; low subsystem specificity. |
| 0x826203F8 | sub_826203F8 | Protected binding-event invoke; defer with `sub_826208D8` cluster. |
| 0x826208D8 | sub_826208D8 | Pushes binding-variant vector onto stack; needs event-dispatch context. |
| 0x827DD170 | sub_827DD170 | Thin 3-way dispatch to `sub_827EF3D0`. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch; needs sub-ctor cluster. |
