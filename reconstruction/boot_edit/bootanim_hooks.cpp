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

ColorTriplet RunBootAnimationColorForwardHook(
    float scale, std::uintptr_t bootanim_context,
    std::uintptr_t original_argument, std::uintptr_t forwarded_argument,
    const ColorTriplet& theme_color,
    const BootAnimationColorForwardServices& services) {
  const ColorTriplet scaled_color{
      theme_color[0] * scale,
      theme_color[1] * scale,
      theme_color[2] * scale,
  };
  if (services.invoke_original) {
    services.invoke_original(bootanim_context, original_argument, scaled_color,
                             forwarded_argument);
  }
  if (services.invoke_installed_continuation) {
    services.invoke_installed_continuation(original_argument);
  }
  return scaled_color;
}

namespace {

std::uint32_t PackArgb(float red, float green, float blue) {
  const auto r = static_cast<std::uint8_t>(static_cast<std::int64_t>(red));
  const auto g = static_cast<std::uint8_t>(static_cast<std::int64_t>(green));
  const auto b = static_cast<std::uint8_t>(static_cast<std::int64_t>(blue));
  return 0xff000000u | (static_cast<std::uint32_t>(r) << 16) |
         (static_cast<std::uint32_t>(g) << 8) |
         static_cast<std::uint32_t>(b);
}

}  // namespace

BootAnimationNoiseTexture GenerateBootAnimationNoiseTexture(
    const std::array<double, 512>& noise, const ColorTriplet& lower_color,
    const ColorTriplet& anchor_color, const ColorTriplet& upper_color,
    const BootAnimationShapeTransform& shape_transform) {
  BootAnimationNoiseTexture result;
  result.argb.resize(result.width * result.height);

  const float lower_normalizer = static_cast<float>(
      1.0 / shape_transform(0.10000000149011612));
  const float upper_normalizer =
      static_cast<float>(1.0 / shape_transform(0.5));

  for (std::uint32_t y = 0; y < result.height; ++y) {
    const float vertical = static_cast<float>(y) * 0.007843138f;
    ColorTriplet base{};
    float profile = 0.0f;

    if (vertical > 1.0f) {
      for (std::size_t channel = 0; channel < base.size(); ++channel) {
        base[channel] = upper_color[channel] * 255.0f;
      }
      const float distance = 2.0f - vertical;
      if (distance >= 0.0f) {
        profile = static_cast<float>(shape_transform(distance) *
                                     upper_normalizer);
      }
    } else {
      const float blend = vertical * vertical;
      for (std::size_t channel = 0; channel < base.size(); ++channel) {
        base[channel] = lower_color[channel] * 255.0f * (1.0f - blend) +
                        anchor_color[channel] * 255.0f * blend;
      }
      if (vertical >= 0.0f) {
        profile = static_cast<float>(shape_transform(vertical) *
                                     lower_normalizer);
      }
    }

    for (std::uint32_t x = 0; x < result.width; ++x) {
      float amount = static_cast<float>(noise[x]) * profile;
      if (!(amount < 1.0f)) {
        amount = 1.0f;
      }
      const float inverse = 1.0f - amount;
      result.argb[y * result.width + x] = PackArgb(
          base[0] * amount + anchor_color[0] * 255.0f * inverse,
          base[1] * amount + anchor_color[1] * 255.0f * inverse,
          base[2] * amount + anchor_color[2] * 255.0f * inverse);
    }
  }
  return result;
}

}  // namespace bandwagon::boot_edit
