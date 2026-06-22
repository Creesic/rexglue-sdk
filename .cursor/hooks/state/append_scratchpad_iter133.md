## Iteration 133

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827003E8 | sub_827003E8 | FM2_SQLite_Vdbe_OpMinMax | 0.89 | MIN/MAX aggregate: scans args with `ValueCompare` and collate direction from VDBE +80; copies winning value via `Mem_Copy`. |
| 0x827006C8 | sub_827006C8 | FM2_SQLite_Vdbe_OpSubstringUtf8 | 0.90 | SUBSTR with UTF-8 codepoint offset/length: counts codepoints, adjusts byte bounds, appends slice. |
| 0x826FF608 | sub_826FF608 | FM2_SQLite_Value_CreateDefault | 0.88 | Allocates 0x80-byte sqlite3_value, initializes four `MemMethods_InitDefault` slots; used by mem helpers. |
| 0x826FAC28 | sub_826FAC28 | FM2_SQLite_Connection_GetScratchBuffer | 0.87 | Lazy-allocates connection scratch buffer at db+56 with destructor callback; caller `Value_CreateDefault`. |
| 0x826F7678 | sub_826F7678 | FM2_SQLite_Btree_CursorMoveToLast | 0.90 | Loads page, if valid cursor calls `MoveToRightmost`, else sets eof=1; btree last-record seek helper. |
| 0x826F8A80 | sub_826F8A80 | FM2_SQLite_Btree_InsertCellsOnPage | 0.89 | Inserts N cells on page: updates cell count byte, `FindCellOffset`, memcpy cell payloads into page buffer. |
| 0x827004A0 | sub_827004A0 | FM2_SQLite_Vdbe_OpTypeof | 0.92 | TYPEOF opcode: switch on mem type returns strings `integer`/`real`/`text`/`blob`/`null`. |
| 0x82700600 | sub_82700600 | FM2_SQLite_Vdbe_OpAbs | 0.91 | ABS opcode: int64 abs via `ValueToInt64`, null passthrough, else double abs; error string "integer overflow". |
| 0x827008F0 | sub_827008F0 | FM2_SQLite_Vdbe_OpLower | 0.90 | lower() SQL func: copies string, applies `tolower` per byte, appends to result mem. |
| 0x826FE7D8 | sub_826FE7D8 | FM2_SQLite_Btree_ReadRecordPayload | 0.89 | Reads record blob from cursor rowid via `Btree_GetRecord`, `Record_ReadVarintLength`, payload extract. |
| 0x82700A70 | sub_82700A70 | FM2_SQLite_Vdbe_OpCoalesce | 0.90 | COALESCE/IFNULL: skips leading NULL args, copies first non-null value to result. |
| 0x826FE688 | sub_826FE688 | FM2_SQLite_Record_ReadVarintLength | 0.88 | Reads serialized record header length from UTF-8 varint prefix via `ReadUtf8Codepoint` + lookup table. |

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
| 0x826FABB0 | sub_826FABB0 | Thin wrapper → sub_8242A348 (12 bytes). |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line `Btree_CheckMetaPageLock` wrapper; too thin alone. |
| 0x826FE160 | sub_826FE160 | Serialize value to buffer; defer with record cluster. |
| 0x82701400 | sub_82701400 | SUM aggregate finalize; defer aggregate cluster. |
| 0x827015E0 | sub_827015E0 | MIN aggregate step; defer aggregate cluster. |
| 0x82701808 | sub_82701808 | Trigger function lookup; defer parse cluster. |
| 0x826F0680 | sub_826F0680 | Finish SELECT codegen; large; defer dedicated pass. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x826E1058 | sub_826E1058 | Low-level bit/mem transform; no strings; purpose unclear. |
| 0x826E1C38 | sub_826E1C38 | XAudio2 leap buffer path; outside SQLite cluster. |
| 0x826E8838 | sub_826E8838 | Win32 critical-section state machine; domain unclear. |
| 0x827294D8 | sub_827294D8 | D3D render pass setup; outside SQLite cluster. |
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
