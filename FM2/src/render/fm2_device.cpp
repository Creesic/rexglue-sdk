#include "render/fm2_device.h"

namespace fm2::render {
namespace {

GuestDevice* g_active_guest_device = nullptr;

}  // namespace

void SetActiveGuestDevice(GuestDevice* device) { g_active_guest_device = device; }

GuestDevice* GetActiveGuestDevice() { return g_active_guest_device; }

}  // namespace fm2::render
