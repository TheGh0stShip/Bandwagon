#include "theme_editor.hpp"

namespace bandwagon::boot_edit {

void DrawNavigationIcon(const std::array<std::uintptr_t, 10>& textures,
                        std::size_t icon_index, const char* tooltip,
                        const NavigationIconServices& services) {
  if (icon_index >= textures.size()) return;
  const float font_size = services.font_size ? services.font_size() : 0.0f;
  const Vec2 cursor = services.cursor_screen_position
                          ? services.cursor_screen_position()
                          : Vec2{};
  const float top = cursor.y + font_size * 0.5f - 16.0f;
  if (services.add_image) {
    services.add_image(textures[icon_index], {cursor.x, top},
                       {cursor.x + 32.0f, top + 32.0f}, {0.0f, 0.0f},
                       {1.0f, 1.0f}, 0xffffffffu);
  }
  const float cursor_y = services.cursor_y ? services.cursor_y() : 0.0f;
  if (services.set_cursor_y) services.set_cursor_y(cursor_y + 34.0f);
  if (tooltip && services.set_tooltip) services.set_tooltip(tooltip);
}

void BeginBootEditFrame(FrameClock* clock,
                        const FrameLifecycleServices& services) {
  if (!clock) return;
  if (services.backend_new_frame) services.backend_new_frame();
  if (services.imgui_new_frame) services.imgui_new_frame();
  clock->previous_ticks = clock->current_ticks;
  if (services.query_milliseconds)
    clock->current_ticks = services.query_milliseconds();
  const std::uint32_t elapsed = clock->current_ticks - clock->previous_ticks;
  clock->delta_seconds = static_cast<float>(elapsed) * 0.001f;
}

void ShutdownBootEdit(const FrameLifecycleServices& services) {
  if (services.destroy_platform_windows) services.destroy_platform_windows();
  if (services.destroy_imgui_context) services.destroy_imgui_context();
  if (services.shutdown_input_backend) services.shutdown_input_backend();
  if (services.shutdown_renderer_backend) services.shutdown_renderer_backend();
}

SavedTheme BuildSavedTheme(std::string_view file_name) {
  const std::size_t extension = file_name.rfind(".xbat");
  return {
      "Themes:\\" + std::string(file_name),
      std::string(file_name.substr(0, extension)),
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

ThemeEditorFrameResult DrawThemeEditor(
    ThemeEditorState* state, const ThemeEditorInput& input,
    float boot_animation_seconds, ColorClipboard* clipboard,
    ThemeEditorServices* services) {
  ThemeEditorFrameResult result;
  if (!state || !clipboard || !services) return result;

  for (std::size_t index = 0; index < input.flag_changed.size(); ++index) {
    if (!input.flag_changed[index]) continue;
    const std::uint32_t mask = std::uint32_t{1} << index;
    if (input.flag_value[index])
      state->mesh_flags |= mask;
    else
      state->mesh_flags &= ~mask;
    state->dirty = true;
  }

  for (std::size_t index = 0; index < state->meshes.size(); ++index) {
    auto& mesh = state->meshes[index];
    const bool changed = EditMeshColors(mesh.enabled, &mesh.edited,
                                        mesh.defaults, mesh.undo,
                                        input.mesh_events[index], clipboard);
    if (!changed) continue;
    state->dirty = true;
    if (index < 11) mesh.live = mesh.edited;
    if (index == 11) services->ApplyXWallsTexture();
    if (index == 12) services->ApplyCoreSphereGlow();
  }

  // The marker is derived from the playback clock, clamped to the exact
  // 0..10 second range, and scaled by the recovered 0.1 constant.
  result.timeline_seconds = boot_animation_seconds;
  if (result.timeline_seconds < 0.0f) result.timeline_seconds = 0.0f;
  if (result.timeline_seconds > 10.0f) result.timeline_seconds = 10.0f;
  result.timeline_fraction = result.timeline_seconds * 0.1f;

  services->UpdateBootEditFrame();

  if (state->view == 0 && input.main_menu_pressed) {
    services->ReleaseEditorTask();
    services->StartThemeEnumeration();
    state->view = 1;
    state->selection = 0;
    state->refresh_pending = true;
    state->navigation_state = 0;
    state->root_state = 0;
    services->ConsumeMainMenuButton();
    result.returned_to_main_menu = true;
  }

  if (state->notice_present && state->notice_suppresses_input) {
    if (!state->notice_completion_byte) services->UpdateNotice();
    if (services->NoticeComplete()) {
      services->ReleaseNotice();
      state->notice_present = false;
    }
    state->notice_seconds -= state->delta_seconds;
    if (state->notice_seconds <= 0.0f) {
      state->notice_suppresses_input = false;
      if (state->notice_present) {
        services->ReleaseNotice();
        state->notice_present = false;
      }
    }
  }
  return result;
}

}  // namespace bandwagon::boot_edit
