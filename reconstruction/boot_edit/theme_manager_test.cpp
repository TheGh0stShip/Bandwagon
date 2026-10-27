#include "theme_manager.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace bandwagon::boot_edit;

  assert(FormatThemePath("Blue") == "Themes:\\Blue.xbat");
  const ThemeNameCompletion accepted = CompleteThemeNameDialog(true, "Blue");
  assert(!accepted.cancelled && !accepted.invoke_continuation);
  assert(accepted.continuation_state == 2);
  assert(accepted.name == "Blue");
  assert(accepted.path == "Themes:\\Blue.xbat");
  bool continuation_called = false;
  const ThemeNameCompletion cancelled = CompleteThemeNameDialog(
      false, "Lost", [&continuation_called] { continuation_called = true; });
  assert(cancelled.cancelled && cancelled.invoke_continuation);
  assert(continuation_called);
  assert(cancelled.path.empty() && cancelled.name.empty());

  ThemeManagerState state = InitializeThemeManager();
  assert(state.theme_name == "Unnamed Theme");
  assert(state.active_path.empty());
  assert(state.live_theme.boot_animation_version == 9199);
  assert(state.edited_theme.boot_animation_version == 9199);
  assert(state.refresh_lock_initialized);
  assert(!state.live_dirty && !state.edited_dirty);
  assert(state.live_theme.selected_mesh == -1);
  assert(state.live_theme.colors.size() == 39);
  assert(state.live_theme.colors[0] == Color({0.7f, 0.7f, 0.7f, 1.0f}));
  assert(state.live_theme.colors[1][3] == 8.0f);
  assert(state.live_theme.colors[11] == Color({0.0f, 0.0f, 0.0f, 1.0f}));
  std::uint32_t final_green_bits = 0;
  std::memcpy(&final_green_bits, &state.live_theme.colors[38][1],
              sizeof(final_green_bits));
  assert(final_green_bits == 0x3f4ccccd);

  ThemeDirectoryScan scan;
  scan.file_names = {"Blue.xbat", "Odd.xbat.backup"};
  const ThemeEnumerationResult enumeration = EnumerateSavedThemes(&state, scan);
  assert(enumeration.publish && enumeration.worker_exit_code == 0);
  assert(enumeration.find_handle_closed);
  assert(enumeration.themes.size() == 2);
  assert(state.saved_themes.size() == enumeration.themes.size());
  assert(state.saved_themes[0].display_name ==
         enumeration.themes[0].display_name);
  assert(state.saved_themes[1].path == enumeration.themes[1].path);
  assert(enumeration.themes[0].display_name == "Blue");
  assert(enumeration.themes[1].display_name == "Odd");
  scan.terminal_error_code = 5;
  const ThemeEnumerationResult failed_enumeration =
      EnumerateSavedThemes(&state, scan);
  assert(!failed_enumeration.publish);
  assert(failed_enumeration.find_handle_closed);
  assert(failed_enumeration.worker_exit_code == 1);
  assert(failed_enumeration.error_code == 5);
  assert(failed_enumeration.themes.empty());

  std::uint32_t reported_error = 0;
  assert(!StartThemeEnumeration(&state, 0, 0x1234, &reported_error));
  assert(reported_error == 0x1234 && state.enumeration_worker == 0);
  assert(StartThemeEnumeration(&state, 0x55, 0, &reported_error));
  assert(state.enumeration_worker == 0x55);
  assert(StartThemeEnumeration(&state, 0, 0x9999, &reported_error));
  assert(reported_error == 0x1234);

  ThemePayload live_defaults;
  live_defaults.boot_animation_version = 1;
  live_defaults.colors[0][1] = 0.25f;
  ThemePayload scratch_defaults;
  scratch_defaults.boot_animation_version = 2;
  scratch_defaults.colors[0][1] = 0.75f;
  state.live_dirty = true;
  state.edited_dirty = true;
  state.theme_name = "Changed";
  state.active_path = "Themes:\\Changed.xbat";
  ResetToUnnamedTheme(&state, live_defaults, scratch_defaults);
  assert(state.theme_name == "Unnamed Theme" && state.active_path.empty());
  assert(state.live_theme.boot_animation_version == 9199);
  assert(state.live_theme.colors[0][1] == 0.25f);
  assert(state.edited_theme.colors[0][1] == 0.75f);
  assert(!state.live_dirty && !state.edited_dirty);

  state.live_dirty = true;
  state.edited_dirty = true;
  FeedbackDialog feedback = SaveThemeWithFeedback(&state, false, false);
  assert(feedback.kind == FeedbackKind::kNone && state.live_dirty);
  feedback = SaveThemeWithFeedback(&state, true, true);
  assert(feedback.kind == FeedbackKind::kError);
  assert(feedback.title == "Error saving file" && state.live_dirty);
  feedback = SaveThemeWithFeedback(&state, true, false);
  assert(feedback.kind == FeedbackKind::kInformation);
  assert(feedback.title == "Theme saved");
  assert(feedback.display_seconds == 2.0f);
  assert(!state.live_dirty && !state.edited_dirty);

  state.live_dirty = true;
  state.edited_dirty = true;
  feedback = LoadThemeWithFeedback(&state, 0, false);
  assert(feedback.kind == FeedbackKind::kError && state.live_dirty);
  feedback = LoadThemeWithFeedback(&state, 0, true);
  assert(feedback.kind == FeedbackKind::kNone);
  assert(!state.live_dirty && !state.edited_dirty);

  std::uint32_t flags = 0x80000000;
  LaunchNameEnumeration launch_names;
  launch_names.symbolic_names = {"XLNI_DET_MEDIA", "ignored",
                                 "XLNI_DASH_ARCADE", "XLNI_DASH_NUIHUB"};
  assert(TranslateLaunchSymbolicNames(launch_names, &flags));
  assert(flags == (0x80000000 | kLaunchFlagDetMedia |
                   kLaunchFlagDashArcade | kLaunchFlagDashNuiHub));
  launch_names.opened = false;
  assert(!TranslateLaunchSymbolicNames(launch_names, &flags));

  std::cout << "BootEdit theme-manager reconstruction tests passed\n";
}
