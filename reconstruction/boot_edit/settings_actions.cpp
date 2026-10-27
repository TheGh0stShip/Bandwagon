#include "settings_actions.hpp"

namespace bandwagon::boot_edit {
namespace {

void BeginDiscard(SettingsActionState* state,
                  const SettingsActionServices& services) {
  state->async_task_pending = true;
  if (services.start_discard_confirmation) {
    // The target stores action value 1 in the discard task created from this
    // strip. Its completion adapter performs the deferred destructive action.
    services.start_discard_confirmation(1);
  }
}

void ResetOrConfirm(SettingsActionState* state,
                    const SettingsActionServices& services) {
  if (state->unsaved_changes) {
    BeginDiscard(state, services);
  } else if (services.reset_to_new_theme) {
    services.reset_to_new_theme();
  }
}

void RebootOrConfirm(SettingsActionState* state,
                     const SettingsActionServices& services) {
  if (state->unsaved_changes) {
    BeginDiscard(state, services);
  } else if (services.return_to_firmware) {
    services.return_to_firmware(kRebootFirmwareRoutine);
  }
}

}  // namespace

void UpdateSettingsActions(SettingsActionState* state,
                           const SettingsActionInput& input,
                           const SettingsActionServices& services) {
  if (!state) return;

  for (std::uint32_t i = 0; i < kSettingsActionLabels.size(); ++i) {
    const bool selected = state->selected_action == i;
    const std::uint32_t color = state->view == 0
                                    ? kSelectedActionColor
                                    : kInactiveSelectedActionColor;
    if (services.draw_action) {
      services.draw_action(kSettingsActionLabels[i], selected, color);
    }
  }

  if (state->feedback_modal_pending || state->async_task_pending ||
      state->view != 0) {
    return;
  }

  if (input.clear_editor_change) state->editor_change_pending = false;
  if (input.next) {
    state->selected_action = (state->selected_action + 1) % 7;
  }
  if (input.previous) {
    state->selected_action = state->selected_action == 0
                                 ? 6
                                 : state->selected_action - 1;
  }
  if (!input.activate) return;

  switch (static_cast<SettingsAction>(state->selected_action)) {
    case SettingsAction::kNewTheme:
      ResetOrConfirm(state, services);
      break;
    case SettingsAction::kLoad:
      state->view = 1;
      break;
    case SettingsAction::kSave:
    case SettingsAction::kSaveAs:
      state->async_task_pending = true;
      if (services.start_save) {
        services.start_save(state->selected_action ==
                            static_cast<std::uint32_t>(
                                SettingsAction::kSaveAs));
      }
      break;
    case SettingsAction::kSetActive: {
      const bool success =
          services.set_as_active_theme && services.set_as_active_theme();
      state->feedback_modal_pending = true;
      if (services.show_feedback) {
        services.show_feedback(
            success ? "Settings updated successfully!"
                    : "Failed to update settings!",
            success ? "Test" : "Error", success);
      }
      if (success && services.show_success_notification) {
        services.show_success_notification(2.0f);
      }
      break;
    }
    case SettingsAction::kReboot:
      RebootOrConfirm(state, services);
      break;
    case SettingsAction::kExit:
      if (state->unsaved_changes) {
        BeginDiscard(state, services);
        break;
      }
      if (services.request_exit) services.request_exit();
      // The original re-reads the unsaved byte after the exit request. If the
      // request returns instead of terminating and sets it, confirmation is
      // started; otherwise HalReturnToFirmware(6) is its fallback.
      if (state->unsaved_changes) {
        BeginDiscard(state, services);
      } else if (services.return_to_firmware) {
        services.return_to_firmware(kRebootFirmwareRoutine);
      }
      break;
  }
}

}  // namespace bandwagon::boot_edit
