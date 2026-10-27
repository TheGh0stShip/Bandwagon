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

}  // namespace bandwagon::boot_edit
