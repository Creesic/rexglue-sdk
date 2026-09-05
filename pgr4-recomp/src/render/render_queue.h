// render/render_queue.h
//
// Dedicated render thread (Unleashed pattern, slim): guest hooks enqueue work;
// only the render thread records/submits Plume command lists.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "render/render_commands.h"
#include "render/byte_snapshot_cache.h"

namespace pgr4::render {

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
                   source.setDrawGeometrySnapshot.streams[i].rawSize,
                   &command.setDrawGeometrySnapshot.streams[i].rawIdentity);
        }
        command.setDrawGeometrySnapshot.rawIndexData =
            Copy(source.setDrawGeometrySnapshot.rawIndexData,
                 source.setDrawGeometrySnapshot.rawIndexSize,
                 &command.setDrawGeometrySnapshot.rawIndexIdentity);
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

  std::vector<RenderCommand> BuildReplayCommands() const {
    std::vector<RenderCommand> result = commands_;
    for (const TextureFixup& fixup : textureFixups_) {
      if (!fixup.hasReplacement)
        continue;
      for (size_t commandIndex : fixup.commandIndices) {
        const RenderCommand& original = result[commandIndex];
        const uint32_t sampler = original.type == RenderCommandType::SetTexture
                                     ? original.setTexture.index
                                     : original.setTextureBase.index;
        result[commandIndex] = fixup.replacement;
        if (result[commandIndex].type == RenderCommandType::SetTexture)
          result[commandIndex].setTexture.index = sampler;
        else
          result[commandIndex].setTextureBase.index = sampler;
      }
    }
    return result;
  }

 private:
  struct TextureFixup {
    uint32_t handle = 0;
    std::vector<size_t> commandIndices;
    RenderCommand replacement{};
    bool hasReplacement = false;
  };

  uint8_t* Copy(const uint8_t* source, uint32_t size, uint64_t* identity = nullptr) {
    return payloads_.Copy(source, size, 0, identity);
  }

  std::vector<RenderCommand> commands_;
  ByteSnapshotCache payloads_;
  std::vector<TextureFixup> textureFixups_;
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
  static size_t AssociatePendingTextureFixup(uint32_t handle, uint32_t guestAddress);
  static void BindPendingRecording(uint32_t cloneAddress);
  static bool SetRecordingTextureFixup(uint32_t cloneAddress, uint32_t handle,
                                       const RenderCommand& replacement);
  static bool ReplayRecording(uint32_t cloneAddress,
                              const DeferredExecutionSnapshot* executionSnapshot);
};

}  // namespace pgr4::render
