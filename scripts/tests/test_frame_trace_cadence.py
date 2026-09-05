"""Check that production frame hashing runs only when its output is logged.

Run: python scripts/tests/test_frame_trace_cadence.py (requires clang++).
"""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def test_frame_trace_cadence():
    source = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    functions = []
    for name in ("CollectFrameTrace", "MixFrameTrace", "MixFrameTraceBytes"):
        match = re.search(rf"^(?:bool|void) {name}\(.*?^}}", source, re.M | re.S)
        assert match is not None
        functions.append(match.group())
    program = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
uint64_t g_frameTraceIndex = 0;
unsigned hashCalls = 0;
uint64_t XXH3_64bits(const void* data, size_t size) {
  assert(data != nullptr && size == sizeof(uint64_t));
  ++hashCalls;
  return *static_cast<const uint64_t*>(data);
}
''' + "\n".join(functions) + r'''
int main() {
  constexpr uint64_t payload = 0x123456789abcdef0;
  unsigned collected = 0;
  for (uint64_t frame = 1; frame <= 1000; ++frame) {
    g_frameTraceIndex = frame - 1;
    const bool logged = frame <= 64 || frame == 301 || frame == 601 || frame == 901;
    assert(CollectFrameTrace() == logged);
    uint64_t actual = 7, expected = 7;
    if (logged) {
      ++collected;
      MixFrameTrace(expected, payload);
    }
    // Null on unlogged frames proves the diagnostic never reads the payload.
    MixFrameTraceBytes(actual, logged ? &payload : nullptr, sizeof(payload));
    assert(actual == expected && hashCalls == collected);
  }
  assert(collected == 67);
  std::puts("Frame trace: 1000 frames, 67 logged/hashed, 933 with no payload reads passed");
}
'''
    with tempfile.TemporaryDirectory(prefix="pgr4-frame-trace-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(program, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O2",
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_frame_trace_cadence()
