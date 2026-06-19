### Infrastructure pass 4 (33 functions)

Lua binding-record module, image resample kernels, list/wstring helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824ec650` | `FM2_Lua_RegisterModuleBindings` | Iterates 104-byte binding records; registers module table + `_LOADED`. |
| `0x824ec400` | `FM2_Lua_PushBindingRecordToTable` | Pushes one binding record (name, func, pair vectors) into Lua table. |
| `0x824ec210` | `FM2_LuaBindingRecord_CopyAssign` | Copy-assign 104-byte Lua binding record incl. four pair vectors. |
| `0x824ec2a0` | `FM2_LuaBindingRecord_InitFromCStr` | Construct binding record from C string name + property id. |
| `0x824ec788` | `FM2_LuaBindingRecord_VectorShiftInsertCopies` | Vector insert: backward-copy 104-byte records to make room. |
| `0x824ec7e0` | `FM2_LuaBindingRecord_VectorConstructRange` | Construct N copies of binding record via `FM2_SceneNode_CopyAssignExtended`. |
| `0x824ecbb0` | `FM2_LuaBindingRecord_VectorReserveAndReset` | Reallocate binding-record vector; reset begin/end iterators. |
| `0x824ecc90` | `FM2_LuaBindingRecord_VectorEmplaceBack` | Emplace binding record at vector end (copy or grow). |
| `0x824ecd50` | `FM2_LuaBindingVector_EmplaceQualifiedName` | Build `scope::name` qualified binding and emplace into vector. |
| `0x824ec890` | `FM2_LuaBindingRecord_VectorInsert` | Full vector insert with reallocate/shift of 104-byte records. |
| `0x824ebf78` | `FM2_Lua_BindingPairVector_Assign` | Assign/copy Lua binding pair vector `{key,func}` pairs. |
| `0x8254e4f0` | `FM2_Lua_RegisterBindingPairsInModuleTable` | Register `{name,func}` pairs into module `_LOADED` table. |
| `0x824ead78` | `FM2_Lua_PushMetatableWithGcAndProps` | Push metatable with `__gc`, optional `__getprop`/`__setprop`. |
| `0x822a2ed8` | `FM2_LuaBindingRecord_Dtor` | Dtor: free four pair vectors + SSO string in binding record. |
| `0x82466aa0` | `FM2_LuaBindingRecord_GetPropertyFlags` | Returns property-flag dword at binding-record offset +96. |
| `0x824b7750` | `FM2_Lua_SetTableFieldFromStack` | Set table field from two stack slots (metatable assignment). |
| `0x824b69b0` | `FM2_Lua_CopyStackSlotToTop` | Copy stack slot value to stack top. |
| `0x824b7210` | `FM2_Lua_PushLightUserdataWithArgs` | Push light userdata/C closure capturing N stack args. |
| `0x824b7a88` | `FM2_Lua_ProtectedCallWithTraceback` | Protected call with traceback hook (`sub_824B9780`). |
| `0x8254de38` | `FM2_Lua_LoadFileFromCStringPath` | Load Lua chunk from C string path via reader callback. |
| `0x824ece68` | `FM2_LuaIo_GetFileHandleReadable` | Lua IO: get readable file handle (op 2). |
| `0x824ece98` | `FM2_LuaIo_TryOpenFileForLoad` | Lua IO: try open file for load if not already loading. |
| `0x824ecef0` | `FM2_LuaIo_SetFileModeBits` | Lua IO: set file mode bits (ops 6/7). |
| `0x824ecf50` | `FM2_LuaIo_IsFileWritable` | Lua IO: query writable flag (op 5). |
| `0x824ecf90` | `FM2_LuaIo_GetFileSizeFlags` | Lua IO: combine size flag bits from ops 3/4. |
| `0x823928d8` | `FM2_Image_ResampleKernel_ApplyTexelTransform` | Bilinear resample texel transform by source/dest format modes. |
| `0x82393c38` | `FM2_Image_ResampleKernel_AccumulateAndClearRow` | Resample row accumulate from temp buffer then zero scratch. |
| `0x823d7460` | `FM2_Image_ResampleKernel_ApplySqrtOrGammaTable` | Resample apply sqrt/gamma LUT (`flt_820303F8/FC`) on texels. |
| `0x82231f10` | `FM2_WString_AssignFromWideCStr` | Wide-string assign from `wchar_t*` via internal append. |
| `0x82257f18` | `FM2_IntrusiveList_ClearAndDestroyNodes` | Clear intrusive list: dtor each node then free block. |
| `0x821f8250` | `FM2_IntrusiveList_ClearAndFreeEntries` | Clear intrusive list: free nodes without per-node dtor. |
| `0x821e6a08` | `FM2_SceneNode_GetExtendedPayloadOffset` | Returns scene-node extended payload at `this+10016`. |
| `0x821fc2b0` | `FM2_ComObject_ReleaseAndOptionalFree` | Release COM object at +8; optional `FM2_Memory_FreeSmallBlockOrNull`. |