## Iteration 70

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257DFC0 | sub_8257DFC0 | audio_engine_get_xhv_port_device_id | 0.91 | Returns `*(a1+188)`; compared to port id in sub_824B2568 XHV lookup; fallback in sub_824B1488 when local talker path unused. Vtable 0x82049EF0. |
| 0x8257E018 | sub_8257E018 | audio_engine_get_occlusion_fade_settings_ptr | 0.89 | Returns `*(a1+236)` as float*; distance inner/outer fade via fsel in sub_821E8EA0/sub_821F88F0; race-time gates on v24[4]/v24[5]. Vtable 0x82049F20. |
| 0x8257E7A8 | sub_8257E7A8 | audio_emission_dtor | 0.93 | Sets off_82049984; calls sub_82583CC0 (`CAudioEmission::vftable`); optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x82191FF8. |
| 0x8257E800 | sub_8257E800 | audio_emission_dtor_threadsafe_vtbl | 0.92 | Same sub_82583CC0 teardown; vtable off_82049A1C from sub_8257F428 thread-safe emission create path. RTTI `TRefCountedObjectThreadSafe@VCAudioEmission`. |
| 0x8257E668 | sub_8257E668 | audio_emission_release | 0.92 | COM Release refcount at +140; destroys at zero via vtable. CAudioEmission vtable 0x8204998C. |
| 0x8257E6E0 | sub_8257E6E0 | audio_emission_threadsafe_release | 0.93 | lwarx/stwcx atomic dec at +140; thread-safe emission vtable 0x82049A24. |
| 0x8257E858 | sub_8257E858 | audio_emission_group_threadsafe_dtor | 0.90 | off_82049AB0; sub_82583A20 sets `CAudioEmissionGroup::vftable`. RTTI `TRefCountedObjectThreadSafe@VCAudioEmissionGroup`. |
| 0x8257E8B0 | sub_8257E8B0 | audio_geometry_dtor | 0.93 | off_82049AEC; sub_825851C0 sets `CAudioGeometry::vftable`; sub_82670E18 geometry release. |
| 0x8257E930 | sub_8257E930 | audio_geometry_threadsafe_release | 0.91 | Atomic COM Release at +40; paired with audio_geometry_dtor in vtable cluster 0x82192018. RTTI `TRefCountedObjectThreadSafe@VCAudioGeometry`. |
| 0x8257E9A0 | sub_8257E9A0 | audio_cue_project_dtor | 0.93 | off_82049B7C; sub_82585378 sets `CAudioCueProject::vftable`; releases held object and clears STL strings. |
| 0x8257EA98 | sub_8257EA98 | audio_cue_ui_dtor | 0.90 | off_82049B9C from sub_8257FFD0 factory; sub_82586470→sub_82586310 teardown. RTTI `CAudioCueUI` / off_8204A864. |
| 0x8257EB58 | sub_8257EB58 | audio_cue_basic_deferred_dtor | 0.88 | off_82049C24 from sub_825800C0; sub_82586310 stops FMOD channel + clears cue strings. RTTI `CAudioCueBasicDeferred`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257E000 | sub_8257E000 | Returns dword at +224 only; caller use in sub_821EA058 still ambiguous. |
| 0x8257E600 | sub_8257E600 | Generic COM Release at +12 on CAudioEngine vtable slot; defer. |
| 0x8257EB08 | sub_8257EB08 | COM Release at +88 on cue class; defer with EB58 cluster iter 71. |
| 0x8257EBC0 | sub_8257EBC0 | Zeroed struct call via vtable+352; target semantics need sub_82581CB0 chain naming. |
| 0x8257EC50 | sub_8257EC50 | Same vtable+352 path as EBC0; defer together. |
| 0x82583CC0 | sub_82583CC0 | Internal CAudioEmission teardown helper; covered by emission dtors. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
