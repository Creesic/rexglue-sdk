## Iteration 209

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826209E8 | sub_826209E8 | FM2_Lua_InitBindingSlotWithBoolValue | 0.91 | Binding-slot ctor type `1`; `FM2_Lua_PushBool`; records stack depth; pairs with nil-slot init family. |
| 0x82620A48 | sub_82620A48 | FM2_Lua_InitBindingSlotWithCharNumberValue | 0.90 | Type tag `1`; pushes char as Lua number; stack depth at `+8`; deferred from iter 208. |
| 0x82620AB0 | sub_82620AB0 | FM2_Lua_InitBindingSlotWithInt16NumberValue | 0.90 | Type tag `1`; pushes int16 as number; same binding-slot layout as bool/char variants. |
| 0x82620B18 | sub_82620B18 | FM2_Lua_InitBindingSlotWithUint32AsInt64Value | 0.91 | Type tag `3`; pushes `(uint32 \| 0x300000000)` as double; 4 event-binding callers. |
| 0x82620CB8 | sub_82620CB8 | FM2_Lua_InitBindingSlotWithDoubleValue | 0.91 | Type tag `3`; `FM2_Lua_PushNumber` with native double; 4 binding callers. |
| 0x82620D10 | sub_82620D10 | FM2_Lua_InitBindingSlotWithCStringValue | 0.91 | Type tag `4`; `FM2_Lua_PushLStringOrNil`; records stack depth; 5 binding callers. |
| 0x826205F8 | sub_826205F8 | FM2_Lua_InitScriptLoaderFromCopiedBindingSlot | 0.90 | `FM2_Lua_CopyBindingSlotFromSource` + grow 4-slot variant vector + clear loader string/callback fields; 8 callers. |
| 0x827EAB38 | sub_827EAB38 | FM2_Render_ReleaseMaterialPassVariantRecord | 0.90 | Frees string payload for type `14`; clears variant header fields; callee of pass-flag clear path. |
| 0x827DD170 | sub_827DD170 | FM2_Render_LinkUnitStringSubtreeBySlotSelector | 0.89 | Selects `a1[130/131/132]` root by `*a4` index 0/1/2; calls `sub_827EF3D0`; 8 material-tree callers. |
| 0x827EF3D0 | sub_827EF3D0 | FM2_Render_LinkUnitStringTableNodeChain | 0.88 | `FM2_Lua_LookupOrCreateUnitStringTableSlot`; rewires child id; removes dup ref via `sub_827F5F58`; appends via `sub_827DEAB8`. |
| 0x827F5F58 | sub_827F5F58 | FM2_Render_RemoveMatchingUnitStringChildRef | 0.89 | Scans child dword vector at node `+12/+16`; pops back on id match via `FM2_Stl_DwordVector_PopBackAndShrink`. |
| 0x827DE0A0 | sub_827DE0A0 | FM2_Render_FlushPendingUnitStringLinkQueue | 0.88 | Walks pending link queue at `a1+45`; if `sub_827E4A08` ready calls `sub_827E5218`; erases processed range. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82611DC0 | sub_82611DC0 | Generic 16-byte vector grow/append; low subsystem specificity. |
| 0x82620B80 | sub_82620B80 | Near-duplicate uint8-number binding slot; covered by int16/char variants. |
| 0x82620BE8 | sub_82620BE8 | Near-duplicate uint16-number binding slot. |
| 0x82620C50 | sub_82620C50 | Duplicate of uint32-as-int64 binding slot (`sub_82620B18`). |
| 0x827E5218 | sub_827E5218 | Thin two-call wrapper (`sub_827E4EB8` + destroy). |
| 0x827E4EB8 | sub_827E4EB8 | Profile callback fanout under critsec; defer dedicated naming. |
| 0x827E47F0 | sub_827E47F0 | 32B vtable destroy thunk only. |
| 0x827DEAB8 | sub_827DEAB8 | 8B `Vector_PushBack32` wrapper. |
| 0x827E4A08 | sub_827E4A08 | Thin XML navigator readiness probe. |
| 0x824F0168 | sub_824F0168 | 8B offset helper `a1+16`. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
