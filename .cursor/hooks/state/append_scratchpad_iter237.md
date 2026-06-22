## Iteration 237

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8261C660 | sub_8261C660 | FM2_Stl_Ostream_WriteBoolFalse | 0.91 | Variant serializer helper; numpunct facet vtable `+28` with bool `0`; flush/unlock pattern. |
| 0x8261C7A8 | sub_8261C7A8 | FM2_Stl_Ostream_WriteChar | 0.90 | Numpunct facet vtable `+24`; writes `unsigned __int16` char via locale stream sentry. |
| 0x8261C8D8 | sub_8261C8D8 | FM2_Stl_Ostream_WriteBool | 0.91 | Numpunct facet vtable `+28` with caller bool value; used for unsigned-char arrays in variant export. |
| 0x8261CB38 | sub_8261CB38 | FM2_Stl_Ostream_WriteDouble | 0.91 | Numpunct facet vtable `+12` (`FormatDoubleToOstream`); formats `double` to ostream. |
| 0x8261BA40 | sub_8261BA40 | FM2_Stl_Ostream_WriteAlignedByte | 0.89 | Writes single byte with ios field-width padding using fill char; clears width after. |
| 0x8261BD70 | sub_8261BD70 | FM2_Stl_Ostream_WriteStringWithFieldWidth | 0.90 | Writes STL string body with left/right fill padding per ios width/fill. |
| 0x8261DA90 | sub_8261DA90 | FM2_Stl_WideIostream_CtorWithBindingStreambuf | 0.90 | Builds wide iostream with `FM2_BindingScript_Streambuf_CtorWithOpenMode`; vtable `off_8210EF28`. |
| 0x8261D2B8 | sub_8261D2B8 | FM2_BindingScript_PresentationLoader_Ctor | 0.91 | Presentation loader object ctor; vtable `off_8210F028`; font sentinel lists + three strings. |
| 0x8261A7E0 | sub_8261A7E0 | FM2_Lua_ImportBindingLogicTriggerNodeFromXml | 0.92 | Parses logic trigger XML `name`/`xmlref`/`xmlrefactive`; assigns profile variants and child properties. |
| 0x8261AA08 | sub_8261AA08 | FM2_BindingScript_FinalizePresentationUnitStringBindings | 0.91 | Post-load pass from `LoadPresentationXmlV2`; links unit-string subtrees and refreshes material passes. |
| 0x8261AED8 | sub_8261AED8 | FM2_Lua_NumpunctFacet_FormatBoolToOstream | 0.90 | Numpunct vtable slot 8; uses moneypunct true/false strings; grouping/padding write path. |
| 0x8261DB18 | sub_8261DB18 | FM2_BindingScript_FormatProfileVariantToOstream | 0.90 | Large variant-type switch; formats profile/binding values to ostream for XML export. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826194C8 | sub_826194C8 | Numpunct facet dtor wrapper; defer with `sub_82619470` cluster. |
| 0x82619468 | sub_82619468 | One-byte accessor; only xref from SQLite btree open (likely incidental). |
| 0x827D47C8 | sub_827D47C8 | Render/material helper calling `8261AA08`; defer `0x827Dxxxx` pass. |
