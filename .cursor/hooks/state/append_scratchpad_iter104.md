## Iteration 104

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826ED720 | sub_826ED720 | FM2_SQLite_ContextAppendErrorMessage | 0.92 | Variadic formatter; frees prior buffer via `dword_82A3CEB8`; calls `FM2_SQLite_Vsnprintf`; increments error counter at +24. Caller `sub_826F0A58` uses `"no such table: %s"`. |
| 0x826ED8C0 | sub_826ED8C0 | FM2_SQLite_StrcasecmpCollated | 0.93 | Case-insensitive strcmp via collation table `byte_82118378`. Used by `FM2_SQLite_LookupTable` for schema/db name matching. |
| 0x826ED1C8 | sub_826ED1C8 | FM2_SQLite_ContextPrepareStatement | 0.90 | If parse context byte+68 clear, calls `FM2_SQLite_OpenMasterTables`; stores result code at context+4 and bumps counter+6. |
| 0x82707108 | sub_82707108 | FM2_SQLite_Vsnprintf | 0.91 | Wraps `sub_82707020` with `j_FM2_SQLite_ReallocOrAllocZeroed` allocator and 350-byte stack buffer. |
| 0x826ED0A0 | sub_826ED0A0 | FM2_SQLite_OpenMasterTables | 0.91 | Iterates attached DB entries; calls `FM2_SQLite_InitMasterSchema` per slot; invokes detach/compact helpers on success. Sets byte+68 busy flag during open. |
| 0x826ED228 | sub_826ED228 | FM2_SQLite_FindDatabaseIndexByCookie | 0.92 | Returns -1000000 when cookie arg is 0; else scans 24-byte DB records at +20 for matching cookie value. |
| 0x826ECCC8 | sub_826ECCC8 | FM2_SQLite_InitMasterSchema | 0.94 | Embeds `CREATE TABLE sqlite_master` / `sqlite_temp_master` strings; inserts schema row; reads encoding/BINARY constraints from existing master page. |
| 0x826F0990 | sub_826F0990 | FM2_SQLite_LookupTable | 0.91 | Scans attached databases; optional collated db-name filter; hash lookup table in schema btree via `sub_826F02E0`. |
| 0x826F0CA8 | sub_826F0CA8 | FM2_SQLite_DetachDatabase | 0.90 | Detaches DB at index: runs close hooks; compacts 24-byte DB array; frees names via `FM2_SQLite_FreeIfNonNull`; may shrink to inline storage at context+60. |
| 0x826EBB50 | sub_826EBB50 | FM2_SQLite_GetErrorString | 0.95 | Maps SQLite result codes to strings (`"database is locked"`, `"out of memory"`, etc.) via jump table `byte_82117DC0`. |
| 0x826EE718 | sub_826EE718 | FM2_SQLite_SetErrorMessageV | 0.91 | Variadic concatenation into error-message buffer: frees old `*ptr`, `FM2_SQLite_Alloc`, strcat each arg. Used when reporting schema/init failures. |
| 0x826ED678 | sub_826ED678 | FM2_SQLite_ContextSetErrorV | 0.90 | Sets error code at context+12; optional format through `FM2_SQLite_Vsnprintf`; delegates to `sub_826EF998` with `FM2_SQLite_FreeIfNonNull` deleter. |

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
| 0x826F02E0 | sub_826F02E0 | Schema btree hash lookup; defer with `sub_826F0150` cluster. |
| 0x826F0A58 | sub_826F0A58 | Thin wrapper around prepare+lookup+error; defer iter 105. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
