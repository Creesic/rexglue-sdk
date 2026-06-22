#include <rex/input/automation/automation_input_driver.h>

#include <rex/logging.h>
#include <rex/platform/env.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>

namespace rex::input::automation {
namespace {

constexpr uint32_t kUserIndex = 0;

std::string_view Trim(std::string_view text) {
  auto is_space = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };

  while (!text.empty() && is_space(static_cast<unsigned char>(text.front()))) {
    text.remove_prefix(1);
  }
  while (!text.empty() && is_space(static_cast<unsigned char>(text.back()))) {
    text.remove_suffix(1);
  }
  return text;
}

template <typename T>
std::optional<T> ParseInteger(std::string_view text) {
  text = Trim(text);
  if (text.empty()) {
    return std::nullopt;
  }

  int base = 10;
  bool negative = false;
  if (text.front() == '-') {
    negative = true;
    text.remove_prefix(1);
  }
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
    text.remove_prefix(2);
  }
  if (text.empty()) {
    return std::nullopt;
  }

  int64_t parsed = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, base);
  if (result.ec != std::errc() || result.ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  if (negative) {
    parsed = -parsed;
  }
  if (parsed < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
      parsed > static_cast<int64_t>(std::numeric_limits<T>::max())) {
    return std::nullopt;
  }
  return static_cast<T>(parsed);
}

std::optional<std::filesystem::path> AutomationGamepadFilePath() {
  auto value = rex::platform::env::get(kAutomationGamepadFileEnv);
  if (!value || value->empty()) {
    return std::nullopt;
  }
  return std::filesystem::path(*value);
}

}  // namespace

X_INPUT_STATE ParseAutomationGamepadState(std::string_view text) {
  X_INPUT_STATE state = {};

  while (!text.empty()) {
    size_t line_end = text.find('\n');
    std::string_view line =
        line_end == std::string_view::npos ? text : text.substr(0, line_end);
    text = line_end == std::string_view::npos ? std::string_view{} : text.substr(line_end + 1);

    line = Trim(line);
    if (line.empty() || line.front() == '#') {
      continue;
    }

    const size_t separator = line.find('=');
    if (separator == std::string_view::npos) {
      continue;
    }

    const std::string_view key = Trim(line.substr(0, separator));
    const std::string_view value = Trim(line.substr(separator + 1));

    if (key == "packet") {
      if (auto parsed = ParseInteger<uint32_t>(value)) {
        state.packet_number = *parsed;
      }
    } else if (key == "buttons") {
      if (auto parsed = ParseInteger<uint16_t>(value)) {
        state.gamepad.buttons = *parsed;
      }
    } else if (key == "left_trigger") {
      if (auto parsed = ParseInteger<uint8_t>(value)) {
        state.gamepad.left_trigger = *parsed;
      }
    } else if (key == "right_trigger") {
      if (auto parsed = ParseInteger<uint8_t>(value)) {
        state.gamepad.right_trigger = *parsed;
      }
    } else if (key == "thumb_lx") {
      if (auto parsed = ParseInteger<int16_t>(value)) {
        state.gamepad.thumb_lx = *parsed;
      }
    } else if (key == "thumb_ly") {
      if (auto parsed = ParseInteger<int16_t>(value)) {
        state.gamepad.thumb_ly = *parsed;
      }
    } else if (key == "thumb_rx") {
      if (auto parsed = ParseInteger<int16_t>(value)) {
        state.gamepad.thumb_rx = *parsed;
      }
    } else if (key == "thumb_ry") {
      if (auto parsed = ParseInteger<int16_t>(value)) {
        state.gamepad.thumb_ry = *parsed;
      }
    }
  }

  return state;
}

bool IsAutomationGamepadEnabled() {
  return AutomationGamepadFilePath().has_value();
}

AutomationInputDriver::AutomationInputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

AutomationInputDriver::~AutomationInputDriver() = default;

X_STATUS AutomationInputDriver::Setup() {
  if (auto path = AutomationGamepadFilePath()) {
    state_path_ = *path;
  }
  if (!state_path_.empty()) {
    REXLOG_INFO("Automation gamepad input enabled: {}", state_path_.string());
  }
  return X_STATUS_SUCCESS;
}

X_RESULT AutomationInputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,
                                                X_INPUT_CAPABILITIES* out_caps) {
  (void)flags;
  if (user_index != kUserIndex || state_path_.empty()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  if (out_caps) {
    std::memset(out_caps, 0, sizeof(*out_caps));
    out_caps->type = XINPUT_DEVTYPE_GAMEPAD;
    out_caps->sub_type = 0x01;
    out_caps->gamepad.buttons = 0xFFFF;
    out_caps->gamepad.left_trigger = 0xFF;
    out_caps->gamepad.right_trigger = 0xFF;
    out_caps->gamepad.thumb_lx = INT16_MAX;
    out_caps->gamepad.thumb_ly = INT16_MAX;
    out_caps->gamepad.thumb_rx = INT16_MAX;
    out_caps->gamepad.thumb_ry = INT16_MAX;
    out_caps->vibration.left_motor_speed = 0xFFFF;
    out_caps->vibration.right_motor_speed = 0xFFFF;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT AutomationInputDriver::GetState(uint32_t user_index, X_INPUT_STATE* out_state) {
  if (user_index != kUserIndex || state_path_.empty()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  std::lock_guard lock(state_mutex_);

  std::ifstream file(state_path_, std::ios::binary);
  if (file) {
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    last_state_ = ParseAutomationGamepadState(contents);
  }

  if (out_state) {
    *out_state = last_state_;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT AutomationInputDriver::SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) {
  (void)vibration;
  if (user_index != kUserIndex || state_path_.empty()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT AutomationInputDriver::GetKeystroke(uint32_t user_index, uint32_t flags,
                                             X_INPUT_KEYSTROKE* out_keystroke) {
  (void)flags;
  (void)out_keystroke;
  if (user_index != kUserIndex || state_path_.empty()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_EMPTY;
}

}  // namespace rex::input::automation
