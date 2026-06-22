## Iteration 245

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262FA30 | sub_8262FA30 | FM2_Lua_RegisterAkClassGraphToluaBindings | 0.92 | Registers `akclassgraph` module with attach/detach/parent from `InitBindingObjectContext`. |
| 0x8262FE30 | sub_8262FE30 | FM2_Lua_RegisterAkSceneGraphToluaBindings | 0.92 | Registers `akscenegraph` module with attach/detach/attachBefore. |
| 0x827E8EB8 | sub_827E8EB8 | FM2_CTimeContext_PauseTimeline | 0.91 | Pauses timeline: invalidates pass 26 or seeks node 5; sets pass flag 22; pair of `FM2_CTimeContext_PlayTimeline`. |
| 0x827E8F88 | sub_827E8F88 | FM2_CTimeContext_SetPassFlagFromPauseState | 0.89 | Sets pass flag 22 if playing else 26 based on `FM2_Diag_LogEventWithSubContext`; used from go-to-time path. |
| 0x82634588 | sub_82634588 | FM2_Lua_PushCStringOrNil | 0.91 | Pushes C string via `FM2_Lua_PushLStringOrNil` or nil; shared by Color/Matrix/Vector/Rotation/TimeContext tostring. |
| 0x82632A50 | sub_82632A50 | FM2_Lua_UnregisterToluaGcEntry | 0.90 | Clears `tolua_gc` registry entry for userdata pointer; called from GC collector epilogue. |
| 0x82625568 | sub_82625568 | FM2_Lua_CMatrixNew | 0.93 | Registered as `Matrix.new`; allocates `D3DXMATRIX` and registers GC userdata. |
| 0x826256F0 | sub_826256F0 | FM2_Lua_CMatrixMultiply | 0.93 | Registered as `multiply`; calls `FM2_Render_MultiplyGMatrix3DInPlaceSimd`. |
| 0x826257F8 | sub_826257F8 | FM2_Lua_CMatrixTranspose | 0.92 | Registered as `transpose`; error string references `Transpose`. |
| 0x826258B0 | sub_826258B0 | FM2_Lua_CMatrixIdentity | 0.92 | Registered as `identity`; resets matrix via `D3DXMATRIX` ctor. |
| 0x82625968 | sub_82625968 | FM2_Lua_CMatrixToString | 0.93 | Registered as `tostring`; `CLuaBindings::MatrixToString` error string. |
| 0x82628E28 | sub_82628E28 | FM2_Lua_CVector3New | 0.93 | Registered as `Vector.new`; allocates vector via `sub_827EB510`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82634830 | sub_82634830 | 8-byte forwarder to `FM2_CTimeContext_PlayTimeline`. |
| 0x82634838 | sub_82634838 | 8-byte forwarder to `FM2_CTimeContext_PauseTimeline`. |
| 0x82634840 | sub_82634840 | Thin ms-conversion wrapper; defers to pause-state pass-flag backend. |
| 0x82634888 | sub_82634888 | 8-byte forwarder to `FM2_Diag_LogEvent22`. |
| 0x826347F8 | sub_826347F8 | Thin GC epilogue: unregister + `SetStackTop(-2)`. |
| 0x826349E0 | sub_826349E0 | Clears description string field on GC; defer destroy cluster. |
| 0x82632F40 | sub_82632F40 | Thin `FM2_Lua_SetStackTop(a1, -2)`. |
