### Infrastructure pass 6 (33 functions)

Refreshed callee list (1173 remaining). Lua intern/load path, STL append helpers, XAudio2 stream pool, SQLite parse teardown.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824bf480` | `FM2_Lua_InternString` | Lua string intern table: hash lookup or create `TString` via GC alloc. |
| `0x824c0298` | `FM2_Lua_ErrorBlockTooBig` | Raise Lua error `memory allocation error: block too big`. |
| `0x824b67e8` | `FM2_Lua_RemoveStackSlotAtIndex` | Remove stack slot at index by shifting slots down 16 bytes. |
| `0x824b7190` | `FM2_Lua_ErrorVprintfCore` | Vararg core for Lua error printf (`FM2_Lua_ErrorVprintf` path). |
| `0x824bc370` | `FM2_Lua_LoadStringWithFormatSpecifiers` | Load/eval Lua chunk string; handles `>` prefix and `f`/`L` format flags. |
| `0x824c02c8` | `FM2_Lua_AllocGcObjectFromState` | Allocate GC object from Lua global state allocator vtable. |
| `0x824bc110` | `FM2_Lua_ParseLoadStringFormatSpec` | Parse load-string format spec (`S`/`L`/etc.) into chunk metadata. |
| `0x824b7b20` | `FM2_LuaIO_OpenFileWithMode` | Lua IO open: build mode string then delegate to file open helper. |
| `0x824b6d68` | `FM2_Lua_IsStackSlotTruthy` | Returns whether stack slot is truthy (non-nil/non-false). |
| `0x824bd068` | `FM2_Lua_ErrorAppendStackArgs` | Append formatted stack args to Lua error message buffer. |
| `0x824bb668` | `FM2_Lua_PushLoadedClosureUpvalues` | Push closure upvalues after load (`L` format / loaded function). |
| `0x824bbfe0` | `FM2_Lua_ErrorThrowWithLongJmpRestore` | Restore longjmp frame then set Lua error status and unwind. |
| `0x824ebe68` | `FM2_Lua_BindingPairVector_CopyAssign` | Copy-assign Lua binding `{key,func}` pair vector (8-byte pairs). |
| `0x824ebd48` | `FM2_Lua_BindingPairVector_SortByPathComponent` | Quicksort binding pair vector by path component compare. |
| `0x821d2968` | `FM2_Stl_String_AssignAppendCStr` | STL string assign = copy base + append C string bytes. |
| `0x821ec4a0` | `FM2_Stl_String_AssignAppendSubrange` | STL string assign = copy base + append subrange from source. |
| `0x821d1720` | `FM2_Stl_String_AppendRange` | Append byte range to SSO/heap string (handles self-append overlap). |
| `0x821d1620` | `FM2_Stl_String_AppendBytesFromSource` | Append bytes from source string subrange into destination string. |
| `0x821d1500` | `FM2_Stl_String_DtorFromEhUnwind` | EH unwind helper: clear STL string via `InitOrClear`. |
| `0x8221be68` | `FM2_WString_ResizeOrReleaseHeapStorage` | Wide-string resize: memcpy SSO or free heap buffer when shrinking. |
| `0x82346390` | `FM2_Lua_BindingPairVector_ReserveCapacity` | Reserve capacity for 8-byte pair vector; throw on overflow. |
| `0x825d0ab8` | `FM2_CarParts_GetGlobalUpgradeRegistryPtr` | Returns global car-parts/upgrade registry singleton `dword_82A028D8`. |
| `0x822518f8` | `FM2_CarParts_AdvanceUpgradeListIterator` | Advance intrusive-list iterator over upgrade-path nodes. |
| `0x826c1ac0` | `FM2_XAudio2_HeapFreeVoiceBufferByTag` | Free XAudio2 voice buffer via tagged pool or process heap. |
| `0x826ce780` | `FM2_XAudio2_Stream_SignalSubmitEventIfZero` | Decrement stream submit refcount; signal event when zero. |
| `0x826c9bc0` | `FM2_XAudio2_Stream_DecRefAndFinalizePacket` | Decrement packet ref; unlink/requeue or finalize on last ref. |
| `0x826c4db0` | `FM2_XAudio2_CLeapBuffer_AllocateSlot` | Allocate/reuse CLeap buffer slot in voice stream pool. |
| `0x826bc110` | `FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback` | Decrement CLeap buffer ref; invoke completion callback at zero. |
| `0x826b2d00` | `FM2_XAudio2_Stream_AcquireVoiceRef` | Acquire voice reference from stream under interlocked refcount. |
| `0x826b1510` | `FM2_XAudio2_Stream_LookupVoiceByHandle` | Lookup XAudio2 voice object by handle in stream hash table. |
| `0x826b10c0` | `FM2_XAudio2_Stream_CloseWaitHandleIfIdle` | Decrement stream wait refcount; close wait handle when idle. |
| `0x82707f20` | `FM2_SQLite_ParseContext_DestroyRecursive` | Recursively destroy SQLite parse context tree and owned stacks. |
| `0x826ef068` | `FM2_SQLite_Database_ReleaseOpenStatements` | Release open SQLite statements/callbacks before database free. |