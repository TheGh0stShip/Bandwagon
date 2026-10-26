#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint16_t kOemUnitedStatesCodePage = 437;

// Lift of the OEM character decoder at 0x801fda38. ASCII values preserve the
// complete input word; extended values decode only for code page 437 and only
// across the one-byte 0x80..0xff range.
std::uint32_t DecodeOem437Character(std::uint32_t encoded_character,
                                    std::uint32_t code_page);

}  // namespace bandwagon::kernel_expansion
