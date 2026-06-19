### Infrastructure pass 11 (33 functions)

STL/render adapter, buf-file, config entries, hash table helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821d0350` | `FM2_Stl_CompareDwordLess` | STL comparator: `*a < *b` for dword pointers. |
| `0x821d0368` | `FM2_Stl_SprintfToBuffer` | Vararg sprintf into stack buffer. |
| `0x821d03d0` | `FM2_Stl_SwapChars` | Swap two char values in place. |
| `0x821d0cd0` | `FM2_ResourceLock_AssignAndProbeInterface` | Assign resource lock handle; probe COM interface thread-safe. |
| `0x821d3ed8` | `FM2_ComObject_ReleaseViaVtable16` | COM release via vtable offset +16. |
| `0x821d7400` | `FM2_Render_MatrixMultiplyVMX128` | VMX128 matrix/vector multiply for render lighting. |
| `0x821e1af0` | `FM2_BufFile32768_Ctor` | Construct 32KiB buffered file object. |
| `0x821e1f08` | `FM2_BufFile32768_Dtor` | Destroy 32KiB buffered file; free buffer. |
| `0x821e9cb0` | `FM2_SceneObject_DtorReleaseRefs` | Scene object dtor: release multiple COM/ref fields. |
| `0x821ea5c8` | `FM2_Stl_String_StripTrailingPath` | Strip trailing path segment from string iterator. |
| `0x821ef8c8` | `FM2_ContentEntry_CopyAssign` | Copy-assign content DB entry incl. intrusive lists. |
| `0x821f1798` | `FM2_Render_InitLightingConstantsVMX` | Init render lighting VMX constant splats. |
| `0x821f1dd0` | `FM2_RenderAdapter_UpdateFrameStats` | Render adapter: update per-frame timing stats. |
| `0x821f2150` | `FM2_RenderAdapter_PollPresentThrottle` | Throttle present polling when interval exceeded. |
| `0x821f24a8` | `FM2_RenderAdapter_QueueFrameTimingUpdate` | Queue render-adapter frame timing update. |
| `0x821f9618` | `FM2_IntVector_GetIteratorEndPtr` | Advance int-vector iterator to end pointer. |
| `0x821f97b0` | `FM2_IntVector_EraseOneShift` | Erase one int-vector element with shift. |
| `0x821ff8f0` | `FM2_BufFile_WriteCString` | Write C string into buffered file stream. |
| `0x822030c8` | `FM2_HashTable_FindEntryByKey` | Hash table lookup walk by key hash. |
| `0x82203d10` | `FM2_HashTableList_DestroyAndFree` | Destroy hash-table list nodes and free block. |
| `0x82204338` | `FM2_Crt_StaticInit_ScriptBindingTable_829C2410` | CRT static init script-binding table + atexit. |
| `0x82204750` | `FM2_D3D_ReleaseFrameCounterCritSec` | Record D3D frame counter and leave critsec. |
| `0x82204790` | `FM2_D3D_EndFrameDeferredCleanup` | End-of-frame D3D deferred cleanup when flagged. |
| `0x82204860` | `FM2_AsyncOp_IncrementRefAndWait` | Interlocked inc ref; wait on handle if negative. |
| `0x82204f68` | `FM2_ConfigEntry_CopyAssignPartial` | Partial copy-assign config entry (string + flags). |
| `0x82205188` | `FM2_ConfigEntry_CopyAssign` | Full copy-assign 36-byte config entry. |
| `0x82205e40` | `FM2_ConfigEntryVector_DestroyRange` | Destroy range of config entries (clear strings). |
| `0x822061b0` | `FM2_ConfigEntryVector_MoveConstructRange` | Move-construct config entry vector subrange. |
| `0x82453d80` | `FM2_NetworkMessage_AllocRbTreeNode` | Allocate network-message RB-tree node. |
| `0x825c6700` | `FM2_TuningDb_AllocListNode` | Allocate tuning-db list node. |
| `0x8242a350` | `FM2_TuningDb_AllocFloatListNode` | Allocate tuning-db float list node. |
| `0x82527ae0` | `FM2_IntVector_AdvanceIterator` | Advance int-vector iterator by N slots. |
| `0x821f8330` | `FM2_IntVector_ShiftEraseOne` | Shift-erase one element from int vector. |