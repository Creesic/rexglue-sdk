#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

#include "render/guest_resources.h"

namespace fm2::render {

// XGRegisterVertexShader/PixelShader retain the original container after
// their mutable runtime prefix; only its physical allocation is separate.
constexpr uint32_t ShaderObjectPrefixBytes(bool vertex) { return vertex ? 0x368u : 0x28u; }
constexpr uint32_t kShaderContainerHeaderBytes = 0x24;

inline uint32_t ShaderContainerWord(std::span<const uint8_t> bytes, size_t offset) {
  uint32_t word;
  std::memcpy(&word, bytes.data() + offset, sizeof(word));
  return std::byteswap(word);
}

struct ShaderContainerSizes {
  uint32_t virtualBytes;
  uint32_t physicalBytes;
};

inline std::optional<ShaderContainerSizes> GetShaderContainerSizes(
    std::span<const uint8_t> header, bool vertex) {
  if (header.size() < kShaderContainerHeaderBytes)
    return std::nullopt;
  const uint32_t flags = ShaderContainerWord(header, 0);
  const uint32_t virtualBytes = ShaderContainerWord(header, 4);
  const uint32_t physicalBytes = ShaderContainerWord(header, 8);
  if ((flags & 0xFFFFFF00u) != 0x102A1100u || bool(flags & 1u) != vertex ||
      virtualBytes < kShaderContainerHeaderBytes || virtualBytes > 0x40000u ||
      physicalBytes == 0 || physicalBytes > 0x40000u - virtualBytes) {
    return std::nullopt;
  }
  return ShaderContainerSizes{virtualBytes, physicalBytes};
}

inline std::vector<uint32_t> AssembleShaderContainer(std::span<const uint8_t> virtualPart,
                                                     std::span<const uint8_t> physicalPart,
                                                     bool vertex) {
  const auto sizes = GetShaderContainerSizes(virtualPart, vertex);
  if (!sizes || virtualPart.size() < sizes->virtualBytes ||
      physicalPart.size() < sizes->physicalBytes)
    return {};
  const uint32_t shaderOffset = ShaderContainerWord(virtualPart, 0x18);
  const uint32_t shaderHeaderBytes = vertex ? 0x24u : 0x20u;
  if ((shaderOffset & 3u) != 0 || shaderOffset < kShaderContainerHeaderBytes ||
      shaderOffset > sizes->virtualBytes || shaderHeaderBytes > sizes->virtualBytes - shaderOffset)
    return {};
  const uint32_t physicalOffset = ShaderContainerWord(virtualPart, shaderOffset);
  const uint32_t shaderBytes = ShaderContainerWord(virtualPart, shaderOffset + 4);
  if (physicalOffset > sizes->physicalBytes || shaderBytes == 0 ||
      shaderBytes > sizes->physicalBytes - physicalOffset)
    return {};
  std::vector<uint32_t> result((sizes->virtualBytes + sizes->physicalBytes + 3u) / 4u);
  auto* bytes = reinterpret_cast<uint8_t*>(result.data());
  std::memcpy(bytes, virtualPart.data(), sizes->virtualBytes);
  std::memcpy(bytes + sizes->virtualBytes, physicalPart.data(), sizes->physicalBytes);
  return result;
}

// D3D::SetPending_Shaders bypasses declaration patching for container flag 0x40.
// Recover that immutable layout once, not by guessing from a draw's stride.
inline std::unique_ptr<GuestVertexDeclaration> ParseBakedVertexDeclaration(
    std::span<const uint8_t> bytes) {
  const auto sizes = GetShaderContainerSizes(bytes, true);
  if (!sizes || !(ShaderContainerWord(bytes, 0) & 0x40u) ||
      bytes.size() < size_t(sizes->virtualBytes) + sizes->physicalBytes)
    return nullptr;
  const uint32_t shader = ShaderContainerWord(bytes, 0x18);
  if ((shader & 3u) || shader < kShaderContainerHeaderBytes ||
      shader > sizes->virtualBytes || sizes->virtualBytes - shader < 36)
    return nullptr;
  const uint32_t physicalOffset = ShaderContainerWord(bytes, shader);
  const uint32_t codeBytes = ShaderContainerWord(bytes, shader + 4);
  const uint32_t tableOffset = ShaderContainerWord(bytes, shader + 24);
  const uint32_t count = ShaderContainerWord(bytes, shader + 28);
  const uint64_t table = uint64_t(shader) + 36 + uint64_t(tableOffset) * 4;
  if (!count || count > 32 || table + uint64_t(count) * 4 > sizes->virtualBytes ||
      physicalOffset > sizes->physicalBytes || !codeBytes ||
      codeBytes > sizes->physicalBytes - physicalOffset)
    return nullptr;
  const size_t code = size_t(sizes->virtualBytes) + physicalOffset;
  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->bakedVertexFetch = true;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(count);
  uint32_t fullWord0 = 0, stride = 0, previousAddress = 0;
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t entry = ShaderContainerWord(bytes, size_t(table) + i * 4);
    const uint32_t address = entry & 0xFFFu;
    if ((i && address <= previousAddress) || uint64_t(address) * 12 + 12 > codeBytes)
      return nullptr;
    previousAddress = address;
    const uint32_t word0 = ShaderContainerWord(bytes, code + address * 12);
    const uint32_t word1 = ShaderContainerWord(bytes, code + address * 12 + 4);
    const uint32_t word2 = ShaderContainerWord(bytes, code + address * 12 + 8);
    if ((word0 & 31u) != 0 || (word0 & 0x800u))  // not vfetch / relative index
      return nullptr;
    if (!(word1 & 0x40000000u)) {
      fullWord0 = word0;
      stride = (word2 & 0xFFu) * 4;
    }
    const uint32_t fetch = ((fullWord0 >> 20) & 31u) * 3 + ((fullWord0 >> 25) & 3u);
    const uint32_t format = (word1 >> 16) & 63u;
    const uint32_t offset = (word2 >> 8) & 0x7FFFFFu;
    // These are the raw IA formats supported by the baked-fetch translator.
    // Negative/overflowing offsets and unsupported layouts fail closed.
    const uint32_t width = format == 57 ? 12u : (format == 7 || format == 25) ? 4u : 0u;
    if (!width || !stride || fetch < 80 || fetch > 95 ||
        offset > UINT16_MAX / 4u || offset * 4 + width > stride)
      return nullptr;
    const uint32_t stream = 95 - fetch;
    if (decl->bakedStrides[stream] && decl->bakedStrides[stream] != stride)
      return nullptr;
    const uint8_t usage = uint8_t((entry >> 12) & 15u);
    const uint8_t usageIndex = uint8_t((entry >> 16) & 15u);
    // One native semantic cannot represent two distinct shader fetch addresses.
    for (uint32_t j = 0; j < i; ++j) {
      const auto& old = decl->vertexElements[j];
      if (old.usage == usage && old.usageIndex == usageIndex)
        return nullptr;
    }
    decl->vertexElements[i] = {uint16_t(stream), uint16_t(offset * 4), format,
                               0, usage, usageIndex, 0};
    decl->bakedStrides[stream] = stride;
    decl->vertexStreams[stream] = true;
  }
  decl->vertexElementCount = count;
  return decl;
}

}  // namespace fm2::render
