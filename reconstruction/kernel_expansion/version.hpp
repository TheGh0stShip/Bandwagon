#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::kernel_expansion {

using VersionBytes = std::array<std::uint8_t, 4>;
inline constexpr std::uint8_t kVersionWildcard = 0xff;

// Lift of the four-component version predicate at 0x801f87f8. Each candidate
// byte must be greater than or equal to the corresponding minimum byte. A
// 0xff byte on either side is a wildcard. This is deliberately component-wise,
// not a conventional lexicographic version comparison.
bool IsVersionAtLeastComponentWise(const VersionBytes& candidate,
                                   const VersionBytes& minimum);

}  // namespace bandwagon::kernel_expansion
