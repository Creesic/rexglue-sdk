import json, re

# offset 840 decompile globals
rows = [
("0x828d05a0","82A713D8"),("0x828d1e60","82A7149C"),("0x828d1fc8","82A714B4"),("0x828d2178","82A71450"),
("0x828d3528","82A7160C"),("0x828d4148","82A717DC"),("0x828d4340","82A717E0"),("0x828d5858","82A71AF0"),
("0x828d6bc0","82A7181C"),("0x828d8fe0","82A71DF0"),("0x828da978","82A71E10"),("0x828dbe00","82A71C08"),
("0x828dc2d0","82A720CC"),("0x828dd638","82A71FA4"),("0x828ddc20","82A72248"),("0x828de688","82A71F04"),
("0x828de880","82A71FBC"),("0x828df600","82A7210C"),("0x828df6d8","82A72074"),("0x828e0968","82A72678"),
("0x828e23d8","82A723D4"),("0x828e5200","82A72810"),("0x828e6b50","82A72918"),("0x828e71c8","82A726F4"),
("0x828e8500","82A72E28"),("0x828e8ff8","82A72D90"),("0x828e9ce8","82A72E1C"),("0x828ea5e8","82A72DF8"),
("0x828eaa20","82A72BD4"),("0x828eb290","82A72ACC"),("0x828ebc68","82A72D44"),("0x828ec930","82A73038"),
("0x828ecc00","82A731D4"),("0x828ecfa8","82A72FE0"),("0x828ed428","82A730E4"),
("0x828ed980","82A72F48"),("0x828ef438","82A73004"),("0x828efdc8","82A72FBC"),("0x828eff30","82A73158"),
("0x828f0db0","82A73584"),("0x828f1b30","82A733A8"),("0x828f1e48","82A735DC"),("0x828f2a60","82A735B4"),
("0x828f2e98","82A7358C"),("0x828f5138","82A736E0"),("0x828f55b8","82A73770"),("0x828f6068","82A73860"),
("0x828f6608","82A73814"),("0x828f7028","82A736CC"),("0x828f74a8","82A739A8"),("0x828f7a90","82A73638"),
("0x828f82b8","82A738CC"),("0x828fa678","82A73D50"),("0x828fa9d8","82A73B4C"),("0x828fd020","82A73EE8"),
("0x828fdd58","82A73F04"),("0x828fe928","82A73E68"),("0x828fecd0","82A73FA0"),("0x828ff738","82A7411C"),
("0x828ffe88","82A7407C"),("0x82900f68","82A74304"),("0x82901a60","82A74454"),("0x82902750","82A74434"),
("0x82902948","82A744E8"),("0x82902b40","82A744D8"),("0x82903d88","82A74334"),("0x82906928","82A74730"),
("0x829073d8","82A7462C"),("0x82907cd8","82A745A8"),("0x829097e0","82A74B34"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
items = [{"addr": a, "name": f"FM2_XmlStaticInit_CacheTypeHandle_{g}"} for a, g in rows]
for i, tag in enumerate(["24", "25"], start=0):
    sl = items[i*35:(i+1)*35]
    open(f"{base}\\rename_batch{tag}.json", "w", encoding="utf-8").write(json.dumps(sl))

md = []
for title, sl in [("24", items[:35]), ("25", items[35:])]:
    md.append(f"### Manual re-pass {title} (35 functions)\n")
    md.append("XML type-handle static init hooks (offset 840+).\n")
    md.append("| Address | New name | Reasoning |")
    md.append("| --- | --- | --- |")
    for x in sl:
        g = x["name"].split("_")[-1]
        md.append(f"| `{x['addr']}` | `{x['name']}` | Static init caches XML type -> `dword_{g}`. |")
    md.append("")
open(f"{base}\\append_pass24_25.md", "w", encoding="utf-8").write("\n".join(md))
