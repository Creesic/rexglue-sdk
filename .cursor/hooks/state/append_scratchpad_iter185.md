## Iteration 185

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827EB5C0 | sub_827EB5C0 | FM2_Render_InitElementLinkKey16 | 0.90 | Copies 16-byte key via `FM2_MemcpyVectorized`; zeros `+16/+20`; 22 element-link callers. |
| 0x827EB568 | sub_827EB568 | FM2_Render_InitElementLinkKeyFromSimd | 0.89 | Packs stack floats into vec128 (`lvlx`/`vrlimi128`/`stvx`); zeros `+16/+20`; paired with key16 init; 20 callers. |
| 0x827D68A8 | sub_827D68A8 | FM2_Render_LookupMaterialPassFlagByteByNegativeId | 0.91 | Binary search sorted dword table at material `+348`; returns byte flag at `+508`; negative slot ids only; 23 callers. |
| 0x827D6C58 | sub_827D6C58 | FM2_Render_DispatchMaterialPassByNegativeSlot | 0.88 | Calls pass-flag lookup; `sub_827E0A88`; dispatches via `sub_827E5058`; 20 render callers. |
| 0x8244F5C8 | sub_8244F5C8 | FM2_HashName_EncoderAppendBits | 0.92 | Bit accumulator at encoder `+229764`; flushes bytes when `>=8/16` bits; calls flush helper; 24 HashName callers. |
| 0x8244EEE0 | sub_8244EEE0 | FM2_HashName_EncoderFlushPendingBits | 0.90 | Updates length fields; sets flush arg; calls `sub_82453130`; paired with append-bits; 10 encoder callers. |
| 0x82618698 | sub_82618698 | FM2_Stl_IosBase_SetStateFlagsOrThrow | 0.93 | Sets `ios` state bits `&0x17`; throws `ios_base::badbit/failbit/eofbit set`; 12 stream callers. |
| 0x82617088 | sub_82617088 | FM2_Stl_Streambuf_PutByteOrBuffer | 0.90 | Buffered put-byte when room in internal buffer; else overflow virtual `+4`; used by file-stream write; 9 callers. |
| 0x8261BBC0 | sub_8261BBC0 | FM2_Stl_FileStream_WriteCStringWithFlush | 0.89 | Writes C string via streambuf `+32`; handles put-back/backpressure; sets fail/eof via ios helper; 22 formatters. |
| 0x8278A760 | sub_8278A760 | FM2_Network_KillConnectionWithLog | 0.92 | Sets connection state `3`; logs `"Killing connection: %hs"`; optional notify on port; thread assert; 21 callers. |
| 0x82466040 | sub_82466040 | FM2_Simd_StoreVec128FromStack | 0.91 | `lvx128` from stack spill; `stvx` to destination; 25 math/physics callers. |
| 0x8261E3E0 | sub_8261E3E0 | FM2_Lua_InvokeProtectedClearBindingCallback | 0.90 | Pushes light userdata + nil; `FM2_Lua_InvokeProtectedCall32(-10000)`; binding teardown callback; 9 callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → vector push helper only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask lookup; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper only. |
| 0x82789188 | sub_82789188 | 40B wrapper only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only; too trivial alone. |
| 0x82782C68 | sub_82782C68 | Thin wrapper → `sub_82782F18` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0`. |
| 0x82506380 | sub_82506380 | Scalar dtor pattern only; defer object-base cluster. |
| 0x82620248 | sub_82620248 | Binding buffer reset wrapper; defer with clear-callback cluster. |
| 0x827DD260 | sub_827DD260 | Unit-string table lookup; defer with `sub_827EF320` naming. |
| 0x82619680 | sub_82619680 | Int-vector resize/erase; defer STL vector cluster. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc` implementation; defer stdio cluster. |
| 0x8295C378 | sub_8295C378 | Large (~1KB) unanalyzed function; defer next pass. |
| 0x8295CCD8 | sub_8295CCD8 | Large unanalyzed function; defer next pass. |
| 0x827E55E0 | sub_827E55E0 | Large unanalyzed render function; defer next pass. |
| 0x824F3828 | sub_824F3828 | Large unanalyzed function; defer next pass. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
