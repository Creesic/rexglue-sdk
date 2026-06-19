import json

RENAMES = [
    ("0x824b6f30", "FM2_Lua_GetUserdataPointer"),
    ("0x824b7090", "FM2_Lua_PushInternedString"),
    ("0x824b70f8", "FM2_Lua_PushLStringOrNil"),
    ("0x824b7148", "FM2_Lua_PushFormattedStringGrowStack"),
    ("0x824b74f0", "FM2_Lua_PushCFunction"),
    ("0x824b76b8", "FM2_Lua_SetFieldFromCString"),
    ("0x824b7860", "FM2_Lua_SetClosureEnvFromStack"),
    ("0x824b7e10", "FM2_Lua_PushUserdataForKey"),
    ("0x824b7cc8", "FM2_Lua_ErrorThrow"),
    ("0x824b7d50", "FM2_Lua_ErrorAppendMessagePart"),
    ("0x824b8138", "FM2_Lua_NumberValuesEqual"),
    ("0x824b8318", "FM2_Lua_PushFormattedString"),
    ("0x824bb158", "FM2_Lua_GrowStack"),
    ("0x824bb2c8", "FM2_Lua_UpdateObjectGcMark"),
    ("0x824bb300", "FM2_Lua_LinkGrayObject"),
    ("0x824bb428", "FM2_Lua_GetDebugCallInfoLevel"),
    ("0x824bc778", "FM2_Lua_CoerceToNumberSlot"),
    ("0x824bc960", "FM2_Lua_GetTableField"),
    ("0x824bc0a8", "FM2_Lua_ErrorFormatAndThrow"),
    ("0x824bc7e8", "FM2_Lua_CoerceNumberToString"),
    ("0x824bf650", "FM2_Lua_AllocUserdata"),
    ("0x8254d110", "FM2_Lua_ErrorPrefixWithSourceLocation"),
    ("0x8254d198", "FM2_Lua_ErrorVprintf"),
    ("0x824ec320", "FM2_Lua_AppendBindingEntryAt32"),
    ("0x8245a740", "FM2_HashName_AssignVtable_8203CEA4"),
    ("0x8245dbf8", "FM2_IntrusiveList_InitSentinel"),
    ("0x8245ea60", "FM2_Crt_StaticInit_ForzaCmdLineList_829F194C"),
    ("0x82460aa0", "FM2_Crt_StaticInit_AsyncQueueGlobal_829F1A48"),
    ("0x8276b730", "FM2_Crt_StorePtrPair"),
    ("0x8277d9e0", "FM2_STL_Map_KeyCompareThunk"),
    ("0x826bff38", "FM2_XAudio2_VoicePool_ReleaseVoiceLocked"),
    ("0x826c2e20", "FM2_XAudio2_Voice_UnlinkFromPool"),
    ("0x826c1df0", "FM2_XAudio2_Voice_ReleaseResourcesLocked"),
]

REASONS = {
    "0x824b6f30": "Returns userdata ptr (type 2) or userdata+24 (type 7).",
    "0x824b7090": "Interns bytes via `sub_824BF480`, pushes string (type 4).",
    "0x824b70f8": "Push length-delimited string or nil if `a2` null.",
    "0x824b7148": "Grow stack if needed, delegate to formatted string push.",
    "0x824b74f0": "Push C function closure (type 5) via `sub_824BE8F8`.",
    "0x824b76b8": "Set table field from C string (`sub_824BCA78`).",
    "0x824b7860": "Assign closure/userdata env from value below top.",
    "0x824b7e10": "Alloc userdata for registry key lookup; push (type 7).",
    "0x824b7cc8": "Unwind/throw after Lua error (`sub_824BBFE0`).",
    "0x824b7d50": "Append formatted part to error message on stack.",
    "0x824b8138": "Equality test for two Lua numbers (type 3).",
    "0x824b8318": "Mini printf (`%s/%d/%f/...`) pushing Lua strings/numbers.",
    "0x824bb158": "Grow Lua stack when near limit (`sub_824BAFA0`).",
    "0x824bb2c8": "Update object GC mark bits from global state.",
    "0x824bb300": "Link object into gray list during GC.",
    "0x824bb428": "Resolve call-info level for debug/error prefix.",
    "0x824bc778": "Coerce stack slot to number (incl. string parse).",
    "0x824bc960": "Table field lookup with `__index` metamethod loop.",
    "0x824bc0a8": "Format error string then throw (`sub_824BBED0`).",
    "0x824bc7e8": "Coerce numeric stack slot to interned string.",
    "0x824bf650": "Allocate full userdata with metatable ref.",
    "0x8254d110": "Prefix Lua error with `source:line:` when available.",
    "0x8254d198": "Vararg Lua error formatter; used by bad-arg/type helpers.",
    "0x824ec320": "`FM2_Lua_Register*` thunk: append pair at `a1+32`.",
    "0x8245a740": "Hash-name object dtor: `*a1 = off_8203CEA4`.",
    "0x8245dbf8": "Init self-linked intrusive list node pair.",
    "0x8245ea60": "CRT static init `unk_829F194C` + atexit (cmdline/startup list).",
    "0x82460aa0": "CRT static init `unk_829F1A48` + atexit (async queue global).",
    "0x8276b730": "Store `{ptr,count}` pair into static global (2 dwords).",
    "0x8277d9e0": "STL map insert key-compare thunk -> `sub_827F6180`.",
    "0x826bff38": "Locked XAudio2 voice pool release: COM release + heap free.",
    "0x826c2e20": "Unlink XAudio2 voice from pool intrusive list under critsec.",
    "0x826c1df0": "Locked XAudio2 voice teardown: unlink, release ref, free buffers.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass3.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 3 (33 functions)\n", "Lua error/stack path, binding thunk +32, hash/list/CRT/XAudio2 helpers.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass3.md", "w", encoding="utf-8").write("\n".join(md))
