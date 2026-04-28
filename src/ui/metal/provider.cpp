#include <rex/ui/metal/provider.h>
#include <rex/ui/metal/presenter.h>
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
  if (command_queue_) command_queue_->release();
  if (device_) device_->release();
}

bool MetalProvider::Initialize() {
  device_ = MTL::CreateSystemDefaultDevice();
  if (!device_) {
    REXLOG_ERROR("Failed to create Metal device");
    return false;
  }
  REXLOG_INFO("Metal device: {}", device_->name()->utf8String());
  command_queue_ = device_->newCommandQueue();
  if (!command_queue_) {
    REXLOG_ERROR("Failed to create Metal Command Queue");
    return false;
  }
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
