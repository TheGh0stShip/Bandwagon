#include "drive_binding.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

class FakeProbe final
    : public bandwagon::kernel_expansion::DrivePathProbeServices {
 public:
  const char16_t* expected_path = nullptr;
  int result = 6;
  int calls = 0;

  int Probe(const char16_t** path_cursor, void** device,
            std::uint32_t flags) override {
    assert(path_cursor && *path_cursor == expected_path);
    assert(device && *device == nullptr);
    assert(flags == 0);
    ++calls;
    return result;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  DrivePathBindingTable table{};
  DrivePathBinding previous{1, 3};
  DrivePathBinding replacement{9, 9};
  table.slots[3] = &previous;
  const char16_t path[] = u"3:\\Content";
  FakeProbe probe;
  probe.expected_path = path;
  assert(BindDrivePath(&table, &replacement, path, 1, &probe) == 6);
  assert(previous.state == 0);
  assert(replacement.state == 0 && replacement.drive_number == 3);
  assert(table.slots[3] == &replacement);
  assert(probe.calls == 1);

  DrivePathBinding no_probe{7, 7};
  const char16_t zero_path[] = u"0:root";
  assert(BindDrivePath(&table, &no_probe, zero_path, 0x100, nullptr) == 0);
  assert(no_probe.state == 0 && no_probe.drive_number == 0);
  assert(table.slots[0] == &no_probe);

  const char16_t no_prefix[] = u"content";
  DrivePathBinding implicit_zero{2, 2};
  assert(BindDrivePath(&table, &implicit_zero, no_prefix, 0, nullptr) == 0);
  assert(no_probe.state == 0);
  assert(table.slots[0] == &implicit_zero);
  assert(implicit_zero.drive_number == 0);

  const char16_t invalid[] = u"12:bad";
  const auto unchanged = table.slots;
  DrivePathBinding untouched{4, 4};
  assert(BindDrivePath(&table, &untouched, invalid, 0, nullptr) ==
         kInvalidDrivePathStatus);
  assert(table.slots == unchanged);
  assert(untouched.state == 4 && untouched.drive_number == 4);

  const char16_t clear_path[] = u"3:";
  assert(BindDrivePath(&table, nullptr, clear_path, 0, nullptr) == 0);
  assert(table.slots[3] == nullptr && replacement.state == 0);

  assert(BindDrivePath(nullptr, &replacement, path, 0, nullptr) ==
         kInvalidDrivePathStatus);
  assert(BindDrivePath(&table, &replacement, nullptr, 0, nullptr) ==
         kInvalidDrivePathStatus);

  std::cout << "Kernel Expansion drive binding tests passed\n";
}
