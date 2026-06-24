#pragma once

#include <rex/image_info.h>

/// Register guest-PC fiber config. The strong `DOAX_FiberContextSwitch` override
/// is defined in doax_hooks.cpp and wins over the generated weak alias at link time.
void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info);
