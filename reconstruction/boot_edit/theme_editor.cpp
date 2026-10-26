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

bool ColorsDifferExact(const Color& left, const Color& right) {
  // BootEdit's leaf at 0x82050A00 returns one on the first unordered or
  // unequal component and zero only after all four ordered comparisons match.
  return left[0] != right[0] || left[1] != right[1] ||
         left[2] != right[2] || left[3] != right[3];
}

bool CanUndoColor(const Color& edited, const Color& undo_value) {
  return ColorsDifferExact(edited, undo_value);
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

bool DrawColorContextMenu(const ColorContextMenuEvents& events, Color* edited,
                          const Color& default_value,
                          const Color& undo_value,
                          ColorClipboard* clipboard) {
  if (!events.popup_open || !edited || !clipboard) return false;

  bool changed = false;
  if (events.copy_selected) {
    clipboard->value = *edited;
    clipboard->available = true;
  }
  if (events.paste_selected && clipboard->available) {
    *edited = clipboard->value;
    changed = true;
  }
  if (events.undo_selected && CanUndoColor(*edited, undo_value)) {
    *edited = undo_value;
    changed = true;
  }
  if (events.reset_selected) {
    *edited = default_value;
    changed = true;
  }
  return changed;
}

bool EditMeshColors(const std::array<bool, 3>& enabled,
                    std::array<Color, 3>* edited,
                    const std::array<Color, 3>& default_values,
                    const std::array<Color, 3>& undo_values,
                    const std::array<MeshColorEditEvent, 3>& events,
                    ColorClipboard* clipboard) {
  if (!edited || !clipboard) return false;

  bool changed = false;
  for (std::size_t index = 0; index < enabled.size(); ++index) {
    if (!enabled[index]) continue;
    if (events[index].widget_changed) {
      (*edited)[index] = events[index].widget_value;
      changed = true;
    }
    changed = DrawColorContextMenu(
                  events[index].context_menu, &(*edited)[index],
                  default_values[index], undo_values[index], clipboard) ||
              changed;
  }
  return changed;
}

}  // namespace bandwagon::boot_edit
