## Iteration 123

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827109B8 | sub_827109B8 | FM2_SQLite_CodeGen_ForeignKeyAction | 0.90 | Walks FK list matching onDelete/onUpdate bytes; CASCADE ('c'=99) matches child columns via `IdList_FindIndexByName`; dupes WHERE expr, `CodeGen_ExprIfFalse`, emits FK enforcement opcodes. |
| 0x827124C8 | sub_827124C8 | FM2_SQLite_Schema_DependencyChainContains | 0.89 | Recursive schema walk: checks trigger deps, FK lists, and sibling chains; returns 1 if target object found in dependency graph. |
| 0x82712458 | sub_82712458 | FM2_SQLite_Schema_FkeyDependsOn | 0.90 | Iterates FK reference list entries; calls `Schema_DependencyChainContains` on each referenced table. |
| 0x82712628 | sub_82712628 | FM2_SQLite_Schema_TriggerDependsOn | 0.89 | Walks trigger program list; checks FK deps, column refs, and nested dependency chains for schema object use. |
| 0x82712558 | sub_82712558 | FM2_SQLite_Schema_CheckCrossDatabaseReference | 0.92 | Validates trigger/view cross-db refs; error `"%s %T cannot reference objects in database %s"`; calls trigger/dependency walkers. |
| 0x8270CE68 | sub_8270CE68 | FM2_SQLite_Token_AssignDup | 0.91 | Assigns string token: frees old buffer if dynamic, `Strdup` new text, sets high bit of length field; used in DDL token copying. |
| 0x8270D368 | sub_8270D368 | FM2_SQLite_ExprTree_Equal | 0.90 | Recursive structural equality of expr trees (op, flags, children, COLLATE name); used for index/view comparison. |
| 0x82708638 | sub_82708638 | FM2_SQLite_CodeGen_EmitFkCheckEpilogue | 0.91 | Emits FK-check epilogue opcodes 43/22/91 and backpatches jump; used from insert-row and FK codegen paths. |
| 0x8270D310 | sub_8270D310 | FM2_SQLite_CodeGen_EmitExprList | 0.92 | Iterates ExprList calling `CodeGen_ExprNode` per entry; used before INSERT VALUES and SET clause codegen. |
| 0x826EE268 | sub_826EE268 | FM2_SQLite_Varint_Write | 0.93 | Writes SQLite varint encoding of 64-bit value to output buffer; returns byte count; used in record serialization. |
| 0x82710370 | sub_82710370 | FM2_SQLite_ParseContext_Destroy | 0.91 | Frees Parse/Select context: parse tree, index list, token strings, context struct itself. |
| 0x82713F50 | sub_82713F50 | FM2_SQLite_Pager_Reset | 0.89 | Resets pager to schema version 1: rereads header via VFS, clears dirty-page list, resets journal/WAL state flags. |

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
| 0x826FABE0 | sub_826FABE0 | Returns whether btree savepoint type == 2; too small alone. |
| 0x826EF408 | sub_826EF408 | Sets Mem rowid (type 4); defer small Mem helpers cluster. |
| 0x827086D0 | sub_827086D0 | Large INSERT row codegen (1KB); defer with insert codegen pass. |
| 0x8271C1A0 | sub_8271C1A0 | Large WHERE planner driver (4.2KB); defer dedicated pass. |
| 0x8271FD40 | sub_8271FD40 | PRAGMA handler (5.2KB); defer dedicated pass. |
| 0x82729920 | sub_82729920 | Render/math sinc kernel; outside SQLite cluster. |
| 0x82721BD8 | sub_82721BD8 | Render matrix upload; outside SQLite cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
