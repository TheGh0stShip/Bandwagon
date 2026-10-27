#include "discard_confirmation.hpp"

namespace bandwagon::boot_edit {
namespace {

constexpr const char* kUnsavedTitle = "Unsaved changes";
constexpr const char* kUnsavedMessage =
    "The current theme has unsaved changes, would you like to save them "
    "before continuing?";
constexpr const char* kSaveFailureTitle = "Error saving theme";
constexpr const char* kSaveFailureMessage =
    "An error occurred while saving the theme. If you continue all changes "
    "will be lost, are you sure you want to continue?";

void Complete(DiscardConfirmationState* state, std::uint32_t result,
              bool save_failure_recorded) {
  state->result = result;
  state->completion_pending = true;
  state->save_failure_recorded = save_failure_recorded;
  if (state->continuation) state->continuation();
}

}  // namespace

void InitializeDiscardConfirmation(DiscardConfirmationState* state,
                                   bool ask_unsaved_changes,
                                   bool needs_theme_name,
                                   bool show_save_failure_confirmation,
                                   const std::string& current_theme_name,
                                   const std::string& current_theme_path) {
  if (!state) return;

  *state = {};
  state->needs_theme_name = needs_theme_name;
  state->show_save_failure_confirmation =
      show_save_failure_confirmation;
  state->theme_name = current_theme_name;
  state->theme_path = current_theme_path;
  state->phase = ask_unsaved_changes
                     ? DiscardPhase::kAskUnsaved
                     : (needs_theme_name ? DiscardPhase::kPromptForThemeName
                                         : DiscardPhase::kSaveTheme);
}

void ApplyUnsavedChangesChoice(DiscardConfirmationState* state,
                               std::uint32_t modal_result) {
  if (!state) return;
  state->unsaved_dialog_active = false;
  if (modal_result == kModalAccept) {
    state->phase = state->needs_theme_name
                       ? DiscardPhase::kPromptForThemeName
                       : DiscardPhase::kSaveTheme;
  } else if (modal_result == kModalDecline) {
    Complete(state, 3, false);
  } else {
    Complete(state, 1, false);
  }
}

void ApplySaveFailureChoice(DiscardConfirmationState* state,
                            std::uint32_t modal_result) {
  if (!state) return;
  state->save_failure_dialog_active = false;
  Complete(state, modal_result == kModalAccept ? 3u : 1u, true);
}

DiscardStep AdvanceDiscardConfirmation(DiscardConfirmationState* state,
                                       bool save_succeeded,
                                       bool restart_name_task_before_save) {
  if (!state) return {};
  if (state->unsaved_dialog_active || state->name_task_active ||
      state->save_failure_dialog_active) {
    return {};
  }
  if (state->completion_pending) {
    return {DiscardStepKind::kCompleted, nullptr, nullptr, 0};
  }

  if (state->phase == DiscardPhase::kAskUnsaved) {
    state->unsaved_dialog_active = true;
    return {DiscardStepKind::kOpenedUnsavedDialog, kUnsavedTitle,
            kUnsavedMessage, 4};
  }
  if (state->phase == DiscardPhase::kPromptForThemeName) {
    state->name_task_active = true;
    return {DiscardStepKind::kStartedNameTask, nullptr, nullptr, 0};
  }

  if (restart_name_task_before_save) {
    state->phase = DiscardPhase::kPromptForThemeName;
    return {};
  }
  if (!save_succeeded && state->show_save_failure_confirmation) {
    state->save_failure_dialog_active = true;
    return {DiscardStepKind::kOpenedSaveFailureDialog, kSaveFailureTitle,
            kSaveFailureMessage, 3};
  }

  Complete(state, 3, !save_succeeded);
  return {DiscardStepKind::kCompleted, nullptr, nullptr, 0};
}

}  // namespace bandwagon::boot_edit
