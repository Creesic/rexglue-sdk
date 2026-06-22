
## Iteration 9

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82529508 | sub_82529508 | render_sort_partition_ptrs_by_distance_key | 0.89 | Dual-pointer partition on render-object ptr array comparing dword at object+104 (distance/sort key); calls sub_82527F60 median setup. Used by get-distance-key core. |
| 0x825296f0 | sub_825296F0 | render_sort_insertion_sort_ptrs_by_distance | 0.88 | Stable insertion sort shifting 4-byte pointer slots when key at *(ptr+104) out of order; calls sub_82523310 rotate helper. Distance-key update tail. |
| 0x8252a218 | sub_8252A218 | render_sort_heapsort_ptrs_by_distance | 0.87 | In-place heapsort on pointer range: swaps head/tail then sub_82526880 heapify per element. Paired with build-max-heap path. |
| 0x8252bcd8 | sub_8252BCD8 | render_sort_introsort_ptrs_by_distance | 0.90 | Introsort driver: recursion budget a3, pivot via sub_825297B8, falls back to sub_825299A0 insertion when span<=32 else sub_82528100/sub_8252A280 quicksort. |
| 0x8252cb90 | sub_8252CB90 | memory_tagged_16byte_vector_insert_n | 0.86 | STL vector insert for 16-byte tagged records: capacity math >>4, memmove via sub_82365720, fills new slots from 4-dword template in a4. Pool-grow path. |
| 0x8252d7f0 | sub_8252D7F0 | render_sort_vector_resize_and_sort_keys | 0.85 | Validates 48-byte element vector bounds; on shrink calls FM2_Render_Helper16E0SortKeyCompare to sort truncated tail; overflow traps to sub_827F1740. |
| 0x825349d0 | sub_825349D0 | heap_sift_up_12byte_triple | 0.88 | Binary-heap sift-up on 12-byte {dword,dword,dword} records with comparator callback; parent index (i-1)/2. Hybrid draw-path sort inner. |
| 0x82534f38 | sub_82534F38 | sort_median3_swap_12byte_triple | 0.87 | Orders three 12-byte records via comparator swaps (classic median-of-three network). Called from introsort partition on 8-byte pairs. |
| 0x82535968 | sub_82535968 | sort_partition_introselect_8byte_pair | 0.86 | When span>40 elements, median-of-3 at n/8 offsets via sub_82534DB0 then final pivot partition. Instance-path wrapper inner core. |
| 0x825484b8 | sub_825484B8 | lua_garage_car_record_manager_init_fields | 0.88 | Sets vtable off_820460A0; initializes eight FM2_IntrusiveList critsec/sentinel nodes plus head at +52; clears counters, sets byte +120=1. Garage lookup parse. |
| 0x82548a70 | sub_82548A70 | lua_garage_car_record_entry_init_fields | 0.87 | Sets vtable off_820460D8; four FM2_IntrusiveList_InitSentinel_0 lists; clears pointer/count fields. Paired garage entry ctor. |
| 0x825666f8 | sub_825666F8 | render_object_pass_prepare_draw_if_visible | 0.90 | Tests material vtable +36 visibility; on success resolves shader keys via sub_82559C38 (fallback strings missing/missing_packed), binds pass state, optional debug constant submit when byte_82A00B01. Feeds indexed draw emit. |
