#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kBootSectorSize = 512;

enum class BootSectorKind : std::uint32_t {
  kFat = 0,
  kExFat = 1,
  kProtectiveOrLegacyMbr = 2,
  kUnknown = 3,
  kReadError = 4,
};

std::string_view BootSectorKindName(BootSectorKind kind);

class SectorReader {
 public:
  virtual ~SectorReader() = default;
  virtual int ReadSector(
      std::uint64_t sector,
      std::array<std::uint8_t, kBootSectorSize>* output) = 0;
};

// Typed lift of ClassifyFatBootSector at 0x801fa320. Despite its historical
// FAT32-only label, the routine distinguishes FAT, exFAT, MBR/partition-table,
// unknown, and read-error outcomes.
BootSectorKind ClassifyFatBootSector(SectorReader* reader,
                                    std::uint64_t sector);

}  // namespace bandwagon::kernel_expansion
