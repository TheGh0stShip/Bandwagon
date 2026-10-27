#include "theme_manager.hpp"

namespace bandwagon::boot_edit {

ThemePayload MakeDefaultThemePayload() {
  ThemePayload payload;
  payload.colors = {{
      {0.7f, 0.7f, 0.7f, 1.0f}, {0.5f, 0.5f, 0.5f, 8.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {0.7f, 0.7f, 0.7f, 1.0f},
      {0.5f, 0.5f, 0.5f, 8.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {0.7f, 0.7f, 0.7f, 1.0f}, {0.5f, 0.5f, 0.5f, 8.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
      {0.7f, 0.7f, 0.7f, 1.0f}, {0.8f, 0.8f, 0.8f, 1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {0.7f, 0.9f, 0.6f, 1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {0.6f, 0.9f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f},
      {0.5725489854812622f, 0.749019980430603f, 0.22745099663734436f,
       1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {0.6274510025978088f, 0.6509799957275391f, 0.6196079850196838f,
       1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {0.6f, 0.9f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f},
      {0.5725489854812622f, 0.749019980430603f, 0.22745099663734436f,
       1.0f},
      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
      {0.0470588244497776f, 0.5333333611488342f, 0.062745101749897f,
       1.0f},
      {0.800000011920929f, 0.9019607901573181f, 0.27450981736183167f,
       1.0f},
      {0.9803921580314636f, 1.0f, 0.4117647111415863f, 1.0f},
      {0.30588236451148987f, 0.7411764860153198f, 0.1568627506494522f,
       1.0f},
      {0.3499999940395355f, 1.0f, 0.0f, 1.0f},
      {0.699999988079071f, 0.800000011920929f, 0.5f, 1.0f},
  }};
  return payload;
}

ThemeSnapshot CaptureThemeSnapshot(std::string_view active_path,
                                   std::string_view theme_name,
                                   const ThemePayload& payload,
                                   const LiveMeshColors& live_meshes) {
  ThemeSnapshot snapshot;
  snapshot.active_path = std::string(active_path);
  snapshot.theme_name = std::string(theme_name);
  snapshot.payload = payload;
  for (std::size_t mesh = 0; mesh < live_meshes.size(); ++mesh) {
    for (std::size_t channel = 0; channel < live_meshes[mesh].size();
         ++channel) {
      snapshot.payload.colors[mesh * 3 + channel] =
          live_meshes[mesh][channel];
    }
  }
  return snapshot;
}

ThemeLoadResult DecodeThemeSnapshot(const SavedTheme& saved_theme,
                                    const LiveMeshColors& live_meshes,
                                    const ThemePayloadDecoder& decoder) {
  ThemeLoadResult result;
  ThemePayload payload = MakeDefaultThemePayload();
  if (!decoder || !decoder(saved_theme.path, &payload)) return result;
  result.snapshot = CaptureThemeSnapshot(saved_theme.path,
                                         saved_theme.display_name, payload,
                                         live_meshes);
  result.loaded = true;
  return result;
}

std::string FormatThemePath(std::string_view name) {
  return std::string(kThemeDirectory) + std::string(name) +
         std::string(kThemeExtension);
}

ThemeNameCompletion CompleteThemeNameDialog(bool accepted,
                                            std::string_view name,
                                            const std::function<void()>&
                                                continuation) {
  ThemeNameCompletion result;
  if (accepted) {
    result.continuation_state = 2;
    result.path = FormatThemePath(name);
    result.name = std::string(name);
  } else {
    result.cancelled = true;
    result.invoke_continuation = true;
    if (continuation) continuation();
  }
  return result;
}

ThemeManagerState InitializeThemeManager() {
  ThemeManagerState state;
  state.theme_name = std::string(kUnnamedThemeName);
  state.live_theme = MakeDefaultThemePayload();
  state.edited_theme = MakeDefaultThemePayload();
  return state;
}

ThemeEnumerationResult EnumerateSavedThemes(ThemeManagerState* state,
                                             const ThemeDirectoryScan& scan) {
  ThemeEnumerationResult result;
  if (!state || !scan.open_succeeded) {
    result.worker_exit_code = 1;
    result.error_code = scan.open_error_code;
    result.find_handle_closed = scan.open_succeeded;
    return result;
  }
  result.find_handle_closed = true;
  if (scan.terminal_error_code != kNoMoreFiles) {
    result.worker_exit_code = 1;
    result.error_code = scan.terminal_error_code;
    return result;
  }

  result.themes.reserve(scan.file_names.size());
  for (const std::string& file_name : scan.file_names) {
    result.themes.push_back(BuildSavedTheme(file_name));
  }
  // The target performs this replacement while holding its refresh lock. A
  // host implementation supplies that synchronization around this portable
  // state boundary rather than relifting the XDK critical-section wrapper.
  state->saved_themes = result.themes;
  result.publish = true;
  return result;
}

bool StartThemeEnumeration(ThemeManagerState* state,
                           std::uintptr_t created_worker,
                           std::uint32_t creation_error,
                           std::uint32_t* reported_error) {
  if (!state) return false;
  if (state->enumeration_worker != 0) return true;
  if (created_worker == 0) {
    if (reported_error) *reported_error = creation_error;
    return false;
  }
  state->enumeration_worker = created_worker;
  return true;
}

void ResetToUnnamedTheme(ThemeManagerState* state,
                         const ThemePayload& live_default,
                         const ThemePayload& scratch_default) {
  if (!state) return;
  state->active_path.clear();
  state->theme_name = std::string(kUnnamedThemeName);
  state->live_theme = live_default;
  state->live_theme.boot_animation_version = kBootAnimationVersion;
  state->edited_theme = scratch_default;
  state->edited_theme.boot_animation_version = kBootAnimationVersion;
  state->live_dirty = false;
  state->edited_dirty = false;
}

FeedbackDialog SaveThemeWithFeedback(ThemeManagerState* state,
                                     bool completion_accepted,
                                     bool save_failed) {
  if (!state || !completion_accepted) return {};
  if (save_failed) {
    return {FeedbackKind::kError, "Error saving file",
            "An error occurred while saving the theme, your changes have not "
            "been saved!", 0.0f};
  }

  state->edited_theme = state->live_theme;
  state->live_dirty = false;
  state->edited_dirty = false;
  return {FeedbackKind::kInformation, "Theme saved",
          "Theme saved successfully!", 2.0f};
}

FeedbackDialog LoadThemeWithFeedback(ThemeManagerState* state,
                                     std::size_t selected_index,
                                     bool load_succeeded) {
  if (!state) return {};
  if (selected_index >= state->saved_themes.size() || !load_succeeded) {
    return {FeedbackKind::kError, "Error reading theme",
            "An error occurred while loading the theme!", 0.0f};
  }

  state->active_path = state->saved_themes[selected_index].path;
  state->theme_name = state->saved_themes[selected_index].display_name;
  state->edited_theme = state->live_theme;
  state->live_dirty = false;
  state->edited_dirty = false;
  return {};
}

std::uint32_t LaunchFlagForSymbolicName(std::string_view symbolic_name) {
  if (symbolic_name == "XLNI_DET_MEDIA") return kLaunchFlagDetMedia;
  if (symbolic_name == "XLNI_DASH_ARCADE") return kLaunchFlagDashArcade;
  if (symbolic_name == "XLNI_DASH_NUIHUB") return kLaunchFlagDashNuiHub;
  return 0;
}

bool TranslateLaunchSymbolicNames(const LaunchNameEnumeration& enumeration,
                                  std::uint32_t* flags) {
  if (!enumeration.opened || !flags) return false;
  for (const std::string& name : enumeration.symbolic_names) {
    *flags |= LaunchFlagForSymbolicName(name);
  }
  return true;
}

}  // namespace bandwagon::boot_edit
