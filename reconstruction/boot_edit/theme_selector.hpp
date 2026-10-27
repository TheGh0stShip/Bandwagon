#pragma once

#include "theme_editor.hpp"

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace bandwagon::boot_edit {

inline constexpr std::string_view kThemeListId = "##file_list_box";
inline constexpr std::string_view kThemeLoadingText = "Loading themes...";
inline constexpr std::string_view kNoThemesText =
    "No saved themes available";

struct ThemeSelectorInput {
  bool back = false;      // controller mask 0x2000
  bool next = false;      // controller mask 0x0002
  bool previous = false;  // controller mask 0x0001
  bool activate = false;  // controller mask 0x1000
};

struct ThemeSelectorState {
  std::uint32_t view = 0;
  std::uint32_t selected_theme = 0;
  bool refresh_requested = false;
  bool unsaved_changes = false;
  bool feedback_modal_pending = false;
  bool async_task_pending = false;
  std::vector<SavedTheme> displayed_themes;
};

struct ThemeSelectorServices {
  std::function<void(std::string_view)> draw_current_theme;
  std::function<bool(std::string_view)> begin_list;
  std::function<void(std::string_view, bool)> draw_theme;
  std::function<void(std::string_view)> draw_status;
  std::function<void()> end_list;
  std::function<void(std::uint32_t)> start_discard_confirmation;
  std::function<void(std::uint32_t)> load_theme;
};

struct ThemeEnumerationServices {
  // Returns the platform wait status. The target treats exactly zero as
  // signaled and every other value as still pending.
  std::function<std::uint32_t(std::uintptr_t, std::uint32_t)> wait;
  std::function<void(std::uintptr_t)> close;
};

// Readable model of 0x8204cbd8. A null handle is already complete; otherwise
// the target performs a zero-timeout poll and closes/clears only on status 0.
bool PollThemeEnumeration(std::uintptr_t* thread_handle,
                          const ThemeEnumerationServices& services);

void DrawThemeSelector(ThemeSelectorState* state, std::string_view current_theme,
                       bool enumeration_in_progress,
                       const std::vector<SavedTheme>& published_themes,
                       const ThemeSelectorInput& input,
                       const ThemeSelectorServices& services);

struct BootEditFrameState {
  bool modal_present = false;
  bool modal_completion_pending = false;
  bool modal_poll_suppressed = false;
  bool task_present = false;
};

struct BootEditFrameServices {
  std::function<bool(std::string_view)> begin_panel;
  std::function<void()> end_panel;
  std::function<void()> draw_settings_actions;
  std::function<void()> draw_theme_selector;
  std::function<void()> update_modal;
  std::function<bool()> modal_finished;
  std::function<void()> destroy_modal;
  std::function<bool()> poll_task;
  std::function<void()> destroy_task;
  std::function<void()> end_frame;
};

void UpdateBootEditFrame(BootEditFrameState* state,
                         const BootEditFrameServices& services);

// The non-pdata 0x82052a60 constructor is embedded at the tail of the
// 0x82052830 inventory extent.
struct PackedResourceState {
  bool initialized = false;
  std::uint32_t resource_count = 0;
  std::uint32_t table_size = 0;
  std::uint32_t data_offset = 0;
  std::uint32_t data_size = 0;
  std::uint32_t table_offset = 0;
  std::uint32_t flags = 0;
};

PackedResourceState InitializePackedResourceState();

}  // namespace bandwagon::boot_edit
