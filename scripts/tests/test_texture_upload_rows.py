"""Run the production texture-row conversion against byte-level reference data.

python scripts/tests/test_texture_upload_rows.py
Requires clang++; no GPU or game launch. Also rejects swapping mapped upload
memory in place, even when that would produce identical texture bytes.
"""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

PREAMBLE = r'''
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
namespace rex {
template<class T> T align(T value, T alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}
namespace graphics::texture_util {
namespace xenos { constexpr uint32_t kTextureTileWidthHeight = 32; }
TILED_FUNCTION
}}
SOURCE_HELPERS
uintptr_t uploadBegin, uploadEnd;
void CheckedSwap(uint8_t* data, size_t size, uint32_t endian) {
  assert(uintptr_t(data) + size <= uploadBegin || uintptr_t(data) >= uploadEnd);
  EndianSwapBuffer(data, size, endian);
}
struct Info { uint32_t bytesPerBlock, endian, expand16From; bool tiled; };
struct Layout { uint32_t row_pitch_bytes; };
void Convert(uint8_t* mapped, const uint8_t* sourceData, const Info& info,
             uint32_t widthBlocks, uint32_t heightBlocks, uint32_t packedX,
             uint32_t packedY, const GuestTextureSource* sourceRange,
             uint64_t sourceBaseOffset, uint32_t sourceExtent) {
  const uint32_t guestRowBytes = widthBlocks * info.bytesPerBlock;
  const uint32_t hostRowBytes = widthBlocks * (info.expand16From ? 4 : info.bytesPerBlock);
  const uint32_t hostRowPitch = (hostRowBytes + 255u) & ~255u;
  const uint32_t pitchBlocks = 128, bytesPerBlockLog2 = std::countr_zero(info.bytesPerBlock);
  Layout layout{pitchBlocks * info.bytesPerBlock};
  const auto* sourceLayout = &layout;
  struct { uint64_t srcOffset; } region{512};
  ROW_CONVERSION
}
int main() {
  struct Case { uint32_t width, height, px, py, baseOffset, physical; bool truncate; };
  const Case layouts[] = {
    {67,37,3,5,0,0x80,false}, {64,32,0,0,0,0,false},
    {1,1,1,1,0,0xFFF,false}, {17,19,7,31,257,0xFF9,false},
    {65,65,31,15,128,0x80,true}, {9,3,0,0,0,0xFF9,false}
  };
  unsigned cases = 0;
  for (bool tiled : {false, true})
  for (uint32_t bytes : {1u, 2u, 4u, 8u, 16u})
  for (uint32_t endian : {0u, 1u, 2u})
  for (uint32_t expand : {0u, 4u, 5u}) {
    if (expand && bytes != 2) continue;
    for (bool holes : {false, true})
    for (const auto& layout : layouts) {
      Info info{bytes, endian, expand, tiled};
      const auto [width, height, px, py, baseOffset, physical, truncate] = layout;
      const uint32_t rowBytes = width * bytes;
      const uint32_t hostPitch = (width * (expand ? 4 : bytes) + 255u) & ~255u;
      GuestTextureSource range{physical, 128 * 128 * bytes, {}};
      range.readablePages.resize((range.size + physical + 4095) / 4096, 1);
      if (holes) range.readablePages[1] = 0;
      const uint32_t extent = truncate ? range.size / 2 : range.size - baseOffset;
      std::vector<uint8_t> source(range.size), output(512 + hostPitch * height + 512, 0);
      for (size_t i = 0; i < source.size(); ++i) source[i] = uint8_t(i * 31 + (i >> 8));
      uploadBegin = uintptr_t(output.data()); uploadEnd = uploadBegin + output.size();
      Convert(output.data(), source.data() + baseOffset, info, width, height, px, py,
              &range, baseOffset, extent);
      std::vector<uint8_t> expected(output.size(), 0), row(rowBytes);
      for (uint32_t y = 0; y < height; ++y) {
        std::fill(row.begin(), row.end(), 0);
        for (uint32_t x = 0; x < width; ++x) {
          const uint32_t offset = tiled
              ? rex::graphics::texture_util::GetTiledOffset2D(x+px, y+py, 128, std::countr_zero(bytes))
              : ((y+py)*128 + x+px)*bytes;
          // Independent page-range check, including the unaligned physical base.
          bool readable = offset + bytes <= extent && baseOffset + offset + bytes <= source.size();
          for (uint32_t b = 0; b < bytes && readable; ++b)
            readable = range.readablePages[(physical + baseOffset + offset + b) / 4096] != 0;
          if (readable) std::copy_n(source.data()+baseOffset+offset, bytes, row.data()+x*bytes);
        }
        const size_t group = endian == 1 ? 2 : endian == 2 ? 4 : 1;
        for (size_t i = 0; i + group <= row.size(); i += group)
          std::reverse(row.begin()+i, row.begin()+i+group);
        auto* dst = expected.data() + 512 + y*hostPitch;
        if (!expand) {
          std::copy(row.begin(), row.end(), dst);
        } else {
          for (uint32_t x = 0; x < width; ++x) {
            uint16_t pixel; std::memcpy(&pixel, row.data()+2*x, 2);
            const uint32_t red = pixel >> (expand == 4 ? 10 : 11) & 31;
            const uint32_t green = pixel >> 5 & (expand == 4 ? 31 : 63);
            const uint32_t blue = pixel & 31;
            dst[4*x] = (red << 3) | (red >> 2);
            dst[4*x+1] = expand == 4 ? (green << 3) | (green >> 2) : (green << 2) | (green >> 4);
            dst[4*x+2] = (blue << 3) | (blue >> 2);
            dst[4*x+3] = expand == 5 || (pixel & 0x8000) ? 255 : 0;
          }
        }
      }
      assert(output == expected);
      ++cases;
    }
  }
  std::printf("Texture uploads: %u linear/tiled, endian, expansion, packed/slice offset, partial extent, sparse-page and padding cases passed; no upload-memory byte swaps\n", cases);
}
'''


def test_texture_upload_rows():
    source = (ROOT / "pgr4-recomp/src/render/d3d_resource_hooks.cpp").read_text(encoding="utf-8")
    helpers = source[source.index("void EndianSwapBuffer("):source.index("GuestTextureSource GetGuestTextureSource(")]
    rows = re.search(r"    uint8_t\* destination = mapped \+ region.srcOffset;.*?\n  }\n", source, re.S)
    assert rows is not None
    rows = rows.group().removesuffix("\n  }\n").replace("EndianSwapBuffer(", "CheckedSwap(")
    util = (ROOT / "src/graphics/pipeline/texture/util.cpp").read_text(encoding="utf-8")
    tiled = util[util.index("int32_t GetTiledOffset2D("):util.index("int32_t GetTiledOffset3D(")]
    program = PREAMBLE.replace("TILED_FUNCTION", tiled).replace("SOURCE_HELPERS", helpers).replace("ROW_CONVERSION", rows)
    with tempfile.TemporaryDirectory(prefix="pgr4-texture-rows-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(program, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O2", str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_texture_upload_rows()
