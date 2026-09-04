"""Headless regression for the Arcade card's mesh -> UP -> mesh transition.

Run: python scripts/tests/test_up_vertex_stream.py
Requires clang++ and the generated PGR4 headers. Compiles the actual draw and
declaration functions with a fake upload/command backend; no game launch or GPU.
"""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "pgr4-recomp/src/render/render_state.cpp"


def function(source, name):
    # Top-level definitions in this file close at column zero.
    match = re.search(r"^[\w:* ]+\b" + name + r"\([^;]*?\{.*?^}\n", source, re.M | re.S)
    if match is None:
        raise AssertionError(f"Missing production function: {name}")
    return match.group()


PREAMBLE = r'''
#include <cassert>
#include <cstdio>
#include "render/guest_device.h"
#include "render/guest_resources.h"
#include "render/render_commands.h"
using namespace plume;
using namespace pgr4::render;

namespace pgr4::render { GuestShader::~GuestShader() = default; }
struct {
  uint8_t vertexStrides[16]{};
  GuestShader* vertexShader = nullptr;
  GuestVertexDeclaration* vertexDeclaration = nullptr;
} g_pipelineState;
struct { bool pipelineState = false; uint8_t vertexStreamFirst = 15, vertexStreamLast = 0; }
    g_dirtyStates;
struct { unsigned attemptedDraws = 0; } g_frameTrace;
RenderVertexBufferView g_vertexBufferViews[16];
RenderInputSlot g_inputSlots[16];
GuestVertexDeclaration* g_boundVertexDeclaration = nullptr;
std::vector<GuestVertexDeclaration*> declarations;
auto SnapshotGameDeclarations() { return declarations; }
bool g_hasBoundPipeline = false, g_viewportEnabled = true;
bool deviceLost = false, uploadFails = false, flushFails = false;
bool IsDeviceLost() { return deviceLost; }
template<class T> void SetDirtyValue(bool& dirty, T& dst, T value) {
  if (dst != value) { dst = value; dirty = true; }
}
auto* uploadBuffer = reinterpret_cast<RenderBuffer*>(uintptr_t(1));
struct UploadAllocator {
  RenderBufferReference Upload(const void*, uint32_t, bool) {
    return RenderBufferReference(uploadFails ? nullptr : uploadBuffer);
  }
} allocator;
auto& CurrentUploadAllocator() { return allocator; }
uint32_t PrepareConvertedIndices(uint32_t primitive, uint32_t count) {
  return primitive == D3DPT_QUADLIST ? count / 4 * 6 : 0;
}
void TraceIssuedDraw(uint32_t, const uint32_t*, size_t, const void*, uint32_t) {}
GuestVertexDeclaration* expectedDeclaration = nullptr;
uint32_t expectedStride = 44, expectedBytes = 176;
unsigned draws = 0, indexedDraws = 0, flushes = 0;
struct Commands {
  void drawInstanced(uint32_t count, uint32_t, uint32_t, uint32_t) {
    assert(count == 4);
    assert(g_pipelineState.vertexDeclaration == expectedDeclaration);
    assert(g_inputSlots[0].stride == expectedStride);
    assert(g_vertexBufferViews[0].buffer.ref == uploadBuffer);
    ++draws;
  }
  void drawIndexedInstanced(uint32_t count, uint32_t, uint32_t, int32_t, uint32_t) {
    assert(count == 6);
    drawInstanced(4, 1, 0, 0);
    ++indexedDraws;
  }
} commands;
auto* CommandList() { return &commands; }
'''

FLUSH = r'''
void FlushRenderState(GuestDevice* device, uint32_t) {
  ++flushes;
  // These are the inputs consumed by the real flush's declaration selection,
  // indexed-position descriptors, and vertex-buffer binding.
  assert(ResolveVertexDeclaration(device) == expectedDeclaration);
  assert(g_pipelineState.vertexStrides[0] == expectedStride);
  assert(g_vertexBufferViews[0].buffer.ref == uploadBuffer);
  assert(g_vertexBufferViews[0].size == expectedBytes);
  assert(g_inputSlots[0].stride == expectedStride);
  assert(g_dirtyStates.vertexStreamFirst == 0);
  g_pipelineState.vertexDeclaration = ResolveVertexDeclaration(device);
  g_hasBoundPipeline = !flushFails;
  if (!flushFails) {
    g_dirtyStates.pipelineState = false;
    g_dirtyStates.vertexStreamFirst = 15;
    g_dirtyStates.vertexStreamLast = 0;
  }
}
'''

MAIN = r'''
int main() {
  GuestDevice device{};
  GuestShader shader(ResourceType::VertexShader);
  shader.headerElements = {{D3DDECLUSAGE_POSITION, 0}, {D3DDECLUSAGE_COLOR, 0},
                           {D3DDECLUSAGE_TEXCOORD, 0}};
  g_pipelineState.vertexShader = &shader;
  GuestVertexDeclaration ui, mesh;
  ui.vertexElements.reset(new GuestVertexElement[5]{
      {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, 0},
      {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
      {0, 20, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
      {0, 28, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 1},
      {0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 2}});
  ui.vertexElementCount = 5;
  mesh.vertexElements.reset(new GuestVertexElement[3]{
      {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
      {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_COLOR, 0},
      {0, 28, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0}});
  mesh.vertexElementCount = 3;
  declarations = {&ui, &mesh};
  assert(!DeclarationFitsStreamStride(&ui, 36));
  assert(DeclarationFitsStreamStride(&ui, 44));
  assert(MatchDeclarationForShader(&shader, 36) == &mesh);
  auto* meshBuffer = reinterpret_cast<RenderBuffer*>(uintptr_t(2));
  g_inputSlots[1] = RenderInputSlot(1, 24);
  g_vertexBufferViews[1] = RenderVertexBufferView(RenderBufferReference(meshBuffer), 221760);
  uint8_t vertices[176]{};
  unsigned cases = 0;
  for (uint32_t oldStride : {0u, 8u, 36u, 44u}) {
    for (bool failFlush : {false, true}) {
      for (bool failUpload : {false, true}) {
        for (auto primitive : {D3DPT_TRIANGLESTRIP, D3DPT_QUADLIST}) {
          g_pipelineState.vertexStrides[0] = uint8_t(oldStride);
          g_inputSlots[0] = RenderInputSlot(0, oldStride);
          g_vertexBufferViews[0] = RenderVertexBufferView(RenderBufferReference(meshBuffer, 128), 360);
          g_dirtyStates = {false, 1, 1};
          g_boundVertexDeclaration = expectedDeclaration = &ui;
          flushFails = failFlush;
          uploadFails = failUpload;
          const unsigned before = draws, beforeFlush = flushes, beforeIndexed = indexedDraws;
          ProcDrawPrimitiveUP(&device, primitive, 4, vertices, 44, sizeof(vertices));
          assert(draws == before + (!failFlush && !failUpload));
          assert(indexedDraws == beforeIndexed +
              (!failFlush && !failUpload && primitive == D3DPT_QUADLIST));
          assert(flushes == beforeFlush + !failUpload);
          assert(g_pipelineState.vertexStrides[0] == oldStride);
          assert(g_inputSlots[0].stride == oldStride);
          assert(g_vertexBufferViews[0].buffer.ref == meshBuffer);
          assert(g_vertexBufferViews[0].buffer.offset == 128);
          assert(g_vertexBufferViews[0].size == 360);
          assert(g_inputSlots[1].stride == 24);
          assert(g_vertexBufferViews[1].size == 221760);
          if (!failUpload) assert(g_dirtyStates.vertexStreamFirst == 0);
          if (!failUpload && failFlush) assert(g_dirtyStates.vertexStreamLast == 1);
          // A subsequent mesh draw sees its original stream and declaration.
          if (oldStride == 36) {
            g_boundVertexDeclaration = &mesh;
            assert(ResolveVertexDeclaration(&device) == &mesh);
          }
          ++cases;
        }
      }
    }
  }
  deviceLost = true;
  const unsigned before = g_frameTrace.attemptedDraws;
  ProcDrawPrimitiveUP(&device, D3DPT_QUADLIST, 4, vertices, 44, sizeof(vertices));
  assert(g_frameTrace.attemptedDraws == before);
  std::printf("UP stream regression: %u transitions passed, plus device-lost early exit\n", cases);
}
'''


def test_up_vertex_stream():
    source = SOURCE.read_text(encoding="utf-8")
    functions = ["ConvertDeclType", "DeclTypeByteSize", "DeclarationFitsStreamStride",
                 "DeclarationStream0PackedEnd", "MatchDeclarationForShader",
                 "EffectiveStream0Stride", "ResolveVertexDeclaration"]
    code = PREAMBLE + "\n".join(function(source, name) for name in functions)
    code += FLUSH + function(source, "ProcDrawPrimitiveUP") + MAIN
    with tempfile.TemporaryDirectory(prefix="pgr4-up-stream-") as folder:
        cpp = Path(folder) / "up_stream.cpp"
        exe = Path(folder) / "up_stream.exe"
        cpp.write_text(code, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O0",
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
                        *[f"-I{ROOT / path}" for path in
                          ["include", "thirdparty/plume", "pgr4-recomp", "pgr4-recomp/src"]],
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_up_vertex_stream()
