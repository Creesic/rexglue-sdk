/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/graphics/flags.h>
#include <rex/logging.h>
#include <rex/ui/renderdoc_api.h>

REXCVAR_DEFINE_BOOL(gpu_allow_invalid_fetch_constants, false, "GPU",
                    "Allow invalid fetch constants");
REXCVAR_DEFINE_BOOL(native_2x_msaa, true, "GPU", "Enable native 2x MSAA");
REXCVAR_DEFINE_BOOL(depth_float24_round, false, "GPU", "Round float24 depth values");
REXCVAR_DEFINE_BOOL(depth_float24_convert_in_pixel_shader, false, "GPU",
                    "Convert float24 depth in pixel shader");
REXCVAR_DEFINE_BOOL(depth_transfer_not_equal_test, true, "GPU",
                    "Use not-equal test for depth transfer");
REXCVAR_DEFINE_BOOL(gamma_render_target_as_unorm16, true, "GPU",
                    "Use R16G16B16A16_UNORM for gamma render targets (more accurate than sRGB)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_BOOL(snorm16_render_target_full_range, false, "GPU",
                    "Use full range for snorm16 render targets");
REXCVAR_DEFINE_STRING(dump_shaders, "", "GPU", "Path to dump shaders to");
REXCVAR_DEFINE_BOOL(use_fuzzy_alpha_epsilon, false, "GPU",
                    "Use approximate compare for alpha test values to prevent "
                    "flickering on NVIDIA graphics cards");
REXCVAR_DEFINE_BOOL(gpu_debug_markers, false, "GPU",
                    "Insert debug markers into GPU command streams for tools "
                    "like PIX and RenderDoc. Automatically enabled when "
                    "RenderDoc is detected.");

bool IsGpuDebugMarkersEnabled() {
  static bool cached = false;
  static bool result = false;
  if (!cached) {
    cached = true;
    if (REXCVAR_GET(gpu_debug_markers)) {
      result = true;
      REXLOG_INFO("GPU debug markers enabled via CVar");
    } else {
      auto renderdoc_api = rex::ui::RenderDocAPI::CreateIfConnected();
      if (renderdoc_api) {
        result = true;
        REXLOG_INFO("GPU debug markers auto-enabled (RenderDoc detected)");
      }
    }
  }
  return result;
}

// Metal backend cvars
REXCVAR_DEFINE_BOOL(metal_shader_disk_cache, true, "GPU/Metal",
                    "Cache compiled Metal shader libraries to disk");
REXCVAR_DEFINE_BOOL(metal_pipeline_binary_archive, true, "GPU/Metal",
                    "Use MTLBinaryArchive for Metal pipeline caching");
REXCVAR_DEFINE_BOOL(metal_pipeline_disk_cache, true, "GPU/Metal",
                    "Store Metal pipeline descriptor keys to disk");
REXCVAR_DEFINE_BOOL(metal_use_heaps, true, "GPU/Metal",
                    "Use MTLHeap-backed texture allocations");
REXCVAR_DEFINE_BOOL(metal_shared_memory_zero_copy, true, "GPU/Metal",
                    "Use zero-copy shared memory mapping");
REXCVAR_DEFINE_INT32(metal_heap_min_bytes, 33554432, "GPU/Metal",
                     "Minimum heap size for Metal heap allocations");
REXCVAR_DEFINE_BOOL(metal_texture_cache_use_private, true, "GPU/Metal",
                    "Use MTLStorageModePrivate for texture cache textures");
REXCVAR_DEFINE_BOOL(metal_texture_upload_via_blit, true, "GPU/Metal",
                    "Upload textures via blit command encoder");
REXCVAR_DEFINE_INT32(metal_pipeline_creation_threads, 0, "GPU/Metal",
                     "Number of threads for pipeline creation (0 = auto)");
REXCVAR_DEFINE_BOOL(metal_force_bc_decompress, false, "GPU/Metal",
                    "Force BC texture decompression");
REXCVAR_DEFINE_BOOL(metal_transfer_fast_divmod, true, "GPU/Metal",
                    "Use fast divmod in transfer shaders");
REXCVAR_DEFINE_BOOL(metal_transfer_msaa_sample_id, true, "GPU/Metal",
                    "Use MSAA sample ID in transfer shaders");
REXCVAR_DEFINE_BOOL(metal_transfer_tile_instancing, true, "GPU/Metal",
                    "Use tile instancing in transfer shaders");
REXCVAR_DEFINE_BOOL(metal_memory_log_rate, false, "GPU/Metal",
                    "Log memory usage rate");
REXCVAR_DEFINE_BOOL(submit_on_primary_buffer_end, false, "GPU/Metal",
                    "Submit GPU commands on primary buffer end");
REXCVAR_DEFINE_BOOL(metal_allow_gamma_unorm16, false, "GPU/Metal",
                    "Allow gamma_render_target_as_unorm16 on Metal despite known issues");

