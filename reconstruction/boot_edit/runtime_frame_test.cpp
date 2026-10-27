#include "runtime_frame.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  RuntimeFrameState state;
  RuntimeFrameGlobals globals;
  std::vector<std::string> events;
  std::uint16_t active_mask = kToggleEditorMask;
  RuntimeFrameServices services;
  services.initialize_controllers = [&] { events.push_back("init-controllers"); };
  services.initialize_editor = [&] { events.push_back("init-editor"); };
  services.update_controllers = [&] { events.push_back("poll"); };
  services.controller_matches = [&](std::uint16_t mask, bool consume) {
    assert(!consume);
    events.push_back("mask-" + std::to_string(mask));
    return mask == active_mask;
  };
  services.prepare_editor_frame = [&] { events.push_back("prepare-editor"); };
  services.render_editor_frame = [&] { events.push_back("render-editor"); };
  services.read_ticks = [] { return 50u; };
  services.rewind_amount = [] { return static_cast<std::uint8_t>(0); };
  services.update_primary = [&](float delta) {
    assert(std::fabs(delta - 0.05f) < 0.00001f);
    events.push_back("primary");
    return true;
  };
  assert(AdvanceRuntimeFrame(&state, &globals, services));
  assert(globals.initialized && globals.editor_visible);
  assert(state.previous_ticks == 50);
  assert((events == std::vector<std::string>{
                        "init-controllers", "init-editor", "poll", "mask-64",
                        "prepare-editor", "render-editor", "mask-128",
                        "mask-256", "primary"}));

  events.clear();
  active_mask = kToggleUnitDeltaMask;
  state.primary_completion_pending = true;
  state.update_auxiliary = true;
  state.auxiliary_handle = 0x1234;
  services.read_ticks = [] { return 250u; };
  services.rewind_amount = [] { return static_cast<std::uint8_t>(128); };
  services.update_primary = [&](float delta) {
    assert(delta == kUnitDelta);
    events.push_back("primary");
    return true;
  };
  services.complete_primary = [&] { events.push_back("complete"); };
  services.update_auxiliary = [&](std::uintptr_t handle, float delta) {
    assert(handle == 0x1234 && delta == kUnitDelta);
    events.push_back("auxiliary");
  };
  assert(AdvanceRuntimeFrame(&state, &globals, services));
  assert(globals.force_unit_delta && state.primary_complete);
  assert(state.previous_ticks == 250);
  assert(events.front() == "poll");
  assert((std::vector<std::string>(events.end() - 3, events.end()) ==
          std::vector<std::string>{"primary", "complete", "auxiliary"}));

  active_mask = kRestartAnimationMask;
  globals.force_unit_delta = false;
  state.primary_completion_pending = false;
  state.update_auxiliary = false;
  services.read_ticks = [] { return 260u; };
  services.rewind_amount = [] { return static_cast<std::uint8_t>(255); };
  services.update_primary = [](float delta) {
    assert(std::fabs(delta + 0.01f) < 0.00001f);
    return false;
  };
  assert(!AdvanceRuntimeFrame(&state, &globals, services));
  assert(state.restart_value == kUnitDelta);
  assert(state.previous_ticks == 250);  // early failure skips publication

  assert(!AdvanceRuntimeFrame(nullptr, &globals, services));
  assert(!AdvanceRuntimeFrame(&state, nullptr, services));
}
