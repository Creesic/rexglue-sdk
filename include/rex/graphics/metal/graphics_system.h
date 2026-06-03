#pragma once

#include <rex/graphics/graphics_system.h>

namespace rex::graphics::metal {

class MetalGraphicsSystem : public GraphicsSystem {
 public:
  MetalGraphicsSystem();
  ~MetalGraphicsSystem() override;

  std::string name() const override;

 protected:
  void CreateProvider(bool with_presentation) override;
  std::unique_ptr<CommandProcessor> CreateCommandProcessor() override;
};

}  // namespace rex::graphics::metal
