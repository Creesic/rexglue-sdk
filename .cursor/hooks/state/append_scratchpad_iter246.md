## Iteration 246

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82625FA0 | sub_82625FA0 | FM2_Lua_CMatrixSetFromConstMatrix | 0.92 | Registered as `set` (const-matrix overload); copies via `GMatrix3D::GMatrix3D`; falls back to float-component setter. |
| 0x82626078 | sub_82626078 | FM2_Lua_CMatrixSetFromOtherMatrix | 0.93 | Registered as `setMatrix`; error `#ferror in function 'setMatrix'`. |
| 0x82626170 | sub_82626170 | FM2_Lua_CMatrixCopy | 0.92 | Registered as `copy`; `CLuaBindings::MatrixCopy` error string. |
| 0x82626230 | sub_82626230 | FM2_Lua_CMatrixScale | 0.93 | Registered as `scale`; takes xyz floats; `CLuaBindings::MatrixScale`. |
| 0x826263D0 | sub_826263D0 | FM2_Lua_CMatrixTranslate | 0.93 | Registered as `translate`; `CLuaBindings::MatrixTranslate`. |
| 0x82626570 | sub_82626570 | FM2_Lua_CMatrixRotate | 0.93 | Registered as `rotate`; `CLuaBindings::MatrixRotate`. |
| 0x82626710 | sub_82626710 | FM2_Lua_CMatrixInvert | 0.92 | Registered as `invert`; `CLuaBindings::MatrixInvert`. |
| 0x826267C8 | sub_826267C8 | FM2_Lua_CMatrixEquals | 0.92 | Registered as `equals`; `CLuaBindings::MatrixEquals`. |
| 0x826268D0 | sub_826268D0 | FM2_Lua_CMatrixOperatorEquals | 0.92 | Registered as `.eq`; `operator==` error string. |
| 0x82629120 | sub_82629120 | FM2_Lua_CVector3OperatorEquals | 0.92 | Registered as `.eq` in Vector module; compares via `sub_827EB610`. |
| 0x82629FB8 | sub_82629FB8 | FM2_Lua_CVector3ToString | 0.93 | Registered as `tostring`; `CLuaBindings::VectorToString`. |
| 0x82627EF0 | sub_82627EF0 | FM2_Lua_CRotationNew | 0.93 | Registered as `Rotation.new`; allocates via `sub_827EB358`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82625A30 | sub_82625A30 | Large 16-float component `set` implementation; defer dedicated pass. |
| 0x826269D8 | sub_826269D8 | Matrix `_11` getter; defer m11–m44 accessor cluster. |
| 0x82626A40 | sub_82626A40 | Matrix `_11` setter; defer accessor cluster. |
| 0x82629228 | sub_82629228 | Vector `set` with xyz floats; defer vector method cluster. |
| 0x82628450 | sub_82628450 | Rotation `tostring`; defer rotation method cluster. |
| 0x82627FA8 | sub_82627FA8 | Rotation `.eq`; defer rotation method cluster. |
