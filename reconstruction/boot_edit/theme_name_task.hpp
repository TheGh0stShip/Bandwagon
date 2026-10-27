#pragma once

#include "modal_state.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace bandwagon::boot_edit {

inline constexpr std::uint32_t kAsyncPending = 997;
inline constexpr std::uint32_t kOperationCancelled = 1223;

enum class ThemeNameDialog { kNone, kInvalidName, kOverwrite };

struct KeyboardPollResult {
  std::uint32_t status = kAsyncPending;
  std::uint32_t extended_error = 0;
  std::wstring text;
};

struct ThemeNameTaskState {
  bool completion_pending = false;
  std::uint32_t result = 0;
  bool keyboard_in_progress = false;
  ThemeNameDialog dialog = ThemeNameDialog::kNone;
  std::string path;
  std::string name;
  std::function<void(ThemeNameTaskState*)> continuation;
};

struct ThemeNameTaskServices {
  std::function<std::optional<std::uint32_t>(ThemeNameDialog)> poll_dialog;
  std::function<std::uint32_t(std::string_view, std::string_view,
                              std::string_view)>
      start_keyboard;
  std::function<KeyboardPollResult()> poll_keyboard;
  std::function<std::string(std::wstring_view)> narrow;
  std::function<bool(std::string_view)> file_exists;
  std::function<void(ThemeNameDialog, std::string_view, std::string_view)>
      show_dialog;
  std::function<void(std::string_view, std::uint32_t)> debug_print;
};

// Exact product-state initialization performed by fn_82011598 after its
// asynchronous-task base constructor: empty result/name buffers, no active
// keyboard or dialog, and no completion/continuation state.
ThemeNameTaskState InitializeThemeNameTask();

// Advances the asynchronous task at 0x820118e8 by one update. Returns true
// only when the target has published its completion state.
bool AdvanceThemeNameTask(ThemeNameTaskState* state,
                          std::string_view current_theme_name,
                          const ThemeNameTaskServices& services);

// Non-pdata inline helpers embedded after the task body in the same inventory
// extent. Vec4 is interpreted as a rectangle (min.x, min.y, max.x, max.y).
bool RectContains(const Rect& outer, const Rect& inner);
bool RectOverlaps(const Rect& first, const Rect& second);
void CopyRectIfDifferent(Rect* destination, const Rect& source);
void CopyRect(Rect* destination, const Rect& source);
void ClipRect(Rect* destination, const Rect& clip);

}  // namespace bandwagon::boot_edit
