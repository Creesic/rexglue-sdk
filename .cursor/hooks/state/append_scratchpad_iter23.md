
## Iteration 23

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82551B30 | sub_82551B30 | lua_string_find_or_gsub | 0.91 | Lua args: haystack, pattern, start index; a2==0 runs FM2_FindAndReplaceDelimitedTextRange gsub loop else sub_82551948 find + push start/end indices. Regex guard sub_82759340 on "^$*+?.([%-". Registered 0x82190D48. |
| 0x82551948 | sub_82551948 | cstring_find_plain_substring | 0.92 | Non-regex find: memchr first char then byte-compare pattern length; returns match ptr or 0. lua_string_find_or_gsub plain-pattern path. |
| 0x82553890 | sub_82553890 | presentation_timeline_update_nodes_to_time | 0.88 | Stores time at result[18]; walks intrusive list head; vtable+64 on nodes[6]/[7] with time; vtable+40 on [8] triggers intrusive_list_erase_node_release_refs. Vtable entry 0x82046E7C; reg 0x82190EC0. |
| 0x82553788 | sub_82553788 | intrusive_list_erase_node_release_refs | 0.90 | Unlinks doubly-linked node, timeline_node_release_attached_com_objects on +8, FM2_Memory_FreeSmallBlockOrNull, decrements list count. presentation_timeline_update_nodes_to_time helper. |
| 0x82553388 | sub_82553388 | timeline_node_release_attached_com_objects | 0.89 | Clears node fields + releases COM at +16/+20/+24 via vtable+8. Called when timeline node erased. |
| 0x82557940 | sub_82557940 | audio_quantize_float_triplet_to_int16_32767 | 0.91 | Clamps float triplet with fsel against ±1 then stores (int)(fp*32767.0) into 3×int16 output. sub_821F7EE8 audio path after filter coeffs. |
| 0x82558BA8 | sub_82558BA8 | render_vmx_normalize_vec4_if_w_nonzero | 0.88 | If |w|>=0.001: VMX vsubfp/vmsum3fp128/vrsqrtefp normalize path; else passthrough lvx128. sub_82647838 render callee. |
| 0x8255FCE0 | sub_8255FCE0 | render_shader_resource_view_assign_and_wait_ready | 0.90 | render_shader_resource_view_clear_lock; AssignRetainedHandle; resource_lock_wait_ready_state3; stores shader blob +80 at bundle+36. render_shader_resource_view_bundle_init. |
| 0x824A6998 | sub_824A6998 | resource_lock_wait_ready_state3 | 0.91 | FM2_ResourceLock_WaitForReadyOrTimeout loop until +36/+40 both 3; sets +48 and increments +56 under critsec. Shader view assign wait helper. |
| 0x8250F640 | sub_8250F640 | render_shader_resource_view_clear_lock | 0.90 | If lock set calls sub_824A1028 release path; SetInterfaceThreadSafe(0); clears +36. Paired with assign helper. |
| 0x825593F0 | sub_825593F0 | render_vmx_project_point_between_dual_spheres | 0.87 | VMX vmsum4fp distance tests vs radius²; returns 0 outside both spheres; else sub_82559340 projection writes VMX128 to out. sub_82559508 callee. |
| 0x82553098 | sub_82553098 | lua_math_random | 0.92 | rand() scaled; 0 args returns [0,1); 1 arg [1,n]; 2 args uniform in [m,n] with empty-interval error. FM2_Lua_PushNumber result. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82558E70 | sub_82558E70 | FP8 geometry kernel; still no standalone naming context. |
| 0x82559508 | sub_82559508 | VMX wrapper calling render_vmx_project_point_between_dual_spheres; defer with 825591C8 raycast cluster. |
| 0x825591C8 | sub_825591C8 | Multi-ray vmsum3fp hit-distance loop; needs sub_82557F80 pairing. |
| 0x8255B318 | sub_8255B318 | VMX reflection/direction compute with D3D validate; truncated register use. |
| 0x82552200 | sub_82552200 | Lua quoted-string append with escape rules; defer with string lib batch. |
