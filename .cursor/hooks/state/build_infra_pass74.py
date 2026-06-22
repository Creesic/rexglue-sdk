import json

RENAMES = [
    ("0x82232090", "FM2_XmlReader_CopyAttrTableEntryFields"),
    ("0x823900a8", "FM2_D3D_TextureDesc_ComputeFormatBlockSizeA"),
    ("0x82390a70", "FM2_D3D_TextureDesc_AllocFormatConversionBuffer"),
    ("0x82390d70", "FM2_D3D_TextureDesc_ReleaseFormatChain"),
    ("0x823aaf88", "FM2_Image_DecodeJpegFromMemory_OutputReadyCallback"),
    ("0x823cc398", "FM2_D3D_CreateTextureFromSurfaceLevelBodyB_CopyPixels"),
    ("0x821d3c20", "FM2_DeferredAudioManagerUpdate_DtorFields"),
    ("0x821e7ff8", "FM2_ComObject_CompareStringFieldPrefix"),
    ("0x821f0f20", "FM2_CareerRace_GetPlaybackFrameTimingPtr"),
    ("0x821f15a8", "FM2_CareerRace_UpdatePlaybackTimerInner"),
    ("0x821f23c8", "FM2_CareerRace_GetEndRaceTimerFieldPtr"),
    ("0x8221e520", "FM2_GraphicsStreamList_DeleteQueryDispatch"),
    ("0x8221f8f8", "FM2_GraphicsStreamList_DeleteQueryCallbackBody"),
    ("0x8223d990", "FM2_RaceGhost_AttachUpgradeNodeFinalize"),
    ("0x8224a6a0", "FM2_LiveryMask_GrowPendingEntryCheckBounds"),
    ("0x8224b530", "FM2_LiveryMask_GrowPendingEntryAppendSlot"),
    ("0x8224c100", "FM2_LiveryMask_GrowPendingEntryShiftTail"),
    ("0x82255eb0", "FM2_ComObject_InitRefCountAggregateBindField"),
    ("0x82262fb8", "FM2_ComObject_InitRefCountAggregateSetFlag"),
    ("0x822643b0", "FM2_ComObject_RefCountAggregateIncrementOne"),
    ("0x822699b0", "FM2_ComObject_InitRefCountAggregateFromCarRecord"),
    ("0x82270228", "FM2_ComObject_InitRefCountAggregateLinkNode"),
    ("0x82270a20", "FM2_ComObject_InitCarRecordFromDataQuery"),
    ("0x8229a6d8", "FM2_AudioSample_BuildOutputPairDescriptorValidate"),
    ("0x8229ae88", "FM2_AudioSample_BuildOutputPairDescriptorReleaseIter"),
    ("0x823357d0", "FM2_Audio_VolumeListInsertNodeRebalance"),
    ("0x8236c1e8", "FM2_D3D_GatherVolumeMetadataFromResourceDesc"),
    ("0x8237cac8", "FM2_AudioMix_SubmitPendingOutputInitPacket"),
    ("0x823852f8", "FM2_D3D_CreateTextureResourceFromFormat"),
    ("0x82386dc8", "FM2_AudioRenderFrame_EnqueueD3DCommandInitA"),
    ("0x82386e58", "FM2_AudioRenderFrame_EnqueueD3DCommandInitB"),
    ("0x823892f0", "FM2_AudioRenderFrame_EnqueueD3DCommandBindSurface"),
    ("0x8238e490", "FM2_AudioRenderFrame_EnqueueD3DCommandEmitPackets"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass74.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 74 (33 functions)\n",
    "XML attr table, D3D texture-desc/JPEG, career race, livery grow, com-object aggregate, audio render enqueue.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass74.md", "w", encoding="utf-8").write("\n".join(md))
