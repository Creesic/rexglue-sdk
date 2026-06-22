## Iteration 183

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824A9A60 | sub_824A9A60 | FM2_CarAudio_GetStreamRecord784AtIndex | 0.92 | Ensures capacity via grow helper; returns `784*index + base`; used in CarAudio stream-buffer path compare/update at `+188`; 29 callers. |
| 0x824A8E78 | sub_824A8E78 | FM2_CarAudio_GrowStreamRecord784Vector | 0.91 | Reallocates vector of 784-byte records; `memset` new slots; `FM2_MemcpyAligned` old entries; vtable xref in CarAudio cluster. |
| 0x82494C38 | sub_82494C38 | FM2_Physics_ParseEngineTorqueCurveXml | 0.92 | Reads XML children `RPM`/`Throttle`/`PosTorque`/`NegTorque` plus triangle `x0`–`y2`; clamps via `fsel`; fills float curve array; 26 callers. |
| 0x824F8250 | sub_824F8250 | FM2_Render_ClearRefCounted160ByteVector | 0.90 | Iterates 160-byte (40-dword) elements calling pair-release dtor; frees backing store; zeros begin/end/cap; 26 callers. |
| 0x821D0EE8 | sub_821D0EE8 | FM2_Com_ReleaseRefCountedPairFields | 0.91 | `FM2_RefCounted_Dtor_Field27to30`; releases two vtable+8 ref-counted fields; used by vector clear and render paths. |
| 0x82297B70 | sub_82297B70 | FM2_Lua_PushStdStringIntoStackSlot | 0.90 | Reads SSO/`std::string`-like arg; `FM2_Lua_PushLStringOrNil`; `FM2_Lua_PopStackSlot` into binding slot; 24 callers. |
| 0x8254E2D0 | sub_8254E2D0 | FM2_Lua_ToLStringOrRaiseMismatch | 0.93 | `FM2_Lua_ToLString`; on null raises type mismatch for tag 4 (string); 22 callers. |
| 0x827DDC88 | sub_827DDC88 | FM2_Render_ElementLinkVector_FindAndEraseTriple | 0.91 | Linear search 12-byte triple `(field0,field2,field1)`; calls erase-at-index; 23 render callers. |
| 0x827DDB70 | sub_827DDB70 | FM2_Render_ElementLinkVector_EraseAtIndex | 0.92 | `memmove` 12-byte entries after index; decrements count; paired with find-erase helper. |
| 0x824B7A10 | sub_824B7A10 | FM2_Lua_UpdateCallDepthAfterInvoke | 0.89 | `FM2_Lua_IncrementCallDepthOrOverflow`; updates max stack depth when result is -1; 23 Lua binding callers. |
| 0x8220BA60 | sub_8220BA60 | FM2_RewardsQuery_GetPtrFromStateChangeContext | 0.88 | Lazy static init; returns `&dword_829C25F8` or vtable+124 rewards-query ptr; feeds `FM2_RewardsQuery_GetRecordOffset24` in scene loader. |
| 0x8263A7A8 | sub_8263A7A8 | FM2_Tuning_ValidateAndNormalizeDefault | 0.90 | Logs `"Tune default out of range"`; validates min/max; normalizes default into `[0,1]` via `fsel`; 26 tuning vtable callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → vector push helper only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask lookup; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper → `sub_8279F5E0` only. |
| 0x82789188 | sub_82789188 | 40B wrapper → `sub_82790848` only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper → `sub_827B6828` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82466040 | sub_82466040 | Single `stvx` vec128 store; too trivial alone. |
| 0x82201CB8 | sub_82201CB8 | Resource-lock frame alloc/copy; defer with frame-slot cluster. |
| 0x822ECCE8 | sub_822ECCE8 | ProfileLua binding+number marker init; defer next ProfileLua cluster. |
| 0x824D8B98 | sub_824D8B98 | 56B pool alloc wrapper; defer with `sub_824D8130` init naming. |
| 0x827EB5C0 | sub_827EB5C0 | 16-byte memcpy init; too thin alone without key struct name. |
| 0x82238510 | sub_82238510 | Large (~996B) unanalyzed function; defer next pass. |
| 0x8295C378 | sub_8295C378 | Large (~1KB) unanalyzed function; defer next pass. |
| 0x8295CCD8 | sub_8295CCD8 | Large unanalyzed function; defer next pass. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x82753E00 | sub_82753E00 | Jump thunk only. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
