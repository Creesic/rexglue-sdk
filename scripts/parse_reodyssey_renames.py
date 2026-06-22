"""Parse reodyssey_config.toml named entries to JSON for IDA batch renames."""
import json
import re
import sys

CONFIG = r"C:\Users\Tera\Documents\GitHub\ReOdyssey\reodyssey_config.toml"
OUT = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\reodyssey_ida_renames.json"
LINE_RE = re.compile(r'^(0x[0-9A-Fa-f]+)\s*=\s*\{\s*name\s*=\s*"([^"]+)"\s*\}')

entries = []
with open(CONFIG, encoding="utf-8") as f:
    for line in f:
        m = LINE_RE.match(line.strip())
        if m:
            entries.append({"addr": m.group(1), "name": m.group(2)})

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(entries, f)

print(len(entries))
