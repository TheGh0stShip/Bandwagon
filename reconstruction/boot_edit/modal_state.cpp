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

bool UpdateModalWindow(ModalCompletionState* modal,
                       const ModalFrameServices& services) {
  if (!modal) return false;
  if (modal->result == 0) return true;

  bool restore_focus = false;
  if (modal->position_unset) {
    if (services.request_initial_placement) services.request_initial_placement();
    restore_focus = true;
  }

  const bool began =
      services.begin_popup ? services.begin_popup(kModalWindowFlags) : false;
  if (began) {
    if (modal->render_content) modal->render_content();
    if (modal->result != 0 && modal->publish_state) modal->publish_state();
    if (services.end_popup) services.end_popup();
  } else if (modal->completion_pending && modal->result == 0) {
    modal->result = 4;
    modal->completion_pending = false;
    if (modal->publish_state) modal->publish_state();
  }

  if (restore_focus && services.restore_focus) services.restore_focus();
  return modal->result != 0;
}

}  // namespace bandwagon::boot_edit
