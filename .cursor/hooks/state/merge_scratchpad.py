from pathlib import Path

root = Path(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume")
scratchpad = root / ".cursor" / "ida-rename-scratchpad.md"
import sys

iter_num = int(sys.argv[1]) if len(sys.argv) > 1 else 41
append = root / ".cursor" / "hooks" / "state" / f"append_scratchpad_iter{iter_num}.md"

text = scratchpad.read_text(encoding="utf-8")
chunk = append.read_text(encoding="utf-8")
marker = f"## Iteration {iter_num}"
if marker not in text:
    scratchpad.write_text(text.rstrip() + "\n\n" + chunk.lstrip(), encoding="utf-8")
    print(f"appended iteration {iter_num}")
else:
    print(f"iteration {iter_num} already present")
