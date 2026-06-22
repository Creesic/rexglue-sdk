## Iteration 187

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827D6960 | sub_827D6960 | FM2_Lua_ReleaseAndAssignUnitStringTableSlot | 0.91 | Releases prior ref at `4*(slot+4)` via vtable `+0`; assigns new pointer; used by unit-string lazy create. |
| 0x82762918 | sub_82762918 | FM2_Network_OpenConnectionWithSliceHandleOut | 0.90 | Validates context; `FM2_Render_InitSliceHandleBase`; network vtable `+80` open; optional `XTS_TASK_HANDLE` out; thread assert; 19 callers. |
| 0x827623A0 | sub_827623A0 | FM2_Network_ValidateConnectionContextReady | 0.91 | Requires global connection com-ptr; calls vtable `+92` then `+180` ready check; HRESULT-style errors; 4 callers. |
| 0x82762280 | sub_82762280 | FM2_Network_GetXtsTaskHandleFromSlice | 0.92 | Invokes slice vtable with literal `"XTS_TASK_HANDLE"`; 10 network open callers. |
| 0x825C5870 | sub_825C5870 | FM2_CareerRace_LookupGameOptionIdByToken | 0.91 | `FM2_CareerRace_QueryGameOptionsByToken`; reads `"Id"` column via vtable `+108`; 4 callers. |
| 0x825C5CE0 | sub_825C5CE0 | FM2_HashName_AssignFromGameOptionToken | 0.90 | Token lookup then `sub_8245FF68` hash assign at `a1+28`; 38 career/menu callers. |
| 0x82334768 | sub_82334768 | FM2_Render_PushElementLinkTriple12 | 0.92 | Growable 12-byte triple vector push; in-place when capacity or `sub_82334688` grow path; 5 callers incl. `sub_8235ED30`. |
| 0x824B7498 | sub_824B7498 | FM2_Lua_CopyProtoConstantToBindingStack | 0.91 | `FM2_Lua_GetStackSlotPointer` + proto constant; copies 16 bytes to binding stack; 19 Lua codegen callers. |
| 0x82491E50 | sub_82491E50 | FM2_XmlReader_SetOwnedBufferTagged | 0.90 | Frees prior buffer if owned; sets ptr/len; flags type `3` and owned bit; 19 XML reader vtable xrefs. |
| 0x8247D470 | sub_8247D470 | FM2_AIDriver_OnSectorChangeResetRaceLine | 0.91 | Stores `FM2_AIOvertake_GetGlobalRaceTimeFloat` at `+64`; calls `FM2_AIDriver_ResetRaceLineStateOnSectorChange`; 19 AI callers. |
| 0x82470C80 | sub_82470C80 | FM2_Physics_GetSurfaceGripSample | 0.90 | Indexed grip from surface table at `+12` or default `flt_8299AA8C`; flag at `+0` selects path; 19 physics callers. |
| 0x82781DE0 | sub_82781DE0 | FM2_Render_InitSliceHandleWithBrandNotifier | 0.89 | Slice-handle vtable `off_82144D30`; `dscMAKE_FAMILY::RegisterBrand<entENTITY>` via `sub_8279EA90`; 18 render callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → `FM2_Render_PushElementLinkTriple12` at `+4` only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask `&`; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper only. |
| 0x82789188 | sub_82789188 | 40B wrapper only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only; too trivial alone. |
| 0x82782C68 | sub_82782C68 | Thin wrapper → `sub_82782F18` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0`. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc` implementation; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single vtable `+56` delegate only; too thin alone. |
| 0x827B9748 | sub_827B9748 | Wchar shader-macro registry; defer with shader macro cluster. |
| 0x82334688 | sub_82334688 | Element-link vector grow helper; defer with push cluster. |
| 0x8295C378 | sub_8295C378 | Large (~1KB) unanalyzed function; defer next pass. |
| 0x8295CCD8 | sub_8295CCD8 | Large unanalyzed function; defer next pass. |
| 0x827E55E0 | sub_827E55E0 | Large unanalyzed render function; defer next pass. |
| 0x824F3828 | sub_824F3828 | Large unanalyzed function; defer next pass. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
