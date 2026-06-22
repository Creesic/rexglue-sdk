## Iteration 107

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F34C8 | sub_826F34C8 | FM2_SQLite_CodeGen_DropTable | 0.93 | DROP TABLE/VIEW codegen: `ContextLookupTable`, auth via `CheckAuthorization`, emits DELETE FROM `sqlite_master`/`sqlite_temp_master`, error strings `"use DROP TABLE..."` / `"table %s may not be dropped"`. |
| 0x826FD950 | sub_826FD950 | FM2_SQLite_Vdbe_SetRegisterText | 0.91 | Indexes 64-byte register slot `(row*cols+col)<<6` from Vdbe+52; calls `ErrorObjectSetMessage` with text/len; sets static-text flag when len=-1. 42 callers. |
| 0x826FCBA0 | sub_826FCBA0 | FM2_SQLite_ParseContext_Alloc | 0.90 | `AllocZeroed(0x248)` parse context; links into db parse-context list at +72; stores magic `0x26B1A1A5` at `[20]`. |
| 0x82709EE0 | sub_82709EE0 | FM2_SQLite_ParseContext_GetOrCreate | 0.91 | Lazy getter: returns `a1[3]` or allocates via `ParseContext_Alloc`. 42 callers across DDL/DML codegen. |
| 0x826ED7B8 | sub_826ED7B8 | FM2_SQLite_ContextClearErrorMessage | 0.89 | Frees accumulated error string at context+8 via `dword_82A3CEB8`; zeros +8 and error counter +24. |
| 0x8270FF28 | sub_8270FF28 | FM2_SQLite_CheckAuthorization | 0.94 | Invokes db auth callback at +144/+148; maps DENY→`"not authorized"` (SQLITE_AUTH), IGNORE=2, invalid codes get format error. 26 callers. |
| 0x8270DEC8 | sub_8270DEC8 | FM2_SQLite_ParseTree_AllocNode | 0.90 | Allocates 0x44-byte parse-tree node with opcode byte, left/right children; copies span coords or calls `sub_8270CC38`. 49 callers. |
| 0x826FD8D8 | sub_826FD8D8 | FM2_SQLite_Vdbe_GrowRegisterArray | 0.91 | Frees old register array via `sub_826FD368`; reallocates `320*newCount` bytes; initializes refcount words at +24 stride. |
| 0x826FCD20 | sub_826FCD20 | FM2_SQLite_Vdbe_AllocLabel | 0.90 | Grows label array at Vdbe+36; appends `-1` placeholder; returns negative label id `-(index+1)`. 29 callers. |
| 0x826EE6C0 | sub_826EE6C0 | FM2_SQLite_Strdup | 0.93 | `Alloc(len+1)` + `MemcpyAligned` + null terminator. Used for P4 strings and value-to-error conversion. |
| 0x826EE860 | sub_826EE860 | FM2_SQLite_HexDecodeToBlob | 0.92 | Parses even-length hex string (0-9/a-f/A-F) into allocated binary buffer. Used when converting `}` blob literal values. |
| 0x826F08D0 | sub_826F08D0 | FM2_SQLite_ParseContext_Printf | 0.90 | Variadic `Vsnprintf` into temp buffer; runs `sub_82707A68` tokenizer on result; saves/restores 0x44-byte parse state around nested parse. |

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
| 0x826EEAE0 | sub_826EEAE0 | Thin `ReallocOrAllocZeroed` grow helper; defer with memory-util cluster. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826FD368 | sub_826FD368 | Release-register loop only; defer with Vdbe register cluster. |
| 0x82707A68 | sub_82707A68 | Large SQL tokenizer driver; defer iter 108 with `sub_82707300`. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
