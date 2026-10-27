#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace bandwagon::boot_edit {

inline constexpr std::array<std::string_view, 7> kSettingsActionLabels = {{
    "New Theme", "Load", "Save", "Save As", "Set As Active Theme",
    "Reboot", "Exit",
}};
inline constexpr std::uint32_t kSelectedActionColor = 0x9600FF00;
inline constexpr std::uint32_t kInactiveSelectedActionColor = 0x96505050;
inline constexpr std::uint32_t kRebootFirmwareRoutine = 6;

enum class DeferredAction : std::uint32_t {
  kNone = 0,
  kResetTheme = 1,
  kLoadTheme = 2,
  kNoOperation = 3,
  kReturnToFirmware = 4,
};

inline constexpr std::uint32_t kAcceptedDiscardResult = 3;

struct DeferredActionCompletion {
  std::uint32_t result = 0;
  DeferredAction action = DeferredAction::kNone;
};

struct DeferredActionTaskState {
  bool pending = false;
  DeferredAction action = DeferredAction::kNone;
};

struct DeferredActionServices {
  std::function<void()> acquire_application_state;
  std::function<void()> reset_to_new_theme;
  std::function<void()> load_selected_theme;
  std::function<void()> prepare_firmware_return;
  std::function<void(std::uint32_t)> return_to_firmware;
  std::function<bool(std::uint32_t modal_type, bool first_option,
                     bool second_option, DeferredAction action)>
      create_discard_confirmation;
};

// Exact product control-flow models for 0x82051F28, 0x82051F98, and
// 0x82051FE8 respectively. Modal allocation and Xbox firmware calls are
// explicit dependency boundaries.
void DispatchDeferredAction(DeferredAction action,
                            const DeferredActionServices& services);
void CompleteDeferredAction(const DeferredActionCompletion& completion,
                            const DeferredActionServices& services);
bool StartDeferredActionTask(DeferredActionTaskState* state,
                             DeferredAction action,
                             const DeferredActionServices& services);

enum class SettingsAction : std::uint32_t {
  kNewTheme = 0,
  kLoad = 1,
  kSave = 2,
  kSaveAs = 3,
  kSetActive = 4,
  kReboot = 5,
  kExit = 6,
};

struct SettingsActionInput {
  bool clear_editor_change = false;  // controller mask 0x2000
  bool next = false;                 // controller mask 0x0002
  bool previous = false;             // controller mask 0x0001
  bool activate = false;             // controller mask 0x1000
};

struct SettingsActionState {
  std::uint32_t view = 0;
  bool editor_change_pending = false;
  std::uint32_t selected_action = 0;
  bool feedback_modal_pending = false;
  bool async_task_pending = false;
  bool unsaved_changes = false;
};

struct SettingsActionServices {
  std::function<void(std::string_view, bool, std::uint32_t)> draw_action;
  std::function<void()> reset_to_new_theme;
  std::function<void(std::uint32_t)> start_discard_confirmation;
  std::function<void(bool)> start_save;
  std::function<bool()> set_as_active_theme;
  std::function<void(std::string_view, std::string_view, bool)>
      show_feedback;
  std::function<void(float)> show_success_notification;
  std::function<void()> request_exit;
  std::function<void(std::uint32_t)> return_to_firmware;
};

// Readable behavioral model of the complete 0x82052060 inventory extent.
void UpdateSettingsActions(SettingsActionState* state,
                           const SettingsActionInput& input,
                           const SettingsActionServices& services);

}  // namespace bandwagon::boot_edit
