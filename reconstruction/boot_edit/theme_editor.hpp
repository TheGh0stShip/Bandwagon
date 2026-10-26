#pragma once

#include <array>
#include <string>
#include <string_view>

namespace bandwagon::boot_edit {

struct SavedTheme {
  std::string display_name;
  std::string path;
};

// Models the string transformation applied to each Themes:\*.xbat result.
SavedTheme BuildSavedTheme(std::string_view file_name);

using Color = std::array<float, 4>;

struct ColorClipboard {
  bool available = false;
  Color value{};
};

enum class ColorAction { kNone, kCopy, kPaste, kUndo, kReset };

// Reconstructs the non-ImGui state changes in the color context menu. Returns
// true exactly for actions that replace the edited color.
bool ApplyColorAction(ColorAction action, Color* edited,
                      const Color& undo_value, const Color& default_value,
                      ColorClipboard* clipboard);

}  // namespace bandwagon::boot_edit
