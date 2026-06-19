### Infrastructure pass 2 (30 functions)

Lua stack core (`sub_824B*`), binding thunks, SQLite alloc, CRT static inits.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824b6518` | `FM2_Lua_GetStackSlotPointer` | Core Lua stack: resolve absolute/relative/registry index to TValue pointer. |
| `0x824b6770` | `FM2_Lua_GetStackDepth` | Returns `(top - base) >> 4` stack depth. |
| `0x824b6788` | `FM2_Lua_SetStackTop` | Sets Lua stack top; grows stack with nil slots when needed. |
| `0x824b68a0` | `FM2_Lua_PopStackSlot` | Copies stack slot then decrements top (pop). |
| `0x824b69f8` | `FM2_Lua_GetStackValueType` | Returns Lua type tag for stack index, or -1 if absent. |
| `0x824b6a38` | `FM2_Lua_GetTypeNameForTag` | Maps type tag to name string (`no value`, `nil`, etc.). |
| `0x824b6b80` | `FM2_Lua_StackSlotsEqual` | Compares two stack slots for equality. |
| `0x824b6cc8` | `FM2_Lua_ToNumberOrZero` | Reads stack slot as double (type 3) or 0. |
| `0x824b6db8` | `FM2_Lua_ToLString` | Coerces stack slot to Lua string; returns `TString*` data. |
| `0x824b7020` | `FM2_Lua_PushNil` | Pushes nil onto Lua stack. |
| `0x824b7040` | `FM2_Lua_PushNumber` | Pushes double onto Lua stack (type tag 3). |
| `0x824b72e8` | `FM2_Lua_PushBool` | Pushes boolean onto Lua stack. |
| `0x824b73b0` | `FM2_Lua_PushCString` | Pushes C string onto Lua stack. |
| `0x824b7558` | `FM2_Lua_PushCFunctionFromStack` | Pushes C function/clojure from stack slot (types 5/7). |
| `0x822a02b0` | `FM2_Lua_GetArgPointerOrLogMissing` | Bounds-checks Lua args vector; logs missing arg via execution error. |
| `0x822a0028` | `FM2_Lua_PushBoolFromStackArg` | Pushes bool from stack arg through thread context. |
| `0x82297fb0` | `FM2_Lua_PushDisplayStringById` | Pushes `UserInterface::DisplayString` by id then copies stack arg. |
| `0x822b1c00` | `FM2_Lua_PushIntAndStackArg` | Pushes int as number then copies stack arg (return helper). |
| `0x8229ff60` | `FM2_Lua_RaiseExecutionError` | Formats `Error executing '<fn>': ...` and raises Lua error. |
| `0x8254e148` | `FM2_Lua_RaiseTypeMismatchError` | Raises `%s expected, got %s` type mismatch via bad-arg helper. |
| `0x824ec3c8` | `FM2_Lua_AppendBindingEntryAt80` | `FM2_Lua_Register*` thunk: append pair at `a1+80`. |
| `0x824ec618` | `FM2_Lua_AppendBindingEntryAt44` | `FM2_Lua_Register*` thunk: append pair at `a1+44`. |
| `0x8220caf0` | `FM2_Lua_BindingVector_PopBackIndex` | Pops binding-vector back index when clearing thread context. |
| `0x8245a4d8` | `FM2_Object_AssignBaseVtable_82000E18` | Dtor prologue: `*result = off_82000E18`. |
| `0x826ed658` | `FM2_SQLite_FreeIfNonNull` | If non-null ptr, calls SQLite free dispatch `dword_82A3CEB8`. |
| `0x826ee578` | `FM2_SQLite_Alloc` | SQLite malloc wrapper via `dword_82A3CEB0`; sets OOM flag on failure. |
| `0x826ee5e8` | `FM2_SQLite_AllocZeroed` | `FM2_SQLite_Alloc` + `memset(0)`. |
| `0x82272650` | `FM2_Crt_StaticInit_GraphicsStreamList_829C45B0` | CRT static init for graphics stream list `unk_829C45B0` + atexit. |
| `0x82206d08` | `FM2_Crt_StaticInit_ScriptBindingManager_829C24B8` | Lazy singleton init for script binding manager `dword_829C24B8`. |
| `0x82803670` | `FM2_Crt_AssertCurrentThreadOrBugcheck` | If not current thread id `82998F54`, triggers bugcheck path. |