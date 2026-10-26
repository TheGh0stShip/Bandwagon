#include "flash_versions.hpp"

#include <array>

namespace bandwagon::kernel_expansion {
namespace {

constexpr std::uint32_t kNandXbppRecordOffsets[3][4] = {
    {0x007be000u, 0x00f7c000u, 0x01efc000u, 0x03dfc000u},
    {0x00f7c000u, 0x00f7c000u, 0x03be0000u, 0x03bc0000u},
    {0x00f7c000u, 0x03dfc000u, 0x03be0000u, 0x03bc0000u},
};

std::uint32_t LoadBigEndian32(const std::uint8_t* bytes) {
  return (static_cast<std::uint32_t>(bytes[0]) << 24) |
         (static_cast<std::uint32_t>(bytes[1]) << 16) |
         (static_cast<std::uint32_t>(bytes[2]) << 8) |
         static_cast<std::uint32_t>(bytes[3]);
}

}  // namespace

std::uint32_t ResolveNandXbppRecordOffset(
    std::uint32_t flash_configuration) {
  const std::uint32_t row = (flash_configuration >> 17) & 3u;
  const std::uint32_t column = (flash_configuration >> 4) & 3u;
  if (row >= 3) return kInvalidNandXbppRecordOffset;
  return kNandXbppRecordOffsets[row][column];
}

std::uint32_t QueryPeerPressureVersionsFromFlash(
    FlashVersionServices* services, std::uint32_t* current_version,
    std::uint32_t* installed_version) {
  if (!services || !current_version || !installed_version) {
    return kVersionQueryStatusInvalidParameter;
  }

  *current_version = kCurrentPeerPressureVersion;
  *installed_version = 0;

  const FlashOpenRequest request{
      kFlashDevicePath,
      0xc0100000u,
      0x40u,
      0x80u,
      3u,
      1u,
      0x20u,
  };

  FlashHandle handle = 0;
  std::uint32_t status = services->Open(request, &handle);
  const auto close_handle = [&]() {
    if (handle != 0) (void)services->Close(handle);
  };
  if (status != kVersionQueryStatusSuccess) {
    close_handle();
    return status;
  }

  const std::uint32_t offset =
      ResolveNandXbppRecordOffset(services->ReadFlashConfiguration());
  if (offset == kInvalidNandXbppRecordOffset) {
    close_handle();
    return kVersionQueryStatusIoDeviceError;
  }

  std::array<std::uint8_t, kNandXbppRecordSize> record{};
  status = services->Read(handle, offset, record.data(), record.size());
  if (status != kVersionQueryStatusSuccess) {
    close_handle();
    return status;
  }

  if (LoadBigEndian32(record.data() + 0x110) != kNandXbppMagic) {
    close_handle();
    return kVersionQueryStatusDataError;
  }

  *installed_version = LoadBigEndian32(record.data() + 0x114);
  close_handle();
  return kVersionQueryStatusSuccess;
}

}  // namespace bandwagon::kernel_expansion
