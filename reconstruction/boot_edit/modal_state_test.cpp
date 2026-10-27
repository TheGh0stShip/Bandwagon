#include "modal_state.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  ModalCompletionState modal;
  modal.completion_pending = true;
  modal.continuation_argument = 0x55;
  std::vector<std::string> order;
  modal.release_runtime = [&] { order.push_back("release"); };
  modal.continuation = [&](ModalCompletionState* self,
                           std::uintptr_t argument) {
    assert(self == &modal);
    assert(argument == 0x55);
    assert(!self->completion_pending);
    order.push_back("continue");
  };
  FinalizeModalState(&modal);
  assert((order == std::vector<std::string>{"release", "continue"}));

  ModalCompletionState without_continuation;
  without_continuation.completion_pending = true;
  FinalizeModalState(&without_continuation);
  assert(!without_continuation.completion_pending);
  FinalizeModalState(nullptr);

  modal.result = 4;
  modal.completion_pending = false;
  modal.text = std::string(80, 'x');
  CompleteModalTextStorage(&modal);
  assert(modal.result == 0);
  assert(modal.text.empty());
  assert(modal.text_storage_released);
  assert(modal.completion_pending);

  CompleteModalTextStorage(nullptr);
}
