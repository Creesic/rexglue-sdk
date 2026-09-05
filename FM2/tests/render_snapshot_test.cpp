#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <array>
#include <bit>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <rex/kernel/xam/game_region.h>

#define NOMINMAX
#include <plume_render_interface_types.h>

#include "render/render_commands.h"
#include "render/guest_device.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/shader_container.h"
#include "render/shaders/rectangle_list.hlsli"

namespace {

void Check(bool condition) {
  if (!condition)
    std::abort();
}

}  // namespace

int main(int argc, char** argv) {
  {
    using namespace fm2::render;
    using plume::RenderFormat;
    Check(CanReceiveColorResolveBlit(RenderFormat::R8_UNORM));
    Check(CanReceiveColorResolveBlit(RenderFormat::B8G8R8A8_UNORM));
    Check(CanReceiveColorResolveBlit(RenderFormat::R16G16B16A16_FLOAT));
    Check(!CanReceiveColorResolveBlit(RenderFormat::BC1_UNORM));
    Check(!CanReceiveColorResolveBlit(RenderFormat::BC4_UNORM));
    Check(!CanReceiveColorResolveBlit(RenderFormat::D32_FLOAT));
    Check(!CanReceiveColorResolveBlit(RenderFormat::UNKNOWN));
  }
  {
    using namespace fm2::render;
    GuestTexture texture;
    texture.width = texture.height = 512;
    texture.levels = 10;
    uint32_t width = 0, height = 0;
    for (uint32_t level = 0; level < 10; ++level) {
      Check(ResolveDestinationExtent(texture, level, 0, width, height));
      Check(width == (512u >> level) && height == width);
      RenderCommand command{};
      command.resolveToTexture.destLevel = level;
      command.resolveToTexture.destSlice = 0;
      const RenderCommand snapshot = command;
      const auto location = plume::RenderTextureCopyLocation::Subresource(
          nullptr, snapshot.resolveToTexture.destLevel, snapshot.resolveToTexture.destSlice);
      Check(location.subresource.mipLevel == level && location.subresource.arrayIndex == 0);
    }
    Check(!ResolveDestinationExtent(texture, 10, 0, width, height));
    Check(!ResolveDestinationExtent(texture, UINT32_MAX, 0, width, height));
    Check(!ResolveDestinationExtent(texture, 0, 1, width, height));
      texture.viewDimension = plume::RenderTextureViewDimension::TEXTURE_CUBE;
      for (uint32_t face = 0; face < 6; ++face) {
        Check(ResolveDestinationExtent(texture, 1, face, width, height));
        Check(IsFullResolveRegion(256, 256, width, height, 0, 0, false, {}));
        Check(IsFullResolveRegion(256, 256, width, height, 0, 0, true,
                                  plume::RenderRect(0, 0, 256, 256)));
        Check(!IsFullResolveRegion(256, 256, width, height, 1, 0, false, {}));
        Check(!IsFullResolveRegion(256, 256, width, height, 0, 0, true,
                                   plume::RenderRect(0, 0, 128, 256)));
        Check(!IsFullResolveRegion(512, 512, width, height, 0, 0, false, {}));
      }
      Check(ResolveDestinationExtent(texture, 9, 5, width, height));
    Check(!ResolveDestinationExtent(texture, 9, 6, width, height));
    texture.width = 0;
    Check(!ResolveDestinationExtent(texture, 0, 0, width, height));
  }
  {
    using namespace fm2::render;
    // Synthetic prepatched layout: FLOAT3 position, DEC3N normal and two
    // SHORT2 UVs, on stream 0 at offsets 0/12/16/20 with a 24-byte stride.
    std::array<uint8_t, 192> bytes{};
    auto word = [&](size_t offset, uint32_t value) {
      value = std::byteswap(value);
      std::memcpy(bytes.data() + offset, &value, 4);
    };
    word(0, 0x102A1141); word(4, 96); word(8, 96); word(24, 36);
    word(36, 0); word(40, 60); word(60, 0); word(64, 4);
    word(72, 1); word(76, 0x3002); word(80, 0x5003); word(84, 0x15004);
    const uint32_t full = (31u << 20) | (2u << 25) | (1u << 19);
    for (uint32_t i = 0; i < 4; ++i) {
      word(108 + i * 12, full);
      word(112 + i * 12, ((i == 0 ? 57u : i == 1 ? 7u : 25u) << 16) |
                            (i ? 0x40000000u : 0u));
      word(116 + i * 12, 6u | ((i ? i + 2 : 0u) << 8));
    }
    auto decl = ParseBakedVertexDeclaration(bytes);
    Check(decl && decl->bakedVertexFetch && decl->vertexElementCount == 4);
    Check(decl->bakedStrides[0] == 24 && decl->vertexStreams[0]);
    Check(decl->vertexElements[1].offset == 12 && decl->vertexElements[1].type == 7);
    Check(decl->vertexElements[3].offset == 20 && decl->vertexElements[3].usageIndex == 1);
    Check(!ParseBakedVertexDeclaration(std::span(bytes).first(191)));
    word(0, 0x102A1101); Check(!ParseBakedVertexDeclaration(bytes));
    word(0, 0x102A1141);
    word(116, 4); Check(!ParseBakedVertexDeclaration(bytes)); // last UV overflows stride
    word(116, 6);
    word(152, 6 | (0x7FFFFFu << 8)); Check(!ParseBakedVertexDeclaration(bytes));
    word(152, 6 | (5u << 8));
    word(112, 0); Check(!ParseBakedVertexDeclaration(bytes)); // unpatched format
    word(112, 57u << 16);
    word(76, 0x3FFF); Check(!ParseBakedVertexDeclaration(bytes)); // code bounds
    word(76, 0x3002);
    word(60, UINT32_MAX); Check(!ParseBakedVertexDeclaration(bytes)); // table overflow
  }
  // Optional local corpus proof; raw title shader blobs remain ignored.
  if (argc == 2) {
    unsigned baked = 0;
    for (const auto& entry : std::filesystem::directory_iterator(argv[1])) {
      if (entry.path().extension() != ".bin")
        continue;
      std::ifstream input(entry.path(), std::ios::binary);
      std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{input}, {});
      Check(bytes.size() >= fm2::render::kShaderContainerHeaderBytes);
      if ((fm2::render::ShaderContainerWord(bytes, 0) & 0x41) != 0x41)
        continue;
      Check(fm2::render::ParseBakedVertexDeclaration(bytes) != nullptr);
      ++baked;
    }
    Check(baked != 0);
    std::printf("Validated %u baked vertex layouts\n", baked);
  }
  {
    using namespace fm2::render;
    Check(ShaderObjectPrefixBytes(true) == 0x368);
    Check(ShaderObjectPrefixBytes(false) == 0x28);
    for (bool vertex : {false, true}) {
      std::array<uint8_t, 80> virtualPart{};
      std::array<uint8_t, 24> physicalPart{};
      auto word = [&](size_t offset, uint32_t value) {
        value = std::byteswap(value);
        std::memcpy(virtualPart.data() + offset, &value, 4);
      };
      word(0, 0x102A1100u | uint32_t(vertex));
      word(4, uint32_t(virtualPart.size()));
      word(8, uint32_t(physicalPart.size()));
      word(0x18, 0x24);  // shader record inside the virtual portion
      word(0x24, 4);     // microcode offset inside the physical portion
      word(0x28, 12);
      physicalPart.fill(0xA5);
      const auto joined = AssembleShaderContainer(virtualPart, physicalPart, vertex);
      Check(joined.size() * 4 == virtualPart.size() + physicalPart.size());
      Check(std::memcmp(joined.data(), virtualPart.data(), virtualPart.size()) == 0);
      Check(std::memcmp(reinterpret_cast<const uint8_t*>(joined.data()) + virtualPart.size(),
                        physicalPart.data(), physicalPart.size()) == 0);
      Check(AssembleShaderContainer(virtualPart, physicalPart, !vertex).empty());
      Check(AssembleShaderContainer(std::span(virtualPart).first(35), physicalPart, vertex).empty());
      Check(AssembleShaderContainer(virtualPart, std::span(physicalPart).first(23), vertex).empty());
      word(4, UINT32_MAX);
      Check(!GetShaderContainerSizes(virtualPart, vertex));
      word(4, uint32_t(virtualPart.size()));
      word(0x18, 76);
      Check(AssembleShaderContainer(virtualPart, physicalPart, vertex).empty());
      word(0x18, 0x24);
      word(0x28, 24);  // offset 4 + size 24 crosses the physical boundary
      Check(AssembleShaderContainer(virtualPart, physicalPart, vertex).empty());
      word(0, 0);
      Check(!GetShaderContainerSizes(virtualPart, vertex));
    }
  }
  {
    using namespace fm2::render;
    XenosTextureInfo shadow;
    shadow.baseAddress = 0xE8F3B000;
    shadow.width = shadow.height = shadow.pitchTexels = 256;
    shadow.format = plume::RenderFormat::R16G16B16A16_FLOAT;
    shadow.gpuFormat = 32;
    shadow.bytesPerBlock = 8;
    shadow.endian = 1;
    shadow.tiled = shadow.valid = true;
    auto postprocess = shadow;
    postprocess.width = postprocess.pitchTexels = 320;
    postprocess.height = 180;
    postprocess.format = plume::RenderFormat::R10G10B10A2_UNORM;
    postprocess.gpuFormat = 54;
    postprocess.bytesPerBlock = 4;
    postprocess.endian = 2;
    GuestTexture recordedShadow, postprocessTexture;
    recordedShadow.descriptorIndex = 246;
    recordedShadow.guestMemoryStale.store(true);
    std::map<XenosTextureInfo, GuestTexture*> aliases;
    for (unsigned frame = 0; frame < 100; ++frame) {
      Check(aliases.try_emplace(shadow, &recordedShadow).first->second == &recordedShadow);
      Check(aliases.try_emplace(postprocess, &postprocessTexture).first->second ==
            &postprocessTexture);
      Check(aliases.at(shadow)->descriptorIndex == 246);
      Check(!aliases.at(shadow)->NeedsGuestUpload(true, frame));
    }
    Check(aliases.size() == 2);
    // Same host shape is insufficient if the guest byte interpretation differs.
    for (unsigned field = 0; field < 7; ++field) {
      auto different = shadow;
      switch (field) {
        case 0: different.baseAddress += 0x1000; break;
        case 1: different.mipAddress = 0xE9000000; break;
        case 2: ++different.pitchTexels; break;
        case 3: different.gpuFormat = 29; break;
        case 4: different.endian = 2; break;
        case 5: different.cube = true; break;
        case 6: different.tiled = false; break;
      }
      Check(!aliases.contains(different));
    }
    // Run516: full-frame resolve/read fetches differ only by this unused bit.
    auto reader = postprocess;
    reader.packedMips = true;
    reader.NormalizeStorageIdentity(4);  // ceil(log2(min(320,180))) - 4
    Check(reader == postprocess);
    Check(aliases.at(reader) == &postprocessTexture);
    // The flag remains storage-significant when a chain reaches its tail,
    // including a small base level. Preserve pitch/endian/shape distinctions.
    reader = shadow;
    reader.packedMips = true;
    reader.mipMaxLevel = 4;  // 256x256 packed tail
    reader.mipLevels = reader.mipMaxLevel + 1;
    reader.NormalizeStorageIdentity(4);
    Check(reader.packedMips);
    --reader.mipMaxLevel;
    --reader.mipLevels;
    reader.NormalizeStorageIdentity(4);
    Check(!reader.packedMips);
    reader = shadow;
    reader.width = 16;
    reader.height = 8;
    reader.packedMips = true;
    reader.NormalizeStorageIdentity(0);
    Check(reader.packedMips);
  }
  {
    using namespace fm2::render;
    using plume::RenderFormat;
    using plume::RenderSwizzle;
    // race6 E24240 and oracle E300 have byte-identical geometry. Native
    // sampler1 was null because raw DXT5A lightmaps were rejected as format59.
    struct FormatCase { uint32_t guest; RenderFormat host; uint32_t block, bytes; };
    const FormatCase cases[] = {
        {2, RenderFormat::R8_UNORM, 1, 1},
        {6, RenderFormat::B8G8R8A8_UNORM, 1, 4},
        {7, RenderFormat::R10G10B10A2_UNORM, 1, 4},
        {54, RenderFormat::R10G10B10A2_UNORM, 1, 4},
        {10, RenderFormat::R8G8_UNORM, 1, 2},
        {18, RenderFormat::BC1_UNORM, 4, 8},
        {19, RenderFormat::BC2_UNORM, 4, 16},
        {20, RenderFormat::BC3_UNORM, 4, 16},
        {26, RenderFormat::R16G16B16A16_UNORM, 1, 8},
        {29, RenderFormat::R16G16B16A16_FLOAT, 1, 8},
        {32, RenderFormat::R16G16B16A16_FLOAT, 1, 8},
        {59, RenderFormat::BC4_UNORM, 4, 8},
    };
    for (const auto& c : cases) {
      Check(XenosTextureStorageFormat(c.guest) == c.host);
      Check(plume::RenderFormatBlockWidth(c.host) == c.block);
      Check(plume::RenderFormatSize(c.host) == c.bytes);
    }
    Check(XenosTextureStorageFormat(58) == RenderFormat::UNKNOWN);  // DXT3A needs decoding.
    Check(XenosTextureStorageFormat(60) == RenderFormat::UNKNOWN);  // CTX1 is not BC4.
    Check(XenosTextureStorageFormat(UINT32_MAX) == RenderFormat::UNKNOWN);
    const auto scalar = TranslatedTextureComponentMapping(XenosTextureStorageFormat(59));
    Check(scalar.r == RenderSwizzle::R && scalar.g == RenderSwizzle::R &&
          scalar.b == RenderSwizzle::R && scalar.a == RenderSwizzle::R);
    const auto luminance = TranslatedTextureComponentMapping(RenderFormat::R8_UNORM);
    Check(luminance.r == RenderSwizzle::R && luminance.g == RenderSwizzle::R &&
          luminance.b == RenderSwizzle::R && luminance.a == RenderSwizzle::ONE);
    const auto color = TranslatedTextureComponentMapping(RenderFormat::BC3_UNORM);
    Check(color.r == RenderSwizzle::IDENTITY && color.g == RenderSwizzle::IDENTITY &&
          color.b == RenderSwizzle::IDENTITY && color.a == RenderSwizzle::IDENTITY);
  }
  {
    // Xenos formats 7/54 use packed 32-bit RGBA, including resolve targets.
    // Plume's format metadata drives upload pitch and RT eligibility.
    constexpr auto format = plume::RenderFormat::R10G10B10A2_UNORM;
    static_assert(plume::RenderFormatSize(format) == 4);
    static_assert(plume::RenderFormatBlockWidth(format) == 1);
    static_assert(!plume::RenderFormatIsDepth(format));
    Check(plume::RenderFormatBlockWidth(plume::RenderFormat::BC3_UNORM) == 4);
  }
  {
    using namespace fm2::render;
    GuestSurface surface(ResourceType::RenderTarget);
    surface.refCount.store(7);
    surface.width = 1280;
    surface.height = 720;
    surface.format = plume::RenderFormat::R8G8B8A8_UNORM;
    surface.sampleCount = plume::RenderSampleCount::COUNT_1;
    surface.descriptorIndex = 91;
    std::memset(surface.guestHeader, 0xCD, sizeof(surface.guestHeader));
    // Exact destructive writes made by D3D::SetSurfaceHeader at8236BA3C/BA54.
    std::array<uint32_t, kGuestResourceHeaderBytes / 4> xdkHeader{};
    xdkHeader[0] = std::byteswap(4u);
    xdkHeader[1] = std::byteswap(1u);
    xdkHeader[3] = std::byteswap(0x12345678u);
    xdkHeader[11] = std::byteswap(1280u * 720u * 4u);
    surface.UpdateGuestHeader(xdkHeader.data());
    Check(IsFm2Resource(&surface));
    Check(surface.refCount.load() == 7);
    Check(surface.type == ResourceType::RenderTarget && surface.descriptorIndex == 91);
    Check(std::memcmp(surface.guestHeader, reinterpret_cast<uint8_t*>(xdkHeader.data()) + 8,
                      0x30 - 8) == 0);
    for (size_t i = 0x30 - 8; i < sizeof(surface.guestHeader); ++i)
      Check(surface.guestHeader[i] == 0xCD);  // not part of an XDK surface
    Check(SurfaceHostHeight(1280, 256) == 720);
    Check(SurfaceHostHeight(1280, 512) == 720);
    Check(SurfaceHostHeight(1280, 720) == 720);
    Check(SurfaceHostHeight(1280, 240) == 240);  // movie target, not a tile
    Check(SurfaceHostHeight(512, 256) == 256);
    Check(SurfaceHostHeight(512, 512) == 512);  // shadow target
    Check(surface.MatchesHostLayout(1280, SurfaceHostHeight(1280, 256), surface.format,
                                    plume::RenderSampleCount::COUNT_1));
    Check(surface.MatchesHostLayout(1280, SurfaceHostHeight(1280, 512), surface.format,
                                    plume::RenderSampleCount::COUNT_1));
    Check(!surface.MatchesHostLayout(640, 720, surface.format, surface.sampleCount));
    Check(!surface.MatchesHostLayout(1280, 480, surface.format, surface.sampleCount));
    Check(!surface.MatchesHostLayout(1280, 720, plume::RenderFormat::R16G16B16A16_FLOAT,
                                     surface.sampleCount));
    Check(!surface.MatchesHostLayout(1280, 720, surface.format, plume::RenderSampleCount::COUNT_2));
    surface.type = ResourceType::DepthStencil;
    surface.UpdateGuestHeader(xdkHeader.data());
    Check(IsFm2Resource(&surface) && surface.type == ResourceType::DepthStencil);
    Check(surface.refCount.load() == 7);
  }
  {
    using namespace fm2::render;
    // 825A2398 creates 32x32 black/white defaults with Levels=0, then
    // fills every level returned by GetLevelCount. The placeholder XDK
    // header is zero, but the native allocation has six mip levels.
    GuestTexture texture;
    texture.width = texture.height = 32;
    texture.levels = std::bit_width(texture.width);
    Check(GetTextureLevelCount(&texture) == 6);
    uint32_t filledTexels = 0;
    for (uint32_t mip = 0; mip < GetTextureLevelCount(&texture); ++mip)
      filledTexels += (texture.width >> mip) * (texture.height >> mip);
    Check(filledTexels == 1365);  // 32^2 + 16^2 + 8^2 + 4^2 + 2^2 + 1.
    texture.levels = 1;
    Check(GetTextureLevelCount(&texture) == 1);
    texture.type = ResourceType::VolumeTexture;
    texture.levels = 4;
    Check(GetTextureLevelCount(&texture) == 4);
    GuestBuffer buffer(ResourceType::VertexBuffer);
    Check(GetTextureLevelCount(&buffer) == 0);
    Check(GetTextureLevelCount(nullptr) == 0);

    // Preserve the original four-instruction getter for raw XDK textures,
    // including unrelated bits and the exact +0x2C (not +0x28) offset.
    alignas(uint32_t) std::array<uint8_t, 0x34> raw{};
    for (uint32_t maxMip = 0; maxMip < 16; ++maxMip) {
      const uint32_t dword4 = std::byteswap(0xFFFFFC3Fu | (maxMip << 6));
      std::memcpy(raw.data() + 0x2C, &dword4, sizeof(dword4));
      Check(GetTextureLevelCount(raw.data()) == maxMip + 1);
    }
  }

  {
    using fm2::render::RectangleFirstVertex;
    using fm2::render::RectangleFourthComponent;
    // Oracle saving E228: first rectangle after the VS and generated corner
    // after the GS. Rotate the input order so all three diagonals are tested.
    const float corners[3][4] = {{-1, 1, 0, 1}, {-0.75f, 1, 0, 1}, {-1, -1, 0, 1}};
    const float uv[3][4] = {{0, 0, 0, 0}, {0.125f, 0, 160, 0}, {0, 1, 0, 720}};
    const float expectedPosition[4] = {-0.75f, -1, 0, 1};
    const float expectedUv[4] = {0.125f, 1, 160, 720};
    for (int rotation = 0; rotation < 3; ++rotation) {
      float edges[3];
      for (int i = 0; i < 3; ++i) {
        const auto& a = corners[(rotation + i + 1) % 3];
        const auto& b = corners[(rotation + i + 2) % 3];
        const float dx = b[0] - a[0], dy = b[1] - a[1];
        edges[i] = dx * dx + dy * dy;
      }
      const int first = (RectangleFirstVertex(edges[0], edges[1], edges[2]) + rotation) % 3;
      Check(first == 0);
      for (int lane = 0; lane < 4; ++lane) {
        Check(RectangleFourthComponent(corners[first][lane], corners[(first + 1) % 3][lane],
                                       corners[(first + 2) % 3][lane]) == expectedPosition[lane]);
        Check(RectangleFourthComponent(uv[first][lane], uv[(first + 1) % 3][lane],
                                       uv[(first + 2) % 3][lane]) == expectedUv[lane]);
      }
    }
    Check(RectangleFirstVertex(0, 0, 0) == 2);
    Check(RectangleFirstVertex(2, 2, 1) == 1);
    Check(RectangleFourthComponent(0.2f, 0.4f, 0.3f) == 0.5f);
  }

  {
    // Begin/End vertices: data is filled after Begin, consumed once at End,
    // then owned by the existing recorded UP-command path.
    fm2::render::PendingVertexWrites pending;
    std::array<uint8_t, 72> vertices{};
    fm2::render::PendingVertexWrite write{vertices.data(), 4, 6, 12, 0x10, 0x20};
    Check(!pending.Begin(0, write));
    auto invalid = write;
    invalid.data = nullptr;
    Check(!pending.Begin(1, invalid));
    invalid = write;
    invalid.vertexCount = UINT32_MAX;
    Check(!pending.Begin(1, invalid));
    invalid = write;
    invalid.stride = 0;
    Check(!pending.Begin(1, invalid));
    Check(pending.Begin(1, write));
    Check(!pending.Begin(1, write));
    Check(pending.Begin(2, write));
    Check(!pending.End(3));
    vertices[0] = 42;
    const auto ready = pending.End(1);
    Check(ready.has_value());
    Check(ready->primitiveType == 4 && ready->vertexCount == 6 && ready->stride == 12);
    Check(ready->vsDirtyFlags == 0x10 && ready->psDirtyFlags == 0x20);
    Check(static_cast<const uint8_t*>(ready->data)[0] == 42);
    Check(!pending.End(1));
    Check(pending.End(2).has_value());
    fm2::render::RenderCommand command{};
    command.type = fm2::render::RenderCommandType::DrawPrimitiveUP;
    command.drawPrimitiveUP.vertexData = vertices.data();
    command.drawPrimitiveUP.bytes = ready->vertexCount * ready->stride;
    fm2::render::RecordedRenderBatch batch;
    batch.Append(command);
    vertices[0] = 99;
    Check(batch.commands()[0].drawPrimitiveUP.vertexData[0] == 42);
  }

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

  {
    RecordedRenderBatch batch;
    RenderCommand color{}, depth{};
    color.type = RenderCommandType::SetRenderTarget;
    color.setRenderTarget.caller = 0x8251BB48;
    depth.type = RenderCommandType::SetDepthStencilSurface;
    depth.setDepthStencilSurface.caller = 0x8251BB50;
    batch.Append(color);
    batch.Append(depth);
    std::vector<uint8_t> payload;
    const auto replay = batch.BuildReplayCommands(payload);
    Check(replay.size() == 2);
    Check(replay[0].setRenderTarget.caller == 0x8251BB48);
    Check(replay[1].setDepthStencilSurface.caller == 0x8251BB50);
    Check(replay[0].setRenderTarget.renderTarget == nullptr);
    Check(replay[1].setDepthStencilSurface.depthStencil == nullptr);
  }

  {
    using namespace fm2::render;
    GuestSurface color(ResourceType::RenderTarget), lastColor(ResourceType::RenderTarget);
    GuestSurface depth(ResourceType::DepthStencil);
    color.width = lastColor.width = 1280;
    color.height = lastColor.height = 720;
    depth.width = depth.height = 512;
    depth.format = plume::RenderFormat::D32_FLOAT_S8_UINT;
    Check(SelectResolveSource(0, &color, &depth, &lastColor) == &color);
    Check(SelectResolveSource(0, nullptr, &depth, &lastColor) == &lastColor);
    Check(SelectResolveSource(4, &color, &depth, &lastColor) == &depth);
    Check(SelectResolveSource(4, nullptr, &depth, &lastColor) == &depth);
    Check(SelectResolveSource(4, &color, nullptr, &lastColor) == nullptr);

    // The flag must survive reusable recorded command buffers unchanged.
    RenderCommand resolve{};
    resolve.type = RenderCommandType::ResolveToTexture;
    resolve.resolveToTexture.flags = 4 | 0x200;
    resolve.resolveToTexture.postClearFlags = D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
    RecordedRenderBatch batch;
    batch.Append(resolve);
    resolve.resolveToTexture.flags = 0;
    const auto& recorded = batch.commands()[0].resolveToTexture;
    Check(recorded.flags == 0x204);
    Check(recorded.postClearFlags == (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL));
    Check(SelectResolveSource(recorded.flags, &color, &depth, &lastColor) == &depth);

    const plume::RenderViewport oversized(0, 0, 65535, 65535, 0, 1);
    auto viewport = ClampViewportToSurface(oversized, &depth);
    Check(viewport.width == 512 && viewport.height == 512);
    viewport = ClampViewportToSurface(oversized, &color);
    Check(viewport.width == 1280 && viewport.height == 720);
    viewport = ClampViewportToSurface(plume::RenderViewport(400, 300, 256, 64, 1, 0), &depth);
    Check(viewport.x == 400 && viewport.y == 300 && viewport.width == 112 && viewport.height == 64);
    Check(viewport.minDepth == 1 && viewport.maxDepth == 0);
    viewport = ClampViewportToSurface(plume::RenderViewport(600, 0, 32, 32), &depth);
    Check(viewport.width == 0 && viewport.height == 0);
    viewport = ClampViewportToSurface(plume::RenderViewport(559, 503, 163, 47), &color);
    Check(viewport.width == 163 && viewport.height == 47);  // Saving UI unchanged.
    viewport = ClampViewportToSurface(plume::RenderViewport(0, 0, 1280, 256), &color);
    Check(viewport.width == 1280 && viewport.height == 256);  // Tile expansion remains at flush.

    GuestTexture dest;
    dest.width = dest.height = 512;
    dest.depth = 1;
    dest.format = depth.format;
    Check(CanCopyDepthSurface(depth, dest));
    depth.sampleCount = plume::RenderSampleCount::COUNT_2;
    Check(!CanCopyDepthSurface(depth, dest));  // Never use a color MSAA resolve for depth.
    depth.sampleCount = plume::RenderSampleCount::COUNT_1;
    dest.width = 256;
    Check(!CanCopyDepthSurface(depth, dest));
    dest.width = 512;
    dest.levels = 2;
    Check(!CanCopyDepthSurface(depth, dest));
    dest.levels = 1;
    dest.viewDimension = plume::RenderTextureViewDimension::TEXTURE_CUBE;
    Check(!CanCopyDepthSurface(depth, dest));
    dest.viewDimension = plume::RenderTextureViewDimension::TEXTURE_2D;
    dest.format = plume::RenderFormat::R16G16B16A16_FLOAT;
    Check(!CanCopyDepthSurface(depth, dest));
    Check(!CanCopyDepthSurface(color, dest));
  }

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
    const uint32_t colorTarget = std::byteswap(0x81234560u);
    const std::array<uint32_t, 4> viewportWords = {
        std::byteswap(12u), std::byteswap(24u), std::byteswap(1280u), std::byteswap(720u)};
    std::memcpy(context.data() + DeferredExecutionSnapshot::kViewportOffset,
                viewportWords.data(), sizeof(viewportWords));
    const uint32_t depthTarget = std::byteswap(0x89ABCDF0u);
    std::memcpy(context.data() + 0x2F80, &colorTarget, sizeof(colorTarget));
    std::memcpy(context.data() + 0x2F90, &depthTarget, sizeof(depthTarget));
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
    Check(snapshot.guestColorTarget == 0x81234560u);
    Check(snapshot.guestDepthTarget == 0x89ABCDF0u);
    Check((snapshot.viewport == std::array<float, 6>{12, 24, 1280, 720, 1, 0}));
    fm2::render::GuestSurface executionColor(fm2::render::ResourceType::RenderTarget);
    fm2::render::GuestSurface executionDepth(fm2::render::ResourceType::DepthStencil);
    snapshot.colorTarget = &executionColor;
    snapshot.depthTarget = &executionDepth;
    context.fill(0);
    snapshot.vertexExecutionConstants.coverage[0] = 1;
    snapshot.pixelExecutionConstants.coverage[0] = 2;
    DeferredExecutionSnapshot copied = snapshot;
    snapshot.vertexExecutionConstants.coverage[0] = 0;
    snapshot.pixelExecutionConstants.coverage[0] = 0;
    Check(copied.vertexExecutionConstants.coverage[0] == 1);
    Check(copied.pixelExecutionConstants.coverage[0] == 2);
    Check(copied.guestColorTarget == 0x81234560u);
    Check(copied.guestDepthTarget == 0x89ABCDF0u);
    Check(copied.colorTarget == &executionColor);
    Check(copied.depthTarget == &executionDepth);
    Check((copied.viewport == std::array<float, 6>{12, 24, 1280, 720, 1, 0}));
    Check(CaptureDeferredExecutionSnapshot(snapshot, context.data()));
    Check(snapshot.guestColorTarget == 0);
    Check(snapshot.guestDepthTarget == 0);
    Check(snapshot.colorTarget == nullptr);
    Check(snapshot.depthTarget == nullptr);
    Check((snapshot.viewport == std::array<float, 6>{}));
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

  {
    // Run524: replay receives valid camera uploads, then restores stale host
    // values. A following direct sky draw only dirties high material groups.
    // Exercise the production restore for both VS and PS, including clean gaps
    // and the last valid register, without letting recorded/fixup writes escape.
    for (const uint32_t registerCount : {256u, 224u}) {
      std::array<uint32_t, 256 * 4> saved, live, replay;
      saved.fill(0xDEADBEEF);
      live.fill(0x0000803F);  // Guest big-endian 1.0f, distinct from inherited state.
      replay = saved;
      PendingShaderConstantFile emitted;
      emitted.StageDirtyGroups(0xF000000000000000ull, live.data(), registerCount / 4);
      emitted.Stage(registerCount - 1, live.data(), 1);
      emitted.Overlay(replay.data(), registerCount);
      Check(replay[3 * 4] == live[3 * 4]);

      // Recorded material writes and marker-scoped fixups may overwrite either
      // emitted registers or clean gaps, but neither is the restored baseline.
      replay.fill(0x12345678);
      fm2::render::RestoreDeferredShaderConstants(saved.data(), replay.data(), registerCount,
                                                  &emitted);
      for (uint32_t reg = 0; reg < registerCount; ++reg) {
        const uint32_t expected = reg < 16 || reg == registerCount - 1
                                      ? 0x0000803F : 0xDEADBEEF;
        for (uint32_t lane = 0; lane < 4; ++lane)
          Check(replay[reg * 4 + lane] == expected);
      }
      if (registerCount < 256)
        Check(replay[registerCount * 4] == 0x12345678);  // PS restore stays in bounds.

      // Direct sky material/UV update: no low-register upload follows replay.
      live.fill(0x87654321);
      PendingShaderConstantFile direct;
      direct.StageDirtyGroups(0x0000200000000000ull, live.data(), registerCount / 4);
      direct.Overlay(replay.data(), registerCount);
      Check(replay[3 * 4] == 0x0000803F && replay[11 * 4] == 0x0000803F);
      Check(replay[72 * 4] == 0x87654321);
      Check(replay[32 * 4] == saved[32 * 4]);

      // A later replay with no new emissions must retain the corrected state.
      const auto nextSaved = replay;
      replay.fill(0x12345678);
      PendingShaderConstantFile empty;
      fm2::render::RestoreDeferredShaderConstants(nextSaved.data(), replay.data(), registerCount,
                                                  &empty);
      Check(std::memcmp(replay.data(), nextSaved.data(), registerCount * 16) == 0);
      replay.fill(0x12345678);
      fm2::render::RestoreDeferredShaderConstants(saved.data(), replay.data(), registerCount,
                                                  nullptr);
      Check(std::memcmp(replay.data(), saved.data(), registerCount * 16) == 0);
    }
  }

  fm2::render::GuestTexture uploadState;
  Check(uploadState.NeedsGuestUpload(true, 7));
  uploadState.lastUploadFrame = 7;
  Check(!uploadState.NeedsGuestUpload(true, 7));
  Check(!uploadState.NeedsGuestUpload(false, 8));
  Check(uploadState.NeedsGuestUpload(true, 8));  // Movies still refresh next frame.
  uploadState.guestMemoryStale.store(true, std::memory_order_relaxed);
  Check(!uploadState.NeedsGuestUpload(true, 8));
  // Completing the deferred copy clears sourceSurface, not GPU ownership.
  Check(uploadState.sourceSurface == nullptr);
  Check(!uploadState.NeedsGuestUpload(true, 100));
  fm2::render::GuestTexture resolveBeforeFirstUpload;
  resolveBeforeFirstUpload.guestMemoryStale.store(true, std::memory_order_relaxed);
  Check(!resolveBeforeFirstUpload.NeedsGuestUpload(true, 0));
  {
    using rex::kernel::xam::GameRegionFromCountry;
    Check(GameRegionFromCountry(103) == 0x00FF);  // USA; FM2 ShowESRB.
    Check(GameRegionFromCountry(16) == 0x00FF);   // Canada.
    Check(GameRegionFromCountry(53) == 0x0101);   // Japan.
    Check(GameRegionFromCountry(35) == 0x02FE);   // UK.
    Check(GameRegionFromCountry(0) == 0xFFFF);
    Check(GameRegionFromCountry(17) == 0xFFFF);   // Unassigned country.
    Check(GameRegionFromCountry(110) == 0x03FF);  // Last table entry.
    Check(GameRegionFromCountry(111) == 0xFFFF);
    Check(GameRegionFromCountry(UINT32_MAX) == 0xFFFF);
  }
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
    auto clone = batch;
    auto secondGeneration = clone;
    batch = {};
    clone = {};
    Check(secondGeneration.commands()[0].setShaderConstants.memory[0] == 1);
    Check(secondGeneration.commands()[1].setDrawGeometrySnapshot.streams[0].rawData[0] == 5);
    Check(secondGeneration.commands()[1].setDrawGeometrySnapshot.rawIndexData[0] == 9);
    Check(secondGeneration.commands()[2].drawPrimitiveUP.vertexData[0] == 13);
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

    Check(batch.RegisterTextureFixup(0x55, 0xA000, 0, 0) == 0x55);
    RenderCommand replacement{};
    replacement.type = RenderCommandType::SetTexture;
    replacement.setTexture = {0, live, 0xC000, -3};
    Check(batch.SetTextureFixup(0x55, replacement));
    std::vector<uint8_t> shaderConstantPayload;
    std::vector<RenderCommand> replay = batch.BuildReplayCommands(shaderConstantPayload);
    Check(replay[0].setTexture.index == 1 && replay[0].setTexture.texture == live);
    Check(replay[0].setTexture.exponentAdjust == -3);
    Check(replay[1].setTexture.texture == other);
    Check(replay[2].type == RenderCommandType::SetTexture);
    Check(replay[2].setTexture.index == 3 && replay[2].setTexture.texture == live);
    Check(replay[2].setTexture.exponentAdjust == -3);
    Check(batch.commands()[0].setTexture.texture == initial);
    Check(batch.commands()[2].type == RenderCommandType::SetTextureBase);

    const auto clone = batch;

    replacement.setTexture.texture = nullptr;
    Check(batch.SetTextureFixup(0x55, replacement));
    replay = batch.BuildReplayCommands(shaderConstantPayload);
    const auto clonedReplay = clone.BuildReplayCommands(shaderConstantPayload);
    Check(clonedReplay[0].setTexture.texture == live);
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
    auto clone = batch;
    const uint32_t changedValue[4] = {9, 8, 7, 6};
    Check(batch.SetShaderConstantFixup(0x77, changedValue));
    auto secondGeneration = clone;
    const auto clonedReplay = secondGeneration.BuildReplayCommands(payload);
    Check(std::memcmp(clonedReplay[0].setShaderConstants.memory, pixelValue, sizeof(pixelValue)) == 0);
    Check(clonedReplay[0].setShaderConstants.index == 180 * 4);
    Check(clonedReplay[2].setShaderConstants.index == 4 * 4);
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

  for (int32_t exponent = -32; exponent <= 31; ++exponent) {
    const uint32_t encoded = (uint32_t(exponent) & 63u) << 13;
    Check(fm2::render::TextureFetchExponentAdjust(encoded | 0xFFF81FFFu) == exponent);
  }
  Check(fm2::render::TextureFetchExponentAdjust(0x7A000u) == -3);
  Check(std::exp2(float(fm2::render::TextureFetchExponentAdjust(0x7A000u))) * 8.0f == 1.0f);

  uint8_t unlockSnapshot[8] = {};
  command.type = RenderCommandType::UnlockBuffer16;
  command.unlockBuffer.buffer = nullptr;
  command.unlockBuffer.data = unlockSnapshot;
  command.unlockBuffer.size = sizeof(unlockSnapshot);
  Check(command.unlockBuffer.data == unlockSnapshot);
  Check(command.unlockBuffer.size == sizeof(unlockSnapshot));

  return 0;
}
