import json, re, sys
p = sys.argv[1]
raw = open(p, encoding="utf-8").read()
inner = json.loads(json.loads(raw)["stdout"])
for i, x in enumerate(inner["items"]):
    dc = x["decompiled"]
    s = re.findall(r'"[^"]{3,100}"', dc)
    v = re.findall(r"(?:off_82[0-9A-Fa-f]+|::[A-Za-z0-9_:]+|`vftable'|fmod_[a-z_]+|XAUDIO2::[A-Za-z0-9_]+)", dc)
    if s or v:
        print(f"{i:2d} {x['addr']} {x['old']}")
        if v:
            print("  vft:", ", ".join(dict.fromkeys(v)[:10]))
        if s:
            print("  str:", ", ".join(s[:8]))
