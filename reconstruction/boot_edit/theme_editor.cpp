#include "theme_editor.hpp"

namespace bandwagon::boot_edit {

SavedTheme BuildSavedTheme(std::string_view file_name) {
  const std::size_t extension = file_name.rfind(".xbat");
  return {
      std::string(file_name.substr(0, extension)),
      "Themes:\\" + std::string(file_name),
  };
}

bool ApplyColorAction(ColorAction action, Color* edited,
                      const Color& undo_value, const Color& default_value,
                      ColorClipboard* clipboard) {
  if (!edited || !clipboard) return false;
  switch (action) {
    case ColorAction::kCopy:
      clipboard->value = *edited;
      clipboard->available = true;
      return false;
    case ColorAction::kPaste:
      if (!clipboard->available) return false;
      *edited = clipboard->value;
      return true;
    case ColorAction::kUndo:
      *edited = undo_value;
      return true;
    case ColorAction::kReset:
      *edited = default_value;
      return true;
    case ColorAction::kNone:
      return false;
  }
  return false;
}

}  // namespace bandwagon::boot_edit
