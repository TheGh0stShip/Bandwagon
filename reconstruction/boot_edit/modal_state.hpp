#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace bandwagon::boot_edit {

struct ModalCompletionState {
  bool completion_pending = false;  // byte +4
  std::uint32_t result = 0;         // word +8 or +44 by modal type
  bool dialog_open = true;          // byte +304 in keyboard task
  std::uintptr_t continuation_argument = 0;
  std::function<void(ModalCompletionState*, std::uintptr_t)> continuation;
  std::string text;
  bool text_storage_released = false;
  std::function<void()> release_runtime;
  bool position_unset = false;
  std::function<void()> render_content;
  std::function<void()> publish_state;
};

struct ModalFrameServices {
  std::function<void()> request_initial_placement;
  std::function<bool(std::uint32_t window_flags)> begin_popup;
  std::function<void()> end_popup;
  std::function<void()> restore_focus;
};

inline constexpr std::uint32_t kModalWindowFlags = 0x12B;

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Rect {
  Vec2 minimum;
  Vec2 maximum;
};

struct ModalTextGeometry {
  Vec2 cursor;
  Vec2 text_size;
  Vec2 frame_padding;
  Vec2 text_alignment;
  float content_right = 0.0f;
};

struct ModalTextLayout {
  std::uint32_t item_id = 0;
  Vec2 item_size;
  Rect bounds;
  Rect inner_bounds;
  bool item_added = false;
  bool rendered = false;
};

struct ModalTextServices {
  std::function<std::uint32_t(std::string_view)> get_id;
  std::function<bool(const Rect&, std::uint32_t)> item_add;
  std::function<void(const Vec2&, float)> item_size;
  std::function<void(const Rect&, std::string_view, const Vec2&, const Vec2&,
                     const Rect&)>
      render_text_clipped;
};

// 0x8204b998: release the modal's runtime-owned payload, clear its completion
// byte, then notify the optional continuation with the stored argument.
void FinalizeModalState(ModalCompletionState* modal);

// 0x8204bb60: reset result state, release the modal text's backing storage,
// and publish completion. The original leaf explicitly destroys heap-backed
// string storage rather than assigning an empty string.
void CompleteModalTextStorage(ModalCompletionState* modal);

// 0x8204bbc0: position and render one modal frame, publish nonzero state after
// content rendering, convert a completed-but-closed zero state to state 4,
// and report whether the modal remains in a nonzero state.
bool UpdateModalWindow(ModalCompletionState* modal,
                       const ModalFrameServices& services);

// 0x8204b9e8: build a padded text item using ImGui's CalcItemSize rules,
// submit its bounding box, and render centered/clipped text only when ItemAdd
// accepts the item.
ModalTextLayout DrawModalText(std::string_view text, float requested_width,
                              const ModalTextGeometry& geometry,
                              const ModalTextServices& services);

}  // namespace bandwagon::boot_edit
