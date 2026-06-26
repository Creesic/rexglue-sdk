#include <rex/input/input_probe.h>

namespace rex::input {
namespace {

InputGetStateProbeFn g_get_state_probe = nullptr;
InputGetKeystrokeProbeFn g_get_keystroke_probe = nullptr;

}  // namespace

void SetInputGetStateProbe(InputGetStateProbeFn fn) {
  g_get_state_probe = fn;
}

void SetInputGetKeystrokeProbe(InputGetKeystrokeProbeFn fn) {
  g_get_keystroke_probe = fn;
}

void InvokeInputGetStateProbe(const InputStateProbeSample& sample) {
  if (g_get_state_probe != nullptr) {
    g_get_state_probe(sample);
  }
}

void InvokeInputGetKeystrokeProbe(const InputKeystrokeProbeSample& sample) {
  if (g_get_keystroke_probe != nullptr) {
    g_get_keystroke_probe(sample);
  }
}

}  // namespace rex::input
