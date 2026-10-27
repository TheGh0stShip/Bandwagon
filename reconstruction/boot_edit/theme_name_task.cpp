#include "theme_name_task.hpp"

namespace bandwagon::boot_edit {
namespace {

constexpr std::string_view kKeyboardTitle = "Save Theme";
constexpr std::string_view kKeyboardPrompt = "Enter a name for the theme";
constexpr std::string_view kInvalidMessage =
    "The theme name entered is invalid, please enter a valid name.";
constexpr std::string_view kInvalidTitle = "Invalid theme name";
constexpr std::string_view kOverwriteTitle = "Overwrite existing theme";
constexpr std::string_view kKeyboardStartFailure =
    "PromptForFileNameTask::Update: XShowKeyboardUI failed 0x%08x\n";
constexpr std::string_view kKeyboardPollFailure =
    "PromptForFileNameTask::Update: XGetOverlappedResult failed 0x%08x\n";

void Complete(ThemeNameTaskState* state, std::uint32_t result) {
  state->result = result;
  state->completion_pending = true;
  if (state->continuation) state->continuation(state);
}

void Show(ThemeNameTaskState* state, ThemeNameDialog dialog,
          std::string_view message, std::string_view title,
          const ThemeNameTaskServices& services) {
  state->dialog = dialog;
  if (services.show_dialog) services.show_dialog(dialog, message, title);
}

bool HandleDialog(ThemeNameTaskState* state,
                  const ThemeNameTaskServices& services) {
  if (state->dialog == ThemeNameDialog::kNone) return false;
  const auto selected =
      services.poll_dialog ? services.poll_dialog(state->dialog) : std::nullopt;
  if (!selected) return true;
  const ThemeNameDialog dialog = state->dialog;
  state->dialog = ThemeNameDialog::kNone;
  if (dialog == ThemeNameDialog::kInvalidName) {
    if (*selected == 1) {
      state->keyboard_in_progress = false;
    } else {
      Complete(state, 1);
    }
  } else if (*selected == 2) {
    Complete(state, 3);
  } else if (*selected == 3) {
    state->keyboard_in_progress = false;
  } else {
    Complete(state, 1);
  }
  return true;
}

}  // namespace

ThemeNameTaskState InitializeThemeNameTask() { return {}; }

bool AdvanceThemeNameTask(ThemeNameTaskState* state,
                          std::string_view current_theme_name,
                          const ThemeNameTaskServices& services) {
  if (!state || state->completion_pending) return state && state->completion_pending;
  if (HandleDialog(state, services)) return state->completion_pending;

  if (!state->keyboard_in_progress) {
    const std::uint32_t status = services.start_keyboard
                                     ? services.start_keyboard(
                                           kKeyboardTitle, kKeyboardPrompt,
                                           current_theme_name)
                                     : 1;
    if (status != kAsyncPending) {
      if (services.debug_print) services.debug_print(kKeyboardStartFailure, status);
      Complete(state, 2);
      return true;
    }
    state->keyboard_in_progress = true;
    return false;
  }

  const KeyboardPollResult poll =
      services.poll_keyboard ? services.poll_keyboard()
                             : KeyboardPollResult{1, 0, {}};
  if (poll.status == kAsyncPending) return false;
  if (poll.status != 0) {
    if (services.debug_print) services.debug_print(kKeyboardPollFailure, poll.status);
    Complete(state, 2);
    return true;
  }
  if (poll.extended_error == kOperationCancelled) {
    Complete(state, 1);
    return true;
  }
  if (poll.extended_error != 0) {
    Complete(state, 2);
    return true;
  }

  if (poll.text.empty()) {
    Show(state, ThemeNameDialog::kInvalidName, kInvalidMessage, kInvalidTitle,
         services);
    return false;
  }
  state->name = services.narrow ? services.narrow(poll.text)
                                : std::string(poll.text.begin(), poll.text.end());
  state->path = "Themes:\\" + state->name + ".xbat";
  if (services.file_exists && services.file_exists(state->path)) {
    const std::string message = "A theme with the name '" + state->name +
                                "' already exists, do you want to overwrite it?";
    Show(state, ThemeNameDialog::kOverwrite, message, kOverwriteTitle, services);
    return false;
  }
  Complete(state, 3);
  return true;
}

bool RectContains(const Rect& outer, const Rect& inner) {
  return inner.minimum.x >= outer.minimum.x &&
         inner.minimum.y >= outer.minimum.y &&
         inner.maximum.x <= outer.maximum.x &&
         inner.maximum.y <= outer.maximum.y;
}

bool RectOverlaps(const Rect& first, const Rect& second) {
  return second.minimum.y < first.maximum.y &&
         second.maximum.y > first.minimum.y &&
         second.minimum.x < first.maximum.x &&
         second.maximum.x > first.minimum.x;
}

void CopyRectIfDifferent(Rect* destination, const Rect& source) {
  if (!destination) return;
  if (destination->minimum.x != source.minimum.x)
    destination->minimum.x = source.minimum.x;
  if (destination->minimum.y != source.minimum.y)
    destination->minimum.y = source.minimum.y;
  if (destination->maximum.x != source.maximum.x)
    destination->maximum.x = source.maximum.x;
  if (destination->maximum.y != source.maximum.y)
    destination->maximum.y = source.maximum.y;
}

void CopyRect(Rect* destination, const Rect& source) {
  if (destination) *destination = source;
}

void ClipRect(Rect* destination, const Rect& clip) {
  if (!destination) return;
  if (destination->minimum.x < clip.minimum.x)
    destination->minimum.x = clip.minimum.x;
  if (destination->minimum.y < clip.minimum.y)
    destination->minimum.y = clip.minimum.y;
  if (destination->maximum.x > clip.maximum.x)
    destination->maximum.x = clip.maximum.x;
  if (destination->maximum.y > clip.maximum.y)
    destination->maximum.y = clip.maximum.y;
}

}  // namespace bandwagon::boot_edit
