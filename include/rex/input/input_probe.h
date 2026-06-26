#pragma once

#include <cstdint>

namespace rex::input {

struct InputStateProbeSample {
  uint32_t user_index = 0;
  uint32_t result = 0;
  uint16_t buttons = 0;
  uint8_t left_trigger = 0;
  uint8_t right_trigger = 0;
  uint32_t packet_number = 0;
};

struct InputKeystrokeProbeSample {
  uint32_t user_index = 0;
  uint32_t result = 0;
  uint16_t virtual_key = 0;
  uint16_t unicode = 0;
  uint32_t flags = 0;
};

using InputGetStateProbeFn = void (*)(const InputStateProbeSample& sample);
using InputGetKeystrokeProbeFn = void (*)(const InputKeystrokeProbeSample& sample);

void SetInputGetStateProbe(InputGetStateProbeFn fn);
void SetInputGetKeystrokeProbe(InputGetKeystrokeProbeFn fn);

void InvokeInputGetStateProbe(const InputStateProbeSample& sample);
void InvokeInputGetKeystrokeProbe(const InputKeystrokeProbeSample& sample);

}  // namespace rex::input
