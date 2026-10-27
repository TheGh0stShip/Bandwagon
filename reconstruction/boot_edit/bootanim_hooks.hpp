#pragma once

#include "theme_editor.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace bandwagon::boot_edit {

struct BootAnimationHook {
  std::uintptr_t patch_address;
  std::uintptr_t replacement_address;
};

inline constexpr std::array<BootAnimationHook, 7> kBootAnimationHooks{{
    {0x98014400u, 0x82010180u},
    {0x9800b438u, 0x820104a8u},
    {0x980144a8u, 0x82010cb8u},
    {0x98017960u, 0x820104d8u},
    {0x98011f90u, 0x82010708u},
    {0x98012588u, 0x820109e8u},
    {0x9801a23cu, 0x820106e0u},
}};

inline constexpr std::array<std::uintptr_t, 4> kBootAnimationNopSites{{
    0x9800b380u, 0x9800b384u, 0x9800b388u, 0x9800b38cu,
}};
inline constexpr std::uint32_t kKernelBootAnimationMode = 0x80000000u;

struct BootAnimationHookServices {
  std::function<void(std::uintptr_t, std::uintptr_t)> install_jump;
  std::function<void(std::uintptr_t)> install_nop;
};

// Complete main body at 0x82010ed0: install seven fixed hooks in order, then
// replace the four-word bootanim sequence at 0x9800b380 with NOPs.
void InstallBootAnimationHooks(const BootAnimationHookServices& services);

// Embedded non-pdata leaf at 0x82010fe0. The target places 0x80000000 in r0
// and tail-calls the cached xboxkrnl ordinal-932 routine.
void InvokeKernelBootAnimationRoutine(
    const std::function<void(std::uint32_t)>& routine);

struct BootAnimationShutdownHookServices {
  std::function<void()> shutdown_editor;
  std::function<void(bool)> continue_boot_animation;
};

// Compound 0x82010148 extent: the entry shuts down BootEdit UI state only
// while it is visible; the installed 0x82010180 hook then selects the original
// bootanim continuation from its inherited state flag.
void RunBootAnimationShutdownHook(
    bool editor_visible, bool alternate_continuation,
    const BootAnimationShutdownHookServices& services);

// Embedded leaf at 0x820101c8, called by the animation hook at 0x820104d8.
float SmoothStep(float lower_edge, float upper_edge, float value);

struct BootAnimationMaskState {
  std::uint32_t flags = 0;
  bool dynamic_window_enabled = false;
};

// Complete 0x82010220 extent, including the register-preserving 0x820104a8
// hook wrapper. Dynamic values correspond to bootanim globals 0x98043068 and
// 0x98043064; the final mask is ThemeManager offset 108.
std::uint32_t UpdateBootAnimationVisibilityMask(
    BootAnimationMaskState* state, float time, float dynamic_time_offset,
    float dynamic_window_origin, std::uint32_t enabled_mask);

struct BootAnimationThemeColors {
  Color logo{};       // target object offset 180
  Color live_mesh{};  // target object offset 28
  Color glow{};       // target object offset 212
};

// Complete installed color hook at 0x820104d8. The input color is the three
// floats at ThemeManager offsets 720..728; all other values are exact target
// constants and smoothstep windows.
BootAnimationThemeColors ComputeBootAnimationThemeColors(
    float time, const Color& theme_color);

using ColorTriplet = std::array<float, 3>;

struct BootAnimationColorForwardServices {
  // Original bootanim routine reached through 0x9801bab8.
  std::function<void(std::uintptr_t bootanim_context,
                     std::uintptr_t original_argument,
                     const ColorTriplet& scaled_color,
                     std::uintptr_t forwarded_argument)>
      invoke_original;
  // Indirect continuation loaded from offset 32 by the installed 0x820106e0
  // adapter. Its fixed first argument (0x9801a24c) is an injected boundary.
  std::function<void(std::uintptr_t original_argument)>
      invoke_installed_continuation;
};

// Complete 0x82010660 extent, including the embedded installed adapter at
// 0x820106e0. The adapter supplies its offset-40 scale, then continues only
// after the scaled-color original call has returned.
ColorTriplet RunBootAnimationColorForwardHook(
    float scale, std::uintptr_t bootanim_context,
    std::uintptr_t original_argument, std::uintptr_t forwarded_argument,
    const ColorTriplet& theme_color,
    const BootAnimationColorForwardServices& services);

}  // namespace bandwagon::boot_edit
