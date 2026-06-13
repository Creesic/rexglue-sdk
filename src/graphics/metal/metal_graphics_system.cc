/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "rex/graphics/metal/metal_graphics_system.h"

#include "rex/graphics/metal/metal_command_processor.h"
#include "rex/ui/metal/metal_provider.h"
#include "rex/system/kernel_state.h"

namespace rex::graphics::metal {

MetalGraphicsSystem::MetalGraphicsSystem() {}

MetalGraphicsSystem::~MetalGraphicsSystem() {}

bool MetalGraphicsSystem::IsAvailable() {
  return rex::ui::metal::MetalProvider::IsMetalAPIAvailable();
}

std::string MetalGraphicsSystem::name() const { return "Metal"; }

void MetalGraphicsSystem::CreateProvider(bool /*with_presentation*/) {
  provider_ = rex::ui::metal::MetalProvider::Create();
}

std::unique_ptr<CommandProcessor>
MetalGraphicsSystem::CreateCommandProcessor() {
  return std::unique_ptr<CommandProcessor>(
      new MetalCommandProcessor(this, kernel_state_));
}

}  // namespace rex::graphics::metal
