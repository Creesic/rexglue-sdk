## Iteration 74

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82580150 | sub_82580150 | audio_cue_basic_deferred_init_params_ctor | 0.92 | Sets `CAudioCueBasicDeferred::CParams4IAudioCueBasicInit::vftable`; copies two STL strings + byte flag + render notify. Called from enqueue_init. Vtable 0x82192170. |
| 0x825801E8 | sub_825801E8 | audio_cue_basic_deferred_init_params_dtor | 0.91 | Releases notify ComPtr + clears strings; reverts to `CDeferredQueue::CCommandParams::vftable`. Vtable 0x82192178. |
| 0x82580298 | sub_82580298 | audio_cue_basic_deferred_dispatch_init_to_play | 0.90 | Unpacks CParams4IAudioCueBasicInit fields; calls deferred-queue vtable+20 with cue name strings and flag. Vtable 0x82192188. |
| 0x82580308 | sub_82580308 | audio_cue_basic_deferred_enqueue_init | 0.91 | Bump-alloc 0x44 params; builds init_params_ctor; enqueues on CAudioCueBasicDeferred deferred queue. Vtable 0x82049FA0 / 0x82192190. |
| 0x8257F7F0 | sub_8257F7F0 | audio_cue_basic_deferred_play_params_ctor | 0.91 | Sets `CParams2IAudioCueBasicPlay::vftable`; stores play flag byte + render notify. Used by enqueue_play. |
| 0x8257F940 | sub_8257F940 | audio_cue_basic_deferred_enqueue_play | 0.90 | Bump-alloc play params; play_params_ctor; enqueue on deferred queue at a1+4. Vtable 0x82049FA4. |
| 0x8257F9D8 | sub_8257F9D8 | audio_cue_basic_deferred_stop_params_ctor | 0.90 | Sets `CParams1IAudioCueBasicStop::vftable`; copies render notify state. Used by enqueue_stop. |
| 0x8257FB28 | sub_8257FB28 | audio_cue_basic_deferred_enqueue_stop | 0.90 | Bump-alloc stop params; stop_params_ctor; enqueue on deferred queue. Vtable 0x82049FA8. |
| 0x82581CB0 | sub_82581CB0 | audio_thread_singleton_get_or_create | 0.92 | Alloc 304 bytes; audio_thread_ctor; stores in dword_82A00CF0. Called from FM2 startup paths. Vtable 0x821921F0. |
| 0x82581A68 | sub_82581A68 | audio_thread_main | 0.93 | Worker entry from audio_thread_ctor thunk; waits on semaphore; loops audio_thread_process_deferred_queue_step until stop flag +20. |
| 0x82581C60 | sub_82581C60 | audio_thread_scalar_dtor | 0.91 | Calls audio_thread_shutdown; optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x821921E8. |
| 0x82581760 | sub_82581760 | audio_thread_link_enqueue_destroy_car_audio | 0.89 | Alloc `CParams1IAudioThreadDestroyCarAudioLinkInternal` on deferred queue; stores car-audio handle. RTTI confirmed. Vtable 0x821921B8. |

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
| 0x82581D48 | sub_82581D48 | CAudioManager car-audio list acquire; defer with 81F10/82020 iter 75. |
| 0x82586658 | sub_82586658 | Internal FMOD channel release; covered by audio_effect_dtor. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
