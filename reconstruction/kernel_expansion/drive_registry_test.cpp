#include "drive_registry.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

class FakeLocks final
    : public bandwagon::kernel_expansion::DriveRegistryLockServices {
 public:
  bandwagon::kernel_expansion::DriveRegistryLockCookie next_cookie = 7;
  std::vector<std::string> events;

  bandwagon::kernel_expansion::DriveRegistryLockCookie Acquire() override {
    events.emplace_back("acquire");
    return next_cookie;
  }

  void Release(
      bandwagon::kernel_expansion::DriveRegistryLockCookie cookie) override {
    assert(cookie == next_cookie);
    events.emplace_back("release");
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  DriveDeviceRegistry registry{};
  registry.slots[0] = 0x1000;
  registry.slots[1] = 0x2000;
  FakeLocks locks;
  assert(RegisterDriveDevice(&registry, 0x3333, &locks) == 2);
  assert(registry.slots[2] == 0x3333);
  assert((locks.events == std::vector<std::string>{"acquire", "release"}));

  locks.events.clear();
  UnregisterDriveDevice(&registry, 1, &locks);
  assert(registry.slots[1] == 0);
  assert((locks.events == std::vector<std::string>{"acquire", "release"}));

  locks.events.clear();
  assert(RegisterDriveDevice(&registry, 0x4444, &locks) == 1);
  assert(registry.slots[1] == 0x4444);

  DriveDeviceRegistry full{};
  full.slots.fill(0x5555);
  FakeLocks full_locks;
  assert(RegisterDriveDevice(&full, 0x6666, &full_locks) == -1);
  for (DriveDeviceHandle slot : full.slots) assert(slot == 0x5555);
  assert((full_locks.events ==
          std::vector<std::string>{"acquire", "release"}));

  FakeLocks zero_locks;
  DriveDeviceRegistry zero_registry{};
  assert(RegisterDriveDevice(&zero_registry, 0, &zero_locks) == 0);
  assert(zero_registry.slots[0] == 0);

  const auto unchanged = registry.slots;
  locks.events.clear();
  UnregisterDriveDevice(&registry, 10, &locks);
  assert(registry.slots == unchanged && locks.events.empty());

  assert(RegisterDriveDevice(nullptr, 1, &locks) == -1);
  assert(RegisterDriveDevice(&registry, 1, nullptr) == -1);
  UnregisterDriveDevice(nullptr, 0, &locks);

  std::cout << "Kernel Expansion drive registry tests passed\n";
}
