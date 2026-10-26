#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kVersionQueryStatusSuccess = 0x00000000u;
inline constexpr std::uint32_t kVersionQueryStatusInvalidParameter =
    0xc000000du;
inline constexpr std::uint32_t kVersionQueryStatusDataError = 0xc000003eu;
inline constexpr std::uint32_t kVersionQueryStatusIoDeviceError = 0xc0000185u;

inline constexpr std::uint32_t kCurrentPeerPressureVersion = 0x010001ddu;
inline constexpr std::uint32_t kNandXbppMagic = 0x58425050u;
inline constexpr std::uint32_t kInvalidNandXbppRecordOffset = 0xffffffffu;
inline constexpr std::size_t kNandXbppRecordSize = 0x200;
inline constexpr std::string_view kFlashDevicePath = "\\Device\\Flash";

using FlashHandle = std::uintptr_t;

struct FlashOpenRequest {
  std::string_view path;
  std::uint32_t desired_access;
  std::uint32_t object_attributes;
  std::uint32_t file_attributes;
  std::uint32_t share_access;
  std::uint32_t create_disposition;
  std::uint32_t create_options;
};

class FlashVersionServices {
 public:
  virtual ~FlashVersionServices() = default;

  virtual std::uint32_t Open(const FlashOpenRequest& request,
                             FlashHandle* handle) = 0;

  // Returns the native numeric value produced by the target's byte-reversed
  // load from the flash-controller configuration register at 0x7feac000.
  virtual std::uint32_t ReadFlashConfiguration() = 0;

  virtual std::uint32_t Read(FlashHandle handle, std::uint64_t offset,
                             std::uint8_t* buffer, std::size_t size) = 0;
  virtual std::uint32_t Close(FlashHandle handle) = 0;
};

// Pure lift of the selector at 0x801f2f98. Bits 17..18 select one of three
// geometry rows and bits 4..5 select its offset. Row three is unsupported.
std::uint32_t ResolveNandXbppRecordOffset(
    std::uint32_t flash_configuration);

// Table-exposed lift of 0x801f1000. The installed version is read from the
// 512-byte XBPP record in NAND while the current version is compiled in.
std::uint32_t QueryPeerPressureVersionsFromFlash(
    FlashVersionServices* services, std::uint32_t* current_version,
    std::uint32_t* installed_version);

}  // namespace bandwagon::kernel_expansion
