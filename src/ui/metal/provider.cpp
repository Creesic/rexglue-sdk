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
  fprintf(stderr, "[metal] Provider::Initialize: creating device\n"); fflush(stderr);
  device_ = MTL::CreateSystemDefaultDevice();
  if (!device_) {
    fprintf(stderr, "[metal] Provider::Initialize: failed to create Metal device\n"); fflush(stderr);
    return false;
  }
  fprintf(stderr, "[metal] Provider::Initialize: device=%s\n", device_->name()->utf8String()); fflush(stderr);
  command_queue_ = device_->newCommandQueue();
  if (!command_queue_) {
    fprintf(stderr, "[metal] Provider::Initialize: failed to create command queue\n"); fflush(stderr);
    return false;
  }
  fprintf(stderr, "[metal] Provider::Initialize: complete\n"); fflush(stderr);
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
