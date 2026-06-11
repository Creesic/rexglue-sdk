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
REXCVAR_DEFINE_STRING(dump_shaders, "", "GPU", "Path to dump shaders to");
REXCVAR_DEFINE_STRING(
    spirv_version_override, "1.0", "GPU",
    "Override the SPIR-V version used in shader translation.\n"
    "Use: [1.0, 1.3, 1.4, 1.5, 1.6, auto]\n"
    " 1.0: SPIR-V 1.0 (Vulkan 1.0) (default)\n"
    " 1.3: SPIR-V 1.3 (Vulkan 1.1)\n"
    " 1.4: SPIR-V 1.4 (Vulkan 1.1 with KHR_spirv_1_4 extension)\n"
    " 1.5: SPIR-V 1.5 (Vulkan 1.2+)\n"
    " 1.6: SPIR-V 1.6 (Vulkan 1.3+)\n"
    " auto: Test for SPIR-V 1.5 support, fall back to 1.0");
REXCVAR_DEFINE_BOOL(
    spirv_disable_rounding_mode_rte, false, "GPU",
    "Disable RoundingModeRTE capability in SPIR-V shaders. Enable this to "
    "allow shader debugging in tools that don't support this capability.");
REXCVAR_DEFINE_BOOL(use_fuzzy_alpha_epsilon, false, "GPU",
                    "Use approximate compare for alpha test values to prevent "
                    "flickering on NVIDIA graphics cards");
REXCVAR_DEFINE_BOOL(gpu_debug_markers, false, "GPU",
                    "Insert debug markers into GPU command streams for tools "
                    "like PIX and RenderDoc. Automatically enabled when "
                    "RenderDoc is detected.");
REXCVAR_DEFINE_BOOL(
    vulkan_precise_interpolation, true, "GPU/Vulkan",
    "Use manual barycentric interpolation in fragment shaders to avoid "
    "precision issues on some Vulkan drivers. Requires fragment shader "
    "barycentric support.");
REXCVAR_DEFINE_BOOL(
    spirv_moltenvk_allow_contraction, true, "GPU/Vulkan",
    "When translating SPIR-V for MoltenVK, omit NoContraction decorations so "
    "SPIRV-Cross doesn't emit MSL helper wrappers with disabled optimization. "
    "Other Vulkan drivers keep NoContraction enabled.");

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
