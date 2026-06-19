"""List sub_ functions called from named FM2_ functions (optionally outside emit cluster)."""
import json
import sys

import ida_funcs
import idautils

EMIT_ROOTS = (
    0x8250F7C0,
    0x8236E780,
    0x82383A70,
    0x8236F228,
    0x825B3838,
    0x825B3848,
    0x82371A30,
)


def emit_cluster():
    seen = set()
    stack = list(EMIT_ROOTS)
    while stack:
        cur = stack.pop()
        if cur in seen:
            continue
        seen.add(cur)
        fn = ida_funcs.get_func(cur)
        if not fn:
            continue
        for insn in idautils.FuncItems(cur):
            for ref in idautils.CodeRefsFrom(insn, 0):
                cf = ida_funcs.get_func(ref)
                if cf:
                    stack.append(cf.start_ea)
    return seen


def main():
    outside_only = "--outside-emit" in sys.argv
    cluster = emit_cluster() if outside_only else set()

    sub_callees = {}
    for i in range(ida_funcs.get_func_qty()):
        fn = ida_funcs.getn_func(i)
        if not fn:
            continue
        name = ida_funcs.get_func_name(fn.start_ea)
        if not name.startswith("FM2_") or name.startswith("sub_"):
            continue
        for insn in idautils.FuncItems(fn.start_ea):
            for ref in idautils.CodeRefsFrom(insn, 0):
                cf = ida_funcs.get_func(ref)
                if not cf:
                    continue
                cn = ida_funcs.get_func_name(cf.start_ea)
                if not cn.startswith("sub_"):
                    continue
                ea = cf.start_ea
                if outside_only and ea in cluster:
                    continue
                sub_callees.setdefault(ea, {"callers": set(), "size": cf.end_ea - cf.start_ea})
                sub_callees[ea]["callers"].add(name)

    rows = []
    for ea, info in sub_callees.items():
        rows.append(
            {
                "addr": hex(ea),
                "name": ida_funcs.get_func_name(ea),
                "size": info["size"],
                "caller_count": len(info["callers"]),
                "callers_sample": sorted(info["callers"])[:5],
            }
        )
    rows.sort(key=lambda r: (-r["caller_count"], int(r["addr"], 16)))

    out = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\unnamed-sub-callees.json"
    payload = {"count": len(rows), "outside_emit_only": outside_only, "rows": rows}
    with open(out, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
    print(json.dumps({"count": len(rows), "written": out, "top10": rows[:10]}, indent=2))


if __name__ == "__main__":
    main()
