// FH1 loader epoch — block worker dispatch until track-loader setup completes.
//
// Loader paths call fh1_load_gate_enter/leave around critical setup (nested
// enter/leave uses thread-local depth; only the outermost pair moves the global
// counter). Worker hooks call fh1_load_gate_dispatch_wait before running.

#pragma once

#include <cstdint>

extern "C" {

void fh1_load_gate_enter(void);
void fh1_load_gate_leave(void);
void fh1_load_gate_dispatch_wait(uint32_t target);

/// Spin until FMOD/audio consumer pointers at this+24 / sound+68 look valid.
void fh1_audio_consumer_acquire(void* guest_base, uint32_t this_ptr);

}  // extern "C"
