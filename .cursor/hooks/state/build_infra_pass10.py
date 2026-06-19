import json

RENAMES = [
    ("0x821e98d0", "FM2_Stl_StringIter_InitFromStringEnd"),
    ("0x8221c630", "FM2_WString_ReserveCapacity"),
    ("0x8221f150", "FM2_IntrusiveList_EraseNodeAndRebalance"),
    ("0x82419a74", "FM2_Crt_StackProbeAlloc"),
    ("0x8241de18", "FM2_Char_ToLowerAscii"),
    ("0x8242fa30", "FM2_RbTree_CompareKeyLess"),
    ("0x8245bc20", "FM2_Stl_StringIter_InitAtOffset"),
    ("0x82466aa8", "FM2_Profile_GetOptionalHeapBlockPtr"),
    ("0x824a0f08", "FM2_PresentationCarConfig_Dtor"),
    ("0x824de690", "FM2_Profile_FreeOptionalHeapBlock"),
    ("0x825a0430", "FM2_LiveryMask_AtexitFreeSingleton"),
    ("0x825eaa78", "FM2_DirectIface_ResetPixelShaderBinding"),
    ("0x82671728", "FM2_FMOD_Dsp_AdjustDelayLinePointers"),
    ("0x825c5158", "FM2_CareerRace_QueryGameOptionsByToken"),
    ("0x8245b080", "FM2_PropertyBag_RbTreeLowerBound"),
    ("0x8221bee0", "FM2_PropertyBag_AllocListNode"),
    ("0x8253f9f8", "FM2_HashNamePropertyList_EraseNode"),
    ("0x82331cc8", "FM2_HashName_RbTreeLowerBoundInit"),
    ("0x824e7fd0", "FM2_FrameAllocMap_AdvanceIterator"),
    ("0x82617f48", "FM2_FrameAllocMap_InsertOrAssign"),
    ("0x82656e78", "FM2_IntVector_EraseRangeShift"),
    ("0x825a04e0", "FM2_LiveryMask_Ctor"),
    ("0x825a05f0", "FM2_LiveryMask_CopyCreateParams"),
    ("0x82207398", "FM2_Stl_SnprintfToBuffer"),
    ("0x82460968", "FM2_TuningDb_InitIntListSentinel"),
    ("0x8221cb08", "FM2_TuningDb_InitFloatListSentinel"),
    ("0x821e67b8", "FM2_CarSetup_Ctor"),
    ("0x822930c0", "FM2_CarDynamics_InitSubsystems"),
    ("0x824cbfd0", "FM2_CameraScript_DestroyModule"),
    ("0x82204908", "FM2_Stl_StringIter_GetCursorPtr"),
    ("0x82204958", "FM2_Stl_StringIter_AdvanceChar"),
    ("0x824541a8", "FM2_NetworkMessage_RbTreeLowerBound"),
    ("0x8245b008", "FM2_PropertyBag_AllocRbTreeNode"),
]

REASONS = {
    "0x821e98d0": "Init string iterator at end of SSO/heap string buffer.",
    "0x8221c630": "Reserve/grow wide-string capacity to requested char count.",
    "0x8221f150": "Erase intrusive-list node and rebalance parent links.",
    "0x82419a74": "Stack probe / alloca chunk adjustment helper.",
    "0x8241de18": "Lowercase ASCII A-Z to a-z for hash-name normalization.",
    "0x8242fa30": "RB-tree key compare: `left.key < right.key`.",
    "0x8245bc20": "Init string iterator at byte offset in source string.",
    "0x82466aa8": "Returns optional profile heap block pointer at +32.",
    "0x824a0f08": "Presentation car-config dtor; reset base vtable.",
    "0x824de690": "Free optional profile heap block at +32.",
    "0x825a0430": "Livery-mask singleton atexit: free backing storage.",
    "0x825eaa78": "Direct3D iface: clear pixel-shader binding and release.",
    "0x82671728": "FMOD DSP: adjust delay-line buffer pointers by delta.",
    "0x825c5158": "SQL query GameOptions by token string for career race.",
    "0x8245b080": "Property-bag RB-tree lower_bound recursive walk.",
    "0x8221bee0": "Allocate property-bag intrusive-list node (121-byte).",
    "0x8253f9f8": "Erase node from hash-name property intrusive list.",
    "0x82331cc8": "Init hash-name RB-tree lower_bound iterator pair.",
    "0x824e7fd0": "Advance frame-alloc map ordered-set iterator.",
    "0x82617f48": "Insert/assign entry in frame-alloc ordered map.",
    "0x82656e78": "Erase int-vector subrange and memmove tail.",
    "0x825a04e0": "Construct livery-mask COM object with default params.",
    "0x825a05f0": "Copy livery-mask create-params struct fields.",
    "0x82207398": "Vararg snprintf into fixed stack buffer.",
    "0x82460968": "Init tuning-db intrusive int-list sentinel.",
    "0x8221cb08": "Init tuning-db intrusive float-list sentinel.",
    "0x821e67b8": "Construct car-setup object with installed-parts defaults.",
    "0x822930c0": "Init car-dynamics subsystem blocks and ramp samples.",
    "0x824cbfd0": "Destroy camera script module when last ref.",
    "0x82204908": "String iterator: get current cursor pointer.",
    "0x82204958": "String iterator: advance cursor by one char.",
    "0x824541a8": "Network-message RB-tree lower_bound recursive walk.",
    "0x8245b008": "Allocate property-bag RB-tree node skeleton.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass10.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 10 (33 functions)\n", "Hash-name/property-bag helpers, profile, livery-mask, car setup/dynamics.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass10.md", "w", encoding="utf-8").write("\n".join(md))
