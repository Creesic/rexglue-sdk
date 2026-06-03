#include <rex/graphics/metal/metal4_context.h>
#include <rex/graphics/metal/metal4_ir_runtime.h>

#include <Metal/Metal.hpp>
#define IR_RUNTIME_METALCPP
#define IR_PRIVATE_IMPLEMENTATION
#include <metal_irconverter_runtime.h>

namespace rex {
namespace graphics {
namespace metal {

thread_local Metal4Context* g_mtl4_ir_ctx = nullptr;

void MTL4DrawPrimitives(MTL4::RenderCommandEncoder* enc,
                        MTL::PrimitiveType primitiveType,
                        uint64_t vertexStart, uint64_t vertexCount,
                        uint64_t instanceCount, uint64_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc) return;

  IRRuntimeDrawArgument da = {
      (uint32_t)vertexCount, (uint32_t)instanceCount,
      (uint32_t)vertexStart, (uint32_t)baseInstance};
  IRRuntimeDrawParams dp = {.draw = da};
  const uint16_t kNonIndexed = kIRNonIndexedDraw;

  ctx->SetVertexAddress(ctx->AllocInlineConstant(&dp, sizeof(dp)),
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->SetVertexAddress(
      ctx->AllocInlineConstant(&kNonIndexed, sizeof(kNonIndexed)),
      kIRArgumentBufferUniformsBindPoint);
  ctx->FlushRenderBindings(enc);

  enc->drawPrimitives(primitiveType, NS::UInteger(vertexStart),
                      NS::UInteger(vertexCount), NS::UInteger(instanceCount),
                      NS::UInteger(baseInstance));
}

void MTL4DrawIndexedPrimitives(MTL4::RenderCommandEncoder* enc,
                               MTL::PrimitiveType primitiveType,
                               uint64_t indexCount, MTL::IndexType indexType,
                               MTL::Buffer* indexBuffer,
                               uint64_t indexBufferOffset,
                               uint64_t instanceCount, int64_t baseVertex,
                               uint64_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc || !indexBuffer) return;

  IRRuntimeDrawIndexedArgument da = {
      (uint32_t)indexCount, (uint32_t)instanceCount,
      (uint32_t)indexBufferOffset, (int32_t)baseVertex,
      (uint32_t)baseInstance};
  IRRuntimeDrawParams dp = {.drawIndexed = da};
  const uint16_t IRIndexType = IRMetalIndexToIRIndex(indexType);

  ctx->SetVertexAddress(ctx->AllocInlineConstant(&dp, sizeof(dp)),
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->SetVertexAddress(
      ctx->AllocInlineConstant(&IRIndexType, sizeof(IRIndexType)),
      kIRArgumentBufferUniformsBindPoint);
  ctx->FlushRenderBindings(enc);

  MTL::GPUAddress idx_addr =
      indexBuffer->gpuAddress() + indexBufferOffset;
  NS::UInteger idx_len =
      indexBuffer->length() - NS::UInteger(indexBufferOffset);
  enc->drawIndexedPrimitives(primitiveType, NS::UInteger(indexCount),
                             indexType, idx_addr, idx_len,
                             NS::UInteger(instanceCount),
                             NS::Integer(baseVertex),
                             NS::UInteger(baseInstance));
}

void MTL4DrawIndexedPrimitivesGeometryEmulation(
    MTL4::RenderCommandEncoder* enc,
    MTL4IRPrimitiveType primType, MTL::IndexType indexType,
    MTL::Buffer* indexBuffer,
    const MTL4GeometryPipelineConfig& geometryConfig,
    uint32_t instanceCount, uint32_t indexCountPerInstance,
    uint32_t startIndex, int baseVertex, uint32_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc || !indexBuffer) return;

  IRRuntimePrimitiveType primitiveType = (IRRuntimePrimitiveType)primType;
  IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(
      primitiveType, indexType, geometryConfig.gsVertexSizeInBytes,
      geometryConfig.gsMaxInputPrimitivesPerMeshThreadgroup, instanceCount);
  drawInfo.indexBuffer = indexBuffer->gpuAddress();

  MTL::Size objectThreadgroupCount =
      IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(
          indexCountPerInstance, drawInfo.objectThreadgroupVertexStride,
          (IRRuntimePrimitiveType)primitiveType, instanceCount);

  uint32_t objectThreadgroupSize = 0, meshThreadgroupSize = 0;
  IRRuntimeCalculateThreadgroupSizeForGeometry(
      (IRRuntimePrimitiveType)primitiveType,
      geometryConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      drawInfo.objectThreadgroupVertexStride, &objectThreadgroupSize,
      &meshThreadgroupSize);

  IRRuntimeDrawParams drawParams;
  drawParams.drawIndexed = (IRRuntimeDrawIndexedArgument){
      indexCountPerInstance, instanceCount, startIndex,
      baseVertex, baseInstance};

  MTL::GPUAddress drawInfo_addr =
      ctx->AllocInlineConstant(&drawInfo, sizeof(drawInfo));
  MTL::GPUAddress drawParams_addr =
      ctx->AllocInlineConstant(&drawParams, sizeof(drawParams));

  ctx->SetVertexAddress(drawInfo_addr, kIRArgumentBufferUniformsBindPoint);
  ctx->SetVertexAddress(drawParams_addr,
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->FlushRenderBindings(enc);

  enc->drawMeshThreadgroups(objectThreadgroupCount,
                            MTL::Size::Make(objectThreadgroupSize, 1, 1),
                            MTL::Size::Make(meshThreadgroupSize, 1, 1));
}

void MTL4DrawPrimitivesGeometryEmulation(
    MTL4::RenderCommandEncoder* enc,
    MTL4IRPrimitiveType primType,
    const MTL4GeometryPipelineConfig& geometryConfig,
    uint32_t instanceCount, uint32_t vertexCountPerInstance,
    uint32_t baseVertex, uint32_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc) return;

  IRRuntimePrimitiveType primitiveType = (IRRuntimePrimitiveType)primType;

  IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(
      primitiveType, (MTL::IndexType)-1, geometryConfig.gsVertexSizeInBytes,
      geometryConfig.gsMaxInputPrimitivesPerMeshThreadgroup, instanceCount);
  drawInfo.indexType = kIRNonIndexedDraw;

  MTL::Size objectThreadgroupCount =
      IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(
          vertexCountPerInstance, drawInfo.objectThreadgroupVertexStride,
          primitiveType, instanceCount);

  uint32_t objectThreadgroupSize = 0, meshThreadgroupSize = 0;
  IRRuntimeCalculateThreadgroupSizeForGeometry(
      primitiveType, geometryConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      drawInfo.objectThreadgroupVertexStride, &objectThreadgroupSize,
      &meshThreadgroupSize);

  IRRuntimeDrawParams drawParams;
  drawParams.draw = (IRRuntimeDrawArgument){vertexCountPerInstance,
                                            instanceCount, baseVertex,
                                            baseInstance};

  MTL::GPUAddress drawInfo_addr =
      ctx->AllocInlineConstant(&drawInfo, sizeof(drawInfo));
  MTL::GPUAddress drawParams_addr =
      ctx->AllocInlineConstant(&drawParams, sizeof(drawParams));

  ctx->SetVertexAddress(drawInfo_addr, kIRArgumentBufferUniformsBindPoint);
  ctx->SetVertexAddress(drawParams_addr,
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->FlushRenderBindings(enc);

  enc->drawMeshThreadgroups(objectThreadgroupCount,
                            MTL::Size::Make(objectThreadgroupSize, 1, 1),
                            MTL::Size::Make(meshThreadgroupSize, 1, 1));
}

void MTL4DrawIndexedPatchesTessellationEmulation(
    MTL4::RenderCommandEncoder* enc,
    const MTL4TessellationPipelineConfig& tessConfig,
    MTL::Buffer* indexBuffer, uint32_t instanceCount,
    uint32_t indexCountPerInstance, uint32_t startIndex, int baseVertex,
    uint32_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc || !indexBuffer) return;

  IRRuntimePrimitiveType primType =
      (IRRuntimePrimitiveType)tessConfig.primitiveType;
  IRRuntimeTessellatorOutputPrimitive tessOutPrim =
      (IRRuntimeTessellatorOutputPrimitive)
          tessConfig.tessellatorOutputPrimitive;

  IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSTSEmulation(
      primType, (indextype_t)tessConfig.indexType, tessOutPrim,
      tessConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      tessConfig.hsPatchesPerObjectThreadgroup,
      tessConfig.hsInputControlPointsPerPatch,
      tessConfig.hsObjectThreadsPerPatch, tessConfig.gsInstanceCount);
  drawInfo.indexBuffer = indexBuffer->gpuAddress();

  MTL::Size objectThreadgroupCount =
      IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(
          indexCountPerInstance, drawInfo.objectThreadgroupVertexStride,
          primType, instanceCount);

  uint32_t objectThreadgroupSize = 0, meshThreadgroupSize = 0;
  IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(
      tessConfig.hsPatchesPerObjectThreadgroup,
      tessConfig.hsObjectThreadsPerPatch,
      tessConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      &objectThreadgroupSize, &meshThreadgroupSize);

  IRRuntimeDrawParams drawParams;
  drawParams.drawIndexed = (IRRuntimeDrawIndexedArgument){
      indexCountPerInstance, instanceCount, startIndex,
      baseVertex, baseInstance};

  MTL::GPUAddress drawInfo_addr =
      ctx->AllocInlineConstant(&drawInfo, sizeof(drawInfo));
  MTL::GPUAddress drawParams_addr =
      ctx->AllocInlineConstant(&drawParams, sizeof(drawParams));

  ctx->SetVertexAddress(drawInfo_addr, kIRArgumentBufferUniformsBindPoint);
  ctx->SetVertexAddress(drawParams_addr,
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->FlushRenderBindings(enc);

  enc->setObjectThreadgroupMemoryLength(15360, 0);

  enc->drawMeshThreadgroups(objectThreadgroupCount,
                            MTL::Size::Make(objectThreadgroupSize, 1, 1),
                            MTL::Size::Make(meshThreadgroupSize, 1, 1));
}

void MTL4DrawPatchesTessellationEmulation(
    MTL4::RenderCommandEncoder* enc,
    const MTL4TessellationPipelineConfig& tessConfig,
    uint32_t instanceCount, uint32_t vertexCountPerInstance,
    uint32_t baseVertex, uint32_t baseInstance) {
  Metal4Context* ctx = g_mtl4_ir_ctx;
  if (!ctx || !enc) return;

  IRRuntimePrimitiveType primType =
      (IRRuntimePrimitiveType)tessConfig.primitiveType;
  IRRuntimeTessellatorOutputPrimitive tessOutPrim =
      (IRRuntimeTessellatorOutputPrimitive)
          tessConfig.tessellatorOutputPrimitive;

  IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSTSEmulation(
      primType, (MTL::IndexType)-1, tessOutPrim,
      tessConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      tessConfig.hsPatchesPerObjectThreadgroup,
      tessConfig.hsInputControlPointsPerPatch,
      tessConfig.hsObjectThreadsPerPatch, tessConfig.gsInstanceCount);
  drawInfo.indexType = kIRNonIndexedDraw;

  MTL::Size objectThreadgroupCount =
      IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(
          vertexCountPerInstance, drawInfo.objectThreadgroupVertexStride,
          primType, instanceCount);

  uint32_t objectThreadgroupSize = 0, meshThreadgroupSize = 0;
  IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(
      tessConfig.hsPatchesPerObjectThreadgroup,
      tessConfig.hsObjectThreadsPerPatch,
      tessConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
      &objectThreadgroupSize, &meshThreadgroupSize);

  IRRuntimeDrawParams drawParams;
  drawParams.draw = (IRRuntimeDrawArgument){vertexCountPerInstance,
                                            instanceCount, baseVertex,
                                            baseInstance};

  MTL::GPUAddress drawInfo_addr =
      ctx->AllocInlineConstant(&drawInfo, sizeof(drawInfo));
  MTL::GPUAddress drawParams_addr =
      ctx->AllocInlineConstant(&drawParams, sizeof(drawParams));

  ctx->SetVertexAddress(drawInfo_addr, kIRArgumentBufferUniformsBindPoint);
  ctx->SetVertexAddress(drawParams_addr,
                        kIRArgumentBufferDrawArgumentsBindPoint);
  ctx->FlushRenderBindings(enc);

  enc->setObjectThreadgroupMemoryLength(15360, 0);

  enc->drawMeshThreadgroups(objectThreadgroupCount,
                            MTL::Size::Make(objectThreadgroupSize, 1, 1),
                            MTL::Size::Make(meshThreadgroupSize, 1, 1));
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
