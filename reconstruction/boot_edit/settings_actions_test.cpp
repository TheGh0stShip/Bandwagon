#include "settings_actions.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

namespace {

struct Harness {
  std::vector<std::string> events;
  bool set_active_success = true;
  bool exit_marks_unsaved = false;

  SettingsActionServices Services(SettingsActionState* state) {
    SettingsActionServices s;
    s.draw_action = [this](std::string_view label, bool selected,
                           std::uint32_t color) {
      events.push_back("draw:" + std::string(label) + ":" +
                       (selected ? "1:" : "0:") + std::to_string(color));
    };
    s.reset_to_new_theme = [this] { events.push_back("reset"); };
    s.start_discard_confirmation = [this](std::uint32_t action) {
      events.push_back("discard:" + std::to_string(action));
    };
    s.start_save = [this](bool save_as) {
      events.push_back(save_as ? "save-as" : "save");
    };
    s.set_as_active_theme = [this] {
      events.push_back("set-active");
      return set_active_success;
    };
    s.show_feedback = [this](std::string_view message, std::string_view title,
                             bool success) {
      events.push_back("feedback:" + std::string(message) + ":" +
                       std::string(title) + (success ? ":1" : ":0"));
    };
    s.show_success_notification = [this](float seconds) {
      assert(seconds == 2.0f);
      events.push_back("notify");
    };
    s.request_exit = [this, state] {
      events.push_back("exit");
      state->unsaved_changes = exit_marks_unsaved;
    };
    s.return_to_firmware = [this](std::uint32_t routine) {
      assert(routine == 6);
      events.push_back("firmware");
    };
    return s;
  }
};

SettingsActionInput Activate() {
  SettingsActionInput input;
  input.activate = true;
  return input;
}

void TestDrawingNavigationAndGates() {
  SettingsActionState state;
  state.selected_action = 6;
  state.editor_change_pending = true;
  Harness h;
  SettingsActionInput input;
  input.clear_editor_change = true;
  input.next = true;
  input.previous = true;
  UpdateSettingsActions(&state, input, h.Services(&state));
  assert(h.events.size() == 7);
  assert(h.events[0].find("New Theme") != std::string::npos);
  assert(h.events[6].find("Exit:1:") != std::string::npos);
  assert(state.selected_action == 6);  // next then previous, both honored
  assert(!state.editor_change_pending);

  state.view = 2;
  input.activate = true;
  UpdateSettingsActions(&state, input, h.Services(&state));
  assert(state.view == 2);  // input is gated, drawing is not
}

void TestNewLoadAndSaveActions() {
  SettingsActionState state;
  Harness h;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(h.events.back() == "reset");

  state.unsaved_changes = true;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(state.async_task_pending && h.events.back() == "discard:1");

  state = {};
  state.selected_action = 1;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(state.view == 1);

  state = {};
  state.selected_action = 2;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(state.async_task_pending && h.events.back() == "save");
  state = {};
  state.selected_action = 3;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(state.async_task_pending && h.events.back() == "save-as");
}

void TestSetActiveFeedback() {
  SettingsActionState state;
  state.selected_action = 4;
  Harness ok;
  UpdateSettingsActions(&state, Activate(), ok.Services(&state));
  assert(state.feedback_modal_pending);
  assert(ok.events[8].find("Settings updated successfully!") !=
         std::string::npos);
  assert(ok.events.back() == "notify");

  state = {};
  state.selected_action = 4;
  Harness fail;
  fail.set_active_success = false;
  UpdateSettingsActions(&state, Activate(), fail.Services(&state));
  assert(fail.events.back().find("Failed to update settings!") !=
         std::string::npos);
}

void TestRebootAndExit() {
  SettingsActionState state;
  state.selected_action = 5;
  Harness h;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(h.events.back() == "firmware");

  state = {};
  state.selected_action = 5;
  state.unsaved_changes = true;
  UpdateSettingsActions(&state, Activate(), h.Services(&state));
  assert(state.async_task_pending && h.events.back() == "discard:1");

  state = {};
  state.selected_action = 6;
  Harness exit;
  UpdateSettingsActions(&state, Activate(), exit.Services(&state));
  assert(exit.events[7] == "exit" && exit.events[8] == "firmware");

  state = {};
  state.selected_action = 6;
  Harness returning_exit;
  returning_exit.exit_marks_unsaved = true;
  UpdateSettingsActions(&state, Activate(), returning_exit.Services(&state));
  assert(returning_exit.events.back() == "discard:1");
}

}  // namespace

int main() {
  TestDrawingNavigationAndGates();
  TestNewLoadAndSaveActions();
  TestSetActiveFeedback();
  TestRebootAndExit();
}
