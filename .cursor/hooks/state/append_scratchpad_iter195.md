## Iteration 195

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8260AFA8 | sub_8260AFA8 | FM2_ProfileLuaRbTree_RotateLeftChildAt8 | 0.91 | Mirror of `FM2_RbTree_RotateLeftChildAt8` with sentinel `+61`; used by ProfileLua map erase `sub_824E2960`/`sub_8260BFB8`; 15 callers. |
| 0x8260ADF0 | sub_8260ADF0 | FM2_ProfileLuaRbTree_RotateRightChildAt8 | 0.91 | Mirror of `FM2_RbTree_RotateRightChildAt8` with sentinel `+61`; paired left rotate; 15 ProfileLua tree rebalance callers. |
| 0x8253D778 | sub_8253D778 | FM2_LiveryEditorRbTree_LowerBoundByKeyDword | 0.90 | RB-tree lower_bound on key at node `+12`; sentinel byte `+81`; `"invalid map/set<T> iterator"` in sibling erase. |
| 0x8253DE08 | sub_8253DE08 | FM2_LiveryEditorRbTree_InitIteratorLowerBound | 0.89 | Wraps lower_bound into `{tree, node}` iterator pair; vtable data xref; used by insert-hint helper. |
| 0x8253E4E0 | sub_8253E4E0 | FM2_LiveryEditorRbTree_FindInsertHintPair | 0.90 | Lower_bound iterator + insert-hint selection via key compare at `+12`; 15 livery editor map callers. |
| 0x8253E5D0 | sub_8253E5D0 | FM2_LiveryEditorState_Dtor | 0.89 | Clears string `+8`; COM release `+7`; `FM2_IntrusiveList_ClearAndFreeEntries`; frees pool `+4`; 14 livery dtors. |
| 0x827D1E40 | sub_827D1E40 | FM2_Render_CopyStateVector160FromSource | 0.90 | Counts `(end-begin)/160`; reserves via `sub_827D1D58`; memcpy range via `sub_827D1CF8`; 14 render state copies. |
| 0x824A1DD0 | sub_824A1DD0 | FM2_PresentationCarConfig_DtorWithComRelease | 0.91 | COM `Release` on `a1[17]` if set; then `FM2_PresentationCarConfig_Dtor`; 14 car/render config vtable dtors. |
| 0x827D6858 | sub_827D6858 | FM2_Profile_HashStdString65599ToOut | 0.90 | SSO string buffer select; `FM2_String_Hash65599SetHighBit`; stores hash to out-param; 10 profile lookup callers. |
| 0x827E7EF0 | sub_827E7EF0 | FM2_Profile_RemoveManagerBindingByHashedName | 0.89 | `FM2_BufFile_WriteCString` debug; hash via `FM2_Profile_HashStdString65599ToOut`; `sub_827E7080` binding removal; 15 profile callers. |
| 0x827D6DC8 | sub_827D6DC8 | FM2_Render_ForEachResourceCacheByPassFlag | 0.90 | `FM2_Render_LookupMaterialPassFlagByteByNegativeId`; `sub_827E1120`; `FM2_Render_ResourceCache_ForEachMatchingEntry`; 14 render callers. |
| 0x8254E4B0 | sub_8254E4B0 | FM2_Lua_MathTwoArgDispatchOrDefault | 0.91 | If `FM2_Lua_GetStackValueType>0` call `FM2_Lua_MathTwoArgDispatch`; else return default `a3`; 14 Lua math bindings. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8275E148 | sub_8275E148 | CRT wide-char fread; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single nested vtable `+56` delegate only. |
| 0x8242C960 | sub_8242C960 | Thin ComPtr assign → `sub_8242C7A0` wrapper only. |
| 0x8242C920 | sub_8242C920 | Thin ComPtr assign → `sub_8242C660` wrapper only. |
| 0x827E4A68 | sub_827E4A68 | Thin wrapper → `sub_827E0E40(..., 10, ...)` only. |
| 0x827D78E8 | sub_827D78E8 | Thin wrapper → `sub_827D77D8` after material pass lookup. |
| 0x827E8358 | sub_827E8358 | Hash-name wrapper → `sub_827E8278`; defer with `sub_827E7080` cluster. |
| 0x827E7080 | sub_827E7080 | Manager binding pair erase; defer full profile binding cluster. |
| 0x82224658 | sub_82224658 | Bounded `FM2_Crt_VsprintfS_L` wrapper; defer CRT cluster. |
| 0x82762C18 | sub_82762C18 | Bounded sprintf thunk → `sub_82388A48`. |
| 0x8245C5A0 | sub_8245C5A0 | Wide-string STL assign; UTF conversion path needs more context. |
| 0x827D1D58 | sub_827D1D58 | 160-byte vector reserve helper; defer with copy cluster tail. |
| 0x8253DE98 | sub_8253DE98 | Livery RB-tree equal_range pair; defer next livery iterator cluster. |
