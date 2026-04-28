#include <rex/graphics/metal/pipeline_cache.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

MetalPipelineCache::MetalPipelineCache(
    MetalCommandProcessor& command_processor,
    const RegisterFile& register_file,
    MetalRenderTargetCache& render_target_cache)
    : command_processor_(command_processor) {
  REXLOG_INFO("MetalPipelineCache stub");
}

MetalPipelineCache::~MetalPipelineCache() = default;

}  // namespace metal
}  // namespace graphics
}  // namespace rex
