#include "modal_state.hpp"

namespace bandwagon::boot_edit {

void FinalizeModalState(ModalCompletionState* modal) {
  if (!modal) return;
  if (modal->release_runtime) modal->release_runtime();
  modal->completion_pending = false;
  if (modal->continuation) {
    modal->continuation(modal, modal->continuation_argument);
  }
}

void CompleteModalTextStorage(ModalCompletionState* modal) {
  if (!modal) return;
  modal->result = 0;
  std::string{}.swap(modal->text);
  modal->text_storage_released = true;
  modal->completion_pending = true;
}

}  // namespace bandwagon::boot_edit
