#include "drive_registry.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t RegisterDriveDevice(DriveDeviceRegistry* registry,
                                 DriveDeviceHandle device,
                                 DriveRegistryLockServices* locks) {
  if (!registry || !locks) return -1;

  const DriveRegistryLockCookie cookie = locks->Acquire();
  std::int32_t result = -1;
  for (std::size_t index = 0; index < registry->slots.size(); ++index) {
    if (registry->slots[index] == 0) {
      registry->slots[index] = device;
      result = static_cast<std::int32_t>(index);
      break;
    }
  }
  locks->Release(cookie);
  return result;
}

void UnregisterDriveDevice(DriveDeviceRegistry* registry, std::uint32_t slot,
                           DriveRegistryLockServices* locks) {
  if (!registry || !locks || slot >= registry->slots.size()) return;

  const DriveRegistryLockCookie cookie = locks->Acquire();
  registry->slots[slot] = 0;
  locks->Release(cookie);
}

bool IsDriveDeviceSlotEmpty(const DriveDeviceRegistry* registry,
                            std::uint32_t slot) {
  if (!registry) return true;
  const std::uint8_t masked_slot = static_cast<std::uint8_t>(slot);
  if (masked_slot >= registry->slots.size()) return true;
  return registry->slots[masked_slot] == 0;
}

bool IsDriveDeviceSlotEmptyDuplicate(const DriveDeviceRegistry* registry,
                                     std::uint32_t slot) {
  return IsDriveDeviceSlotEmpty(registry, slot);
}

}  // namespace bandwagon::kernel_expansion
