#pragma once

#include "theme_editor.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::boot_edit {

inline constexpr std::uint32_t kBootAnimationVersion = 9199;
inline constexpr std::string_view kUnnamedThemeName = "Unnamed Theme";
inline constexpr std::string_view kThemeDirectory = "Themes:\\";
inline constexpr std::string_view kThemeExtension = ".xbat";
inline constexpr std::uint32_t kNoMoreFiles = 18;

// Exact 632-byte on-disk/in-memory theme payload recovered from
// fn_8204CC40.  The target initializes a version/sentinel pair followed by
// 39 four-float color records.  Keeping this typed prevents the serialized
// theme format from disappearing behind an opaque byte blob.
struct ThemePayload {
  std::uint32_t boot_animation_version = kBootAnimationVersion;
  std::int32_t selected_mesh = -1;
  std::array<Color, 39> colors{};
};

static_assert(sizeof(ThemePayload) == 632,
              "PeerPressure theme payload must remain byte-exact");

ThemePayload MakeDefaultThemePayload();

using LiveMeshColors = std::array<std::array<Color, 3>, 11>;

struct ThemeSnapshot {
  std::string active_path;
  std::string theme_name;
  ThemePayload payload{};
};

// fn_8204E1A8 copies the serialized payload and then replaces its first
// eleven diffuse/specular/ambient triples with the current renderer state.
ThemeSnapshot CaptureThemeSnapshot(std::string_view active_path,
                                   std::string_view theme_name,
                                   const ThemePayload& payload,
                                   const LiveMeshColors& live_meshes);

struct ThemeManagerState {
  std::string active_path;
  std::string theme_name;
  ThemePayload live_theme{};
  ThemePayload edited_theme{};
  bool live_dirty = false;
  bool edited_dirty = false;
  bool refresh_lock_initialized = true;
  std::uintptr_t enumeration_worker = 0;
  std::vector<SavedTheme> saved_themes;
};

struct ThemeNameCompletion {
  bool cancelled = false;
  bool invoke_continuation = false;
  std::uint32_t continuation_state = 0;
  std::string path;
  std::string name;
};

struct ThemeDirectoryScan {
  bool open_succeeded = true;
  std::uint32_t open_error_code = 0;
  std::uint32_t terminal_error_code = kNoMoreFiles;
  std::vector<std::string> file_names;
};

struct ThemeEnumerationResult {
  std::uint32_t worker_exit_code = 0;
  bool publish = false;
  bool find_handle_closed = false;
  std::uint32_t error_code = 0;
  std::vector<SavedTheme> themes;
};

enum class FeedbackKind { kNone, kInformation, kError };

struct FeedbackDialog {
  FeedbackKind kind = FeedbackKind::kNone;
  std::string title;
  std::string message;
  float display_seconds = 0.0f;
};

struct LaunchNameEnumeration {
  bool opened = true;
  std::vector<std::string> symbolic_names;
};

inline constexpr std::uint32_t kLaunchFlagDetMedia = 0x00000002;
inline constexpr std::uint32_t kLaunchFlagDashArcade = 0x00000100;
inline constexpr std::uint32_t kLaunchFlagDashNuiHub = 0x00040000;

std::string FormatThemePath(std::string_view name);
ThemeNameCompletion CompleteThemeNameDialog(bool accepted,
                                            std::string_view name,
                                            const std::function<void()>&
                                                continuation = {});
ThemeManagerState InitializeThemeManager();
ThemeEnumerationResult EnumerateSavedThemes(ThemeManagerState* state,
                                             const ThemeDirectoryScan& scan);

// Partial model of the single-worker gate inside the compound 0x82050958
// inventory entry. Thread creation and GetLastError are platform dependencies
// represented by the two inputs. The compound entry remains explicitly
// uncovered until its other bodies and padding are reconstructed too.
bool StartThemeEnumeration(ThemeManagerState* state,
                           std::uintptr_t created_worker,
                           std::uint32_t creation_error,
                           std::uint32_t* reported_error);

void ResetToUnnamedTheme(ThemeManagerState* state,
                         const ThemePayload& live_default,
                         const ThemePayload& scratch_default);
FeedbackDialog SaveThemeWithFeedback(ThemeManagerState* state,
                                     bool completion_accepted,
                                     bool save_failed);
FeedbackDialog LoadThemeWithFeedback(ThemeManagerState* state,
                                     std::size_t selected_index,
                                     bool load_succeeded);

std::uint32_t LaunchFlagForSymbolicName(std::string_view symbolic_name);
bool TranslateLaunchSymbolicNames(const LaunchNameEnumeration& enumeration,
                                  std::uint32_t* flags);

// The two launch-name helpers above are retained as useful exploratory models,
// but their unsplittable 0x82053238 inventory entry is classified unknown and
// is intentionally absent from the readable-source coverage count. Exact
// cross-component matches do not by themselves establish public/platform
// provenance.

}  // namespace bandwagon::boot_edit
