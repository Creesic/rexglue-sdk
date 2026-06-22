## Iteration 196

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827E60D0 | sub_827E60D0 | FM2_Profile_LookupBindingBucketByHash | 0.91 | Binary search 16B `{hash, bucket*}` records; returns bucket `+8`; used by append/remove binding paths. |
| 0x827E7080 | sub_827E7080 | FM2_Profile_RemoveManagerBindingPairByHashAndIds | 0.90 | `FM2_Profile_LookupBindingBucketByHash`; scans dword pairs; compacts vector on `(a4,a5)` match; caller of hashed-name wrapper. |
| 0x827E8278 | sub_827E8278 | FM2_Profile_AppendManagerBindingPairByHash | 0.90 | Lookup/create bucket; `FM2_Lua_BindingVector_AppendPair`; finalize via `sub_827E81E8`; 14 profile binding callers. |
| 0x827E8358 | sub_827E8358 | FM2_Profile_AppendManagerBindingByHashedName | 0.89 | BufFile debug write; `FM2_Profile_HashStdString65599ToOut`; forwards to append pair helper; 14 profile callers. |
| 0x8253D920 | sub_8253D920 | FM2_LiveryEditorRbTree_UpperBoundByKeyDword | 0.90 | Upper_bound mirror of lower_bound; sentinel `+81`; key at `+12`; used by equal_range builder. |
| 0x8253DAD0 | sub_8253DAD0 | FM2_LiveryEditorRbTree_InitIteratorUpperBound | 0.89 | Wraps upper_bound into `{tree, node}` iterator; vtable data xref; paired with lower_bound init. |
| 0x8253DE68 | sub_8253DE68 | FM2_LiveryEditorRbTree_BuildEqualRangeIterators | 0.90 | Combines upper/lower bound iterators into 4-field equal_range struct; livery map caller `0x82541874`. |
| 0x827D1D58 | sub_827D1D58 | FM2_Render_ReserveStateVector160 | 0.91 | Alloc `160*count` via `sub_827D1690`; sets begin/end/cap pointers; `FM2_Stl_ThrowLengthError_VectorTooLong` guard. |
| 0x827D1CF8 | sub_827D1CF8 | FM2_Render_CopyStateRange160Loop | 0.90 | Loop copies 160B records via `FM2_Render_CopyManagerNotifyState160`; used by `FM2_Render_CopyStateVector160FromSource`. |
| 0x82487228 | sub_82487228 | FM2_AIOvertake_IsRivalWithinBlockingTimeMargin | 0.90 | SIMD sample + vtable race-time query; sum vs threshold `+676`; pairs with `FM2_AIOvertake_IsRivalAheadByTimeMargin`; 13 AI callers. |
| 0x8245B890 | sub_8245B890 | FM2_Stl_ConvertWideCharRangeToUtf8 | 0.91 | UTF-16 surrogate handling; emits 1–4 byte UTF-8; error codes 1–3; sole callee of wide-string assign. |
| 0x8245C5A0 | sub_8245C5A0 | FM2_Stl_StringAssignFromWideCharUtf8 | 0.90 | Reserves `4*wideLen`; calls UTF-8 converter; optional `sub_827F1FF8` validate; 15 string-assign callers. |
| 0x82224658 | sub_82224658 | FM2_Crt_VsprintfS_260_NulTerminate | 0.92 | `FM2_Crt_VsprintfS_L` with `0x104` cap; forces `a1[259]=0`; 16 debug/log format callers. |
| 0x82762C18 | sub_82762C18 | FM2_Crt_VsnprintfS_OrEInvalidArg | 0.91 | Returns `E_INVALIDARG` if `a2>0x7FFFFFFF`; else `sub_82388A48`; 16 bounded printf callers. |
| 0x82415FD8 | sub_82415FD8 | FM2_Crt_StrncpyS | 0.92 | Bounded C-string copy with `invalid_parameter`; errno 22/34; `count==-1` unbounded mode; 14 CRT callers. |
| 0x824D5EC8 | sub_824D5EC8 | FM2_Lua_PushSslUnitStringsFromFileTime | 0.91 | `FM2_Xam_FileTimeToLocalTimeWithBias`; `FileTimeToSystemTime`; `FM2_Lua_PushSslUnitStringsTable`; 13 SSL unit-string bindings. |
| 0x827E1698 | sub_827E1698 | FM2_Profile_TryGetCategoryDwordById | 0.90 | Category bitmask test; indirect table lookup; writes dword out-param; negative id via sorted category map; 13 profile callers. |
| 0x8232D298 | sub_8232D298 | FM2_CarAudioHashTree_LowerBoundByKeyDword | 0.90 | RB-tree lower_bound sentinel `+21`; key `v3[3] >= *a2`; feeds car-audio insert-hint helper. |
| 0x824976D8 | sub_824976D8 | FM2_CarAudioHashTree_FindInsertHintPair | 0.90 | Lower_bound iterator + insert-hint pair selection; 13 car-audio hash-map callers. |
| 0x8249EFE0 | sub_8249EFE0 | FM2_CarAudioHashTree_FindOrInsertByNamePrefix | 0.91 | `FM2_SceneGraph_CompareNodeNamePrefix`; inserts node via `sub_8249EC68`; returns node `+40`; 12 car-audio callers. |

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
| 0x827788A8 | sub_827788A8 | Thin wrapper → `sub_8277C7E8` → `sub_8277DF80` only. |
| 0x827D6D78 | sub_827D6D78 | Thin variant of `FM2_Render_ForEachResourceCacheByPassFlag`; defer with `sub_827E1050`. |
| 0x827E1050 | sub_827E1050 | Profile variant dword setter; needs more binding cluster context. |
| 0x8253FB60 | sub_8253FB60 | Hash erase + audio descriptor validate; defer property-list cluster. |
| 0x8246EC30 | sub_8246EC30 | `CMLPArray<int>` ctor; defer template array cluster. |
| 0x82360BA0 | sub_82360BA0 | Piecewise tuning-curve interpolate; defer tuning curve cluster. |
| 0x825E4268 | sub_825E4268 | Config-entry vector grow/move; defer config vector cluster. |
| 0x8232D3B0 | sub_8232D3B0 | `IAsyncWrapper` dtor; defer async wrapper cluster. |
| 0x825FB3E0 | sub_825FB3E0 | Car-audio RB-tree iterator init; thin wrapper over lower_bound. |
