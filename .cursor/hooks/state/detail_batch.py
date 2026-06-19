import json
import re
import sys

p = sys.argv[1]
indices = [int(x) for x in sys.argv[2:]]
raw = open(p, encoding="utf-8").read()
inner = json.loads(json.loads(raw)["stdout"])
for i in indices:
    x = inner["items"][i]
    dc = x["decompiled"]
    s = re.findall(r'"[^"]{3,100}"', dc)
    v = re.findall(
        r"off_82[0-9A-Fa-f]+|::[A-Za-z0-9_:]+|WebGate|Broadcast|Notification|FeedStart|fontcache",
        dc,
    )
    print("===", i, x["addr"], x["old"], "===")
    if v:
        print("vft:", ", ".join(list(dict.fromkeys(v))[:12]))
    if s:
        print("str:", ", ".join(s[:10]))
    print(dc[:2800])
