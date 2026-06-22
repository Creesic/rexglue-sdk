#include <catch2/catch_test_macros.hpp>

#include <rex/input/automation/automation_input_driver.h>
#include <rex/input/input.h>

TEST_CASE("automation gamepad parser maps button state text",
          "[input][automation]") {
  const auto state = rex::input::automation::ParseAutomationGamepadState(
      "packet=42\n"
      "buttons=0x1000\n"
      "left_trigger=255\n"
      "right_trigger=17\n"
      "thumb_lx=-100\n"
      "thumb_ly=100\n"
      "thumb_rx=-32768\n"
      "thumb_ry=32767\n");

  CHECK(state.packet_number == 42u);
  CHECK(state.gamepad.buttons == rex::input::X_INPUT_GAMEPAD_A);
  CHECK(state.gamepad.left_trigger == 255u);
  CHECK(state.gamepad.right_trigger == 17u);
  CHECK(state.gamepad.thumb_lx == -100);
  CHECK(state.gamepad.thumb_ly == 100);
  CHECK(state.gamepad.thumb_rx == INT16_MIN);
  CHECK(state.gamepad.thumb_ry == INT16_MAX);
}

TEST_CASE("automation gamepad parser ignores unknown and malformed fields",
          "[input][automation]") {
  const auto state = rex::input::automation::ParseAutomationGamepadState(
      "buttons=0x1000\n"
      "buttons=not-a-number\n"
      "left_trigger=999\n"
      "unknown=0xFFFF\n");

  CHECK(state.gamepad.buttons == rex::input::X_INPUT_GAMEPAD_A);
  CHECK(state.gamepad.left_trigger == 0u);
  CHECK(state.gamepad.right_trigger == 0u);
}
