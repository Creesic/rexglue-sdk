#pragma once

#include <cstdint>

/// Last plausible work-queue pointer seen on this thread (0 if none).
uint32_t Fh1GetActiveWorkQueue();
