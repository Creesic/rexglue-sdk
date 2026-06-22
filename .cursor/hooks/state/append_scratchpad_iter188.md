## Iteration 188

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827B9748 | sub_827B9748 | FM2_Render_ShaderMacroRegistry_SetOrReplace | 0.92 | Wchar macro table `word_82A43E88` (34 wchar/name); `wcsicmp` lookup; replace callback at `dword_82A43EC8[17*idx]`; append up to 0x100; font ctor registers `font`/`font_popfont`/`Colour` handlers. |
| 0x82334688 | sub_82334688 | FM2_Render_ElementLinkVector_GrowAndPush12 | 0.91 | Grow path for 12-byte triple vector; calls `FM2_Render_ElementLinkVector_InsertTripleGrow`; returns begin/end pair in QWORD; called from `FM2_Render_PushElementLinkTriple12`. |
| 0x823342C0 | sub_823342C0 | FM2_Render_ElementLinkVector_InsertTripleGrow | 0.92 | 880B STL vector insert for 12-byte triples; capacity doubling/memmove; writes 3-dword element from arg; max growth guard `357913941`; sole caller from grow-and-push. |
| 0x824518C8 | sub_824518C8 | FM2_Zlib_Bitstream_ReadBits | 0.93 | Shifts `*a1` bit buffer by `a2`; refills from dword buffer or `FM2_Zlib_InputStream_ReadByte` x4; returns masked low bits; 18 zlib inflate callers. |
| 0x8261F940 | sub_8261F940 | FM2_Lua_AdjustStackTopRelative | 0.92 | `FM2_Lua_GetStackDepth` then `FM2_Lua_SetStackTop(state, -1 - popCount)`; 17 Lua binding callers. |
| 0x8261F9D8 | sub_8261F9D8 | FM2_Lua_GetLightUserdataAndRestoreStack | 0.91 | Pushes light userdata slot; `FM2_Lua_GetUserdataPointer`; calls `FM2_Lua_AdjustStackTopRelative` with 1; 18 profile/Lua callers. |
| 0x822EC760 | sub_822EC760 | FM2_ProfileLua_PushStdStringAndInvokeCallback | 0.90 | SSO `std::string` buffer select; `FM2_Lua_PushLStringOrNil`; `FM2_ProfileLua_InvokeManagerCallback`; 18 profile/menu callers. |
| 0x824CDD20 | sub_824CDD20 | FM2_XmlReader_GetChildFloatOrDefault | 0.92 | `FM2_XmlReader_GetChildElementByName`; `FM2_XmlReader_ParseFloatAttribute` or default `a3`; 18 XML tuning callers. |
| 0x82420548 | sub_82420548 | FM2_Sqlite_GetOpenFlagMask | 0.88 | Single lookup `off_82998458[a1] & a2`; 36 SQLite open-path callers. |
| 0x8235ED30 | sub_8235ED30 | FM2_Render_PushElementLinkTriple12FromArgs | 0.89 | Packs 3 args into stack triple; calls `FM2_Render_PushElementLinkTriple12` at `vector+4`; 112 render vtable thunks. |
| 0x823E68D0 | sub_823E68D0 | FM2_CritSec_InvokeVtableMethod8 | 0.91 | `RtlEnterCriticalSection(&unk_82997D24)`; vtable `+8` call; leave; 18 audio/resource callers. |
| 0x82225188 | sub_82225188 | FM2_ComObject_EnsureStaticLifetimeInit | 0.87 | `sub_82260E70` once-guard on `dword_829C3F50`; `FM2_ComObject_GetStaticLifetimeBlock`; vtable `+128`; 33 COM static-init callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x827B06B8 | sub_827B06B8 | 56B vtable `off_8214C9A0` init + delegate; defer render transaction-reader cluster. |
| 0x827AF060 | sub_827AF060 | 24B inner vtable assign only; pair with `sub_827B06B8`. |
| 0x827664C0 | sub_827664C0 | XTS list-node ctor cluster; needs pairing with `sub_82766A58`. |
| 0x82798A88 | sub_82798A88 | 60B thin wrapper → `sub_82798D90` at `this+88`. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x827D7308 | sub_827D7308 | Recursive render lookup; defer with `sub_827E1EF0` cluster. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper only. |
| 0x82789188 | sub_82789188 | 40B wrapper only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8295C378 | sub_8295C378 | Large (1056B) unanalyzed. |
| 0x8295CCD8 | sub_8295CCD8 | Large (392B) unanalyzed. |
| 0x827E55E0 | sub_827E55E0 | Large (412B) unanalyzed. |
| 0x824F3828 | sub_824F3828 | Large (460B) unanalyzed. |
