## Iteration 249

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82628738 | sub_82628738 | FM2_Lua_CRotationScale | 0.92 | Registered as `scale`; scales `CRotation` by float; `#ferror in function 'scale'`. |
| 0x82628838 | sub_82628838 | FM2_Lua_CRotationEquals | 0.92 | Registered as `equals`; compares two `const CRotation`; `#ferror in function 'equals'`. |
| 0x82628940 | sub_82628940 | FM2_Lua_CRotationCopy | 0.92 | Registered as `copy`; clones `CRotation` userdata with GC; `VectorCopy` error string. |
| 0x82626D08 | sub_82626D08 | FM2_Lua_CMatrixGetProperty_14 | 0.92 | Pushes float at offset 12; error `invalid 'self' in accessing variable '_14'`. |
| 0x82626D70 | sub_82626D70 | FM2_Lua_CMatrixSetProperty_14 | 0.92 | Sets float at offset 12; `_14` variable assignment errors. |
| 0x82626E18 | sub_82626E18 | FM2_Lua_CMatrixGetProperty_21 | 0.92 | Pushes float at offset 16; error `invalid 'self' in accessing variable '_21'`. |
| 0x82626E80 | sub_82626E80 | FM2_Lua_CMatrixSetProperty_21 | 0.92 | Sets float at offset 16; `_21` variable assignment errors. |
| 0x82629B10 | sub_82629B10 | FM2_Lua_CVector3MinVector | 0.93 | Registered as `minVector`; component-wise min; `Minimize` / `#ferror in function 'minVector'`. |
| 0x82629C20 | sub_82629C20 | FM2_Lua_CVector3MaxVector | 0.93 | Registered as `maxVector`; component-wise max; `Maximize` error string. |
| 0x82629D30 | sub_82629D30 | FM2_Lua_CVector3Linear | 0.92 | Registered as `linear`; lerps two `CVector3` by float t; `InterpolateLinear` error string. |
| 0x82629E80 | sub_82629E80 | FM2_Lua_CVector3Transform | 0.93 | Registered as `transform`; multiplies `CVector3` by `const CMatrix`; `Transform` error string. |
| 0x8262A080 | sub_8262A080 | FM2_Lua_CVector3Add | 0.92 | Registered as `add`; adds `const CVector3`; `#ferror in function 'add'`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262A190 | sub_8262A190 | Vector3 `subtract`; defer with scale/equals cluster. |
| 0x8262A2A0 | sub_8262A2A0 | Vector3 `scale`; defer vector method cluster. |
| 0x8262A3A0 | sub_8262A3A0 | Vector3 `equals`; defer vector method cluster. |
| 0x82626EE8 | sub_82626EE8 | Matrix `_22` getter; defer m22–m44 accessor cluster. |
