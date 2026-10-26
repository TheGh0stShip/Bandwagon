#include "bootstrap_record.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

std::uint32_t ObservingCrc(std::uint32_t seed, const std::uint8_t* data,
                           std::size_t size) {
  assert(seed == 0);
  assert(size == 17);
  assert(data[0] == 0x10 && data[15] == 0x1f && data[16] == 0x20);
  return 0x12345678;
}

}  // namespace

int main() {
  std::vector<std::uint8_t> stage2(17);
  for (std::size_t index = 0; index < stage2.size(); ++index) {
    stage2[index] = static_cast<std::uint8_t>(0x10 + index);
  }
  const auto record = bandwagon::updater::BuildHddBootstrapRecord(
      stage2.data(), stage2.size(), 0xa1b2c3d4, ObservingCrc);
  assert(record.size() == 512);
  const std::uint8_t expected_header[] = {
      0x58, 0x42, 0x50, 0x50, 0xa1, 0xb2, 0xc3, 0xd4,
      0x00, 0x00, 0x00, 0x11, 0x12, 0x34, 0x56, 0x78};
  for (std::size_t index = 0; index < sizeof(expected_header); ++index) {
    assert(record[index] == expected_header[index]);
  }
  assert(record[16] == 0x20);
  for (std::size_t index = 17; index < record.size(); ++index) {
    assert(record[index] == 0);
  }

  bool rejected = false;
  try {
    bandwagon::updater::BuildHddBootstrapRecord(nullptr, 0, 0, ObservingCrc);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  assert(rejected);
}
