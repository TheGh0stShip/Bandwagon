#pragma once

#include <cstdint>
#include <functional>

namespace bandwagon::boot_edit {

inline constexpr std::uint16_t kToggleEditorMask = 0x0040;
inline constexpr std::uint16_t kToggleUnitDeltaMask = 0x0080;
inline constexpr std::uint16_t kRestartAnimationMask = 0x0100;
inline constexpr float kMillisecondsToSeconds = 0.001f;
inline constexpr float kMaximumOrdinaryDelta = 0.1f;
inline constexpr float kUnitDelta = 1.0f;
inline constexpr float kByteToUnit = 1.0f / 255.0f;

struct RuntimeFrameState {
  std::uint32_t previous_ticks = 0;
  float restart_value = 0.0f;
  float delta = 0.0f;
  bool skip_primary_update = false;
  bool primary_completion_pending = false;
  bool update_auxiliary = false;
  bool primary_complete = false;
  std::uintptr_t auxiliary_handle = 0;
};

struct RuntimeFrameGlobals {
  bool initialized = false;
  bool editor_visible = false;
  bool force_unit_delta = false;
};

struct RuntimeFrameServices {
  std::function<void()> initialize_controllers;
  std::function<void()> initialize_editor;
  std::function<void()> update_controllers;
  std::function<bool(std::uint16_t, bool)> controller_matches;
  std::function<void()> prepare_editor_frame;
  std::function<void()> render_editor_frame;
  std::function<std::uint32_t()> read_ticks;
  std::function<std::uint8_t()> rewind_amount;
  std::function<bool(float)> update_primary;
  std::function<void()> complete_primary;
  std::function<void(std::uintptr_t, float)> update_auxiliary;
};

// Complete readable model of BootEdit 0x82010cb8. The service callbacks are
// the already recovered product routines or Xbox/bootanim boundaries called
// by the target; this function preserves their exact ordering and state policy.
bool AdvanceRuntimeFrame(RuntimeFrameState* state, RuntimeFrameGlobals* globals,
                         const RuntimeFrameServices& services);

}  // namespace bandwagon::boot_edit
