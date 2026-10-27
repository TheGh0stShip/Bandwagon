#include "theme_editor.hpp"

#include <cassert>
#include <iostream>
#include <limits>

namespace {
struct Services final : bandwagon::boot_edit::ThemeEditorServices {
  int walls = 0, core = 0, frame = 0, task = 0, enumerate = 0, consume = 0;
  int notice_updates = 0, notice_releases = 0;
  bool notice_complete = false;
  void ApplyXWallsTexture() override { ++walls; }
  void ApplyCoreSphereGlow() override { ++core; }
  void UpdateBootEditFrame() override { ++frame; }
  void ReleaseEditorTask() override { ++task; }
  void StartThemeEnumeration() override { ++enumerate; }
  void ConsumeMainMenuButton() override { ++consume; }
  void UpdateNotice() override { ++notice_updates; }
  bool NoticeComplete() override { return notice_complete; }
  void ReleaseNotice() override { ++notice_releases; }
};
}  // namespace

int main() {
  using bandwagon::boot_edit::ApplyColorAction;
  using bandwagon::boot_edit::BuildSavedTheme;
  using bandwagon::boot_edit::Color;
  using bandwagon::boot_edit::ColorAction;
  using bandwagon::boot_edit::ColorClipboard;
  using bandwagon::boot_edit::ColorContextMenuEvents;
  using bandwagon::boot_edit::MeshColorEditEvent;

  const auto theme = BuildSavedTheme("Blue.xbat");
  assert(theme.display_name == "Blue");
  assert(theme.path == "Themes:\\Blue.xbat");
  const auto no_extension = BuildSavedTheme("RawTheme");
  assert(no_extension.display_name == "RawTheme");
  assert(no_extension.path == "Themes:\\RawTheme");
  // rfind is used rather than a suffix check.
  assert(BuildSavedTheme("Odd.xbat.backup").display_name == "Odd");

  for (std::uint32_t index = 0; index < 7; ++index) {
    assert(bandwagon::boot_edit::NextMeshIndex(index) == (index + 1) % 7);
    assert(bandwagon::boot_edit::PreviousMeshIndex(index) ==
           (index == 0 ? 6u : index - 1));
  }

  Color edited{1.0f, 0.5f, 0.25f, 1.0f};
  const Color undo{0.1f, 0.2f, 0.3f, 0.4f};
  const Color defaults{0.9f, 0.8f, 0.7f, 0.6f};
  ColorClipboard clipboard;
  assert(bandwagon::boot_edit::ColorsEqual(edited, edited));
  assert(!bandwagon::boot_edit::ColorsDifferExact(edited, edited));
  assert(!bandwagon::boot_edit::CanUndoColor(edited, edited));
  assert(bandwagon::boot_edit::CanUndoColor(edited, undo));
  Color nan_color = edited;
  nan_color[2] = std::numeric_limits<float>::quiet_NaN();
  assert(!bandwagon::boot_edit::ColorsEqual(nan_color, nan_color));
  assert(bandwagon::boot_edit::ColorsDifferExact(nan_color, nan_color));
  assert(bandwagon::boot_edit::CanUndoColor(nan_color, nan_color));
  assert(!ApplyColorAction(ColorAction::kPaste, &edited, undo, defaults,
                           &clipboard));
  assert(!ApplyColorAction(ColorAction::kCopy, &edited, undo, defaults,
                           &clipboard));
  assert(clipboard.available && clipboard.value == edited);
  edited = {};
  assert(ApplyColorAction(ColorAction::kPaste, &edited, undo, defaults,
                          &clipboard));
  assert(edited == clipboard.value);
  assert(ApplyColorAction(ColorAction::kUndo, &edited, undo, defaults,
                          &clipboard));
  assert(edited == undo);
  assert(ApplyColorAction(ColorAction::kReset, &edited, undo, defaults,
                          &clipboard));
  assert(edited == defaults);
  assert(!ApplyColorAction(ColorAction::kNone, &edited, undo, defaults,
                           &clipboard));
  assert(!ApplyColorAction(ColorAction::kReset, nullptr, undo, defaults,
                           &clipboard));

  edited = {1.0f, 0.5f, 0.25f, 1.0f};
  clipboard = {};
  ColorContextMenuEvents context_events;
  context_events.copy_selected = true;
  context_events.paste_selected = true;
  assert(bandwagon::boot_edit::DrawColorContextMenu(
      context_events, &edited, defaults, undo, &clipboard));
  assert(clipboard.available && edited == clipboard.value);
  context_events = {};
  context_events.popup_open = false;
  context_events.reset_selected = true;
  assert(!bandwagon::boot_edit::DrawColorContextMenu(
      context_events, &edited, defaults, undo, &clipboard));

  std::array<Color, 3> mesh_colors{
      Color{1.0f, 0.0f, 0.0f, 1.0f}, Color{0.0f, 1.0f, 0.0f, 1.0f},
      Color{0.0f, 0.0f, 1.0f, 1.0f}};
  const std::array<Color, 3> mesh_defaults{
      defaults, defaults, defaults};
  const std::array<Color, 3> mesh_undo{undo, undo, undo};
  std::array<MeshColorEditEvent, 3> mesh_events{};
  mesh_events[0].widget_changed = true;
  mesh_events[0].widget_value = {0.2f, 0.3f, 0.4f, 1.0f};
  mesh_events[1].context_menu.reset_selected = true;
  mesh_events[2].context_menu.reset_selected = true;
  assert(bandwagon::boot_edit::EditMeshColors(
      {true, true, false}, &mesh_colors, mesh_defaults, mesh_undo,
      mesh_events, &clipboard));
  assert(mesh_colors[0] == mesh_events[0].widget_value);
  assert(mesh_colors[1] == defaults);
  assert(mesh_colors[2] == Color({0.0f, 0.0f, 1.0f, 1.0f}));

  using bandwagon::boot_edit::DrawThemeEditor;
  using bandwagon::boot_edit::ThemeEditorInput;
  using bandwagon::boot_edit::ThemeEditorState;
  assert(bandwagon::boot_edit::kMeshFlagLabels.size() == 12);
  assert(bandwagon::boot_edit::kMeshColorLabels.size() == 15);
  assert(bandwagon::boot_edit::kMeshFlagLabels[11] == "Text Lighting");
  assert(bandwagon::boot_edit::kMeshColorLabels[11] == "X Walls Texture");
  assert(bandwagon::boot_edit::kNavigationLegend.size() == 8);
  assert(bandwagon::boot_edit::kNavigationLegend[3].icon_index == 6);
  assert(bandwagon::boot_edit::kNavigationLegend[6].label == "Main Menu");

  ThemeEditorState editor;
  for (auto& mesh : editor.meshes) mesh.enabled = {true, true, true};
  ThemeEditorInput editor_input;
  editor_input.flag_changed[0] = true;
  editor_input.flag_value[0] = true;
  editor_input.flag_changed[4] = true;
  editor_input.flag_value[4] = true;
  editor_input.mesh_events[0][0].widget_changed = true;
  editor_input.mesh_events[0][0].widget_value = {1, 2, 3, 4};
  editor_input.mesh_events[11][1].widget_changed = true;
  editor_input.mesh_events[11][1].widget_value = {4, 3, 2, 1};
  editor_input.mesh_events[12][2].widget_changed = true;
  editor_input.mesh_events[12][2].widget_value = {5, 6, 7, 8};
  editor_input.main_menu_pressed = true;
  editor.notice_present = true;
  editor.notice_suppresses_input = true;
  editor.notice_seconds = 2.0f;
  editor.delta_seconds = 0.25f;
  Services services;
  const auto frame = DrawThemeEditor(&editor, editor_input, 12.0f,
                                     &clipboard, &services);
  assert(editor.mesh_flags == ((1u << 0) | (1u << 4)));
  assert(editor.dirty && editor.meshes[0].live[0] == Color({1, 2, 3, 4}));
  assert(services.walls == 1 && services.core == 1 && services.frame == 1);
  assert(frame.timeline_seconds == 10.0f && frame.timeline_fraction == 1.0f);
  assert(frame.returned_to_main_menu);
  assert(editor.view == 1 && editor.selection == 0 && editor.refresh_pending);
  assert(services.task == 1 && services.enumerate == 1 && services.consume == 1);
  assert(services.notice_updates == 1 && services.notice_releases == 0);
  assert(editor.notice_seconds == 1.75f);

  editor_input = {};
  editor.notice_seconds = 0.1f;
  editor.delta_seconds = 0.2f;
  const auto negative = DrawThemeEditor(&editor, editor_input, -3.0f,
                                        &clipboard, &services);
  assert(negative.timeline_seconds == 0.0f && negative.timeline_fraction == 0.0f);
  assert(!editor.notice_present && !editor.notice_suppresses_input);
  assert(services.notice_releases == 1);

  editor.notice_present = true;
  editor.notice_suppresses_input = true;
  editor.notice_completion_byte = true;
  editor.notice_seconds = 1.0f;
  services.notice_complete = true;
  const int prior_updates = services.notice_updates;
  DrawThemeEditor(&editor, {}, 5.0f, &clipboard, &services);
  assert(services.notice_updates == prior_updates);
  assert(services.notice_releases == 2 && !editor.notice_present);

  std::cout << "BootEdit theme-editor reconstruction tests passed\n";
}
