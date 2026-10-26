#include "absolute_branch.hpp"

namespace bandwagon::aurora_fixer {
namespace {

void StoreBigEndian(std::array<std::uint8_t, 16>& output, std::size_t offset,
                    std::uint32_t value) {
  output[offset + 0] = static_cast<std::uint8_t>(value >> 24);
  output[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  output[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  output[offset + 3] = static_cast<std::uint8_t>(value);
}

}  // namespace

std::array<std::uint8_t, 16> EncodeAbsoluteBranch(std::uint32_t destination,
                                                  bool link) {
  std::array<std::uint8_t, 16> output{};
  std::uint32_t high = destination >> 16;
  if (destination & 0x8000) {
    ++high;  // addi sign-extends its low immediate.
  }
  StoreBigEndian(output, 0, 0x3d600000 | (high & 0xffff));
  StoreBigEndian(output, 4, 0x396b0000 | (destination & 0xffff));
  StoreBigEndian(output, 8, 0x7d6903a6);  // mtctr r11
  StoreBigEndian(output, 12, link ? 0x4e800421 : 0x4e800420);
  return output;
}

}  // namespace bandwagon::aurora_fixer
