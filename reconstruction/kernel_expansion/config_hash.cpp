#include "config_hash.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t ComputeConfigKeyHash(const char* text) {
  if (!text) return 0;

  std::uint32_t checksum = 0xffffffffu;
  while (*text != '\0') {
    std::uint8_t value = static_cast<std::uint8_t>(*text++);
    if (value >= 'a' && value <= 'z') value -= 'a' - 'A';
    checksum ^= value;
    for (int bit = 0; bit < 8; ++bit) {
      const std::uint32_t polynomial =
          (checksum & 1u) != 0 ? 0xedb88320u : 0;
      checksum = (checksum >> 1) ^ polynomial;
    }
  }
  return ~checksum;
}

}  // namespace bandwagon::kernel_expansion
