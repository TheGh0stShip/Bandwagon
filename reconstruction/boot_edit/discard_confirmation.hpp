#pragma once

#include <cstdint>
#include <functional>

namespace bandwagon::boot_edit {

inline constexpr std::uint32_t kModalAccept = 2;
inline constexpr std::uint32_t kModalDecline = 3;
inline constexpr std::uint32_t kModalCancel = 4;

enum class DiscardPhase : std::uint32_t {
  kAskUnsaved = 0,
  kPromptForThemeName = 1,
  kSaveTheme = 2,
};

struct DiscardConfirmationState {
  bool completion_pending = false;
  std::uint32_t result = 0;
  DiscardPhase phase = DiscardPhase::kAskUnsaved;
  bool needs_theme_name = false;
  bool show_save_failure_confirmation = false;
  bool save_failure_recorded = false;
  bool unsaved_dialog_active = false;
  bool name_task_active = false;
  bool save_failure_dialog_active = false;
  std::function<void()> continuation;
};

enum class DiscardStepKind {
  kWaiting,
  kOpenedUnsavedDialog,
  kStartedNameTask,
  kOpenedSaveFailureDialog,
  kCompleted,
};

struct DiscardStep {
  DiscardStepKind kind = DiscardStepKind::kWaiting;
  const char* title = nullptr;
  const char* message = nullptr;
  std::uint32_t modal_type = 0;
};

void ApplyUnsavedChangesChoice(DiscardConfirmationState* state,
                               std::uint32_t modal_result);
void ApplySaveFailureChoice(DiscardConfirmationState* state,
                            std::uint32_t modal_result);

// Readable control-flow model of 0x8204c870. Child modal/task update and
// destruction are host boundaries represented by the active flags. The save
// attempt is supplied only when phase 2 is reached.
DiscardStep AdvanceDiscardConfirmation(DiscardConfirmationState* state,
                                       bool save_succeeded,
                                       bool restart_name_task_before_save);

}  // namespace bandwagon::boot_edit
