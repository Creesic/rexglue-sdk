#if defined(__APPLE__)

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <objc/runtime.h>

#include <errno.h>
#include <filesystem>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <rex/ui/metal/capture_diag.h>

namespace {

const char* kDefaultTracePath =
    "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-present-capture.gputrace";
const char* kDiagLogPath =
    "/Users/tera/Documents/GitHub/rexglue-sdk/MTL/rex-metal-capture-diag.log";

const char* EnvOrUnset(const char* name) {
  const char* value = getenv(name);
  return value ? value : "(unset)";
}

const char* NSStr(NSString* s) { return s ? s.UTF8String : "(nil)"; }

__attribute__((format(printf, 1, 2))) void RexCapLog(const char* fmt, ...) {
  struct timeval tv {};
  gettimeofday(&tv, nullptr);

  time_t sec = tv.tv_sec;
  struct tm tmv {};
  localtime_r(&sec, &tmv);

  char timebuf[64] = {};
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tmv);

  va_list args;
  va_start(args, fmt);

  va_list file_args;
  va_copy(file_args, args);

  fprintf(stderr, "[rex-cap] %s.%03d pid=%d ", timebuf, int(tv.tv_usec / 1000),
          getpid());
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  fflush(stderr);

  FILE* f = fopen(kDiagLogPath, "a");
  if (f) {
    fprintf(f, "[rex-cap] %s.%03d pid=%d ", timebuf, int(tv.tv_usec / 1000),
            getpid());
    vfprintf(f, fmt, file_args);
    fprintf(f, "\n");
    fclose(f);
  }

  va_end(file_args);
  va_end(args);
}

void LogNSError(NSError* error) {
  if (!error) {
    RexCapLog("NSError: nil");
    return;
  }

  RexCapLog("NSError domain=%s code=%ld", NSStr(error.domain), (long)error.code);
  RexCapLog("NSError localizedDescription=%s", NSStr(error.localizedDescription));
  RexCapLog("NSError localizedFailureReason=%s",
            NSStr(error.localizedFailureReason));
  RexCapLog("NSError localizedRecoverySuggestion=%s",
            NSStr(error.localizedRecoverySuggestion));

  NSDictionary* info = error.userInfo;
  for (id key in info) {
    id value = info[key];
    RexCapLog("NSError userInfo[%s]=%s", NSStr([key description]),
              NSStr([value description]));
  }
}

const char* ResolvePath(const char* output_path) {
  return (output_path && output_path[0]) ? output_path : kDefaultTracePath;
}

uint64_t ComputeRecursiveBytes(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec) || ec) {
    return 0;
  }
  if (std::filesystem::is_regular_file(path, ec) && !ec) {
    return static_cast<uint64_t>(std::filesystem::file_size(path, ec));
  }
  uint64_t total = 0;
  for (std::filesystem::recursive_directory_iterator it(path, ec), end;
       !ec && it != end; it.increment(ec)) {
    if (it->is_regular_file(ec) && !ec) {
      total += static_cast<uint64_t>(it->file_size(ec));
    }
  }
  return total;
}

}  // namespace

extern "C" void RexMetalCaptureDiagFileStatus(const char* output_path) {
  const char* path = ResolvePath(output_path);
  struct stat st {};
  if (stat(path, &st) == 0) {
    const bool is_dir = S_ISDIR(st.st_mode);
    const bool is_reg = S_ISREG(st.st_mode);
    const char* type = is_dir ? "directory/package"
                              : (is_reg ? "regular-file" : "other");
    const uint64_t recursive_bytes =
        is_dir ? ComputeRecursiveBytes(std::filesystem::path(path)) : 0;
    RexCapLog(
        "file status: exists path=%s type=%s stat_size=%lld recursive_size=%llu mode=%o mtime=%lld",
        path, type, (long long)st.st_size,
        static_cast<unsigned long long>(recursive_bytes),
        (unsigned)st.st_mode, (long long)st.st_mtime);
  } else {
    RexCapLog("file status: missing path=%s errno=%d strerror=%s", path, errno,
              strerror(errno));
  }
}

extern "C" void RexMetalCaptureDiagMark(const char* label) {
  @autoreleasepool {
    MTLCaptureManager* manager = [MTLCaptureManager sharedCaptureManager];
    RexCapLog("MARK label=%s isCapturing=%d", label ? label : "(null)",
              manager.isCapturing ? 1 : 0);
  }
}

extern "C" bool RexMetalCaptureDiagStart(void* capture_object_ptr,
                                         const char* output_path) {
  @autoreleasepool {
    const char* path = ResolvePath(output_path);

    RexCapLog("START requested captureObjectPtr=%p outputPath=%s",
              capture_object_ptr, path);
    RexCapLog("env MTL_CAPTURE_ENABLED=%s", EnvOrUnset("MTL_CAPTURE_ENABLED"));
    RexCapLog("env REX_METAL_CAPTURE_TO_FILE=%s",
              EnvOrUnset("REX_METAL_CAPTURE_TO_FILE"));
    RexCapLog("env REX_METAL_CAPTURE_TO_FILE_PATH=%s",
              EnvOrUnset("REX_METAL_CAPTURE_TO_FILE_PATH"));
    RexCapLog("env REX_METAL_CAPTURE_TO_FILE_FRAMES=%s",
              EnvOrUnset("REX_METAL_CAPTURE_TO_FILE_FRAMES"));

    NSBundle* bundle = [NSBundle mainBundle];
    id metal_capture_enabled =
        [bundle objectForInfoDictionaryKey:@"MetalCaptureEnabled"];
    RexCapLog("bundle path=%s", NSStr(bundle.bundlePath));
    RexCapLog("Info.plist MetalCaptureEnabled=%s",
              NSStr([metal_capture_enabled description]));

    if (!capture_object_ptr) {
      RexCapLog("START failed: captureObjectPtr is null");
      return false;
    }

    id capture_object = (__bridge id)capture_object_ptr;
    RexCapLog("captureObject class=%s description=%s",
              object_getClassName(capture_object),
              NSStr([capture_object description]));

    BOOL is_device = [capture_object conformsToProtocol:@protocol(MTLDevice)];
    BOOL is_queue =
        [capture_object conformsToProtocol:@protocol(MTLCommandQueue)];
    BOOL is_scope =
        [capture_object conformsToProtocol:@protocol(MTLCaptureScope)];

    RexCapLog(
        "captureObject protocols: MTLDevice=%d MTLCommandQueue=%d MTLCaptureScope=%d",
        is_device ? 1 : 0, is_queue ? 1 : 0, is_scope ? 1 : 0);

    if (is_device) {
      id<MTLDevice> device = (id<MTLDevice>)capture_object;
      RexCapLog("captureObject MTLDevice name=%s registryID=%llu",
                NSStr(device.name), (unsigned long long)device.registryID);
    }
    if (is_queue) {
      id<MTLCommandQueue> queue = (id<MTLCommandQueue>)capture_object;
      RexCapLog("captureObject MTLCommandQueue deviceName=%s",
                NSStr(queue.device.name));
    }

    MTLCaptureManager* manager = [MTLCaptureManager sharedCaptureManager];
    BOOL supports_gpu_trace =
        [manager supportsDestination:MTLCaptureDestinationGPUTraceDocument];
    BOOL supports_tools =
        [manager supportsDestination:MTLCaptureDestinationDeveloperTools];
    RexCapLog(
        "manager=%p isCapturing(before)=%d supportsGPUTraceDocument=%d supportsDeveloperTools=%d",
        manager, manager.isCapturing ? 1 : 0, supports_gpu_trace ? 1 : 0,
        supports_tools ? 1 : 0);

    if (!supports_gpu_trace) {
      RexCapLog("START failed: MTLCaptureDestinationGPUTraceDocument unsupported");
      return false;
    }

    if (manager.isCapturing) {
      RexCapLog("START skipped: manager is already capturing");
      return true;
    }

    NSString* ns_path = [[NSString alloc] initWithUTF8String:path];
    if (!ns_path) {
      RexCapLog("START failed: output path is not valid UTF-8");
      return false;
    }

    NSString* dir = [ns_path stringByDeletingLastPathComponent];
    NSError* dir_error = nil;
    BOOL made_dir = [[NSFileManager defaultManager]
        createDirectoryAtPath:dir
   withIntermediateDirectories:YES
                    attributes:nil
                         error:&dir_error];
    RexCapLog("create output dir=%s ok=%d", NSStr(dir), made_dir ? 1 : 0);
    if (!made_dir) {
      LogNSError(dir_error);
      return false;
    }

    if ([[NSFileManager defaultManager] fileExistsAtPath:ns_path]) {
      NSError* remove_error = nil;
      BOOL removed = [[NSFileManager defaultManager] removeItemAtPath:ns_path
                                                                 error:&remove_error];
      RexCapLog("removed old trace path=%s ok=%d", path, removed ? 1 : 0);
      if (!removed) {
        LogNSError(remove_error);
        return false;
      }
    }

    NSURL* url = [NSURL fileURLWithPath:ns_path isDirectory:NO];
    MTLCaptureDescriptor* desc = [[MTLCaptureDescriptor alloc] init];
    desc.captureObject = capture_object;
    desc.destination = MTLCaptureDestinationGPUTraceDocument;
    desc.outputURL = url;
    RexCapLog("descriptor captureObject=%p destination=%ld outputURL=%s",
              desc.captureObject, (long)desc.destination,
              NSStr(desc.outputURL.path));

    NSError* start_error = nil;
    BOOL ok = [manager startCaptureWithDescriptor:desc error:&start_error];
    RexCapLog("startCapture returned ok=%d isCapturing(after)=%d", ok ? 1 : 0,
              manager.isCapturing ? 1 : 0);

    if (!ok) {
      LogNSError(start_error);
      RexMetalCaptureDiagFileStatus(path);
      return false;
    }

    RexCapLog("START success");
    RexMetalCaptureDiagFileStatus(path);
    return true;
  }
}

extern "C" void RexMetalCaptureDiagStop(const char* output_path) {
  @autoreleasepool {
    const char* path = ResolvePath(output_path);
    MTLCaptureManager* manager = [MTLCaptureManager sharedCaptureManager];
    struct timeval t0 {};
    gettimeofday(&t0, nullptr);

    RexCapLog("STOP requested path=%s isCapturing(before)=%d", path,
              manager.isCapturing ? 1 : 0);
    if (manager.isCapturing) {
      RexCapLog("STOP about-to-call stopCapture");
      [manager stopCapture];
      struct timeval t1 {};
      gettimeofday(&t1, nullptr);
      const long long elapsed_ms =
          (long long)(t1.tv_sec - t0.tv_sec) * 1000LL +
          (long long)(t1.tv_usec - t0.tv_usec) / 1000LL;
      RexCapLog("STOP returned-from stopCapture elapsed_ms=%lld", elapsed_ms);
    } else {
      RexCapLog("STOP skipped: manager was not capturing");
    }
    RexCapLog("STOP complete isCapturing(after)=%d",
              manager.isCapturing ? 1 : 0);

    for (int i = 0; i < 5; ++i) {
      RexCapLog("post-stop file check %d", i);
      RexMetalCaptureDiagFileStatus(path);
      usleep(100 * 1000);
    }
  }
}

#endif
