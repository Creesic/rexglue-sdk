## Iteration 243

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262AA20 | sub_8262AA20 | FM2_Lua_CTimeContextGetPaused | 0.93 | Property getter `paused`; reads bool via `sub_82634888`. |
| 0x8262AA98 | sub_8262AA98 | FM2_Lua_CTimeContextGetTime | 0.93 | Property getter `time`; pushes playback seconds from `FM2_CTimeContext_GetPlaybackTimeSeconds`. |
| 0x8262AB08 | sub_8262AB08 | FM2_Lua_CTimeContextToString | 0.92 | Registered as `tostring`; formats via `FM2_Lua_CTimeContextFormatDescriptionString`. |
| 0x8262ABD0 | sub_8262ABD0 | FM2_Lua_CTimeContextPlay | 0.92 | Registered as `play`; calls `sub_827E8E10` on bound node handle. |
| 0x8262AC88 | sub_8262AC88 | FM2_Lua_CTimeContextPause | 0.92 | Registered as `pause`; calls `sub_827E8EB8`. |
| 0x8262AD40 | sub_8262AD40 | FM2_Lua_CTimeContextGoToTime | 0.92 | Registered as `goToTime`; converts seconds to ms and calls `sub_827E8F88`. |
| 0x82631BA8 | sub_82631BA8 | FM2_Lua_CMaterialGetOrCreateDiffuseColor | 0.91 | Lazy-inits diffuse `CColor` at `a1+56` from material field id 15. |
| 0x82631CF8 | sub_82631CF8 | FM2_Lua_CMaterialGetOrCreateSpecularColor | 0.91 | Lazy-inits specular `CColor` at `a1+64` from material field id 23. |
| 0x82631DB8 | sub_82631DB8 | FM2_Lua_CMaterialAssignDiffuseColor | 0.90 | Gets/creates diffuse color then copies source `CColor` via `sub_827EB280`. |
| 0x82631E68 | sub_82631E68 | FM2_Lua_CMaterialAssignSpecularColor | 0.90 | Gets/creates specular color then copies source `CColor`. |
| 0x826349F0 | sub_826349F0 | FM2_Lua_CTimeContextFormatDescriptionString | 0.91 | Builds `"TimeContext: time: ..."` string with paused TRUE/FALSE suffix. |
| 0x82634938 | sub_82634938 | FM2_CTimeContext_GetPlaybackTimeSeconds | 0.90 | Reads XML node type 5 time value; scales by `0.001` to seconds. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82634888 | sub_82634888 | 8-byte forwarder to `FM2_Diag_LogEvent22`; thin paused-flag probe. |
| 0x82634830 | sub_82634830 | 8-byte forwarder to `sub_827E8E10` (play backend). |
| 0x82634838 | sub_82634838 | 8-byte forwarder to `sub_827E8EB8` (pause backend). |
| 0x82634840 | sub_82634840 | Thin ms-conversion wrapper around `sub_827E8F88`. |
| 0x826349E0 | sub_826349E0 | Clears string at `a1+8` on GC; defer with binding destroy cluster. |
| 0x826347F8 | sub_826347F8 | Shared tolua GC stack-pop helper; thin wrapper. |
| 0x82631DA0 | sub_82631DA0 | 4-byte thunk to diffuse lazy init. |
| 0x82631DB0 | sub_82631DB0 | 4-byte thunk to specular lazy init. |
