// render/render_queue.h
//
// Dedicated render thread (Unleashed pattern, slim): guest hooks enqueue work;
// only the render thread records/submits Plume command lists.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "render/render_commands.h"

namespace fm2::render {

// BeginVertices returns writable guest memory. Do not copy or submit it until
// EndVertices; the normal UP upload/recording path owns the copy from then on.
struct PendingVertexWrite {
  const void* data;
  uint32_t primitiveType;
  uint32_t vertexCount;
  uint32_t stride;
  uint64_t vsDirtyFlags;
  uint64_t psDirtyFlags;
};

class PendingVertexWrites {
 public:
  bool Begin(uint32_t device, const PendingVertexWrite& write) {
    if (device == 0 || write.data == nullptr || write.vertexCount == 0 ||
        write.stride == 0 || write.vertexCount > UINT32_MAX / write.stride) {
      return false;
    }
    return writes_.emplace(device, write).second;
  }

  std::optional<PendingVertexWrite> End(uint32_t device) {
    auto entry = writes_.extract(device);
    if (entry.empty())
      return std::nullopt;
    return entry.mapped();
  }

 private:
  std::unordered_map<uint32_t, PendingVertexWrite> writes_;
};

// Persistent copy of the ordinary D3D commands emitted while FM2 compiles a
// reusable guest command buffer. Pointer-backed draw payloads are owned here;
// resource pointers keep their normal guest-resource lifetime.
class RecordedRenderBatch {
 public:
  void Append(const RenderCommand& source) {
    commands_.push_back(source);
    RenderCommand& command = commands_.back();
    switch (command.type) {
      case RenderCommandType::SetVertexShaderConstants:
      case RenderCommandType::SetPixelShaderConstants:
        command.setShaderConstants.memory =
            Copy(source.setShaderConstants.memory, source.setShaderConstants.size);
        break;
      case RenderCommandType::SetDrawGeometrySnapshot:
        for (size_t i = 0; i < std::size(command.setDrawGeometrySnapshot.streams); ++i) {
          command.setDrawGeometrySnapshot.streams[i].rawData =
              Copy(source.setDrawGeometrySnapshot.streams[i].rawData,
                   source.setDrawGeometrySnapshot.streams[i].rawSize);
        }
        command.setDrawGeometrySnapshot.rawIndexData =
            Copy(source.setDrawGeometrySnapshot.rawIndexData,
                 source.setDrawGeometrySnapshot.rawIndexSize);
        break;
      case RenderCommandType::DrawPrimitiveUP:
        command.drawPrimitiveUP.vertexData =
            Copy(source.drawPrimitiveUP.vertexData, source.drawPrimitiveUP.bytes);
        break;
      default:
        break;
    }
  }

  bool empty() const { return commands_.empty(); }
  size_t size() const { return commands_.size(); }
  const std::vector<RenderCommand>& commands() const { return commands_; }

  void RecordMarker(uint32_t marker) {
    for (CommandMarker& existing : markers_) {
      if (existing.marker == marker) {
        existing.commandIndex = commands_.size();
        return;
      }
    }
    markers_.push_back({marker, commands_.size()});
  }

  size_t AssociateTextureFixup(uint32_t handle, uint32_t guestAddress) {
    TextureFixup* fixup = nullptr;
    for (TextureFixup& candidate : textureFixups_) {
      if (candidate.handle == handle) {
        fixup = &candidate;
        break;
      }
    }
    if (fixup == nullptr) {
      textureFixups_.push_back({handle});
      fixup = &textureFixups_.back();
    }
    fixup->commandIndices.clear();
    for (size_t i = 0; i < commands_.size(); ++i) {
      const RenderCommand& command = commands_[i];
      const uint32_t sourceAddress =
          command.type == RenderCommandType::SetTexture       ? command.setTexture.guestAddress
          : command.type == RenderCommandType::SetTextureBase ? command.setTextureBase.guestAddress
                                                              : 0;
      if (sourceAddress == guestAddress && sourceAddress != 0)
        fixup->commandIndices.push_back(i);
    }
    return fixup->commandIndices.size();
  }

  bool SetTextureFixup(uint32_t handle, const RenderCommand& replacement) {
    if (replacement.type != RenderCommandType::SetTexture &&
        replacement.type != RenderCommandType::SetTextureBase) {
      return false;
    }
    for (TextureFixup& fixup : textureFixups_) {
      if (fixup.handle != handle)
        continue;
      fixup.replacement = replacement;
      fixup.hasReplacement = true;
      return true;
    }
    return false;
  }

  size_t AssociateShaderConstantFixup(uint32_t handle, bool pixelShader, uint32_t startRegister,
                                      uint32_t registerCount, uint32_t startMarker,
                                      uint32_t stopMarker) {
    const uint32_t registerCapacity = pixelShader ? 224u : 256u;
    size_t startIndex;
    size_t stopIndex;
    if (handle == UINT32_MAX || registerCount == 0 || registerCount > 64 ||
        startRegister >= registerCapacity || registerCount > registerCapacity - startRegister ||
        !FindMarker(startMarker, startIndex) || !FindMarker(stopMarker, stopIndex) ||
        startIndex > stopIndex) {
      return 0;
    }

    ShaderConstantFixup* fixup = nullptr;
    for (ShaderConstantFixup& candidate : shaderConstantFixups_) {
      if (candidate.handle == handle) {
        fixup = &candidate;
        break;
      }
    }
    if (fixup == nullptr) {
      shaderConstantFixups_.push_back({handle});
      fixup = &shaderConstantFixups_.back();
    }
    fixup->pixelShader = pixelShader;
    fixup->startRegister = startRegister;
    fixup->registerCount = registerCount;
    fixup->startCommandIndex = startIndex;
    fixup->stopCommandIndex = stopIndex;
    fixup->hasReplacement = false;

    return std::count_if(commands_.begin() + startIndex, commands_.begin() + stopIndex,
                         [](const RenderCommand& command) { return IsDraw(command.type); });
  }

  bool SetShaderConstantFixup(uint32_t handle, const uint32_t* source) {
    if (source == nullptr)
      return false;
    for (ShaderConstantFixup& fixup : shaderConstantFixups_) {
      if (fixup.handle != handle)
        continue;
      std::memcpy(fixup.values.data(), source, size_t(fixup.registerCount) * 16u);
      fixup.hasReplacement = true;
      return true;
    }
    return false;
  }

  std::vector<RenderCommand> BuildReplayCommands(
      std::vector<uint8_t>& shaderConstantPayload) const {
    std::vector<RenderCommand> patched = commands_;
    for (const TextureFixup& fixup : textureFixups_) {
      if (!fixup.hasReplacement)
        continue;
      for (size_t commandIndex : fixup.commandIndices) {
        const RenderCommand& original = patched[commandIndex];
        const uint32_t sampler = original.type == RenderCommandType::SetTexture
                                     ? original.setTexture.index
                                     : original.setTextureBase.index;
        patched[commandIndex] = fixup.replacement;
        if (patched[commandIndex].type == RenderCommandType::SetTexture)
          patched[commandIndex].setTexture.index = sampler;
        else
          patched[commandIndex].setTextureBase.index = sampler;
      }
    }

    size_t payloadSize = 0;
    for (const ShaderConstantFixup& fixup : shaderConstantFixups_) {
      if (fixup.hasReplacement)
        payloadSize += size_t(fixup.registerCount) * 16u;
    }
    shaderConstantPayload.resize(payloadSize);
    std::vector<size_t> payloadOffsets(shaderConstantFixups_.size(), SIZE_MAX);
    size_t payloadOffset = 0;
    for (size_t i = 0; i < shaderConstantFixups_.size(); ++i) {
      const ShaderConstantFixup& fixup = shaderConstantFixups_[i];
      if (!fixup.hasReplacement)
        continue;
      const size_t byteCount = size_t(fixup.registerCount) * 16u;
      payloadOffsets[i] = payloadOffset;
      std::memcpy(shaderConstantPayload.data() + payloadOffset, fixup.values.data(), byteCount);
      payloadOffset += byteCount;
    }

    std::vector<RenderCommand> result;
    result.reserve(patched.size() + shaderConstantFixups_.size());
    for (size_t commandIndex = 0; commandIndex < patched.size(); ++commandIndex) {
      if (IsDraw(patched[commandIndex].type)) {
        for (size_t i = 0; i < shaderConstantFixups_.size(); ++i) {
          const ShaderConstantFixup& fixup = shaderConstantFixups_[i];
          if (!fixup.hasReplacement || commandIndex < fixup.startCommandIndex ||
              commandIndex >= fixup.stopCommandIndex) {
            continue;
          }
          RenderCommand constants{};
          constants.type = fixup.pixelShader ? RenderCommandType::ApplyPixelShaderConstantFixup
                                             : RenderCommandType::ApplyVertexShaderConstantFixup;
          constants.setShaderConstants.memory = shaderConstantPayload.data() + payloadOffsets[i];
          constants.setShaderConstants.index = fixup.startRegister * 4u;
          constants.setShaderConstants.size = fixup.registerCount * 16u;
          result.push_back(constants);
        }
      }
      result.push_back(patched[commandIndex]);
    }
    return result;
  }

 private:
  struct CommandMarker {
    uint32_t marker;
    size_t commandIndex;
  };

  struct TextureFixup {
    uint32_t handle = 0;
    std::vector<size_t> commandIndices;
    RenderCommand replacement{};
    bool hasReplacement = false;
  };

  struct ShaderConstantFixup {
    uint32_t handle = 0;
    bool pixelShader = false;
    uint32_t startRegister = 0;
    uint32_t registerCount = 0;
    size_t startCommandIndex = 0;
    size_t stopCommandIndex = 0;
    std::array<uint32_t, 64 * 4> values{};
    bool hasReplacement = false;
  };

  static bool IsDraw(RenderCommandType type) {
    return type == RenderCommandType::DrawPrimitive ||
           type == RenderCommandType::DrawIndexedPrimitive ||
           type == RenderCommandType::DrawPrimitiveUP;
  }

  bool FindMarker(uint32_t marker, size_t& commandIndex) const {
    for (const CommandMarker& candidate : markers_) {
      if (candidate.marker == marker) {
        commandIndex = candidate.commandIndex;
        return true;
      }
    }
    if (marker == 0) {
      commandIndex = 0;
      return true;
    }
    return false;
  }

  uint8_t* Copy(const uint8_t* source, uint32_t size) {
    if (source == nullptr || size == 0)
      return nullptr;
    auto copy = std::make_unique<uint8_t[]>(size);
    std::memcpy(copy.get(), source, size);
    uint8_t* result = copy.get();
    payloads_.push_back(std::move(copy));
    return result;
  }

  std::vector<RenderCommand> commands_;
  std::vector<std::unique_ptr<uint8_t[]>> payloads_;
  std::vector<CommandMarker> markers_;
  std::vector<TextureFixup> textureFixups_;
  std::vector<ShaderConstantFixup> shaderConstantFixups_;
};

struct RenderQueue {
  // Start/stop the render thread. Safe to call once around Video::Init/Shutdown.
  static void Start();
  static void Stop();

  // Sync POD: enqueue `cmd`, block until Dispatch finishes (Unleashed
  // ExecuteCommandList wait pattern for Present/creates/WaitForGPU).
  static void Run(const RenderCommand& cmd);

  // Fire-and-forget POD command (Unleashed RenderCommand path).
  static void Enqueue(const RenderCommand& cmd);

  // Atomically append an ordered command batch. Draw-state snapshots and the
  // draw that consumes them must not be interleaved with another guest
  // thread's batch.
  static void EnqueueBulk(const RenderCommand* commands, size_t count);

  // True when called on the dedicated render thread.
  static bool IsOnRenderThread();

  // Record the native D3D command stream produced while FM2 builds a reusable
  // guest command buffer, bind it to the returned clone, then replay it at the
  // guest's later EmitDirtyStateAndDrawList call.
  static void BeginRecording();
  static void EndRecording();
  static bool IsRecording();
  static void RecordPendingCommandBufferMarker(uint32_t marker);
  static size_t AssociatePendingTextureFixup(uint32_t handle, uint32_t guestAddress);
  static size_t AssociatePendingShaderConstantFixup(uint32_t handle, bool pixelShader,
                                                    uint32_t startRegister, uint32_t registerCount,
                                                    uint32_t startMarker, uint32_t stopMarker);
  static void BindPendingRecording(uint32_t cloneAddress);
  static bool SetRecordingTextureFixup(uint32_t cloneAddress, uint32_t handle,
                                       const RenderCommand& replacement);
  static bool SetRecordingShaderConstantFixup(uint32_t cloneAddress, uint32_t handle,
                                              const uint32_t* source);
  static bool ReplayRecording(uint32_t cloneAddress,
                              const DeferredExecutionSnapshot* executionSnapshot);
};

}  // namespace fm2::render
