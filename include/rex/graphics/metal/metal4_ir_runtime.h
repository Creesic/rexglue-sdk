#pragma once

#include <cstdint>

#include <Metal/Metal.hpp>

namespace rex::graphics::metal {

class Metal4Context;

extern thread_local Metal4Context* g_mtl4_ir_ctx;

enum MTL4IRPrimitiveType : uint32_t {
  kMTL4IRPrimitiveTypePoint = 0,
  kMTL4IRPrimitiveTypeLine = 1,
  kMTL4IRPrimitiveTypeTriangle = 2,
};

struct MTL4GeometryPipelineConfig {
  uint32_t gsVertexSizeInBytes = 0;
  uint32_t gsMaxInputPrimitivesPerMeshThreadgroup = 0;
};

struct MTL4TessellationPipelineConfig {
  uint32_t primitiveType = 0;
  uint32_t tessellatorOutputPrimitive = 0;
  uint32_t indexType = 0;
  uint32_t gsMaxInputPrimitivesPerMeshThreadgroup = 0;
  uint32_t hsPatchesPerObjectThreadgroup = 0;
  uint32_t hsInputControlPointsPerPatch = 0;
  uint32_t hsObjectThreadsPerPatch = 0;
  uint32_t gsInstanceCount = 0;
};

void MTL4SetIRContext(Metal4Context* ctx);

void MTL4DrawPrimitives(MTL4::RenderCommandEncoder* enc,
                        MTL::PrimitiveType primitiveType,
                        uint64_t vertexStart, uint64_t vertexCount,
                        uint64_t instanceCount, uint64_t baseInstance);

void MTL4DrawIndexedPrimitives(MTL4::RenderCommandEncoder* enc,
                               MTL::PrimitiveType primitiveType,
                               uint64_t indexCount, MTL::IndexType indexType,
                               MTL::Buffer* indexBuffer,
                               uint64_t indexBufferOffset,
                               uint64_t instanceCount, int64_t baseVertex,
                               uint64_t baseInstance);

void MTL4DrawIndexedPrimitivesGeometryEmulation(
    MTL4::RenderCommandEncoder* enc,
    MTL4IRPrimitiveType primType, MTL::IndexType indexType,
    MTL::Buffer* indexBuffer,
    const MTL4GeometryPipelineConfig& geometryConfig,
    uint32_t instanceCount, uint32_t indexCountPerInstance,
    uint32_t startIndex, int baseVertex, uint32_t baseInstance);

void MTL4DrawPrimitivesGeometryEmulation(
    MTL4::RenderCommandEncoder* enc,
    MTL4IRPrimitiveType primType,
    const MTL4GeometryPipelineConfig& geometryConfig,
    uint32_t instanceCount, uint32_t vertexCountPerInstance,
    uint32_t baseVertex, uint32_t baseInstance);

void MTL4DrawIndexedPatchesTessellationEmulation(
    MTL4::RenderCommandEncoder* enc,
    const MTL4TessellationPipelineConfig& tessConfig,
    MTL::Buffer* indexBuffer, uint32_t instanceCount,
    uint32_t indexCountPerInstance, uint32_t startIndex, int baseVertex,
    uint32_t baseInstance);

void MTL4DrawPatchesTessellationEmulation(
    MTL4::RenderCommandEncoder* enc,
    const MTL4TessellationPipelineConfig& tessConfig,
    uint32_t instanceCount, uint32_t vertexCountPerInstance,
    uint32_t baseVertex, uint32_t baseInstance);

}  // namespace rex::graphics::metal
