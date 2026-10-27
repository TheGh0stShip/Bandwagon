#include "bootanim_hooks.hpp"

#include <algorithm>

namespace bandwagon::boot_edit {

void InstallBootAnimationHooks(const BootAnimationHookServices& services) {
  if (services.install_jump) {
    for (const BootAnimationHook& hook : kBootAnimationHooks) {
      services.install_jump(hook.patch_address, hook.replacement_address);
    }
  }
  if (services.install_nop) {
    for (const std::uintptr_t address : kBootAnimationNopSites) {
      services.install_nop(address);
    }
  }
}

void InvokeKernelBootAnimationRoutine(
    const std::function<void(std::uint32_t)>& routine) {
  if (routine) routine(kKernelBootAnimationMode);
}

void RunBootAnimationShutdownHook(
    bool editor_visible, bool alternate_continuation,
    const BootAnimationShutdownHookServices& services) {
  if (editor_visible && services.shutdown_editor) services.shutdown_editor();
  if (services.continue_boot_animation) {
    services.continue_boot_animation(alternate_continuation);
  }
}

float SmoothStep(float lower_edge, float upper_edge, float value) {
  if (value < lower_edge) return 0.0f;
  if (!(value < upper_edge)) return 1.0f;
  const float t = (value - lower_edge) / (upper_edge - lower_edge);
  return t * t * (3.0f - 2.0f * t);
}

std::uint32_t UpdateBootAnimationVisibilityMask(
    BootAnimationMaskState* state, float time, float dynamic_time_offset,
    float dynamic_window_origin, std::uint32_t enabled_mask) {
  if (!state) return 0;
  struct Window {
    std::uint32_t bit;
    float lower;
    float upper;
  };
  constexpr Window kWindows[] = {
      {0x0001, 0.0f, 1.7000000476837158f},
      {0x0100, 1.4600000381469727f, 3.5f},
      {0x0200, 3.1999998092651367f, 4.199999809265137f},
      {0x0400, 3.0999999046325684f, 10.0f},
      {0x0800, 4.199999809265137f, 10.0f},
      {0x0080, 0.0f, 3.799999952316284f},
      {0x0004, 3.799999952316284f, 10.0f},
      {0x0002, 4.166666507720947f, 10.0f},
      {0x0010, 1.4600000381469727f, 3.0f},
      {0x0008, 0.0f, 1.6660000085830688f},
      {0x0020, 4.166666507720947f, 10.0f},
  };

  state->flags |= 0x0040;
  for (const Window& window : kWindows) {
    // Ordered comparisons intentionally make NaN fall inside, matching the
    // target's paired "less than" / "greater than" branches.
    const bool inside = !(time < window.lower) && !(time > window.upper);
    if (inside)
      state->flags |= window.bit;
    else
      state->flags &= ~window.bit;
  }

  if (state->dynamic_window_enabled) {
    const float dynamic_time = dynamic_time_offset + time;
    const float lower = std::max(dynamic_window_origin, 5.5f);
    const float upper = std::min(dynamic_window_origin + 4.5f, 10.0f);
    const bool inside =
        !(dynamic_time < lower) && !(dynamic_time > upper);
    if (inside)
      state->flags |= 0x1000;
    else
      state->flags &= ~0x1000u;
  }
  state->flags &= enabled_mask;
  return state->flags;
}

BootAnimationThemeColors ComputeBootAnimationThemeColors(
    float time, const Color& theme_color) {
  BootAnimationThemeColors result;

  const float logo_mix = SmoothStep(3.700000047683716f, 5.0f, time);
  result.logo = {
      1.0f - logo_mix * 0.2549019455909729f,
      1.0f - logo_mix * 0.16470587253570557f,
      1.0f - logo_mix * 0.40392154455184937f,
      SmoothStep(1.5f, 3.0f, time),
  };

  const float theme_mix = SmoothStep(2.5f, 5.0f, time);
  result.live_mesh = {
      1.0f + (theme_color[0] - 1.0f) * theme_mix,
      1.0f + (theme_color[1] - 1.0f) * theme_mix,
      1.0f + (theme_color[2] - 1.0f) * theme_mix,
      1.0f,
  };

  result.glow = {
      1.0f - theme_mix * 0.1499999761581421f,
      1.0f - theme_mix * 0.10000002384185791f,
      1.0f - theme_mix * 0.1499999761581421f,
      1.0f,
  };
  const float glow_mix = SmoothStep(5.0f, 5.75f, time);
  result.glow[0] += (0.637499988079071f - result.glow[0]) * glow_mix;
  result.glow[1] += (0.7200000286102295f - result.glow[1]) * glow_mix;
  result.glow[2] += (0.637499988079071f - result.glow[2]) * glow_mix;
  return result;
}

}  // namespace bandwagon::boot_edit
