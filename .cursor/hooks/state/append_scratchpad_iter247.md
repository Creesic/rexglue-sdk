## Iteration 247

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82629228 | sub_82629228 | FM2_Lua_CVector3SetFromComponents | 0.93 | Registered as `set`; takes xyz floats; `invalid 'self' in function 'Set'`. |
| 0x826293A8 | sub_826293A8 | FM2_Lua_CVector3SetFromOtherVector | 0.92 | Registered as `setVector`; copies `const CVector3`. |
| 0x826294B8 | sub_826294B8 | FM2_Lua_CVector3DistanceSquared | 0.93 | Registered as `distanceSquared`; `DistanceSquared` error string. |
| 0x826295B8 | sub_826295B8 | FM2_Lua_CVector3Distance | 0.92 | Registered as `distance`; calls `point_to_line_distance3d`. |
| 0x826296B8 | sub_826296B8 | FM2_Lua_CVector3LengthSquared | 0.93 | Registered as `lengthSquared`; `LengthSquared` error string. |
| 0x82629780 | sub_82629780 | FM2_Lua_CVector3Length | 0.93 | Registered as `length`; `Length` error string. |
| 0x82629848 | sub_82629848 | FM2_Lua_CVector3Dot | 0.93 | Registered as `dot`; `DotProduct` error string. |
| 0x82629948 | sub_82629948 | FM2_Lua_CVector3Cross | 0.92 | Registered as `cross`; returns new `CVector3` userdata. |
| 0x82629A58 | sub_82629A58 | FM2_Lua_CVector3Normalize | 0.92 | Registered as `normalize`; `Normalize` error string. |
| 0x8262A4A8 | sub_8262A4A8 | FM2_Lua_CVector3Copy | 0.92 | Registered as `copy`; `CLuaBindings::VectorCopy`. |
| 0x826280B0 | sub_826280B0 | FM2_Lua_CRotationSetFromComponents | 0.93 | Registered as `set`; xyz euler floats via `sub_827EB3F0`. |
| 0x82628340 | sub_82628340 | FM2_Lua_CRotationLookAt | 0.92 | Registered as `lookAt`; takes `const CVector3` target. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82625A30 | sub_82625A30 | Large 16-float matrix `set`; defer matrix component setter pass. |
| 0x826269D8 | sub_826269D8 | Matrix `_11` getter; defer m11–m44 accessor cluster. |
| 0x82628230 | sub_82628230 | Rotation `setRotation`; defer with rotation equals/copy cluster. |
| 0x82628450 | sub_82628450 | Rotation `tostring`; defer rotation method cluster. |
| 0x82627FA8 | sub_82627FA8 | Rotation `.eq`; defer rotation method cluster. |
