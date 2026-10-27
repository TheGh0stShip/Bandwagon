#include "runtime_frame.hpp"

namespace bandwagon::boot_edit {

bool AdvanceRuntimeFrame(RuntimeFrameState* state, RuntimeFrameGlobals* globals,
                         const RuntimeFrameServices& services) {
  if (!state || !globals) return false;

  if (!globals->initialized) {
    if (services.initialize_controllers) services.initialize_controllers();
    if (services.initialize_editor) services.initialize_editor();
    globals->initialized = true;
  }
  if (services.update_controllers) services.update_controllers();

  const auto matches = [&](std::uint16_t mask) {
    return services.controller_matches &&
           services.controller_matches(mask, false);
  };
  if (matches(kToggleEditorMask)) {
    globals->editor_visible = !globals->editor_visible;
  }
  if (globals->editor_visible) {
    if (services.prepare_editor_frame) services.prepare_editor_frame();
    if (services.render_editor_frame) services.render_editor_frame();
  }

  const std::uint32_t now =
      services.read_ticks ? services.read_ticks() : state->previous_ticks;
  state->delta = static_cast<float>(now - state->previous_ticks) *
                 kMillisecondsToSeconds;
  if (state->delta > kMaximumOrdinaryDelta) state->delta = kUnitDelta;

  if (matches(kToggleUnitDeltaMask)) {
    globals->force_unit_delta = !globals->force_unit_delta;
  }
  if (matches(kRestartAnimationMask)) state->restart_value = kUnitDelta;

  const std::uint8_t rewind =
      services.rewind_amount ? services.rewind_amount() : 0;
  if (rewind != 0) {
    state->delta *= -static_cast<float>(rewind) * kByteToUnit;
  }
  if (globals->force_unit_delta) state->delta = kUnitDelta;

  if (!state->skip_primary_update && services.update_primary &&
      !services.update_primary(state->delta)) {
    return false;
  }
  if (state->primary_completion_pending) {
    if (services.complete_primary) services.complete_primary();
    state->primary_complete = true;
  }
  if (state->update_auxiliary && services.update_auxiliary) {
    services.update_auxiliary(state->auxiliary_handle, state->delta);
  }
  state->previous_ticks = now;
  return true;
}

}  // namespace bandwagon::boot_edit
