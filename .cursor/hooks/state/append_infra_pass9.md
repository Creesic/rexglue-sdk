### Infrastructure pass 9 (33 functions)

Callee list **1148** remaining. Hash-name/property-bag cluster, render pass core, livery-mask, FMOD/SQLite helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8221c5d8` | `FM2_PropertyBag_InitListSentinel` | Init property-bag intrusive-list sentinel node. |
| `0x82220030` | `FM2_PropertyBagList_DestroyAndFree` | Destroy property-bag list nodes and free backing block. |
| `0x82231648` | `FM2_WString_AssignSubrangeFromSource` | Wide-string assign/copy subrange from source SSO/heap buffer. |
| `0x82331e78` | `FM2_HashName_FindPropertyNodeByKey` | Find hash-name RB-tree node matching 32-byte property key. |
| `0x8245a580` | `FM2_HashName_AssignPropertyByTypeId` | Assign hash-name property value by type id (string/wstring/list/etc.). |
| `0x8245b108` | `FM2_PropertyBag_InitRbTreeHeader` | Init property-bag red-black tree header links. |
| `0x8245b2c0` | `FM2_PropertyBag_CtorFromHashNameNode` | Construct `CPropertyBag` from hash-name lookup node. |
| `0x8253faf8` | `FM2_HashNamePropertyList_DestroyAndFree` | Destroy hash-name property list and free sentinel block. |
| `0x82204a28` | `FM2_Stl_String_FindDelimiterIndex` | Find index of delimiter char in STL string (for `a.b` paths). |
| `0x8245c4d8` | `FM2_HashName_NormalizeKeyString` | Normalize hash-name key string (case/char transform loop). |
| `0x8245f1f0` | `FM2_HashName_LookupValueByKeyInTable` | Lookup hash-name table value dword by key in RB-tree. |
| `0x82363bf8` | `FM2_AudioDevice_SetDeferredFreeFlag` | Set audio-device deferred-free flag; enqueue free when enabled. |
| `0x8236dbc8` | `FM2_Render_SetPassShaderFlagsFromArray` | OR shader/pass flag bits from bool array into render pass state. |
| `0x82578970` | `FM2_AudioManager_InitDefaultMixParameters` | Initialize default audio manager mix/fade timing parameters. |
| `0x82555d38` | `FM2_Render_SetupPassMaterialConstants` | Setup render pass material constants (VMX vector splats). |
| `0x82514f90` | `FM2_Render_ObjectPassSortAndEmitDraws` | Sort/object-pass draw emit helper for render setup inner path. |
| `0x82725560` | `FM2_Render_ApplyPassLightingCore` | Core pass-lighting transform application (matrix/vector math). |
| `0x826188c8` | `FM2_Memory_InsertFrameAllocMapEntry` | Insert frame alloc map entry keyed by frame counter. |
| `0x82366af8` | `FM2_Memory_ScheduleDeferredFreeForBlock` | Schedule memory block for deferred free under global critsec. |
| `0x82522f18` | `FM2_AllocPoolAcquire12xCount` | Pool alloc `12 * count` bytes with overflow guard. |
| `0x82657338` | `FM2_CircularBuffer_EraseRange` | Erase subrange from circular/intrusive buffer vector. |
| `0x826719d8` | `FM2_FMOD_HeapAllocFromPoolLocked` | FMOD heap alloc from pool under critsec (optional header skip). |
| `0x82429d08` | `FM2_SQLite_Vdbe_GetProgramCounter` | Returns SQLite Vdbe program counter at `this+16`. |
| `0x824f4138` | `FM2_SQLite_Vfs_AddRef` | SQLite VFS addref via vtable +8. |
| `0x82418560` | `FM2_Image_DecodePngFromMemory` | Decode PNG image bytes from memory buffer (zlib/ihdr path). |
| `0x8258ed20` | `FM2_InstalledParts_CtorDefaults` | Construct `CInstalledParts` with -1 filled defaults. |
| `0x825a10d8` | `FM2_LiveryMask_CreateInterfaceFromPath` | Create livery-mask COM interface from file path params. |
| `0x825a00b0` | `FM2_LiveryMask_InitCreateParams` | Init livery-mask creation parameter struct defaults. |
| `0x825a0ea8` | `FM2_LiveryMask_AllocAndInitMaskObject` | Allocate 240-byte livery-mask object and init from params. |
| `0x824a7278` | `FM2_ComObject_GetRefCountVtablePtr` | Returns COM ref-count vtable pointer `off_8299B824`. |
| `0x824a7688` | `FM2_ComObject_GetStaticLifetimeBlock` | Returns static COM lifetime block `unk_829F2EC8`. |
| `0x824cc0f8` | `FM2_CameraScript_DecRefAndUnloadIfLast` | Decref camera script module; unload when last ref. |
| `0x824fb108` | `FM2_DeferredTaskHolder_Dtor` | Deferred-task holder dtor: invoke callback then reset vtable. |