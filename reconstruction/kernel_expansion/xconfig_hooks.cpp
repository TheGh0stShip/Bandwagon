#include "xconfig_hooks.hpp"

#include <cstring>

namespace bandwagon::kernel_expansion {

std::uint32_t ForceDvdRegionFreeHook(
    std::uint32_t category, std::uint32_t setting, void* output,
    std::uint32_t output_size, std::uint32_t* required_size,
    XConfigHookServices* services) {
  if (!services) return kXConfigInvalidParameterStatus;

  const std::uint32_t status = services->GetXConfigSetting(
      category, setting, output, output_size, required_size);
  if (status != 0) return status;

  if (static_cast<std::uint16_t>(category) == kSecuredXConfigCategory &&
      static_cast<std::uint16_t>(setting) == kDvdRegionXConfigSetting) {
    // The target blindly stores through r5 here. The null check is a host-only
    // safety guard; it leaves an otherwise successful query unchanged.
    if (output) {
      const std::uint32_t region = kRegionFreeDvdRegion;
      std::memcpy(output, &region, sizeof(region));
    }
  }
  return 0;
}

}  // namespace bandwagon::kernel_expansion
