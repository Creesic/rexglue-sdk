#pragma once

#include <cstdint>

namespace rex::memory {
class Memory;
}

namespace fh1::workqueue {

/// True when `queue` looks like a committed FH1 work-queue object (ring readable).
bool IsPlausibleWorkQueue(rex::memory::Memory* memory, uint32_t queue);

/// Return `hint` if plausible, else last known good queue from BC88/BEC8, else 0.
uint32_t RecoverWorkQueue(rex::memory::Memory* memory, uint32_t hint);

void StashWorkQueue(uint32_t queue);

}  // namespace fh1::workqueue
