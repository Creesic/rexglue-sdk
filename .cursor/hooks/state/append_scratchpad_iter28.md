
## Iteration 28

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82552948 | sub_82552948 | lua_math_sinh | 0.94 | ToNumberStrict + sub_827593D0 sinh helper; push result. Math lib "sinh" off_82046D00. |
| 0x825529C8 | sub_825529C8 | lua_math_cosh | 0.94 | ToNumberStrict + sub_82759640 cosh helper; push result. Math lib "cosh". |
| 0x82552A08 | sub_82552A08 | lua_math_tan | 0.93 | ToNumberStrict + tan helper push. Math lib "tan" (decompiler labels helper FM2_Render_SinDegreesFloat_Helper). |
| 0x82552A48 | sub_82552A48 | lua_math_tanh | 0.94 | ToNumberStrict + sub_82757FD8 tanh; push. Math lib "tanh". |
| 0x82552BE8 | sub_82552BE8 | lua_math_floor | 0.93 | ToNumberStrict + FM2_LuaSyntax_RoundDoubleForStringConcat (floor toward -inf). Math lib "floor". |
| 0x82552C28 | sub_82552C28 | lua_math_fmod | 0.94 | Two-arg ToNumberStrict; fmod(x,y) push. Math lib "fmod". |
| 0x82552C88 | sub_82552C88 | lua_math_modf | 0.93 | ToNumberStrict + sub_82759810 split; pushes fractional and integer parts (2 returns). Math lib "modf". |
| 0x82552CF8 | sub_82552CF8 | lua_math_sqrt | 0.95 | ToNumberStrict + __fsqrt push. Math lib "sqrt". |
| 0x82552D38 | sub_82552D38 | lua_math_pow | 0.93 | Two-arg ToNumberStrict; pow helper push. Math lib "pow". |
| 0x82552DD8 | sub_82552DD8 | lua_math_log10 | 0.95 | ToNumberStrict + log10 push. Math lib "log10". |
| 0x82552E18 | sub_82552E18 | lua_math_exp | 0.95 | ToNumberStrict + exp push. Math lib "exp". |
| 0x82552E58 | sub_82552E58 | lua_math_deg | 0.94 | ToNumberStrict * 57.29577951308232 (rad→deg). Math lib "deg". |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82552EA0 | sub_82552EA0 | math.rad; deferred to iter 29 with frexp/max/randomseed cluster. |
| 0x82552EE8 | sub_82552EE8 | math.frexp; same batch next pass. |
| 0x82553020 | sub_82553020 | math.max; same batch next pass. |
| 0x825531F0 | sub_825531F0 | math.randomseed (srand); same batch next pass. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
