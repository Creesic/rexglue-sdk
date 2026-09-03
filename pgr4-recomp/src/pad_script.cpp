// pad_script.cpp
//
// Scripted controller presses for unattended testing. The game reads pad state
// through its XInputGetState wrapper (sub_82673A90 -> XamInputGetState); this
// hook lets the original run, then ORs scripted button presses into the
// returned state, so a capture script can drive the menus without a physical
// pad, a virtual-pad driver, or the keyboard mapping.
//
//   --pgr4_pad_script=12:START,20:START,30:A+DOWN
//
// Each entry is "seconds:BUTTON[+BUTTON]" (seconds since the first pad poll,
// which happens right after boot); each press is held for 200 ms.

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/input/input.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(pgr4_pad_script, "", "Input",
                      "Scripted controller presses for unattended testing: "
                      "'sec:BUTTON[+BUTTON],...' (A,B,X,Y,START,BACK,LB,RB,UP,DOWN,LEFT,RIGHT), "
                      "each held 200 ms, seconds counted from the first pad poll");

namespace {

using rex::input::X_INPUT_STATE;

struct ScriptedPress {
  double atSeconds;
  uint16_t buttons;
};

uint16_t ButtonFromName(std::string_view name) {
  using namespace rex::input;
  if (name == "A") return X_INPUT_GAMEPAD_A;
  if (name == "B") return X_INPUT_GAMEPAD_B;
  if (name == "X") return X_INPUT_GAMEPAD_X;
  if (name == "Y") return X_INPUT_GAMEPAD_Y;
  if (name == "START") return X_INPUT_GAMEPAD_START;
  if (name == "BACK") return X_INPUT_GAMEPAD_BACK;
  if (name == "LB") return X_INPUT_GAMEPAD_LEFT_SHOULDER;
  if (name == "RB") return X_INPUT_GAMEPAD_RIGHT_SHOULDER;
  if (name == "UP") return X_INPUT_GAMEPAD_DPAD_UP;
  if (name == "DOWN") return X_INPUT_GAMEPAD_DPAD_DOWN;
  if (name == "LEFT") return X_INPUT_GAMEPAD_DPAD_LEFT;
  if (name == "RIGHT") return X_INPUT_GAMEPAD_DPAD_RIGHT;
  REXLOG_WARN("pgr4_pad_script: unknown button '{}'", name);
  return 0;
}

std::vector<ScriptedPress> ParseScript(std::string_view script) {
  std::vector<ScriptedPress> presses;
  size_t pos = 0;
  while (pos < script.size()) {
    size_t end = script.find(',', pos);
    if (end == std::string_view::npos) end = script.size();
    const std::string_view entry = script.substr(pos, end - pos);
    pos = end + 1;
    const size_t colon = entry.find(':');
    if (colon == std::string_view::npos) continue;
    ScriptedPress press{std::stod(std::string(entry.substr(0, colon))), 0};
    std::string_view names = entry.substr(colon + 1);
    while (!names.empty()) {
      size_t plus = names.find('+');
      if (plus == std::string_view::npos) plus = names.size();
      press.buttons |= ButtonFromName(names.substr(0, plus));
      names = plus < names.size() ? names.substr(plus + 1) : std::string_view{};
    }
    presses.push_back(press);
  }
  return presses;
}

// Buttons the script holds down right now (0 when idle or no script).
uint16_t ScriptedButtons() {
  static const std::vector<ScriptedPress> presses = ParseScript(REXCVAR_GET(pgr4_pad_script));
  if (presses.empty()) return 0;
  static const auto start = std::chrono::steady_clock::now();
  const double now = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  uint16_t buttons = 0;
  for (const ScriptedPress& press : presses) {
    if (now >= press.atSeconds && now < press.atSeconds + 0.2) buttons |= press.buttons;
  }
  return buttons;
}

REX_IMPORT(__imp__XInputGetState, g_origXInputGetState, uint32_t(uint32_t, X_INPUT_STATE*));

uint32_t XInputGetStateHook(uint32_t userIndex, X_INPUT_STATE* state) {
  const uint32_t result = g_origXInputGetState(userIndex, state);
  if (userIndex != 0 || state == nullptr) return result;
  const uint16_t buttons = ScriptedButtons();
  if (buttons == 0) return result;
  if (result != 0) *state = {};  // no pad connected: present the scripted one
  state->gamepad.buttons = uint16_t(state->gamepad.buttons.get() | buttons);
  state->packet_number = state->packet_number.get() + 1;
  return 0;
}

REX_HOOK(XInputGetState, XInputGetStateHook);  // @ 0x82673A90

}  // namespace
