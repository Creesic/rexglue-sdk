#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>

#include "render/render_commands.h"
#include "render/guest_device.h"

namespace {

void Check(bool condition) {
  if (!condition)
    std::abort();
}

}  // namespace

int main() {
  using fm2::render::ConstantSnapshotRange;
  using fm2::render::DrawGeometrySnapshot;
  using fm2::render::GetConstantSnapshotRange;
  using fm2::render::PendingShaderConstantFile;
  using fm2::render::RenderCommand;
  using fm2::render::RenderCommandType;

  Check(offsetof(fm2::render::GuestDevice, vertexShaderFloatConstants) == 0x700);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderFloatConstants) == 0x1700);
  Check(offsetof(fm2::render::GuestDevice, vertexShaderBoolConstants) == 0x2700);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderBoolConstants) == 0x2710);
  Check(offsetof(fm2::render::GuestDevice, vertexShaderIntConstants) == 0x2720);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderIntConstants) == 0x2760);

  {
    const ConstantSnapshotRange range = GetConstantSnapshotRange(0, 64);
    Check(range.size == 0);
  }
  {
    const ConstantSnapshotRange range =
        GetConstantSnapshotRange((uint64_t{1} << 63) | (uint64_t{1} << 60), 64);
    Check(range.index == 0);
    Check(range.size == 4 * 64);
  }
  {
    const ConstantSnapshotRange range =
        GetConstantSnapshotRange((uint64_t{1} << 63) | uint64_t{1}, 56);
    Check(range.index == 0);
    Check(range.size == 56 * 64);
  }
  {
    const ConstantSnapshotRange range = GetConstantSnapshotRange(uint64_t{1} << 7, 56);
    Check(range.size == 0);
  }

  {
    PendingShaderConstantFile pending;
    uint32_t destination[PendingShaderConstantFile::kRegisterCount *
                         PendingShaderConstantFile::kDwordsPerRegister];
    for (uint32_t i = 0; i < std::size(destination); ++i)
      destination[i] = 0xA0000000u + i;

    const uint32_t first[] = {0x10, 0x11, 0x12, 0x13, 0x20, 0x21, 0x22, 0x23};
    const uint32_t replacement[] = {0x30, 0x31, 0x32, 0x33};
    pending.Stage(2, first, 2);
    pending.Stage(3, replacement, 1);
    Check(!pending.empty());
    pending.OverlayAndClear(destination, PendingShaderConstantFile::kRegisterCount);
    Check(pending.empty());
    Check(destination[1 * 4] == 0xA0000004u);
    Check(destination[2 * 4] == 0x10u);
    Check(destination[2 * 4 + 3] == 0x13u);
    Check(destination[3 * 4] == 0x30u);
    Check(destination[3 * 4 + 3] == 0x33u);
    Check(destination[4 * 4] == 0xA0000010u);

    const uint32_t clipped[] = {0x40, 0x41, 0x42, 0x43, 0x50, 0x51, 0x52, 0x53};
    pending.Stage(255, clipped, 2);
    pending.OverlayAndClear(destination, PendingShaderConstantFile::kRegisterCount);
    Check(destination[255 * 4] == 0x40u);
    Check(destination[255 * 4 + 3] == 0x43u);
  }

  RenderCommand command{};
  command.type = RenderCommandType::SetBooleans;
  command.setBooleans.booleans = 0x12345678;
  Check(command.setBooleans.booleans == 0x12345678);

  command.type = RenderCommandType::SetLoopConstants;
  command.setLoopConstants.values[0] = 0x00010203;
  command.setLoopConstants.values[16] = 0x00040506;
  Check(command.setLoopConstants.values[0] == 0x00010203);
  Check(command.setLoopConstants.values[16] == 0x00040506);

  command.type = RenderCommandType::SetSamplerState;
  command.setSamplerState.index = 15;
  command.setSamplerState.data0 = 1;
  command.setSamplerState.data3 = 3;
  command.setSamplerState.data5 = 5;
  Check(command.setSamplerState.index == 15);

  command.type = RenderCommandType::SetVertexShaderConstants;
  command.setShaderConstants.memory = nullptr;
  command.setShaderConstants.index = 0;
  command.setShaderConstants.size = 64;
  Check(command.setShaderConstants.size == 64);

  command.type = RenderCommandType::SetPixelShaderConstants;
  Check(command.type == RenderCommandType::SetPixelShaderConstants);

  command.type = RenderCommandType::SetDrawGeometrySnapshot;
  command.setDrawGeometrySnapshot = DrawGeometrySnapshot{};
  command.setDrawGeometrySnapshot.streams[0].offset = 24;
  command.setDrawGeometrySnapshot.streams[0].stride = 32;
  command.setDrawGeometrySnapshot.streams[0].rawSize = 576;
  Check(command.setDrawGeometrySnapshot.streams[0].offset == 24);
  Check(command.setDrawGeometrySnapshot.streams[0].stride == 32);
  Check(command.setDrawGeometrySnapshot.streams[0].rawSize == 576);
  Check(command.setDrawGeometrySnapshot.streams[1].buffer == nullptr);
  Check(command.setDrawGeometrySnapshot.indexBuffer == nullptr);
  Check(command.setDrawGeometrySnapshot.rawIndexData == nullptr);

  uint8_t unlockSnapshot[8] = {};
  command.type = RenderCommandType::UnlockBuffer16;
  command.unlockBuffer.buffer = nullptr;
  command.unlockBuffer.data = unlockSnapshot;
  command.unlockBuffer.size = sizeof(unlockSnapshot);
  Check(command.unlockBuffer.data == unlockSnapshot);
  Check(command.unlockBuffer.size == sizeof(unlockSnapshot));

  return 0;
}
