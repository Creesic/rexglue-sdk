import json

RENAMES = [
    ("0x824ec650", "FM2_Lua_RegisterModuleBindings"),
    ("0x824ec400", "FM2_Lua_PushBindingRecordToTable"),
    ("0x824ec210", "FM2_LuaBindingRecord_CopyAssign"),
    ("0x824ec2a0", "FM2_LuaBindingRecord_InitFromCStr"),
    ("0x824ec788", "FM2_LuaBindingRecord_VectorShiftInsertCopies"),
    ("0x824ec7e0", "FM2_LuaBindingRecord_VectorConstructRange"),
    ("0x824ecbb0", "FM2_LuaBindingRecord_VectorReserveAndReset"),
    ("0x824ecc90", "FM2_LuaBindingRecord_VectorEmplaceBack"),
    ("0x824ecd50", "FM2_LuaBindingVector_EmplaceQualifiedName"),
    ("0x824ec890", "FM2_LuaBindingRecord_VectorInsert"),
    ("0x824ebf78", "FM2_Lua_BindingPairVector_Assign"),
    ("0x8254e4f0", "FM2_Lua_RegisterBindingPairsInModuleTable"),
    ("0x824ead78", "FM2_Lua_PushMetatableWithGcAndProps"),
    ("0x822a2ed8", "FM2_LuaBindingRecord_Dtor"),
    ("0x82466aa0", "FM2_LuaBindingRecord_GetPropertyFlags"),
    ("0x824b7750", "FM2_Lua_SetTableFieldFromStack"),
    ("0x824b69b0", "FM2_Lua_CopyStackSlotToTop"),
    ("0x824b7210", "FM2_Lua_PushLightUserdataWithArgs"),
    ("0x824b7a88", "FM2_Lua_ProtectedCallWithTraceback"),
    ("0x8254de38", "FM2_Lua_LoadFileFromCStringPath"),
    ("0x824ece68", "FM2_LuaIo_GetFileHandleReadable"),
    ("0x824ece98", "FM2_LuaIo_TryOpenFileForLoad"),
    ("0x824ecef0", "FM2_LuaIo_SetFileModeBits"),
    ("0x824ecf50", "FM2_LuaIo_IsFileWritable"),
    ("0x824ecf90", "FM2_LuaIo_GetFileSizeFlags"),
    ("0x823928d8", "FM2_Image_ResampleKernel_ApplyTexelTransform"),
    ("0x82393c38", "FM2_Image_ResampleKernel_AccumulateAndClearRow"),
    ("0x823d7460", "FM2_Image_ResampleKernel_ApplySqrtOrGammaTable"),
    ("0x82231f10", "FM2_WString_AssignFromWideCStr"),
    ("0x82257f18", "FM2_IntrusiveList_ClearAndDestroyNodes"),
    ("0x821f8250", "FM2_IntrusiveList_ClearAndFreeEntries"),
    ("0x821e6a08", "FM2_SceneNode_GetExtendedPayloadOffset"),
    ("0x821fc2b0", "FM2_ComObject_ReleaseAndOptionalFree"),
]

REASONS = {
    "0x824ec650": "Iterates 104-byte binding records; registers module table + `_LOADED`.",
    "0x824ec400": "Pushes one binding record (name, func, pair vectors) into Lua table.",
    "0x824ec210": "Copy-assign 104-byte Lua binding record incl. four pair vectors.",
    "0x824ec2a0": "Construct binding record from C string name + property id.",
    "0x824ec788": "Vector insert: backward-copy 104-byte records to make room.",
    "0x824ec7e0": "Construct N copies of binding record via `FM2_SceneNode_CopyAssignExtended`.",
    "0x824ecbb0": "Reallocate binding-record vector; reset begin/end iterators.",
    "0x824ecc90": "Emplace binding record at vector end (copy or grow).",
    "0x824ecd50": "Build `scope::name` qualified binding and emplace into vector.",
    "0x824ec890": "Full vector insert with reallocate/shift of 104-byte records.",
    "0x824ebf78": "Assign/copy Lua binding pair vector `{key,func}` pairs.",
    "0x8254e4f0": "Register `{name,func}` pairs into module `_LOADED` table.",
    "0x824ead78": "Push metatable with `__gc`, optional `__getprop`/`__setprop`.",
    "0x822a2ed8": "Dtor: free four pair vectors + SSO string in binding record.",
    "0x82466aa0": "Returns property-flag dword at binding-record offset +96.",
    "0x824b7750": "Set table field from two stack slots (metatable assignment).",
    "0x824b69b0": "Copy stack slot value to stack top.",
    "0x824b7210": "Push light userdata/C closure capturing N stack args.",
    "0x824b7a88": "Protected call with traceback hook (`sub_824B9780`).",
    "0x8254de38": "Load Lua chunk from C string path via reader callback.",
    "0x824ece68": "Lua IO: get readable file handle (op 2).",
    "0x824ece98": "Lua IO: try open file for load if not already loading.",
    "0x824ecef0": "Lua IO: set file mode bits (ops 6/7).",
    "0x824ecf50": "Lua IO: query writable flag (op 5).",
    "0x824ecf90": "Lua IO: combine size flag bits from ops 3/4.",
    "0x823928d8": "Bilinear resample texel transform by source/dest format modes.",
    "0x82393c38": "Resample row accumulate from temp buffer then zero scratch.",
    "0x823d7460": "Resample apply sqrt/gamma LUT (`flt_820303F8/FC`) on texels.",
    "0x82231f10": "Wide-string assign from `wchar_t*` via internal append.",
    "0x82257f18": "Clear intrusive list: dtor each node then free block.",
    "0x821f8250": "Clear intrusive list: free nodes without per-node dtor.",
    "0x821e6a08": "Returns scene-node extended payload at `this+10016`.",
    "0x821fc2b0": "Release COM object at +8; optional `FM2_Memory_FreeSmallBlockOrNull`.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass4.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 4 (33 functions)\n", "Lua binding-record module, image resample kernels, list/wstring helpers.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass4.md", "w", encoding="utf-8").write("\n".join(md))
