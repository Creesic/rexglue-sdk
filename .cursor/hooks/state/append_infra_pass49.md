### Infrastructure pass 49 (33 functions)

XML reader/writer attr helpers, render adapter init, profile/Lua binding alloc, network dispatch.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824ccf40` | `FM2_XmlReader_ParseBoolAttrDefault` | Evidence from decompile and caller context. |
| `0x824cce90` | `FM2_XmlReader_ParseFloatAttrDefault` | Evidence from decompile and caller context. |
| `0x824cd740` | `FM2_XmlReader_ParseBoolAttrStrict` | Evidence from decompile and caller context. |
| `0x824cd0d0` | `FM2_XmlReader_BuildErrorPathString` | Evidence from decompile and caller context. |
| `0x824cd1b0` | `FM2_XmlReader_StreamReadLineIntoBuffer` | Evidence from decompile and caller context. |
| `0x824cd3a8` | `FM2_XmlWriter_AppendStringAttr` | Evidence from decompile and caller context. |
| `0x824cd430` | `FM2_XmlWriter_AppendFloatAttr` | Evidence from decompile and caller context. |
| `0x824ce240` | `FM2_XmlWriter_WriteIndentBeforeClose` | Evidence from decompile and caller context. |
| `0x824ce390` | `FM2_XmlWriter_WriteSelfClosingTag` | Evidence from decompile and caller context. |
| `0x824ce970` | `FM2_XmlReader_LoadAttributesFromArchive` | Evidence from decompile and caller context. |
| `0x824cf688` | `FM2_XmlReader_ApplyAttrDefaultsFromNode` | Evidence from decompile and caller context. |
| `0x824cf8f8` | `FM2_XmlReader_FindAttrEntryByName` | Evidence from decompile and caller context. |
| `0x824cfa78` | `FM2_XmlReader_FindAttrByPathSegments` | Evidence from decompile and caller context. |
| `0x824cfc10` | `FM2_XmlReader_CopyAttrDefaultsToNode` | Evidence from decompile and caller context. |
| `0x824cc840` | `FM2_XmlReader_DtorAttrNodeList` | Evidence from decompile and caller context. |
| `0x824cc4d0` | `FM2_BufFile_CompactModuleRefTableLocked` | Evidence from decompile and caller context. |
| `0x824cd4d0` | `FM2_XmlReader_DtorStringNodeA` | Evidence from decompile and caller context. |
| `0x824cd538` | `FM2_XmlReader_DtorStringNodeB` | Evidence from decompile and caller context. |
| `0x824e3fb0` | `FM2_RenderAdapter_InitSwitchModeBase` | Evidence from decompile and caller context. |
| `0x824e53c0` | `FM2_RenderAdapter_InitSwitchModeAlt` | Evidence from decompile and caller context. |
| `0x824e7bd0` | `FM2_CareerRace_MoveRewardsBlock` | Evidence from decompile and caller context. |
| `0x824e8368` | `FM2_HashName_AssignFromPropertySlice` | Evidence from decompile and caller context. |
| `0x824e9960` | `FM2_LiveProfile_ReadWriteBufferHeader` | Evidence from decompile and caller context. |
| `0x824eae80` | `FM2_Lua_AllocBindingPairVector` | Evidence from decompile and caller context. |
| `0x824eaf90` | `FM2_Lua_BindingPairMedianOfThree` | Evidence from decompile and caller context. |
| `0x824ebf20` | `FM2_Lua_BindingPairSortEntryThunk` | Evidence from decompile and caller context. |
| `0x824d6480` | `FM2_Lua_PushDampingFromKeyframeDouble` | Evidence from decompile and caller context. |
| `0x824d75a0` | `FM2_Profile_MakeStringKeyComPtr` | Evidence from decompile and caller context. |
| `0x824d76a0` | `FM2_Profile_ParseUnsignedFromSubString` | Evidence from decompile and caller context. |
| `0x824d8b28` | `FM2_Math_AllocForceVectorComPtr` | Evidence from decompile and caller context. |
| `0x824d3cb0` | `FM2_Scene_GetNotifyStateFromParamHelper` | Evidence from decompile and caller context. |
| `0x82587c88` | `FM2_Network_DispatchMessageFromQueueLocked` | Evidence from decompile and caller context. |
| `0x8258d228` | `FM2_Set_LowerBoundByKeyInTree` | Evidence from decompile and caller context. |