#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uintptr_t kSataChannelAddress = 0x801a60e0u;
inline constexpr std::size_t kSataIdentifyDataSize = 0xec;
inline constexpr std::size_t kSataUltraDmaCapabilityOffset = 0xb0;
inline constexpr std::uint8_t kMinimumSataUltraDmaMode = 2;
inline constexpr std::uint8_t kMaximumSataUltraDmaMode = 6;
inline constexpr std::uint8_t kSataUltraDmaTransferModeBase = 0x40;
inline constexpr std::uint32_t kSataTransferModeSettleMicroseconds = 100;

class SataUdmaServices {
 public:
  virtual ~SataUdmaServices() = default;

  virtual std::uint32_t IdentifyDevice(std::uintptr_t channel,
                                       std::uint32_t output_size,
                                       std::uint8_t* output) = 0;
  virtual std::uint32_t SetTransferMode(std::uintptr_t channel,
                                        std::uint8_t transfer_mode) = 0;
  virtual void StallExecutionProcessor(std::uint32_t microseconds) = 0;
};

// Lift of 0x801f04b8. Selects the highest advertised Ultra DMA mode, requires
// at least mode two, clamps the request to mode six, waits 100 microseconds,
// and refreshes IDENTIFY data after a successful mode change.
void ConfigureSataUltraDmaMode(SataUdmaServices* services);

}  // namespace bandwagon::kernel_expansion
