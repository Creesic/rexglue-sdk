
## Iteration 11

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827492a8 | sub_827492A8 | render_shader_constant_apply_packed_slot | 0.89 | Decodes packed slot id (a2>>1)&0x1FFFF and (a2>>15)&0x1FFF8; computes dest ptr from pass fields +256/+260/+264/+268; delegates to render_shader_constant_sparse_tree_merge_bits. 49 calls from DrawPassMaterialSetupBodyB. |
| 0x82748d50 | sub_82748D50 | render_shader_constant_sparse_tree_merge_bits | 0.88 | Tagged tree walk: low 2 bits select branch vs leaf; leaf merges 64-bit values into buffer at a2 using bit index *a3 with shift/mask. Recursive on inner nodes. |
| 0x82803280 | sub_82803280 | render_byteswap_value_by_element_width | 0.90 | Switch on width 2/4/8 bytes: ROL16 for word, custom 32/64-bit byte swap. Called many times from FM2_Render_InitShaderConstantTables endian fixups. |
| 0x823abb20 | sub_823ABB20 | jpeg_write_output_byte_to_dest_manager | 0.91 | Libjpeg dest manager: writes byte via *ptr++, decrements free count, calls flush callback at +12 when empty; sets error 24 on flush failure. Used by marker emit paths. |
| 0x82732db0 | sub_82732DB0 | math_matrix_vector_multiply_accumulate | 0.87 | For result rows, dot-products columns of a2/a3 into output a4 (*a4 += sum a2[i]*a3[i]). Render field helper cluster. |
| 0x823a2b78 | sub_823A2B78 | jpeg_compress_install_huffman_table | 0.89 | Alloc table if null; copies 17-byte header; validates summed code lengths in 1..256 else JERR(8); copies bits block and clears field +276. From jpeg_compress_init_default_huff_tables. |
| 0x823b2438 | sub_823B2438 | jpeg_mem_alloc_aligned_from_pool | 0.88 | FM2_Memory_AllocGpuTagged(a2+16,612433920); stores alignment offset byte before 16-byte aligned pointer. jinit_memory_mgr / alloc_large/small. |
| 0x82758ed8 | sub_82758ED8 | crt_fgetc_locked_fill_buffer | 0.86 | CRT locked fgetc: validates FILE*, checks _flag/_fileno, errno=22 on invalid, decrements _cnt or calls sub_82421138 refill. FM2_LuaIO_FileOpen read path. |
| 0x825297b8 | sub_825297B8 | render_sort_introsort_pivot_partition_ptrs_by_distance | 0.87 | Same dual-pointer partition as render_sort_partition_ptrs_by_distance_key but uses sort_introselect_median3_ptrs_by_distance_desc setup (sub_82528060). Introsort pivot only. |
| 0x825299a0 | sub_825299A0 | render_sort_insertion_sort_ptrs_by_distance_small_span | 0.86 | Insertion sort on pointer span comparing *(ptr+104); uses stl_rotate_pointer_range. Introsort fallback when span<=32 (render_sort_introsort_ptrs_by_distance). |
| 0x82559cb0 | sub_82559CB0 | render_shader_constant_store_float_to_packed_slot | 0.88 | VMX: vcfsx converts stack float; ORs presence bit in byte table via byte_8200E4CC; stvewx stores to computed slot address from packed index a2. Object-pass bind state. |
| 0x82565028 | sub_82565028 | render_object_pass_match_interior_draw_category | 0.91 | Prefix-tests material name at a1+4 against seatL/seatR/bumperFrame/cagerace/interior/steering_wheel/wishbone; caches bool at a1+372. Flush lists path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8264eb08 | sub_8264EB08 | 8-byte thunk: `return *(float*)a1` only; used as callback wrapper, no standalone semantics. |
| 0x823eb2e8 | sub_823EB2E8 | 12-byte XAudio EH/vector destructor stub; insufficient behavior beyond compiler glue. |
| 0x8255a120 | sub_8255A120 | VMX constant-mask loop clear but tail delegates to sub_82747C10; defer until callee analyzed. |
