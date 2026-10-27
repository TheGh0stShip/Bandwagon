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
