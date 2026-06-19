### Infrastructure pass 10 (33 functions)

Hash-name/property-bag helpers, profile, livery-mask, car setup/dynamics.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821e98d0` | `FM2_Stl_StringIter_InitFromStringEnd` | Init string iterator at end of SSO/heap string buffer. |
| `0x8221c630` | `FM2_WString_ReserveCapacity` | Reserve/grow wide-string capacity to requested char count. |
| `0x8221f150` | `FM2_IntrusiveList_EraseNodeAndRebalance` | Erase intrusive-list node and rebalance parent links. |
| `0x82419a74` | `FM2_Crt_StackProbeAlloc` | Stack probe / alloca chunk adjustment helper. |
| `0x8241de18` | `FM2_Char_ToLowerAscii` | Lowercase ASCII A-Z to a-z for hash-name normalization. |
| `0x8242fa30` | `FM2_RbTree_CompareKeyLess` | RB-tree key compare: `left.key < right.key`. |
| `0x8245bc20` | `FM2_Stl_StringIter_InitAtOffset` | Init string iterator at byte offset in source string. |
| `0x82466aa8` | `FM2_Profile_GetOptionalHeapBlockPtr` | Returns optional profile heap block pointer at +32. |
| `0x824a0f08` | `FM2_PresentationCarConfig_Dtor` | Presentation car-config dtor; reset base vtable. |
| `0x824de690` | `FM2_Profile_FreeOptionalHeapBlock` | Free optional profile heap block at +32. |
| `0x825a0430` | `FM2_LiveryMask_AtexitFreeSingleton` | Livery-mask singleton atexit: free backing storage. |
| `0x825eaa78` | `FM2_DirectIface_ResetPixelShaderBinding` | Direct3D iface: clear pixel-shader binding and release. |
| `0x82671728` | `FM2_FMOD_Dsp_AdjustDelayLinePointers` | FMOD DSP: adjust delay-line buffer pointers by delta. |
| `0x825c5158` | `FM2_CareerRace_QueryGameOptionsByToken` | SQL query GameOptions by token string for career race. |
| `0x8245b080` | `FM2_PropertyBag_RbTreeLowerBound` | Property-bag RB-tree lower_bound recursive walk. |
| `0x8221bee0` | `FM2_PropertyBag_AllocListNode` | Allocate property-bag intrusive-list node (121-byte). |
| `0x8253f9f8` | `FM2_HashNamePropertyList_EraseNode` | Erase node from hash-name property intrusive list. |
| `0x82331cc8` | `FM2_HashName_RbTreeLowerBoundInit` | Init hash-name RB-tree lower_bound iterator pair. |
| `0x824e7fd0` | `FM2_FrameAllocMap_AdvanceIterator` | Advance frame-alloc map ordered-set iterator. |
| `0x82617f48` | `FM2_FrameAllocMap_InsertOrAssign` | Insert/assign entry in frame-alloc ordered map. |
| `0x82656e78` | `FM2_IntVector_EraseRangeShift` | Erase int-vector subrange and memmove tail. |
| `0x825a04e0` | `FM2_LiveryMask_Ctor` | Construct livery-mask COM object with default params. |
| `0x825a05f0` | `FM2_LiveryMask_CopyCreateParams` | Copy livery-mask create-params struct fields. |
| `0x82207398` | `FM2_Stl_SnprintfToBuffer` | Vararg snprintf into fixed stack buffer. |
| `0x82460968` | `FM2_TuningDb_InitIntListSentinel` | Init tuning-db intrusive int-list sentinel. |
| `0x8221cb08` | `FM2_TuningDb_InitFloatListSentinel` | Init tuning-db intrusive float-list sentinel. |
| `0x821e67b8` | `FM2_CarSetup_Ctor` | Construct car-setup object with installed-parts defaults. |
| `0x822930c0` | `FM2_CarDynamics_InitSubsystems` | Init car-dynamics subsystem blocks and ramp samples. |
| `0x824cbfd0` | `FM2_CameraScript_DestroyModule` | Destroy camera script module when last ref. |
| `0x82204908` | `FM2_Stl_StringIter_GetCursorPtr` | String iterator: get current cursor pointer. |
| `0x82204958` | `FM2_Stl_StringIter_AdvanceChar` | String iterator: advance cursor by one char. |
| `0x824541a8` | `FM2_NetworkMessage_RbTreeLowerBound` | Network-message RB-tree lower_bound recursive walk. |
| `0x8245b008` | `FM2_PropertyBag_AllocRbTreeNode` | Allocate property-bag RB-tree node skeleton. |