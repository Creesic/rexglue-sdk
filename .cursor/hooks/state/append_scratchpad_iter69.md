## Iteration 69

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257DB78 | sub_8257DB78 | audio_engine_add_geometry_and_set_emitter_positions | 0.91 | FM2_FMOD_Geometry_AddPolygonFromVMX_0 twice (-20000..20000, -300..300); held object vtable+100 sets four float3 emitter vectors. CAudioEngine vtable 0x82191FB8. |
| 0x8257DC80 | sub_8257DC80 | audio_engine_get_3d_audio_vectors | 0.90 | Held object vtable+104 fills four float vectors (position/velocity pairs). CAudioEngine vtable 0x82191FC0. |
| 0x8257DD28 | sub_8257DD28 | audio_engine_update_listener_orientations | 0.89 | vtable+32 listener count loop; VMX128 forward/up per listener; vtable+36 per index. CAudioEngine vtable 0x82191FC8. |
| 0x8257DFC8 | sub_8257DFC8 | audio_engine_get_xhv_headset_present_flag | 0.92 | Returns `*(a1+192)`; sole callee from XHVEngineIsHeadsetPresent on per-port engine entry. Vtable 0x82049EF4. |
| 0x8257DFF8 | sub_8257DFF8 | audio_engine_get_script_binding_ptr | 0.88 | Returns `*(a1+216)`; Lua nil-check path in sub_825EEC10. Vtable 0x82049F0C. |
| 0x8257E038 | sub_8257E038 | audio_engine_get_channel_parameter_float | 0.90 | Held object vtable+76 lookup by index; nested vtable+44 get float param. CAudioEngine vtable 0x82191FD0. |
| 0x8257ECE0 | sub_8257ECE0 | audio_engine_apply_master_and_dsp_settings | 0.92 | sub_8266F160 master volume; calls audio_engine_apply_eq_dsp_parameters + audio_engine_apply_reverb_dsp_parameters; Release held object. Vtable 0x82192058. |
| 0x8257E110 | sub_8257E110 | audio_fmod_reverb_config_init_defaults | 0.93 | FMOD reverb/EQ sentinel defaults (-10000 dry/room, 0.5 HF, 0.02 decay, 39.99, 100, 5000); called from audio_engine_ctor at a1+7. |
| 0x8257E208 | sub_8257E208 | audio_crossfade_lerp_dsp_parameter_block | 0.88 | fsel-clamped lerp of full DSP parameter struct between two blocks; called from audio mix path 0x824C6B00. |
| 0x8257F0B8 | sub_8257F0B8 | audio_engine_ctor | 0.95 | Sets CAudioEngine::vftable; zeroes fields; calls audio_fmod_reverb_config_init_defaults; master/EQ defaults. Vtable 0x82192090. |
| 0x8257F180 | sub_8257F180 | audio_engine_dtor | 0.95 | Calls audio_engine_shutdown_fmod_resources; CAudioEngine teardown. Vtable 0x82192098. |
| 0x8257E750 | sub_8257E750 | audio_emission_group_dtor | 0.91 | Sets CAudioEmissionGroup::vftable via sub_82583A20; FMOD emission release path; optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x82191FF0. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257DFC0 | sub_8257DFC0 | Returns dword at +188 only; XHV fallback port handle getter needs more caller semantics. |
| 0x8257E000 | sub_8257E000 | Returns dword at +224 only. |
| 0x8257E018 | sub_8257E018 | Returns dword at +236 only. |
| 0x8257E600 | sub_8257E600 | Generic COM Release at +12; defer with E668/E6E0 cluster. |
| 0x8257E668 | sub_8257E668 | Generic COM Release at +140. |
| 0x8257E6E0 | sub_8257E6E0 | Atomic COM Release at +140; defer naming until class identified. |
| 0x8257E7A8 | sub_8257E7A8 | Audio emission-group variant dtor; needs sub_82583CC0 class name. |
| 0x8257E800 | sub_8257E800 | Audio emission-group variant dtor; needs sub_82583CC0 class name. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
