#include <catch2/catch_test_macros.hpp>

#include <rex/input/mnk/mnk_input_driver.h>

TEST_CASE("MnK mouse policy tracks input without locking the window",
          "[input][mnk]") {
  using rex::input::mnk::BuildMnkMouseWindowPolicy;

  const auto active_policy =
      BuildMnkMouseWindowPolicy(/*enabled=*/true, /*has_focus=*/true,
                                /*active=*/true);

  CHECK(active_policy.track_mouse);
  CHECK_FALSE(active_policy.capture_window_mouse);
  CHECK_FALSE(active_policy.hide_cursor);
  CHECK_FALSE(active_policy.recenter_cursor);
}

TEST_CASE("MnK mouse policy is inactive unless enabled, focused, and active",
          "[input][mnk]") {
  using rex::input::mnk::BuildMnkMouseWindowPolicy;

  const auto disabled_policy =
      BuildMnkMouseWindowPolicy(/*enabled=*/false, /*has_focus=*/true,
                                /*active=*/true);
  const auto unfocused_policy =
      BuildMnkMouseWindowPolicy(/*enabled=*/true, /*has_focus=*/false,
                                /*active=*/true);
  const auto inactive_policy =
      BuildMnkMouseWindowPolicy(/*enabled=*/true, /*has_focus=*/true,
                                /*active=*/false);

  CHECK_FALSE(disabled_policy.track_mouse);
  CHECK_FALSE(unfocused_policy.track_mouse);
  CHECK_FALSE(inactive_policy.track_mouse);

  CHECK_FALSE(disabled_policy.capture_window_mouse);
  CHECK_FALSE(unfocused_policy.hide_cursor);
  CHECK_FALSE(inactive_policy.recenter_cursor);
}
