#include "archive_wrapper.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using bandwagon::updater::Sha1Digest;

static Sha1Digest TestDigest(const std::uint8_t* data, std::size_t size) {
  Sha1Digest result{};
  for (std::size_t i = 0; i < size; ++i) {
    result[(i * 7) % result.size()] ^= static_cast<std::uint8_t>(data[i] + i);
  }
  return result;
}

int main() {
  const std::vector<std::uint8_t> zip = {
      'P', 'K', 3, 4, 20, 0, 0, 0, 0, 0, 0xaa, 0xbb, 0xcc, 0xdd};
  const bandwagon::updater::SoftmodVersion version{1, 0, 477};
  const auto encoded = bandwagon::updater::BuildUpdateArchiveWrapper(
      version, zip.data(), zip.size(), TestDigest);
  assert(encoded.size() == 24 + zip.size());
  const auto decoded = bandwagon::updater::ParseUpdateArchiveWrapper(
      encoded.data(), encoded.size(), TestDigest);
  assert(decoded.version.major == 1);
  assert(decoded.version.minor == 0);
  assert(decoded.version.revision == 477);
  assert(decoded.zip_data == zip);
  assert(bandwagon::updater::BuildUpdateArchiveWrapper(
             decoded.version, decoded.zip_data.data(), decoded.zip_data.size(),
             TestDigest) == encoded);

  auto corrupt = encoded;
  corrupt.back() ^= 1;
  bool rejected = false;
  try {
    (void)bandwagon::updater::ParseUpdateArchiveWrapper(
        corrupt.data(), corrupt.size(), TestDigest);
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  assert(rejected);
  std::cout << "update archive wrapper reconstruction tests passed\n";
}
