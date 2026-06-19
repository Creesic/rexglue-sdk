"""Reconstruct random-1000 sample addresses (seed 20260618) and export final names."""
import json
import random
import re

import ida_funcs

SEED = 20260618

BATCH_NAME_PATTERNS = [
    re.compile(r"^FM2_Helper_[0-9A-F]{4}$", re.I),
    re.compile(r"_Caller(?:_\d+)?$"),
    re.compile(r"^FM2_(Thunk|JumpTail|Trap)$"),
    re.compile(
        r"^FM2_(Stl|D3D|Crt|Kernel|XMem|Class|MemCopy|MemSet)_[A-Za-z0-9_]*(Helper|Method|Dtor)(?:_\d+)?$"
    ),
    re.compile(r"^FM2_Str_[A-Za-z0-9_]+$"),
]


def is_batch_heuristic_name(ea: int, name: str) -> bool:
    if not name.startswith("FM2_"):
        return False
    m = re.match(r"^FM2_Helper_([0-9A-F]{4})$", name, re.I)
    if m:
        return int(m.group(1), 16) == (ea & 0xFFFF)
    return any(p.match(name) for p in BATCH_NAME_PATTERNS)


def build_population() -> list[int]:
    population: list[int] = []
    for i in range(ida_funcs.get_func_qty()):
        fn = ida_funcs.getn_func(i)
        if not fn:
            continue
        ea = fn.start_ea
        name = ida_funcs.get_func_name(ea)
        if name.startswith("sub_") or is_batch_heuristic_name(ea, name):
            population.append(ea)
    return population


def main() -> None:
    population = build_population()
    random.seed(SEED)
    sample = sorted(random.sample(population, min(1000, len(population))))
    rows = []
    still_sub = 0
    for ea in sample:
        name = ida_funcs.get_func_name(ea)
        fn = ida_funcs.get_func(ea)
        size = fn.end_ea - fn.start_ea if fn else 0
        if name.startswith("sub_"):
            still_sub += 1
        rows.append({"addr": hex(ea), "name": name, "size": size})

    out_path = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\random-rename-log-1000.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump({"seed": SEED, "count": len(rows), "still_sub": still_sub, "rows": rows}, f, indent=2)

    print(
        json.dumps(
            {
                "population_size": len(population),
                "sample_size": len(sample),
                "still_sub_in_sample": still_sub,
                "written": out_path,
            },
            indent=2,
        )
    )


main()
