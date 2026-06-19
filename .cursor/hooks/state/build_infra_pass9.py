import json

RENAMES = [
    ("0x8221c5d8", "FM2_PropertyBag_InitListSentinel"),
    ("0x82220030", "FM2_PropertyBagList_DestroyAndFree"),
    ("0x82231648", "FM2_WString_AssignSubrangeFromSource"),
    ("0x82331e78", "FM2_HashName_FindPropertyNodeByKey"),
    ("0x8245a580", "FM2_HashName_AssignPropertyByTypeId"),
    ("0x8245b108", "FM2_PropertyBag_InitRbTreeHeader"),
    ("0x8245b2c0", "FM2_PropertyBag_CtorFromHashNameNode"),
    ("0x8253faf8", "FM2_HashNamePropertyList_DestroyAndFree"),
    ("0x82204a28", "FM2_Stl_String_FindDelimiterIndex"),
    ("0x8245c4d8", "FM2_HashName_NormalizeKeyString"),
    ("0x8245f1f0", "FM2_HashName_LookupValueByKeyInTable"),
    ("0x82363bf8", "FM2_AudioDevice_SetDeferredFreeFlag"),
    ("0x8236dbc8", "FM2_Render_SetPassShaderFlagsFromArray"),
    ("0x82578970", "FM2_AudioManager_InitDefaultMixParameters"),
    ("0x82555d38", "FM2_Render_SetupPassMaterialConstants"),
    ("0x82514f90", "FM2_Render_ObjectPassSortAndEmitDraws"),
    ("0x82725560", "FM2_Render_ApplyPassLightingCore"),
    ("0x826188c8", "FM2_Memory_InsertFrameAllocMapEntry"),
    ("0x82366af8", "FM2_Memory_ScheduleDeferredFreeForBlock"),
    ("0x82522f18", "FM2_AllocPoolAcquire12xCount"),
    ("0x82657338", "FM2_CircularBuffer_EraseRange"),
    ("0x826719d8", "FM2_FMOD_HeapAllocFromPoolLocked"),
    ("0x82429d08", "FM2_SQLite_Vdbe_GetProgramCounter"),
    ("0x824f4138", "FM2_SQLite_Vfs_AddRef"),
    ("0x82418560", "FM2_Image_DecodePngFromMemory"),
    ("0x8258ed20", "FM2_InstalledParts_CtorDefaults"),
    ("0x825a10d8", "FM2_LiveryMask_CreateInterfaceFromPath"),
    ("0x825a00b0", "FM2_LiveryMask_InitCreateParams"),
    ("0x825a0ea8", "FM2_LiveryMask_AllocAndInitMaskObject"),
    ("0x824a7278", "FM2_ComObject_GetRefCountVtablePtr"),
    ("0x824a7688", "FM2_ComObject_GetStaticLifetimeBlock"),
    ("0x824cc0f8", "FM2_CameraScript_DecRefAndUnloadIfLast"),
    ("0x824fb108", "FM2_DeferredTaskHolder_Dtor"),
]

REASONS = {
    "0x8221c5d8": "Init property-bag intrusive-list sentinel node.",
    "0x82220030": "Destroy property-bag list nodes and free backing block.",
    "0x82231648": "Wide-string assign/copy subrange from source SSO/heap buffer.",
    "0x82331e78": "Find hash-name RB-tree node matching 32-byte property key.",
    "0x8245a580": "Assign hash-name property value by type id (string/wstring/list/etc.).",
    "0x8245b108": "Init property-bag red-black tree header links.",
    "0x8245b2c0": "Construct `CPropertyBag` from hash-name lookup node.",
    "0x8253faf8": "Destroy hash-name property list and free sentinel block.",
    "0x82204a28": "Find index of delimiter char in STL string (for `a.b` paths).",
    "0x8245c4d8": "Normalize hash-name key string (case/char transform loop).",
    "0x8245f1f0": "Lookup hash-name table value dword by key in RB-tree.",
    "0x82363bf8": "Set audio-device deferred-free flag; enqueue free when enabled.",
    "0x8236dbc8": "OR shader/pass flag bits from bool array into render pass state.",
    "0x82578970": "Initialize default audio manager mix/fade timing parameters.",
    "0x82555d38": "Setup render pass material constants (VMX vector splats).",
    "0x82514f90": "Sort/object-pass draw emit helper for render setup inner path.",
    "0x82725560": "Core pass-lighting transform application (matrix/vector math).",
    "0x826188c8": "Insert frame alloc map entry keyed by frame counter.",
    "0x82366af8": "Schedule memory block for deferred free under global critsec.",
    "0x82522f18": "Pool alloc `12 * count` bytes with overflow guard.",
    "0x82657338": "Erase subrange from circular/intrusive buffer vector.",
    "0x826719d8": "FMOD heap alloc from pool under critsec (optional header skip).",
    "0x82429d08": "Returns SQLite Vdbe program counter at `this+16`.",
    "0x824f4138": "SQLite VFS addref via vtable +8.",
    "0x82418560": "Decode PNG image bytes from memory buffer (zlib/ihdr path).",
    "0x8258ed20": "Construct `CInstalledParts` with -1 filled defaults.",
    "0x825a10d8": "Create livery-mask COM interface from file path params.",
    "0x825a00b0": "Init livery-mask creation parameter struct defaults.",
    "0x825a0ea8": "Allocate 240-byte livery-mask object and init from params.",
    "0x824a7278": "Returns COM ref-count vtable pointer `off_8299B824`.",
    "0x824a7688": "Returns static COM lifetime block `unk_829F2EC8`.",
    "0x824cc0f8": "Decref camera script module; unload when last ref.",
    "0x824fb108": "Deferred-task holder dtor: invoke callback then reset vtable.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass9.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 9 (33 functions)\n",
    "Callee list **1148** remaining. Hash-name/property-bag cluster, render pass core, livery-mask, FMOD/SQLite helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass9.md", "w", encoding="utf-8").write("\n".join(md))
