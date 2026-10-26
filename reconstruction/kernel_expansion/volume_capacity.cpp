#include "volume_capacity.hpp"

namespace bandwagon::kernel_expansion {

int QueryFatVolumeCapacity(const FatVolumeCapacitySource* source,
                           std::uint64_t* capacity) {
  if (!capacity) return 0;
  *capacity = 0;
  if (!source || !source->capacity_available) return 0;

  std::uint32_t wrapped_capacity =
      (source->allocation_entry_count - 2u) * source->units_per_cluster;
  wrapped_capacity *= source->bytes_per_unit;
  *capacity = wrapped_capacity;
  return 0;
}

}  // namespace bandwagon::kernel_expansion
