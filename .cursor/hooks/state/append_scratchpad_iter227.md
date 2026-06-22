## Iteration 227

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82615DD8 | sub_82615DD8 | FM2_Binding_ParsePropertyTypeNameToId | 0.94 | strcmp chain maps binding property type strings (`char`, `float`, `bool`, `list`, `string`, `float2`…`matrix16`) to numeric ids. |
| 0x826162A8 | sub_826162A8 | FM2_Binding_ParseColonDelimitedPropertyTypeAndValue | 0.91 | Parses `type:` / `type;` prefix (max 10 chars); calls type-name parser; for `list` extracts value substring after `;`. |
| 0x82615B98 | sub_82615B98 | FM2_Lua_ImportBindingAnimationTracksFromXml | 0.92 | Walks lua/XML `animationtrack` nodes; reads `id`/`time`/`value` keyframes; uploads float arrays via profile variant setters. |
| 0x82615B40 | sub_82615B40 | FM2_Binding_LookupPassFlagByElementTypeId | 0.90 | Linear search `dword_829A56D8` triple table (181 entries); returns pass flag dword and optional out-param; type 28 fallback returns 5. |
| 0x82615918 | sub_82615918 | FM2_Profile_GetOrAllocThreadLocalTypeId | 0.89 | Lazy-init under `FM2_ThreadLocalCsArray_LockByType`; assigns `++dword_82A438A8` when unset. |
| 0x82615A60 | sub_82615A60 | FM2_Profile_GetDeferredTaskDwordFieldByIndex | 0.90 | Indexed dword array lookup on deferred-task params; if zero and parent-flag set, falls back via `FM2_Profile_GetGlobalDeferredTaskParams`. |
| 0x8275EF98 | sub_8275EF98 | FM2_Profile_GetGlobalDeferredTaskParams | 0.88 | Returns global `dword_82A438A0`; used as deferred-task parent fallback. |
| 0x827DD110 | sub_827DD110 | FM2_Render_GetMaterialNodeByTypeNameForProfile | 0.90 | Resolves material node by type name from profile scene graph (`a1+516`); optional pass filter; writes node ptr to out-param. |
| 0x825446A0 | sub_825446A0 | FM2_SharedIntrusiveList_AssignAndAddRef | 0.89 | Assigns intrusive-list head; calls vtable `+4` addref on source node when non-null. |
| 0x82544888 | sub_82544888 | FM2_RbTreeNode_InitWithKeyAndSharedList | 0.90 | Inits RB-tree node key fields and embeds shared child list at `+16` via assign/addref helper. |
| 0x82612660 | sub_82612660 | FM2_Lua_ThunkDispatchOnInitializeFromXmlLua | 0.88 | Callback stored at `a1[10]` in presentation-element init; forwards to `FM2_Lua_DispatchBindingOnInitializeFromXmlLuaScriptlet`. |
| 0x82610E38 | sub_82610E38 | FM2_Lua_ThunkForEachBindingIntVectorInvokeProfileCallback | 0.87 | Thin thunk; passes `*(a1+4)` and callback to `FM2_Lua_ForEachBindingIntVectorInvokeProfileCallback`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826158C0 | sub_826158C0 | Locale char-class table init; defer lexer cluster with `82615AE8`. |
| 0x82615AE8 | sub_82615AE8 | Copies char-class state into binding script lexer object; defer with `826185F8`. |
| 0x82616678 | sub_82616678 | Nested binding-object vtable offset fixup; defer dtor/vtable cluster. |
| 0x826166E0 | sub_826166E0 | Refcount release dtor; defer with `82616908`/`8275F698` cluster. |
| 0x82616758 | sub_82616758 | Minimal vtable dtor stub (`off_8210EEB4`). |
| 0x826167B0 | sub_826167B0 | Clears triple pointer slots in binding parse state; needs struct context. |
| 0x82616838 | sub_82616838 | Streambuf-style ctor with optional skip; defer iostream cluster. |
| 0x82616970 | sub_82616970 | Stream get-char underflow; defer iostream cluster. |
| 0x82616FD8 | sub_82616FD8 | Multi-level dtor calling vtable fixup; defer dtor cluster. |
| 0x8275EC78 | sub_8275EC78 | Locale facet + char-class table calloc; defer lexer cluster. |
