#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::aurora_fixer {

// Encodes the exact 16-byte Xenon absolute branch stub emitted by
// PatchAbsoluteBranch. Bytes are returned in target-native big-endian order.
std::array<std::uint8_t, 16> EncodeAbsoluteBranch(std::uint32_t destination,
                                                  bool link);

}  // namespace bandwagon::aurora_fixer
