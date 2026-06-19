import json

rows = [
("0x8290a908","82A74A0C"),("0x8290aa70","82A74C80"),("0x8290b568","82A74A3C"),("0x8290b688","82A74C8C"),
("0x8290c258","82A74A58"),("0x8290de80","82A74E20"),("0x8290e8a0","82A74DE4"),("0x82910598","82A74FB8"),
("0x82910af0","82A74F90"),("0x82910e20","82A75238"),("0x82911180","82A75120"),("0x82911570","82A75218"),
("0x82911a38","82A75194"),("0x82912800","82A75230"),("0x82913928","82A75090"),("0x82915d30","82A75458"),
("0x82915fb8","82A754F0"),("0x82917ef0","82A75524"),("0x82919b18","82A75B14"),("0x8291a070","82A758B0"),
("0x8291a220","82A758F8"),("0x8291a388","82A75AF4"),("0x8291b468","82A7589C"),("0x8291b540","82A758AC"),
("0x8291c500","82A75974"),("0x8291d5a0","82A75DF0"),("0x8291e1b8","82A75F10"),("0x8291e4d0","82A75F68"),
("0x8291ef38","82A75C94"),("0x8291efc8","82A75CA0"),("0x8291fd00","82A75C00"),("0x829202e8","82A75F30"),
("0x82921028","82A76318"),("0x82921850","82A76118"),("0x82922300","82A760E4"),
("0x82922780","82A75F74"),("0x82926910","82A765AC"),("0x82928338","82A76660"),("0x82929300","82A76A04"),
("0x82929930","82A7690C"),("0x82929bb8","82A769BC"),("0x8292a668","82A769F4"),("0x8292a7d0","82A76950"),
("0x8292ba60","82A767FC"),("0x8292c2d0","82A768E4"),("0x8292e720","82A76DB4"),("0x829304a8","82A76DE0"),
("0x8293a920","82A771BC"),("0x8293b070","82A76E6C"),("0x8293c108","82A77054"),("0x8293cc18","82A77480"),
("0x8293e0a0","82A77400"),("0x8293f570","82A772F8"),("0x8293fde0","82A77468"),("0x82941660","82A7784C"),
("0x82942bc0","82A77758"),("0x82943dc0","82A7767C"),("0x82944240","82A77638"),("0x82945568","82A77C64"),
("0x82946018","82A779F0"),("0x829460a8","82A77BBC"),("0x82946378","82A77B04"),("0x82946b58","82A77D0C"),
("0x82948268","82A77D74"),
("0x829495d0","Crt_AtexitDtor_Sub822A7C00_829D8228"),
("0x8294b6d8","Crt_AtexitDtor_Sub825A0430_829F2E90"),
("0x8294bdd0","Crt_AtexitDtor_Sub825A0430_82A00C6C"),
("0x8294d228","Crt_AtexitFreeSmallBlock_82A0BA44"),
]

def name_for(tag):
    if tag.startswith("Crt_"):
        return f"FM2_{tag}"
    return f"FM2_XmlStaticInit_CacheTypeHandle_{tag}"

def reason(tag):
    if tag == "Crt_AtexitDtor_Sub822A7C00_829D8228":
        return "CRT atexit dtor thunk: `sub_822A7C00(&unk_829D8228)` frees block, clears fields, re-inits string."
    if tag == "Crt_AtexitDtor_Sub825A0430_829F2E90":
        return "CRT atexit dtor thunk: `sub_825A0430(&unk_829F2E90)` frees small block + clears triple."
    if tag == "Crt_AtexitDtor_Sub825A0430_82A00C6C":
        return "CRT atexit dtor thunk: `sub_825A0430(&unk_82A00C6C)` frees small block + clears triple."
    if tag == "Crt_AtexitFreeSmallBlock_82A0BA44":
        return "CRT atexit: `FM2_Memory_FreeSmallBlockOrNull(dword_82A0BA44)` then zeroes `82A0BA44..4C`."
    return f"Static init caches XML type -> `dword_{tag}`."

items = [{"addr": a, "name": name_for(t), "reason": reason(t)} for a, t in rows]
base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(f"{base}\\rename_batch26.json", "w", encoding="utf-8").write(json.dumps([{"addr": x["addr"], "name": x["name"]} for x in items[:35]]))
open(f"{base}\\rename_batch27.json", "w", encoding="utf-8").write(json.dumps([{"addr": x["addr"], "name": x["name"]} for x in items[35:]]))

md = []
for title, sl, n in [("26", items[:35], 35), ("27", items[35:], 33)]:
    md.append(f"### Manual re-pass {title} ({n} functions)\n")
    md.append("Final batch-4 placeholders (offset 910+): XML static init + CRT atexit dtors.\n")
    md.append("| Address | New name | Reasoning |")
    md.append("| --- | --- | --- |")
    for x in sl:
        md.append(f"| `{x['addr']}` | `{x['name']}` | {x['reason']} |")
    md.append("")
open(f"{base}\\append_pass26_27.md", "w", encoding="utf-8").write("\n".join(md))
