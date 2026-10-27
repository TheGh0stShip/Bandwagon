#include "disk_ioctl.hpp"

#include <cstring>

namespace bandwagon::kernel_expansion {

std::uint8_t PeerDiskIoControl(std::uint32_t drive, std::uint32_t command,
                               void* output, const DiskIoDriveTable& drives,
                               DiskIoDiagnostics* diagnostics) {
  const std::uint8_t drive_index = static_cast<std::uint8_t>(drive);
  const std::uint8_t operation = static_cast<std::uint8_t>(command);
  diagnostics->PrintCommand(operation);

  const DiskIoDevice* const device = drives[drive_index];
  if (device == nullptr) {
    diagnostics->PrintMissingDrive(drive_index);
    diagnostics->DebugBreak();
    return kDiskResultParameterError;
  }

  switch (operation) {
    case kDiskIoControlSync:
      break;
    case kDiskIoGetSectorCount:
      std::memcpy(output, &device->parent->sector_count,
                  sizeof(device->parent->sector_count));
      break;
    case kDiskIoGetSectorSize:
      std::memcpy(output, &device->sector_size, sizeof(device->sector_size));
      break;
    case kDiskIoGetBlockSize: {
      constexpr std::uint32_t kBlockSize = 0x80;
      std::memcpy(output, &kBlockSize, sizeof(kBlockSize));
      break;
    }
    default:
      return kDiskResultParameterError;
  }
  return kDiskResultOk;
}

}  // namespace bandwagon::kernel_expansion
