import json

RENAMES = [
    ("0x824c7e28", "FM2_LuaCompiler_ResetOptionalState"),
    ("0x824ca780", "FM2_CarAudioMixChannel_DtorBase"),
    ("0x824ca9b8", "FM2_Stl_Vector_IncRefCopyRange"),
    ("0x824caa20", "FM2_Stl_Vector_AssignRefCountedRange"),
    ("0x824caa80", "FM2_Stl_Vector_DecRefRange"),
    ("0x824cab60", "FM2_CarAudioMixChannel_ClearVoiceList"),
    ("0x824cabb8", "FM2_CarAudioMixChannel_EraseVoiceRange"),
    ("0x824ca908", "FM2_Stl_Vector_MoveConstructRefCountedRange"),
    ("0x824caad8", "FM2_CarAudioMixChannel_ReplaceVoiceRange"),
    ("0x824cbe78", "FM2_BufFile_GetLazyInitFlagPtr"),
    ("0x824cbee8", "FM2_BufFile_DerefStreamHandle"),
    ("0x824cc1a0", "FM2_BufFile_BinarySearchModuleRef"),
    ("0x824cc438", "FM2_BufFile_EnsureCapacityAndCopy"),
    ("0x824cc6f0", "FM2_BufFile_AppendFromBufferPtr"),
    ("0x824cc760", "FM2_BufFile_AppendCString"),
    ("0x824cc890", "FM2_BufFile_FindModuleRefIfLoaded"),
    ("0x824cbef8", "FM2_BufFile_AllocGrowableStringBuffer"),
    ("0x824cbf60", "FM2_BufFile_StreqOptionalCase"),
    ("0x8242d140", "FM2_Profile_ApplyTuningRecordFromDevice"),
    ("0x82464020", "FM2_Lua_GetComPtrMetatableSingleton"),
    ("0x82463b40", "FM2_Lua_InitBindingPairListHead"),
    ("0x824bf9e0", "FM2_Lua_AllocParserStateGcObject"),
    ("0x824c2c10", "FM2_LuaSyntax_ParseStatement"),
    ("0x824c79d0", "FM2_LuaCompiler_EraseOptionalRange"),
    ("0x824c7818", "FM2_LuaCompiler_ReplaceOptionalRange"),
    ("0x82204e90", "FM2_Lua_BindingPairVector_ShrinkToSize"),
    ("0x82204c50", "FM2_Lua_BindingPairVector_MoveTailElements"),
    ("0x82205488", "FM2_Lua_BindingPairVector_GrowCapacity"),
    ("0x82417bb0", "FM2_Render_SortDrawListByMaterialKey"),
    ("0x82455bd8", "FM2_Network_ClonePayloadListIntoNode"),
    ("0x824c7658", "FM2_LuaCompiler_MoveOptionalSlotRange"),
    ("0x826af8c0", "FM2_D3D_InitVoicePresentationSubsystem"),
    ("0x824c72d0", "FM2_LuaCompiler_CopyOptionalSlotFromSource"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass43.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 43 (33 functions)\n",
    "BufFile module refs, car-audio mix channel, Lua binding vector, compiler optional slots, render sort.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass43.md", "w", encoding="utf-8").write("\n".join(md))
