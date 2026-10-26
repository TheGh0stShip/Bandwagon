#include "decimal.hpp"

#include <cstring>

namespace bandwagon::kernel_expansion {

std::int32_t ParseSignedDecimal(const char* text) {
  if (!text) return 0;

  bool negative = false;
  if (*text == '-') {
    negative = true;
    ++text;
  }

  std::uint32_t value = 0;
  while (*text >= '0' && *text <= '9') {
    value = value * 10u + static_cast<std::uint8_t>(*text - '0');
    ++text;
  }

  const std::uint32_t result_bits = negative ? 0u - value : value;
  std::int32_t result = 0;
  static_assert(sizeof(result) == sizeof(result_bits));
  std::memcpy(&result, &result_bits, sizeof(result));
  return result;
}

}  // namespace bandwagon::kernel_expansion
