#include "theme_editor.hpp"

#include <cassert>
#include <iostream>
#include <limits>

int main() {
  using bandwagon::boot_edit::ApplyColorAction;
  using bandwagon::boot_edit::BuildSavedTheme;
  using bandwagon::boot_edit::Color;
  using bandwagon::boot_edit::ColorAction;
  using bandwagon::boot_edit::ColorClipboard;

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
  assert(!bandwagon::boot_edit::CanUndoColor(edited, edited));
  assert(bandwagon::boot_edit::CanUndoColor(edited, undo));
  Color nan_color = edited;
  nan_color[2] = std::numeric_limits<float>::quiet_NaN();
  assert(!bandwagon::boot_edit::ColorsEqual(nan_color, nan_color));
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

  std::cout << "BootEdit theme-editor reconstruction tests passed\n";
}
