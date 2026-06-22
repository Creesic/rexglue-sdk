## Iteration 73

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257FFD0 | sub_8257FFD0 | audio_engine_create_cue_ui_with_thread_link | 0.91 | Alloc CAudioCueUI off_82049B9C; CAudioThreadLink via audio_thread_link_ctor; links cue to thread frame counter; render notify. Vtable 0x82049E6C. |
| 0x825800C0 | sub_825800C0 | audio_engine_create_cue_basic_deferred_and_play | 0.90 | Alloc CAudioCueBasicDeferred off_82049C24; vtable+32 play/init with a3..a5. Vtable 0x82049E70. |
| 0x8257F060 | sub_8257F060 | audio_cue_basic_deferred_scalar_dtor | 0.90 | Calls sub_82585F80 (`CAudioCueBasicDeferred::vftable` teardown); optional free. Vtable 0x82192088. |
| 0x8257F730 | sub_8257F730 | audio_cue_basic_deferred_threadsafe_ctor | 0.93 | audio_cue_basic_deferred_ctor then `TRefCountedObjectThreadSafe<CAudioCueBasicDeferred>::vftable`. Used from FFD0/81D48. |
| 0x82586988 | sub_82586988 | audio_effect_create_from_dsp_type_index | 0.91 | dword_829A2694 DSP type table; sub_8266EC40 create DSP; alloc off_8204A924; vtable+52 init. RTTI `CAudioEffect`. Caller: audio_engine_create_audio_effect_and_notify. |
| 0x82586930 | sub_82586930 | audio_effect_dtor | 0.90 | off_8204A924; sub_82586658 FMOD channel release; scalar delete optional. |
| 0x82581560 | sub_82581560 | audio_engine_init_fmod_system | 0.92 | audio_engine_load_config_from_xml; guarded pool alloc; FMOD System create; getDriverCaps/setOutput/setGeometrySettings; DSP capture. Vtable 0x82049F70 / 0x821921A8. |
| 0x825816F0 | sub_825816F0 | audio_thread_link_ctor | 0.93 | FM2_ComObject_InitBaseVtable423C0; sets `CAudioThreadLink::vftable`. RTTI confirmed. |
| 0x82581998 | sub_82581998 | audio_thread_shutdown | 0.92 | Sets `CAudioThread::vftable`; signal stop; release thread link; close semaphores; FM2_LiveryMask_CloseWorkerThreadHandle on "Forza Audio Thread" worker. |
| 0x82581AE0 | sub_82581AE0 | audio_thread_ctor | 0.93 | `CAudioThread::vftable`; semaphores; spawns worker `sub_82581AD8` with name `"Forza Audio Thread"`; audio_thread_link alloc at +300. |
| 0x825817D0 | sub_825817D0 | audio_thread_process_deferred_queue_step | 0.88 | CAudioThread queue at +24; sub_8245CD38/D048 deferred command dispatch; increments +192 frame counter. Called from audio_thread_main loop. |
| 0x82583CC0 | sub_82583CC0 | audio_emission_destroy_fmod_resources | 0.91 | Sets `CAudioEmission::vftable`; releases DSP handles +3/+4/+5; sub_8266FF98 on channel +2. Shared by emission dtors. |

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
| 0x825801E8 | sub_825801E8 | CParams4IAudioCueBasicInit dtor; defer with 80150/80308 cluster. |
| 0x82580268 | sub_82580268 | Thin wrapper → sub_825801E8 only. |
| 0x82580298 | sub_82580298 | Deferred play dispatch; needs CDeferredQueue vtable context iter 74. |
| 0x82580308 | sub_82580308 | Deferred init enqueue; defer with 80150 iter 74. |
| 0x82581760 | sub_82581760 | Deferred destroy-car-audio-link enqueue; thin params wrapper. |
| 0x82581CB0 | sub_82581CB0 | CAudioThread singleton allocator; defer with 81A68 main. |
| 0x82586658 | sub_82586658 | Internal audio_effect channel release; covered by audio_effect_dtor. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
