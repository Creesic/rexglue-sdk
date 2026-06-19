"""Export rename log for the random-1000 batch (seed 20260618).

Identifies batch-renamed functions by heuristic name patterns applied by
ida_fm2_random_rename_batch.py.
"""
import json
import re

import ida_funcs

PATTERNS = [
    re.compile(r"^FM2_Helper_[0-9A-F]{4}$", re.I),
    re.compile(r"_Caller"),
    re.compile(r"^FM2_(Thunk|JumpTail|Trap)$"),
    re.compile(
        r"^FM2_(Stl|D3D|Crt|Kernel|XMem|Class|Str|MemCopy|MemSet)_"
    ),
]

rows = []
for i in range(ida_funcs.get_func_qty()):
    fn = ida_funcs.getn_func(i)
    if not fn:
        continue
    ea = fn.start_ea
    name = ida_funcs.get_func_name(ea)
    if not name.startswith("FM2_"):
        continue
    if any(p.search(name) for p in PATTERNS):
        if re.match(r"^FM2_Helper_[0-9A-F]{4}$", name, re.I):
            if int(name[-4:], 16) != (ea & 0xFFFF):
                continue
        rows.append({"addr": hex(ea), "name": name, "size": fn.end_ea - fn.start_ea})

rows.sort(key=lambda r: int(r["addr"], 16))
out_path = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\random-rename-log-1000.json"
with open(out_path, "w", encoding="utf-8") as f:
    json.dump({"seed": 20260618, "count": len(rows), "rows": rows}, f, indent=2)

print(json.dumps({"written": out_path, "count": len(rows)}, indent=2))
