## Iteration 197

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827E1050 | sub_827E1050 | FM2_Profile_SetCategoryVariantDword | 0.91 | Negative id path via `FM2_Profile_LookupSortedCategoryByteById`; `FM2_Profile_SetVariantTypeAndAlloc(..., 5)`; stores dword at `+16`; pairs with float setter. |
| 0x827DFC88 | sub_827DFC88 | FM2_Profile_UpdateCategoryMaskAndSetDword | 0.90 | Category bitmask OR at `+16`; indirect dword table write or variant alloc type 5; callee of dword setter. |
| 0x827E1120 | sub_827E1120 | FM2_Profile_SetCategoryVariantFloat | 0.91 | Mirror of dword setter; variant type `9` float; `sub_827DFD58` body path; 13 profile/render callers. |
| 0x827DFD58 | sub_827DFD58 | FM2_Profile_UpdateCategoryMaskAndSetFloat | 0.90 | Bitmask update + float table write or variant alloc type 9; callee of float setter. |
| 0x827D6D78 | sub_827D6D78 | FM2_Render_SetPassFlagDwordAndForEachCache | 0.89 | `FM2_Render_LookupMaterialPassFlagByteByNegativeId`; `FM2_Profile_SetCategoryVariantDword`; `FM2_Render_ResourceCache_ForEachMatchingEntry`; 13 render callers. |
| 0x8253FB60 | sub_8253FB60 | FM2_HashNamePropertyList_EraseWithAudioPairValidate | 0.90 | `sub_825448F8` iterator; `FM2_AudioSample_BuildOutputPairDescriptorValidateBody_0`; `FM2_HashNamePropertyList_EraseNode`; 13 livery/audio callers. |
| 0x8246EC30 | sub_8246EC30 | FM2_CmlpArray_IntCtor | 0.92 | `CMLPArray<int>` vtable; pool alloc `4*count`; stores count at `+12`; flag bytes `+4/+5`; 13 physics/array callers. |
| 0x82360BA0 | sub_82360BA0 | FM2_TuningCurve_InterpolatePiecewiseLinear | 0.90 | Two-segment linear interp between knot floats; clamps below min/above max; stores result at `a1[12]`; 13 tuning callers. |
| 0x825E4268 | sub_825E4268 | FM2_ConfigEntryVector_GrowAndMove | 0.91 | `FM2_AllocPoolAcquire4xCount`; move via `sub_8222C9D8`; `FM2_ConfigEntry_DestroyRange`; 13 config-vector callers. |
| 0x8232D3B0 | sub_8232D3B0 | FM2_AsyncWrapper_Dtor | 0.92 | Calls `sub_8232D360` (`IAsyncWrapper` release); optional pool free; 13 async vtable dtors. |
| 0x825FB3E0 | sub_825FB3E0 | FM2_CarAudioHashTree_InitIteratorLowerBound | 0.89 | Wraps `FM2_CarAudioHashTree_LowerBoundByKeyDword` into `{tree, node}`; vtable data xref; feeds insert-hint helper. |
| 0x827E2510 | sub_827E2510 | FM2_Profile_AssignUnitStringTableSlot | 0.89 | Sets slot type `*a2=a3`; clears prior string for type 14; `sub_827E04B0` at `a1+1240`; 13 profile unit-string callers. |

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
| 0x8232D360 | sub_8232D360 | `IAsyncWrapper` release body only; called solely from `FM2_AsyncWrapper_Dtor`. |
| 0x82363688 | sub_82363688 | CRT free tail jump; defer CRT allocator cluster. |
| 0x8221A930 | sub_8221A930 | `FM2_HashName_AssignFromSlashPath` gate; defer hash-name cluster. |
| 0x8264A7A8 | sub_8264A7A8 | Physics fsel blend helper; needs `sub_82644230` cluster context. |
| 0x82768D30 | sub_82768D30 | Network message send path; needs slot/cluster context. |
