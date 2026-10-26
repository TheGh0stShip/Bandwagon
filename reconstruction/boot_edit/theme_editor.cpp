#include "theme_editor.hpp"

namespace bandwagon::boot_edit {

SavedTheme BuildSavedTheme(std::string_view file_name) {
  const std::size_t extension = file_name.rfind(".xbat");
  return {
      std::string(file_name.substr(0, extension)),
      "Themes:\\" + std::string(file_name),
  };
}

bool ColorsEqual(const Color& left, const Color& right) {
  return left[0] == right[0] && left[1] == right[1] &&
         left[2] == right[2] && left[3] == right[3];
}

bool CanUndoColor(const Color& edited, const Color& undo_value) {
  return !ColorsEqual(edited, undo_value);
}

std::uint32_t NextMeshIndex(std::uint32_t index) {
  return (index + 1u) % 7u;
}

std::uint32_t PreviousMeshIndex(std::uint32_t index) {
  return index == 0 ? 6u : index - 1u;
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
