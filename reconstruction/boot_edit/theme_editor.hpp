#pragma once

#include <array>
#include <cstdint>
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

// Input-side model of the four ImGui menu items emitted by
// DrawColorContextMenu.  The booleans are deliberately kept separate: the
// original routine evaluates the items in this order and a Copy can therefore
// make Paste available later in the same pass.
struct ColorContextMenuEvents {
  bool popup_open = true;
  bool copy_selected = false;
  bool paste_selected = false;
  bool undo_selected = false;
  bool reset_selected = false;
};

struct MeshColorEditEvent {
  bool widget_changed = false;
  Color widget_value{};
  ColorContextMenuEvents context_menu{};
};

bool ColorsEqual(const Color& left, const Color& right);
bool ColorsDifferExact(const Color& left, const Color& right);
bool CanUndoColor(const Color& edited, const Color& undo_value);

std::uint32_t NextMeshIndex(std::uint32_t index);
std::uint32_t PreviousMeshIndex(std::uint32_t index);

// Reconstructs the non-ImGui state changes in the color context menu. Returns
// true exactly for actions that replace the edited color.
bool ApplyColorAction(ColorAction action, Color* edited,
                      const Color& undo_value, const Color& default_value,
                      ColorClipboard* clipboard);

// Portable state/control-flow model of BootEdit 0x82050B08.  ImGui's popup
// and MenuItem calls are represented by events; the color mutations, enable
// rules, shared clipboard, ordering, and return value match the recovered
// routine.
bool DrawColorContextMenu(const ColorContextMenuEvents& events, Color* edited,
                          const Color& default_value,
                          const Color& undo_value,
                          ColorClipboard* clipboard);

// Portable non-ImGui model of BootEdit 0x82050D80.  The original walks the
// diffuse/specular/ambient colors in that order, skips disabled rows, and ORs
// both widget and context-menu changes into its result.
bool EditMeshColors(const std::array<bool, 3>& enabled,
                    std::array<Color, 3>* edited,
                    const std::array<Color, 3>& default_values,
                    const std::array<Color, 3>& undo_values,
                    const std::array<MeshColorEditEvent, 3>& events,
                    ColorClipboard* clipboard);

}  // namespace bandwagon::boot_edit
