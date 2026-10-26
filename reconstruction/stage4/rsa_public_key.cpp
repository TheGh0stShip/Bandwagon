#include "rsa_public_key.hpp"

#include <algorithm>

namespace bandwagon::stage4 {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

std::uint64_t LoadBigEndian64(const std::uint8_t* input) {
  return (static_cast<std::uint64_t>(LoadBigEndian32(input)) << 32) |
         LoadBigEndian32(input + 4);
}

}  // namespace

bool ParseXeCryptRsaPublic2048(const std::uint8_t* data, std::size_t size,
                               XeCryptRsaPublic2048* output) {
  if (!data || !output || size != kXeCryptRsaPublic2048Size) return false;
  XeCryptRsaPublic2048 parsed;
  parsed.qword_count = LoadBigEndian32(data);
  if (parsed.qword_count != 0x20) return false;
  parsed.public_exponent = LoadBigEndian32(data + 4);
  parsed.reserved = LoadBigEndian64(data + 8);
  std::copy(data + 0x10, data + size, parsed.modulus.begin());
  *output = parsed;
  return true;
}

}  // namespace bandwagon::stage4
