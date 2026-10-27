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

  const BootEditStartupState startup = InitializeBootEditStartupState();
  assert(startup.root_state == 0 && !startup.active);
  assert((startup.service_handles == std::array<std::uint32_t, 3>{}));
  for (std::uint8_t value : startup.editor_storage) assert(value == 0);
  for (std::uint8_t value : startup.navigation_storage) assert(value == 0);
  assert(startup.previous_ticks == 0 && startup.current_ticks == 0);
  std::uint32_t timeline_bits = 0;
  std::memcpy(&timeline_bits, &startup.timeline_origin, sizeof(timeline_bits));
  assert(timeline_bits == 0x3a83126f);
  assert(!startup.editor_visible && !startup.refresh_pending &&
         !startup.notice_present);
  assert((startup.task_state == std::array<std::uint32_t, 3>{}));
  assert(startup.selection == 0 && startup.navigation_state == 0);
  assert(startup.startup_theme.boot_animation_version ==
         kBootAnimationVersion);
  assert(startup.startup_theme.selected_mesh == -1);
  assert(startup.startup_theme.colors[38] ==
         MakeDefaultThemePayload().colors[38]);
  std::uint32_t final_green_bits = 0;
  std::memcpy(&final_green_bits, &state.live_theme.colors[38][1],
              sizeof(final_green_bits));
  assert(final_green_bits == 0x3f4ccccd);

  LiveMeshColors live_meshes{};
  for (std::size_t mesh = 0; mesh < live_meshes.size(); ++mesh) {
    for (std::size_t channel = 0; channel < live_meshes[mesh].size();
         ++channel) {
      live_meshes[mesh][channel] =
          Color({static_cast<float>(mesh), static_cast<float>(channel),
                 static_cast<float>(mesh * 3 + channel), 1.0f});
    }
  }
  ThemePayload capture_input = MakeDefaultThemePayload();
  const Color untouched_color = capture_input.colors[33];
  const ThemeSnapshot snapshot = CaptureThemeSnapshot(
      "Themes:\\Green.xbat", "Green", capture_input, live_meshes);
  assert(snapshot.active_path == "Themes:\\Green.xbat");
  assert(snapshot.theme_name == "Green");
  assert(snapshot.payload.colors[0] == live_meshes[0][0]);
  assert(snapshot.payload.colors[32] == live_meshes[10][2]);
  assert(snapshot.payload.colors[33] == untouched_color);

  const SavedTheme load_record{"Themes:\\Loaded.xbat", "Loaded"};
  bool decoder_called = false;
  ThemeLoadResult load_result = DecodeThemeSnapshot(
      load_record, live_meshes,
      [&decoder_called](std::string_view path, ThemePayload* payload) {
        decoder_called = true;
        assert(path == "Themes:\\Loaded.xbat");
        assert(payload->boot_animation_version == kBootAnimationVersion);
        assert(payload->selected_mesh == -1);
        payload->colors[33] = Color({0.1f, 0.2f, 0.3f, 0.4f});
        return true;
      });
  assert(decoder_called && load_result.loaded);
  assert(load_result.snapshot.active_path == load_record.path);
  assert(load_result.snapshot.theme_name == load_record.display_name);
  assert(load_result.snapshot.payload.colors[32] == live_meshes[10][2]);
  assert(load_result.snapshot.payload.colors[33] ==
         Color({0.1f, 0.2f, 0.3f, 0.4f}));
  decoder_called = false;
  load_result = DecodeThemeSnapshot(
      load_record, live_meshes,
      [&decoder_called](std::string_view, ThemePayload*) {
        decoder_called = true;
        return false;
      });
  assert(decoder_called && !load_result.loaded);
  assert(load_result.snapshot.active_path.empty());

  ThemeSnapshot encode_snapshot = snapshot;
  bool encoder_called = false;
  bool encoded = EncodeThemeSnapshot(
      &encode_snapshot, "Themes:\\Saved.xbat", "Saved",
      [&encoder_called](const ThemeSnapshot& value) {
        encoder_called = true;
        assert(value.active_path == "Themes:\\Saved.xbat");
        assert(value.theme_name == "Saved");
        return true;
      });
  assert(encoded && encoder_called);
  encoder_called = false;
  encoded = EncodeThemeSnapshot(
      &encode_snapshot, "Themes:\\Failed.xbat", "Failed",
      [&encoder_called](const ThemeSnapshot& value) {
        encoder_called = true;
        assert(value.active_path == "Themes:\\Failed.xbat");
        assert(value.theme_name == "Failed");
        return false;
      });
  assert(!encoded && encoder_called);
  assert(encode_snapshot.active_path == "Themes:\\Failed.xbat");
  assert(encode_snapshot.theme_name == "Failed");
  assert(!EncodeThemeSnapshot(nullptr, "ignored", "ignored", {}));

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
