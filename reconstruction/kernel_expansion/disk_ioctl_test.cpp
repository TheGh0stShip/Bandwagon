#include "disk_ioctl.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using namespace bandwagon::kernel_expansion;

namespace {

class FakeDiagnostics final : public DiskIoDiagnostics {
 public:
  void PrintCommand(std::uint8_t command) override {
    commands.push_back(command);
  }
  void PrintMissingDrive(std::uint8_t drive) override {
    missing_drives.push_back(drive);
  }
  void DebugBreak() override { ++break_count; }

  std::vector<std::uint8_t> commands;
  std::vector<std::uint8_t> missing_drives;
  int break_count = 0;
};

template <typename T>
T Read(const std::array<std::uint8_t, 8>& bytes) {
  T value{};
  std::memcpy(&value, bytes.data(), sizeof(value));
  return value;
}

}  // namespace

int main() {
  DiskIoParentDevice parent{0x1122334455667788ull};
  DiskIoDevice device{&parent, 0x200};
  DiskIoDriveTable drives{};
  drives[1] = &device;
  FakeDiagnostics diagnostics;
  std::array<std::uint8_t, 8> output{};

  assert(PeerDiskIoControl(0x101, 0x100, output.data(), drives,
                           &diagnostics) == kDiskResultOk);
  assert(diagnostics.commands.back() == kDiskIoControlSync);

  assert(PeerDiskIoControl(1, kDiskIoGetSectorCount, output.data(), drives,
                           &diagnostics) == kDiskResultOk);
  assert(Read<std::uint64_t>(output) == parent.sector_count);
  assert(PeerDiskIoControl(1, kDiskIoGetSectorSize, output.data(), drives,
                           &diagnostics) == kDiskResultOk);
  assert(Read<std::uint16_t>(output) == 0x200);
  assert(PeerDiskIoControl(1, kDiskIoGetBlockSize, output.data(), drives,
                           &diagnostics) == kDiskResultOk);
  assert(Read<std::uint32_t>(output) == 0x80);
  assert(PeerDiskIoControl(1, 4, output.data(), drives, &diagnostics) ==
         kDiskResultParameterError);
  assert(diagnostics.break_count == 0);

  assert(PeerDiskIoControl(9, 1, output.data(), drives, &diagnostics) ==
         kDiskResultParameterError);
  assert(diagnostics.missing_drives == std::vector<std::uint8_t>{9});
  assert(diagnostics.break_count == 1);

  std::cout << "Kernel Expansion disk ioctl tests passed\n";
}
