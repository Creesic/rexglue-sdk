## Iteration 250

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262A190 | sub_8262A190 | FM2_Lua_CVector3Subtract | 0.92 | Registered as `subtract`; subtracts `const CVector3`; `#ferror in function 'subtract'`. |
| 0x8262A2A0 | sub_8262A2A0 | FM2_Lua_CVector3Scale | 0.92 | Registered as `scale`; scales `CVector3` by float; `VectorScale` error string. |
| 0x8262A3A0 | sub_8262A3A0 | FM2_Lua_CVector3Equals | 0.92 | Registered as `equals`; compares two `CVector3`; `#ferror in function 'equals'`. |
| 0x82626F28 | sub_82626F28 | FM2_Lua_CMatrixGetProperty_22 | 0.92 | Pushes float at offset 20; error `invalid 'self' in accessing variable '_22'`. |
| 0x82626F90 | sub_82626F90 | FM2_Lua_CMatrixSetProperty_22 | 0.92 | Sets float at offset 20; `_22` variable assignment errors. |
| 0x82627038 | sub_82627038 | FM2_Lua_CMatrixGetProperty_23 | 0.92 | Pushes float at offset 24; error `invalid 'self' in accessing variable '_23'`. |
| 0x826270A0 | sub_826270A0 | FM2_Lua_CMatrixSetProperty_23 | 0.92 | Sets float at offset 24; `_23` variable assignment errors. |
| 0x82627148 | sub_82627148 | FM2_Lua_CMatrixGetProperty_24 | 0.92 | Pushes float at offset 28; error `invalid 'self' in accessing variable '_24'`. |
| 0x826271B0 | sub_826271B0 | FM2_Lua_CMatrixSetProperty_24 | 0.92 | Sets float at offset 28; `_24` variable assignment errors. |
| 0x8262F760 | sub_8262F760 | FM2_Lua_CLuaObjectElementAttach | 0.93 | Registered in `akclassgraph`/`akscenegraph` as `attach`; two `CLuaObjectElement` args; calls `sub_82635510`. |
| 0x8262F860 | sub_8262F860 | FM2_Lua_CLuaObjectElementDetach | 0.93 | Registered as `detach`; two `CLuaObjectElement` args; calls `sub_82635570`. |
| 0x8262F960 | sub_8262F960 | FM2_Lua_CLuaObjectElementParent | 0.92 | Registered as `parent`; returns parent `CLuaObjectElement` userdata via `sub_826355D0`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82627258 | sub_82627258 | Matrix `_31` getter; defer m31–m44 accessor cluster. |
| 0x82635510 | sub_82635510 | Scene-graph attach backend; defer helper cluster with `sub_82635570`. |
