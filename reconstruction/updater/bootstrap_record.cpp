#include "bootstrap_record.hpp"

#include <algorithm>
#include <stdexcept>

namespace bandwagon::updater {
namespace {

void StoreBigEndian32(std::vector<std::uint8_t>& output, std::size_t offset,
                      std::uint32_t value) {
  output[offset + 0] = static_cast<std::uint8_t>(value >> 24);
  output[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  output[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  output[offset + 3] = static_cast<std::uint8_t>(value);
}

}  // namespace

std::vector<std::uint8_t> BuildHddBootstrapRecord(
    const std::uint8_t* stage2, std::size_t stage2_size,
    std::uint32_t softmod_context_tag, Crc32 crc32) {
  if (!stage2 || !stage2_size || stage2_size > kMaximumStage2Size || !crc32) {
    throw std::invalid_argument("invalid Stage2 bootstrap input");
  }

  const std::size_t rounded_size =
      (stage2_size + kBootstrapSectorSize - 1) & ~(kBootstrapSectorSize - 1);
  std::vector<std::uint8_t> output(rounded_size, 0);
  std::copy(stage2, stage2 + stage2_size, output.begin());

  // The original computes this before replacing the first 16 payload bytes.
  const std::uint32_t checksum = crc32(0, output.data(), stage2_size);
  StoreBigEndian32(output, 0x00, 0x58425050);  // "XBPP"
  StoreBigEndian32(output, 0x04, softmod_context_tag);
  StoreBigEndian32(output, 0x08, static_cast<std::uint32_t>(stage2_size));
  StoreBigEndian32(output, 0x0c, checksum);
  return output;
}

}  // namespace bandwagon::updater
