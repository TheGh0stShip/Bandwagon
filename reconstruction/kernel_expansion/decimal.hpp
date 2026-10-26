#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

// Lift of the decimal parser at 0x801f78e0. It accepts only an optional leading
// minus and decimal digits, stops at the first other byte, and keeps the
// target's low-32-bit arithmetic behavior.
std::int32_t ParseSignedDecimal(const char* text);

}  // namespace bandwagon::kernel_expansion
