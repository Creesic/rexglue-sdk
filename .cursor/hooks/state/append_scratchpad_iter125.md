## Iteration 125

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82716A00 | sub_82716A00 | FM2_SQLite_DateTime_ParseTokenToContext | 0.91 | Clears 0x30 context; handles `"now"` via global time fn, numeric tokens via `ParseAsciiDouble`; called from `DateTime_TokensMatchInterval`. |
| 0x82716ED8 | sub_82716ED8 | FM2_SQLite_DateTime_ApplyModifier | 0.92 | Applies datetime modifier tokens (`day`, `hour`, `minute`, `second`, `month`, `year`, `localtime`, `unixepoch`, `utc`); uses `DateTime_DecomposeJulianDay`/`DecomposeTimeOfDay`. |
| 0x826F8688 | sub_826F8688 | FM2_SQLite_Btree_RelocateChildPointers | 0.89 | During balance, walks interior cells and right-child pointer updating child page refs via `Btree_SetChildPageRef`; called from `Btree_BalancePage`/`CopyNodeForBalance`. |
| 0x826F85D8 | sub_826F85D8 | FM2_SQLite_Btree_SetChildPageRef | 0.90 | Looks up child slot in parent page, acquires pager ref, stores cell index; optional freelist trunk update when auto-vacuum enabled. |
| 0x826F9C28 | sub_826F9C28 | FM2_SQLite_Btree_SplitPage | 0.91 | Allocates new page, memcpy cell region, `Btree_InitPage`, reinserts overflow cells, calls `Btree_BalancePage`; invoked from `Btree_BalanceIfNeeded`. |
| 0x826F3978 | sub_826F3978 | FM2_SQLite_CodeGen_CreateIndexStatement | 0.90 | CREATE INDEX driver: auth checks, error strings (`index %s already exists`, `table %s may not be indexed`), calls `CodeGen_CreateIndex`; 3 parse callers. |
| 0x8271C1A0 | sub_8271C1A0 | FM2_SQLite_Where_BuildJoinPlan | 0.91 | WHERE/join planner (4.2KB): builds join order, emits `ORDER BY`/`USING PRIMARY KEY`/`WITH INDEX` strings; 5 codegen callers incl. `CodeGen_SelectInExpr`. |
| 0x8271FD40 | sub_8271FD40 | FM2_SQLite_CodeGen_Pragma | 0.92 | PRAGMA handler (5.2KB): `cache_size`, `page_size`, `auto_vacuum`, `synchronous`, `temp_store_directory`; called from `ParseTable_Reduce`. |
| 0x82713A28 | sub_82713A28 | FM2_SQLite_Pager_GetDirtyPageByNumber | 0.88 | Hash lookup of dirty page by page number, bumps refcnt or links via `Pager_LinkDirtyPage`; returns page struct offset +56. |
| 0x82716898 | sub_82716898 | FM2_SQLite_DateTime_ParseIso8601Date | 0.90 | Parses ISO-8601 date (`YYYY-MM-DD` with optional `-`), then delegates time portion to `ParseIso8601Time`; sets Y/M/D fields in context. |
| 0x82716520 | sub_82716520 | FM2_SQLite_DateTime_ParseIso8601Time | 0.89 | Parses `HH:MM:SS` with optional fractional seconds after `:`; fills hour/minute/second and Julian fraction in datetime context. |
| 0x82711668 | sub_82711668 | FM2_SQLite_CodeGen_DeleteFromSelect | 0.90 | DELETE FROM SELECT codegen: opens read cursors, auth check, resolves names, emits delete opcodes; error string `"rows deleted"`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257EFA0 | sub_8257EFA0 | Abstract off_82049CAC vtable with purecall slots only. |
| 0x8257F7E8 | sub_8257F7E8 | Thin adjustor thunk → audio_cue_basic_deferred_threadsafe_dtor. |
| 0x82580268 | sub_82580268 | Thin wrapper → init_params_dtor only. |
| 0x8257F8C8 | sub_8257F8C8 | Thin wrapper → play_params_dtor only. |
| 0x8257FAA0 | sub_8257FAA0 | Thin wrapper → stop_params_dtor only. |
| 0x82581948 | sub_82581948 | Shared scalar-delete thunk (CSkidMarksLink, CAudioThreadLink vtables). |
| 0x82582AA0 | sub_82582AA0 | Body of audio_manager_deferred_dtor; no separate rename needed. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x82583A10 | sub_82583A10 | Single-byte zero store only; too trivial alone. |
| 0x82495868 | sub_82495868 | Adjustor scalar-dtor thunk at this-4 only. |
| 0x82495870 | sub_82495870 | Adjustor release thunk at this-4 only. |
| 0x82687320 | sub_82687320 | Thin wrapper → fmod_sound_stop_child_channels with fixed args only. |
| 0x8268D5D8 | sub_8268D5D8 | Adjustor thunk (-24) to fmod_system_fill_speaker_levels_from_dsp only. |
| 0x82681750 | sub_82681750 | Thin forwarder → fmod_channel_get_dsp_unit_by_index only. |
| 0x82684F30 | sub_82684F30 | Stores single dword at +32 only. |
| 0x826B4820 | sub_826B4820 | Generic `HeapFree` if non-null; too trivial alone. |
| 0x826B4940 | sub_826B4940 | Zeros two dwords only; defer callback-context cluster. |
| 0x826C0CC0 | sub_826C0CC0 | Thin pool-free wrapper (`unk_82A3C880`) only. |
| 0x826DDE48 | sub_826DDE48 | Thin wrapper → `FM2_XAudio2_XapoEffect_TryAcquireRef` only. |
| 0x826EEAD8 | sub_826EEAD8 | Thin `AllocZeroed` callback only. |
| 0x826EF998 | sub_826EF998 | Thin wrapper → `ErrorObjectSetMessage` only. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826EC450 | sub_826EC450 | Thin wrapper → `Statement_Finalize` only. |
| 0x826EC510 | sub_826EC510 | Thin wrapper → `Mem_SetRowid(mem+8)` only. |
| 0x8271D2B8 | sub_8271D2B8 | UPDATE FROM SELECT codegen (`rows updated`); defer dedicated pass. |
| 0x82729920 | sub_82729920 | Render/math sinc kernel; outside SQLite cluster. |
| 0x82721BD8 | sub_82721BD8 | Render matrix upload; outside SQLite cluster. |
| 0x8272B630 | sub_8272B630 | D3D texture size alignment; outside SQLite cluster. |
| 0x8272D308 | sub_8272D308 | Shader string-table lookup; outside SQLite cluster. |
| 0x82725DB8 | sub_82725DB8 | VMX128 normalize vector; outside SQLite cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
