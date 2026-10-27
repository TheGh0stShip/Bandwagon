#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace bandwagon::boot_edit {

struct SavedTheme {
  std::string path;
  std::string display_name;
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

inline constexpr std::array<std::string_view, 12> kMeshFlagLabels{
    "Sphere X",          "Text",          "Sphere Rings Final",
    "X Bullet",          "Sphere Rings Impact", "Sphere TM",
    "Outer Sphere",      "Inner Sphere",  "X Impact Glow",
    "Lens Flare",        "Sphere Back Glow", "Text Lighting"};

inline constexpr std::array<std::string_view, 15> kMeshColorLabels{
    "Complete Sphere", "Impacted Sphere", "X Lip", "X Wall",
    "Inner Sphere", "X Bullet", "Final Rings", "Xbox Text",
    "360 Text", "Impact Rings", "Sphere TM", "X Walls Texture",
    "Core Sphere Glow", "Impact Glow", "Background"};

struct NavigationLegendEntry {
  std::uint32_t icon_index;
  std::string_view label;
};

inline constexpr std::array<NavigationLegendEntry, 8> kNavigationLegend{{
    {0, "Select"}, {1, "Cancel"}, {2, "Color Options"}, {6, "Restart"},
    {7, "Rewind"}, {9, "Pause"}, {4, "Main Menu"}, {8, "Show/Hide UI"},
}};

struct ThemeEditorMesh {
  std::array<bool, 3> enabled{};
  std::array<Color, 3> edited{};
  std::array<Color, 3> defaults{};
  std::array<Color, 3> undo{};
  // The first eleven records are copied to the live renderer only after an
  // edit, matching the 48-byte memcpy in the recovered routine.
  std::array<Color, 3> live{};
};

struct ThemeEditorState {
  std::uint32_t mesh_flags = 0;
  std::array<ThemeEditorMesh, 15> meshes{};
  bool dirty = false;
  std::uint8_t view = 0;
  std::uint32_t selection = 0;
  bool refresh_pending = false;
  std::uint32_t navigation_state = 0;
  std::uint32_t root_state = 0;
  bool notice_present = false;
  bool notice_suppresses_input = false;
  bool notice_completion_byte = false;
  float notice_seconds = 0.0f;
  float delta_seconds = 0.0f;
};

struct ThemeEditorInput {
  std::array<bool, 12> flag_changed{};
  std::array<bool, 12> flag_value{};
  std::array<std::array<MeshColorEditEvent, 3>, 15> mesh_events{};
  bool main_menu_pressed = false;  // controller mask 0x10
};

struct ThemeEditorServices {
  virtual ~ThemeEditorServices() = default;
  virtual void ApplyXWallsTexture() = 0;
  virtual void ApplyCoreSphereGlow() = 0;
  virtual void UpdateBootEditFrame() = 0;
  virtual void ReleaseEditorTask() = 0;
  virtual void StartThemeEnumeration() = 0;
  virtual void ConsumeMainMenuButton() = 0;
  virtual void UpdateNotice() = 0;
  virtual bool NoticeComplete() = 0;
  virtual void ReleaseNotice() = 0;
};

struct ThemeEditorFrameResult {
  float timeline_seconds = 0.0f;
  float timeline_fraction = 0.0f;
  bool returned_to_main_menu = false;
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

// Complete portable state/control-flow model of BootEdit 0x820512E8.
// Window geometry and draw calls are dependency boundaries, while every
// product-owned edit, callback, navigation transition, timeline clamp, and
// timed-notice mutation is represented here.
ThemeEditorFrameResult DrawThemeEditor(
    ThemeEditorState* state, const ThemeEditorInput& input,
    float boot_animation_seconds, ColorClipboard* clipboard,
    ThemeEditorServices* services);

}  // namespace bandwagon::boot_edit
