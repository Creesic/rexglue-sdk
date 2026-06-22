"""Apply one batch of ReOdyssey renames in IDA (Lost Odyssey).

Usage (via IDA MCP py_exec_file):
  set BATCH_OFFSET / BATCH_LIMIT below, or pass through globals before exec.
"""
from __future__ import annotations

import json
import sys

import ida_funcs
import ida_name

JSON_PATH = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\reodyssey_ida_renames.json"
BATCH_OFFSET = int(globals().get("BATCH_OFFSET", 0))
BATCH_LIMIT = int(globals().get("BATCH_LIMIT", 800))


def set_func_name(ea: int, name: str) -> tuple[bool, str]:
    if not ida_funcs.get_func(ea):
        return False, "no_func"
    if ida_name.set_name(ea, name, ida_name.SN_CHECK):
        return True, "ok"
    if ida_name.set_name(ea, name, ida_name.SN_FORCE):
        return True, "forced"
    return False, "failed"


with open(JSON_PATH, encoding="utf-8") as f:
    all_entries = json.load(f)

batch = all_entries[BATCH_OFFSET : BATCH_OFFSET + BATCH_LIMIT]
print(f"batch offset={BATCH_OFFSET} limit={BATCH_LIMIT} size={len(batch)} total={len(all_entries)}")

ok = already = no_func = failed = 0
fail_samples: list[str] = []

for item in batch:
    ea = int(item["addr"], 16)
    name = item["name"]
    current = ida_name.get_name(ea)
    if current == name:
        already += 1
        continue
    success, reason = set_func_name(ea, name)
    if success:
        ok += 1
    elif reason == "no_func":
        no_func += 1
    else:
        failed += 1
        if len(fail_samples) < 8:
            fail_samples.append(f"{item['addr']} {name!r} was {current!r}")

print(f"renamed={ok} already={already} no_func={no_func} failed={failed}")
if fail_samples:
    print("fail_samples:")
    for s in fail_samples:
        print(f"  {s}")

next_offset = BATCH_OFFSET + BATCH_LIMIT
done = next_offset >= len(all_entries)
print(f"next_offset={next_offset} done={done}")
