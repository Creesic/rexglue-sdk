## Iteration 182

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8254E230 | sub_8254E230 | FM2_Lua_CheckStackValueTypeOrRaise | 0.93 | `FM2_Lua_GetStackValueType` vs expected tag; raises `FM2_Lua_RaiseTypeMismatchError` with `FM2_Lua_GetTypeNameForTag`; 29 callers. |
| 0x82360D70 | sub_82360D70 | FM2_XmlWriter_AppendTriangleUvFloatAttrs | 0.94 | Writes XML float attrs `x0`,`y0`,`x1`,`y1`,`x2`,`y2` via `FM2_XmlWriter_AppendFloatAttr`; 27 vtable xrefs. |
| 0x824A1998 | sub_824A1998 | FM2_Com_InitCriticalSectionThreadSafeInterface | 0.92 | Zeros handle; `RtlInitializeCriticalSectionAndSpinCount(0x600)`; `SetInterfaceThreadSafe`; 26 callers. |
| 0x822707A0 | sub_822707A0 | FM2_Career_GetCarClassIdIfForceStockTuning | 0.91 | When profile flag `ForceStockUpgradesAndTuning` set, SQL `SELECT * FROM Data_Car WHERE Id=%i`; reads `ClassId` column; 25 callers. |
| 0x827DDEE8 | sub_827DDEE8 | FM2_Render_ElementLinkVector_PushCopy12 | 0.92 | Growable vector of 12-byte triples; copies 3 dwords; doubles capacity via `sub_827DDD48`; vtable xref. |
| 0x827DE060 | sub_827DE060 | FM2_Render_ElementLinkVector_PushAtIndex | 0.91 | Indexes `12*slot + base+28`; packs 3 args into stack triple; calls push-copy; 26 callers. |
| 0x82798528 | sub_82798528 | FM2_Render_SliceHandleView_Dtor | 0.90 | Sets slice-handle vtables `off_82148D74/44/30`; `sub_82798AC8` teardown; `FM2_STL_EhUnwind_ReleaseSharedPtrGuard`; 28 vtable xrefs. |
| 0x823939B0 | sub_823939B0 | FM2_Image_ResampleKernel_ApplySquareOrGammaLUT_RGBA | 0.90 | Per-texel square or LUT-interpolate (`*254` index into `flt_82028338/3C`); format branches at `+8`/`+20`; 35 callers in resample cluster. |
| 0x823D77A0 | sub_823D77A0 | FM2_Image_ResampleScalarKernel_ApplySquareOrGammaLUT_RGBA | 0.89 | Same algorithm as `823939B0` with scalar-kernel LUT `flt_820307F8/FC`; paired with scalar-kernel vtable `off_82030BF8`; 33 callers. |
| 0x823D6DD0 | sub_823D6DD0 | FM2_Image_ResampleScalarKernel_DtorFreeTaggedBuffers | 0.91 | Scalar-kernel vtable `off_82030BF8`; frees tagged buffers at `+56/+88/+92`; mirrors main resampler dtor pattern. |
| 0x823E0C80 | sub_823E0C80 | FM2_Image_ResampleScalarKernel_ScalarDtorMaybeFree | 0.90 | Calls scalar-kernel dtor; optional `FM2_Memory_FreeSmallBlockOrNull` when flag bit set; 32 vtable xrefs. |
| 0x826206F8 | sub_826206F8 | FM2_Lua_ReadStackSlotIntoVariant16 | 0.91 | Resolves relative/absolute stack index; fills 16-byte variant (type, index, value) for bool/number/string/userdata; 24 callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → vector push helper only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask lookup; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x824A9A60 | sub_824A9A60 | 784-byte record accessor; defer with `FM2_CarAudio_GrowStreamRecord784Vector` cluster. |
| 0x824A8E78 | sub_824A8E78 | Grow 784-byte CarAudio stream-record vector; pair rename next pass. |
| 0x82494C38 | sub_82494C38 | Large engine-curve XML parser (`RPM`/`Throttle`/`PosTorque`); defer physics cluster. |
| 0x82466040 | sub_82466040 | Single `stvx` vec128 store; too trivial alone. |
| 0x824F8250 | sub_824F8250 | Clears 160-byte ref-counted vector; defer with `sub_821D0EE8` naming. |
| 0x821D0EE8 | sub_821D0EE8 | Releases two ref-counted fields; defer pair with vector clear. |
| 0x8220BA60 | sub_8220BA60 | Lazy singleton + vtable+124 dispatch; callee type name unresolved. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper → `sub_8279F5E0` only. |
| 0x82789188 | sub_82789188 | 40B wrapper → `sub_82790848` only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper → `sub_827B6828` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82782848 | sub_82782848 | 52B wrapper → `AdvanceElementSliceOffset` only. |
| 0x827897C0 | sub_827897C0 | 60B wrapper → `InitElementSliceView` only. |
| 0x826F0E48 | sub_826F0E48 | SQLite trivia; single flag-bit clear. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x82753E00 | sub_82753E00 | Jump thunk only. |
| 0x82753ED0 | sub_82753ED0 | Jump thunk only. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
| 0x824F0160 | sub_824F0160 | 8-byte jump thunk only. |
