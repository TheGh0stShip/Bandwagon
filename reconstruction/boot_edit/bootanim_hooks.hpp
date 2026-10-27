#pragma once

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

}  // namespace bandwagon::boot_edit
