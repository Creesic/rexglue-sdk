/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <rex/system/xtypes.h>
#include <rex/system/xvideo.h>

namespace rex::runtime {
class ExportResolver;
}

namespace rex::system {
class KernelState;
}

namespace rex::kernel::xboxkrnl {

void VdQueryVideoMode(system::X_VIDEO_MODE* video_mode);

// Fires the guest's graphics interrupt callback (registered via
// VdSetGraphicsInterruptCallback), if one has been registered. For use by a
// detached-mode renderer (no IGraphicsSystem/gpu_plugin) that needs to
// simulate vblank interrupts itself, since GraphicsSystem's own vsync worker
// thread never exists in that mode. Must be called from a registered XThread.
// Returns false if no callback has been registered yet.
bool DispatchGraphicsInterruptCallback(uint32_t cpu = 0xFFFFFFFF);

// Register video variable exports (VdGlobalDevice, VdHSIOCalibrationLock, etc.)
// Must be called during kernel initialization before XEX modules are loaded.
void RegisterVideoExports(rex::runtime::ExportResolver* export_resolver,
                          system::KernelState* kernel_state);

}  // namespace rex::kernel::xboxkrnl
