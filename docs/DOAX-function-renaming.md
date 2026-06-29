# DOAX Function Renaming Log

IDA database: `ida37`

This file records function names applied in IDA and the evidence used for each
name. Keep entries tied to observed disassembly/decompiler behavior, not just
desired hook names.

## 2026-06-28

| Address | Old name | New name | Reason |
| --- | --- | --- | --- |
| `0x824C1208` | `sub_824C1208` | `DOAX_MenuTransitionPlayMovie` | Clears/wakes transition work slots, calls `DOAX_MenuTransitionOverlaySetup(30)`, selects a movie index from the handler byte/table, then calls `DOAX_PlayMovie` when the index is below 6. This is the menu-transition movie start handler and matches the archived hook name. |
| `0x824C12D0` | `sub_824C12D0` | `DOAX_MenuTransitionMoviePoll` | Calls `DOAX_IsMovieFinished()` and, when true, writes `1` to the handler done byte at `a1 + 2`. This is the paired movie completion poll for the menu-transition handler. |
| `0x824195C8` | `sub_824195C8` | `DOAX_MenuTransitionTimeline` | Validates transition slot index `< 8`, reads the 4144-byte animation slot under `*(a1 + 928)`, requires slot state `3`, scans the command stream, and accumulates opcode-24 durations into the returned frame/timeline value. |
| `0x82782BF0` | `sub_82782BF0` | `DOAX_XamInputGetStateWrapper` | Thin wrapper around `XamInputGetState(a1, 0, a2)` with the flags argument fixed to zero. Keeping "wrapper" in the name avoids overstating its role. |
| `0x8274B650` | `sub_8274B650` | `DOAX_ControllerPollUpdate` | Iterates four controller slots, calls `DOAX_XamInputGetStateWrapper`, tracks connect/disconnect edges, refreshes capabilities, copies raw `XINPUT_STATE`, normalizes sticks/triggers, and builds per-controller action/edge masks. |
| `0x8250BB60` | `sub_8250BB60` | `DOAX_WarningScreenUpdate` | Checks `XGetVideoMode`, warning/scheduler state bytes, and controller action bits `0x100`/`0x10`; on confirm it draws the warning/Press Start sprite path and queues handler `18` via `sub_824C1350(18, 0, 15)`. This matches the warning-screen update role documented in the skip notes. |

### IDA37 continuation batch - 200 renames

| Address | Old name | New name | Reason |
| --- | --- | --- | --- |
| `0x8250AC08` | `sub_8250AC08` | `DOAX_BootEnterWarningMode` | Calls the boot warning setup path and advances the boot movie state to warning mode when profile state allows it. |
| `0x8250AC50` | `sub_8250AC50` | `DOAX_BootWarningDismiss` | Calls warning-screen update, wakes the scheduler, and sets the next present target once the warning state reaches completion. |
| `0x8250AC98` | `sub_8250AC98` | `DOAX_BootWarningFrame` | Clears a boot flag, schedules the worker fiber loop, and clears the active-user byte when the profile is invalid. |
| `0x824C1310` | `sub_824C1310` | `DOAX_PostPromotionCleanup` | Marks promotion cleanup state and queues the follow-up transition after promotion movie playback. |
| `0x82768270` | `sub_82768270` | `DOAX_FM2EventDispatch` | Walks the subscriber ring, filters event codes, and invokes registered callbacks. |
| `0x8266E618` | `sub_8266E618` | `DOAX_MenuSpritePumpTick` | Top-level menu sprite pump gated by `byte_839472D8`; it calls each pump slot and services the deferred action queue. |
| `0x82670AE8` | `sub_82670AE8` | `DOAX_MenuSelectionSpritePumpSlot` | State machine over 16-byte selection sprite slots that loads selection resources and publishes the scene id. |
| `0x82670E10` | `sub_82670E10` | `DOAX_RequestMenuSelectionSprite` | Chooses one of two selection slots, releases old resources, records the requested id, and marks the pump dirty. |
| `0x82671110` | `sub_82671110` | `DOAX_ClearMenuSelectionSpriteSlot` | Clears a selection sprite slot, yields while unwinding an active sprite object, and releases both resource owners. |
| `0x82669FF8` | `sub_82669FF8` | `DOAX_QueueMenuSpriteLoad` | Enqueues an async menu sprite/resource load in the 128-slot queue while avoiding duplicate asset and owner pairs. |
| `0x8266A128` | `sub_8266A128` | `DOAX_ReleaseMenuSpriteLoadsByOwner` | Scans pending load records by owner handle and clears matching entries. |
| `0x8266EF00` | `sub_8266EF00` | `DOAX_ClearDeferredMenuSpriteAction` | Clears a deferred menu sprite action slot when its state is pending or done. |
| `0x8266EF50` | `sub_8266EF50` | `DOAX_IsMenuSpriteActionPendingForOwner` | Scans the deferred action queue for an owner handle, reports pending state, and clears completed states. |
| `0x8266EE38` | `sub_8266EE38` | `DOAX_AllocDeferredMenuSpriteAction` | Allocates a free deferred action slot with an object pointer and owner handle. |
| `0x8266EEA8` | `sub_8266EEA8` | `DOAX_ConsumeDeferredMenuSpriteActionDone` | Consumes completion state for a deferred action slot or reports no pending action. |
| `0x8266ADC0` | `sub_8266ADC0` | `DOAX_DestroyMenuSpriteObject` | Destroys a sprite object, frees optional payload, finalizes the embedded object, then frees the holder. |
| `0x8266A3B8` | `sub_8266A3B8` | `DOAX_ReleaseMenuSpriteResourcesByOwner` | Scans loaded menu sprite resource records by owner handle and releases matching records. |
| `0x8266A208` | `sub_8266A208` | `DOAX_FindMenuSpriteAssetIdByOwner` | Scans the loaded resource table for an owner and returns its asset id. |
| `0x8266A258` | `sub_8266A258` | `DOAX_FindOrFallbackMenuSpriteAssetId` | Uses the loaded owner-to-asset lookup first, then falls back to the pending load queue. |
| `0x8266A2D0` | `sub_8266A2D0` | `DOAX_SortMenuSpriteLoadQueue` | Insertion-sorts the 128 menu sprite load records by the priority byte. |
| `0x8266BCD8` | `sub_8266BCD8` | `DOAX_MenuBannerSpritePump` | Banner sprite state machine that queues and waits for resources from the banner resource table. |
| `0x8266BFB0` | `sub_8266BFB0` | `DOAX_MenuOptionMainSpritePump` | Multi-stage resource swapper over the main option sprite slot table. |
| `0x8266CE40` | `sub_8266CE40` | `DOAX_MenuOptionAuxSpritePump` | State machine over aux option sprite slots that queues aux option resources. |
| `0x8266D760` | `sub_8266D760` | `DOAX_MenuOptionIconSpritePump` | State machine over icon sprite slots that maps icon ids through the option icon table. |
| `0x8266E068` | `sub_8266E068` | `DOAX_MenuSpriteReadyFlagPump` | Single ready-flag sprite state machine that queues, waits, then publishes the ready flag object. |
| `0x82A4A640` | `sub_82A4A640` | `DOAX_RegisterNullAtexit3` | The function only calls `rex_atexit(nullsub_3)`. |
| `0x82A55220` | `sub_82A55220` | `DOAX_RegisterNullAtexit4` | The function only calls `rex_atexit(nullsub_4)`. |
| `0x82A59B00` | `sub_82A59B00` | `DOAX_RegisterNullAtexit5` | The function only calls `rex_atexit(nullsub_5)`. |
| `0x82A54F70` | `sub_82A54F70` | `DOAX_InitE49EVariantRecordTable` | Bulk-copies three-word records from the `0x82E49E40` table into the `0x82E49E70` runtime table. |
| `0x82411438` | `sub_82411438` | `DOAX_WriteVideoModeDat` | Builds `:\videomode.dat`, reads `XGetVideoMode`, timestamps the record, and writes a 512-byte settings block. |
| `0x824119C0` | `sub_824119C0` | `DOAX_ReadVideoModeDat` | Builds `:\videomode.dat`, opens it for reading, and loads 512 bytes into `byte_82E96490`. |
| `0x824118A0` | `sub_824118A0` | `DOAX_WritePhotoDataPtcTexture` | Builds `:\photodata.ptc`, creates a texture header, and writes the texture payload. |
| `0x82412EA0` | `sub_82412EA0` | `DOAX_CountSavedPhotoPtcFiles` | Storage state machine counts saved content entries whose names contain `.ptc`. |
| `0x824123A0` | `sub_824123A0` | `DOAX_CreateScreenshotContentFile` | Creates screenshot content with a timestamp/randomized filename under the screenshot drive. |
| `0x82415E88` | `sub_82415E88` | `DOAX_UserProfileStorageStateMachine` | Large user/content queue state machine for profile storage files such as `%s:\ups.dat` and `%s:\rds.dat`. |
| `0x825CAB38` | `sub_825CAB38` | `DOAX_LoadStageCK1HrtTable` | Opens `D:\dat\stage\stage_CK1\stage_CK1.hrt` and parses its stage records. |
| `0x825CCFF0` | `sub_825CCFF0` | `DOAX_LoadStageCK2HrtTable` | Uses the same HRT table parser for the CK2 stage file path. |
| `0x825CDF68` | `sub_825CDF68` | `DOAX_LoadStageCK3HrtTable` | Uses the same HRT table parser for the CK3 stage file path. |
| `0x825CEEE0` | `sub_825CEEE0` | `DOAX_LoadStageCK4HrtTable` | Uses the same HRT table parser for the CK4 stage file path. |
| `0x82476028` | `sub_82476028` | `DOAX_UnpackAcsnmlExtData` | Copies a packed ACSNML record, checks `ACSNML_EXT_DATA`, and imports optional extension pointers and counts. |
| `0x82485E20` | `sub_82485E20` | `DOAX_RelocateAcsnmlExtDataPointers` | Checks `ACSNML_EXT_DATA` and relocates the extension pointer table plus nested fields. |
| `0x82491ED8` | `sub_82491ED8` | `DOAX_ValidateMultiClothChunks` | Checks `MULTI_CLOTH`, iterates children, calls the callback for `CLOTH_DATA`, and handles `SPARE_CHUNK`. |
| `0x82492000` | `sub_82492000` | `DOAX_GetMultiClothEntry` | Validates `MULTI_CLOTH` and returns the indexed child pointer. |
| `0x82492088` | `sub_82492088` | `DOAX_GetMultiClothSpareChunk` | Validates `MULTI_CLOTH` and returns the optional spare chunk pointer. |
| `0x82492110` | `sub_82492110` | `DOAX_CalcSpareChunkWorkBufferSize` | Validates `SPARE_CHUNK` and sums aligned work-buffer sizes by node type. |
| `0x82492290` | `sub_82492290` | `DOAX_RelocateSpareChunkAndBindBuffers` | Validates `SPARE_CHUNK`, relocates internal pointers, assigns 0x140-byte buffers, and zeroes them. |
| `0x824925C0` | `sub_824925C0` | `DOAX_CalcClothDataWorkBufferSize` | Validates `CLOTH_DATA` and computes aligned runtime/work size from cloth, grid, thread, and spare data. |
| `0x82491C68` | `sub_82491C68` | `DOAX_CalcMultiClothWorkBufferSize` | Validates `MULTI_CLOTH` and sums each child `CLOTH_DATA` work buffer plus the optional spare chunk. |
| `0x82491D90` | `sub_82491D90` | `DOAX_RelocateMultiClothAndBindBuffers` | Relocates multi-cloth offsets, then binds each child cloth and spare chunk into the runtime buffer. |
| `0x824924F0` | `sub_824924F0` | `DOAX_FindSpareChunkNodeByName` | Validates `SPARE_CHUNK` and returns the type-1 named spare node matching the requested name. |
| `0x824928A0` | `sub_824928A0` | `DOAX_RelocateClothDataAndBindBuffers` | Validates `CLOTH_DATA`, relocates grid/fix/thread/key/group pointers, and assigns runtime buffers. |
| `0x82493090` | `sub_82493090` | `DOAX_GetClothKeyChainCount` | Returns the key-chain count from the relocated `CLOTH_DATA` key-chain chunk. |
| `0x82493120` | `sub_82493120` | `DOAX_GetClothKeyChainEntry` | Returns an indexed key-chain entry pointer from relocated `CLOTH_DATA`. |
| `0x824931C8` | `sub_824931C8` | `DOAX_LinkClothDataNode` | Links a cloth runtime node into the `CLOTH_DATA` linked node list and renumbers nodes. |
| `0x824932B8` | `sub_824932B8` | `DOAX_UnlinkClothDataNode` | Removes a node from the `CLOTH_DATA` linked list and clears references. |
| `0x82493398` | `sub_82493398` | `DOAX_InitClothDataRuntimePose` | Initializes runtime pose buffers, transforms grid vertices, updates fixed flags, and calls callback code 1. |
| `0x824938B0` | `sub_824938B0` | `DOAX_ReadClothDataFixGridVertices` | Validates `CLOTH_DATA` and calls the fix-grid vertex readback helper. |
| `0x82493920` | `sub_82493920` | `DOAX_StepClothDataSimulation` | Main cloth simulation step: copies params, updates fix grids, integrates, collides, and post-processes. |
| `0x824969F0` | `sub_824969F0` | `DOAX_UploadClothDataVertexBuffer` | Locks and writes cloth vertex positions into render buffers, then unlocks them. |
| `0x824974F8` | `sub_824974F8` | `DOAX_UpdateClothDataRenderBuffers` | Transforms cloth/grid data into render vertex buffers using the lock/update helpers. |
| `0x824981C0` | `sub_824981C0` | `DOAX_CalcClothDataRenderIndexCount` | Derives render index count from cloth grid count and linked cloth nodes. |
| `0x824982A0` | `sub_824982A0` | `DOAX_SetClothDataCallback` | Stores a callback pointer and callback user data in the cloth runtime record. |
| `0x82498328` | `sub_82498328` | `DOAX_RelocateGridDataPointers` | Validates `GRID_DATA` and relocates grid arrays plus nested pointer lists. |
| `0x82498520` | `sub_82498520` | `DOAX_RelocateFixGridPointers` | Validates `FIX_GRID` and relocates fix-grid entries and the optional table. |
| `0x82498608` | `sub_82498608` | `DOAX_RelocateGridWriteChain` | Validates `GRID_WRITE`, relocates its table, and recursively relocates chained grid-write records. |
| `0x824986A0` | `sub_824986A0` | `DOAX_RelocateThreadDataPointers` | Validates `THREAD_DATA` and relocates primary/secondary thread arrays and lists. |
| `0x824AF308` | `sub_824AF308` | `DOAX_CalcMultiClothSlotRenderIndexCount` | For a runtime slot, sums each `MULTI_CLOTH` child cloth render count. |
| `0x824AFC40` | `sub_824AFC40` | `DOAX_BuildMultiClothRenderQueue` | Walks loaded multi-cloth slots under the critical section and queues render/update entries. |
| `0x824AF6F0` | `sub_824AF6F0` | `DOAX_UpdateMultiClothRuntimeSlot` | Updates each cloth entry in a runtime slot, including init, readback, simulation, and render-buffer work. |
| `0x824AF970` | `sub_824AF970` | `DOAX_DrainClothRenderQueue` | Pops queued cloth entries and dispatches readback, init, simulation, and render-buffer updates. |
| `0x824AEB30` | `sub_824AEB30` | `DOAX_PopClothRenderQueueEntry` | Selects an unconsumed render queue entry under the cloth queue critical section and copies it out. |
| `0x824AECD8` | `sub_824AECD8` | `DOAX_RegisterMultiClothRuntimeSlot` | Allocates and initializes a multi-cloth runtime slot, binds work buffers, and installs callbacks. |
| `0x824AEF68` | `sub_824AEF68` | `DOAX_ClearMultiClothRuntimeSlot` | Detaches paired spare links, validates chunks, invokes cleanup callbacks, and clears slot fields. |
| `0x824ADFA0` | `sub_824ADFA0` | `DOAX_ClothLsodeCallback` | Callback installed for cloth entries whose name contains `lsode`; updates matching attachment points. |
| `0x824AE568` | `sub_824AE568` | `DOAX_ClothRsodeCallback` | Callback installed for cloth entries whose name contains `rsode`; mirrors the right-side attachment behavior. |
| `0x82496F88` | `sub_82496F88` | `DOAX_ReadClothFixGridVertexBuffer` | Locks the fix-grid vertex buffer and reads positions back into cloth fix-grid entries. |
| `0x824970D8` | `sub_824970D8` | `DOAX_UpdateFixGridAttachmentPositions` | Transforms fix-grid attachment points and invokes cloth callback code 2. |
| `0x82497450` | `sub_82497450` | `DOAX_RecomputeClothGridNormals` | Loops grid entries and calls the per-entry normal recompute helper. |
| `0x8249A100` | `sub_8249A100` | `DOAX_RecomputeClothGridNormal` | Computes cross products, normalizes neighbor vectors, and writes the grid normal. |
| `0x82494380` | `sub_82494380` | `DOAX_UpdateClothThreadConstraints` | Iterates `THREAD_DATA` arrays and updates thread constraints for each entry. |
| `0x824948B8` | `sub_824948B8` | `DOAX_UpdateClothConstraintNodes` | Dispatches linked cloth constraint/collision nodes by node type 1 through 9. |
| `0x824949A8` | `sub_824949A8` | `DOAX_IntegrateClothGridParticles` | Integrates cloth grid particles using runtime parameters, gravity/forces, and current transform. |
| `0x82495030` | `sub_82495030` | `DOAX_ResolveClothNodeCollisions` | Projects particle positions against linked node collision shapes and constraints. |
| `0x82495FE8` | `sub_82495FE8` | `DOAX_UpdateClothCollisionContacts` | Rebuilds collision/contact flags and adjacency bookkeeping after collision resolution. |
| `0x82499900` | `sub_82499900` | `DOAX_ResolveClothThreadConstraint` | Resolves distance constraints along thread entries and moves attached particle positions. |
| `0x8248DA58` | `sub_8248DA58` | `DOAX_RelocateTdpackResourceRecord` | Relocates pointer fields in a tdpack-like resource record and fills a 92-byte runtime descriptor. |
| `0x8248E5A8` | `sub_8248E5A8` | `DOAX_SelectMainTdpackVariant` | Resets the main tdpack runtime descriptor and selects child resources by mapped variant index. |
| `0x8248E890` | `sub_8248E890` | `DOAX_SelectAuxTdpackVariant` | Performs the same tdpack child selection for the auxiliary descriptor table. |
| `0x8248EA58` | `sub_8248EA58` | `DOAX_SelectTdpackVariantIndex` | Maps requested variant ids and sentinel 255 into a tdpack child index. |
| `0x824FFF78` | `sub_824FFF78` | `DOAX_ResetVoiceArchiveKeys` | Sets archive keys including `voice` to disabled state and refreshes the voice manager. |
| `0x8273B1E0` | `sub_8273B1E0` | `DOAX_EnableVoiceArchiveKeys` | Sets the same archive keys including `voice` to enabled state. |
| `0x8265E328` | `sub_8265E328` | `DOAX_InitSoundSystem` | Opens DOAX2 sound `.xgs`, `.ssc`, `.sso`, and `.dat` files, allocates audio buffers, and initializes audio state. |
| `0x8265F0F8` | `sub_8265F0F8` | `DOAX_LoadFileToPhysicalAudioMemory` | Generic sound loader that opens a file, gets its size, allocates physical memory, and reads the bytes. |
| `0x8265E038` | `sub_8265E038` | `DOAX_SetAudioEffectPreset` | Calls `XAudioVoice_SetEffectParam` and copies a 48-byte effect preset into audio state. |
| `0x8265E7B8` | `sub_8265E7B8` | `DOAX_UpdateSoundSystem` | Per-frame sound tick for voice/archive timers, audio channel updates, and countdown handles. |
| `0x8265EAA8` | `sub_8265EAA8` | `DOAX_UpdateSoundVolumeFades` | Decrements four fade timers and advances the corresponding volume values by stored deltas. |
| `0x826B2270` | `sub_826B2270` | `DOAX_InitMoviePlaybackSystem` | Sets base path `D:\movie\`, configures movie playback state, and initializes the movie subsystem. |
| `0x826B0FA8` | `sub_826B0FA8` | `DOAX_CreateMovieYuvShaders` | Compiles the fullscreen movie vertex shader and YUV-to-RGB pixel shaders. |
| `0x82710F08` | `sub_82710F08` | `D3D_SetupMovieCaptureFileSegments` | D3D infrastructure mounts the `xbmovie` symbolic link and creates segmented movie capture files. |
| `0x826BFDE8` | `sub_826BFDE8` | `DOAX_FilterSavedPhotosByTimestampPtc` | Filters saved photo entries matching timestamp-shaped `.ptc` filenames and sorts matches. |
| `0x824137C0` | `sub_824137C0` | `DOAX_PollStorageDeviceGate` | Polls storage-device gate states and advances `DOAX_GateStorageDevState` to ready. |
| `0x82413738` | `sub_82413738` | `DOAX_CloseStorageEnumeration` | Closes active storage enumeration, clears the count, and finalizes the storage enumerator. |
| `0x825243D0` | `sub_825243D0` | `DOAX_UpdateOnlineRuleDataStateMachine` | Processes `RULEDATA`, `anm%d.afs`, and XSession join/delete/flush states. |
| `0x82523BB8` | `sub_82523BB8` | `DOAX_PollOnlineRequestState` | Polls an overlapped online request and normalizes success/error state. |
| `0x82524010` | `sub_82524010` | `DOAX_PollOnlineContentTask` | Polls content/storage task states and calls `XOnlineGetTaskProgress`. |
| `0x82523C40` | `sub_82523C40` | `DOAX_StartOnlineStorageEnumeration` | Prepares a wildcard path, calls the stats/process helper, then starts `XStorageEnumerate`. |
| `0x82523B38` | `sub_82523B38` | `DOAX_ClearOnlineSearchResults` | Clears online search/stat result fields and frees two result buffers. |
| `0x825238A8` | `sub_825238A8` | `DOAX_StartXUserReadStats` | Performs the two-pass `XUserReadStats` allocation and overlapped read. |
| `0x82524160` | `sub_82524160` | `DOAX_BuildRuleDataStatsSpec` | Builds stat spec entries for rule-data ids 11 and 12 from discovered rule-data flags. |
| `0x82524B38` | `sub_82524B38` | `DOAX_FlushQueuedRuleDataStats` | Batches queued rule-data stats and starts a session stats write. |
| `0x82523A80` | `sub_82523A80` | `DOAX_StartXSessionWriteStats` | Direct `XSessionWriteStats` wrapper with overlapped state handling. |
| `0x82524AD8` | `sub_82524AD8` | `DOAX_GetRuleDataQueueCount` | Computes circular queue count from rule-data head and tail indexes. |
| `0x82524B00` | `sub_82524B00` | `DOAX_PeekRuleDataQueueEntry` | Returns the current rule-data queue entry pointer when the queue is not empty. |
| `0x82523450` | `sub_82523450` | `DOAX_GetRuleDataPlayerSlotById` | Looks up a queued rule-data user id in the 244-byte table and extracts the 6-bit player slot. |
| `0x8258DD60` | `sub_8258DD60` | `DOAX_InitRuntimeSystems` | Broad startup init for cache, gamma, input, events, sound, movie, and the root `D:\DOAX2\` mount. |
| `0x8258C178` | `sub_8258C178` | `DOAX_InitGraphicsRuntime` | Initializes video mode flags, D3D state, vertex declaration, buffers, and render allocator. |
| `0x8258BF50` | `sub_8258BF50` | `DOAX_RebuildGammaRamp` | Rebuilds RGB gamma ramp words from brightness, gamma, and offset globals. |
| `0x82669C10` | `sub_82669C10` | `DOAX_InitMenuSpriteResourcePools` | Initializes pending-load and cache tables for menu sprite assets and optional cache backing. |
| `0x8266E418` | `sub_8266E418` | `DOAX_InitMenuSpritePumpState` | Clears all menu sprite pump state arrays, request slots, and the deferred action queue. |
| `0x8266BEC8` | `sub_8266BEC8` | `DOAX_ResetMenuSpritePumpSlots` | Resets banner, option, icon, and ready sprite pump records to sentinel values. |
| `0x82669CF8` | `sub_82669CF8` | `DOAX_MenuSpriteLoadQueuePump` | Pumps the async menu sprite load queue, cache mount state, and pending device reads. |
| `0x8266A410` | `sub_8266A410` | `DOAX_AddMenuSpriteCacheRange` | Invalidates overlapping cache ranges, inserts an asset/offset/length cache entry, and sorts the table. |
| `0x8266A4E0` | `sub_8266A4E0` | `DOAX_SortMenuSpriteCacheRanges` | Insertion-sorts the 256-entry menu sprite cache range table by offset. |
| `0x8266A590` | `sub_8266A590` | `DOAX_StartMenuSpriteAsyncRead` | Opens a packed asset, starts an async read into cache, and stores slot/device metadata. |
| `0x8266A658` | `sub_8266A658` | `DOAX_PollMenuSpriteAsyncRead` | Polls one async read slot, handles completion/error, and requeues or closes the device. |
| `0x8266A830` | `sub_8266A830` | `DOAX_CloseMenuSpriteActiveDevice` | Releases/closes the active menu sprite device and clears active cache metadata. |
| `0x8266A8F0` | `sub_8266A8F0` | `DOAX_LoadMenuSpriteAssetToPhysicalMemory` | Opens an asset, sizes it, allocates aligned physical memory, and queues the blocking load. |
| `0x82669F88` | `sub_82669F88` | `DOAX_LoadMenuSpriteAssetBlocking` | Queues a sprite load and pumps until its cache range is loaded. |
| `0x8266A0B0` | `sub_8266A0B0` | `DOAX_IsMenuSpriteCacheRangeLoaded` | Checks the cache range table for a matching asset/owner and nonzero loaded length. |
| `0x8266A1B8` | `sub_8266A1B8` | `DOAX_FindMenuSpriteCacheRangeLength` | Returns cached range length for a loaded owner/offset entry. |
| `0x8266AAB0` | `sub_8266AAB0` | `DOAX_ReloadMenuSpritePackage` | Reloads a package entry, waits for package state, and refreshes dependent sprite objects. |
| `0x8266AB98` | `sub_8266AB98` | `DOAX_AvatarMenuSpritePumpSlot` | Avatar menu sprite state machine that queues assets, creates the sprite object, and publishes it. |
| `0x8266B008` | `sub_8266B008` | `DOAX_UnpackAvatarSpriteResource` | Validates `AVATAR` resource data and unpacks child pointers/offsets into the sprite descriptor. |
| `0x8266AE40` | `sub_8266AE40` | `DOAX_ClearAvatarSpriteLoadSlot` | Cancels an avatar slot's pending loads and resets its state bytes. |
| `0x8266AEC8` | `sub_8266AEC8` | `DOAX_DestroyAvatarSpriteSlot` | Clears loads, detaches draw resources, destroys the object, and releases avatar resources. |
| `0x8266BE40` | `sub_8266BE40` | `DOAX_RequestMenuBannerSprite` | Requests a banner sprite id, clears old banner loads, and marks the pump dirty. |
| `0x8266C618` | `sub_8266C618` | `DOAX_RequestMenuOptionMainSprite` | Allocates or updates an option-main sprite slot with requested option ids and resets state. |
| `0x8266C908` | `sub_8266C908` | `DOAX_ClearMenuOptionMainSprite` | Releases option-main sprite loads and deferred actions, then resets the 48-byte slot. |
| `0x8266CBD8` | `sub_8266CBD8` | `DOAX_AreMenuOptionMainActionsIdle` | Checks deferred action owners for an option-main slot and reports whether actions are idle. |
| `0x8266CFB8` | `sub_8266CFB8` | `DOAX_FindFreeMenuOptionAuxSlot` | Scans aux-option slots for an unused or matching free slot. |
| `0x8266D040` | `sub_8266D040` | `DOAX_RequestMenuOptionAuxSprite` | Requests aux option sprite ids into a 16-byte aux slot and marks the pump dirty. |
| `0x8266D2A8` | `sub_8266D2A8` | `DOAX_ClearMenuOptionAuxSprite` | Releases aux-option sprite loads and resets the slot fields. |
| `0x8266D330` | `sub_8266D330` | `DOAX_DestroyMenuOptionAuxSprite` | Clears the aux slot and releases both loaded resource owners. |
| `0x8266DA38` | `sub_8266DA38` | `DOAX_RequestMenuOptionIconSprite` | Requests an icon sprite id/variant into a 20-byte icon slot and marks the pump dirty. |
| `0x8266DC68` | `sub_8266DC68` | `DOAX_ClearMenuOptionIconSprite` | Releases icon sprite loads and the deferred action, optionally waiting for the owner action to clear. |
| `0x8266DD28` | `sub_8266DD28` | `DOAX_DestroyMenuOptionIconSprite` | Waits through the menu fiber, destroys the icon object, clears linked cloth state, and releases resources. |
| `0x8266DFC0` | `sub_8266DFC0` | `DOAX_RequestMenuSpriteReadyFlag` | Requests the ready-flag sprite, records asset id/owner, and marks the pump dirty. |
| `0x8266E150` | `sub_8266E150` | `DOAX_ClearMenuSpriteReadyFlag` | Clears ready-flag loads/resources and resets ready-flag state. |
| `0x8266B208` | `sub_8266B208` | `DOAX_SharedMenuSpritePumpSlot` | Single global sprite pump over `byte_839476EC` that queues a table asset, builds a sprite object, and publishes `dword_839427B4`. |
| `0x8266B3D8` | `sub_8266B3D8` | `DOAX_ClearSharedMenuSpriteLoad` | Releases the shared sprite owner load and resets state, requested id, loaded id, and cached length fields. |
| `0x8266B440` | `sub_8266B440` | `DOAX_DestroySharedMenuSpriteSlot` | Calls the shared-load clear path, destroys the shared sprite object, clears the published pointer, and releases resources. |
| `0x8266B4C0` | `sub_8266B4C0` | `DOAX_QueueSharedMenuSpriteCacheLoad` | Looks up `word_82B2B070[index]`, compares it with the shared cache owner's loaded asset, and queues a load only when different. |
| `0x8266B538` | `sub_8266B538` | `DOAX_WaitCurrentSharedMenuSpriteCacheLoad` | Queues the cache load for the current shared sprite index and yields until the shared cache range has a nonzero length. |
| `0x8266B600` | `sub_8266B600` | `DOAX_MenuDetailSpritePumpSlot` | Indexed state machine over `unk_83947700` that loads primary/secondary detail sprite resources and publishes `dword_839471FC[slot]`. |
| `0x8266B888` | `sub_8266B888` | `DOAX_RequestMenuDetailSprite` | Requests a detail sprite id into a slot, cancels old loads/actions, resets state, and marks the pump dirty. |
| `0x8266B958` | `sub_8266B958` | `DOAX_ClearMenuDetailSpriteLoads` | Releases detail sprite loads, clears the deferred action, resets slot ids, and can wait for owner action completion. |
| `0x8266B9F0` | `sub_8266B9F0` | `DOAX_DestroyMenuDetailSpriteSlot` | Releases secondary cached data, yields once, destroys the object, clears the slot, and releases primary resources. |
| `0x8266BB48` | `sub_8266BB48` | `DOAX_ApplyMenuDetailSpritePatchTable` | Applies a pointer override table into sprite object resource entries by matching resource indices. |
| `0x8266C758` | `sub_8266C758` | `DOAX_IsMenuOptionMainSpriteFinished` | Reports true when an option-main slot is idle and has finished option ids recorded. |
| `0x8266C7A0` | `sub_8266C7A0` | `DOAX_MenuOptionMainSpriteMatches` | Compares active or finished option-main slot ids against requested ids, with 255 as a wildcard. |
| `0x8266C838` | `sub_8266C838` | `DOAX_WaitMenuOptionMainSpriteFinished` | Yields through the menu fiber until `DOAX_IsMenuOptionMainSpriteFinished(slot)` succeeds. |
| `0x8266CC48` | `sub_8266CC48` | `DOAX_AcquireMenuOptionMainSpriteSlot` | Finds or reuses a matching option-main slot, marks it claimed, or requests a free slot. |
| `0x8266CD40` | `sub_8266CD40` | `DOAX_AssignMenuOptionMainSpriteSlot` | Finds/acquires the requested option-main sprite slot, waits for it, unclaims the old slot, and stores the new slot index. |
| `0x8266D108` | `sub_8266D108` | `DOAX_IsMenuOptionAuxSpriteFinished` | Reports true when an aux option slot is idle and both finished aux ids are recorded. |
| `0x8266D150` | `sub_8266D150` | `DOAX_MenuOptionAuxSpriteMatches` | Compares active or finished aux option slot ids against requested ids, with 255 as a wildcard. |
| `0x8266D1D8` | `sub_8266D1D8` | `DOAX_WaitMenuOptionAuxSpriteFinished` | Yields through the menu fiber until the aux option slot reaches finished state. |
| `0x8266D3D0` | `sub_8266D3D0` | `DOAX_AcquireMenuOptionAuxSpriteSlot` | Reuses a matching aux option slot or requests a free aux slot when no match exists. |
| `0x8266D4A8` | `sub_8266D4A8` | `DOAX_AssignMenuOptionAuxSpriteSlot` | Finds/acquires the requested aux slot, waits for it, unclaims the old slot, and stores the new slot index. |
| `0x8266D598` | `sub_8266D598` | `DOAX_CountMenuSpritePumpOutstandingSteps` | Sums remaining stage counts across main, aux, icon, and ready-flag menu sprite slots. |
| `0x8266D6F0` | `sub_8266D6F0` | `DOAX_GetMenuOptionAuxSpriteAssetRefs` | Maps aux option id/variant inputs to the resource-pair table used by the aux sprite pump. |
| `0x8266DB58` | `sub_8266DB58` | `DOAX_IsMenuOptionIconSpriteFinished` | Reports true when an icon slot is idle and its finished icon id is recorded. |
| `0x8266DBA0` | `sub_8266DBA0` | `DOAX_WaitMenuOptionIconSpriteFinished` | Yields through the menu fiber until the icon sprite slot reaches finished state. |
| `0x8266DE70` | `sub_8266DE70` | `DOAX_FindFinishedMenuOptionIconSpriteSlot` | Scans icon slots for an idle slot whose finished icon id and variant match the request. |
| `0x8266DED0` | `sub_8266DED0` | `DOAX_ClaimMenuOptionIconSprite` | Finds an active or finished matching icon slot, marks it claimed, or requests a new icon sprite. |
| `0x8266E1F0` | `sub_8266E1F0` | `DOAX_IsMenuSpriteReadyFlagLoaded` | Checks whether the ready-flag sprite is idle and matches the requested ready-flag id. |
| `0x8266E238` | `sub_8266E238` | `DOAX_SelectMenuOptionIconVariant` | Uses present state, character mapping, and small lookup tables to select an icon variant byte. |
| `0x8266E2E0` | `sub_8266E2E0` | `DOAX_SelectMenuOptionIconVariantFromItems` | Scans item ids and a fallback id to select an option-icon variant from lookup tables. |
| `0x8266E808` | `sub_8266E808` | `DOAX_ClearAllMenuSpriteSlots` | Clears every menu sprite family: option slots, selection slots, avatar slots, shared slots, detail slots, and asset slots. |
| `0x8266EA50` | `sub_8266EA50` | `DOAX_WaitMenuSpritePumpDisabled` | Yields while the menu sprite pump dirty/enabled byte remains active. |
| `0x8266EAD8` | `sub_8266EAD8` | `DOAX_CountAllMenuSpritePumpOutstandingSteps` | Sums outstanding stage counts across all menu sprite families and asset-slot groups. |
| `0x8266EC30` | `sub_8266EC30` | `DOAX_GetMenuSpriteLoadingProgress` | Converts total outstanding steps versus `dword_8394720C` into a normalized loading-progress value. |
| `0x8266ECC0` | `sub_8266ECC0` | `DOAX_InitMenuSpriteStartupAssets` | Requests initial banner/asset sprites, loads startup assets 2979-2981, allocates the startup handle, and waits for current shared cache load. |
| `0x8266EFC0` | `sub_8266EFC0` | `DOAX_MenuCharacterSpritePumpSlot` | 17-slot sprite pump that loads character-associated resources, clears cloth runtime slots, and publishes per-slot sprite objects. |
| `0x8266F2C0` | `sub_8266F2C0` | `DOAX_RequestMenuCharacterSprite` | Finds a free character sprite slot, records resource id and character id, resets state, and marks the pump dirty. |
| `0x8266F3D8` | `sub_8266F3D8` | `DOAX_ClearMenuCharacterSpriteLoads` | Releases character sprite owner loads, clears the deferred action, resets ids, and can wait for owner action completion. |
| `0x8266F498` | `sub_8266F498` | `DOAX_DestroyMenuCharacterSpriteSlot` | Destroys the character sprite object, clears cloth runtime and related state, and releases both resource owners. |
| `0x8266F5E8` | `sub_8266F5E8` | `DOAX_DestroyMenuCharacterSpriteObjectIfIdle` | Checks whether the owner action is pending, then destroys the per-slot object only when idle. |
| `0x8266F6A8` | `sub_8266F6A8` | `DOAX_FindActiveMenuCharacterSpriteSlot` | Finds an idle character sprite slot with matching resource and character ids and a live published object. |
| `0x8266F728` | `sub_8266F728` | `DOAX_FindMenuCharacterSpriteSlot` | Finds an active or finished character sprite slot matching resource and character ids. |
| `0x8266F7B0` | `sub_8266F7B0` | `DOAX_RequestMenuCharacterSpriteSet` | Enumerates character resource ids and requests a character sprite slot for each entry. |
| `0x8266F858` | `sub_8266F858` | `DOAX_UnregisterMenuAssetSpriteHandle` | Enters the render critical section, scans the 32-entry handle table, and clears the matching handle. |
| `0x8266F8F0` | `sub_8266F8F0` | `DOAX_MenuAssetSpritePumpSlot` | State machine over 24-byte asset sprite slots that queues resources, creates handles, and records loaded ids. |
| `0x8266FBF8` | `sub_8266FBF8` | `DOAX_SelectMenuAssetSpriteSlot` | Chooses an asset sprite slot from the type table, requested subslot, or first available finished slot. |
| `0x8266FD80` | `sub_8266FD80` | `DOAX_GetMenuAssetSpriteResourcePair` | Maps asset sprite type, resource id, and variant to the two-word resource pair table. |
| `0x8266FEE0` | `sub_8266FEE0` | `DOAX_RequestMenuAssetSprite` | Selects a slot, skips empty resource pairs, records requested ids, and marks the menu sprite pump dirty. |
| `0x82670020` | `sub_82670020` | `DOAX_IsMenuAssetSpriteFinished` | Checks the selected asset sprite slot for idle state and finished type/resource ids. |
| `0x82670090` | `sub_82670090` | `DOAX_WaitMenuAssetSpriteFinished` | Yields through the menu fiber until the selected asset sprite slot is finished. |
| `0x82670138` | `sub_82670138` | `DOAX_ClearMenuAssetSprite` | Releases asset sprite owner loads and resets the selected 24-byte asset sprite slot. |
| `0x826701E0` | `sub_826701E0` | `DOAX_GetMenuAssetSpriteResourceId` | Verifies the selected asset sprite type and returns either the primary or secondary loaded resource id. |
| `0x82670A08` | `sub_82670A08` | `DOAX_CountMenuAssetSpriteOutstandingSteps` | Sums remaining pump stages across the first 16 menu asset sprite slots. |

### Not Renamed

| Address | Current name | Reason |
| --- | --- | --- |
| `0x82782B58` | `rex_UnhandledExceptionFilter` | Older notes called this an indirect input dispatch, but current IDA disassembly reads `KeDebugMonitorData`, calls a debug monitor callback, then consults `dword_83DCBCAC` as an exception callback. Left unchanged until a caller/cross-reference proves a different role. |
