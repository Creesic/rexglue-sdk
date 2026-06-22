
## Iteration 27

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82552FA8 | sub_82552FA8 | lua_math_min | 0.93 | Min of arg1 and args 2..n via FM2_Lua_ToNumberStrict loop. Math lib "min" off_82046D00. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82551CF8 | (code label) | Not a function—inline `li r4,1; b lua_string_find_or_gsub` find entry. |
| 0x82551D00 | (code label) | Not a function—inline match entry before FM2_FindAndReplaceDelimitedTextRange. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no side effects. |
