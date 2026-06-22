## Iteration 68

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257D618 | sub_8257D618 | audio_engine_create_dsp_capture_buffer | 0.89 | sub_8266EC40(FMOD system, type 17, a1+100); sub_8266EF18 stores DSP handle at +100. CAudioEngine vtable 0x82191F70. |
| 0x8257D668 | sub_8257D668 | audio_engine_shutdown_fmod_resources | 0.90 | vtable+72; release DSP +100; sub_8266EFF0 on FMOD system +8; Release held objects +4/+8. Called from CAudioEngine dtor 0x8257F180. Vtable 0x82191F78. |
| 0x8257D700 | sub_8257D700 | audio_engine_query_device_and_open_channel | 0.88 | FMOD::System::getDriverInfo; FM2_FMOD_System_CreateChannelEx; format enum via held object vtable+24; sums stream buffer sizes. Vtable 0x82191F80. |
| 0x8257D820 | sub_8257D820 | audio_engine_apply_eq_dsp_parameters | 0.89 | Channel at +24: sub_8266F260 bypass +28; sub_8266F308 slots 0..12 from floats +32..+80. Called from 0x8257ECE0. Vtable 0x82191F88. |
| 0x8257D980 | sub_8257D980 | audio_engine_apply_reverb_dsp_parameters | 0.89 | Channel at +100: bypass +104; four sub_8266F308 params +108..+120; sub_8266F368 3-vector at +124. Vtable 0x82191F90. |
| 0x8257DA18 | sub_8257DA18 | audio_engine_dsp_add_parameter_indexed | 0.87 | sub_8266EE40 get master DSP; sub_8266F730 (FM2_FMOD_Channel_GetVolumeFromEventTable + add path). Vtable 0x82191F98. |
| 0x8257DA58 | sub_8257DA58 | audio_engine_dsp_set_parameter_indexed | 0.87 | sub_8266EE40; sub_8266F6E8 (set path via sub_826823A8). Vtable 0x82191FA0. |
| 0x8257DA98 | sub_8257DA98 | audio_engine_reverb_dsp_set_parameter | 0.91 | Held object vtable+52 lookup DSP by name "Reverb"; nested +52 get channel; sub_8266F6E8 set param. Vtable 0x82191FA8. |
| 0x8257DB38 | sub_8257DB38 | audio_engine_get_output_driver_handle | 0.88 | Held object at +4 vtable+96 writes driver handle to out-param. Vtable 0x82191FB0. |
| 0x827281E8 | sub_827281E8 | render_skinned_mesh_get_max_influence_count | 0.90 | Returns `*(ushort*)(a1+32)` max bone influence count; used to clamp in get_active_vertex_count. |
| 0x82728218 | sub_82728218 | render_skinned_mesh_build_vertex_influence_mask | 0.88 | FM2_RenderTls_GetWorkerSlotMask16 loop; FM2_Render_ComputePassLightingSlotOffset64B per slot; byte mask at a1+120 if slot+51 <= max influence. Vtable 0x8219CA98. |
| 0x8257D200 | sub_8257D200 | hash_name_construct_from_cstr | 0.88 | Zero object fields; calls hash_name_set_from_cstr_and_hash. Common ctor used from property/hash paths. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257DB78 | sub_8257DB78 | Large CAudioEngine mixer/orchestrator; defer iter 69. |
| 0x8257DC80 | sub_8257DC80 | 3D audio vector query via vtable+104; defer. |
| 0x8257DD28 | sub_8257DD28 | Listener loop with vtable+32; defer. |
| 0x8257DFC0 | sub_8257DFC0 | Returns dword at +188 only. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
