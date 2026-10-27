#include "theme_selector.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

namespace {

std::vector<SavedTheme> Themes() {
  return {BuildSavedTheme("Alpha.xbat"), BuildSavedTheme("Beta.xbat")};
}

struct SelectorHarness {
  std::vector<std::string> events;
  ThemeSelectorServices Services() {
    ThemeSelectorServices s;
    s.draw_current_theme = [this](std::string_view name) {
      events.push_back("current:" + std::string(name));
    };
    s.begin_list = [this](std::string_view id) {
      events.push_back("begin:" + std::string(id));
      return true;
    };
    s.draw_theme = [this](std::string_view name, bool selected) {
      events.push_back("theme:" + std::string(name) +
                       (selected ? ":1" : ":0"));
    };
    s.draw_status = [this](std::string_view text) {
      events.push_back("status:" + std::string(text));
    };
    s.end_list = [this] { events.push_back("end"); };
    s.start_discard_confirmation = [this](std::uint32_t action) {
      events.push_back("discard:" + std::to_string(action));
    };
    s.load_theme = [this](std::uint32_t index) {
      events.push_back("load:" + std::to_string(index));
    };
    return s;
  }
};

void TestRefreshDrawingAndStatus() {
  ThemeSelectorState state;
  state.view = 1;
  state.refresh_requested = true;
  SelectorHarness h;
  DrawThemeSelector(&state, "Current", false, Themes(), {}, h.Services());
  assert(!state.refresh_requested && state.displayed_themes.size() == 2);
  assert(h.events[2] == "theme:Alpha:1");

  state = {};
  SelectorHarness loading;
  DrawThemeSelector(&state, "Current", true, Themes(), {},
                    loading.Services());
  assert(loading.events[2] == "status:Loading themes...");
  assert(state.displayed_themes.empty());

  SelectorHarness empty;
  DrawThemeSelector(&state, "Current", false, {}, {}, empty.Services());
  assert(empty.events[2] == "status:No saved themes available");
}

void TestThemeEnumerationPolling() {
  std::uintptr_t handle = 0;
  std::uint32_t wait_timeout = 99;
  int closes = 0;
  ThemeEnumerationServices services;
  services.wait = [&](std::uintptr_t value, std::uint32_t timeout) {
    assert(value == 0x1234);
    wait_timeout = timeout;
    return 258u;
  };
  services.close = [&](std::uintptr_t value) {
    assert(value == 0x1234);
    ++closes;
  };

  assert(PollThemeEnumeration(&handle, services));
  handle = 0x1234;
  assert(!PollThemeEnumeration(&handle, services));
  assert(handle == 0x1234 && wait_timeout == 0 && closes == 0);
  services.wait = [](std::uintptr_t, std::uint32_t) { return 0u; };
  assert(PollThemeEnumeration(&handle, services));
  assert(handle == 0 && closes == 1);
  assert(PollThemeEnumeration(nullptr, services));
}

void TestInputWrapLoadAndDiscard() {
  ThemeSelectorState state;
  state.view = 1;
  state.displayed_themes = Themes();
  SelectorHarness h;
  ThemeSelectorInput input;
  input.previous = true;
  DrawThemeSelector(&state, "Current", false, {}, input, h.Services());
  assert(state.selected_theme == 1);
  input = {};
  input.next = true;
  DrawThemeSelector(&state, "Current", false, {}, input, h.Services());
  assert(state.selected_theme == 0);
  input = {};
  input.activate = true;
  DrawThemeSelector(&state, "Current", false, {}, input, h.Services());
  assert(h.events.back() == "load:0");

  state.unsaved_changes = true;
  DrawThemeSelector(&state, "Current", false, {}, input, h.Services());
  assert(state.async_task_pending && h.events.back() == "discard:2");

  state.async_task_pending = false;
  input = {};
  input.back = true;
  DrawThemeSelector(&state, "Current", false, {}, input, h.Services());
  assert(state.view == 0);
}

void TestFrameOrchestrationAndPolling() {
  BootEditFrameState state{true, false, false, true};
  std::vector<std::string> events;
  BootEditFrameServices s;
  s.begin_panel = [&events](std::string_view id) {
    events.push_back("begin:" + std::string(id));
    return true;
  };
  s.end_panel = [&events] { events.push_back("end-panel"); };
  s.draw_settings_actions = [&events] { events.push_back("actions"); };
  s.draw_theme_selector = [&events] { events.push_back("selector"); };
  s.update_modal = [&events] { events.push_back("modal-update"); };
  s.modal_finished = [&events] {
    events.push_back("modal-finished");
    return true;
  };
  s.destroy_modal = [&events] { events.push_back("modal-destroy"); };
  s.poll_task = [&events] {
    events.push_back("task-poll");
    return true;
  };
  s.destroy_task = [&events] { events.push_back("task-destroy"); };
  s.end_frame = [&events] { events.push_back("frame-end"); };
  UpdateBootEditFrame(&state, s);
  assert(!state.modal_present && !state.task_present);
  assert(events.front() == "begin:##menu_options");
  assert(events.back() == "frame-end");

  state = {true, false, true, false};
  events.clear();
  UpdateBootEditFrame(&state, s);
  for (const std::string& event : events) assert(event != "modal-update");
  assert(state.modal_present);
}

void TestPackedResourceConstructor() {
  const PackedResourceState state = InitializePackedResourceState();
  assert(!state.initialized && state.resource_count == 0 &&
         state.table_size == 0 && state.data_offset == 0 &&
         state.data_size == 0 && state.table_offset == 0 && state.flags == 0);
}

}  // namespace

int main() {
  TestRefreshDrawingAndStatus();
  TestThemeEnumerationPolling();
  TestInputWrapLoadAndDiscard();
  TestFrameOrchestrationAndPolling();
  TestPackedResourceConstructor();
}
