## Iteration 109

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F2D40 | sub_826F2D40 | FM2_SQLite_CodeGen_BeginDatabaseWrite | 0.90 | Emits one-time Vdbe opcode 91; per-db-index sets write-lock bitmask and stores root btree ptr; for temp db (index 1) calls `CodeGen_OpenTempDatabase`. |
| 0x826F2DE8 | sub_826F2DE8 | FM2_SQLite_CodeGen_OpenSchemaForWrite | 0.91 | Calls `BeginDatabaseWrite`; sets schema-write mask at +44; emits Vdbe opcode 38 (OpenWrite) when requested; loops temp+main when temp tables enabled. 17 callers in DDL/DML codegen. |
| 0x826F2C78 | sub_826F2C78 | FM2_SQLite_CodeGen_OpenTempDatabase | 0.93 | Opens temp db via `sub_826EBF18`; errors `"unable to open a temporary database file..."` and `"unable to get a write lock on the temporary database file"`. |
| 0x826F54E0 | sub_826F54E0 | FM2_SQLite_Btree_ParseCellHeader | 0.91 | Parses btree cell: optional header size via `ReadUtf8Codepoint`, payload via int/varint (`ReadVarint64`); computes local payload + overflow page offsets. 22 callers in btree layer. |
| 0x826EE3E0 | sub_826EE3E0 | FM2_SQLite_ReadUtf8Codepoint | 0.89 | Reads 1–9 byte UTF-8 sequence into `*out`; returns bytes consumed. Used by btree cell header parsing. |
| 0x826EE310 | sub_826EE310 | FM2_SQLite_ReadVarint64 | 0.92 | Classic SQLite 64-bit varint decoder (up to 9 bytes); stores value in `*out`. Used for rowid/payload sizes in cells. |
| 0x82719188 | sub_82719188 | FM2_SQLite_ParseStack_Push | 0.91 | Pushes (state,token,span) onto parser stack; on depth≥100 pops and reports `"parser stack overflow"`. Called from reduce path. |
| 0x82718F50 | sub_82718F50 | FM2_SQLite_ParseStack_DestroySymbol | 0.88 | Jump-table destructor for parse-stack semantic values (token types 154–0x1A6); invoked from `ParseStack_Pop`. |
| 0x826EE638 | sub_826EE638 | FM2_SQLite_StrdupCString | 0.92 | `strlen` + `Alloc` + copy including terminator. Distinct from bounded `Strdup`. Used when duplicating id-list names. |
| 0x826F5E90 | sub_826F5E90 | FM2_SQLite_Btree_GetOrCreateCursor | 0.90 | `HashLookupOrInsert` on btree page; initializes cursor struct at `page+header+offset` with page type flag (100 for leaf). 18 callers. |
| 0x8270CEF8 | sub_8270CEF8 | FM2_SQLite_IdList_Dup | 0.90 | Deep-copies id-list: dup each `ParseTree_DupNode`, `StrdupCString` alias strings, copy collation/sort flags. Used by `Select_Dup`. |
| 0x8270D778 | sub_8270D778 | FM2_SQLite_Select_Dup | 0.91 | Deep-copies 68-byte SELECT parse node: dup id-lists, expr lists, nested selects, ORDER/GROUP/WHERE/HAVING/LIMIT/OFFSET children recursively. |

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
| 0x826EF2C8 | sub_826EF2C8 | Thin int64-value setter; defer with `sub_826EF0F0` cluster. |
| 0x826EEB78 | sub_826EEB78 | Returns `&dword_82A3CE70` only; likely mem-methods ptr getter—defer. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826FA1F8 | sub_826FA1F8 | Variadic error append helper; defer with expr error cluster. |
| 0x8270D960 | sub_8270D960 | Parse-tree recursive walk; defer with `sub_8270EA58` resolve cluster. |
| 0x8270EA58 | sub_8270EA58 | SELECT name-resolution walker; defer dedicated pass. |
| 0x8270EE18 | sub_8270EE18 | 2.6KB expression codegen switch; defer dedicated pass. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
