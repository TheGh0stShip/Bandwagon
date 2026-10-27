#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint8_t kDiskIoControlSync = 0;
inline constexpr std::uint8_t kDiskIoGetSectorCount = 1;
inline constexpr std::uint8_t kDiskIoGetSectorSize = 2;
inline constexpr std::uint8_t kDiskIoGetBlockSize = 3;
inline constexpr std::uint8_t kDiskResultOk = 0;
inline constexpr std::uint8_t kDiskResultParameterError = 4;

struct DiskIoParentDevice {
  std::uint64_t sector_count = 0; // target parent offset +0xb0
};

struct DiskIoDevice {
  DiskIoParentDevice* parent = nullptr; // target offset +0x18
  std::uint16_t sector_size = 0;        // target offset +0x20
};

using DiskIoDriveTable = std::array<DiskIoDevice*, 256>;

class DiskIoDiagnostics {
 public:
  virtual ~DiskIoDiagnostics() = default;
  virtual void PrintCommand(std::uint8_t command) = 0;
  virtual void PrintMissingDrive(std::uint8_t drive) = 0;
  virtual void DebugBreak() = 0;
};

// PeerPressure FatFs/Xbox disk_ioctl adapter at 0x801fe9f8.
std::uint8_t PeerDiskIoControl(std::uint32_t drive, std::uint32_t command,
                               void* output, const DiskIoDriveTable& drives,
                               DiskIoDiagnostics* diagnostics);

}  // namespace bandwagon::kernel_expansion
