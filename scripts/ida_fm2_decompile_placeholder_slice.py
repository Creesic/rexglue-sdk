"""Decompile a slice of batch-4 placeholders for manual rename review."""
import json
import sys

import ida_funcs
import ida_hexrays

INDEX = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\batch4-placeholders.json"
START = int(sys.argv[1]) if len(sys.argv) > 1 else 0
COUNT = int(sys.argv[2]) if len(sys.argv) > 2 else 35

with open(INDEX, encoding="utf-8") as f:
    rows = json.load(f)["rows"]

slice_rows = rows[START : START + COUNT]
out = []
for row in slice_rows:
    ea = int(row["addr"], 16)
    try:
        dec = str(ida_hexrays.decompile(ea))
        if len(dec) > 900:
            dec = dec[:900] + "\n..."
    except Exception as exc:
        dec = str(exc)
    out.append(
        {
            "addr": row["addr"],
            "old": row["name"],
            "size": row["size"],
            "decompiled": dec,
        }
    )

print(json.dumps({"start": START, "count": len(out), "items": out}, indent=2))
