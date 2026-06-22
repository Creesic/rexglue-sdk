## Iteration 62

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257B2B0 | sub_8257B2B0 | render_pass_lighting_init_pass_context_thunk | 0.93 | Thin wrapper dereferencing `*a1` then calling `render_pass_lighting_init_pass_context_vmux128_defaults`. |
| 0x8257B2B8 | sub_8257B2B8 | render_pass_lighting_apply_randomization_thunk | 0.92 | Thin wrapper: `return sub_8257A678(*a1, a2)`; vtable slot 0x82191E08 points at randomization body. |
| 0x8257B2C0 | sub_8257B2C0 | render_r1_asset_resource_lock_release | 0.91 | Sets `TResourceLock<CR1AssetResource>` vftable; `FM2_ResourceLock_ClearAndReleaseHandle`; Release on held object. Vtable 0x82191E10. |
| 0x8257B318 | sub_8257B318 | render_skinned_model_resource_lock_free_small_pools | 0.90 | `SetInterfaceThreadSafe(a1+104,0)`; frees small blocks at offsets 184/185/186 via `FM2_Memory_FreeSmallBlock`. Vtable 0x82191E18. |
| 0x8257B398 | sub_8257B398 | render_skinned_model_resource_lock_init | 0.91 | Critsec init; `FM2_Render_InitSkinnedModelResourceLockBody`; same TResourceLock vftable. Vtable 0x82191E20. |
| 0x8257B3F0 | sub_8257B3F0 | render_environment_binding_context_init | 0.90 | Zero object; VMX128 identity rows at +16/+32/+48/+64; critsec at +160; three `FM2_Render_InitEnvironmentBindingContext` at +192/+224/+256. Vtable 0x82191E28. |
| 0x8257B588 | sub_8257B588 | render_environment_binding_context_alloc | 0.92 | `FM2_AllocPoolAcquireOrInit_Thunk(1808)` then `render_environment_binding_context_init`; stores pointer in out-param. Vtable 0x82191E30. |
| 0x8257B5D8 | sub_8257B5D8 | render_environment_binding_context_assign_resource_handle | 0.91 | Forwards to `FM2_ResourceLock_AssignRetainedHandle` at `(*a1)+256`. |
| 0x8257B5E8 | sub_8257B5E8 | render_environment_binding_context_release | 0.90 | Calls `render_skinned_model_resource_lock_free_small_pools`; Release on nine ComPtr fields at +40..+104. Vtable 0x82191E38. |
| 0x8257BAC0 | sub_8257BAC0 | render_environment_binding_context_destroy | 0.91 | Calls release on `*result`, `FM2_Memory_FreeSmallBlockOrNull`, nulls out pointer. Vtable 0x82191E48. |
| 0x8257B710 | sub_8257B710 | render_pass_lighting_dispatch_skinned_model_draw | 0.88 | Gate on byte+80 and draw flags; `render_pass_lighting_is_pass_ready`; `FM2_Render_InitSkinnedModelResourceLock`; matrix upload; material vtable +76/+68/+100/+116/+128; `FM2_Render_DestroySkinnedModelResourceLock`. Vtable 0x82191E40. |
| 0x8257BAA8 | sub_8257BAA8 | render_pass_lighting_dispatch_skinned_model_draw_thunk | 0.93 | Thin wrapper: `return render_pass_lighting_dispatch_skinned_model_draw(*a1, a2, a3)`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x82579BF0 | sub_82579BF0 | Large pass-lighting orchestrator (vector normalize, sub_82727E80 slot fetch, triple TransformVec4x4, store_incoming); defer deeper read. |
| 0x82579EF8 | sub_82579EF8 | Large dual-slot VMX128 blend path; calls compute_blended_slot + sub_82727E80; defer to iter 63. |
| 0x8257A678 | sub_8257A678 | Complex FP/rand/transform orchestrator (RandUnitFloat, ClearPassLightingSlotVMX, transform_store); vtable 0x82191E08 body. |
| 0x8257BB18 | sub_8257BB18 | Skinned-model worker setup from STL string; needs full callee chain before naming. |
| 0x8257BC88 | sub_8257BC88 | Car-driver shader/livery/decl init orchestrator; defer to iter 63. |
