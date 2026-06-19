import json, sys
p = sys.argv[1]
raw = open(p, encoding="utf-8").read()
d = json.loads(raw)
inner = json.loads(d["stdout"])
for i, x in enumerate(inner["items"]):
    dc = x["decompiled"]
    print(f"{i:2d} {x['addr']} {x['old']} ({x['size']}b)")
