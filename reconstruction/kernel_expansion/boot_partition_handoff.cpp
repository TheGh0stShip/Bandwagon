#include "boot_partition_handoff.hpp"

namespace bandwagon::kernel_expansion {

void HandOffBootPartitions(BootPartitionLoaderState* state,
                           BootPartitionHandoffServices* services) {
  const BootPartitionBinding& required = kBootPartitionBindings[0];
  if (!services->BindPartition(state, required.partition_name,
                               required.device_path, true)) {
    services->DebugBreak();
  }

  for (std::size_t i = 1; i < kBootPartitionBindings.size(); ++i) {
    const BootPartitionBinding& optional = kBootPartitionBindings[i];
    services->BindPartition(state, optional.partition_name,
                            optional.device_path, false);
  }

  services->FreePhysicalMemory(2, state->physical_mapping);
  state->physical_mapping = 0;
  state->mapping_size = 0;
  state->mapping_flags = 0;
}

}  // namespace bandwagon::kernel_expansion
