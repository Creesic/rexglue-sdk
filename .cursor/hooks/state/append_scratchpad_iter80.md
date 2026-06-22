## Iteration 80

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826745E8 | sub_826745E8 | fmod_channel_handle_resolve | 0.93 | Returns 35/36 on null; writes resolved channel ptr to out. Shared prologue for all 8266Fxxx channel wrappers. |
| 0x8266F038 | sub_8266F038 | fmod_channel_handle_stop | 0.92 | resolve then channel vtable+20(arg=1). Called from audio_fmod_channel_holder_dtor and audio_effect_detach_channel. |
| 0x8266F120 | sub_8266F120 | fmod_channel_handle_play | 0.92 | resolve then channel vtable+32. Tail-called by audio_effect_start_channel. |
| 0x8266F1B8 | sub_8266F1B8 | fmod_channel_handle_set_paused | 0.93 | Sets/clears FMOD channel flag bit 2 at +232. Tail-called by audio_effect_set_channel_paused. |
| 0x8266F218 | sub_8266F218 | fmod_channel_handle_get_paused | 0.93 | Reads flag bit 2 at channel+232 into out-bool. Called by audio_effect_is_channel_paused. |
| 0x8266F260 | sub_8266F260 | fmod_channel_handle_set_bypass | 0.93 | Sets/clears flag bit 4 at channel+232. Tail-called by audio_effect_set_channel_bypass. |
| 0x8266F2C0 | sub_8266F2C0 | fmod_channel_handle_get_bypass | 0.93 | Reads flag bit 4 at channel+232. Called by audio_effect_is_channel_bypassed. |
| 0x8266F308 | sub_8266F308 | fmod_channel_handle_set_parameter_float | 0.92 | resolve then channel vtable+40(index, float). Used by audio_effect_lerp_set_channel_parameter. |
| 0x8266F368 | sub_8266F368 | fmod_channel_handle_get_parameter_float | 0.92 | resolve then channel vtable+44 5-arg get. Called by audio_effect_get_channel_parameter_float. |
| 0x8266EC40 | sub_8266EC40 | fmod_system_create_dsp_for_channel | 0.90 | FM2_FMOD_System_ValidateChannelInGlobalList then sub_8267B1E0 DSP create (mixer unit path for type 1). Caller audio_effect_create_from_dsp_type_index. |
| 0x8245CE70 | sub_8245CE70 | audio_thread_link_free_deferred_task_node | 0.89 | Frees deferred-task params at node+8 via FM2_DeferredTaskParams_FreeIfOutsidePool; frees node if outside pool range. |
| 0x8245D210 | sub_8245D210 | audio_thread_link_free_deferred_task_list | 0.90 | Walks intrusive list at *result via next+20; frees each node; clears head/tail; may free list header block. Called from audio_thread_link_shutdown. |

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
| 0x824647D0 | sub_824647D0 | Atomic COM release; defer iter 81 with render-frame-link cluster. |
| 0x8249BFC0 | sub_8249BFC0 | CCarAudio singleton alloc; defer iter 81 with car_audio ctor cluster. |
| 0x8249B2F8 | sub_8249B2F8 | Large CCarAudio ctor; defer iter 81. |
| 0x824950A0 | sub_824950A0 | Render frame-link base ctor; defer iter 81. |
| 0x82495300 | sub_82495300 | Frame-counter link ctor; defer iter 81. |
| 0x8266F558 | sub_8266F558 | Sets channel+164 only; need more context on field semantics. |
| 0x8266F080 | sub_8266F080 | Thin wrapper → sub_82681740; defer with 826817xx cluster. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
