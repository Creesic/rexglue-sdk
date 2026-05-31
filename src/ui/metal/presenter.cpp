#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/surface_mac.h>
#include <rex/logging/macros.h>
#include <rex/cvar.h>
#include <rex/graphics/metal/metal4_context.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <array>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include <cstring>
#include <utility>

#if defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <unistd.h>

#include <rex/ui/metal/capture_diag.h>
#endif

#include "shaders/blit_metallib.h"

REXCVAR_DEFINE_BOOL(metal_hud, true, "GPU", "Enable Metal performance HUD overlay");
REXCVAR_DEFINE_BOOL(metal_frame_timing, true, "GPU", "Log GPU frame timing info");
REXCVAR_DEFINE_BOOL(metal_present_debug_probes, false, "GPU",
                    "Enable extra Metal present-path debug probes/logs");
REXCVAR_DEFINE_BOOL(metal_present_capture_scope, true, "GPU",
                    "Emit per-present Metal capture scope boundaries for Xcode GPU capture");
REXCVAR_DEFINE_BOOL(metal_capture_to_file, false, "GPU",
                    "Capture one short Metal trace to a .gputrace file and stop");
REXCVAR_DEFINE_BOOL(
    metal_capture_diag_shim, true, "GPU",
    "Use Objective-C++ diagnostic shim for Metal capture start/stop logging");
REXCVAR_DEFINE_BOOL(
    metal_capture_to_developer_tools, true, "GPU",
    "Route programmatic capture directly to Xcode developer tools instead of .gputrace file");
REXCVAR_DEFINE_INT32(metal_capture_to_file_frames, 1, "GPU",
                     "How many present frames to record when metal_capture_to_file is enabled");
REXCVAR_DEFINE_INT32(
    metal_capture_to_file_min_frames, 1, "GPU",
    "Minimum present frames for programmatic capture (helps avoid fragile one-frame traces)");
REXCVAR_DEFINE_BOOL(
    metal_capture_to_file_use_queue, true, "GPU",
    "Use command-queue capture object for programmatic .gputrace capture");
REXCVAR_DEFINE_BOOL(
    metal_capture_to_file_use_scope, false, "GPU",
    "Use presenter capture scope object first for programmatic .gputrace capture");
REXCVAR_DEFINE_INT32(
    metal_capture_to_file_max_mb, 1024, "GPU",
    "Hard cap for programmatic .gputrace bundle size in MB (force-stop when exceeded)");
REXCVAR_DEFINE_INT32(
    metal_capture_to_file_max_ms, 2500, "GPU",
    "Hard cap for programmatic capture duration in milliseconds (force-stop)");
REXCVAR_DEFINE_STRING(metal_capture_to_file_path,
                      "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-present-capture.gputrace",
                      "GPU", "Output path for programmatic Metal .gputrace capture");
REXCVAR_DEFINE_BOOL(
    metal_capture_tiny_sanity, false, "GPU",
    "When capture is triggered, record a tiny standalone command-buffer trace and stop immediately");
REXCVAR_DEFINE_STRING(
    metal_capture_tiny_path,
    "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-tiny-sanity.gputrace", "GPU",
    "Base output path for tiny sanity .gputrace capture");
// Legacy aliases kept so FM2 profiles that still set copy-scope capture toggles
// can drive the newer presenter file-capture path without extra scheme edits.
REXCVAR_DEFINE_BOOL(metal_capture_copy_scope_once, false, "GPU",
                    "Legacy alias: trigger one-shot presenter .gputrace capture");
REXCVAR_DEFINE_BOOL(metal_capture_copy_scope_to_file, false, "GPU",
                    "Legacy alias: enable presenter .gputrace file output");
REXCVAR_DEFINE_BOOL(metal_capture_trigger_file_enabled, true, "GPU",
                    "Enable file-based runtime trigger for Metal capture");
REXCVAR_DEFINE_STRING(metal_capture_trigger_file_path,
                      "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-capture-now",
                      "GPU", "Touch this file to trigger one-shot Metal capture");

#if defined(__APPLE__)
namespace {
__attribute__((used)) static const char kRexMetalCapturePatchId[] =
    "rex-metal-capture-patch-2026-05-31-a";

__attribute__((constructor)) static void RexMetalCapturePatchLoaded() {
  char exe_path[4096] = {};
  uint32_t exe_path_size = sizeof(exe_path);
  _NSGetExecutablePath(exe_path, &exe_path_size);

  Dl_info image_info = {};
  dladdr(reinterpret_cast<const void*>(&RexMetalCapturePatchLoaded), &image_info);

  std::fprintf(stderr, "[rex-build] %s pid=%d exe=%s image=%s source=%s\n",
               kRexMetalCapturePatchId, getpid(), exe_path,
               image_info.dli_fname ? image_info.dli_fname : "?", __FILE__);
  std::fflush(stderr);

  // Fallback proof when Xcode swallows stderr: append a marker on disk.
  if (FILE* marker = std::fopen(
          "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-build-marker.txt",
          "a")) {
    std::fprintf(marker, "[rex-build] %s pid=%d exe=%s image=%s source=%s\n",
                 kRexMetalCapturePatchId, getpid(), exe_path,
                 image_info.dli_fname ? image_info.dli_fname : "?", __FILE__);
    std::fclose(marker);
  }
}

void AppendCaptureEvent(const char* fmt, ...) {
  if (!fmt) {
    return;
  }
  if (FILE* marker = std::fopen(
          "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-capture-events.txt",
          "a")) {
    va_list args;
    va_start(args, fmt);
    std::vfprintf(marker, fmt, args);
    va_end(args);
    std::fputc('\n', marker);
    std::fclose(marker);
  }
}

bool EnvTruthy(const char* value) {
  if (!value || !value[0]) {
    return false;
  }
  return std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0 ||
         std::strcmp(value, "TRUE") == 0 || std::strcmp(value, "yes") == 0 ||
         std::strcmp(value, "YES") == 0 || std::strcmp(value, "on") == 0 ||
         std::strcmp(value, "ON") == 0;
}

void ApplyCaptureEnvOverridesOnce() {
  static bool applied = false;
  if (applied) {
    return;
  }
  applied = true;

  const char* mtl_capture_enabled = std::getenv("MTL_CAPTURE_ENABLED");
  if (EnvTruthy(mtl_capture_enabled)) {
    rex::cvar::SetFlagByName("metal_capture_to_file", "true");
    AppendCaptureEvent(
        "[metal-capture] env override MTL_CAPTURE_ENABLED=%s -> metal_capture_to_file=true",
        mtl_capture_enabled);
  }

  struct EnvMap {
    const char* env_name;
    const char* cvar_name;
  };
  static const EnvMap kEnvMaps[] = {
      {"REX_METAL_CAPTURE_TO_FILE", "metal_capture_to_file"},
      {"REX_METAL_CAPTURE_TO_FILE_PATH", "metal_capture_to_file_path"},
      {"REX_METAL_CAPTURE_TO_FILE_FRAMES", "metal_capture_to_file_frames"},
      {"REX_METAL_CAPTURE_TO_FILE_MIN_FRAMES",
       "metal_capture_to_file_min_frames"},
      {"REX_METAL_CAPTURE_TO_FILE_MAX_MB", "metal_capture_to_file_max_mb"},
      {"REX_METAL_CAPTURE_TO_FILE_MAX_MS", "metal_capture_to_file_max_ms"},
      {"REX_METAL_CAPTURE_TO_DEVELOPER_TOOLS",
       "metal_capture_to_developer_tools"},
      {"REX_METAL_CAPTURE_TO_FILE_USE_SCOPE",
       "metal_capture_to_file_use_scope"},
      {"REX_METAL_CAPTURE_TO_FILE_USE_QUEUE",
       "metal_capture_to_file_use_queue"},
      {"REX_METAL_CAPTURE_TINY_SANITY", "metal_capture_tiny_sanity"},
      {"REX_METAL_CAPTURE_TINY_PATH", "metal_capture_tiny_path"},
  };
  for (const EnvMap& map : kEnvMaps) {
    const char* value = std::getenv(map.env_name);
    if (!value || !value[0]) {
      continue;
    }
    rex::cvar::SetFlagByName(map.cvar_name, value);
    AppendCaptureEvent(
        "[metal-capture] env override %s=%s -> %s", map.env_name, value,
        map.cvar_name);
  }
}

uint64_t ComputePathBytes(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec) || ec) {
    return 0;
  }
  if (std::filesystem::is_regular_file(path, ec) && !ec) {
    return static_cast<uint64_t>(std::filesystem::file_size(path, ec));
  }
  uint64_t total = 0;
  for (std::filesystem::recursive_directory_iterator it(path, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (it->is_regular_file(ec) && !ec) {
      total += static_cast<uint64_t>(it->file_size(ec));
    }
  }
  return total;
}

std::string MakeUniqueCapturePath(const std::string& configured_path,
                                  uint64_t sequence) {
  std::filesystem::path base_path(configured_path);
  std::filesystem::path parent = base_path.parent_path();
  std::string stem = base_path.stem().string();
  std::string ext = base_path.extension().string();
  if (stem.empty()) {
    stem = "rex-present-capture";
  }
  if (ext.empty()) {
    ext = ".gputrace";
  }
  char suffix[96] = {};
  std::snprintf(suffix, sizeof(suffix), "-pid%d-seq%llu", getpid(),
                static_cast<unsigned long long>(sequence));
  std::string filename = stem + suffix + ext;
  if (parent.empty()) {
    return filename;
  }
  return (parent / filename).string();
}
}  // namespace
#endif

namespace rex {
namespace ui {
namespace metal {

MetalPresenter::MetalPresenter(MetalProvider* provider,
                                HostGpuLossCallback host_gpu_loss_callback)
    : Presenter(host_gpu_loss_callback), provider_(provider) {
  guest_output_textures_.fill(nullptr);
  guest_output_submissions_.fill(0);
  if (provider_) {
    device_ = provider_->GetDevice();
    mtl4_ = provider_->GetMetal4Context();
  }
}

MetalPresenter::~MetalPresenter() { Shutdown(); }

bool MetalPresenter::Initialize() {
  if (!device_) {
    if (provider_) {
      device_ = provider_->GetDevice();
      mtl4_ = provider_->GetMetal4Context();
    }
    if (!device_) {
      REXLOG_ERROR("MetalPresenter: No Metal device");
      return false;
    }
  }
  if (!mtl4_) {
    REXLOG_ERROR("MetalPresenter: No Metal4Context");
    return false;
  }

  ApplyCaptureEnvOverridesOnce();

  static bool logged_capture_config_once = false;
  if (!logged_capture_config_once) {
    const int capture_to_file = REXCVAR_GET(metal_capture_to_file) ? 1 : 0;
    const int capture_frames = std::max(REXCVAR_GET(metal_capture_to_file_frames), 0);
    const std::string capture_path = REXCVAR_GET(metal_capture_to_file_path);
    const int copy_scope_once = REXCVAR_GET(metal_capture_copy_scope_once) ? 1 : 0;
    const int copy_scope_to_file =
        REXCVAR_GET(metal_capture_copy_scope_to_file) ? 1 : 0;
    std::fprintf(
        stderr,
        "[metal-capture] config to_file=%d frames=%d path=%s copy_scope_once=%d "
        "copy_scope_to_file=%d\n",
        capture_to_file, capture_frames, capture_path.c_str(),
        copy_scope_once, copy_scope_to_file);
    std::fflush(stderr);
    if (FILE* marker = std::fopen(
            "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-capture-config-marker.txt",
            "a")) {
      std::fprintf(
          marker,
          "[metal-capture] config to_file=%d frames=%d path=%s copy_scope_once=%d "
          "copy_scope_to_file=%d\n",
          capture_to_file, capture_frames, capture_path.c_str(),
          copy_scope_once, copy_scope_to_file);
      std::fclose(marker);
    }
    logged_capture_config_once = true;
  }

  if (REXCVAR_GET(metal_present_capture_scope) && mtl4_->queue()) {
    MTL::CaptureManager* capture_manager =
        MTL::CaptureManager::sharedCaptureManager();
    if (capture_manager) {
      presenter_capture_scope_ = capture_manager->newCaptureScope(mtl4_->queue());
      if (presenter_capture_scope_) {
        presenter_capture_scope_->setLabel(
            NS::String::string("rex.present.scope", NS::UTF8StringEncoding));
        capture_manager->setDefaultCaptureScope(presenter_capture_scope_);
      } else {
        REXLOG_WARN("MetalPresenter: Failed to create capture scope for MTL4 queue");
      }
    }
  }

  guest_output_shared_event_ = device_->newSharedEvent();
  if (!guest_output_shared_event_) {
    REXLOG_WARN("MetalPresenter: SharedEvent unavailable; no submission tracking");
  }

  dispatch_data_t libData = dispatch_data_create(
      blit_metallib, blit_metallib_len,
      dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NS::Error* err = nullptr;
  blit_lib_ = device_->newLibrary(libData, &err);
  dispatch_release(libData);
  if (blit_lib_) {
    REXLOG_INFO("MetalPresenter: Blit shader loaded");
  } else {
    REXLOG_ERROR("MetalPresenter: Failed to load blit shader: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
  }

  MTL4::ArgumentTableDescriptor* blit_v_desc =
      MTL4::ArgumentTableDescriptor::alloc()->init();
  blit_v_desc->setMaxBufferBindCount(1);
  blit_v_desc->setMaxTextureBindCount(0);
  blit_v_desc->setMaxSamplerStateBindCount(0);
  blit_v_desc->setInitializeBindings(true);
  err = nullptr;
  blit_vertex_arg_table_ = device_->newArgumentTable(blit_v_desc, &err);
  blit_v_desc->release();
  if (!blit_vertex_arg_table_) {
    REXLOG_ERROR("MetalPresenter: Failed to create blit vertex argument table: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
    return false;
  }

  MTL4::ArgumentTableDescriptor* blit_f_desc =
      MTL4::ArgumentTableDescriptor::alloc()->init();
  blit_f_desc->setMaxBufferBindCount(0);
  blit_f_desc->setMaxTextureBindCount(1);
  blit_f_desc->setMaxSamplerStateBindCount(1);
  blit_f_desc->setInitializeBindings(true);
  err = nullptr;
  blit_fragment_arg_table_ = device_->newArgumentTable(blit_f_desc, &err);
  blit_f_desc->release();
  if (!blit_fragment_arg_table_) {
    REXLOG_ERROR(
        "MetalPresenter: Failed to create blit fragment argument table: {}",
        err ? err->localizedDescription()->utf8String() : "unknown");
    return false;
  }

  REXLOG_INFO("MetalPresenter: Initialized (MTL4) on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
  if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
  if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }
  if (guest_output_shared_event_) { guest_output_shared_event_->release(); guest_output_shared_event_ = nullptr; }
  if (blit_fragment_arg_table_) { blit_fragment_arg_table_->release(); blit_fragment_arg_table_ = nullptr; }
  if (blit_vertex_arg_table_) { blit_vertex_arg_table_->release(); blit_vertex_arg_table_ = nullptr; }
  for (auto& tex : guest_output_textures_) {
    if (tex) tex->release();
    tex = nullptr;
  }
  if (presenter_capture_scope_active_ && presenter_capture_scope_) {
    presenter_capture_scope_->endScope();
    presenter_capture_scope_active_ = false;
  }
  if (presenter_capture_scope_) {
    presenter_capture_scope_->release();
    presenter_capture_scope_ = nullptr;
  }
  metal_layer_ = nullptr;
  mtl4_ = nullptr;
}

Surface::TypeFlags MetalPresenter::GetSupportedSurfaceTypes() const {
  return Surface::kTypeFlag_CAMetalLayer;
}

bool MetalPresenter::CaptureGuestOutput(RawImage& image_out) {
  return false;
}

bool MetalPresenter::CopyTextureToGuestOutput(
    MTL::Texture* source_texture, MTL::Texture* dest_texture,
    uint32_t source_width, uint32_t source_height,
    bool force_swap_rb, bool use_pwl_gamma_ramp,
    uint64_t* submission_out, uint64_t swap_sequence) {
  if (submission_out) {
    *submission_out = 0;
  }
  if (!source_texture || !dest_texture || !mtl4_) {
    return false;
  }

  uint32_t copy_width = std::min<uint32_t>(
      source_width,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->width()),
                         static_cast<uint32_t>(dest_texture->width())));
  uint32_t copy_height = std::min<uint32_t>(
      source_height,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->height()),
                         static_cast<uint32_t>(dest_texture->height())));
  if (!copy_width || !copy_height) {
    return false;
  }

  MTL::Texture* sample_texture = source_texture;
  MTL::Texture* present_view = nullptr;
  if (sample_texture->textureType() == MTL::TextureType2DArray) {
    NS::Range level_range = NS::Range::Make(0, sample_texture->mipmapLevelCount());
    NS::Range slice_range = NS::Range::Make(0, 1);
    present_view = sample_texture->newTextureView(
        sample_texture->pixelFormat(), MTL::TextureType2D, level_range,
        slice_range, sample_texture->swizzle());
    if (present_view) {
      sample_texture = present_view;
    }
  }

  MTL::Texture* swizzle_view = nullptr;
  bool swap_rb_in_shader = force_swap_rb;
  if (force_swap_rb) {
    NS::Range level_range = NS::Range::Make(0, sample_texture->mipmapLevelCount());
    NS::Range slice_range = NS::Range::Make(0, sample_texture->arrayLength());
    MTL::TextureSwizzleChannels swizzle = {
        MTL::TextureSwizzleBlue, MTL::TextureSwizzleGreen,
        MTL::TextureSwizzleRed, MTL::TextureSwizzleAlpha};
    swizzle_view = sample_texture->newTextureView(
        sample_texture->pixelFormat(), sample_texture->textureType(),
        level_range, slice_range, swizzle);
    if (swizzle_view) {
      sample_texture = swizzle_view;
      swap_rb_in_shader = false;
    }
  }

  auto release_views = [&]() {
    if (swizzle_view) {
      swizzle_view->release();
      swizzle_view = nullptr;
    }
    if (present_view) {
      present_view->release();
      present_view = nullptr;
    }
  };
  MTL4::ArgumentTable* copy_vertex_arg_table = nullptr;
  MTL4::ArgumentTable* copy_fragment_arg_table = nullptr;
  auto release_copy_tables = [&]() {
    if (copy_fragment_arg_table) {
      copy_fragment_arg_table->release();
      copy_fragment_arg_table = nullptr;
    }
    if (copy_vertex_arg_table) {
      copy_vertex_arg_table->release();
      copy_vertex_arg_table = nullptr;
    }
  };
  auto commit_with_view_lifetime = [&](MTL4::CommandBuffer* cb) {
    if (!cb) {
      return;
    }
    if (swizzle_view || present_view || copy_vertex_arg_table ||
        copy_fragment_arg_table) {
      if (swizzle_view) {
        swizzle_view->retain();
      }
      if (present_view) {
        present_view->retain();
      }
      if (copy_vertex_arg_table) {
        copy_vertex_arg_table->retain();
      }
      if (copy_fragment_arg_table) {
        copy_fragment_arg_table->retain();
      }
      MTL::Texture* swizzle_to_release = swizzle_view;
      MTL::Texture* present_to_release = present_view;
      MTL4::ArgumentTable* vertex_table_to_release = copy_vertex_arg_table;
      MTL4::ArgumentTable* fragment_table_to_release = copy_fragment_arg_table;
      MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
      options->addFeedbackHandler(
          [swizzle_to_release, present_to_release, vertex_table_to_release,
           fragment_table_to_release](MTL4::CommitFeedback*) {
            if (swizzle_to_release) {
              swizzle_to_release->release();
            }
            if (present_to_release) {
              present_to_release->release();
            }
            if (vertex_table_to_release) {
              vertex_table_to_release->release();
            }
            if (fragment_table_to_release) {
              fragment_table_to_release->release();
            }
          });
      mtl4_->Commit(cb, options);
      options->release();
    } else {
      mtl4_->Commit(cb);
    }
  };

  const bool needs_shader_copy =
      swap_rb_in_shader || use_pwl_gamma_ramp ||
      sample_texture->pixelFormat() != dest_texture->pixelFormat() ||
      sample_texture->textureType() != MTL::TextureType2D;
  static uint32_t copy_path_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && copy_path_log_count < 64) {
    fprintf(stderr,
            "[present-copy-path] seq=%llu %s srcfmt=%u dstfmt=%u rb=%u "
            "gamma=%u src_type=%u dst_type=%u\n",
            static_cast<unsigned long long>(swap_sequence),
            needs_shader_copy ? "shader" : "raw",
            static_cast<uint32_t>(sample_texture->pixelFormat()),
            static_cast<uint32_t>(dest_texture->pixelFormat()),
            force_swap_rb ? 1u : 0u, use_pwl_gamma_ramp ? 1u : 0u,
            static_cast<uint32_t>(sample_texture->textureType()),
            static_cast<uint32_t>(dest_texture->textureType()));
    fflush(stderr);
    ++copy_path_log_count;
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) {
    release_views();
    return false;
  }

  // MTL4 explicit residency: source/destination textures in the present path
  // may not be tracked by draw-time bindings for this command buffer.
  mtl4_->AddResidentAllocation(source_texture);
  mtl4_->AddResidentAllocation(sample_texture);
  mtl4_->AddResidentAllocation(dest_texture);
  mtl4_->CommitResidency();

  bool copy_success = false;
  if (!needs_shader_copy) {
    MTL4::ComputeCommandEncoder* compute = cmd->computeCommandEncoder();
    if (compute) {
      compute->copyFromTexture(sample_texture, 0, 0, MTL::Origin(0, 0, 0),
                               MTL::Size(copy_width, copy_height, 1),
                               dest_texture, 0, 0, MTL::Origin(0, 0, 0));
      compute->endEncoding();
      copy_success = true;
    }
  } else {
    if (blit_lib_) {
      MTL::RenderPipelineState* pipe =
          GetOrCreateBlitPipeline(dest_texture->pixelFormat());
      if (pipe) {
        MTL4::ArgumentTableDescriptor* blit_v_desc =
            MTL4::ArgumentTableDescriptor::alloc()->init();
        blit_v_desc->setMaxBufferBindCount(1);
        blit_v_desc->setMaxTextureBindCount(0);
        blit_v_desc->setMaxSamplerStateBindCount(0);
        blit_v_desc->setInitializeBindings(true);
        NS::Error* table_err = nullptr;
        copy_vertex_arg_table = device_->newArgumentTable(blit_v_desc, &table_err);
        blit_v_desc->release();
        if (!copy_vertex_arg_table) {
          REXLOG_WARN(
              "MetalPresenter: Failed to create copy vertex argument table: {}",
              table_err ? table_err->localizedDescription()->utf8String()
                        : "unknown");
        }
        MTL4::ArgumentTableDescriptor* blit_f_desc =
            MTL4::ArgumentTableDescriptor::alloc()->init();
        blit_f_desc->setMaxBufferBindCount(0);
        blit_f_desc->setMaxTextureBindCount(1);
        blit_f_desc->setMaxSamplerStateBindCount(1);
        blit_f_desc->setInitializeBindings(true);
        table_err = nullptr;
        copy_fragment_arg_table =
            device_->newArgumentTable(blit_f_desc, &table_err);
        blit_f_desc->release();
        if (!copy_fragment_arg_table) {
          REXLOG_WARN(
              "MetalPresenter: Failed to create copy fragment argument table: {}",
              table_err ? table_err->localizedDescription()->utf8String()
                        : "unknown");
        }
        MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
        auto* ca = rpd->colorAttachments()->object(0);
        ca->setTexture(dest_texture);
        ca->setLoadAction(MTL::LoadActionClear);
        ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
        ca->setStoreAction(MTL::StoreActionStore);
        MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
        if (enc) {
          enc->setRenderPipelineState(pipe);
          enc->setViewport(
              MTL::Viewport(0.0, 0.0, double(copy_width), double(copy_height), 0.0, 1.0));
          enc->setScissorRect(MTL::ScissorRect(0, 0, copy_width, copy_height));
          struct {
            float w, h, src_w, src_h;
          } ub = {(float)dest_texture->width(), (float)dest_texture->height(),
                  (float)sample_texture->width(), (float)sample_texture->height()};
          if (copy_vertex_arg_table && copy_fragment_arg_table) {
            copy_vertex_arg_table->setAddress(
                mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
            copy_fragment_arg_table->setTexture(sample_texture->gpuResourceID(),
                                                0);
            copy_fragment_arg_table->setSamplerState(
                GetNearestSampler()->gpuResourceID(), 0);
            enc->setArgumentTable(copy_vertex_arg_table, MTL::RenderStageVertex);
            enc->setArgumentTable(copy_fragment_arg_table,
                                  MTL::RenderStageFragment);
          }
          enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                              NS::UInteger(3));
          enc->endEncoding();
          copy_success = true;
        }
        rpd->release();
      }
    }
  }

  if (!copy_success) {
    commit_with_view_lifetime(cmd);
    release_views();
    release_copy_tables();
    return false;
  }

  uint64_t submission_id = 0;
  if (guest_output_shared_event_) {
    submission_id =
        guest_output_submission_counter_.fetch_add(1, std::memory_order_relaxed) +
        1;
  }

  commit_with_view_lifetime(cmd);

  static uint32_t copy_probe_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && copy_probe_log_count < 8 &&
      device_ && mtl4_) {
    constexpr size_t kProbeStride = 16;
    constexpr size_t kProbePointCount = 5;
    auto make_probe_points = [](uint32_t w, uint32_t h)
        -> std::array<std::pair<uint32_t, uint32_t>, kProbePointCount> {
      const uint32_t max_x = w ? (w - 1) : 0;
      const uint32_t max_y = h ? (h - 1) : 0;
      return {{{max_x / 2, max_y / 2},
               {max_x / 4, max_y / 4},
               {(max_x * 3) / 4, max_y / 4},
               {max_x / 4, (max_y * 3) / 4},
               {(max_x * 3) / 4, (max_y * 3) / 4}}};
    };
    const auto src_points =
        make_probe_points(static_cast<uint32_t>(sample_texture->width()),
                          static_cast<uint32_t>(sample_texture->height()));
    const auto dst_points =
        make_probe_points(static_cast<uint32_t>(dest_texture->width()),
                          static_cast<uint32_t>(dest_texture->height()));
    MTL::Buffer* probe_buffer =
        device_->newBuffer(kProbeStride * kProbePointCount * 2,
                           MTL::ResourceStorageModeShared);
    if (probe_buffer) {
      MTL4::CommandBuffer* probe_cmd = mtl4_->BeginStandaloneCommandBuffer();
      if (probe_cmd) {
        MTL4::ComputeCommandEncoder* probe_enc = probe_cmd->computeCommandEncoder();
        if (probe_enc) {
          mtl4_->AddResidentAllocation(sample_texture);
          mtl4_->AddResidentAllocation(dest_texture);
          mtl4_->AddResidentAllocation(probe_buffer);
          mtl4_->CommitResidency();
          for (size_t i = 0; i < kProbePointCount; ++i) {
            const size_t src_off = i * kProbeStride;
            const size_t dst_off = (kProbePointCount * kProbeStride) + src_off;
            probe_enc->copyFromTexture(
                sample_texture, 0, 0,
                MTL::Origin(src_points[i].first, src_points[i].second, 0),
                MTL::Size(1, 1, 1), probe_buffer, src_off, kProbeStride,
                kProbeStride);
            probe_enc->copyFromTexture(
                dest_texture, 0, 0,
                MTL::Origin(dst_points[i].first, dst_points[i].second, 0),
                MTL::Size(1, 1, 1), probe_buffer, dst_off, kProbeStride,
                kProbeStride);
          }
          probe_enc->endEncoding();
          mtl4_->CommitStandaloneAndWait(probe_cmd);
          const uint8_t* b =
              reinterpret_cast<const uint8_t*>(probe_buffer->contents());
          uint32_t src_coherence = 0;
          uint32_t dst_coherence = 0;
          std::array<uint32_t, kProbePointCount> src_nonzero = {};
          std::array<uint32_t, kProbePointCount> dst_nonzero = {};
          std::array<uint32_t, kProbePointCount> src_rgba = {};
          std::array<uint32_t, kProbePointCount> dst_rgba = {};
          for (size_t p = 0; p < kProbePointCount; ++p) {
            const size_t src_off = p * kProbeStride;
            const size_t dst_off = (kProbePointCount * kProbeStride) + src_off;
            for (size_t i = 0; i < kProbeStride; ++i) {
              src_nonzero[p] += b[src_off + i] ? 1u : 0u;
              dst_nonzero[p] += b[dst_off + i] ? 1u : 0u;
            }
            src_rgba[p] = uint32_t(b[src_off + 0]) |
                          (uint32_t(b[src_off + 1]) << 8) |
                          (uint32_t(b[src_off + 2]) << 16) |
                          (uint32_t(b[src_off + 3]) << 24);
            dst_rgba[p] = uint32_t(b[dst_off + 0]) |
                          (uint32_t(b[dst_off + 1]) << 8) |
                          (uint32_t(b[dst_off + 2]) << 16) |
                          (uint32_t(b[dst_off + 3]) << 24);
            src_coherence += src_nonzero[p] ? 1u : 0u;
            dst_coherence += dst_nonzero[p] ? 1u : 0u;
          }
          fprintf(stderr,
                  "[present-probe] seq=%llu src_coh=%u dst_coh=%u "
                  "c=%u/0x%08X->%u/0x%08X ul=%u/0x%08X->%u/0x%08X "
                  "ur=%u/0x%08X->%u/0x%08X ll=%u/0x%08X->%u/0x%08X "
                  "lr=%u/0x%08X->%u/0x%08X\n",
                  static_cast<unsigned long long>(swap_sequence), src_coherence,
                  dst_coherence, src_nonzero[0], src_rgba[0], dst_nonzero[0],
                  dst_rgba[0], src_nonzero[1], src_rgba[1], dst_nonzero[1],
                  dst_rgba[1], src_nonzero[2], src_rgba[2], dst_nonzero[2],
                  dst_rgba[2], src_nonzero[3], src_rgba[3], dst_nonzero[3],
                  dst_rgba[3], src_nonzero[4], src_rgba[4], dst_nonzero[4],
                  dst_rgba[4]);
          fflush(stderr);
          ++copy_probe_log_count;
        } else {
          probe_cmd->release();
        }
      }
      probe_buffer->release();
    }
  }

  if (submission_id && guest_output_shared_event_) {
    mtl4_->SignalEvent(guest_output_shared_event_, submission_id);
  }

  if (submission_out) {
    *submission_out = submission_id;
  }
  release_views();
  release_copy_tables();
  return true;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentImpl(
    bool execute_ui_drawers) {
  if (!metal_layer_ || !device_ || !mtl4_) {
    return PaintResult::kNotPresented;
  }

  auto* pool = NS::AutoreleasePool::alloc()->init();
  CA::MetalLayer* layer = reinterpret_cast<CA::MetalLayer*>(metal_layer_);
  CA::MetalDrawable* drawable = layer->nextDrawable();
  if (!drawable) {
    pool->drain();
    return PaintResult::kNotPresented;
  }

  MTL::Texture* dst = drawable->texture();
  const uint64_t paint_sequence =
      present_count_.fetch_add(1, std::memory_order_relaxed) + 1;
  file_capture_last_seq_ = paint_sequence;
  if (file_capture_active_ && file_capture_stop_pending_) {
#if defined(__APPLE__)
    if (REXCVAR_GET(metal_capture_diag_shim)) {
      RexMetalCaptureDiagMark("before-stop-deferred");
      RexMetalCaptureDiagStop(file_capture_path_.c_str());
      RexMetalCaptureDiagMark("after-stop-deferred");
    } else
#endif
    {
      MTL::CaptureManager* capture_manager =
          MTL::CaptureManager::sharedCaptureManager();
      if (capture_manager && capture_manager->isCapturing()) {
        capture_manager->stopCapture();
      }
    }
    std::error_code ec;
    const bool output_exists = std::filesystem::exists(file_capture_path_, ec);
    const auto captured_size =
        output_exists ? ComputePathBytes(std::filesystem::path(file_capture_path_))
                      : uintmax_t(0);
    AppendCaptureEvent(
        "[metal-capture] stopped(deferred) path=%s seq=%llu exists=%d size=%llu ec=%d",
        file_capture_path_.c_str(),
        static_cast<unsigned long long>(paint_sequence),
        output_exists ? 1 : 0,
        static_cast<unsigned long long>(captured_size), ec ? ec.value() : 0);
    file_capture_active_ = false;
    file_capture_stop_pending_ = false;
    file_capture_frames_remaining_ = 0;
    file_capture_max_bytes_ = 0;
    file_capture_start_us_ = 0;
    file_capture_path_.clear();
    REXCVAR_SET(metal_capture_to_file, false);
    REXCVAR_SET(metal_capture_copy_scope_once, false);
  }
  if (!file_capture_active_ && REXCVAR_GET(metal_capture_trigger_file_enabled)) {
    const std::string trigger_path = REXCVAR_GET(metal_capture_trigger_file_path);
    if (!trigger_path.empty()) {
      std::error_code ec;
      if (std::filesystem::exists(trigger_path, ec) && !ec) {
        std::filesystem::remove(trigger_path, ec);
        REXCVAR_SET(metal_capture_to_file, true);
        if (REXCVAR_GET(metal_capture_to_file_frames) < 1) {
          REXCVAR_SET(metal_capture_to_file_frames, 1);
        }
        AppendCaptureEvent(
            "[metal-capture] trigger-file detected path=%s seq=%llu remove_ec=%d",
            trigger_path.c_str(),
            static_cast<unsigned long long>(paint_sequence),
            ec ? ec.value() : 0);
      }
    }
  }
  const bool legacy_capture_once = REXCVAR_GET(metal_capture_copy_scope_once);
  const bool legacy_capture_to_file = REXCVAR_GET(metal_capture_copy_scope_to_file);
  if (!file_capture_active_ && (legacy_capture_once || legacy_capture_to_file)) {
    REXCVAR_SET(metal_capture_to_file, true);
    if (REXCVAR_GET(metal_capture_to_file_frames) < 1) {
      REXCVAR_SET(metal_capture_to_file_frames, 1);
    }
  }
  if (legacy_capture_once || legacy_capture_to_file) {
    static bool logged_legacy_alias_once = false;
    if (!logged_legacy_alias_once) {
      fprintf(stderr,
              "[metal-capture] legacy capture alias active once=%d to_file=%d\n",
              legacy_capture_once ? 1 : 0, legacy_capture_to_file ? 1 : 0);
      fflush(stderr);
      logged_legacy_alias_once = true;
    }
  }
  if (!file_capture_active_ && REXCVAR_GET(metal_capture_to_file) && mtl4_) {
    MTL::CaptureManager* capture_manager = MTL::CaptureManager::sharedCaptureManager();
    if (capture_manager && !capture_manager->isCapturing()) {
      const bool tiny_sanity_capture = REXCVAR_GET(metal_capture_tiny_sanity);
      std::string configured_capture_path = REXCVAR_GET(metal_capture_to_file_path);
      if (configured_capture_path.empty()) {
        configured_capture_path =
            "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-present-capture.gputrace";
      }
      if (tiny_sanity_capture) {
        configured_capture_path = REXCVAR_GET(metal_capture_tiny_path);
        if (configured_capture_path.empty()) {
          configured_capture_path =
              "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-tiny-sanity.gputrace";
        }
      }
      const std::string capture_path =
          MakeUniqueCapturePath(configured_capture_path, paint_sequence);
      std::error_code ec;
      std::filesystem::path capture_fs_path = capture_path;
      const std::filesystem::path capture_parent = capture_fs_path.parent_path();
      if (!capture_parent.empty()) {
        std::filesystem::create_directories(capture_parent, ec);
        if (ec) {
          AppendCaptureEvent(
              "[metal-capture] mkdir failed path=%s ec=%d msg=%s",
              capture_parent.string().c_str(), ec.value(),
              ec.message().c_str());
        }
      }
      if (std::filesystem::exists(capture_fs_path, ec)) {
        const uintmax_t removed_count =
            std::filesystem::remove_all(capture_fs_path, ec);
        if (ec) {
          AppendCaptureEvent(
              "[metal-capture] pre-remove failed path=%s ec=%d msg=%s",
              capture_path.c_str(), ec.value(), ec.message().c_str());
        } else {
          AppendCaptureEvent(
              "[metal-capture] pre-remove ok path=%s removed=%llu",
              capture_path.c_str(),
              static_cast<unsigned long long>(removed_count));
        }
      }

      const bool want_devtools = REXCVAR_GET(metal_capture_to_developer_tools);
      const bool supports_devtools =
          capture_manager->supportsDestination(
              MTL::CaptureDestinationDeveloperTools);
      const bool supports_gpu_trace_doc =
          capture_manager->supportsDestination(
              MTL::CaptureDestinationGPUTraceDocument);
      const bool use_devtools = want_devtools ? supports_devtools : false;
      const bool use_gpu_trace = use_devtools ? false : supports_gpu_trace_doc;
      AppendCaptureEvent(
          "[metal-capture] start-request seq=%llu path=%s requested=%s tiny=%d "
          "supports_doc=%d supports_tools=%d is_capturing=%d scope=%d queue=%d dst=%s",
          static_cast<unsigned long long>(paint_sequence),
          capture_path.c_str(), configured_capture_path.c_str(),
          tiny_sanity_capture ? 1 : 0,
          supports_gpu_trace_doc ? 1 : 0,
          supports_devtools ? 1 : 0,
          capture_manager->isCapturing() ? 1 : 0,
          REXCVAR_GET(metal_capture_to_file_use_scope) ? 1 : 0,
          REXCVAR_GET(metal_capture_to_file_use_queue) ? 1 : 0,
          use_devtools ? "tools" : (use_gpu_trace ? "gputrace" : "unsupported"));

      // Scope-object capture can crash on some Xcode/macOS builds when writing
      // GPUTraceDocument output, so only use it when not file-output capturing.
      const bool use_scope_capture =
          false && REXCVAR_GET(metal_capture_to_file_use_scope) &&
          presenter_capture_scope_;
      const bool use_queue_capture =
          !use_scope_capture && REXCVAR_GET(metal_capture_to_file_use_queue) &&
          mtl4_->queue();
      void* capture_object = use_scope_capture
                                 ? reinterpret_cast<void*>(presenter_capture_scope_)
                                 : (use_queue_capture
                                        ? reinterpret_cast<void*>(mtl4_->queue())
                                        : reinterpret_cast<void*>(device_));

#if defined(__APPLE__)
      RexMetalCaptureDiagMark("before-start");
#endif
      bool capture_started = false;
#if defined(__APPLE__)
      if (REXCVAR_GET(metal_capture_diag_shim) && !use_devtools) {
        capture_started =
            RexMetalCaptureDiagStart(capture_object, capture_path.c_str());
      } else
#endif
      {
        NS::String* capture_path_ns =
            NS::String::string(capture_path.c_str(), NS::UTF8StringEncoding);
        NS::URL* capture_url = NS::URL::fileURLWithPath(capture_path_ns);
        MTL::CaptureDescriptor* descriptor = MTL::CaptureDescriptor::alloc()->init();
        descriptor->setCaptureObject(reinterpret_cast<NS::Object*>(capture_object));
        if (use_devtools) {
          descriptor->setDestination(MTL::CaptureDestinationDeveloperTools);
        } else {
          descriptor->setDestination(MTL::CaptureDestinationGPUTraceDocument);
          descriptor->setOutputURL(capture_url);
        }
        NS::Error* capture_err = nullptr;
        capture_started = capture_manager->startCapture(descriptor, &capture_err);
        if (!capture_started) {
          AppendCaptureEvent(
              "[metal-capture] start-failed path=%s err=%s object=%s dst=%s",
              capture_path.c_str(),
              capture_err ? capture_err->localizedDescription()->utf8String()
                          : "unknown",
              use_scope_capture ? "scope"
                                : (use_queue_capture ? "queue" : "device"),
              use_devtools ? "tools" : "gputrace");
        }
        descriptor->release();
      }
#if defined(__APPLE__)
      RexMetalCaptureDiagMark(capture_started ? "after-start-ok"
                                              : "after-start-failed");
#endif
      if (capture_started) {
        if (tiny_sanity_capture) {
          bool tiny_cmd_committed = false;
#if defined(__APPLE__)
          if (REXCVAR_GET(metal_capture_diag_shim)) {
            RexMetalCaptureDiagMark("before-tiny-cmd-create");
          }
#endif
          MTL4::CommandBuffer* tiny_cmd = mtl4_->BeginStandaloneCommandBuffer();
#if defined(__APPLE__)
          if (REXCVAR_GET(metal_capture_diag_shim)) {
            RexMetalCaptureDiagMark("after-tiny-cmd-create");
          }
#endif
          if (tiny_cmd) {
#if defined(__APPLE__)
            if (REXCVAR_GET(metal_capture_diag_shim)) {
              RexMetalCaptureDiagMark("before-tiny-cmd-commit");
            }
#endif
            mtl4_->CommitStandaloneAndWait(tiny_cmd);
            tiny_cmd_committed = true;
#if defined(__APPLE__)
            if (REXCVAR_GET(metal_capture_diag_shim)) {
              RexMetalCaptureDiagMark("after-tiny-cmd-commit");
            }
#endif
          }
#if defined(__APPLE__)
          if (REXCVAR_GET(metal_capture_diag_shim)) {
            RexMetalCaptureDiagMark("before-stop-tiny");
            RexMetalCaptureDiagStop(capture_path.c_str());
            RexMetalCaptureDiagMark("after-stop-tiny");
          } else
#endif
          {
            if (capture_manager->isCapturing()) {
              capture_manager->stopCapture();
            }
          }
          std::error_code ec;
          const bool output_exists = std::filesystem::exists(capture_path, ec);
          const auto stopped_size =
              output_exists ? ComputePathBytes(std::filesystem::path(capture_path))
                            : uintmax_t(0);
          AppendCaptureEvent(
              "[metal-capture] stopped(tiny) path=%s seq=%llu tiny_cmd=%d exists=%d final_size=%llu ec=%d",
              capture_path.c_str(),
              static_cast<unsigned long long>(paint_sequence),
              tiny_cmd_committed ? 1 : 0, output_exists ? 1 : 0,
              static_cast<unsigned long long>(stopped_size),
              ec ? ec.value() : 0);
          fprintf(stderr,
                  "[metal-capture] tiny sanity capture done path=%s seq=%llu cmd=%d size=%llu\n",
                  capture_path.c_str(),
                  static_cast<unsigned long long>(paint_sequence),
                  tiny_cmd_committed ? 1 : 0,
                  static_cast<unsigned long long>(stopped_size));
          REXCVAR_SET(metal_capture_tiny_sanity, false);
          REXCVAR_SET(metal_capture_to_file, false);
          REXCVAR_SET(metal_capture_copy_scope_once, false);
          REXCVAR_SET(metal_capture_copy_scope_to_file, false);
        } else {
        const int32_t requested_frames =
            std::max(1, REXCVAR_GET(metal_capture_to_file_frames));
        const int32_t minimum_frames =
            std::max(1, REXCVAR_GET(metal_capture_to_file_min_frames));
        file_capture_active_ = true;
        file_capture_stop_pending_ = false;
        file_capture_frames_remaining_ = std::max(requested_frames, minimum_frames);
        file_capture_max_bytes_ =
            static_cast<uint64_t>(
                std::max(1, REXCVAR_GET(metal_capture_to_file_max_mb))) *
            1024ull * 1024ull;
        file_capture_start_us_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
        file_capture_path_ = capture_path;
        fprintf(stderr,
                "[metal-capture] started file capture path=%s frames=%d seq=%llu\n",
                file_capture_path_.c_str(), file_capture_frames_remaining_,
                static_cast<unsigned long long>(paint_sequence));
        AppendCaptureEvent(
            "[metal-capture] started path=%s frames=%d seq=%llu object=%s dst=%s max_mb=%d max_ms=%d",
            file_capture_path_.c_str(), file_capture_frames_remaining_,
            static_cast<unsigned long long>(paint_sequence),
            use_scope_capture ? "scope"
                              : (use_queue_capture ? "queue" : "device"),
            use_devtools ? "tools" : "gputrace",
            std::max(1, REXCVAR_GET(metal_capture_to_file_max_mb)),
            std::max(1, REXCVAR_GET(metal_capture_to_file_max_ms)));
        // Consume one-shot triggers immediately so failed/fallback state changes
        // don't auto-rearm capture and create runaway large traces.
        REXCVAR_SET(metal_capture_to_file, false);
        REXCVAR_SET(metal_capture_copy_scope_once, false);
        REXCVAR_SET(metal_capture_copy_scope_to_file, false);
        }
      } else {
        fprintf(stderr, "[metal-capture] start failed path=%s\n",
                capture_path.c_str());
        REXCVAR_SET(metal_capture_to_file, false);
        REXCVAR_SET(metal_capture_copy_scope_once, false);
        REXCVAR_SET(metal_capture_tiny_sanity, false);
      }
    }
  }
  constexpr size_t kProbePointCount = 5;
  auto make_probe_points = [](uint32_t w, uint32_t h)
      -> std::array<std::pair<uint32_t, uint32_t>, kProbePointCount> {
    const uint32_t max_x = w ? (w - 1) : 0;
    const uint32_t max_y = h ? (h - 1) : 0;
    return {{{max_x / 2, max_y / 2},
             {max_x / 4, max_y / 4},
             {(max_x * 3) / 4, max_y / 4},
             {max_x / 4, (max_y * 3) / 4},
             {(max_x * 3) / 4, (max_y * 3) / 4}}};
  };
  static uint32_t drawable_probe_log_count = 0;
  const bool probe_drawable =
      REXCVAR_GET(metal_present_debug_probes) && drawable_probe_log_count < 8;
  MTL::Buffer* drawable_probe_buffer = nullptr;
  if (probe_drawable && device_) {
    drawable_probe_buffer =
        device_->newBuffer(size_t(16 * kProbePointCount),
                           MTL::ResourceStorageModeShared);
  }

  uint32_t mailbox_index = UINT32_MAX;
  GuestOutputProperties guest_output_properties;
  GuestOutputPaintConfig guest_output_paint_config;
  MTL::Texture* guest_output_texture = nullptr;
  {
    std::unique_lock<std::mutex> guest_output_consumer_lock(
        this->ConsumeGuestOutput(mailbox_index, &guest_output_properties,
                                 &guest_output_paint_config));
    if (mailbox_index != UINT32_MAX && mailbox_index < guest_output_textures_.size()) {
      uint64_t await_submission = guest_output_submissions_[mailbox_index];
      if (await_submission > guest_output_waited_submission_ &&
          guest_output_shared_event_) {
        uint64_t completed_submission =
            guest_output_shared_event_->signaledValue();
        if (await_submission > completed_submission) {
          mtl4_->WaitEvent(guest_output_shared_event_, await_submission);
        }
        guest_output_waited_submission_ = await_submission;
      }
      guest_output_texture = guest_output_textures_[mailbox_index];
    }
  }

  static int paint_log_count = 0;
  if (paint_log_count < 12) {
    fprintf(stderr,
            "[present] seq=%llu paint mailbox=%u tex=%p direct=%p draw=%ux%u "
            "tex=%ux%u\n",
            static_cast<unsigned long long>(paint_sequence), mailbox_index,
            (void*)guest_output_texture,
            (void*)nullptr,
            static_cast<uint32_t>(dst->width()),
            static_cast<uint32_t>(dst->height()),
            guest_output_texture
                ? static_cast<uint32_t>(guest_output_texture->width())
                : 0u,
            guest_output_texture
                ? static_cast<uint32_t>(guest_output_texture->height())
                : 0u);
    fflush(stderr);
    ++paint_log_count;
  }

  static uint32_t paint_probe_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && paint_probe_log_count < 8 &&
      guest_output_texture && device_ && mtl4_) {
    constexpr size_t kProbeStride = 16;
    const auto probe_points = make_probe_points(
        static_cast<uint32_t>(guest_output_texture->width()),
        static_cast<uint32_t>(guest_output_texture->height()));
    MTL::Buffer* probe_buffer =
        device_->newBuffer(kProbeStride * kProbePointCount,
                           MTL::ResourceStorageModeShared);
    if (probe_buffer) {
      MTL4::CommandBuffer* probe_cmd = mtl4_->BeginStandaloneCommandBuffer();
      if (probe_cmd) {
        MTL4::ComputeCommandEncoder* probe_enc = probe_cmd->computeCommandEncoder();
        if (probe_enc) {
          mtl4_->AddResidentAllocation(guest_output_texture);
          mtl4_->AddResidentAllocation(probe_buffer);
          mtl4_->CommitResidency();
          for (size_t i = 0; i < kProbePointCount; ++i) {
            probe_enc->copyFromTexture(
                guest_output_texture, 0, 0,
                MTL::Origin(probe_points[i].first, probe_points[i].second, 0),
                MTL::Size(1, 1, 1), probe_buffer, i * kProbeStride,
                kProbeStride, kProbeStride);
          }
          probe_enc->endEncoding();
          mtl4_->CommitStandaloneAndWait(probe_cmd);
          const uint8_t* b =
              reinterpret_cast<const uint8_t*>(probe_buffer->contents());
          std::array<uint32_t, kProbePointCount> nonzero = {};
          std::array<uint32_t, kProbePointCount> rgba = {};
          uint32_t coherence = 0;
          for (size_t p = 0; p < kProbePointCount; ++p) {
            const size_t off = p * kProbeStride;
            for (size_t i = 0; i < kProbeStride; ++i) {
              nonzero[p] += b[off + i] ? 1u : 0u;
            }
            rgba[p] = uint32_t(b[off + 0]) | (uint32_t(b[off + 1]) << 8) |
                      (uint32_t(b[off + 2]) << 16) |
                      (uint32_t(b[off + 3]) << 24);
            coherence += nonzero[p] ? 1u : 0u;
          }
          fprintf(stderr,
                  "[paint-probe] seq=%llu coh=%u c=%u/0x%08X ul=%u/0x%08X "
                  "ur=%u/0x%08X ll=%u/0x%08X lr=%u/0x%08X tex=%ux%u\n",
                  static_cast<unsigned long long>(paint_sequence), coherence,
                  nonzero[0], rgba[0], nonzero[1], rgba[1], nonzero[2],
                  rgba[2], nonzero[3], rgba[3], nonzero[4], rgba[4],
                  static_cast<uint32_t>(guest_output_texture->width()),
                  static_cast<uint32_t>(guest_output_texture->height()));
          fflush(stderr);
          ++paint_probe_log_count;
        } else {
          probe_cmd->release();
        }
      }
      probe_buffer->release();
    }
  }

  const bool capture_to_file_requested =
      file_capture_active_ || REXCVAR_GET(metal_capture_to_file);
  const bool capture_with_scope_object =
      capture_to_file_requested &&
      REXCVAR_GET(metal_capture_to_file_use_scope) && presenter_capture_scope_;
  const bool use_capture_scope =
      REXCVAR_GET(metal_present_capture_scope) && presenter_capture_scope_ &&
      (!capture_to_file_requested || capture_with_scope_object);
  if (use_capture_scope && !presenter_capture_scope_active_) {
    presenter_capture_scope_->beginScope();
    presenter_capture_scope_active_ = true;
  }

#if defined(__APPLE__)
  if (REXCVAR_GET(metal_capture_diag_shim) && file_capture_active_) {
    RexMetalCaptureDiagMark("before-command-buffer-create");
  }
#endif
  MTL4::CommandBuffer* cmd = mtl4_->BeginPresenterCommandBuffer();
#if defined(__APPLE__)
  if (REXCVAR_GET(metal_capture_diag_shim) && file_capture_active_) {
    RexMetalCaptureDiagMark("after-command-buffer-create");
  }
#endif
  if (!cmd) {
    if (presenter_capture_scope_active_) {
      presenter_capture_scope_->endScope();
      presenter_capture_scope_active_ = false;
    }
    pool->drain();
    return PaintResult::kNotPresented;
  }
  MTL4::ArgumentTable* paint_vertex_arg_table = nullptr;
  MTL4::ArgumentTable* paint_fragment_arg_table = nullptr;
  auto release_paint_tables = [&]() {
    if (paint_fragment_arg_table) {
      paint_fragment_arg_table->release();
      paint_fragment_arg_table = nullptr;
    }
    if (paint_vertex_arg_table) {
      paint_vertex_arg_table->release();
      paint_vertex_arg_table = nullptr;
    }
  };

  // MTL4 explicit residency: the present pass writes directly into the
  // drawable and samples the mailbox texture, so both allocations must be
  // resident for this command buffer.
  mtl4_->AddResidentAllocation(dst);
  if (guest_output_texture) {
    mtl4_->AddResidentAllocation(guest_output_texture);
  }
  mtl4_->CommitResidency();

  if (guest_output_texture && blit_lib_) {
    MTL::RenderPipelineState* pipe = GetOrCreateBlitPipeline(dst->pixelFormat());
    if (pipe) {
      MTL4::ArgumentTableDescriptor* blit_v_desc =
          MTL4::ArgumentTableDescriptor::alloc()->init();
      blit_v_desc->setMaxBufferBindCount(1);
      blit_v_desc->setMaxTextureBindCount(0);
      blit_v_desc->setMaxSamplerStateBindCount(0);
      blit_v_desc->setInitializeBindings(true);
      NS::Error* table_err = nullptr;
      paint_vertex_arg_table = device_->newArgumentTable(blit_v_desc, &table_err);
      blit_v_desc->release();
      if (!paint_vertex_arg_table) {
        REXLOG_WARN(
            "MetalPresenter: Failed to create paint vertex argument table: {}",
            table_err ? table_err->localizedDescription()->utf8String()
                      : "unknown");
      }
      MTL4::ArgumentTableDescriptor* blit_f_desc =
          MTL4::ArgumentTableDescriptor::alloc()->init();
      blit_f_desc->setMaxBufferBindCount(0);
      blit_f_desc->setMaxTextureBindCount(1);
      blit_f_desc->setMaxSamplerStateBindCount(1);
      blit_f_desc->setInitializeBindings(true);
      table_err = nullptr;
      paint_fragment_arg_table = device_->newArgumentTable(blit_f_desc, &table_err);
      blit_f_desc->release();
      if (!paint_fragment_arg_table) {
        REXLOG_WARN(
            "MetalPresenter: Failed to create paint fragment argument table: {}",
            table_err ? table_err->localizedDescription()->utf8String()
                      : "unknown");
      }
      MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dst);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);
        enc->setViewport(
            MTL::Viewport(0.0, 0.0, double(dst->width()), double(dst->height()), 0.0, 1.0));
        enc->setScissorRect(
            MTL::ScissorRect(0, 0, uint32_t(dst->width()), uint32_t(dst->height())));

        float src_h = guest_output_texture->height() > 720 ? 720.0f
                          : (float)guest_output_texture->height();
        struct {
          float w, h, src_w, src_h;
        } ub = {(float)dst->width(), (float)dst->height(),
                (float)guest_output_texture->width(), src_h};

        if (paint_vertex_arg_table && paint_fragment_arg_table) {
          paint_vertex_arg_table->setAddress(
              mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
          paint_fragment_arg_table->setTexture(guest_output_texture->gpuResourceID(),
                                               0);
          paint_fragment_arg_table->setSamplerState(
              GetNearestSampler()->gpuResourceID(), 0);
          enc->setArgumentTable(paint_vertex_arg_table, MTL::RenderStageVertex);
          enc->setArgumentTable(paint_fragment_arg_table,
                                MTL::RenderStageFragment);
        }

        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                            NS::UInteger(3));
        enc->endEncoding();
      }
      rpd->release();
    }
  } else {
    static bool logged_once = false;
    if (!logged_once) {
      REXLOG_WARN("Metal Paint: No guest output texture (mailbox_idx={}, tex={})",
                  mailbox_index, (void*)guest_output_texture);
      logged_once = true;
    }
  }

  if (drawable_probe_buffer) {
    MTL4::ComputeCommandEncoder* drawable_probe_enc = cmd->computeCommandEncoder();
    if (drawable_probe_enc) {
      constexpr size_t kProbeStride = 16;
      const auto probe_points = make_probe_points(
          static_cast<uint32_t>(dst->width()),
          static_cast<uint32_t>(dst->height()));
      std::memset(drawable_probe_buffer->contents(), 0,
                  kProbeStride * kProbePointCount);
      for (size_t i = 0; i < kProbePointCount; ++i) {
        drawable_probe_enc->copyFromTexture(
            dst, 0, 0,
            MTL::Origin::Make(probe_points[i].first, probe_points[i].second, 0),
            MTL::Size::Make(1, 1, 1), drawable_probe_buffer, i * kProbeStride,
            kProbeStride, kProbeStride);
      }
      drawable_probe_enc->endEncoding();
    } else {
      drawable_probe_buffer->release();
      drawable_probe_buffer = nullptr;
    }
  }

  MTL4::CommitOptions* paint_commit_options = nullptr;
  if (paint_vertex_arg_table || paint_fragment_arg_table ||
      drawable_probe_buffer) {
    if (paint_vertex_arg_table) {
      paint_vertex_arg_table->retain();
    }
    if (paint_fragment_arg_table) {
      paint_fragment_arg_table->retain();
    }
    if (drawable_probe_buffer) {
      drawable_probe_buffer->retain();
    }
    MTL4::ArgumentTable* vertex_table_to_release = paint_vertex_arg_table;
    MTL4::ArgumentTable* fragment_table_to_release = paint_fragment_arg_table;
    MTL::Buffer* probe_buffer_to_log = drawable_probe_buffer;
    const uint32_t probe_w = uint32_t(dst->width());
    const uint32_t probe_h = uint32_t(dst->height());
    const uint64_t probe_sequence = paint_sequence;
    paint_commit_options = MTL4::CommitOptions::alloc()->init();
    paint_commit_options->addFeedbackHandler(
        [vertex_table_to_release, fragment_table_to_release,
         probe_buffer_to_log, probe_w, probe_h, probe_sequence](
            MTL4::CommitFeedback*) {
          if (vertex_table_to_release) {
            vertex_table_to_release->release();
          }
          if (fragment_table_to_release) {
            fragment_table_to_release->release();
          }
          if (probe_buffer_to_log) {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(
                probe_buffer_to_log->contents());
            if (b) {
              constexpr size_t kProbeStride = 16;
              constexpr size_t kLocalProbePointCount = 5;
              std::array<uint32_t, kLocalProbePointCount> nonzero = {};
              std::array<uint32_t, kLocalProbePointCount> rgba = {};
              uint32_t coherence = 0;
              for (size_t p = 0; p < kLocalProbePointCount; ++p) {
                const size_t off = p * kProbeStride;
                for (size_t i = 0; i < kProbeStride; ++i) {
                  nonzero[p] += b[off + i] ? 1u : 0u;
                }
                rgba[p] = uint32_t(b[off + 0]) | (uint32_t(b[off + 1]) << 8) |
                          (uint32_t(b[off + 2]) << 16) |
                          (uint32_t(b[off + 3]) << 24);
                coherence += nonzero[p] ? 1u : 0u;
              }
              fprintf(stderr,
                      "[drawable-probe] seq=%llu coh=%u c=%u/0x%08X "
                      "ul=%u/0x%08X ur=%u/0x%08X ll=%u/0x%08X lr=%u/0x%08X "
                      "tex=%ux%u\n",
                      static_cast<unsigned long long>(probe_sequence),
                      coherence, nonzero[0], rgba[0], nonzero[1], rgba[1],
                      nonzero[2], rgba[2], nonzero[3], rgba[3], nonzero[4],
                      rgba[4], probe_w, probe_h);
              fflush(stderr);
            }
            probe_buffer_to_log->release();
          }
        });
  }
  mtl4_->WaitDrawable(drawable);
#if defined(__APPLE__)
  if (REXCVAR_GET(metal_capture_diag_shim) && file_capture_active_) {
    RexMetalCaptureDiagMark("before-command-buffer-commit");
  }
#endif
  if (paint_commit_options) {
    mtl4_->Commit(cmd, paint_commit_options);
    paint_commit_options->release();
  } else {
    mtl4_->Commit(cmd);
  }
#if defined(__APPLE__)
  if (REXCVAR_GET(metal_capture_diag_shim) && file_capture_active_) {
    RexMetalCaptureDiagMark("after-command-buffer-commit");
  }
#endif
  mtl4_->SignalDrawable(drawable);
  drawable->present();
  if (file_capture_active_) {
    const uint64_t now_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    const uint64_t elapsed_us =
        file_capture_start_us_ ? (now_us - file_capture_start_us_) : 0;
    const uint64_t elapsed_ms = elapsed_us / 1000ull;
    const uint64_t capture_bytes =
        file_capture_path_.empty()
            ? 0
            : ComputePathBytes(std::filesystem::path(file_capture_path_));
    const uint64_t max_ms =
        static_cast<uint64_t>(std::max(1, REXCVAR_GET(metal_capture_to_file_max_ms)));
    bool did_stop_watchdog = false;
    if (!file_capture_stop_pending_ &&
        ((file_capture_max_bytes_ && capture_bytes >= file_capture_max_bytes_) ||
         elapsed_ms >= max_ms)) {
#if defined(__APPLE__)
      if (REXCVAR_GET(metal_capture_diag_shim)) {
        RexMetalCaptureDiagMark("before-stop-watchdog");
        RexMetalCaptureDiagStop(file_capture_path_.c_str());
        RexMetalCaptureDiagMark("after-stop-watchdog");
      } else
#endif
      {
        MTL::CaptureManager* capture_manager =
            MTL::CaptureManager::sharedCaptureManager();
        if (capture_manager && capture_manager->isCapturing()) {
          capture_manager->stopCapture();
        }
      }
      std::error_code ec;
      const bool output_exists = std::filesystem::exists(file_capture_path_, ec);
      const auto stopped_size =
          output_exists ? ComputePathBytes(std::filesystem::path(file_capture_path_))
                        : uintmax_t(0);
      AppendCaptureEvent(
          "[metal-capture] stopped(watchdog) path=%s seq=%llu bytes=%llu "
          "limit=%llu elapsed_ms=%llu max_ms=%llu exists=%d final_size=%llu ec=%d",
          file_capture_path_.c_str(),
          static_cast<unsigned long long>(paint_sequence),
          static_cast<unsigned long long>(capture_bytes),
          static_cast<unsigned long long>(file_capture_max_bytes_),
          static_cast<unsigned long long>(elapsed_ms),
          static_cast<unsigned long long>(max_ms),
          output_exists ? 1 : 0,
          static_cast<unsigned long long>(stopped_size),
          ec ? ec.value() : 0);
      file_capture_active_ = false;
      file_capture_stop_pending_ = false;
      file_capture_frames_remaining_ = 0;
      file_capture_max_bytes_ = 0;
      file_capture_start_us_ = 0;
      file_capture_path_.clear();
      did_stop_watchdog = true;
    }
    if (!did_stop_watchdog && !file_capture_stop_pending_ &&
        --file_capture_frames_remaining_ <= 0) {
#if defined(__APPLE__)
      if (REXCVAR_GET(metal_capture_diag_shim)) {
        RexMetalCaptureDiagMark("before-stop-frame-limit");
        RexMetalCaptureDiagStop(file_capture_path_.c_str());
        RexMetalCaptureDiagMark("after-stop-frame-limit");
      } else
#endif
      {
        MTL::CaptureManager* capture_manager =
            MTL::CaptureManager::sharedCaptureManager();
        if (capture_manager && capture_manager->isCapturing()) {
          capture_manager->stopCapture();
        }
      }
      std::error_code ec;
      const bool output_exists = std::filesystem::exists(file_capture_path_, ec);
      const auto stopped_size =
          output_exists ? ComputePathBytes(std::filesystem::path(file_capture_path_))
                        : uintmax_t(0);
      AppendCaptureEvent(
          "[metal-capture] stopped(frame-limit) path=%s seq=%llu exists=%d "
          "final_size=%llu ec=%d",
          file_capture_path_.c_str(),
          static_cast<unsigned long long>(paint_sequence),
          output_exists ? 1 : 0,
          static_cast<unsigned long long>(stopped_size), ec ? ec.value() : 0);
      file_capture_active_ = false;
      file_capture_stop_pending_ = false;
      file_capture_frames_remaining_ = 0;
      file_capture_max_bytes_ = 0;
      file_capture_start_us_ = 0;
      file_capture_path_.clear();
    }
  }
  if (presenter_capture_scope_active_) {
    presenter_capture_scope_->endScope();
    presenter_capture_scope_active_ = false;
  }
  if (drawable_probe_buffer) {
    drawable_probe_buffer->release();
    drawable_probe_buffer = nullptr;
    ++drawable_probe_log_count;
  }
  release_paint_tables();

  pool->drain();
  return PaintResult::kPresented;
}

MTL::RenderPipelineState* MetalPresenter::GetOrCreateBlitPipeline(MTL::PixelFormat fmt) {
  if (blit_pipe_ && blit_pipe_fmt_ == fmt) return blit_pipe_;
  if (!blit_lib_) return nullptr;
  auto* vf = blit_lib_->newFunction(NS::String::string("blit_vs", NS::UTF8StringEncoding));
  auto* ff = blit_lib_->newFunction(NS::String::string("blit_fs", NS::UTF8StringEncoding));
  auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vf);
  desc->setFragmentFunction(ff);
  desc->colorAttachments()->object(0)->setPixelFormat(fmt);
  NS::Error* err = nullptr;
  blit_pipe_ = device_->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &err);
  if (blit_pipe_) {
    blit_pipe_fmt_ = fmt;
    REXLOG_INFO("MetalPresenter: Blit pipeline created fmt={}", (int)fmt);
  } else {
    REXLOG_ERROR("MetalPresenter: Blit pipeline failed: {}",
                 err ? err->localizedDescription()->utf8String() : "null");
  }
  desc->release();
  if (vf) vf->release();
  if (ff) ff->release();
  return blit_pipe_;
}

MTL::SamplerState* MetalPresenter::GetNearestSampler() {
  if (nearest_sampler_) return nearest_sampler_;
  auto* desc = MTL::SamplerDescriptor::alloc()->init();
  desc->setMinFilter(MTL::SamplerMinMagFilterNearest);
  desc->setMagFilter(MTL::SamplerMinMagFilterNearest);
  desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
  desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
  nearest_sampler_ = device_->newSamplerState(desc);
  desc->release();
  return nearest_sampler_;
}

Presenter::SurfacePaintConnectResult
MetalPresenter::ConnectOrReconnectPaintingToSurfaceFromUIThread(
    Surface& new_surface, uint32_t new_surface_width,
    uint32_t new_surface_height, bool was_paintable,
    bool& is_vsync_implicit_out) {
  is_vsync_implicit_out = true;

  Surface::TypeIndex surface_type = new_surface.GetType();
  if (surface_type != Surface::kTypeIndex_CAMetalLayer) {
    REXLOG_ERROR("MetalPresenter: Unsupported surface type {}", (int)surface_type);
    return SurfacePaintConnectResult::kFailureSurfaceUnusable;
  }

  auto& metal_surface = static_cast<const CAMetalLayerSurface&>(new_surface);
  CAMetalLayer* raw_layer = metal_surface.layer();
  if (!raw_layer) {
    REXLOG_ERROR("MetalPresenter: Null CAMetalLayer");
    return SurfacePaintConnectResult::kFailureSurfaceUnusable;
  }

  CA::MetalLayer* layer = reinterpret_cast<CA::MetalLayer*>(raw_layer);
  layer->setDevice(reinterpret_cast<MTL::Device*>(device_));
  layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
  layer->setDrawableSize(CGSize{(double)new_surface_width, (double)new_surface_height});

  metal_layer_ = raw_layer;

  REXLOG_INFO("MetalPresenter: Connected to CAMetalLayer {}x{}", new_surface_width, new_surface_height);
  return SurfacePaintConnectResult::kSuccess;
}

void MetalPresenter::DisconnectPaintingFromSurfaceFromUIThreadImpl() {
  metal_layer_ = nullptr;
}

bool MetalPresenter::RefreshGuestOutputImpl(
    uint32_t mailbox_index, uint32_t frontbuffer_width,
    uint32_t frontbuffer_height,
    std::function<bool(Presenter::GuestOutputRefreshContext& context)> refresher,
    bool& is_8bpc_out_ref) {
  if (mailbox_index >= guest_output_textures_.size()) {
    is_8bpc_out_ref = false;
    return false;
  }

  MTL::Texture* guest_output_texture = guest_output_textures_[mailbox_index];

  if (!guest_output_texture ||
      guest_output_texture->width() != frontbuffer_width ||
      guest_output_texture->height() != frontbuffer_height) {
    if (guest_output_texture) {
      guest_output_texture->release();
    }

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    desc->setWidth(frontbuffer_width);
    desc->setHeight(frontbuffer_height);
    desc->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsageShaderRead |
                   MTL::TextureUsageRenderTarget);
    desc->setStorageMode(MTL::StorageModePrivate);

    guest_output_texture = device_->newTexture(desc);
    desc->release();

    if (!guest_output_texture) {
      REXLOG_ERROR("Metal RefreshGuestOutput: Failed to create texture {}x{}",
                   frontbuffer_width, frontbuffer_height);
      is_8bpc_out_ref = false;
      return false;
    }

    guest_output_textures_[mailbox_index] = guest_output_texture;
    REXLOG_INFO("Metal RefreshGuestOutput: Created texture {}x{} for mailbox {}",
                frontbuffer_width, frontbuffer_height, mailbox_index);
  }

  MetalGuestOutputRefreshContext context(is_8bpc_out_ref, guest_output_texture);
  bool success = refresher(context);

  if (!success) {
    REXLOG_WARN("Metal RefreshGuestOutput: Refresher callback failed");
    return false;
  }

  last_guest_output_mailbox_index_.store(mailbox_index, std::memory_order_relaxed);
  guest_output_submissions_[mailbox_index] = context.submission_id();
  return true;
}

bool MetalPresenter::UpdateGammaRamp(const void* table_data, size_t table_bytes,
                                     const void* pwl_data, size_t pwl_bytes) {
  if (!table_data || !pwl_data || !table_bytes || !pwl_bytes || !device_) {
    gamma_ramp_table_valid_ = false;
    gamma_ramp_pwl_valid_ = false;
    return false;
  }

  size_t total_bytes = table_bytes + pwl_bytes;
  if (!gamma_ramp_buffer_ || gamma_ramp_buffer_size_ < total_bytes) {
    if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
    if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
    if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }

    gamma_ramp_buffer_ = device_->newBuffer(total_bytes, MTL::ResourceStorageModeShared);
    if (!gamma_ramp_buffer_) {
      gamma_ramp_buffer_size_ = 0;
      gamma_ramp_table_valid_ = false;
      gamma_ramp_pwl_valid_ = false;
      return false;
    }
    gamma_ramp_buffer_size_ = static_cast<uint32_t>(total_bytes);
  }

  void* contents = gamma_ramp_buffer_->contents();
  std::memcpy(contents, table_data, table_bytes);
  std::memcpy(reinterpret_cast<uint8_t*>(contents) + table_bytes, pwl_data, pwl_bytes);

  if (!gamma_ramp_table_texture_) {
    MTL::TextureDescriptor* table_desc = MTL::TextureDescriptor::textureBufferDescriptor(
        MTL::PixelFormatRGB10A2Unorm, 256, MTL::ResourceStorageModeShared,
        MTL::TextureUsageShaderRead);
    gamma_ramp_table_texture_ = gamma_ramp_buffer_->newTexture(
        table_desc, 0, 256 * sizeof(uint32_t));
    table_desc->release();
  }
  if (!gamma_ramp_pwl_texture_) {
    MTL::TextureDescriptor* pwl_desc = MTL::TextureDescriptor::textureBufferDescriptor(
        MTL::PixelFormatRG16Uint, 384, MTL::ResourceStorageModeShared,
        MTL::TextureUsageShaderRead);
    gamma_ramp_pwl_texture_ = gamma_ramp_buffer_->newTexture(
        pwl_desc, table_bytes, 384 * sizeof(uint32_t));
    pwl_desc->release();
  }

  gamma_ramp_table_valid_ = gamma_ramp_table_texture_ != nullptr;
  gamma_ramp_pwl_valid_ = gamma_ramp_pwl_texture_ != nullptr;
  return gamma_ramp_table_valid_ && gamma_ramp_pwl_valid_;
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
