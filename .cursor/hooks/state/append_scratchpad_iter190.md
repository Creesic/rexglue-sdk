## Iteration 190

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827AF060 | sub_827AF060 | FM2_Render_ByteStreamReader_ResetVtable | 0.89 | Assigns base vtable `off_8214C434`; called from `FM2_Render_TransactionLogReader_InitVtable` dtor/reset path. |
| 0x827BE550 | sub_827BE550 | FM2_RbTree_IncrementIterator | 0.92 | Classic `std::map` iterator++ with sentinel byte at node `+29`; parent/child walk; 17 RB-tree erase/loop callers. |
| 0x82527B38 | sub_82527B38 | FM2_Render_ElementLinkVector_OffsetFromEnd | 0.91 | Computes `end - 12*count` with begin/end bounds trap; sole helper for element-link vector indexing. |
| 0x822073E8 | sub_822073E8 | FM2_Render_ElementLinkVector_GetEndPointer | 0.90 | Returns end pointer via offset-from-end with `count=1`; 18 render element-link callers. |
| 0x827E1EF0 | sub_827E1EF0 | FM2_Render_ProfileCategoryLookupOrDefault | 0.91 | Bitmask test on `this+16`; indirect table at `this+8`; negative id via `FM2_Profile_LookupSortedCategoryByteById`; returns default `a3` on miss. |
| 0x827D7308 | sub_827D7308 | FM2_Render_ProfileCategoryLookupRecursive | 0.90 | Wraps profile lookup; recurses through child pointer at `4*(slot+4)+this` when flag unset; 18 Lua/profile callers. |
| 0x821FE1A8 | sub_821FE1A8 | FM2_SceneGraph_DestroySubtreeAndClearNode | 0.90 | Calls `sub_821FE008` subtree destroy; frees body at `+4`; clears `+4/+8`; 18 scene-graph callers. |
| 0x826E07D0 | sub_826E07D0 | FM2_NuiSpeech_VoiceInput_OnInternalConstruct | 0.93 | Explicit `NUISPEECH::CVoiceInput::InternalFinalConstructAddRef`; flag massaging at `+16/+20`; critsec at `+524`; 17 XUI speech callers. |
| 0x8295DE30 | sub_8295DE30 | FM2_Jpeg_FwhtDispatchBySize | 0.91 | Dispatches inline FWHT for sizes 4/8; calls `FM2_Jpeg_Fwht32_InPlace` for 32; vtable `0x821c5f78`; parent of JPEG transform pipeline. |
| 0x824B77D8 | sub_824B77D8 | FM2_Lua_CopyStackSlotToUpvalue | 0.92 | `FM2_Lua_GetStackSlotPointer` + `FM2_Lua_PushLoadedClosureUpvalues_0`; copies 16B TValue; gray-object link; 17 Lua sort/binding callers. |
| 0x822A7CE0 | sub_822A7CE0 | FM2_Lua_PushU16IntoBindingSlot | 0.90 | `FM2_Lua_PushNumber` from ushort + `FM2_Lua_PopStackSlot`; pair to `FM2_Lua_PushNumberIntoBindingSlot`; 17 menu callers. |
| 0x82620DC8 | sub_82620DC8 | FM2_Lua_RemoveBindingSlotIfOwned | 0.91 | If owned-flag at `+12`, `FM2_Lua_RemoveStackSlotAtIndex`; 17 profile-Lua binding callers. |
| 0x82798D90 | sub_82798D90 | FM2_Render_BoundedStringCopy | 0.90 | Length guard `<=0x7FFFFFFF`; delegates to `sub_82793088` strncpy; else `E_INVALIDARG`; shader/string paths. |
| 0x82798A88 | sub_82798A88 | FM2_Render_CopyShaderNameField64 | 0.89 | Thin wrapper copying into `this+88` field, max 63 chars; 18 render shader/material thunks. |
| 0x8277C7B0 | sub_8277C7B0 | FM2_XtsClient_InitListNodePacketHeader | 0.90 | `FM2_XtsClient_SendRequestPacket_NoOpStub` on list-node header; called from `FM2_XtsClient_InitListNodePacketFields`; 16 network callers. |
| 0x8246EEE0 | sub_8246EEE0 | FM2_CmlpCharArray_Dtor | 0.92 | Sets `CMLPArray<char>::vftable`; frees heap buffer when SSO flag `+5` clear; 17 physics/XML callers. |
| 0x8240CC50 | sub_8240CC50 | FM2_Crt_Vsprintf | 0.88 | Forwards varargs to `vsprintf`; 17 debug/log format callers. |
| 0x825261E8 | sub_825261E8 | FM2_AI_CompareEntitiesByDistanceAndRadius | 0.91 | Sort predicate: flag bits at `+192` then radius²/distance² via `FM2_Math_ComputeVectorDistanceSquared`; 17 AI sort callers. |
| 0x827E5058 | sub_827E5058 | FM2_Render_ResourceCache_ForEachMatchingEntry | 0.91 | Critsec hash bucket `% 0xBB9`; iterates 24B entries; invokes callback when key match + flag mask; pair to update/remove helper. |
| 0x823BA330 | sub_823BA330 | FM2_Jpeg_BitWriter_PutBits | 0.92 | JPEG entropy bit buffer at `+24/+28`; emits bytes with `div_0xFF` stuffing; dest-manager refill; 17 libjpeg encode callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper chain → `sub_8279F5E0` → `sub_827A0400`; insufficient standalone evidence. |
| 0x82789188 | sub_82789188 | 40B wrapper → `sub_82790848` only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper → `sub_827B6828` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8275E148 | sub_8275E148 | CRT wide-char fread; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single nested vtable `+56` delegate only. |
| 0x8276E950 | sub_8276E950 | MSVC `std::string` SSO buffer select; defer STL helper cluster. |
| 0x826561E8 | sub_826561E8 | 152-byte indexed entry helper; defer car-audio material cluster. |
| 0x821E6348 | sub_821E6348 | AI path SIMD lerp; defer with `FM2_AIDriver_GetPathBufferLength` cluster. |
| 0x82790848 | sub_82790848 | State-pool index math; defer with iterator cluster. |
| 0x82792980 | sub_82792980 | Thin `!sub_82789768` iterator compare wrapper. |
| 0x827B0670 | sub_827B0670 | Transaction-log dtor; defer pairing with delete operator. |
| 0x827EBA50 | sub_827EBA50 | Vec128 store + callback invoke; defer element-link callback cluster. |
| 0x82762C18 | sub_82762C18 | Bounded sprintf thunk; defer with `sub_82388A48`. |
| 0x82793088 | sub_82793088 | Core strncpy helper; now named via `FM2_Render_BoundedStringCopy` caller. |
