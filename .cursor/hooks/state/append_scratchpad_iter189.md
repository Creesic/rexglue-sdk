## Iteration 189

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827B05F8 | sub_827B05F8 | FM2_Render_TransactionLogReader_Ctor | 0.91 | Calls `FM2_Render_ByteStreamReader_InitVtable`; sets vtable `off_8214C9A0`; stores controller id + buffer ptr at `+4..+16`; used before `FM2_Render_TransactionLogReader_Read` in transaction-log replay path. |
| 0x827B06B8 | sub_827B06B8 | FM2_Render_TransactionLogReader_InitVtable | 0.90 | Sets `off_8214C9A0` then `sub_827AF060` base reset; 57 render cleanup/ctor thunks; pair to dtor `sub_827B0670`. |
| 0x827B06F0 | sub_827B06F0 | FM2_Render_TransactionLogReader_Read | 0.92 | Bounded `FM2_MemcpyAligned` from internal buffer; advances `+8`/`+16`; optional bytes-read out; HRESULT `0x80070057`/`0x80004005`; vtable entry at `off_8214C9A0+4`. |
| 0x827AEDD0 | sub_827AEDD0 | FM2_Render_ByteStreamReader_InitVtable | 0.89 | Assigns base vtable `off_8214C434`; first step of transaction-log reader ctor chain. |
| 0x827664C0 | sub_827664C0 | FM2_XtsClient_InitListNode | 0.90 | `FM2_STL_ListNode_LinkNext`; calls packet-field init; zeros `+4..+16`; 18 network/XTS ctor callers. |
| 0x82766A58 | sub_82766A58 | FM2_XtsClient_InitListNodePacketFields | 0.91 | `FM2_XtsClient_SendRequestPacket_NoOpStub` twice around `sub_8277C7B0`; initializes linked node packet headers. |
| 0x8295CCD8 | sub_8295CCD8 | FM2_Jpeg_Fwht4x4_InPlace | 0.92 | 16-float in-place Walsh-Hadamard butterfly; scaled by coeff struct `+4`; used for 64-byte blocks in `sub_8295D400`; vtable `0x821c5f38`. |
| 0x8295C378 | sub_8295C378 | FM2_Jpeg_Fwht32_InPlace | 0.91 | 32-float in-place FWHT stage; scaled by `a2+4`/`a2+8`; called from `sub_8295DE30` size-32 path and 128-byte block dispatcher; vtable `0x821c5f28`. |
| 0x824F3828 | sub_824F3828 | FM2_CareerRace_SetGameOptionValueByToken | 0.92 | `FM2_CareerRace_QueryGameOptionsByToken`; reads `"Id"`/`"Query"`/`"NumMin"`/`"NumMax"`; applies via `sub_825C4F78`→`sub_8245FF68`; 24 career menu thunks. |
| 0x822AEEA0 | sub_822AEEA0 | FM2_ProfileLua_PushNumberAndInvokeCallback | 0.90 | `FM2_Lua_PushNumber` then `FM2_ProfileLua_InvokeManagerCallback`; 18 profile UI binding callers. |
| 0x822A7C80 | sub_822A7C80 | FM2_Lua_PushNumberIntoBindingSlot | 0.90 | `FM2_Lua_PushNumber` + `FM2_Lua_PopStackSlot` into binding slot; 18 menu/profile callers. |
| 0x827E55E0 | sub_827E55E0 | FM2_Render_ResourceCache_UpdateOrRemoveEntry | 0.91 | Critsec at `this+13900`; hash bucket `% 0xBB9`; 24-byte entry match/update/remove via `sub_827E5250`; 24 render cache callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x827AF060 | sub_827AF060 | 24B base vtable reset only; pair with `FM2_Render_TransactionLogReader_InitVtable`. |
| 0x827BE550 | sub_827BE550 | RB-tree iterator++ cluster; defer with erase helpers. |
| 0x822073E8 | sub_822073E8 | Element-link end-index helper; defer with `sub_82527B38` cluster. |
| 0x821FE1A8 | sub_821FE1A8 | Scene-graph subtree destroy wrapper; defer with `sub_821FE008`. |
| 0x827E1EF0 | sub_827E1EF0 | Profile category bitmask lookup; defer with `sub_827D7308` cluster. |
| 0x827D7308 | sub_827D7308 | Recursive render lookup wrapper; defer with profile lookup cluster. |
| 0x82798A88 | sub_82798A88 | 60B thin wrapper → `sub_82798D90` at `this+88`. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper only. |
| 0x82789188 | sub_82789188 | 40B wrapper only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8275E148 | sub_8275E148 | CRT wide-char fread; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single nested vtable `+56` delegate only. |
| 0x826E07D0 | sub_826E07D0 | NUISPEECH `CVoiceInput` lifecycle; defer speech cluster. |
| 0x8295DE30 | sub_8295DE30 | Large FWHT dispatcher; defer with JPEG transform cluster. |
