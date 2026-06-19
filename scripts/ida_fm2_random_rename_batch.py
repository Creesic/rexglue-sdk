"""IDA script: rename a batch of random sub_ functions using decompiler heuristics.

Usage from IDA MCP py_exec_file:
  Set BATCH_START and BATCH_END below, or pass via environment.

Outputs JSON summary to stdout.
"""
import json
import os
import random
import re

import ida_funcs
import ida_hexrays
import ida_idaapi
import ida_name

BATCH_START = int(os.environ.get("BATCH_START", "0"))
BATCH_END = int(os.environ.get("BATCH_END", "1000"))
SEED = 20260618
PROGRESS_EVERY = 100

_used_names: set[str] = set()


def _uniq(base: str) -> str:
    base = re.sub(r"[^A-Za-z0-9_]", "", base)[:80]
    if not base:
        base = "FM2_UnknownHelper"
    if not (base[0].isalpha() or base[0] == "_"):
        base = "FM2_" + base
    name = base
    suffix = 2
    while (
        name.lower() in _used_names
        or ida_name.get_name_ea(ida_idaapi.BADADDR, name) != ida_idaapi.BADADDR
    ):
        name = f"{base}_{suffix}"
        suffix += 1
    _used_names.add(name.lower())
    return name


def _suggest(ea: int, dec: str, size: int) -> str:
    d = dec
    if "std::" in d:
        if "out_of_range" in d:
            return _uniq("FM2_Stl_OutOfRangeHelper")
        if "length_error" in d:
            return _uniq("FM2_Stl_LengthErrorHelper")
        if "logic_error" in d:
            return _uniq("FM2_Stl_LogicErrorHelper")
        if "basic_string" in d or "Assign" in d or "Reserve" in d:
            return _uniq("FM2_Stl_StringHelper")
        if "vector" in d.lower():
            return _uniq("FM2_Stl_VectorHelper")
        if "_Tree" in d or "map" in d.lower():
            return _uniq("FM2_Stl_TreeHelper")
        return _uniq("FM2_Stl_Helper")
    if "D3D::" in d or "D3D_" in d:
        if "Hang" in d:
            return _uniq("FM2_D3D_HangHelper")
        if "CommandBuffer" in d or "Submit" in d or "PM4" in d:
            return _uniq("FM2_D3D_CommandBufferHelper")
        if "Resource" in d or "Texture" in d:
            return _uniq("FM2_D3D_ResourceHelper")
        if "Shader" in d:
            return _uniq("FM2_D3D_ShaderHelper")
        return _uniq("FM2_D3D_RuntimeHelper")
    if "Rtl" in d:
        return _uniq("FM2_Kernel_RtlHelper")
    if "XMem" in d:
        return _uniq("FM2_XMem_Helper")
    if "KeTls" in d or "KeBugCheck" in d:
        return _uniq("FM2_Kernel_KeHelper")
    if "FM2_" in d:
        matches = re.findall(r"FM2_[A-Za-z0-9_]+", d)
        if matches:
            callee = max(set(matches), key=len)
            return _uniq(callee + "_Caller")
    if "__trap" in d and size < 48:
        return _uniq("FM2_Trap")
    if size <= 8:
        if "JUMPOUT" in d:
            return _uniq("FM2_JumpTail")
        return _uniq("FM2_Thunk")
    if "FM2_Memcpy" in d or "memcpy" in d:
        return _uniq("FM2_MemCopyHelper")
    if "memset" in d:
        return _uniq("FM2_MemSetHelper")
    if "malloc" in d or "HeapAlloc" in d or "_heap" in d:
        return _uniq("FM2_Crt_AllocHelper")
    if "free" in d or "HeapFree" in d:
        return _uniq("FM2_Crt_FreeHelper")
    if "lock(" in d or "RtlEnterCriticalSection" in d:
        return _uniq("FM2_Crt_LockHelper")
    if "atexit" in d:
        return _uniq("FM2_Crt_AtExitHelper")
    if "vftable" in d:
        if "Dtor" in d or "~" in d:
            return _uniq("FM2_Class_Dtor")
        return _uniq("FM2_Class_Method")
    m = re.search(r'"([^"\\]{4,40})"', d)
    if m:
        slug = re.sub(r"[^A-Za-z0-9]", "", m.group(1))[:30]
        if len(slug) > 3:
            return _uniq("FM2_Str_" + slug)
    return _uniq(f"FM2_Helper_{ea & 0xFFFF:04X}")


def _build_sample() -> list[int]:
    subs: list[int] = []
    for i in range(ida_funcs.get_func_qty()):
        fn = ida_funcs.getn_func(i)
        if not fn:
            continue
        name = ida_funcs.get_func_name(fn.start_ea)
        if name.startswith("sub_"):
            subs.append(fn.start_ea)
    random.seed(SEED)
    return sorted(random.sample(subs, min(1000, len(subs))))


def main() -> None:
    sample = _build_sample()
    start = max(0, BATCH_START)
    end = min(len(sample), BATCH_END)

    ok_count = 0
    skip_count = 0
    fail_count = 0
    results: list[dict] = []

    for idx, ea in enumerate(sample[start:end], start=start):
        cur = ida_funcs.get_func_name(ea)
        if not cur.startswith("sub_"):
            skip_count += 1
            continue
        fn = ida_funcs.get_func(ea)
        size = fn.end_ea - fn.start_ea if fn else 0
        try:
            dec = str(ida_hexrays.decompile(ea))
        except Exception as exc:
            dec = str(exc)
        new_name = _suggest(ea, dec, size)
        if ida_name.set_name(ea, new_name, ida_name.SN_CHECK):
            ok_count += 1
            results.append({"addr": hex(ea), "old": cur, "new": new_name})
        else:
            fail_count += 1
        if (idx + 1) % PROGRESS_EVERY == 0:
            print(f"PROGRESS {idx + 1}/{end}", flush=True)

    print(
        json.dumps(
            {
                "batch_start": start,
                "batch_end": end,
                "ok": ok_count,
                "skip": skip_count,
                "fail": fail_count,
                "examples": results[:20],
            },
            indent=2,
        )
    )

    log_path = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\random-rename-log-1000.json"
    with open(log_path, "w", encoding="utf-8") as f:
        json.dump({"seed": SEED, "count": len(results), "rows": results}, f, indent=2)


main()
