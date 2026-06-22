## Iteration 66

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257CB40 | sub_8257CB40 | render_car_shader_pack_load_from_named_stream_notify | 0.92 | FM2_Render_NotifyManagerStateChange then render_car_shader_pack_load_from_named_stream; Release on a2. Vtable 0x82191EE0. |
| 0x8257CC98 | sub_8257CC98 | hash_name_vtable_init_capacity_32 | 0.91 | Sets CHashName vftable; capacity 32 at +72; zero 64-byte name buffer at +8; null heap str at +4. Vtable 0x82191EF8. |
| 0x8257CE18 | sub_8257CE18 | hash_name_copy_ctor_from | 0.90 | Clears dest buffer; FM2_HashName_CtorEmpty_0 from source +8 with source cap/flag +72/+76. |
| 0x8257CE80 | sub_8257CE80 | hash_name_load_from_stream | 0.89 | Stream vtable +88/+116/+140 read name/cap/flag into XTS buf; optional FM2_HashName_CtorEmpty_0 rebuild. Vtable 0x82191F20. |
| 0x8257D0D8 | sub_8257D0D8 | hash_name_reset_and_free_heap_str | 0.90 | FM2_Memory_FreeSmallBlockOrNull at +4; null hash id; memset 64-byte inline name at +8. |
| 0x8257D138 | sub_8257D138 | hash_name_set_from_cstr_and_hash | 0.91 | Pool alloc + strcpy_s heap copy; truncate into +8 buffer; FM2_HashName_CtorEmptyBody; stores cap/flag +72/+76. Vtable 0x82191F38. |
| 0x8257CF68 | sub_8257CF68 | lcg_rand_next_hi15 | 0.92 | Glibc-style LCG `1103515245*state+12345`; returns HIWORD&0x7FFF; advances *a1. |
| 0x8257CFD0 | sub_8257CFD0 | lcg_rand_range_float | 0.91 | LCG step then maps hi15 to [a2,a3] via scale 0.000030518509 (1/32768). |
| 0x8257D018 | sub_8257D018 | lcg_rand_advance_state | 0.92 | Single LCG advance; returns passthrough a2. |
| 0x8257D308 | sub_8257D308 | memory_alloc_xphysical_guarded_pool | 0.89 | FM2_Memory_XPhysicalAllocTracked; optional byte_82A00CE8 guard headers (sentinel 0x12345678), linked list dword_82A00CE0, canary memset. Vtable 0x82191F48. |
| 0x8257D468 | sub_8257D468 | memory_alloc_small_block_guarded_pool | 0.89 | FM2_Memory_AllocSmallBlockWithRetry with optional +32 guard; sentinel list dword_82A00CE4; 0xFF/0xAB fill pattern. |
| 0x8257D5A8 | sub_8257D5A8 | memory_alloc_guarded_pool_dispatch | 0.90 | If a2==0x100000 call xphysical_guarded_pool else small_block_guarded_pool. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257D200 | sub_8257D200 | Thin wrapper: zero fields then hash_name_set_from_cstr_and_hash. |
| 0x82727ED8 | sub_82727ED8 | Mesh ptr getter batch deferred to iter 67. |
| 0x82727F60 | sub_82727F60 | Mesh ptr getter batch deferred. |
| 0x82727EE8 | sub_82727EE8 | Mesh ptr getter batch deferred. |
| 0x82727F20 | sub_82727F20 | Mesh ptr getter batch deferred. |
| 0x82728140 | sub_82728140 | Active vertex count getter; defer with mesh batch. |
| 0x827C5938 | sub_827C5938 | Animation sample evaluator; partial read only. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x82727ED0 | sub_82727ED0 | Stores dword at +108 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
