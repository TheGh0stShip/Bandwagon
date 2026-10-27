#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kDriveDeviceSlotCount = 10;
using DriveDeviceHandle = std::uint32_t;
using DriveRegistryLockCookie = std::uint32_t;

struct DriveDeviceRegistry {
  std::array<DriveDeviceHandle, kDriveDeviceSlotCount> slots{};
};

class DriveRegistryLockServices {
 public:
  virtual ~DriveRegistryLockServices() = default;
  virtual DriveRegistryLockCookie Acquire() = 0;
  virtual void Release(DriveRegistryLockCookie cookie) = 0;
};

// Lift of the ten-slot registration routine at 0x801f3d90. It returns the
// first free slot or -1 when the table is full.
std::int32_t RegisterDriveDevice(DriveDeviceRegistry* registry,
                                 DriveDeviceHandle device,
                                 DriveRegistryLockServices* locks);

// Lift of the slot-clear routine at 0x801f3e08. The target trusts its internal
// caller's index; the host lift rejects an out-of-range index safely.
void UnregisterDriveDevice(DriveDeviceRegistry* registry, std::uint32_t slot,
                           DriveRegistryLockServices* locks);

// Byte-identical predicates recovered at 0x801fa2e0 and 0x801fa300. The
// compiler emitted separate copies for two media-state branches.
bool IsDriveDeviceSlotEmpty(const DriveDeviceRegistry* registry,
                            std::uint32_t slot);
bool IsDriveDeviceSlotEmptyDuplicate(const DriveDeviceRegistry* registry,
                                     std::uint32_t slot);

struct DriveDeviceReleaseContext {
  std::uint32_t registry_slot = 0;    // nested device offset 0xc0
  std::uintptr_t backing_object = 0;  // nested device offset 0xa0
  std::uintptr_t owner_object = 0;    // incoming object
};

using ObjectDereference = std::function<void(std::uintptr_t)>;

// Complete cleanup routine at 0x801f5110: unregister the nested device slot,
// dereference its backing object, then dereference the incoming owner object.
void ReleaseDriveDeviceObject(const DriveDeviceReleaseContext& context,
                              DriveDeviceRegistry* registry,
                              DriveRegistryLockServices* locks,
                              const ObjectDereference& dereference);

}  // namespace bandwagon::kernel_expansion
