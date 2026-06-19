import json

rows = [
("0x828619b8","82A066B0"),("0x82861d60","82A06724"),("0x82862300","82A067B0"),("0x82862930","82A06980"),
("0x82862a50","82A0689C"),("0x82862a98","82A068F4"),("0x82863940","82A06C28"),("0x82865128","82A06B70"),
("0x828655f0","82A06E10"),("0x828668c8","82A06DA4"),("0x82867f20","82A071E8"),("0x82868e98","82A0720C"),
("0x8286a6c8","82A06EA4"),("0x8286b298","82A06E60"),("0x8286cf98","82A07458"),("0x8286d4f0","82A072A8"),
("0x8286df58","82A07400"),("0x8286ea98","82A07404"),("0x8286f038","82A07398"),("0x8286f620","82A07450"),
("0x8286faf8","82A0798C"),("0x82872c78","82A07938"),("0x828753e8","82A07C00"),("0x828758b0","82A07AD0"),
("0x82876a68","82A07A14"),("0x82876f78","82A07BAC"),("0x82877e28","82A0804C"),("0x828789f8","82A07FD0"),
("0x8287bee8","82A08304"),("0x8287c248","82A08464"),("0x8287c998","82A08138"),("0x8287e7f8","82A08170"),
("0x8287ea80","82A082B0"),("0x8287ed98","82A082E0"),("0x82880038","82A084AC"),
("0x82881550","82A086EC"),("0x82881598","82A084C0"),("0x828823a8","82A08804"),("0x82882ee8","82A0877C"),
("0x82883440","82A08738"),("0x82885b68","82A08A44"),("0x82886468","82A08AA8"),("0x82887230","82A089A4"),
("0x828879c8","82A08AC0"),("0x82887f30","82A08ED0"),("0x828881b8","82A08CB0"),("0x828883f8","82A08E18"),
("0x828895b0","82A08F58"),("0x828898c8","82A08C6C"),("0x8288ae70","82A08DC8"),("0x8288bdf8","82A0920C"),
("0x8288c230","82A09054"),("0x8288c5d8","82A09198"),("0x8288c620","82A0925C"),("0x8288c6b0","82A092C8"),
("0x8288c8a8","82A0907C"),("0x8288ecf0","82A08F94"),("0x82892078","82A093EC"),("0x828922b8","82A09650"),
("0x82892738","82A095DC"),("0x828931a0","82A095C0"),("0x82893590","82A09524"),("0x828936b0","82A09548"),
("0x828939c8","82A096B8"),("0x82894050","82A09818"),("0x82896a80","82A096C0"),("0x82896eb8","82A0977C"),
("0x82898278","82A09CE8"),("0x82898590","82A09DE0"),("0x82898620","82A09D8C"),
]

b0 = [{"addr": a, "name": f"FM2_XmlStaticInit_CacheTypeHandle_{g}"} for a, g in rows[:35]]
b1 = [{"addr": a, "name": f"FM2_XmlStaticInit_CacheTypeHandle_{g}"} for a, g in rows[35:]]
open(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\rename_batch20.json","w").write(json.dumps(b0))
open(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\rename_batch21.json","w").write(json.dumps(b1))

md = []
for title, slice_rows in [("20", rows[:35]), ("21", rows[35:])]:
    md.append(f"### Manual re-pass {title} (35 functions)\n")
    md.append("XML type-handle static init hooks.\n")
    md.append("| Address | New name | Reasoning |")
    md.append("| --- | --- | --- |")
    for a, g in slice_rows:
        n = f"FM2_XmlStaticInit_CacheTypeHandle_{g}"
        md.append(f"| `{a.upper()}` | `{n}` | Static init caches XML type -> `dword_{g}`. |")
    md.append("")
open(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\append_pass20_21.md","w").write("\n".join(md))
