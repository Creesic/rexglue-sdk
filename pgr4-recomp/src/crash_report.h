// pgr4_recompiled - crash reporter (see crash_report.cpp).
#pragma once

#include <cstdint>

// Logs a symbolized backtrace and writes a minidump when an unhandled fault
// reaches the SDK's exception handler. Install after runtime setup.
void InstallCrashReporter();

// Writes a minidump of the live process (all thread stacks) when the GPU
// watchdog sees the guest stop kicking the ring; once per process.
void WriteHangDump(uint32_t stalledSeconds);
