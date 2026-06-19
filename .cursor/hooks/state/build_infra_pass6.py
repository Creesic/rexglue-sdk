import json

RENAMES = [
    ("0x824bf480", "FM2_Lua_InternString"),
    ("0x824c0298", "FM2_Lua_ErrorBlockTooBig"),
    ("0x824b67e8", "FM2_Lua_RemoveStackSlotAtIndex"),
    ("0x824b7190", "FM2_Lua_ErrorVprintfCore"),
    ("0x824bc370", "FM2_Lua_LoadStringWithFormatSpecifiers"),
    ("0x824c02c8", "FM2_Lua_AllocGcObjectFromState"),
    ("0x824bc110", "FM2_Lua_ParseLoadStringFormatSpec"),
    ("0x824b7b20", "FM2_LuaIO_OpenFileWithMode"),
    ("0x824b6d68", "FM2_Lua_IsStackSlotTruthy"),
    ("0x824bd068", "FM2_Lua_ErrorAppendStackArgs"),
    ("0x824bb668", "FM2_Lua_PushLoadedClosureUpvalues"),
    ("0x824bbfe0", "FM2_Lua_ErrorThrowWithLongJmpRestore"),
    ("0x824ebe68", "FM2_Lua_BindingPairVector_CopyAssign"),
    ("0x824ebd48", "FM2_Lua_BindingPairVector_SortByPathComponent"),
    ("0x821d2968", "FM2_Stl_String_AssignAppendCStr"),
    ("0x821ec4a0", "FM2_Stl_String_AssignAppendSubrange"),
    ("0x821d1720", "FM2_Stl_String_AppendRange"),
    ("0x821d1620", "FM2_Stl_String_AppendBytesFromSource"),
    ("0x821d1500", "FM2_Stl_String_DtorFromEhUnwind"),
    ("0x8221be68", "FM2_WString_ResizeOrReleaseHeapStorage"),
    ("0x82346390", "FM2_Lua_BindingPairVector_ReserveCapacity"),
    ("0x825d0ab8", "FM2_CarParts_GetGlobalUpgradeRegistryPtr"),
    ("0x822518f8", "FM2_CarParts_AdvanceUpgradeListIterator"),
    ("0x826c1ac0", "FM2_XAudio2_HeapFreeVoiceBufferByTag"),
    ("0x826ce780", "FM2_XAudio2_Stream_SignalSubmitEventIfZero"),
    ("0x826c9bc0", "FM2_XAudio2_Stream_DecRefAndFinalizePacket"),
    ("0x826c4db0", "FM2_XAudio2_CLeapBuffer_AllocateSlot"),
    ("0x826bc110", "FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback"),
    ("0x826b2d00", "FM2_XAudio2_Stream_AcquireVoiceRef"),
    ("0x826b1510", "FM2_XAudio2_Stream_LookupVoiceByHandle"),
    ("0x826b10c0", "FM2_XAudio2_Stream_CloseWaitHandleIfIdle"),
    ("0x82707f20", "FM2_SQLite_ParseContext_DestroyRecursive"),
    ("0x826ef068", "FM2_SQLite_Database_ReleaseOpenStatements"),
]

REASONS = {
    "0x824bf480": "Lua string intern table: hash lookup or create `TString` via GC alloc.",
    "0x824c0298": "Raise Lua error `memory allocation error: block too big`.",
    "0x824b67e8": "Remove stack slot at index by shifting slots down 16 bytes.",
    "0x824b7190": "Vararg core for Lua error printf (`FM2_Lua_ErrorVprintf` path).",
    "0x824bc370": "Load/eval Lua chunk string; handles `>` prefix and `f`/`L` format flags.",
    "0x824c02c8": "Allocate GC object from Lua global state allocator vtable.",
    "0x824bc110": "Parse load-string format spec (`S`/`L`/etc.) into chunk metadata.",
    "0x824b7b20": "Lua IO open: build mode string then delegate to file open helper.",
    "0x824b6d68": "Returns whether stack slot is truthy (non-nil/non-false).",
    "0x824bd068": "Append formatted stack args to Lua error message buffer.",
    "0x824bb668": "Push closure upvalues after load (`L` format / loaded function).",
    "0x824bbfe0": "Restore longjmp frame then set Lua error status and unwind.",
    "0x824ebe68": "Copy-assign Lua binding `{key,func}` pair vector (8-byte pairs).",
    "0x824ebd48": "Quicksort binding pair vector by path component compare.",
    "0x821d2968": "STL string assign = copy base + append C string bytes.",
    "0x821ec4a0": "STL string assign = copy base + append subrange from source.",
    "0x821d1720": "Append byte range to SSO/heap string (handles self-append overlap).",
    "0x821d1620": "Append bytes from source string subrange into destination string.",
    "0x821d1500": "EH unwind helper: clear STL string via `InitOrClear`.",
    "0x8221be68": "Wide-string resize: memcpy SSO or free heap buffer when shrinking.",
    "0x82346390": "Reserve capacity for 8-byte pair vector; throw on overflow.",
    "0x825d0ab8": "Returns global car-parts/upgrade registry singleton `dword_82A028D8`.",
    "0x822518f8": "Advance intrusive-list iterator over upgrade-path nodes.",
    "0x826c1ac0": "Free XAudio2 voice buffer via tagged pool or process heap.",
    "0x826ce780": "Decrement stream submit refcount; signal event when zero.",
    "0x826c9bc0": "Decrement packet ref; unlink/requeue or finalize on last ref.",
    "0x826c4db0": "Allocate/reuse CLeap buffer slot in voice stream pool.",
    "0x826bc110": "Decrement CLeap buffer ref; invoke completion callback at zero.",
    "0x826b2d00": "Acquire voice reference from stream under interlocked refcount.",
    "0x826b1510": "Lookup XAudio2 voice object by handle in stream hash table.",
    "0x826b10c0": "Decrement stream wait refcount; close wait handle when idle.",
    "0x82707f20": "Recursively destroy SQLite parse context tree and owned stacks.",
    "0x826ef068": "Release open SQLite statements/callbacks before database free.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass6.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 6 (33 functions)\n",
    "Refreshed callee list (1173 remaining). Lua intern/load path, STL append helpers, XAudio2 stream pool, SQLite parse teardown.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass6.md", "w", encoding="utf-8").write("\n".join(md))
