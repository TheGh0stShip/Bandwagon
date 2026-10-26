#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bandwagon::stage4 {

constexpr std::size_t kXeCryptRsaPublic2048Size = 0x110;

struct XeCryptRsaPublic2048 {
  std::uint32_t qword_count = 0;
  std::uint32_t public_exponent = 0;
  std::uint64_t reserved = 0;
  std::array<std::uint8_t, 0x100> modulus{};
};

// Decodes the big-endian XECRYPT_RSAPUB_2048 object copied to 0x6000320b8.
// The parser requires the exact object size and its 0x20-qword width marker;
// it does not infer an owner or cryptographic purpose for the particular key.
bool ParseXeCryptRsaPublic2048(const std::uint8_t* data, std::size_t size,
                               XeCryptRsaPublic2048* output);

}  // namespace bandwagon::stage4
