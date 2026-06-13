/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_METAL_METAL_API_H_
#define XENIA_UI_METAL_METAL_API_H_

// metal-cpp C++ wrappers for Metal and Foundation.
#include "thirdparty/metal-cpp/Metal/MTLDevice.hpp"
#include "thirdparty/metal-cpp/Metal/Metal.hpp"

#ifdef METAL_SHADER_CONVERTER_AVAILABLE
#ifndef IR_RUNTIME_METALCPP
#define IR_RUNTIME_METALCPP
#endif
#include "thirdparty/metal-shader-converter/include/metal_irconverter/metal_irconverter.h"
#endif  // METAL_SHADER_CONVERTER_AVAILABLE

#endif
