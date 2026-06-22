## Iteration 236

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8275F788 | sub_8275F788 | FM2_LocaleFacet_LockCriticalSection | 0.88 | Sole caller `FM2_LocaleStreambuf_AssignFromAndLockFacet`; forwards to `RtlEnterCriticalSection` on facet CS ptr. |
| 0x8275F790 | sub_8275F790 | FM2_LocaleFacet_UnlockCriticalSection | 0.88 | Called from `FM2_Stl_Ostream_FlushAndUnlockFacet`; jump thunk to `RtlLeaveCriticalSection`. |
| 0x8261D248 | sub_8261D248 | FM2_Lua_NumpunctFacet_FormatPointerToOstream | 0.91 | Numpunct vtable slot 1; `sprintf_s` with `"%p"`; writes via `FM2_Stl_Ostream_WriteFormattedStringWithLocale`. |
| 0x8261D0B8 | sub_8261D0B8 | FM2_Lua_NumpunctFacet_FormatLongDoubleToOstream | 0.91 | Numpunct vtable slot 2; builds float spec with `'L'` modifier; formats long double then writes to ostream. |
| 0x8261CD90 | sub_8261CD90 | FM2_Lua_NumpunctFacet_FormatLongIntegerToOstream | 0.90 | Numpunct vtable slot 6; integer printf spec with `'l'` modifier (`byte_8210F068`). |
| 0x8261CE18 | sub_8261CE18 | FM2_Lua_NumpunctFacet_FormatLongLongIntegerToOstream | 0.90 | Numpunct vtable slot 5; integer printf spec with `'L'` modifier (`byte_8210F06C`). |
| 0x8261CEA0 | sub_8261CEA0 | FM2_Lua_NumpunctFacet_FormatInt64IntegerToOstream | 0.89 | Numpunct vtable slot 4; integer printf spec with `'L'` modifier (`byte_8210F070`). |
| 0x8261B770 | sub_8261B770 | FM2_Stl_Ostream_WriteFormattedStringWithLocale | 0.92 | Applies moneypunct grouping/padding rules; writes prefix/fill/body via ostream sentry helpers. |
| 0x8261C390 | sub_8261C390 | FM2_BindingScript_MergeDeferredBufferIntoFrameAllocMap | 0.90 | Consumes deferred circular-buffer list; inserts default content-type nodes into frame-alloc map; pushes back to circular buffer. |
| 0x8261A550 | sub_8261A550 | FM2_Lua_ImportBindingCustomEventsFromXml | 0.92 | Loads `"customevent"` material node; imports XML children with type/value; maps `xmlid` into hash table at `a1+60`. |
| 0x8261A698 | sub_8261A698 | FM2_Lua_ImportBindingCustomFunctionsFromXml | 0.92 | Loads `"customfunction"` material node; same import pattern as custom events; maps into hash at `a1+72`. |
| 0x8261C248 | sub_8261C248 | FM2_Lua_ImportBindingLogicTriggersFromXml | 0.91 | Imports `"logic"` XML triggers/results via `sub_8261A7E0`; collects result profile ids into vector per trigger node. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261C660 | sub_8261C660 | Numpunct bool formatter via vtable `+28`; defer bool/uint16 cluster. |
| 0x8261C7A8 | sub_8261C7A8 | Numpunct uint16 formatter via vtable `+24`; defer with `8261C660`. |
| 0x8261AA08 | sub_8261AA08 | Large post-import presentation finalize; defer dedicated pass. |
| 0x8261A7E0 | sub_8261A7E0 | Logic trigger/result XML node importer; defer logic cluster. |
| 0x8261DB18 | sub_8261DB18 | Variant-to-string serializer; large switch; defer dedicated pass. |
