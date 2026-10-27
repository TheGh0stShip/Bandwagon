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

  ModalFrameServices frame;
  order.clear();
  modal = ModalCompletionState{};
  modal.result = 2;
  modal.position_unset = true;
  modal.render_content = [&] { order.push_back("render"); };
  modal.publish_state = [&] { order.push_back("publish"); };
  frame.request_initial_placement = [&] { order.push_back("place"); };
  frame.begin_popup = [&](std::uint32_t flags) {
    assert(flags == 0x12B);
    order.push_back("begin");
    return true;
  };
  frame.end_popup = [&] { order.push_back("end"); };
  frame.restore_focus = [&] { order.push_back("focus"); };
  assert(UpdateModalWindow(&modal, frame));
  assert((order == std::vector<std::string>{"place", "begin", "render",
                                             "publish", "end", "focus"}));

  order.clear();
  modal.result = 2;
  modal.position_unset = false;
  modal.render_content = [&] {
    order.push_back("render");
    modal.result = 0;
  };
  assert(!UpdateModalWindow(&modal, frame));
  assert((order == std::vector<std::string>{"begin", "render", "end"}));

  order.clear();
  modal.result = 3;
  modal.completion_pending = true;
  frame.begin_popup = [&](std::uint32_t flags) {
    assert(flags == kModalWindowFlags);
    order.push_back("begin-closed");
    modal.result = 0;  // Models state changed by the popup boundary.
    return false;
  };
  modal.publish_state = [&] { order.push_back("publish-four"); };
  assert(UpdateModalWindow(&modal, frame));
  assert(modal.result == 4 && !modal.completion_pending);
  assert((order ==
          std::vector<std::string>{"begin-closed", "publish-four"}));

  modal = ModalCompletionState{};
  assert(UpdateModalWindow(&modal, frame));
  assert(!UpdateModalWindow(nullptr, frame));
}
