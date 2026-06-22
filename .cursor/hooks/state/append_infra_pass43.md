### Infrastructure pass 43 (33 functions)

BufFile module refs, car-audio mix channel, Lua binding vector, compiler optional slots, render sort.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824c7e28` | `FM2_LuaCompiler_ResetOptionalState` | Evidence from decompile and caller context. |
| `0x824ca780` | `FM2_CarAudioMixChannel_DtorBase` | Evidence from decompile and caller context. |
| `0x824ca9b8` | `FM2_Stl_Vector_IncRefCopyRange` | Evidence from decompile and caller context. |
| `0x824caa20` | `FM2_Stl_Vector_AssignRefCountedRange` | Evidence from decompile and caller context. |
| `0x824caa80` | `FM2_Stl_Vector_DecRefRange` | Evidence from decompile and caller context. |
| `0x824cab60` | `FM2_CarAudioMixChannel_ClearVoiceList` | Evidence from decompile and caller context. |
| `0x824cabb8` | `FM2_CarAudioMixChannel_EraseVoiceRange` | Evidence from decompile and caller context. |
| `0x824ca908` | `FM2_Stl_Vector_MoveConstructRefCountedRange` | Evidence from decompile and caller context. |
| `0x824caad8` | `FM2_CarAudioMixChannel_ReplaceVoiceRange` | Evidence from decompile and caller context. |
| `0x824cbe78` | `FM2_BufFile_GetLazyInitFlagPtr` | Evidence from decompile and caller context. |
| `0x824cbee8` | `FM2_BufFile_DerefStreamHandle` | Evidence from decompile and caller context. |
| `0x824cc1a0` | `FM2_BufFile_BinarySearchModuleRef` | Evidence from decompile and caller context. |
| `0x824cc438` | `FM2_BufFile_EnsureCapacityAndCopy` | Evidence from decompile and caller context. |
| `0x824cc6f0` | `FM2_BufFile_AppendFromBufferPtr` | Evidence from decompile and caller context. |
| `0x824cc760` | `FM2_BufFile_AppendCString` | Evidence from decompile and caller context. |
| `0x824cc890` | `FM2_BufFile_FindModuleRefIfLoaded` | Evidence from decompile and caller context. |
| `0x824cbef8` | `FM2_BufFile_AllocGrowableStringBuffer` | Evidence from decompile and caller context. |
| `0x824cbf60` | `FM2_BufFile_StreqOptionalCase` | Evidence from decompile and caller context. |
| `0x8242d140` | `FM2_Profile_ApplyTuningRecordFromDevice` | Evidence from decompile and caller context. |
| `0x82464020` | `FM2_Lua_GetComPtrMetatableSingleton` | Evidence from decompile and caller context. |
| `0x82463b40` | `FM2_Lua_InitBindingPairListHead` | Evidence from decompile and caller context. |
| `0x824bf9e0` | `FM2_Lua_AllocParserStateGcObject` | Evidence from decompile and caller context. |
| `0x824c2c10` | `FM2_LuaSyntax_ParseStatement` | Evidence from decompile and caller context. |
| `0x824c79d0` | `FM2_LuaCompiler_EraseOptionalRange` | Evidence from decompile and caller context. |
| `0x824c7818` | `FM2_LuaCompiler_ReplaceOptionalRange` | Evidence from decompile and caller context. |
| `0x82204e90` | `FM2_Lua_BindingPairVector_ShrinkToSize` | Evidence from decompile and caller context. |
| `0x82204c50` | `FM2_Lua_BindingPairVector_MoveTailElements` | Evidence from decompile and caller context. |
| `0x82205488` | `FM2_Lua_BindingPairVector_GrowCapacity` | Evidence from decompile and caller context. |
| `0x82417bb0` | `FM2_Render_SortDrawListByMaterialKey` | Evidence from decompile and caller context. |
| `0x82455bd8` | `FM2_Network_ClonePayloadListIntoNode` | Evidence from decompile and caller context. |
| `0x824c7658` | `FM2_LuaCompiler_MoveOptionalSlotRange` | Evidence from decompile and caller context. |
| `0x826af8c0` | `FM2_D3D_InitVoicePresentationSubsystem` | Evidence from decompile and caller context. |
| `0x824c72d0` | `FM2_LuaCompiler_CopyOptionalSlotFromSource` | Evidence from decompile and caller context. |