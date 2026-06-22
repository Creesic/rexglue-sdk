
## Iteration 26

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825509D8 | sub_825509D8 | lua_string_sub | 0.93 | Returns substring with optional start/end (negative indices normalized). String lib "sub" off_82046A08. |
| 0x82550AA0 | sub_82550AA0 | lua_string_reverse | 0.92 | Reverses string bytes into temp buffer. String lib "reverse". |
| 0x82550B48 | sub_82550B48 | lua_string_lower | 0.92 | FM2_Char_ToLowerAscii per byte. String lib "lower". |
| 0x82550BF8 | sub_82550BF8 | lua_string_upper | 0.92 | sub_82757FC0 upper per byte. String lib "upper". |
| 0x82550CA8 | sub_82550CA8 | lua_string_rep | 0.91 | Repeats string n times via buffer append. String lib "rep". |
| 0x82550D18 | sub_82550D18 | lua_string_byte | 0.91 | Returns byte values for index range; error "string slice too long". String lib "byte". |
| 0x82550E28 | sub_82550E28 | lua_string_char | 0.92 | Concatenates byte args 1..n into string (validates 0..255). String lib "char". |
| 0x82550F00 | sub_82550F00 | lua_string_dump | 0.90 | Dumps Lua function bytecode via sub_824B7B88; error "unable to dump given function". String lib "dump". |
| 0x825528C8 | sub_825528C8 | lua_math_abs | 0.93 | FM2_Lua_ToNumberStrict + __fabs push. Math lib entry near 0x82046D00. |
| 0x82551E50 | sub_82551E50 | lua_string_gfind_deprecated_stub | 0.95 | Errors "'string.gfind' was renamed to 'string.gmatch'". String lib "gfind" legacy slot. |
| 0x82552908 | sub_82552908 | lua_math_sin | 0.94 | ToNumberStrict + sin helper push. Math lib off_82046D00 "sin". |
| 0x82552988 | sub_82552988 | lua_math_cos | 0.94 | FM2_FMOD_NormalizeSinLookupInput path + push. Math lib "cos". |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82552FA8 | sub_82552FA8 | math.min; renamed immediately after this table (see Iteration 27). |
