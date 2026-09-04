#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <array>
#include <bit>
#include <cstring>

#define NOMINMAX
#include <plume_render_interface_types.h>

#include "render/render_commands.h"
#include "render/guest_device.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"

namespace {

void Check(bool condition) {
  if (!condition)
    std::abort();
}

}  // namespace

int main() {
  using fm2::render::CaptureDeferredExecutionSnapshot;
  using fm2::render::ConstantSnapshotRange;
  using fm2::render::DeferredExecutionSnapshot;
  using fm2::render::TextureFetchIsCube;
  using fm2::render::TextureFetchMipAddress;
  using fm2::render::TextureFetchMipLevelCount;
  using fm2::render::TextureFetchMipMaxLevel;

  Check(!TextureFetchIsCube(1u << 9));
  Check(TextureFetchIsCube(3u << 9));
  Check(TextureFetchMipMaxLevel(9u << 6) == 9);
  Check(TextureFetchMipAddress(0xABCDEu << 12) == 0xABCDE000u);
  Check(TextureFetchMipLevelCount(15u << 6, 1u << 12, 1024, 128) == 11);
  Check(TextureFetchMipLevelCount(15u << 6, 1u << 12, 256, 256) == 9);
  Check(TextureFetchMipLevelCount(15u << 6, 0, 1024, 128) == 1);
  using fm2::render::DrawGeometrySnapshot;
  using fm2::render::GetConstantSnapshotRange;
  using fm2::render::InitializeDeferredVertexConstants;
  using fm2::render::NormalizeUnitFullscreenUpQuad;
  using fm2::render::PendingShaderConstantFile;
  using fm2::render::PopConstantSnapshotRange;
  using fm2::render::RecordedRenderBatch;
  using fm2::render::RenderCommand;
  using fm2::render::RenderCommandType;

  Check(offsetof(fm2::render::GuestDevice, vertexShaderFloatConstants) == 0x700);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderFloatConstants) == 0x1700);
  Check(offsetof(fm2::render::GuestDevice, vertexShaderBoolConstants) == 0x2700);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderBoolConstants) == 0x2710);
  Check(offsetof(fm2::render::GuestDevice, vertexShaderIntConstants) == 0x2720);
  Check(offsetof(fm2::render::GuestDevice, pixelShaderIntConstants) == 0x2760);
  Check(offsetof(fm2::render::GuestDevice, viewport) == 0x3168);

  {
    std::array<uint8_t, 4> guestStaging{1, 2, 3, 4};
    std::array<uint8_t, 4> mip0Snapshot = guestStaging;
    RenderCommand mip0{};
    mip0.type = RenderCommandType::UnlockTextureRect;
    mip0.unlockTextureRect.data = mip0Snapshot.data();
    mip0.unlockTextureRect.size = uint32_t(mip0Snapshot.size());
    mip0.unlockTextureRect.level = 0;

    guestStaging = {5, 6, 7, 8};
    std::array<uint8_t, 4> mip1Snapshot = guestStaging;
    RenderCommand mip1{};
    mip1.type = RenderCommandType::UnlockTextureRect;
    mip1.unlockTextureRect.data = mip1Snapshot.data();
    mip1.unlockTextureRect.size = uint32_t(mip1Snapshot.size());
    mip1.unlockTextureRect.level = 1;
    const std::array<RenderCommand, 2> queued{mip0, mip1};

    guestStaging.fill(0);
    Check(queued[0].unlockTextureRect.level == 0);
    Check(queued[1].unlockTextureRect.level == 1);
    Check(queued[0].unlockTextureRect.data[0] == 1);
    Check(queued[1].unlockTextureRect.data[0] == 5);
  }

  {
    std::array<uint8_t, DeferredExecutionSnapshot::kContextBytes> context{};
    context[DeferredExecutionSnapshot::kVsConstantOffset] = 0x11;
    context[DeferredExecutionSnapshot::kVsConstantOffset + 12 * 16] = 0x13;
    context[DeferredExecutionSnapshot::kVsConstantOffset + 32 * 16] = 0x14;
    context[DeferredExecutionSnapshot::kVsConstantOffset + 255 * 16] = 0x15;
    context[DeferredExecutionSnapshot::kVsConstantOffset + 255 * 16 + 15] = 0x12;
    const uint32_t vsBoolean = std::byteswap(0x01020304u);
    const uint32_t psBoolean = std::byteswap(0xA0B0C0D0u);
    const uint32_t minZ = std::byteswap(std::bit_cast<uint32_t>(1.0f));
    const uint32_t maxZ = std::byteswap(std::bit_cast<uint32_t>(0.0f));
    std::memcpy(context.data() + DeferredExecutionSnapshot::kVsBooleanOffset, &vsBoolean,
                sizeof(vsBoolean));
    std::memcpy(context.data() + DeferredExecutionSnapshot::kPsBooleanOffset, &psBoolean,
                sizeof(psBoolean));
    std::memcpy(context.data() + DeferredExecutionSnapshot::kViewportOffset + 16, &minZ,
                sizeof(minZ));
    std::memcpy(context.data() + DeferredExecutionSnapshot::kViewportOffset + 20, &maxZ,
                sizeof(maxZ));

    DeferredExecutionSnapshot snapshot{};
    Check(CaptureDeferredExecutionSnapshot(snapshot, context.data()));
    Check(snapshot.vertexConstants.front() == 0x11);
    Check(snapshot.vertexConstants[12 * 16] == 0x13);
    Check(snapshot.vertexConstants[32 * 16] == 0x14);
    Check(snapshot.vertexConstants[255 * 16] == 0x15);
    Check(snapshot.vertexConstants.back() == 0x12);
    Check(snapshot.vertexExecutionConstants.empty());
    Check(snapshot.pixelExecutionConstants.empty());
    Check(snapshot.booleans[0] == 0x01020304u);
    Check(snapshot.booleans[4] == 0xA0B0C0D0u);
    Check(snapshot.viewportReverseZ == 1);
    context.fill(0);
    snapshot.vertexExecutionConstants.coverage[0] = 1;
    snapshot.pixelExecutionConstants.coverage[0] = 2;
    DeferredExecutionSnapshot copied = snapshot;
    snapshot.vertexExecutionConstants.coverage[0] = 0;
    snapshot.pixelExecutionConstants.coverage[0] = 0;
    Check(copied.vertexExecutionConstants.coverage[0] == 1);
    Check(copied.pixelExecutionConstants.coverage[0] == 2);
    Check(!CaptureDeferredExecutionSnapshot(snapshot, nullptr));
  }

  {
    DeferredExecutionSnapshot snapshot{};
    const uint32_t initial[4] = {1, 2, 3, 4};
    std::memcpy(snapshot.vertexConstants.data() + 32 * 16, initial, sizeof(initial));

    std::array<uint32_t, 256 * 4> replayConstants{};
    InitializeDeferredVertexConstants(snapshot, replayConstants.data(), 256);
    Check(std::memcmp(replayConstants.data() + 32 * 4, initial, sizeof(initial)) == 0);

    uint32_t recorded[4] = {5, 6, 7, 8};
    RenderCommand command{};
    command.type = RenderCommandType::SetVertexShaderConstants;
    command.setShaderConstants.memory = reinterpret_cast<uint8_t*>(recorded);
    command.setShaderConstants.index = 32 * 4;
    command.setShaderConstants.size = sizeof(recorded);
    std::memcpy(replayConstants.data() + command.setShaderConstants.index,
                command.setShaderConstants.memory, command.setShaderConstants.size);
    Check(std::memcmp(replayConstants.data() + 32 * 4, recorded, sizeof(recorded)) == 0);
  }

  fm2::render::GuestTexture uploadState;
  Check(uploadState.NeedsGuestUpload(true, 7));
  uploadState.lastUploadFrame = 7;
  Check(!uploadState.NeedsGuestUpload(true, 7));
  Check(!uploadState.NeedsGuestUpload(false, 8));
  Check(fm2::render::D3DRS_CULLMODE == 56);
  Check(fm2::render::D3DRS_BLENDOPALPHA == 92);
  Check(fm2::render::D3DRS_STENCILENABLE == 108);
  Check(fm2::render::D3DRS_STENCILREF == 132);
  Check(fm2::render::D3DRS_CCWSTENCILREF == 160);
  Check(fm2::render::D3DRS_CCWSTENCILMASK == 164);
  Check(fm2::render::D3DRS_CCWSTENCILWRITEMASK == 168);
  Check(fm2::render::D3DRS_CLIPPLANEENABLE == 172);
  Check(fm2::render::D3DRS_SCISSORTESTENABLE == 200);
  Check(fm2::render::D3DRS_VIEWPORTENABLE == 304);

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
    uint64_t flags = (uint64_t{1} << 63) | (uint64_t{1} << 62) | (uint64_t{1} << 60);
    ConstantSnapshotRange range = PopConstantSnapshotRange(flags, 64);
    Check(range.index == 0 && range.size == 2 * 64);
    range = PopConstantSnapshotRange(flags, 64);
    Check(range.index == 3 * 16 && range.size == 64);
    Check(flags == 0);
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
    Check(pending.DirtyGroups(64) == (uint64_t{1} << 63));
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

    std::array<uint32_t, PendingShaderConstantFile::kRegisterCount *
                             PendingShaderConstantFile::kDwordsPerRegister>
        groupedSource{};
    groupedSource[120 * 4] = 0x12345678u;
    groupedSource[188 * 4] = 0xABCDEF01u;
    groupedSource[240 * 4] = 0xFFFFFFFFu;
    destination[120 * 4] = destination[188 * 4] = destination[240 * 4] = 0;
    pending.StageDirtyGroups((uint64_t{1} << (63 - 30)) | (uint64_t{1} << (63 - 47)) |
                                 (uint64_t{1} << (63 - 60)),
                             groupedSource.data(), 56);
    pending.Overlay(destination, PendingShaderConstantFile::kRegisterCount);
    Check(!pending.empty());
    Check(destination[120 * 4] == 0x12345678u);
    Check(destination[188 * 4] == 0xABCDEF01u);
    Check(destination[240 * 4] == 0);
    pending.OverlayAndClear(destination, PendingShaderConstantFile::kRegisterCount);
    Check(pending.empty());

  }

  RenderCommand command{};
  command.type = RenderCommandType::SetBooleans;
  command.setBooleans.words[0] = 1u;
  command.setBooleans.words[3] = 1u << 31;
  command.setBooleans.words[4] = 1u;
  command.setBooleans.words[7] = 1u << 31;
  const auto boolean = [&](uint32_t address) {
    return (command.setBooleans.words[address / 32] & (1u << (address % 32))) != 0;
  };
  Check(boolean(0));
  Check(boolean(127));
  Check(boolean(128));
  Check(boolean(255));

  command.type = RenderCommandType::SetLoopConstants;
  command.setLoopConstants.values[0] = 0x00010203;
  command.setLoopConstants.values[16] = 0x00040506;
  Check(command.setLoopConstants.values[0] == 0x00010203);
  Check(command.setLoopConstants.values[16] == 0x00040506);

  command.type = RenderCommandType::SetClipPlaneState;
  command.setClipPlaneState.enabled = 1;
  command.setClipPlaneState.plane[3] = 4.0f;
  Check(command.setClipPlaneState.enabled == 1);
  Check(command.setClipPlaneState.plane[3] == 4.0f);

  plume::RenderGraphicsPipelineDesc pipelineDesc;
  pipelineDesc.independentStencilMasksAndReference = true;
  pipelineDesc.stencilReference = 1;
  pipelineDesc.stencilBackReference = 2;
  pipelineDesc.stencilReadMask = 0x0F;
  pipelineDesc.stencilBackReadMask = 0xF0;
  Check(pipelineDesc.stencilReference != pipelineDesc.stencilBackReference);
  Check(pipelineDesc.stencilReadMask != pipelineDesc.stencilBackReadMask);

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

  {
    uint8_t constants[] = {1, 2, 3, 4};
    uint8_t vertices[] = {5, 6, 7, 8};
    uint8_t indices[] = {9, 10, 11, 12};
    uint8_t upVertices[] = {13, 14, 15, 16};
    RecordedRenderBatch batch;

    RenderCommand recorded{};
    recorded.type = RenderCommandType::SetVertexShaderConstants;
    recorded.setShaderConstants.memory = constants;
    recorded.setShaderConstants.size = sizeof(constants);
    batch.Append(recorded);

    recorded = {};
    recorded.type = RenderCommandType::SetDrawGeometrySnapshot;
    recorded.setDrawGeometrySnapshot.streams[0].rawData = vertices;
    recorded.setDrawGeometrySnapshot.streams[0].rawSize = sizeof(vertices);
    recorded.setDrawGeometrySnapshot.rawIndexData = indices;
    recorded.setDrawGeometrySnapshot.rawIndexSize = sizeof(indices);
    batch.Append(recorded);

    recorded = {};
    recorded.type = RenderCommandType::DrawPrimitiveUP;
    recorded.drawPrimitiveUP.vertexData = upVertices;
    recorded.drawPrimitiveUP.bytes = sizeof(upVertices);
    batch.Append(recorded);

    constants[0] = vertices[0] = indices[0] = upVertices[0] = 0xFF;
    Check(batch.commands().size() == 3);
    Check(batch.commands()[0].setShaderConstants.memory != constants);
    Check(batch.commands()[0].setShaderConstants.memory[0] == 1);
    Check(batch.commands()[1].setDrawGeometrySnapshot.streams[0].rawData[0] == 5);
    Check(batch.commands()[1].setDrawGeometrySnapshot.rawIndexData[0] == 9);
    Check(batch.commands()[2].drawPrimitiveUP.vertexData[0] == 13);
  }

  {
    auto* initial = reinterpret_cast<fm2::render::GuestTexture*>(uintptr_t{0x1000});
    auto* other = reinterpret_cast<fm2::render::GuestTexture*>(uintptr_t{0x2000});
    auto* live = reinterpret_cast<fm2::render::GuestTexture*>(uintptr_t{0x3000});
    RecordedRenderBatch batch;

    RenderCommand recorded{};
    recorded.type = RenderCommandType::SetTexture;
    recorded.setTexture = {1, initial, 0xA000};
    batch.Append(recorded);
    recorded.setTexture = {2, other, 0xB000};
    batch.Append(recorded);
    recorded = {};
    recorded.type = RenderCommandType::SetTextureBase;
    recorded.setTextureBase = {3, reinterpret_cast<fm2::render::GuestBaseTexture*>(initial),
                               0xA000};
    batch.Append(recorded);

    Check(batch.AssociateTextureFixup(0x55, 0xA000) == 2);
    RenderCommand replacement{};
    replacement.type = RenderCommandType::SetTexture;
    replacement.setTexture = {0, live, 0xC000};
    Check(batch.SetTextureFixup(0x55, replacement));
    std::vector<uint8_t> shaderConstantPayload;
    std::vector<RenderCommand> replay = batch.BuildReplayCommands(shaderConstantPayload);
    Check(replay[0].setTexture.index == 1 && replay[0].setTexture.texture == live);
    Check(replay[1].setTexture.texture == other);
    Check(replay[2].type == RenderCommandType::SetTexture);
    Check(replay[2].setTexture.index == 3 && replay[2].setTexture.texture == live);
    Check(batch.commands()[0].setTexture.texture == initial);
    Check(batch.commands()[2].type == RenderCommandType::SetTextureBase);

    replacement.setTexture.texture = nullptr;
    Check(batch.SetTextureFixup(0x55, replacement));
    replay = batch.BuildReplayCommands(shaderConstantPayload);
    Check(replay[0].setTexture.texture == nullptr);
    Check(replay[2].setTexture.texture == nullptr);
    Check(batch.commands()[0].setTexture.texture == initial);
  }

  {
    RecordedRenderBatch batch;
    RenderCommand draw{};
    draw.type = RenderCommandType::DrawIndexedPrimitive;

    batch.RecordMarker(10);
    batch.Append(draw);
    batch.RecordMarker(20);
    batch.Append(draw);
    batch.RecordMarker(30);

    Check(batch.AssociateShaderConstantFixup(0x77, true, 180, 1, 10, 20) == 1);
    Check(batch.AssociateShaderConstantFixup(0x88, false, 4, 1, 20, 30) == 1);
    Check(batch.AssociateShaderConstantFixup(0x99, true, 181, 1, 20, 99) == 0);

    const uint32_t pixelValue[4] = {0x3F800000u, 0x40000000u, 0x40400000u, 0x40800000u};
    const uint32_t vertexValue[4] = {1, 2, 3, 4};
    Check(batch.SetShaderConstantFixup(0x77, pixelValue));
    Check(batch.SetShaderConstantFixup(0x88, vertexValue));
    Check(!batch.SetShaderConstantFixup(0x99, pixelValue));

    std::vector<uint8_t> payload;
    const std::vector<RenderCommand> replay = batch.BuildReplayCommands(payload);
    Check(replay.size() == 4);
    Check(replay[0].type == RenderCommandType::ApplyPixelShaderConstantFixup);
    Check(replay[0].setShaderConstants.index == 180 * 4);
    Check(replay[0].setShaderConstants.size == 16);
    Check(std::memcmp(replay[0].setShaderConstants.memory, pixelValue, sizeof(pixelValue)) == 0);
    Check(replay[1].type == RenderCommandType::DrawIndexedPrimitive);
    Check(replay[2].type == RenderCommandType::ApplyVertexShaderConstantFixup);
    Check(replay[2].setShaderConstants.index == 4 * 4);
    Check(std::memcmp(replay[2].setShaderConstants.memory, vertexValue, sizeof(vertexValue)) == 0);
    Check(replay[3].type == RenderCommandType::DrawIndexedPrimitive);
  }

  {
    std::array<uint32_t, 20> vertices{};
    auto store = [&](uint32_t vertex, uint32_t component, float value) {
      vertices[vertex * 5 + component] = std::byteswap(std::bit_cast<uint32_t>(value));
    };
    const float values[4][4] = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
    };
    for (uint32_t vertex = 0; vertex < 4; ++vertex) {
      store(vertex, 0, values[vertex][0]);
      store(vertex, 1, values[vertex][1]);
      store(vertex, 3, values[vertex][2]);
      store(vertex, 4, values[vertex][3]);
    }
    auto read = [&](uint32_t vertex, uint32_t component) {
      return std::bit_cast<float>(std::byteswap(vertices[vertex * 5 + component]));
    };
    std::array<uint32_t, 20> rejected = vertices;
    Check(NormalizeUnitFullscreenUpQuad(reinterpret_cast<uint8_t*>(vertices.data()), 4, 20,
                                        uint32_t(sizeof(vertices))));
    Check(read(0, 0) == -1.0f && read(0, 1) == 1.0f);
    Check(read(3, 0) == 1.0f && read(3, 1) == -1.0f);
    Check(read(0, 3) == 0.0f && read(3, 4) == 1.0f);

    rejected[3 * 5 + 3] = std::byteswap(std::bit_cast<uint32_t>(2.0f));
    Check(!NormalizeUnitFullscreenUpQuad(reinterpret_cast<uint8_t*>(rejected.data()), 4, 20,
                                         uint32_t(sizeof(rejected))));
    Check(!NormalizeUnitFullscreenUpQuad(reinterpret_cast<uint8_t*>(rejected.data()), 3, 20,
                                         uint32_t(sizeof(rejected))));
  }

  uint8_t unlockSnapshot[8] = {};
  command.type = RenderCommandType::UnlockBuffer16;
  command.unlockBuffer.buffer = nullptr;
  command.unlockBuffer.data = unlockSnapshot;
  command.unlockBuffer.size = sizeof(unlockSnapshot);
  Check(command.unlockBuffer.data == unlockSnapshot);
  Check(command.unlockBuffer.size == sizeof(unlockSnapshot));

  return 0;
}
