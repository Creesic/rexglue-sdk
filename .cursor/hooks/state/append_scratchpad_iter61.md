
## Iteration 61

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82579580 | sub_82579580 | render_notify_manager_swap_com_ptr_and_invoke_vtable32 | 0.90 | FM2_ComPtr_ResetAndAssign; dispatches held object vtable+32; render_notify_manager_vector_push_state_change; Release on out ComPtr. Notify manager vtable 0x82191DC0. |
| 0x825795F0 | sub_825795F0 | render_car_driver_load_shader_resources_and_decl | 0.92 | FM2_Render_LoadVertexShaderResourceById slots 6/7; FM2_Render_LoadPixelShaderResourceById slot 6; shader_resource_load_static_decl_by_name "CarDriverDecl". Vtable 0x82191DC8. |
| 0x82579708 | sub_82579708 | render_pass_lighting_transform_store_vmx128_and_bind | 0.90 | Slot 1..12: FM2_MemcpyAligned 64-byte slot; FM2_Render_TransformVec4x4VMX128; stores four VMX128 rows at object+740; FM2_Render_UpdatePassLightingSlotFields; FM2_Render_BindPassLightingResourcePair. Vtable 0x82191DD0. |
| 0x825797E8 | sub_825797E8 | render_pass_lighting_store_incoming_vmx128_slot_and_bind | 0.89 | Slot 1..12: writes four incoming VMX128 rows to object+744+slot*64; FM2_Render_BindPassLightingResourcePair. Vtable 0x82191DD8. |
| 0x82579878 | sub_82579878 | render_pass_lighting_blend_slot_lvlx_vectors | 0.89 | FM2_Render_ComputePassLightingSlotOffset64B for slots 4/5 or 7/8; lvlx/vrlimi128 blend of slot vector components. Called from compute_blended_slot path. |
| 0x825799D8 | sub_825799D8 | render_pass_lighting_compute_blended_slot_vmx128_row | 0.88 | Calls blend_slot_lvlx_vectors; subtracts from slot row; vmaddfp recombination; stvx result row to output. Vtable 0x82191DE8. |
| 0x82579AF8 | sub_82579AF8 | render_pass_lighting_copy_vmx128_triple_to_object | 0.88 | lvx128/stvx copies three VMX128 rows into object fields at +16/+48. Caller 0x8251DC34. |
| 0x82579B60 | sub_82579B60 | render_pass_lighting_set_object_float_at_84 | 0.90 | Stores float param at *(*result)+84. Caller 0x8251EE30. |
| 0x8257A3B0 | sub_8257A3B0 | render_pass_lighting_init_pass_context_vmux128_defaults | 0.88 | Sets pass id fields at +0/+4; when flag at +416 set, builds default VMX128 rows into context at +1584..+1744 from slot-0 lighting data. Vtable 0x82191E00. |
| 0x8257A610 | sub_8257A610 | render_pass_lighting_is_pass_ready | 0.89 | Validates non-null object chain (*a1, (*a1)+4, a1[1]); requires bits at a1[40] and a1[104] set via _cntlzw checks. |
| 0x825DA040 | sub_825DA040 | render_notify_manager_vector_push_state_change | 0.89 | Appends FM2_Render_NotifyManagerStateChange result into notify vector at result+4 (grow or in-place); called from render_notify_manager_swap_com_ptr_and_invoke_vtable32. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x82579BF0 | sub_82579BF0 | Large pass-lighting orchestrator; defer until callee subgraph clearer. |
| 0x82579EF8 | sub_82579EF8 | Large pass-lighting setup; defer to iter 62. |
| 0x8257A678 | sub_8257A678 | Complex FP/rand path; insufficient isolated behavior name. |
