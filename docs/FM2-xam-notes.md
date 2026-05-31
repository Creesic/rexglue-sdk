# FM2 XAM Notes

## Message Box Text Encoding

Date found: 2026-05-19

FM2's "No Gamer Profiles" dialog originally opened through the ReXGlue/XAM ImGui message-box path, but the title and body were mostly rendered as question marks. The button labels were readable.

The useful clue was that `XamShowMessageBoxUI` decoded button labels with:

```cpp
rex::memory::load_and_swap<std::u16string>(...)
```

but decoded the title and body with `ppc_pchar16_t::value()`. Xbox 360 guest UTF-16 strings are big-endian, so the direct `.value()` path fed byte-swapped text into UTF-8 conversion.

SDK-side fix:

- Add a helper that reads guest UTF-16 using `load_and_swap<std::u16string>`.
- Use that helper for `XamShowMessageBoxUI` title and body.
- Leave button decoding unchanged, since it already used the correct path.

Patched SDK file:

- `C:\Users\Tera\rexglue-sdk\src\kernel\xam\xam_ui.cpp`

After rebuilding/installing the SDK and relinking FM2, the dialog text rendered normally:

```text
No Gamer Profiles

No Gamer Profiles signed in. Game will be limited and you will not be able to save progress.

Continue without saving    Sign In
```

## Sign-In Follow-Up

After the text fix, clicking `Sign In` still did not complete the sign-in flow.

Current evidence:

- FM2 imports `XamShowSigninUI` and `XamUserGetSigninState`.
- ReXGlue's `XamShowSigninUI` currently broadcasts UI/sign-in notifications, but it does not present a real profile selector.
- The default `UserProfile::signin_state()` reports `1`, which is a local signed-in state.
- FM2 generated code has paths that explicitly check for `XamUserGetSigninState(...) == 2`, which is the stronger "signed in to Live" state.

Rejected test patch:

- Tried changing `UserProfile::signin_state()` from `1` to `2`.
- Result: FM2 still closed the popup after clicking `Sign In`, then showed the same `No Gamer Profiles` popup again when entering the menu.
- Conclusion: FM2 is not simply waiting for signin state `2`; the next boundary to inspect is the notification/UI flow after `XamShowSigninUI` and any XAM user/profile queries FM2 makes when rebuilding its internal gamer profile list.

Diagnostic build added:

- Added logging for `XamShowMessageBoxUI` title/buttons/chosen button to prove the `Sign In` button index returned to the game.
- Added logging for `XamUserReadProfileSettingsEx` arguments, requested setting IDs, whether each setting is known/set, and final result.
- Added logging for `XamUserCheckPrivilege` result.

Current hypothesis:

- The message-box/UI path returns, but FM2 still decides there is no usable gamer profile.
- Most likely next failure point is `XamUserReadProfileSettings` returning `X_ERROR_INVALID_PARAMETER` for an unimplemented profile setting, or a missing notification/state transition after `XamShowSigninUI`.

Next test:

- Run FM2, click `Sign In`, then capture the XAM log lines around:
  - `XamShowMessageBoxUI title='No Gamer Profiles' chose button ...`
  - `XamShowSigninUI(...) called`
  - `XamUserReadProfileSettingsEx(...)`
  - `XamUserCheckPrivilege(...)`

## XUSER_INDEX_ANY Profile Probe

Date found: 2026-05-20

The fixed logging showed the `Sign In` button path is working:

```text
XamShowMessageBoxUI title='No Gamer Profiles' chose button 1
XamShowSigninUI(1, 0) called
XamShowSigninUI: firing deferred XN_SYS_SIGNINCHANGED + XN_SYS_UI off
XamUserGetSigninState(0) = 1
```

The same run also showed FM2 probing profile settings with `user=255`, which is `XUSER_INDEX_ANY`:

```text
XamUserReadProfileSettingsEx(... user=255 ... buffer_size=0 ...)
XamUserReadProfileSettingsEx -> INSUFFICIENT_BUFFER: buffer_size=0 needed_size=128
```

Temporary SDK-side test patch:

- Resolve `XUSER_INDEX_ANY` (`0xFF`) to user 0 in the XAM profile/user identity paths.
- In `XamUserReadProfileSettingsEx`, write the resolved concrete user index into returned setting records.
- Log original and resolved user indices in `XamUserReadProfileSettingsEx`, `XamUserGetXUID`, `XamUserGetSigninInfo`, `XamUserGetName`, and `XamUserGetGamerTag`.
- Log each requested profile setting ID during profile reads, including size-only calls.

Patched SDK file:

- `C:\Users\Tera\rexglue-sdk\src\kernel\xam\xam_user.cpp`

Next test:

- Rebuild FM2, click `Sign In`, then check whether the popup loop stops.
- If it still loops, inspect the new logs for calls to `XamUserGetXUID`, `XamUserGetSigninInfo`, `XamUserGetName`, `XamUserGetGamerTag`, and any follow-up `XamUserReadProfileSettingsEx(... buffer!=0 ...)` result.

Follow-up temporary test:

- After the `XUSER_INDEX_ANY` patch, FM2 gets as far as repeatedly polling `XamUserGetXUID`.
- Temporarily changed `UserProfile::signin_state()` from `1` to `2` in `C:\Users\Tera\rexglue-sdk\include\rex\system\xam\user_profile.h`.
- Purpose: test whether FM2 needs the profile to report Live sign-in state after the fake sign-in UI closes.
- If this does not change the loop, revert the value to `1` and inspect the caller around the repeated `XamUserGetXUID` probes.

## Sign-In Popup Loop Fixed By XLIVEBASE Presence Init

Date found: 2026-05-20

FM2 continued to show the `No Gamer Profiles` popup even after the direct XAM user identity calls looked healthy:

```text
XamUserGetSigninState(0) = 2
XamUserGetXUID(user=0) -> SUCCESS
XamUserGetName(user=0) -> SUCCESS
XamUserCheckPrivilege(user=0, mask=0xfe) -> SUCCESS value=0
```

The useful clue was a separate XLIVEBASE app message failure:

```text
Unimplemented XLIVEBASE message app=000000FC, msg=00058037, arg1=30C9A000, arg2=7022F920
```

Xenia Canary handles message `0x00058037` as `XPresenceInitialize(...)` and returns success. ReXGlue did not have this case, so it fell through to the unimplemented XLIVEBASE path and returned failure. FM2 appears to require the presence initialization path before accepting the profile as usable at the press-start flow.

SDK-side fix:

- Add an XLIVEBASE message case for `0x00058037`.
- Log it as `XPresenceInitialize(buffer_ptr, buffer_length)`.
- Return `X_E_SUCCESS`.

Patched SDK file:

- `C:\Users\Tera\rexglue-sdk\src\kernel\xam\apps\xlivebase_app.cpp`

Reference behavior:

- `C:\Users\Tera\Documents\GitHub\xenia-canary\src\xenia\kernel\xam\apps\xlivebase_app.cc`

Verification:

- Rebuilt and installed the SDK.
- Relinked FM2.
- The `No Gamer Profiles` popup no longer appears at the intro screen.
- Pressing Start proceeds like Xenia's auto-signed-in behavior.

## Logging Prerequisite

Date found: 2026-05-19

While trying to capture the sign-in diagnostics, `fm2.exe` contained the new XAM logging strings but `C:\temp\fm2-clean.log` was not updating.

Root cause:

- `ReXApp::OnInitialize()` loads `fm2.toml` before explicitly calling `InitLogging(...)`, which is intentional so `log_file` can come from the TOML.
- `rex::cvar::LoadConfig(...)` emits a log line while loading the TOML.
- That log line lazily called `InitLogging()` with no `log_file`, creating only the early default sinks.
- The later explicit `InitLogging(log_file=...)` path treated this as reinitialization and only updated log levels, so it never created the file sink.

SDK-side fix:

- Update `rex::InitLogging(const LogConfig&)` so reinitialization refreshes configured sinks.
- Track the active file sink path.
- If a reinit config supplies a new `log_file`, create or replace the file sink and rebuild existing category logger sink lists.

Patched SDK file:

- `C:\Users\Tera\rexglue-sdk\src\core\logging.cpp`

Verification:

- A controlled FM2 launch now creates/updates `C:\temp\fm2-clean.log`.
- Latest XAM diagnostics include repeated `XamUserReadProfileSettingsEx(... user=255 ...) -> INSUFFICIENT_BUFFER` during startup/profile probing.
