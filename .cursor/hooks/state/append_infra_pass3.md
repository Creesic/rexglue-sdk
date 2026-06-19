### Infrastructure pass 3 (33 functions)

Lua error/stack path, binding thunk +32, hash/list/CRT/XAudio2 helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824b6f30` | `FM2_Lua_GetUserdataPointer` | Returns userdata ptr (type 2) or userdata+24 (type 7). |
| `0x824b7090` | `FM2_Lua_PushInternedString` | Interns bytes via `sub_824BF480`, pushes string (type 4). |
| `0x824b70f8` | `FM2_Lua_PushLStringOrNil` | Push length-delimited string or nil if `a2` null. |
| `0x824b7148` | `FM2_Lua_PushFormattedStringGrowStack` | Grow stack if needed, delegate to formatted string push. |
| `0x824b74f0` | `FM2_Lua_PushCFunction` | Push C function closure (type 5) via `sub_824BE8F8`. |
| `0x824b76b8` | `FM2_Lua_SetFieldFromCString` | Set table field from C string (`sub_824BCA78`). |
| `0x824b7860` | `FM2_Lua_SetClosureEnvFromStack` | Assign closure/userdata env from value below top. |
| `0x824b7e10` | `FM2_Lua_PushUserdataForKey` | Alloc userdata for registry key lookup; push (type 7). |
| `0x824b7cc8` | `FM2_Lua_ErrorThrow` | Unwind/throw after Lua error (`sub_824BBFE0`). |
| `0x824b7d50` | `FM2_Lua_ErrorAppendMessagePart` | Append formatted part to error message on stack. |
| `0x824b8138` | `FM2_Lua_NumberValuesEqual` | Equality test for two Lua numbers (type 3). |
| `0x824b8318` | `FM2_Lua_PushFormattedString` | Mini printf (`%s/%d/%f/...`) pushing Lua strings/numbers. |
| `0x824bb158` | `FM2_Lua_GrowStack` | Grow Lua stack when near limit (`sub_824BAFA0`). |
| `0x824bb2c8` | `FM2_Lua_UpdateObjectGcMark` | Update object GC mark bits from global state. |
| `0x824bb300` | `FM2_Lua_LinkGrayObject` | Link object into gray list during GC. |
| `0x824bb428` | `FM2_Lua_GetDebugCallInfoLevel` | Resolve call-info level for debug/error prefix. |
| `0x824bc778` | `FM2_Lua_CoerceToNumberSlot` | Coerce stack slot to number (incl. string parse). |
| `0x824bc960` | `FM2_Lua_GetTableField` | Table field lookup with `__index` metamethod loop. |
| `0x824bc0a8` | `FM2_Lua_ErrorFormatAndThrow` | Format error string then throw (`sub_824BBED0`). |
| `0x824bc7e8` | `FM2_Lua_CoerceNumberToString` | Coerce numeric stack slot to interned string. |
| `0x824bf650` | `FM2_Lua_AllocUserdata` | Allocate full userdata with metatable ref. |
| `0x8254d110` | `FM2_Lua_ErrorPrefixWithSourceLocation` | Prefix Lua error with `source:line:` when available. |
| `0x8254d198` | `FM2_Lua_ErrorVprintf` | Vararg Lua error formatter; used by bad-arg/type helpers. |
| `0x824ec320` | `FM2_Lua_AppendBindingEntryAt32` | `FM2_Lua_Register*` thunk: append pair at `a1+32`. |
| `0x8245a740` | `FM2_HashName_AssignVtable_8203CEA4` | Hash-name object dtor: `*a1 = off_8203CEA4`. |
| `0x8245dbf8` | `FM2_IntrusiveList_InitSentinel` | Init self-linked intrusive list node pair. |
| `0x8245ea60` | `FM2_Crt_StaticInit_ForzaCmdLineList_829F194C` | CRT static init `unk_829F194C` + atexit (cmdline/startup list). |
| `0x82460aa0` | `FM2_Crt_StaticInit_AsyncQueueGlobal_829F1A48` | CRT static init `unk_829F1A48` + atexit (async queue global). |
| `0x8276b730` | `FM2_Crt_StorePtrPair` | Store `{ptr,count}` pair into static global (2 dwords). |
| `0x8277d9e0` | `FM2_STL_Map_KeyCompareThunk` | STL map insert key-compare thunk -> `sub_827F6180`. |
| `0x826bff38` | `FM2_XAudio2_VoicePool_ReleaseVoiceLocked` | Locked XAudio2 voice pool release: COM release + heap free. |
| `0x826c2e20` | `FM2_XAudio2_Voice_UnlinkFromPool` | Unlink XAudio2 voice from pool intrusive list under critsec. |
| `0x826c1df0` | `FM2_XAudio2_Voice_ReleaseResourcesLocked` | Locked XAudio2 voice teardown: unlink, release ref, free buffers. |