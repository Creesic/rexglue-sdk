from pathlib import Path

base = Path(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state")
doc = Path(r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\docs\FM2-ida-renames-2026-06-18.md")

notes = {
    78: "**Apply: 0/33** - targets already named in IDA (jpeg/png/zlib/d3d_texture/io_sink snake_case from prior work).",
    79: "**Apply: 0/33** - targets already named in IDA (lua/render/presentation snake_case from prior work).",
    80: "**Apply: 33/33.**",
    81: "**Apply: 32/33** - 0x82503668 skipped (already memory_frame_alloc_notify_entry_init).",
    82: "**Apply: 33/33.**",
    83: "**Apply: 33/33.**",
}

parts = []
for n in range(78, 84):
    t = (base / f"append_infra_pass{n}.md").read_text(encoding="utf-8")
    lines = t.splitlines()
    lines.insert(2, "")
    lines.insert(3, notes[n])
    parts.append("\n".join(lines))

doc.write_text(doc.read_text(encoding="utf-8").rstrip() + "\n\n" + "\n\n".join(parts) + "\n", encoding="utf-8")
print("appended passes 78-83")
