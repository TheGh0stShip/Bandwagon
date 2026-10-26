#include "rsa_public_key.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using bandwagon::stage4::ParseXeCryptRsaPublic2048;
  using bandwagon::stage4::XeCryptRsaPublic2048;
  using bandwagon::stage4::kXeCryptRsaPublic2048Size;

  std::vector<std::uint8_t> bytes(kXeCryptRsaPublic2048Size);
  bytes[3] = 0x20;
  bytes[7] = 3;
  bytes[0x10] = 0xc9;
  bytes[0x10f] = 0xbf;
  XeCryptRsaPublic2048 key;
  assert(ParseXeCryptRsaPublic2048(bytes.data(), bytes.size(), &key));
  assert(key.qword_count == 0x20);
  assert(key.public_exponent == 3);
  assert(key.reserved == 0);
  assert(key.modulus.front() == 0xc9 && key.modulus.back() == 0xbf);

  const auto before = key;
  bytes[3] = 0x10;
  assert(!ParseXeCryptRsaPublic2048(bytes.data(), bytes.size(), &key));
  assert(key.qword_count == before.qword_count);
  assert(!ParseXeCryptRsaPublic2048(bytes.data(), bytes.size() - 1, &key));
  assert(!ParseXeCryptRsaPublic2048(nullptr, bytes.size(), &key));

  std::cout << "Stage4 RSA-public-key reconstruction tests passed\n";
}
