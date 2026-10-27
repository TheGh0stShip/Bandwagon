#include "discard_confirmation.hpp"

#include <cassert>
#include <string_view>

using namespace bandwagon::boot_edit;

int main() {
  DiscardConfirmationState state;
  InitializeDiscardConfirmation(&state, true, false, true, "Carbon",
                                "Themes:\\Carbon.xbat");
  assert(state.phase == DiscardPhase::kAskUnsaved);
  assert(!state.needs_theme_name && state.show_save_failure_confirmation);
  assert(state.theme_name == "Carbon");
  assert(state.theme_path == "Themes:\\Carbon.xbat");
  assert(state.child_dialog == 0 && state.child_name_task == 0);
  assert(!state.completion_pending && !state.completion_observed);

  InitializeDiscardConfirmation(&state, false, true, false, "", "");
  assert(state.phase == DiscardPhase::kPromptForThemeName);
  InitializeDiscardConfirmation(&state, false, false, false, "Stock",
                                "Themes:\\Stock.xbat");
  assert(state.phase == DiscardPhase::kSaveTheme);

  InitializeDiscardConfirmation(&state, true, false, false, "", "");
  DiscardStep step = AdvanceDiscardConfirmation(&state, false, false);
  assert(step.kind == DiscardStepKind::kOpenedUnsavedDialog);
  assert(step.modal_type == 4);
  assert(std::string_view(step.title) == "Unsaved changes");
  assert(std::string_view(step.message).find("unsaved changes") !=
         std::string_view::npos);

  state.needs_theme_name = true;
  ApplyUnsavedChangesChoice(&state, kModalAccept);
  assert(state.phase == DiscardPhase::kPromptForThemeName);
  step = AdvanceDiscardConfirmation(&state, false, false);
  assert(step.kind == DiscardStepKind::kStartedNameTask);

  state = {};
  int continuations = 0;
  state.continuation = [&] { ++continuations; };
  ApplyUnsavedChangesChoice(&state, kModalDecline);
  assert(state.result == 3 && state.completion_pending);
  assert(continuations == 1);
  state = {};
  state.continuation = [&] { ++continuations; };
  ApplyUnsavedChangesChoice(&state, kModalCancel);
  assert(state.result == 1 && state.completion_pending);
  assert(continuations == 2);

  state = {};
  state.phase = DiscardPhase::kSaveTheme;
  state.show_save_failure_confirmation = true;
  step = AdvanceDiscardConfirmation(&state, false, false);
  assert(step.kind == DiscardStepKind::kOpenedSaveFailureDialog);
  assert(step.modal_type == 3);
  assert(std::string_view(step.title) == "Error saving theme");
  ApplySaveFailureChoice(&state, kModalAccept);
  assert(state.result == 3 && state.completion_pending);
  assert(state.save_failure_recorded);

  state = {};
  state.phase = DiscardPhase::kSaveTheme;
  state.show_save_failure_confirmation = true;
  ApplySaveFailureChoice(&state, kModalDecline);
  assert(state.result == 1 && state.save_failure_recorded);

  state = {};
  state.phase = DiscardPhase::kSaveTheme;
  step = AdvanceDiscardConfirmation(&state, true, false);
  assert(step.kind == DiscardStepKind::kCompleted);
  assert(state.result == 3 && !state.save_failure_recorded);

  state = {};
  state.phase = DiscardPhase::kSaveTheme;
  step = AdvanceDiscardConfirmation(&state, false, false);
  assert(step.kind == DiscardStepKind::kCompleted);
  assert(state.result == 3 && state.save_failure_recorded);

  state = {};
  state.phase = DiscardPhase::kSaveTheme;
  step = AdvanceDiscardConfirmation(&state, false, true);
  assert(step.kind == DiscardStepKind::kWaiting);
  assert(state.phase == DiscardPhase::kPromptForThemeName);

  state = {};
  state.name_task_active = true;
  assert(AdvanceDiscardConfirmation(&state, true, false).kind ==
         DiscardStepKind::kWaiting);
  assert(AdvanceDiscardConfirmation(nullptr, true, false).kind ==
         DiscardStepKind::kWaiting);
}
