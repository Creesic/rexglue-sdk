#include <rex/graphics/metal/shader_converter.h>

#include <metal_irconverter.h>

#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

constexpr uint32_t kFunctionConstantRegisterSpace = 2147420894u;

MetalShaderConverter::MetalShaderConverter() = default;
MetalShaderConverter::~MetalShaderConverter() = default;

void MetalShaderConverter::SetMinimumTarget(uint32_t gpu_family, uint32_t os,
                                            const std::string& version) {
  has_minimum_target_ = true;
  minimum_gpu_family_ = gpu_family;
  minimum_os_ = os;
  minimum_os_version_ = version;
}

bool MetalShaderConverter::Initialize() {
  fprintf(stderr, "[metal] MetalShaderConverter: calling IRCompilerCreate\n"); fflush(stderr);
  IRCompiler* test_compiler = IRCompilerCreate();
  if (!test_compiler) {
    fprintf(stderr, "[metal] MetalShaderConverter: IRCompilerCreate returned null - MSC not available\n"); fflush(stderr);
    is_available_ = false;
    return false;
  }
  IRCompilerDestroy(test_compiler);
  fprintf(stderr, "[metal] MetalShaderConverter: initialized OK\n"); fflush(stderr);
  is_available_ = true;
  return true;
}

void* MetalShaderConverter::CreateXbox360RootSignature(
    MetalShaderStage stage, bool force_all_visibility) {
  IRShaderVisibility visibility = IRShaderVisibilityAll;
  if (!force_all_visibility) {
    switch (stage) {
      case MetalShaderStage::kVertex:
        visibility = IRShaderVisibilityVertex;
        break;
      case MetalShaderStage::kFragment:
        visibility = IRShaderVisibilityPixel;
        break;
      case MetalShaderStage::kHull:
        visibility = IRShaderVisibilityHull;
        break;
      case MetalShaderStage::kDomain:
        visibility = IRShaderVisibilityDomain;
        break;
      default:
        break;
    }
  }

  IRDescriptorRange1 ranges[20] = {};
  int rangeIdx = 0;

  for (int space = 0; space < 4; space++) {
    ranges[rangeIdx].RangeType = IRDescriptorRangeTypeSRV;
    ranges[rangeIdx].NumDescriptors = 1025;
    ranges[rangeIdx].BaseShaderRegister = 0;
    ranges[rangeIdx].RegisterSpace = space;
    ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
    ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
    rangeIdx++;
  }

  ranges[rangeIdx].RangeType = IRDescriptorRangeTypeSRV;
  ranges[rangeIdx].NumDescriptors = 1025;
  ranges[rangeIdx].BaseShaderRegister = 0;
  ranges[rangeIdx].RegisterSpace = 10;
  ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
  ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
  rangeIdx++;

  for (int space = 0; space < 4; space++) {
    ranges[rangeIdx].RangeType = IRDescriptorRangeTypeUAV;
    ranges[rangeIdx].NumDescriptors = 1025;
    ranges[rangeIdx].BaseShaderRegister = 0;
    ranges[rangeIdx].RegisterSpace = space;
    ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
    ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
    rangeIdx++;
  }

  ranges[rangeIdx].RangeType = IRDescriptorRangeTypeSampler;
  ranges[rangeIdx].NumDescriptors = 257;
  ranges[rangeIdx].BaseShaderRegister = 0;
  ranges[rangeIdx].RegisterSpace = 0;
  ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
  ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
  rangeIdx++;

  for (int space = 0; space < 4; space++) {
    ranges[rangeIdx].RangeType = IRDescriptorRangeTypeCBV;
    ranges[rangeIdx].NumDescriptors = (space == 0) ? 5 : 1;
    ranges[rangeIdx].BaseShaderRegister = 0;
    ranges[rangeIdx].RegisterSpace = space;
    ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
    ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
    rangeIdx++;
  }

  ranges[rangeIdx].RangeType = IRDescriptorRangeTypeCBV;
  ranges[rangeIdx].NumDescriptors = 1;
  ranges[rangeIdx].BaseShaderRegister = 0;
  ranges[rangeIdx].RegisterSpace = kFunctionConstantRegisterSpace;
  ranges[rangeIdx].Flags = IRDescriptorRangeFlagNone;
  ranges[rangeIdx].OffsetInDescriptorsFromTableStart = 0;
  rangeIdx++;

  IRRootDescriptorTable1 tables[20] = {};
  IRRootParameter1 params[20] = {};

  for (int i = 0; i < rangeIdx; i++) {
    tables[i].NumDescriptorRanges = 1;
    tables[i].pDescriptorRanges = &ranges[i];
    params[i].ParameterType = IRRootParameterTypeDescriptorTable;
    params[i].DescriptorTable = tables[i];
    params[i].ShaderVisibility = visibility;
  }

  IRRootSignatureDescriptor1 desc = {};
  desc.NumParameters = rangeIdx;
  desc.pParameters = params;
  desc.NumStaticSamplers = 0;
  desc.pStaticSamplers = nullptr;
  desc.Flags = IRRootSignatureFlagNone;

  IRVersionedRootSignatureDescriptor versionedDesc = {};
  versionedDesc.version = IRRootSignatureVersion_1_1;
  versionedDesc.desc_1_1 = desc;

  IRError* error = nullptr;
  IRRootSignature* rootSig =
      IRRootSignatureCreateFromDescriptor(&versionedDesc, &error);

  if (error) {
    const char* errMsg = (const char*)IRErrorGetPayload(error);
    REXLOG_ERROR("MetalShaderConverter: Failed to create root signature: {}",
                 errMsg ? errMsg : "unknown error");
    IRErrorDestroy(error);
    return nullptr;
  }

  static bool dumped_root_locations = false;
  if (!dumped_root_locations) {
    dumped_root_locations = true;
    size_t resource_count = IRRootSignatureGetResourceCount(rootSig);
    std::vector<IRResourceLocation> locations(resource_count);
    IRRootSignatureGetResourceLocations(rootSig, locations.data());
    fprintf(stderr, "[metal] root signature locations (%zu):\n", resource_count);
    for (const IRResourceLocation& location : locations) {
      fprintf(stderr,
              "[metal]   type=%u space=%u slot=%u top_offset=%u size=%llu name=%s\n",
              uint32_t(location.resourceType), location.space, location.slot,
              location.topLevelOffset,
              static_cast<unsigned long long>(location.sizeBytes),
              location.resourceName ? location.resourceName : "<null>");
    }
    fflush(stderr);
  }

  return rootSig;
}

void MetalShaderConverter::DestroyRootSignature(void* root_sig) {
  if (root_sig) {
    IRRootSignatureDestroy(static_cast<IRRootSignature*>(root_sig));
  }
}

bool MetalShaderConverter::Convert(xenos::ShaderType shader_type,
                                   const std::vector<uint8_t>& dxil_data,
                                   MetalShaderConversionResult& result) {
  MetalShaderStage stage;
  switch (shader_type) {
    case xenos::ShaderType::kVertex:
      stage = MetalShaderStage::kVertex;
      break;
    case xenos::ShaderType::kPixel:
      stage = MetalShaderStage::kFragment;
      break;
    default:
      result.success = false;
      result.error_message = "Unsupported shader type";
      return false;
  }
  return ConvertWithStage(stage, dxil_data, result);
}

bool MetalShaderConverter::ConvertWithStage(
    MetalShaderStage stage, const std::vector<uint8_t>& dxil_data,
    MetalShaderConversionResult& result) {
  return ConvertWithStageEx(stage, dxil_data, result, nullptr, nullptr, nullptr,
                            false, IRInputTopologyUndefined);
}

bool MetalShaderConverter::ConvertWithStageEx(
    MetalShaderStage stage, const std::vector<uint8_t>& dxil_data,
    MetalShaderConversionResult& result, MetalShaderReflectionInfo* reflection,
    const IRVersionedInputLayoutDescriptor* input_layout,
    std::vector<uint8_t>* stage_in_metallib, bool enable_geometry_emulation,
    int input_topology) {
  if (!is_available_) {
    result.success = false;
    result.error_message = "MetalShaderConverter not initialized";
    return false;
  }

  if (dxil_data.empty()) {
    result.success = false;
    result.error_message = "Empty DXIL data";
    return false;
  }

  IRObject* dxilObject = IRObjectCreateFromDXIL(
      dxil_data.data(), dxil_data.size(), IRBytecodeOwnershipNone);
  if (!dxilObject) {
    result.success = false;
    result.error_message = "Failed to create DXIL object";
    return false;
  }

  IRCompiler* compiler = IRCompilerCreate();
  if (!compiler) {
    IRObjectDestroy(dxilObject);
    result.success = false;
    result.error_message = "Failed to create IR compiler";
    return false;
  }

  IRCompilerSetCompatibilityFlags(
      compiler,
      static_cast<IRCompatibilityFlags>(IRCompatibilityFlagForceTextureArray |
                                        IRCompatibilityFlagBoundsCheck));

  if (input_topology != IRInputTopologyUndefined) {
    IRCompilerSetInputTopology(compiler,
                               static_cast<IRInputTopology>(input_topology));
  }
  if (enable_geometry_emulation) {
    IRCompilerEnableGeometryAndTessellationEmulation(compiler, true);
  }
  IRCompilerIgnoreRootSignature(compiler, true);
  IRCompilerSetFunctionConstantResourceSpace(compiler,
                                             kFunctionConstantRegisterSpace);
  if (has_minimum_target_) {
    IRCompilerSetMinimumGPUFamily(
        compiler, static_cast<IRGPUFamily>(minimum_gpu_family_));
    IRCompilerSetMinimumDeploymentTarget(
        compiler, static_cast<IROperatingSystem>(minimum_os_),
        minimum_os_version_.c_str());
  }

  IRRootSignature* rootSig =
      static_cast<IRRootSignature*>(CreateXbox360RootSignature(stage, true));
  if (!rootSig) {
    IRCompilerDestroy(compiler);
    IRObjectDestroy(dxilObject);
    result.success = false;
    result.error_message = "Failed to create root signature";
    return false;
  }
  IRCompilerSetGlobalRootSignature(compiler, rootSig);

  IRError* error = nullptr;
  IRObject* metalObject =
      IRCompilerAllocCompileAndLink(compiler, nullptr, dxilObject, &error);

  if (error) {
    const char* errMsg = (const char*)IRErrorGetPayload(error);
    result.success = false;
    result.error_message = std::string("MSC compilation failed: ") +
                           (errMsg ? errMsg : "unknown error");
    REXLOG_ERROR("MetalShaderConverter: {}", result.error_message);
    IRErrorDestroy(error);
    IRRootSignatureDestroy(rootSig);
    IRCompilerDestroy(compiler);
    IRObjectDestroy(dxilObject);
    return false;
  }

  if (!metalObject) {
    result.success = false;
    result.error_message = "MSC returned null object without error";
    IRRootSignatureDestroy(rootSig);
    IRCompilerDestroy(compiler);
    IRObjectDestroy(dxilObject);
    return false;
  }

  auto extract_metallib = [&](IRShaderStage ir_stage,
                              std::vector<uint8_t>& out_bytes) -> bool {
    IRMetalLibBinary* metallib = IRMetalLibBinaryCreate();
    if (!metallib) return false;
    bool ok = IRObjectGetMetalLibBinary(metalObject, ir_stage, metallib);
    size_t metallib_size = IRMetalLibGetBytecodeSize(metallib);
    if (!ok || metallib_size == 0) {
      IRMetalLibBinaryDestroy(metallib);
      return false;
    }
    out_bytes.resize(metallib_size);
    IRMetalLibGetBytecode(metallib, out_bytes.data());
    IRMetalLibBinaryDestroy(metallib);
    return true;
  };

  IRShaderStage ir_stage = IRShaderStageInvalid;
  switch (stage) {
    case MetalShaderStage::kVertex:
      ir_stage = IRShaderStageVertex;
      break;
    case MetalShaderStage::kFragment:
      ir_stage = IRShaderStageFragment;
      break;
    case MetalShaderStage::kCompute:
      ir_stage = IRShaderStageCompute;
      break;
    case MetalShaderStage::kHull:
      ir_stage = IRShaderStageHull;
      break;
    case MetalShaderStage::kDomain:
      ir_stage = IRShaderStageDomain;
      break;
    case MetalShaderStage::kGeometry:
      break;
    default:
      break;
  }

  result.has_mesh_stage = false;
  result.has_geometry_stage = false;
  if (stage == MetalShaderStage::kGeometry) {
    std::vector<uint8_t> mesh_bytes;
    std::vector<uint8_t> geom_bytes;
    result.has_mesh_stage = extract_metallib(IRShaderStageMesh, mesh_bytes);
    result.has_geometry_stage = extract_metallib(IRShaderStageGeometry, geom_bytes);
    if (result.has_mesh_stage) {
      result.metallib_data = std::move(mesh_bytes);
      ir_stage = IRShaderStageMesh;
    } else if (result.has_geometry_stage) {
      result.metallib_data = std::move(geom_bytes);
      ir_stage = IRShaderStageGeometry;
    }
  } else if (ir_stage != IRShaderStageInvalid) {
    extract_metallib(ir_stage, result.metallib_data);
  }

  if (result.metallib_data.empty()) {
    result.success = false;
    result.error_message = "Generated MetalLib has zero size";
    REXLOG_ERROR("MetalShaderConverter: empty metallib");
    IRObjectDestroy(metalObject);
    IRRootSignatureDestroy(rootSig);
    IRCompilerDestroy(compiler);
    IRObjectDestroy(dxilObject);
    return false;
  }

  IRShaderReflection* shader_reflection = IRShaderReflectionCreate();
  if (shader_reflection && ir_stage != IRShaderStageInvalid) {
    if (IRObjectGetReflection(metalObject, ir_stage, shader_reflection)) {
      const char* entry_name =
          IRShaderReflectionGetEntryPointFunctionName(shader_reflection);
      if (entry_name) {
        result.function_name = entry_name;
      }

      if (stage == MetalShaderStage::kVertex && stage_in_metallib &&
          input_layout && shader_reflection) {
        IRMetalLibBinary* stage_in_lib = IRMetalLibBinaryCreate();
        if (stage_in_lib) {
          if (IRMetalLibSynthesizeStageInFunction(compiler, shader_reflection,
                                                  input_layout, stage_in_lib)) {
            size_t stage_in_size = IRMetalLibGetBytecodeSize(stage_in_lib);
            if (stage_in_size) {
              stage_in_metallib->resize(stage_in_size);
              IRMetalLibGetBytecode(stage_in_lib, stage_in_metallib->data());
            }
          }
          IRMetalLibBinaryDestroy(stage_in_lib);
        }
      }
    }
    IRShaderReflectionDestroy(shader_reflection);
  }

  if (result.function_name.empty()) {
    switch (stage) {
      case MetalShaderStage::kVertex:
        result.function_name = "vertexMain";
        break;
      case MetalShaderStage::kFragment:
        result.function_name = "fragmentMain";
        break;
      case MetalShaderStage::kCompute:
        result.function_name = "computeMain";
        break;
      default:
        result.function_name = "main";
        break;
    }
  }

  REXLOG_DEBUG(
      "MetalShaderConverter: Converted {} bytes DXIL to {} bytes MetalLib",
      dxil_data.size(), result.metallib_data.size());

  IRObjectDestroy(metalObject);
  IRRootSignatureDestroy(rootSig);
  IRCompilerDestroy(compiler);
  IRObjectDestroy(dxilObject);

  result.success = true;
  return true;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
