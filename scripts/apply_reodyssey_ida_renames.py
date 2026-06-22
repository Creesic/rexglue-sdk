"""Apply reodyssey_config.toml function names in IDA (Lost Odyssey default.xex).

Run via IDA38 MCP py_exec_file with IDA on port 13338 selected.
"""
from __future__ import annotations

import re
import sys

import ida_funcs
import ida_name
import ida_idaapi

CONFIG_PATH = r"C:\Users\Tera\Documents\GitHub\ReOdyssey\reodyssey_config.toml"
LINE_RE = re.compile(r'^(0x[0-9A-Fa-f]+)\s*=\s*\{\s*name\s*=\s*"([^"]+)"\s*\}')


def parse_config(path: str) -> list[tuple[int, str]]:
    entries: list[tuple[int, str]] = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = LINE_RE.match(line.strip())
            if m:
                entries.append((int(m.group(1), 16), m.group(2)))
    return entries


def set_func_name(ea: int, name: str) -> tuple[bool, str]:
    if not ida_funcs.get_func(ea):
        return False, "no function at address"
    flags = ida_name.SN_CHECK
    if ida_name.set_name(ea, name, flags):
        return True, "ok"
    if ida_name.set_name(ea, name, ida_name.SN_FORCE):
        return True, "forced"
    return False, "rename failed"


def main() -> int:
    entries = parse_config(CONFIG_PATH)
    print(f"parsed {len(entries)} named entries from config")

    ok = 0
    skipped_no_func = 0
    failed = 0
    already = 0
    failures: list[str] = []

    for ea, name in entries:
        current = ida_name.get_name(ea)
        if current == name:
            already += 1
            continue
        success, reason = set_func_name(ea, name)
        if success:
            ok += 1
        elif reason == "no function at address":
            skipped_no_func += 1
        else:
            failed += 1
            if len(failures) < 40:
                failures.append(f"0x{ea:08X} -> {name}: {reason} (was {current!r})")

    print(f"renamed: {ok}")
    print(f"already correct: {already}")
    print(f"skipped (no func): {skipped_no_func}")
    print(f"failed: {failed}")
    if failures:
        print("sample failures:")
        for line in failures:
            print(f"  {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
