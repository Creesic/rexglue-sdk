#pragma once
/**
 ******************************************************************************
 * ReXGlue automation input driver.
 ******************************************************************************
 */

#include <rex/input/input_driver.h>

#include <filesystem>
#include <mutex>
#include <string_view>

namespace rex::input::automation {

inline constexpr const char* kAutomationGamepadFileEnv = "REX_AUTOMATION_GAMEPAD_FILE";

[[nodiscard]] X_INPUT_STATE ParseAutomationGamepadState(std::string_view text);
[[nodiscard]] bool IsAutomationGamepadEnabled();

class AutomationInputDriver final : public InputDriver {
 public:
  explicit AutomationInputDriver(rex::ui::Window* window, size_t window_z_order);
  ~AutomationInputDriver() override;

  X_STATUS Setup() override;

  X_RESULT GetCapabilities(uint32_t user_index, uint32_t flags,
                           X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT GetState(uint32_t user_index, X_INPUT_STATE* out_state) override;
  X_RESULT SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetKeystroke(uint32_t user_index, uint32_t flags,
                        X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  std::filesystem::path state_path_;
  std::mutex state_mutex_;
  X_INPUT_STATE last_state_ = {};
};

}  // namespace rex::input::automation
