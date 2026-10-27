#include "bootanim_hooks.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  std::vector<std::pair<std::uintptr_t, std::uintptr_t>> jumps;
  std::vector<std::uintptr_t> nops;
  BootAnimationHookServices services;
  services.install_jump = [&](std::uintptr_t patch,
                              std::uintptr_t replacement) {
    jumps.emplace_back(patch, replacement);
  };
  services.install_nop = [&](std::uintptr_t address) { nops.push_back(address); };
  InstallBootAnimationHooks(services);
  assert(jumps.size() == kBootAnimationHooks.size());
  for (std::size_t i = 0; i < jumps.size(); ++i) {
    assert(jumps[i].first == kBootAnimationHooks[i].patch_address);
    assert(jumps[i].second == kBootAnimationHooks[i].replacement_address);
  }
  assert(nops == std::vector<std::uintptr_t>(kBootAnimationNopSites.begin(),
                                              kBootAnimationNopSites.end()));

  std::uint32_t mode = 0;
  InvokeKernelBootAnimationRoutine(
      [&](std::uint32_t value) { mode = value; });
  assert(mode == 0x80000000u);
  InvokeKernelBootAnimationRoutine({});

  std::vector<int> shutdown_events;
  BootAnimationShutdownHookServices shutdown_services;
  shutdown_services.shutdown_editor = [&] { shutdown_events.push_back(1); };
  shutdown_services.continue_boot_animation = [&](bool alternate) {
    shutdown_events.push_back(alternate ? 3 : 2);
  };
  RunBootAnimationShutdownHook(false, false, shutdown_services);
  assert((shutdown_events == std::vector<int>{2}));
  shutdown_events.clear();
  RunBootAnimationShutdownHook(true, true, shutdown_services);
  assert((shutdown_events == std::vector<int>{1, 3}));

  assert(SmoothStep(2.0f, 6.0f, 1.0f) == 0.0f);
  assert(SmoothStep(2.0f, 6.0f, 6.0f) == 1.0f);
  assert(std::fabs(SmoothStep(2.0f, 6.0f, 4.0f) - 0.5f) < 0.00001f);
  assert(SmoothStep(2.0f, 6.0f,
                    std::numeric_limits<float>::quiet_NaN()) == 1.0f);

  BootAnimationMaskState mask_state;
  assert(UpdateBootAnimationVisibilityMask(&mask_state, 0.0f, 0.0f, 0.0f,
                                           0xffffffffu) ==
         (0x0040u | 0x0001u | 0x0080u | 0x0008u));
  assert(UpdateBootAnimationVisibilityMask(&mask_state, 4.2f, 0.0f, 0.0f,
                                           0xffffffffu) ==
         (0x0040u | 0x0200u | 0x0400u | 0x0800u | 0x0004u | 0x0002u |
          0x0020u));

  mask_state.dynamic_window_enabled = true;
  mask_state.flags = 0;
  const std::uint32_t dynamic = UpdateBootAnimationVisibilityMask(
      &mask_state, 2.0f, 4.0f, 5.0f, 0x1041u);
  assert(dynamic == 0x1040u);  // dynamic time 6 lies in [5.5, 9.5]
  mask_state.dynamic_window_enabled = false;
  mask_state.flags = 0x1000;
  assert((UpdateBootAnimationVisibilityMask(&mask_state, 20.0f, 0.0f, 0.0f,
                                            0xffffffffu) & 0x1000u) != 0);
  mask_state.flags = 0;
  assert(UpdateBootAnimationVisibilityMask(
             &mask_state, std::numeric_limits<float>::quiet_NaN(), 0.0f,
             0.0f, 0xffffffffu) == 0x0fffu);
  assert(UpdateBootAnimationVisibilityMask(nullptr, 0, 0, 0, 0xffffffffu) ==
         0);

  const Color theme_color{0.2f, 0.4f, 0.6f, 9.0f};
  BootAnimationThemeColors colors =
      ComputeBootAnimationThemeColors(0.0f, theme_color);
  assert(colors.logo[0] == 1.0f && colors.logo[1] == 1.0f &&
         colors.logo[2] == 1.0f && colors.logo[3] == 0.0f);
  assert(colors.live_mesh[0] == 1.0f && colors.live_mesh[1] == 1.0f &&
         colors.live_mesh[2] == 1.0f && colors.live_mesh[3] == 1.0f);
  assert(colors.glow[0] == 1.0f && colors.glow[1] == 1.0f &&
         colors.glow[2] == 1.0f && colors.glow[3] == 1.0f);

  colors = ComputeBootAnimationThemeColors(5.75f, theme_color);
  assert(std::fabs(colors.logo[0] - 0.74509805f) < 0.00001f);
  assert(std::fabs(colors.logo[1] - 0.83529413f) < 0.00001f);
  assert(std::fabs(colors.logo[2] - 0.59607846f) < 0.00001f);
  assert(colors.logo[3] == 1.0f);
  assert(std::fabs(colors.live_mesh[0] - 0.2f) < 0.00001f);
  assert(std::fabs(colors.live_mesh[1] - 0.4f) < 0.00001f);
  assert(std::fabs(colors.live_mesh[2] - 0.6f) < 0.00001f);
  assert(std::fabs(colors.glow[0] - 0.6375f) < 0.00001f);
  assert(std::fabs(colors.glow[1] - 0.72f) < 0.00001f);
  assert(std::fabs(colors.glow[2] - 0.6375f) < 0.00001f);

  colors = ComputeBootAnimationThemeColors(
      std::numeric_limits<float>::quiet_NaN(), theme_color);
  assert(std::fabs(colors.glow[0] - 0.6375f) < 0.00001f);

  std::array<int, 2> color_forward_order{};
  int color_forward_count = 0;
  ColorTriplet observed_color{};
  BootAnimationColorForwardServices color_forward_services;
  color_forward_services.invoke_original =
      [&](std::uintptr_t context, std::uintptr_t argument,
          const ColorTriplet& color, std::uintptr_t forwarded) {
        color_forward_order[color_forward_count++] = 1;
        assert(context == 0x980787e0u);
        assert(argument == 0x1111u);
        assert(forwarded == 0x2222u);
        observed_color = color;
      };
  color_forward_services.invoke_installed_continuation =
      [&](std::uintptr_t argument) {
        color_forward_order[color_forward_count++] = 2;
        assert(argument == 0x1111u);
      };
  const ColorTriplet scaled = RunBootAnimationColorForwardHook(
      0.25f, 0x980787e0u, 0x1111u, 0x2222u,
      ColorTriplet{0.4f, 0.8f, 1.2f}, color_forward_services);
  assert(color_forward_count == 2);
  assert(color_forward_order[0] == 1 && color_forward_order[1] == 2);
  assert(std::fabs(scaled[0] - 0.1f) < 0.00001f);
  assert(std::fabs(scaled[1] - 0.2f) < 0.00001f);
  assert(std::fabs(scaled[2] - 0.3f) < 0.00001f);
  assert(observed_color == scaled);

  std::array<double, 512> noise{};
  const ColorTriplet lower{0.2f, 0.3f, 0.4f};
  const ColorTriplet anchor{0.6f, 0.7f, 0.8f};
  const ColorTriplet upper{0.1f, 0.2f, 0.3f};
  int shape_calls = 0;
  const BootAnimationNoiseTexture zero_noise_texture =
      GenerateBootAnimationNoiseTexture(
          noise, lower, anchor, upper,
          [&](double value) {
            ++shape_calls;
            return value + 1.0;
          });
  assert(zero_noise_texture.width == 512);
  assert(zero_noise_texture.height == 256);
  assert(zero_noise_texture.argb.size() == 512u * 256u);
  assert(shape_calls == 258);
  assert(zero_noise_texture.argb.front() == 0xff99b2ccu);
  assert(zero_noise_texture.argb.back() == 0xff99b2ccu);

  noise.fill(1.0);
  const BootAnimationNoiseTexture solid_noise_texture =
      GenerateBootAnimationNoiseTexture(
          noise, lower, anchor, upper, [](double) { return 1.0; });
  assert(solid_noise_texture.argb.front() == 0xff334c66u);
  assert(solid_noise_texture.argb.back() == 0xff19334cu);

  int sample_calls = 0;
  float last_sample_x = 0.0f;
  float last_sample_y = 0.0f;
  const BootAnimationSampledTexture dark_sampled_texture =
      GenerateBootAnimationSampledTexture(
          ColorTriplet{0.4f, 0.6f, 0.8f},
          [&](float x, float y) {
            ++sample_calls;
            last_sample_x = x;
            last_sample_y = y;
            return -1.0f;
          });
  assert(dark_sampled_texture.width == 256);
  assert(dark_sampled_texture.height == 256);
  assert(dark_sampled_texture.argb.size() == 256u * 256u);
  assert(sample_calls == 256 * 256);
  assert(dark_sampled_texture.argb.front() == 0xff334c66u);
  assert(dark_sampled_texture.argb.back() == 0xff334c66u);
  assert(std::fabs(last_sample_x - 130.0f) < 0.0001f);
  assert(std::fabs(last_sample_y - 130.0f) < 0.0001f);

  const BootAnimationSampledTexture bright_sampled_texture =
      GenerateBootAnimationSampledTexture(
          ColorTriplet{0.4f, 0.6f, 0.8f},
          [](float, float) { return 1.0f; });
  assert(bright_sampled_texture.argb.front() == 0xff6699ccu);
}
