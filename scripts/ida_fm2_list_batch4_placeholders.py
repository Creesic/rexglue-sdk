"""List batch-4 heuristic placeholder names for manual re-renaming."""
import json
import re

import ida_funcs


def is_batch4_placeholder(ea: int, name: str) -> bool:
    """True only for batch-4 heuristic placeholders, not batches 1-3 manual names."""
    if re.match(r"^FM2_Helper_([0-9A-F]{4})$", name, re.I):
        return int(name[-4:], 16) == (ea & 0xFFFF)
    if "_Caller" in name:
        return True
    if re.match(
        r"^FM2_(Stl|D3D|Crt|Kernel|XMem|MemCopy|MemSet)_"
        r"[A-Za-z0-9_]*Helper(?:_\d+)?$",
        name,
    ):
        return True
    if name in ("FM2_Thunk", "FM2_JumpTail", "FM2_Trap"):
        return True
    if re.match(r"^FM2_Class_(Method|Dtor)(?:_\d+)?$", name):
        return True
    if re.match(r"^FM2_Str_[A-Za-z0-9_]+$", name):
        return True
    return False


rows = []
for i in range(ida_funcs.get_func_qty()):
    fn = ida_funcs.getn_func(i)
    if not fn:
        continue
    ea = fn.start_ea
    name = ida_funcs.get_func_name(ea)
    if is_batch4_placeholder(ea, name):
        rows.append({"addr": hex(ea), "name": name, "size": fn.end_ea - fn.start_ea})

rows.sort(key=lambda r: int(r["addr"], 16))
out = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\batch4-placeholders.json"
with open(out, "w", encoding="utf-8") as f:
    json.dump({"count": len(rows), "rows": rows}, f, indent=2)
print(json.dumps({"count": len(rows), "written": out, "first10": rows[:10]}, indent=2))
