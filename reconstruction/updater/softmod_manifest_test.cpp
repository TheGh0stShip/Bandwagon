#include "softmod_manifest.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using bandwagon::updater::Sha1Digest;
using bandwagon::updater::SoftmodManifest;
using bandwagon::updater::SoftmodManifestEntry;

static Sha1Digest TestDigest(const std::uint8_t* data, std::size_t size) {
  Sha1Digest out{};
  for (std::size_t i = 0; i < size; ++i) {
    out[i % out.size()] = static_cast<std::uint8_t>(out[i % out.size()] + data[i] + i);
  }
  return out;
}

int main() {
  SoftmodManifest source;
  source.flags = 0x10203040;
  source.smc_major = 1;
  source.smc_minor = 1;
  source.softmod_major = 1;
  source.softmod_revision = 477;
  SoftmodManifestEntry first;
  first.flags = 1;
  first.path = "PeerPressure\\OtherOS\\boot.ini";
  first.sha1[0] = 0xdb;
  source.entries.push_back(first);
  SoftmodManifestEntry second;
  second.path = "Hdd\\PeerPressureExploit-Stage2.bin";
  second.sha1[19] = 0x75;
  source.entries.push_back(second);

  const auto encoded = bandwagon::updater::BuildSoftmodManifest(source, TestDigest);
  const auto decoded = bandwagon::updater::ParseSoftmodManifest(
      encoded.data(), encoded.size(), TestDigest);
  assert(decoded.flags == source.flags);
  assert(decoded.smc_major == 1 && decoded.smc_minor == 1);
  assert(decoded.softmod_revision == 477);
  assert(decoded.entries.size() == 2);
  assert(decoded.entries[0].flags == 1);
  assert(decoded.entries[0].path == first.path);
  assert(decoded.entries[0].sha1 == first.sha1);
  assert(decoded.entries[1].path == second.path);
  assert(bandwagon::updater::BuildSoftmodManifest(decoded, TestDigest) == encoded);

  auto corrupt = encoded;
  corrupt.back() ^= 1;
  bool rejected = false;
  try {
    (void)bandwagon::updater::ParseSoftmodManifest(corrupt.data(), corrupt.size(), TestDigest);
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  assert(rejected);
  std::cout << "softmod manifest reconstruction tests passed\n";
}
