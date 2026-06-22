
## Iteration 43

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82570BD8 | sub_82570BD8 | render_post_process_pass_dtor_with_free | 0.91 | vtable off_820485C4; FM2_Object_AssignBaseVtable_82000E18; optional FM2_Memory_FreeSmallBlockOrNull. Pair to render_post_process_pass_init_defaults / off_820485C4 acquire path. |
| 0x82571068 | sub_82571068 | render_post_process_subobject_dtor_with_free | 0.91 | vtable off_820486FC; render_com_object_release_child_and_reset_vtable; optional heap free. |
| 0x82570EE0 | sub_82570EE0 | render_post_process_car_slot_transform_init | 0.88 | FM2_Presentation_InitCarSlotTransformZeros at +8; vtable off_820486FC; clears +32/+36. Pool-40 ctor from sub_82571118. |
| 0x82571370 | sub_82571370 | render_post_process_material_init_defaults | 0.90 | vtable off_82048774; FM2_CritSec_InitAndZeroOwner on locks +4/+36/+68/+100; render_post_process_material_tonemap_weights_init at +144; clears +1104/+1108. |
| 0x82571358 | sub_82571358 | render_post_process_material_get_tonemap_params_ptr | 0.88 | Returns material+144. Caller sub_8222A6B8 after FM2_Render_NotifyManagerStateChange passes result to sub_82355E28 for tonemap param setup. |
| 0x82571650 | sub_82571650 | render_post_process_material_acquire_and_assign | 0.89 | FM2_AllocPoolAcquireOrInit_Thunk(1136); render_post_process_material_init_defaults; vtable off_820487B0; ComPtr assign at a1+20; back-pointer at acquired+1108. |
| 0x825713E0 | sub_825713E0 | render_post_process_material_core_dtor_with_free | 0.90 | render_post_process_material_dtor without pre-setting derived vtable; optional free. Distinct from off_820487B0 deleting dtor at 82571538. |
| 0x825793C0 | sub_825793C0 | render_post_process_material_tonemap_weights_init | 0.89 | Sets RGB weight triplets 0.2/0.2/0.2, 0.1/0.1/0.1, 1.0/1.0/1.0 at +432..+472; clears +916; calls render_post_process_material_tonemap_defaults_init. |
| 0x82578C10 | sub_82578C10 | render_post_process_material_tonemap_defaults_init | 0.91 | Initializes full tonemap block: Rec.709 luma weights 0.2125/0.7154/0.0721 at +48/+52/+56; exposure 0.6/0.4/0.01; bloom/threshold constants; mode +192=3. Used by material init and deferred-command path. |
| 0x82570D78 | sub_82570D78 | render_notify_manager_find_or_dispatch_entry | 0.87 | Scans notify-manager vector at +52 for matching *a2 id; returns existing entry or AddRef-dispatches new entry vtable+8 and returns 0. Called from pass assign path. |
| 0x825711C8 | sub_825711C8 | render_post_process_pass_assign_notify_manager_entry | 0.86 | If notify ptr null releases +32 COM; else FM2_Render_NotifyManagerStateChange + render_notify_manager_find_or_dispatch_entry; assigns result ComPtr at +32. |
| 0x825729B8 | sub_825729B8 | render_post_process_effect_acquire_and_assign | 0.88 | Pool alloc 144; render_post_process_effect_descriptor_init; ComPtr assign at a1+64 via sub_825DA040; returns render_ring_buffer_get_current_index. Mirror of pass acquire pattern. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82570B68 | sub_82570B68 | Atomic refcount dec at +192; no xrefs. |
| 0x825710C0 | sub_825710C0 | Thin deleting dtor off_82048754 only. |
| 0x82571118 | sub_82571118 | Thin pool-alloc wrapper around car_slot_transform_init. |
| 0x82571160 | sub_82571160 | Thin ComPtr assign off_82048754 stub object. |
| 0x825714C8 | sub_825714C8 | Atomic refcount dec at +1120; no callers. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
