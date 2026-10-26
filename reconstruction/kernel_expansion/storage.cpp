#include "storage.hpp"

#include <array>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

constexpr std::array<std::uint8_t, 11> kExFatPrefix{{
    0xeb, 0x76, 0x90, 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' ',
}};
constexpr std::array<std::uint8_t, 8> kFat32Label{{
    'F', 'A', 'T', '3', '2', ' ', ' ', ' ',
}};

std::uint16_t ReadLittleEndian16(const std::uint8_t* value) {
  return static_cast<std::uint16_t>(value[0]) |
         static_cast<std::uint16_t>(value[1] << 8);
}

std::uint32_t ReadLittleEndian32(const std::uint8_t* value) {
  return static_cast<std::uint32_t>(value[0]) |
         (static_cast<std::uint32_t>(value[1]) << 8) |
         (static_cast<std::uint32_t>(value[2]) << 16) |
         (static_cast<std::uint32_t>(value[3]) << 24);
}

bool IsPowerOfTwo(std::uint32_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

}  // namespace

std::string_view BootSectorKindName(BootSectorKind kind) {
  switch (kind) {
    case BootSectorKind::kFat: return "FAT";
    case BootSectorKind::kExFat: return "exFAT";
    case BootSectorKind::kProtectiveOrLegacyMbr:
      return "protective or legacy MBR";
    case BootSectorKind::kUnknown: return "unknown";
    case BootSectorKind::kReadError: return "read error";
  }
  return "unknown";
}

BootSectorKind ClassifyFatBootSector(SectorReader* reader,
                                    std::uint64_t sector_number) {
  if (!reader) return BootSectorKind::kReadError;
  std::array<std::uint8_t, kBootSectorSize> sector{};
  if (reader->ReadSector(sector_number, &sector) != 0)
    return BootSectorKind::kReadError;

  const std::uint16_t signature = ReadLittleEndian16(sector.data() + 0x1fe);
  if (signature == 0xaa55 &&
      std::memcmp(sector.data(), kExFatPrefix.data(), kExFatPrefix.size()) == 0) {
    return BootSectorKind::kExFat;
  }

  if ((sector[0] == 0xeb || sector[0] == 0xe9 || sector[0] == 0xe8) &&
      signature == 0xaa55 &&
      std::memcmp(sector.data() + 0x52, kFat32Label.data(),
                  kFat32Label.size()) == 0) {
    return BootSectorKind::kFat;
  }

  const std::uint16_t bytes_per_sector =
      ReadLittleEndian16(sector.data() + 0x0b);
  const std::uint8_t sectors_per_cluster = sector[0x0d];
  const std::uint16_t reserved_sectors =
      ReadLittleEndian16(sector.data() + 0x0e);
  const std::uint8_t fat_count = sector[0x10];
  const std::uint16_t root_entry_count =
      ReadLittleEndian16(sector.data() + 0x11);
  const std::uint16_t total_sectors_16 =
      ReadLittleEndian16(sector.data() + 0x13);
  const std::uint16_t fat_size_16 =
      ReadLittleEndian16(sector.data() + 0x16);

  const bool plausible_fat =
      IsPowerOfTwo(bytes_per_sector) && bytes_per_sector == 0x200 &&
      IsPowerOfTwo(sectors_per_cluster) && reserved_sectors != 0 &&
      (fat_count == 1 || fat_count == 2) && root_entry_count != 0 &&
      (total_sectors_16 >= 0x80 ||
       ReadLittleEndian32(sector.data() + 0x20) >= 0x10000);
  if (plausible_fat && fat_size_16 != 0) return BootSectorKind::kFat;

  return signature == 0xaa55 ? BootSectorKind::kProtectiveOrLegacyMbr
                             : BootSectorKind::kUnknown;
}

}  // namespace bandwagon::kernel_expansion
