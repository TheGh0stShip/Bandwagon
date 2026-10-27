#include "modal_state.hpp"

#include <algorithm>

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

ModalTextLayout DrawModalText(std::string_view text, float requested_width,
                              const ModalTextGeometry& geometry,
                              const ModalTextServices& services) {
  ModalTextLayout layout;
  if (services.get_id) layout.item_id = services.get_id(text);

  const float default_width =
      geometry.text_size.x + geometry.frame_padding.x * 2.0f;
  const float default_height =
      geometry.text_size.y + geometry.frame_padding.y * 2.0f;
  if (requested_width < 0.0f) {
    layout.item_size.x = std::max(
        4.0f, geometry.content_right - geometry.cursor.x + requested_width);
  } else if (requested_width == 0.0f) {
    layout.item_size.x = default_width;
  } else {
    layout.item_size.x = requested_width;
  }
  layout.item_size.y = default_height;
  layout.bounds = {geometry.cursor,
                   {geometry.cursor.x + layout.item_size.x,
                    geometry.cursor.y + layout.item_size.y}};

  if (services.item_size) {
    services.item_size(layout.item_size, geometry.frame_padding.y);
  }
  layout.item_added =
      services.item_add ? services.item_add(layout.bounds, layout.item_id)
                        : false;
  if (!layout.item_added) return layout;

  layout.inner_bounds = {
      {layout.bounds.minimum.x + geometry.frame_padding.x,
       layout.bounds.minimum.y + geometry.frame_padding.y},
      {layout.bounds.maximum.x - geometry.frame_padding.x,
       layout.bounds.maximum.y - geometry.frame_padding.y}};
  if (services.render_text_clipped) {
    services.render_text_clipped(layout.inner_bounds, text, geometry.text_size,
                                 geometry.text_alignment, layout.bounds);
  }
  layout.rendered = true;
  return layout;
}

ModalActionFrame DrawModalActions(ModalCompletionState* modal,
                                  const ModalActionInput& input) {
  ModalActionFrame frame;
  if (!modal) return frame;
  frame.message_rendered = modal->message_visible;

  switch (modal->modal_type) {
    case 1:
      frame.labels = {"Ok"};
      break;
    case 2:
      frame.labels = {"Ok", "Cancel"};
      break;
    case 3:
      frame.labels = {"Yes", "No"};
      break;
    case 4:
      frame.labels = {"Yes", "No", "Cancel"};
      break;
    default:
      frame.selected_action = modal->selected_action;
      frame.result = modal->result;
      return frame;
  }

  const std::uint32_t count = static_cast<std::uint32_t>(frame.labels.size());
  if (input.move_left) {
    modal->selected_action = modal->selected_action == 0
                                 ? count - 1
                                 : modal->selected_action - 1;
  }
  if (input.move_right) {
    modal->selected_action = (modal->selected_action + 1) % count;
  }

  if (input.activate) {
    static constexpr std::uint32_t kType1[] = {1};
    static constexpr std::uint32_t kType2[] = {1, 4};
    static constexpr std::uint32_t kType3[] = {2, 3};
    static constexpr std::uint32_t kType4[] = {2, 3, 4};
    const std::uint32_t* results = kType1;
    if (modal->modal_type == 2) results = kType2;
    if (modal->modal_type == 3) results = kType3;
    if (modal->modal_type == 4) results = kType4;
    modal->result = results[modal->selected_action];
  }

  frame.selected_action = modal->selected_action;
  frame.result = modal->result;
  return frame;
}

}  // namespace bandwagon::boot_edit
