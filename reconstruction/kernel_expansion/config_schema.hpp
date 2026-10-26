#pragma once

#include "config.hpp"
#include "config_value.hpp"

#include <array>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uintptr_t kPeerPressureConfigSchemaAddress =
    0x801ff738;

// Typed reconstruction of the six 20-byte records at 0x801ff738.
extern const std::array<StructuredConfigEntry, kConfigSchemaEntryCount>
    kPeerPressureConfigSchema;

}  // namespace bandwagon::kernel_expansion
