#include "bootstrap_mapping.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t ReleaseBootstrapMapping(BootstrapMappingServices* services) {
  if (!services) return 0;
  const BootstrapMappingRelease request;
  return services->FreeMappedMemory(request);
}

}  // namespace bandwagon::kernel_expansion
