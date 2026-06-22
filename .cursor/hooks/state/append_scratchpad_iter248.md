## Iteration 248

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82625A30 | sub_82625A30 | FM2_Lua_CMatrixSetFromComponents | 0.93 | Registered as `set`; validates 16 floats on `CMatrix`; `CLuaBindings::MatrixSetData` / `#ferror in function 'set'`. |
| 0x826269D8 | sub_826269D8 | FM2_Lua_CMatrixGetProperty_11 | 0.92 | Pushes `*userdata` m11; error `invalid 'self' in accessing variable '_11'`. |
| 0x82626A40 | sub_82626A40 | FM2_Lua_CMatrixSetProperty_11 | 0.92 | Sets m11 from float arg; `_11` variable assignment error strings. |
| 0x82626AE8 | sub_82626AE8 | FM2_Lua_CMatrixGetProperty_12 | 0.92 | Pushes matrix element at offset 4; `_12` accessor pattern. |
| 0x82626B50 | sub_82626B50 | FM2_Lua_CMatrixSetProperty_12 | 0.92 | Sets matrix element at offset 4; `_12` setter pattern. |
| 0x82626BF8 | sub_82626BF8 | FM2_Lua_CMatrixGetProperty_13 | 0.92 | Pushes matrix element at offset 8; `_13` accessor pattern. |
| 0x82626C60 | sub_82626C60 | FM2_Lua_CMatrixSetProperty_13 | 0.92 | Sets matrix element at offset 8; `_13` setter pattern. |
| 0x82628230 | sub_82628230 | FM2_Lua_CRotationSetRotation | 0.93 | Registered as `setRotation`; copies `const CRotation`; `SetRotation` error string. |
| 0x82628450 | sub_82628450 | FM2_Lua_CRotationToString | 0.93 | Registered as `tostring`; `CLuaBindings::RotationToString` error string. |
| 0x82627FA8 | sub_82627FA8 | FM2_Lua_CRotationOperatorEquals | 0.92 | Registered as `.eq`; compares two `CRotation`; `operator==` error string. |
| 0x82628518 | sub_82628518 | FM2_Lua_CRotationAdd | 0.92 | Registered as `add`; takes `const CRotation`; `#ferror in function 'add'`. |
| 0x82628628 | sub_82628628 | FM2_Lua_CRotationSubtract | 0.92 | Registered as `subtract`; takes `const CRotation`; `#ferror in function 'subtract'`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82628738 | sub_82628738 | Rotation `scale`; defer with equals/copy cluster. |
| 0x82628838 | sub_82628838 | Rotation `equals`; defer rotation method cluster. |
| 0x82628940 | sub_82628940 | Rotation `copy`; defer rotation method cluster. |
| 0x82629B10 | sub_82629B10 | Vector3 `minVector`; defer vector method cluster. |
| 0x82629C20 | sub_82629C20 | Vector3 `maxVector`; defer vector method cluster. |
