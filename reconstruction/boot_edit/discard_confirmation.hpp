#pragma once

#include <cstdint>
#include <functional>
#include <string>

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
  // The target snapshots application strings at offsets 48 and 76 when the
  // task is constructed. They are the current theme name and path consumed
  // by the later naming/save phases.
  std::string theme_name;
  std::string theme_path;
  std::uintptr_t child_dialog = 0;
  std::uintptr_t child_name_task = 0;
  bool completion_observed = false;
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

// Readable construction model of 0x8204c790. The target initializes its
// completion base first, stores the three policy bytes, clears both child
// pointers, snapshots the current theme strings, and selects the first phase
// with ask-unsaved taking precedence over needs-name.
void InitializeDiscardConfirmation(DiscardConfirmationState* state,
                                   bool ask_unsaved_changes,
                                   bool needs_theme_name,
                                   bool show_save_failure_confirmation,
                                   const std::string& current_theme_name,
                                   const std::string& current_theme_path);

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
