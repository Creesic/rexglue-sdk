#pragma once

#include <rex/platform.h>

#if REX_PLATFORM_MAC

extern "C" void RexMetalCaptureDiagFileStatus(const char* output_path);
extern "C" void RexMetalCaptureDiagMark(const char* label);
extern "C" bool RexMetalCaptureDiagStart(void* capture_object_ptr, const char* output_path);
extern "C" void RexMetalCaptureDiagStop(const char* output_path);

#endif  // REX_PLATFORM_MAC
