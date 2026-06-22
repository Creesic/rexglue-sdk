import json

RENAMES = [
    ("0x824e83e0", "FM2_StdList_SpliceAndAdjustCount"),
    ("0x824e8990", "FM2_StdList_SpliceFrontIfNonEmpty"),
    ("0x8236be98", "FM2_AudioMix_SetupRenderTargetTexturesThunk"),
    ("0x82295ff0", "FM2_Profile_DtorReleaseSubobjects"),
    ("0x82460498", "FM2_D3D_GetGpuWaitElapsedSeconds"),
    ("0x82205828", "FM2_Profile_DestroyStringListAt28"),
    ("0x8221cd60", "FM2_Profile_DestroyWStringListAt16"),
    ("0x82251890", "FM2_ProfileDb_RbTreeRotateLeft"),
    ("0x82251aa8", "FM2_ProfileDb_RbTreeRotateRight"),
    ("0x82253130", "FM2_ProfileDb_AllocRbTreeNode220"),
    ("0x8225c278", "FM2_LuaLobbySort_MergeSortPass0"),
    ("0x8225c420", "FM2_LuaLobbySort_MergeSortPass1"),
    ("0x8225c5c8", "FM2_LuaLobbySort_MergeSortPassPass2"),
    ("0x8225c770", "FM2_LuaLobbySort_MergeSortPass3"),
    ("0x8225c918", "FM2_LuaLobbySort_MergeSortPass4"),
    ("0x82464a28", "FM2_StdList_CheckLengthAndAddCount"),
    ("0x8242d0a0", "FM2_Profile_CloseXamContentIfOpen"),
    ("0x8222ee70", "FM2_ProfileWStringNode_DtorOptionalFree"),
    ("0x82251a30", "FM2_AllocPoolAcquire224xCount"),
    ("0x8227c900", "FM2_AudioSignalGate_Ctor_F1CC"),
    ("0x822905e0", "FM2_SceneGraph_SetChildSlotVisibleByType"),
    ("0x82296528", "FM2_Profile_ResetStateAfterNotify"),
    ("0x82297678", "FM2_Profile_ApplyTuningRecordFromDb"),
    ("0x82297bd8", "FM2_Lua_PushDisplayStringClosure"),
    ("0x8229a220", "FM2_AudioSample_BuildIteratorPair"),
    ("0x8229ca88", "FM2_AudioRenderFrame_ProcessSampleBatch"),
    ("0x8229ccf0", "FM2_WaitText_Dtor"),
    ("0x8229dd50", "FM2_WaitAnimation_Ctor"),
    ("0x8229f1b8", "FM2_CompositeAdapterState_Dtor"),
    ("0x824603d8", "FM2_D3D_GetQueryPerformanceElapsedDiv"),
    ("0x82460430", "FM2_D3D_GetTickCountElapsedMs"),
    ("0x82299e18", "FM2_AudioSample_FindNextBufferNode"),
    ("0x827fa088", "FM2_DebugLog_NoOpStub"),
]

# fix typo in pass 21
RENAMES = [(a, n.replace("MergeSortPassPass2", "MergeSortPass2")) for a, n in RENAMES]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824e83e0": "Circular-list splice with count adjustment (lobby sort).",
    "0x8225c278": "Stable merge sort pass for lobby list (mode 0).",
    "0x827fa088": "Empty debug log stub called from audio render path.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass21.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 21 (33 functions)\n", "Lobby sort splice/merge, profile DB RB-tree, D3D timer, audio render, profile tuning.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass21.md", "w", encoding="utf-8").write("\n".join(md))
