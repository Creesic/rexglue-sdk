import json, sys, re
p = sys.argv[1]
start = int(sys.argv[2]) if len(sys.argv) > 2 else 0
end = int(sys.argv[3]) if len(sys.argv) > 3 else 999
raw = open(p, encoding="utf-8").read()
inner = json.loads(json.loads(raw)["stdout"])
for i, x in enumerate(inner["items"]):
    if i < start or i >= end:
        continue
    dc = x["decompiled"]
    # extract interesting strings
    strs = re.findall(r'"[^"]{2,80}"', dc)
    vft = re.findall(r'(?:vftable|off_820[0-9A-Fa-f]+|::[A-Za-z0-9_:]+)', dc)
    print("=" * 60)
    print(i, x["addr"], x["old"])
    if vft:
        print("  vft:", ", ".join(vft[:6]))
    if strs:
        print("  str:", ", ".join(strs[:8]))
    print(dc[:1200])
