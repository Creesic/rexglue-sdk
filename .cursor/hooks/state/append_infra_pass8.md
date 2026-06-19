### Infrastructure pass 8 (33 functions)

Callee list **1144** remaining. Hash-name cluster, audio signal gates, race ghost, render pass, Lua strict number.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82365048` | `FM2_Memory_UpdateFrameAllocCategoryRecord` | Update per-frame memory alloc record for category under global critsec. |
| `0x82367300` | `FM2_AlignUpToPowerOfTwo` | Align integer up to power-of-two boundary. |
| `0x82364460` | `FM2_AudioDevice_BindSignalGateForCategory` | Bind audio signal gate for device category index (boot/audio init). |
| `0x825ad608` | `FM2_AudioManager_GetCategoryDescriptorPtr` | Index audio manager category descriptor table by category id. |
| `0x824604d8` | `FM2_AudioSignalGate_ReadTimestamp` | Read audio signal-gate timestamp from vfunc at +80. |
| `0x82460510` | `FM2_AudioSignalGate_UpdateElapsedTimestamp` | Update elapsed audio signal-gate time; accumulate when active. |
| `0x824a0ef8` | `FM2_AudioManager_GetGlobalConfigPtr` | Returns global audio manager config pointer `dword_829F2DF8`. |
| `0x821d2770` | `FM2_Stl_ConstructLengthErrorFromString` | Construct `std::length_error` from message string. |
| `0x82264438` | `FM2_RefCountedBlock_IsSingleReference` | Returns whether ref-count field at +12 equals 1. |
| `0x8228d628` | `FM2_RaceGhost_ClearReplaySampleState` | Zero race-ghost replay sample float buffers and counters. |
| `0x8228d898` | `FM2_RaceGhost_InitReplaySampleBlock` | Init race-ghost replay block: copy cleared sample state + reset fields. |
| `0x825c6778` | `FM2_RaceEntry_AllocStateBlockArray` | Allocate array of 48-byte race-entry state blocks. |
| `0x825c57c0` | `FM2_CareerRace_LookupRewardIdFromXml` | Parse career-race XML `Id` field into reward hash-name lookup. |
| `0x82296460` | `FM2_Lua_TuningDatabase_InitRecord` | Construct/init large Lua tuning-database singleton record. |
| `0x8254e3c8` | `FM2_Lua_ToNumberStrict` | Lua `ToNumber` strict: reject ambiguous zero non-number. |
| `0x824b6aa8` | `FM2_Lua_IsNumberOrCoercibleToNumber` | True if stack slot is number or coercible to number. |
| `0x824be8f8` | `FM2_Lua_AllocCClosure` | Allocate Lua C closure object (type 5) with upvalues. |
| `0x824b91f0` | `FM2_Lua_SetErrorStatusAndUnwind` | Set Lua error status byte and trigger unwind/longjmp path. |
| `0x822a1c78` | `FM2_CritSec_InitAndZeroOwner` | Init critsec with spin count; zero owner dword. |
| `0x8255d880` | `FM2_Render_ObjectPassDrawSetupInner` | Core render object-pass draw setup (matrices, CB, sort keys). |
| `0x82516ad8` | `FM2_Render_ForwardPassLightingToCore` | Forward pass-lighting args to core renderer helper. |
| `0x8245a4f0` | `FM2_HashName_InitEmptyWithSentinel` | Init empty hash-name object with sentinel qword and vtable. |
| `0x8245b308` | `FM2_HashName_AssignFromPropertyNode` | Assign hash-name result from property node when id > 0x10. |
| `0x8245f120` | `FM2_HashName_LookupPropertyNodeByKey32` | Lookup hash-name property RB-tree node by 32-byte key. |
| `0x8245fd08` | `FM2_HashName_LookupAltModulePropertyImpl` | Recursive alt-module property lookup (`a.b.c` path parsing). |
| `0x8257cba8` | `FM2_HashName_ClearPropertyTable` | Clear 64-byte hash-name property table; reset count. |
| `0x8257cd78` | `FM2_HashName_CtorEmpty` | Construct empty `CHashName` object. |
| `0x8257e9f8` | `FM2_HashName_GetPropertyTablePtr` | Returns pointer to hash-name property table at +8. |
| `0x8258b598` | `FM2_NetworkMessage_InitStateBlockSentinel` | Init network-message state-block intrusive-list sentinel. |
| `0x824ccb30` | `FM2_XmlSchema_AppendCStringValue` | Append C string field to XML schema object via vfunc +32. |
| `0x824fe9f0` | `FM2_FontRegistry_AdvanceModuleListIterator` | Advance font module registry intrusive-list iterator. |
| `0x826eea50` | `FM2_SQLite_ReallocOrAllocZeroed` | SQLite realloc wrapper; falls back to zeroed alloc. |
| `0x826c3570` | `FM2_XAudio2_StreamPool_UnlinkAndNotify` | XAudio2 stream pool unlink/notify on buffer state transition. |