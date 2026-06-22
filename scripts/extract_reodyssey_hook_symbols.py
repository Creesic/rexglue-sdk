"""Extract ReOdyssey d3d_hooks REX_HOOK symbol names."""
import re

HOOKS = r"C:\Users\Tera\Documents\GitHub\ReOdyssey\src\render\d3d_hooks.cpp"
pat = re.compile(r"REX_HOOK\(([^,\s]+)")
symbols = []
with open(HOOKS, encoding="utf-8") as f:
    for line in f:
        if "REX_HOOK(" not in line or line.strip().startswith("//"):
            continue
        m = pat.search(line)
        if m:
            symbols.append(m.group(1))

out = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\reodyssey_hook_symbols.json"
import json
with open(out, "w", encoding="utf-8") as f:
    json.dump(sorted(set(symbols)), f, indent=2)
print(len(set(symbols)))
