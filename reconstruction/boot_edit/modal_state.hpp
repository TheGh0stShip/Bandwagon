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
};

// 0x8204b998: release the modal's runtime-owned payload, clear its completion
// byte, then notify the optional continuation with the stored argument.
void FinalizeModalState(ModalCompletionState* modal);

// 0x8204bb60: reset result state, release the modal text's backing storage,
// and publish completion. The original leaf explicitly destroys heap-backed
// string storage rather than assigning an empty string.
void CompleteModalTextStorage(ModalCompletionState* modal);

}  // namespace bandwagon::boot_edit
