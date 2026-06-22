### Infrastructure pass 27 (33 functions)

XML/buf-file cluster, input SSL bindings, profile string lists, race ghost sort, UI property helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824cda18` | `FM2_XmlReader_FindElementByNameRecursive` | Depth-first XML element lookup by backslash path segment. |
| `0x824cd238` | `FM2_XmlReader_CompareElementNameI` | Evidence from decompile and caller context. |
| `0x824cdae0` | `FM2_XmlReader_GetChildElementByName` | Evidence from decompile and caller context. |
| `0x824cd6d0` | `FM2_XmlReader_ParseFloatAttribute` | Evidence from decompile and caller context. |
| `0x8245bdf8` | `FM2_BufFile_NormalizePathToLowercase` | Lowercases buf-file path in place for prefix matching. |
| `0x82463980` | `FM2_LiveryMask_GrowPendingRecordTable` | Evidence from decompile and caller context. |
| `0x824638b8` | `FM2_LiveryMask_AllocPendingRecordSlot188` | Evidence from decompile and caller context. |
| `0x824d10d8` | `FM2_Input_SslDeviceContext_Ctor` | Evidence from decompile and caller context. |
| `0x824d2710` | `FM2_Input_SslDeviceBinding_Ctor` | Evidence from decompile and caller context. |
| `0x824d0850` | `FM2_Input_SslContext_InitFromPath` | Evidence from decompile and caller context. |
| `0x824d2188` | `FM2_Input_SslBindingRecord_Init` | Evidence from decompile and caller context. |
| `0x821d04c0` | `FM2_Profile_AllocTuningHashNode16` | Evidence from decompile and caller context. |
| `0x824d36e0` | `FM2_ProfileTuningHashNode_Ctor` | Evidence from decompile and caller context. |
| `0x8221d380` | `FM2_Profile_AllocStringListNodeWithKey` | Evidence from decompile and caller context. |
| `0x8221d3d8` | `FM2_ProfileStringList_CheckLengthAndAdd` | Evidence from decompile and caller context. |
| `0x82246630` | `FM2_LiveryEditor_FindOrInsertDecalTabEntry` | Evidence from decompile and caller context. |
| `0x824ffe90` | `FM2_LiveryEditor_RbTreeInsertDecalTabKey` | Evidence from decompile and caller context. |
| `0x822fcfc0` | `FM2_RaceGhost_IntroSortKeyframeBuffer` | Evidence from decompile and caller context. |
| `0x8230bac8` | `FM2_RaceGhost_PartitionKeyframeBuffer` | Dual-pivot partition for race ghost keyframe introsort. |
| `0x82331988` | `FM2_RaceGhost_BuildPlaybackUpdateTask` | Evidence from decompile and caller context. |
| `0x82331b30` | `FM2_RaceGhost_InitDeferredPlaybackWrapper` | Evidence from decompile and caller context. |
| `0x8235d3f8` | `FM2_UI_PropertyMaskMatchesState` | Evidence from decompile and caller context. |
| `0x8235d3b0` | `FM2_UI_GetAnimPropertyBlockById` | Evidence from decompile and caller context. |
| `0x8235e3b0` | `FM2_UI_GetAnimPropertyFloatById` | Evidence from decompile and caller context. |
| `0x8235e540` | `FM2_UI_CountMatchingPropertiesInGroup` | Evidence from decompile and caller context. |
| `0x8235e610` | `FM2_UI_GetPropertyRecordByGroupIndex` | Evidence from decompile and caller context. |
| `0x8235e290` | `FM2_UI_GetDefaultPropertyFloatScaled` | Evidence from decompile and caller context. |
| `0x82360c40` | `FM2_Input_ParseRumbleMotorPairXml` | Evidence from decompile and caller context. |
| `0x823661d8` | `FM2_Memory_DeferredFreeMapInsertNode` | Evidence from decompile and caller context. |
| `0x8236b598` | `FM2_AudioRender_ComputeFrontBufferMixSample` | Evidence from decompile and caller context. |
| `0x8236b4d0` | `FM2_AudioRender_SampleFrontBufferRegion` | Evidence from decompile and caller context. |
| `0x8236c480` | `FM2_GpuKick_SubmitShaderConstantsFromTable` | Evidence from decompile and caller context. |
| `0x8236d948` | `FM2_Render_FreeGpuKickTagAt13404` | Evidence from decompile and caller context. |