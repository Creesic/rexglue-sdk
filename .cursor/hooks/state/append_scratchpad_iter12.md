
## Iteration 12

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255a120 | sub_8255A120 | render_pass_vmx_clear_constant_register_masks | 0.88 | Two VMX128 loops vand-clear register pairs at pass+16 using mask tables at +72/+73 and +64/+65; then calls sub_82747C10. From render_object_pass_prepare_draw_if_visible. |
| 0x82559fa8 | sub_82559FA8 | render_pass_flush_pending_shader_state_bits | 0.87 | Walks 64-bit bitsets at pass+320/+384; for each set bit invokes vtable dispatch from unk_829925D8/8299288A tables; clears pass+516/+520 slots. Sort-keys / material setup paths. |
| 0x82565318 | sub_82565318 | render_object_pass_match_body_draw_category | 0.90 | Caches at +376: prefix match interior/seat/steering_wheel on material name (+4). Simpler subset of render_object_pass_match_interior_draw_category. Emit-draws path. |
| 0x825aae90 | sub_825AAE90 | render_object_pass_apply_material_constant_bindings | 0.89 | Finds material group matching shader slot a3; iterates 20-byte binding records; dispatches FM2_Render_WritePassConstantSlot / FM2_ShaderConstant_SetVectorById / material slot helpers by type field. |
| 0x825680c8 | sub_825680C8 | render_visibility_pass_execute_object_draw | 0.86 | Orchestrates visibility draw: depth/stencil/EDRAM setup, RenderPassResource lock, shared helpers, deferred task init, sub_82566BF0 variant select, sort keys. Visibility VMX core callee. |
| 0x825afb68 | sub_825AFB68 | audio_render_setup_downsample16x_pass | 0.88 | Looks up shader "DownSample16X" via render_pass_lookup_shader_name_slot; builds render target textures; calls sub_825AF3C0. FM2_AudioRenderFrame_PathB. |
| 0x827fc900 | sub_827FC900 | stl_rb_tree_rebalance_on_insert_left | 0.85 | RB-tree left rotation/relink using FM2_STL_ListNode_* helpers; updates root/sentinel links and parent pointers. FM2_STL_Map_InsertWithRebalance. |
| 0x827fcaf8 | sub_827FCAF8 | stl_rb_tree_rebalance_on_insert_right | 0.85 | Mirror right-rotation/relink for map insert rebalance. Paired with left variant. |
| 0x82603e58 | sub_82603E58 | render_set_texture_stage_filter_states | 0.87 | Via device vtable +32 sets sampler/state slots 108/124/128/132/136/140 to on/off presets based on a3 flag. FM2_MemSetHelper_2 paths. |
| 0x82566bf0 | sub_82566BF0 | render_object_pass_select_shader_variant_name | 0.90 | Appends "_packed", selects dropShadow/tire/shadowMap/tireShadowMap/rimShadowMap variants using material flags and strstr blur; calls visibility helpers. Many shader name strings in body. |
| 0x82559718 | sub_82559718 | render_pass_upload_vmx_constant_blocks | 0.86 | VMX128: splat -1, dcbz128/stvx loops upload constant blocks from pass+256/+288 regions into pass state; validates pass+540 binding. Many render/material callers. |
| 0x8255d688 | sub_8255D688 | render_object_pass_is_badge_or_emblem_material | 0.91 | When byte+54 has 0x80, tests material name for "badge" or "emblem" via strstr; caches bool in +54. Shader variant select path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82747c10 | sub_82747C10 | 4.4KB sparse constant-register applicator; truncated decompile—defer until leaf merge pattern fully mapped. |
| 0x825aa988 | sub_825AA988 | 1.2KB material table builder; truncated mid-loop, only partial "_packed" string evidence. |
| 0x82564f78 | sub_82564F78 | Simple 72-byte buffer free/clear; low standalone naming value (deferred_task buffer reset). |
