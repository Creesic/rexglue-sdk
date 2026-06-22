
## Iteration 22

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82554C60 | sub_82554C60 | stl_vector_16byte_insert_n_at | 0.90 | MSVC vector insert at a2 for a3×16-byte elements; stl_vector_56byte_copy_construct_from temp; stl_vector_16byte_backward_shift_swap/shift paths; FM2_Stl_ThrowLengthError_VectorTooLong. sub_82555130 callee. |
| 0x825546C8 | sub_825546C8 | stl_vector_56byte_assign_from | 0.91 | Assigns 56-byte (14-dword) elements from source vector; copy/shrink/grow via stl_vector_56byte_uninitialized_copy_n and sub_824ED780 element copy. stl_vector_16byte_insert_n_at helper. |
| 0x82554440 | sub_82554440 | stl_vector_56byte_copy_construct_from | 0.92 | Ensures capacity via sub_824EDC40 then copies [begin,end) 56-byte records with sub_824ED780. Used by insert/swap helpers. |
| 0x82554560 | sub_82554560 | stl_vector_56byte_clear_and_destroy | 0.90 | Validates begin/end then stl_vector_56byte_erase_suffix_from to clear all 56-byte elements. stl_vector_56byte_assign_from empty-source path. |
| 0x825544D8 | sub_825544D8 | stl_vector_56byte_erase_suffix_from | 0.89 | Destroys suffix via stl_vector_56byte_uninitialized_copy_n + sub_825A5480; updates end pointer. stl_vector_56byte_clear_and_destroy core. |
| 0x825543E8 | sub_825543E8 | stl_vector_56byte_uninitialized_copy_n | 0.91 | stl_vector_56byte_uninitialized_copy_strings over [a1,a2); returns dest advanced by 14*dwords per element. Vector copy building block. |
| 0x82553A68 | sub_82553A68 | stl_vector_56byte_uninitialized_copy_strings | 0.92 | Per 56-byte element: FM2_Stl_String_AssignRange for two embedded strings at +0 and +28. Element-wise copy for material-name pairs. |
| 0x825548F0 | sub_825548F0 | stl_vector_16byte_backward_shift_swap | 0.90 | Swaps 16-byte (4-dword) records from [a1,a2) backward into a3 using temp 56-byte vector copy. Insert-without-realloc path in stl_vector_16byte_insert_n_at. |
| 0x825505A8 | sub_825505A8 | lua_table_quicksort_stack_range | 0.91 | Recursive quicksort over Lua stack indices a2..a3; uses lua_table_compare_less_than_at_slots; FM2_Lua_SetStackTop stack juggling. Self-recursive partition. |
| 0x82550510 | sub_82550510 | lua_table_compare_less_than_at_slots | 0.90 | Compare hook for sort: if type at slot2==0 uses sub_824B6C60 else copies slots and sub_824B7A10 `<` metamethod; FM2_Lua_IsStackSlotTruthy result. |
| 0x825508B0 | sub_825508B0 | lua_table_sort_in_place | 0.91 | Lua table.sort entry: sub_8254D5E0 length; optional comparator at slot2; calls lua_table_quicksort_stack_range(1,n). Registered at 0x82190C98. |
| 0x825524B0 | sub_825524B0 | lua_string_format | 0.92 | Parses `%` format specifiers over Lua string arg; sprintf_0 build; FM2_Lua_ErrorVprintf on invalid option to 'format'. Registered at 0x82190D88. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82551B30 | sub_82551B30 | string.find/gsub body (sub_82551948 + FM2_FindAndReplaceDelimitedTextRange); registered at 0x82190D48—defer thin wrapper split next pass. |
| 0x82553890 | sub_82553890 | Intrusive-list time advance with vtable+64/+40; registered 0x82190EC0 but no readable export name in xref scan. |
| 0x82557940 | sub_82557940 | Quantize float triplet to int16×32767; audio caller sub_821F7EE8—defer with audio cluster. |
| 0x82558BA8 | sub_82558BA8 | VMX vec4 normalize when |w|>=0.001; render callee sub_82647838—math helper without strings. |
| 0x82558E70 | sub_82558E70 | FP8 geometry kernel; still no domain context. |
