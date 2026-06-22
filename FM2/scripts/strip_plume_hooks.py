#!/usr/bin/env python3
from pathlib import Path

PATH = Path(__file__).resolve().parents[1] / "src" / "fm2_hooks.cpp"

def drop_functions_containing(lines: list[str], needles: tuple[str, ...]) -> list[str]:
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if (line.startswith("void ") or line.startswith("bool ")) and "(" in line:
            sig = line
            j = i + 1
            while j < len(lines) and "{" not in sig:
                sig += lines[j]
                j += 1
            if any(n in sig for n in needles):
                depth = 0
                started = False
                k = i
                while k < len(lines):
                    for ch in lines[k]:
                        if ch == "{":
                            depth += 1
                            started = True
                        elif ch == "}":
                            depth -= 1
                    k += 1
                    if started and depth == 0:
                        break
                i = k
                continue
        out.append(line)
        i += 1
    return out


def main() -> None:
    lines = PATH.read_text(encoding="utf-8").splitlines(keepends=True)
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if '#include "native_renderer/' in line:
            i += 1
            continue
        if line.strip() == "namespace fm2nr = fm2::native_renderer;":
            i += 1
            continue
        # Drop contiguous plume cvar block starting at first fm2_plume cvar.
        if "REXCVAR_DEFINE" in line and "fm2_plume_" in line:
            while i < len(lines):
                if (
                    i + 1 < len(lines)
                    and lines[i + 1].startswith("REXCVAR_DEFINE")
                    and "fm2_plume_" not in lines[i + 1]
                ):
                    break
                if (
                    i + 1 < len(lines)
                    and not lines[i + 1].startswith("REXCVAR_DEFINE")
                    and "fm2_plume_" not in lines[i + 1]
                    and lines[i].strip().endswith(");")
                ):
                    i += 1
                    break
                i += 1
            continue
        if "g_plume_" in line or "RememberedDirectReplayPlan" in line:
            i += 1
            continue
        if line.strip().startswith("constexpr uint32_t kRememberedDirectReplayPlanCount"):
            i += 1
            continue
        out.append(line)
        i += 1

    needles = (
        "Plume",
        "DirectDraw",
        "DirectReplay",
        "DirectPlume",
        "fm2nr",
        "native_renderer",
        "ShaderAnalysis",
        "CompiledState",
        "VertexFetchReplay",
        "D3DCommandContext",
    )
    out = drop_functions_containing(out, needles)

    PATH.write_text("".join(out), encoding="utf-8")
    print("done", PATH)


if __name__ == "__main__":
    main()
