#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::hdd_security {

struct TriggerPayload {
  std::uint32_t reserved_00;
  std::uint32_t kernel_pointer_04;
  std::uint32_t reserved_08;
  std::uint32_t kernel_pointer_0c;
  std::uint32_t stack_pivot_entry;
  std::uint32_t marker;
};

inline constexpr TriggerPayload kRetail17559Trigger{
    0,
    0x801ca000,
    0,
    0x801ca1c8,
    0x800d27a0,
    0x58425050,  // "XBPP"
};

std::array<std::uint8_t, 24> BuildTriggerPayload(
    const TriggerPayload& trigger = kRetail17559Trigger);

}  // namespace bandwagon::hdd_security
