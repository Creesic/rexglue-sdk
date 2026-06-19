import json

rows = [
("0x8289a8b8","82A09AF4"),("0x8289b320","82A09DCC"),("0x8289cc38","82A0A060"),("0x8289e030","82A0A0F0"),
("0x8289e780","82A0A154"),("0x8289eb28","82A09EAC"),("0x8289ecd8","82A0A1B0"),("0x828a0bd0","82A0A4DC"),
("0x828a3c78","82A0A204"),("0x828a43d0","82A0A8AC"),("0x828a4580","82A0A664"),("0x828a5cd8","82A0A6B4"),
("0x828a80d8","82A0A748"),("0x828a8680","82A0A9FC"),("0x828a8bd8","82A0AAEC"),("0x828a9328","82A0AC1C"),
("0x828a9640","82A0AAB0"),("0x828a98c8","82A0AC8C"),("0x828a9b98","82A0AB48"),("0x828aa7b0","82A0AB38"),
("0x828ab650","82A0AB54"),("0x828acce0","82A0AFA0"),("0x828ad8b0","82A0AFA4"),("0x828adb80","82A0AD20"),
("0x828af368","82A0AEE0"),("0x828af518","82A0B004"),("0x828b1d18","82A0B3E4"),("0x828b27c8","82A0B09C"),
("0x828b4f78","82A0B708"),("0x828b5248","82A0B734"),("0x828b6910","82A0B5AC"),("0x828b6c70","82A0B72C"),
("0x828b6f88","82A0B4D8"),("0x828b75b8","82A0B4DC"),("0x828b85c8","82A0BB58"),
("0x828b9c90","82A0BDA0"),("0x828ba080","82A0BBEC"),("0x828ba1a0","82A0BCD8"),("0x828ba818","82A0BC04"),
("0x828baa10","82A0BBF0"),("0x828baf68","82A0BDB0"),("0x828bc288","82A0BCA4"),
("0x828bc438","Crt_InitStaticString_82A0BC84"),
("0x828bc5a8","82A0BF00"),("0x828bd298","82A0BE10"),("0x828bdb50","82A0C094"),("0x828c0070","82A0C178"),
("0x828c1308","82A0C2A8"),("0x828c2f28","82A0C210"),("0x828c3948","82A0C358"),
("0x828c5408","Crt_StaticInitPtrPair_82A43B1C"),("0x828c5cc8","Crt_StaticInitPtrPair_82A43C5C"),
("0x828c6160","Crt_StaticInitPtrPair_82A43D04"),
("0x828c6cc0","82A70E50"),("0x828c7578","82A70D08"),("0x828c8388","82A70C30"),("0x828c8580","82A70E28"),
("0x828c8928","82A70F00"),("0x828c9270","82A70FB4"),("0x828c99c0","82A70C7C"),("0x828c9c48","82A70DC4"),
("0x828cb058","82A70C64"),("0x828cb1c0","82A70CFC"),("0x828cb600","82A7112C"),("0x828ccb60","82A71234"),
("0x828cd1d8","82A71134"),("0x828cdd60","82A712B0"),("0x828ce8e8","82A711C4"),("0x828cf278","82A71128"),
("0x828cf8b0","82A716D4"),
]

def name_for(tag):
    if tag.startswith("Crt_"):
        return f"FM2_{tag}"
    return f"FM2_XmlStaticInit_CacheTypeHandle_{tag}"

def reason(a, tag):
    if tag == "Crt_InitStaticString_82A0BC84":
        return "CRT static init: `FM2_Stl_String_InitOrClear(&unk_82A0BC84)` + `atexit(sub_8294D210)`."
    if tag.startswith("Crt_StaticInitPtrPair_"):
        g = tag.split("_")[-1]
        return f"CRT static init: `sub_8276B730` stores ptr+count into `{g.lower()}`."
    return f"Static init caches XML type -> `dword_{tag}`."

items = [{"addr": a, "name": name_for(t), "reason": reason(a, t)} for a, t in rows]
b0 = [{"addr": x["addr"], "name": x["name"]} for x in items[:35]]
b1 = [{"addr": x["addr"], "name": x["name"]} for x in items[35:]]
base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_batch22.json", "w", encoding="utf-8").write(json.dumps(b0))
open(base + r"\rename_batch23.json", "w", encoding="utf-8").write(json.dumps(b1))

md = []
for title, slice_items in [("22", items[:35]), ("23", items[35:])]:
    md.append(f"### Manual re-pass {title} (35 functions)\n")
    md.append("Offset 770+: XML type-handle static init hooks (plus CRT string/ptr-pair inits in pass 23).\n")
    md.append("| Address | New name | Reasoning |")
    md.append("| --- | --- | --- |")
    for x in slice_items:
        md.append(f"| `{x['addr']}` | `{x['name']}` | {x['reason']} |")
    md.append("")
open(base + r"\append_pass22_23.md", "w", encoding="utf-8").write("\n".join(md))
