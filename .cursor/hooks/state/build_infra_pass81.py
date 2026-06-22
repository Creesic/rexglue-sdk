import json

RENAMES = [
    ("0x8253d970", "FM2_SpliceResultObjectsIntoListInitA"),
    ("0x8253db30", "FM2_IntrusiveList_InitSentinelBody"),
    ("0x8253efa0", "FM2_SpliceResultObjectsIntoListInitB"),
    ("0x825422b0", "FM2_LuaGarage_EnsureCarRecordLookupTail"),
    ("0x82544700", "FM2_SceneGraph_DestroySubtreeAndFreeBody"),
    ("0x82545fe0", "FM2_IntrusiveList_ResetToSelfBody"),
    ("0x82548ae8", "FM2_Set_InsertUniqueSortedBody"),
    ("0x8254a4e0", "FM2_LuaParser_GetTokenOrAdvanceLineBody"),
    ("0x82551508", "FM2_FindAndReplaceDelimitedTextRangeBody"),
    ("0x82551a00", "FM2_FindAndReplaceDelimitedTextRangeTail"),
    ("0x82556080", "FM2_CarDynamics_ComputeSuspensionDotsVMXBody"),
    ("0x8255bc00", "FM2_Render_ObjectPassDrawTraversalBody"),
    ("0x8255d430", "FM2_Render_ObjectPassShouldDrawVisibleCheck"),
    ("0x8255d4b8", "FM2_Render_ObjectPassShouldDrawVisibleBody"),
    ("0x82561208", "FM2_Render_DrawPassMaterialSetupBodyA"),
    ("0x82561f58", "FM2_D3D_ValidateResourceHandlesOrRecoverBody"),
    ("0x82563b68", "FM2_Render_DrawPassMaterialSetupBodyB"),
    ("0x825687c8", "FM2_Render_ObjectPassDrawSetupBody"),
    ("0x8256ac18", "FM2_Render_HelperB3E8DrawPathTail"),
    ("0x8257cbe8", "FM2_HashName_CtorEmptyBody"),
    ("0x8257cf90", "FM2_AIDriver_ResetRaceLineInterpBScalar"),
    ("0x82586de0", "FM2_FMOD_Build3DAttributesPairBodyA"),
    ("0x82587048", "FM2_FMOD_Build3DAttributesPairBodyB"),
    ("0x82587b88", "FM2_Network_DispatchMessageFromQueueLockedBody"),
    ("0x82589ae0", "FM2_FileInfoCache_AllocateEntryBody"),
    ("0x8258b060", "FM2_RenderAdapter_DestroyChildAndClearListBody"),
    ("0x8258c008", "FM2_Presentation_InitMediaFoundationFieldBody"),
    ("0x8258d0b8", "FM2_Set_LowerBoundByKeyInTreeBody"),
    ("0x825977a8", "FM2_ComObject_InitCarRecordFromDataQueryBody"),
    ("0x82598048", "FM2_RaceGhost_BuildPlaybackSampleTableCore"),
    ("0x8259dc20", "FM2_RaceGhost_BuildPlaybackSampleTableParse"),
    ("0x82503668", "FM2_Memory_LookupFrameAllocNotifyStateHelper"),
    ("0x824cd5a0", "FM2_STL_WStringInsertCharsRange_LenThunk"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass81.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 81 (33 functions)\n",
    "Scene graph/STL, render object-pass/draw setup, FMOD/network, race ghost playback table.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass81.md", "w", encoding="utf-8").write("\n".join(md))
