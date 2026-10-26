#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

// Lift of the UTF-16 drive-prefix parser at 0x801f6af0. A valid prefix is
// exactly one decimal digit followed by a colon. On success, cursor advances
// past the colon. A missing colon returns zero without changing cursor, while
// malformed colon prefixes and null input strings return -1.
std::int32_t ParseDriveNumberPrefix(const char16_t** cursor);

}  // namespace bandwagon::kernel_expansion
