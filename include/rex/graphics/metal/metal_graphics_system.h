/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#pragma once

#include <memory>

#include "rex/graphics/command_processor.h"
#include "rex/graphics/graphics_system.h"

namespace rex::graphics::metal {

class MetalGraphicsSystem : public GraphicsSystem {
 public:
  MetalGraphicsSystem();
  ~MetalGraphicsSystem() override;

  static bool IsAvailable();

  std::string name() const override;

 protected:
  void CreateProvider(bool with_presentation) override;
  std::unique_ptr<CommandProcessor> CreateCommandProcessor() override;
};

}  // namespace rex::graphics::metal
