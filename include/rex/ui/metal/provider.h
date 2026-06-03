#pragma once

#include <memory>

#include <rex/ui/graphics_provider.h>

namespace MTL {
class Device;
}  // namespace MTL

namespace rex::graphics::metal {
class Metal4Context;
}  // namespace rex::graphics::metal

namespace rex::ui::metal {

class MetalProvider : public GraphicsProvider {
 public:
  ~MetalProvider() override;

  static bool IsMetalAPIAvailable();
  static std::unique_ptr<MetalProvider> Create();

  std::unique_ptr<Presenter> CreatePresenter(
      Presenter::HostGpuLossCallback host_gpu_loss_callback =
          Presenter::FatalErrorHostGpuLossCallback) override;

  std::unique_ptr<ImmediateDrawer> CreateImmediateDrawer() override;

  MTL::Device* GetDevice() const { return device_; }
  graphics::metal::Metal4Context* GetMetal4Context() const {
    return metal4_context_.get();
  }

 private:
  MetalProvider();
  bool Initialize();

  MTL::Device* device_ = nullptr;
  std::unique_ptr<graphics::metal::Metal4Context> metal4_context_;
};

}  // namespace rex::ui::metal
