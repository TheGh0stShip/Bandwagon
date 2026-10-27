#include "theme_selector.hpp"

namespace bandwagon::boot_edit {

bool PollThemeEnumeration(std::uintptr_t* thread_handle,
                          const ThemeEnumerationServices& services) {
  if (!thread_handle || *thread_handle == 0) return true;
  if (!services.wait || services.wait(*thread_handle, 0) != 0) return false;
  if (services.close) services.close(*thread_handle);
  *thread_handle = 0;
  return true;
}

void DrawThemeSelector(ThemeSelectorState* state, std::string_view current_theme,
                       bool enumeration_in_progress,
                       const std::vector<SavedTheme>& published_themes,
                       const ThemeSelectorInput& input,
                       const ThemeSelectorServices& services) {
  if (!state) return;
  if (services.draw_current_theme) services.draw_current_theme(current_theme);

  const bool list_visible = !services.begin_list || services.begin_list(kThemeListId);
  if (list_visible) {
    if (!enumeration_in_progress && state->refresh_requested) {
      state->displayed_themes = published_themes;
      state->refresh_requested = false;
      state->selected_theme = 0;
    }

    if (!enumeration_in_progress && !state->displayed_themes.empty()) {
      for (std::uint32_t i = 0; i < state->displayed_themes.size(); ++i) {
        if (services.draw_theme) {
          services.draw_theme(state->displayed_themes[i].display_name,
                              state->view == 1 && state->selected_theme == i);
        }
      }
    } else if (services.draw_status) {
      services.draw_status(enumeration_in_progress ? kThemeLoadingText
                                                   : kNoThemesText);
    }
    if (services.end_list) services.end_list();
  }

  if (state->feedback_modal_pending || state->async_task_pending ||
      state->view != 1) {
    return;
  }
  if (input.back) state->view = 0;

  const std::uint32_t count =
      static_cast<std::uint32_t>(state->displayed_themes.size());
  if (count == 0) return;
  if (input.next) state->selected_theme = (state->selected_theme + 1) % count;
  if (input.previous) {
    state->selected_theme =
        state->selected_theme == 0 ? count - 1 : state->selected_theme - 1;
  }
  if (!input.activate) return;

  if (state->unsaved_changes) {
    state->async_task_pending = true;
    if (services.start_discard_confirmation) {
      services.start_discard_confirmation(2);
    }
  } else if (services.load_theme) {
    services.load_theme(state->selected_theme);
  }
}

void UpdateBootEditFrame(BootEditFrameState* state,
                         const BootEditFrameServices& services) {
  if (!state) return;

  const bool actions_open =
      !services.begin_panel || services.begin_panel("##menu_options");
  if (actions_open && services.draw_settings_actions) {
    services.draw_settings_actions();
  }
  if (actions_open && services.end_panel) services.end_panel();

  const bool list_open =
      !services.begin_panel || services.begin_panel("##file_list");
  if (list_open && services.draw_theme_selector) {
    services.draw_theme_selector();
  }
  if (list_open && services.end_panel) services.end_panel();

  if (state->modal_present && !state->modal_poll_suppressed) {
    if (!state->modal_completion_pending && services.update_modal) {
      services.update_modal();
    }
    if (services.modal_finished && services.modal_finished()) {
      if (services.destroy_modal) services.destroy_modal();
      state->modal_present = false;
    }
  }
  if (state->task_present && services.poll_task && services.poll_task()) {
    if (services.destroy_task) services.destroy_task();
    state->task_present = false;
  }
  if (services.end_frame) services.end_frame();
}

PackedResourceState InitializePackedResourceState() { return {}; }

}  // namespace bandwagon::boot_edit
