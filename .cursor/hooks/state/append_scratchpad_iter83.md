## Iteration 83

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82681250 | sub_82681250 | fmod_channel_get_dsp_unit_by_index | 0.91 | Flush pending commands; walk DSP unit list at channel+8 by index; returns unit ptr and optional parent at unit+128. Used by disconnect/connect/volume paths. |
| 0x826813E0 | sub_826813E0 | fmod_channel_disconnect_dsp_unit | 0.90 | Finds unit whose parent +128 matches target; unlinks intrusive lists; FM2_FMOD_Dsp_ReverbProcessDelayLine from "..\\src\\fmod_dspi.cpp". Flush case 1. |
| 0x82681B20 | sub_82681B20 | fmod_channel_connect_dsp_units | 0.90 | Allocates connection via fmod_dsp_connection_pool_alloc; splices input/output intrusive lists; updates unit counts. Flush case 0. |
| 0x826817B8 | sub_826817B8 | fmod_channel_is_ancestor_of | 0.89 | Recursively compares parent links at unit+128 via fmod_channel_get_dsp_unit_by_index; returns 0 if a2 in a1 output chain. |
| 0x82680C40 | sub_82680C40 | fmod_channel_get_dsp_unit_count | 0.90 | Flush then returns channel+56 DSP unit count under critical section. Used before group-command iteration. |
| 0x8269CB60 | sub_8269CB60 | fmod_dsp_connection_pool_alloc | 0.91 | Alloc 148-byte connections from pool; source path "..\\src\\fmod_dsp_connectionpool.cpp"; initializes link nodes. |
| 0x8269E670 | sub_8269E670 | fmod_dsp_unit_set_volume_clamped | 0.92 | Clamps float to [-1,1]; stores at unit+140; propagates through nested coefficient tables when unit type != 6. |
| 0x82681770 | sub_82681770 | fmod_channel_set_dsp_unit_volume_by_index | 0.91 | fmod_channel_get_dsp_unit_by_index then fmod_dsp_unit_set_volume_clamped. Tail of fmod_channel_handle_set_volume. |
| 0x82681F00 | sub_82681F00 | fmod_channel_execute_channel_group_command | 0.90 | For mute/group flags: iterates units, calls fmod_channel_disconnect_dsp_unit or reconnect path. Dispatched from flush cases 2-4. |
| 0x8266F430 | sub_8266F430 | fmod_channel_handle_get_dsp_unit_type | 0.90 | resolve then channel vtable+64 into out-buffer; caller checks types 3/4/5/10/18/19 for parameter curve logic. |
| 0x8266F500 | sub_8266F500 | fmod_channel_handle_get_parameter_min_max_default | 0.89 | resolve then vtable+72 fills min/max/default floats; used before Volume parameter automation in sub_8265B7E8. |
| 0x8266F480 | sub_8266F480 | fmod_channel_handle_set_3d_attributes | 0.88 | resolve then channel vtable+68 with three doubles; follows get_parameter_min_max_default in sub_82661458 3D path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257EFA0 | sub_8257EFA0 | Abstract off_82049CAC vtable with purecall slots only. |
| 0x8257F7E8 | sub_8257F7E8 | Thin adjustor thunk → audio_cue_basic_deferred_threadsafe_dtor. |
| 0x82580268 | sub_82580268 | Thin wrapper → init_params_dtor only. |
| 0x8257F8C8 | sub_8257F8C8 | Thin wrapper → play_params_dtor only. |
| 0x8257FAA0 | sub_8257FAA0 | Thin wrapper → stop_params_dtor only. |
| 0x82581948 | sub_82581948 | Shared scalar-delete thunk (CSkidMarksLink, CAudioThreadLink vtables). |
| 0x82582AA0 | sub_82582AA0 | Body of audio_manager_deferred_dtor; no separate rename needed. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x82583A10 | sub_82583A10 | Single-byte zero store only; too trivial alone. |
| 0x82495868 | sub_82495868 | Adjustor scalar-dtor thunk at this-4 only. |
| 0x82495870 | sub_82495870 | Adjustor release thunk at this-4 only. |
| 0x8249AE00 | sub_8249AE00 | 28-byte string vector assign; defer iter 84. |
| 0x8266F3C0 | sub_8266F3C0 | vtable+52 eight-arg call; decompiler loses args at caller. |
| 0x82681740 | sub_82681740 | Thunk to fmod_channel_enqueue_dsp_command only. |
| 0x82681748 | sub_82681748 | Thunk to fmod_channel_enqueue_channel_group_command only. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
