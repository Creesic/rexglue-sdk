#include <cstddef>
#include <cstdint>
#include <cstdlib>

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
  using fm2::render::GetConstantSnapshotRange;
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

  return 0;
}
