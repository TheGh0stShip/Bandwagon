#include "runtime.hpp"

#include <array>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

constexpr std::array<std::uint8_t, 12> kGptSignatureAndRevision{{
    'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T', 0x00, 0x00, 0x01, 0x00,
}};

std::uint32_t ReadLittleEndian32(const std::uint8_t* value) {
  return static_cast<std::uint32_t>(value[0]) |
         (static_cast<std::uint32_t>(value[1]) << 8) |
         (static_cast<std::uint32_t>(value[2]) << 16) |
         (static_cast<std::uint32_t>(value[3]) << 24);
}

}  // namespace

void LaunchBootXell(ModuleServices* services) {
  if (!services) return;

  ModuleHandle module = 0;
  if (services->LoadImage(kBootXellPath, kXexLoadFlags, 0, &module) == 0) {
    Procedure entry = 0;
    if (services->ResolveProcedure(module, kXexEntryOrdinal, &entry) == 0)
      services->InvokeProcedure(entry);
  }
  if (module != 0) services->UnloadImage(module);
}

std::uint32_t ComputeRecoveredGptChecksum(const std::uint8_t* data,
                                          std::size_t size) {
  if (!data && size != 0) return 0;
  std::uint32_t checksum = 0xffffffffu;
  for (std::size_t index = 0; index < size; ++index) {
    for (std::uint32_t mask = 1; mask < 0x100; mask <<= 1) {
      const std::uint32_t input_bit = (data[index] & mask) != 0 ? 1 : 0;
      const std::uint32_t mixed_bit = (checksum & 1) ^ input_bit;
      checksum >>= 1;
      if (mixed_bit != 0) checksum ^= 0xedb88320u;
    }
  }
  return ~checksum;
}

bool IsRecognizedGptHeader(const std::uint8_t* data, std::size_t size) {
  constexpr std::size_t kMinimumHeaderSize = 0x5c;
  constexpr std::size_t kMaximumHeaderSize = 0x200;
  constexpr std::size_t kHeaderSizeOffset = 0x0c;
  constexpr std::size_t kChecksumOffset = 0x10;
  constexpr std::size_t kPartitionCountOffset = 0x50;
  constexpr std::size_t kPartitionEntrySizeOffset = 0x54;

  if (!data || size < kMinimumHeaderSize ||
      std::memcmp(data, kGptSignatureAndRevision.data(),
                  kGptSignatureAndRevision.size()) != 0) {
    return false;
  }

  const std::uint32_t header_size =
      ReadLittleEndian32(data + kHeaderSizeOffset);
  if (header_size < kMinimumHeaderSize ||
      header_size > kMaximumHeaderSize || header_size > size) {
    return false;
  }
  if (ComputeRecoveredGptChecksum(data, header_size) !=
      ReadLittleEndian32(data + kChecksumOffset)) {
    return false;
  }
  if (ReadLittleEndian32(data + kPartitionEntrySizeOffset) != 0x80)
    return false;
  return ReadLittleEndian32(data + kPartitionCountOffset) <= 0x80;
}

bool IsAsciiAlphaNumeric(std::uint8_t value) {
  return (value >= '0' && value <= '9') ||
         (value >= 'A' && value <= 'Z') ||
         (value >= 'a' && value <= 'z');
}

}  // namespace bandwagon::kernel_expansion
