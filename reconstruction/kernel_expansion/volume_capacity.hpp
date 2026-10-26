#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

// Flattened representation of the four fields reached through the recovered
// nested volume/device objects. Their original offsets are documented by the
// lift, while this type expresses the capacity algorithm without fake padding.
struct FatVolumeCapacitySource {
  bool capacity_available = false;          // nested volume offset 0x48
  std::uint32_t allocation_entry_count = 0; // geometry offset 0x18
  std::uint16_t units_per_cluster = 0;       // geometry offset 0x0a
  std::uint32_t bytes_per_unit = 0;          // device geometry offset 0x20
};

// Lift of the eight-byte capacity query at 0x801f9ec0. The observed routine
// returns success even when capacity is unavailable and uses wrapping 32-bit
// products before zero-extending the result to 64 bits.
int QueryFatVolumeCapacity(const FatVolumeCapacitySource* source,
                           std::uint64_t* capacity);

}  // namespace bandwagon::kernel_expansion
