import json

RENAMES = [
    ("0x82509468", "FM2_Render_SpliceAndReleaseNotifyList"),
    ("0x8250cd60", "FM2_DeferredCommand_ClearStringField"),
    ("0x82505ec8", "FM2_Render_ComputeInstancePathBlendMatrix"),
    ("0x82505d70", "FM2_AudioVoice_GrowChannelArrayCapacity"),
    ("0x82512358", "FM2_PresentationCarConfig_DeleteOptional"),
    ("0x825127a0", "FM2_PresentationSlotVector_Clear200Byte"),
    ("0x8254e318", "FM2_Lua_AssertionFailedTypeError"),
    ("0x8254e450", "FM2_Lua_MathTwoArgDispatch"),
    ("0x8254d220", "FM2_Lua_PushMetatableWithGcAndProps"),
    ("0x82551ab0", "FM2_FindAndReplaceDelimitedTextRange"),
    ("0x82555fc8", "FM2_Animation_ClampKeyframeWeightVMX"),
    ("0x8255c210", "FM2_Render_TestObjectPassDrawVisibility"),
    ("0x8255b1c8", "FM2_Render_SubmitSortedDrawListsTail"),
    ("0x8255fb58", "FM2_Render_ObjectPassDrawTraversalInner"),
    ("0x82567060", "FM2_TModel_InitVMXBounds"),
    ("0x82568590", "FM2_Render_HelperB3E8PathA"),
    ("0x82572ca8", "FM2_WString_GrowHeapCapacity"),
    ("0x82572dc8", "FM2_AudioManager_InitSignalGateField"),
    ("0x82579bb8", "FM2_Render_SubmitObjectDrawConstantsTail"),
    ("0x8257ccf8", "FM2_HashName_CtorEmpty"),
    ("0x8257cdc0", "FM2_PropertyBag_AllocRbTreeNode"),
    ("0x82582188", "FM2_XmlElement_Dtor"),
    ("0x82586f40", "FM2_FMOD_Build3DAttributesPairA"),
    ("0x82587738", "FM2_UI_GetMaxPropertyAbsValueHalfStep"),
    ("0x8258a480", "FM2_FileInfoCache_AllocateEntry"),
    ("0x8258b0f8", "FM2_RenderAdapter_DestroyChildAndClearList"),
    ("0x8258b3c0", "FM2_IntrusiveListNode_InitWithOffset"),
    ("0x8258b4d0", "FM2_RbTree_InitIteratorWithHint"),
    ("0x8258c208", "FM2_Presentation_InitMediaFoundationField"),
    ("0x8258ebe0", "FM2_CarDb_QueryStockPartByOrdinal"),
    ("0x825a11f8", "FM2_Render_ViewTraversalNotifyHook"),
    ("0x825a31e0", "FM2_AudioRenderFrame_LogSaveFrontBufferBody"),
    ("0x825a3ca8", "FM2_Boot_ParseCommandLineTokenBuffer"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass53.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 53 (33 functions)\n",
    "Render/audio/Lua/FMOD/RB-tree helpers: presentation slots, Lua assert/math, file cache, boot parse.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass53.md", "w", encoding="utf-8").write("\n".join(md))
