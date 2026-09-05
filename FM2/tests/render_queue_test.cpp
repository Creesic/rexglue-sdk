#include <array>
#include <cstdlib>
#include <vector>
#include <rex/logging/types.h>
#include "render/render_queue.h"

// Only logging and GPU dispatch are replaced; all queue methods are production code.
namespace rex {
spdlog::logger* GetLoggerRaw(LogCategoryId) { return nullptr; }
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId{0}; }
}  // namespace rex

namespace {
std::vector<uint8_t> replayedVertices;
std::vector<fm2::render::RenderCommand> replayedCommands;
void Check(bool condition) {
  if (!condition)
    std::abort();
}
}  // namespace

namespace fm2::render {
void DispatchRenderCommand(const RenderCommand&) { std::abort(); }
void DispatchRecordedRenderCommands(const RenderCommand* commands, size_t count,
                                    const DeferredExecutionSnapshot*) {
  replayedCommands.assign(commands, commands + count);
  if (count != 1 || commands[0].type != RenderCommandType::DrawPrimitiveUP)
    return;
  const auto& draw = commands[0].drawPrimitiveUP;
  replayedVertices.assign(draw.vertexData, draw.vertexData + draw.bytes);
}
}  // namespace fm2::render

int main() {
  using namespace fm2::render;
  std::array<uint8_t, 4> vertices{1, 2, 3, 4};
  RenderCommand draw{};
  draw.type = RenderCommandType::DrawPrimitiveUP;
  draw.drawPrimitiveUP.vertexData = vertices.data();
  draw.drawPrimitiveUP.bytes = uint32_t(vertices.size());
  draw.drawPrimitiveUP.stride = uint32_t(vertices.size());
  draw.drawPrimitiveUP.vertexCount = 1;

  RenderQueue::BeginRecording(1);
  RenderQueue::Enqueue(draw);
  RenderQueue::EndRecording();
  vertices.fill(9);
  Check(RenderQueue::ReplayRecording(1, nullptr));  // No clone required.
  Check(replayedVertices == std::vector<uint8_t>({1, 2, 3, 4}));

  RenderQueue::BindPendingRecording(2, 1, 0);
  RenderQueue::BindPendingRecording(3, 2, 0);
  RenderQueue::BeginRecording(1);  // Replace source, not its existing clones.
  Check(!RenderQueue::ReplayRecording(1, nullptr));
  RenderQueue::Enqueue(draw);
  RenderQueue::EndRecording();
  Check(RenderQueue::ReplayRecording(1, nullptr));
  Check(replayedVertices == std::vector<uint8_t>(4, 9));
  Check(RenderQueue::ReplayRecording(3, nullptr));
  Check(replayedVertices == std::vector<uint8_t>({1, 2, 3, 4}));

  RenderQueue::BeginRecording(4);  // Discarding pending must not lose finalized source.
  RenderQueue::Enqueue(draw);
  RenderQueue::EndRecording();
  Check(RenderQueue::ReplayRecording(1, nullptr));
  Check(!RenderQueue::ReplayRecording(99, nullptr));

  // Native SetTexture leaves no guest packets for CreateTextureFixup's
  // scanner. Distinct handles must still reach clone-local replacements.
  RenderCommand bind{};
  bind.type = RenderCommandType::SetTexture;
  bind.setTexture = {2, nullptr, 0xA000};
  RenderQueue::BeginRecording(10);
  RenderQueue::RecordPendingCommandBufferMarker(100);
  RenderQueue::Enqueue(bind);
  bind.setTexture = {3, nullptr, 0xB000};
  RenderQueue::Enqueue(bind);
  RenderQueue::RecordPendingCommandBufferMarker(200);
  bind.setTexture = {4, nullptr, 0xA000};
  RenderQueue::Enqueue(bind);
  RenderQueue::EndRecording();

  // Registration addresses its actual source, not whichever buffer was
  // most recently finalized on this producer thread.
  RenderQueue::BeginRecording(11);
  RenderQueue::EndRecording();
  auto registerFixup = [](uint32_t source, uint32_t texture, uint32_t start, uint32_t stop) {
    return RenderQueue::RegisterRecordingTextureFixup(source, UINT32_MAX, texture, start, stop);
  };
  Check(registerFixup(99, 0xA000, 0, 0) == UINT32_MAX);
  Check(registerFixup(10, 0xC000, 0, 0) == UINT32_MAX);
  Check(registerFixup(10, 0xA000, 999, 0) == UINT32_MAX);
  Check(registerFixup(10, 0xA000, 200, 100) == UINT32_MAX);
  const uint32_t first = registerFixup(10, 0xA000, 100, 200);
  const uint32_t second = registerFixup(10, 0xB000, 0, 0);
  const uint32_t last = registerFixup(10, 0xA000, 200, 0);
  Check(RecordedRenderBatch::IsNativeTextureFixup(first));
  Check(RecordedRenderBatch::IsNativeTextureFixup(second));
  Check(RecordedRenderBatch::IsNativeTextureFixup(last));
  Check(first != second && first != last && second != last);
  Check(!RecordedRenderBatch::IsNativeTextureFixup(UINT32_MAX));
  Check(!RecordedRenderBatch::IsNativeTextureFixup(0x47000000));

  RenderQueue::BindPendingRecording(20, 10, 0);
  RenderQueue::BindPendingRecording(21, 20, 0);
  bind.setTexture = {0, nullptr, 0xC000};
  Check(RenderQueue::SetRecordingTextureFixup(21, first, bind));
  bind.setTexture.guestAddress = 0xD000;
  Check(RenderQueue::SetRecordingTextureFixup(21, second, bind));
  bind.setTexture.guestAddress = 0xE000;
  Check(RenderQueue::SetRecordingTextureFixup(21, last, bind));
  Check(!RenderQueue::SetRecordingTextureFixup(21, UINT32_MAX, bind));
  Check(RenderQueue::ReplayRecording(21, nullptr));
  Check(replayedCommands.size() == 3);
  Check(replayedCommands[0].setTexture.index == 2 &&
        replayedCommands[0].setTexture.guestAddress == 0xC000);
  Check(replayedCommands[1].setTexture.index == 3 &&
        replayedCommands[1].setTexture.guestAddress == 0xD000);
  Check(replayedCommands[2].setTexture.index == 4 &&
        replayedCommands[2].setTexture.guestAddress == 0xE000);
  for (uint32_t source : {10u, 20u}) {
    Check(RenderQueue::ReplayRecording(source, nullptr));
    Check(replayedCommands[0].setTexture.guestAddress == 0xA000);
    Check(replayedCommands[1].setTexture.guestAddress == 0xB000);
    Check(replayedCommands[2].setTexture.guestAddress == 0xA000);
  }
}
