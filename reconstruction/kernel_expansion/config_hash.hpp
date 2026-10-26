#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

// Lift of the case-insensitive configuration-key CRC-32 at 0x801f7878.
std::uint32_t ComputeConfigKeyHash(const char* text);

}  // namespace bandwagon::kernel_expansion
