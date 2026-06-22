
## Iteration 10

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82526880 | sub_82526880 | render_sort_heap_sift_down_ptrs_by_distance | 0.90 | Max-heap sift-down on pointer array comparing object+104 keys; child pick uses `<` on sibling keys. Used by render_sort_build_max_heap_48byte and heapsort paths. |
| 0x825269b8 | sub_825269B8 | render_sort_heap_sift_down_ptrs_by_distance_desc | 0.90 | Mirror sift-down with reversed child comparison (`>=` on moved key). Paired with render_sort_build_max_heap_ptrs_by_distance_desc / heapsort desc. |
| 0x82526818 | sub_82526818 | sort_median3_order_ptrs_by_distance_asc | 0.88 | Three-pointer median-of-three ordering network using ascending compare on *(ptr+104). Called from sort_introselect_median3_ptrs_by_distance_asc. |
| 0x82526950 | sub_82526950 | sort_median3_order_ptrs_by_distance_desc | 0.88 | Descending-order median-of-three on pointer keys at +104. Used by sort_introselect_median3_ptrs_by_distance_desc and introsort pivot path. |
| 0x82527f60 | sub_82527F60 | sort_introselect_median3_ptrs_by_distance_asc | 0.87 | When span>40 ptrs, samples median3 at n/8 offsets via sort_median3_order_ptrs_by_distance_asc then final median3. Feeds render_sort_partition_ptrs_by_distance_key. |
| 0x82528060 | sub_82528060 | sort_introselect_median3_ptrs_by_distance_desc | 0.87 | Descending introselect median3 helper (mirror of 82527F60) using sort_median3_order_ptrs_by_distance_desc. Feeds render_sort_introsort pivot partition. |
| 0x82559c38 | sub_82559C38 | render_pass_lookup_shader_name_slot | 0.91 | Linear search `(char**)` table at pass+512 count +524; strcmp against a2; returns `(index<<18)|0x3FFFC` packed slot id or 0. Fallback strings missing/missing_packed in prepare_draw_if_visible. |
| 0x82523310 | sub_82523310 | stl_rotate_pointer_range | 0.86 | GCD-based cyclic rotation of dword pointer range [result,a3) moving element from a2; used by insertion-sort and other sort helpers. |
| 0x82534db0 | sub_82534DB0 | sort_median3_swap_8byte_pair | 0.88 | Comparator-driven median3 swap network on three 8-byte {dword,dword} records. Called from sort_partition_introselect_8byte_pair. |
| 0x82534850 | sub_82534850 | heap_sift_up_8byte_pair | 0.89 | Binary-heap sift-up on 8-byte pairs with comparator callback; parent (i-1)/2. Tail of heap_sift_down_8byte_pair. |
| 0x82528100 | sub_82528100 | render_sort_build_max_heap_ptrs_by_distance_desc | 0.88 | Builds heap by descending index calling render_sort_heap_sift_down_ptrs_by_distance_desc. Introsort large-span path (paired with 82528000 asc variant). |
| 0x8252a280 | sub_8252A280 | render_sort_heapsort_ptrs_by_distance_desc | 0.87 | Descending heapsort: swap ends then sift-down desc. Introsort fallback quicksort branch terminus. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825297b8 | sub_825297B8 | Near-duplicate of render_sort_partition_ptrs_by_distance_key (82529508); differs only in introselect median helper choice. Defer unified naming. |
| 0x825299a0 | sub_825299A0 | Near-duplicate of render_sort_insertion_sort_ptrs_by_distance (825296F0); introsort small-span fallback only. |
