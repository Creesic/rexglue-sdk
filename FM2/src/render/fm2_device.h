#pragma once

#include "render/guest_device.h"

namespace fm2::render {

void SetActiveGuestDevice(GuestDevice* device);
GuestDevice* GetActiveGuestDevice();

}  // namespace fm2::render
