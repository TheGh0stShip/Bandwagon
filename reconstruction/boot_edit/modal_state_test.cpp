#include "modal_state.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

namespace {
bool Near(float left, float right) {
  return std::fabs(left - right) < 0.0001f;
}
}  // namespace

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

  ModalTextGeometry geometry{{10.0f, 20.0f}, {30.0f, 12.0f},
                             {4.0f, 3.0f}, {0.5f, 0.5f}, 100.0f};
  ModalTextServices text_services;
  order.clear();
  text_services.get_id = [&](std::string_view text) {
    assert(text == "Question");
    order.push_back("id");
    return 0x1234;
  };
  text_services.item_size = [&](const Vec2& size, float baseline) {
    assert(Near(size.x, 38.0f) && Near(size.y, 18.0f));
    assert(Near(baseline, 3.0f));
    order.push_back("size");
  };
  text_services.item_add = [&](const Rect& bounds, std::uint32_t id) {
    assert(id == 0x1234);
    assert(Near(bounds.maximum.x, 48.0f));
    assert(Near(bounds.maximum.y, 38.0f));
    order.push_back("add");
    return true;
  };
  text_services.render_text_clipped =
      [&](const Rect& inner, std::string_view text, const Vec2& text_size,
          const Vec2& alignment, const Rect& clip) {
        assert(text == "Question");
        assert(Near(inner.minimum.x, 14.0f));
        assert(Near(inner.minimum.y, 23.0f));
        assert(Near(inner.maximum.x, 44.0f));
        assert(Near(inner.maximum.y, 35.0f));
        assert(Near(text_size.x, 30.0f) && Near(text_size.y, 12.0f));
        assert(Near(alignment.x, 0.5f) && Near(alignment.y, 0.5f));
        assert(Near(clip.minimum.x, 10.0f) && Near(clip.maximum.x, 48.0f));
        order.push_back("render");
      };
  ModalTextLayout text_layout =
      DrawModalText("Question", 0.0f, geometry, text_services);
  assert(text_layout.item_added && text_layout.rendered);
  assert((order == std::vector<std::string>{"id", "size", "add", "render"}));

  order.clear();
  text_services.item_size = [&](const Vec2& size, float baseline) {
    assert(Near(size.x, 50.0f) && Near(size.y, 18.0f));
    assert(Near(baseline, 3.0f));
    order.push_back("size");
  };
  text_services.item_add = [&](const Rect&, std::uint32_t) {
    order.push_back("add-hidden");
    return false;
  };
  text_layout = DrawModalText("Question", 50.0f, geometry, text_services);
  assert(Near(text_layout.item_size.x, 50.0f));
  assert(!text_layout.item_added && !text_layout.rendered);
  assert((order == std::vector<std::string>{"id", "size", "add-hidden"}));

  text_services.item_size = {};
  text_services.item_add = {};
  text_layout = DrawModalText("Question", -5.0f, geometry, text_services);
  assert(Near(text_layout.item_size.x, 85.0f));
}
