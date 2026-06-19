import json
import re
import sys

raw = sys.stdin.read()
inner = json.loads(json.loads(raw)["stdout"])
rows = []
for x in inner["items"]:
    m = re.search(r"dword_(82A[0-9A-Fa-f]+)", x["decompiled"])
    g = m.group(1) if m else "UNKNOWN"
    rows.append((x["addr"], x["old"], f"FM2_XmlStaticInit_CacheTypeHandle_{g}"))

# IDA rename batches
for batch_start in (0, 35):
    batch = [{"addr": a, "name": n} for a, _, n in rows[batch_start : batch_start + 35]]
    print("BATCH", batch_start, json.dumps(batch))

# markdown pass 20
print("---MD20---")
print("### Manual re-pass 20 (35 functions)\n")
print("XML type-handle static init hooks.\n")
print("| Address | New name | Reasoning |")
print("| --- | --- | --- |")
for a, _, n in rows[:35]:
    g = n.split("_")[-1]
    print(f"| `{a.upper()}` | `{n}` | Static init caches XML type → `dword_{g}`. |")

print("---MD21---")
print("### Manual re-pass 21 (35 functions)\n")
print("XML type-handle static init hooks (continued).\n")
print("| Address | New name | Reasoning |")
print("| --- | --- | --- |")
for a, _, n in rows[35:70]:
    g = n.split("_")[-1]
    print(f"| `{a.upper()}` | `{n}` | Static init caches XML type → `dword_{g}`. |")
