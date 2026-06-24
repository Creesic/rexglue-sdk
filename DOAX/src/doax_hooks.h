#pragma once

#include <rex/image_info.h>

/// Guest-PC fiber swap + yield GPR preserve. See DOAX/archive/fiber-hooks-2026-06-24/.
void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info);
