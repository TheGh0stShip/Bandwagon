#include "sata_udma.hpp"

#include <array>

namespace bandwagon::kernel_expansion {

void ConfigureSataUltraDmaMode(SataUdmaServices* services) {
  if (!services) return;

  // The target does not clear this stack buffer; a successful IDENTIFY call
  // owns initialization of its 0xec-byte output.
  std::array<std::uint8_t, kSataIdentifyDataSize> identify_data;
  if (services->IdentifyDevice(
          kSataChannelAddress,
          static_cast<std::uint32_t>(identify_data.size()),
          identify_data.data()) != 0) {
    return;
  }

  const std::uint8_t capabilities =
      identify_data[kSataUltraDmaCapabilityOffset];
  int selected_mode = 7;
  while (selected_mode >= 0 &&
         (capabilities & (1u << selected_mode)) == 0) {
    --selected_mode;
  }
  if (selected_mode < kMinimumSataUltraDmaMode) return;
  if (selected_mode > kMaximumSataUltraDmaMode) {
    selected_mode = kMaximumSataUltraDmaMode;
  }

  const std::uint8_t transfer_mode = static_cast<std::uint8_t>(
      kSataUltraDmaTransferModeBase + selected_mode);
  if (services->SetTransferMode(kSataChannelAddress, transfer_mode) != 0) {
    return;
  }

  services->StallExecutionProcessor(kSataTransferModeSettleMicroseconds);
  (void)services->IdentifyDevice(
      kSataChannelAddress, static_cast<std::uint32_t>(identify_data.size()),
      identify_data.data());
}

}  // namespace bandwagon::kernel_expansion
