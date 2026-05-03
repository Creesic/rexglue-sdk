#include <rex/ui/metal/provider.h>
#include <rex/ui/metal/presenter.h>
#include <rex/graphics/metal/metal4_context.h>
#include <Metal/Metal.hpp>
#include <rex/logging/macros.h>

namespace rex {
namespace ui {
namespace metal {

bool MetalProvider::IsMetalAPIAvailable() {
  MTL::Device* device = MTL::CreateSystemDefaultDevice();
  bool available = (device != nullptr);
  if (device) device->release();
  return available;
}

std::unique_ptr<MetalProvider> MetalProvider::Create() {
  auto provider = std::unique_ptr<MetalProvider>(new MetalProvider());
  if (!provider->Initialize()) return nullptr;
  return provider;
}

MetalProvider::MetalProvider() = default;

MetalProvider::~MetalProvider() {
  metal4_context_.reset();
  if (command_queue_) { command_queue_->release(); command_queue_ = nullptr; }
  if (device_) { device_->release(); device_ = nullptr; }
}

bool MetalProvider::Initialize() {
  device_ = MTL::CreateSystemDefaultDevice();
  if (!device_) return false;

  command_queue_ = device_->newCommandQueue();
  if (!command_queue_) return false;

  metal4_context_ = std::make_unique<graphics::metal::Metal4Context>();
  if (!metal4_context_->Initialize(device_)) return false;

  return true;
}

std::unique_ptr<Presenter> MetalProvider::CreatePresenter(
    Presenter::HostGpuLossCallback host_gpu_loss_callback) {
  auto presenter =
      std::make_unique<MetalPresenter>(this, host_gpu_loss_callback);
  if (!presenter->Initialize()) return nullptr;
  return presenter;
}

std::unique_ptr<ImmediateDrawer> MetalProvider::CreateImmediateDrawer() {
  return nullptr;
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
