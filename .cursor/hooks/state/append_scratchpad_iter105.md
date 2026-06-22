## Iteration 105

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F02E0 | sub_826F02E0 | FM2_SQLite_SchemaHashLookup | 0.91 | Hashes table name (`HashNameAscii` or `HashNameCollated` by schema type); walks hash chain via `SchemaHashChainFind`; returns entry payload at `[2]`. Called from both lookup-table helpers. |
| 0x826F0A58 | sub_826F0A58 | FM2_SQLite_ContextLookupTable | 0.93 | Calls `ContextPrepareStatement` then `LookupTable`; on miss formats `"no such table: %s"` / `"%s.%s"` via `ContextAppendErrorMessage` and sets halt flag at +18. |
| 0x826EF958 | sub_826EF958 | FM2_SQLite_AllocErrorObject | 0.92 | `FM2_SQLite_AllocZeroed(0x40)`; initializes refcount word+12=1 and type byte+26=5. Used when lazily allocating context error object at +132. |
| 0x826F0150 | sub_826F0150 | FM2_SQLite_SchemaHashChainFind | 0.90 | Indexes hash bucket at `8*slot + table+24`; walks linked list comparing keys with collated or binary comparator fn ptrs. |
| 0x826EF620 | sub_826EF620 | FM2_SQLite_ErrorObjectSetMessage | 0.91 | Releases open statements; stores message ptr/len/code; sets error-object flags (malloc/static/printf); may grow token buffer when flag 0x100 set. |
| 0x826EFFA0 | sub_826EFFA0 | FM2_SQLite_HashNameAscii | 0.88 | Classic SQLite-style rolling hash over raw bytes; mask `0x7FFFFFFF`. Used for non-collation schema tables. |
| 0x826EFF18 | sub_826EFF18 | FM2_SQLite_HashNameCollated | 0.90 | Same hash loop but maps each byte through `byte_82118378` collation table before mixing. |
| 0x826EFF80 | sub_826EFF80 | FM2_SQLite_SchemaKeyCompareCollated | 0.89 | Length check then `StrncmpCollated` on schema hash keys when table type==3. |
| 0x826EFFD8 | sub_826EFFD8 | FM2_SQLite_SchemaKeyCompareBinary | 0.89 | Length check then byte-by-byte strcmp for default schema hash keys. |
| 0x826ED910 | sub_826ED910 | FM2_SQLite_StrncmpCollated | 0.92 | Bounded collated strcmp via `byte_82118378` mapping; returns 0 on equal prefix when length exhausted. |
| 0x826F0AE8 | sub_826F0AE8 | FM2_SQLite_LookupTableBySchemaHash | 0.88 | Sibling of `LookupTable` but passes `schema+32` to `SchemaHashLookup` (vs `schema+4`); scans attached DB list with optional db-name filter. |
| 0x826F0028 | sub_826F0028 | FM2_SQLite_HashTableResize | 0.87 | Allocates new bucket array via vtable; rehashes existing chain entries into new slots using ascii/collated hash fn. |

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
| 0x826EF998 | sub_826EF998 | Thin null-check wrapper → `ErrorObjectSetMessage` only. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x82707020 | sub_82707020 | Core vsnprintf helper; defer with `sub_82706F18` cluster. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
