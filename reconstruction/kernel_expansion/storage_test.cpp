#include "storage.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

void StoreLittleEndian16(std::uint8_t* output, std::uint16_t value) {
  output[0] = static_cast<std::uint8_t>(value);
  output[1] = static_cast<std::uint8_t>(value >> 8);
}

void StoreLittleEndian32(std::uint8_t* output, std::uint32_t value) {
  output[0] = static_cast<std::uint8_t>(value);
  output[1] = static_cast<std::uint8_t>(value >> 8);
  output[2] = static_cast<std::uint8_t>(value >> 16);
  output[3] = static_cast<std::uint8_t>(value >> 24);
}

class FakeReader final : public bandwagon::kernel_expansion::SectorReader {
 public:
  int result = 0;
  std::uint64_t expected_sector = 7;
  std::array<std::uint8_t, bandwagon::kernel_expansion::kBootSectorSize> data{};

  int ReadSector(
      std::uint64_t sector,
      std::array<std::uint8_t,
                 bandwagon::kernel_expansion::kBootSectorSize>* output)
      override {
    assert(sector == expected_sector && output);
    *output = data;
    return result;
  }
};

void AddSignature(FakeReader* reader) {
  StoreLittleEndian16(reader->data.data() + 0x1fe, 0xaa55);
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeReader exfat;
  constexpr std::uint8_t kExFatPrefix[] = {
      0xeb, 0x76, 0x90, 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' ',
  };
  std::memcpy(exfat.data.data(), kExFatPrefix, sizeof(kExFatPrefix));
  AddSignature(&exfat);
  assert(ClassifyFatBootSector(&exfat, 7) == BootSectorKind::kExFat);

  FakeReader fat32;
  fat32.data[0] = 0xeb;
  std::memcpy(fat32.data.data() + 0x52, "FAT32   ", 8);
  AddSignature(&fat32);
  assert(ClassifyFatBootSector(&fat32, 7) == BootSectorKind::kFat);

  FakeReader fat16;
  fat16.data[0] = 0xeb;
  StoreLittleEndian16(fat16.data.data() + 0x0b, 0x200);
  fat16.data[0x0d] = 8;
  StoreLittleEndian16(fat16.data.data() + 0x0e, 1);
  fat16.data[0x10] = 2;
  StoreLittleEndian16(fat16.data.data() + 0x11, 512);
  StoreLittleEndian16(fat16.data.data() + 0x13, 0x1000);
  StoreLittleEndian16(fat16.data.data() + 0x16, 16);
  AddSignature(&fat16);
  assert(ClassifyFatBootSector(&fat16, 7) == BootSectorKind::kFat);

  FakeReader large_fat;
  large_fat.data = fat16.data;
  StoreLittleEndian16(large_fat.data.data() + 0x13, 0x20);
  StoreLittleEndian32(large_fat.data.data() + 0x20, 0x10000);
  assert(ClassifyFatBootSector(&large_fat, 7) == BootSectorKind::kFat);

  FakeReader mbr;
  AddSignature(&mbr);
  assert(ClassifyFatBootSector(&mbr, 7) ==
         BootSectorKind::kProtectiveOrLegacyMbr);

  FakeReader unknown;
  assert(ClassifyFatBootSector(&unknown, 7) == BootSectorKind::kUnknown);

  FakeReader error;
  error.result = -1;
  assert(ClassifyFatBootSector(&error, 7) == BootSectorKind::kReadError);
  assert(ClassifyFatBootSector(nullptr, 7) == BootSectorKind::kReadError);
  assert(BootSectorKindName(BootSectorKind::kExFat) == "exFAT");

  std::cout << "Kernel Expansion storage reconstruction tests passed\n";
}
