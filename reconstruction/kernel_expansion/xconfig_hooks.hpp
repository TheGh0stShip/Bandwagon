#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint16_t kSecuredXConfigCategory = 2;
inline constexpr std::uint16_t kDvdRegionXConfigSetting = 4;
inline constexpr std::uint32_t kRegionFreeDvdRegion = 0x000000ffu;
inline constexpr std::uint32_t kXConfigInvalidParameterStatus = 0xc000000du;

class XConfigHookServices {
 public:
  virtual ~XConfigHookServices() = default;

  virtual std::uint32_t GetXConfigSetting(
      std::uint32_t category, std::uint32_t setting, void* output,
      std::uint32_t output_size, std::uint32_t* required_size) = 0;
};

// Lift of 0x801f1c60. The five target arguments are forwarded unchanged. On a
// successful secured DVD-region query, the output is replaced with 0xff.
std::uint32_t ForceDvdRegionFreeHook(
    std::uint32_t category, std::uint32_t setting, void* output,
    std::uint32_t output_size, std::uint32_t* required_size,
    XConfigHookServices* services);

}  // namespace bandwagon::kernel_expansion
