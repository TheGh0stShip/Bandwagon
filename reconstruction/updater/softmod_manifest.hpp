#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bandwagon::updater {

using Sha1Digest = std::array<std::uint8_t, 20>;
using Sha1 = std::function<Sha1Digest(const std::uint8_t*, std::size_t)>;

struct SoftmodManifestEntry {
  Sha1Digest sha1{};
  std::uint32_t flags = 0;
  std::string path;
};

struct SoftmodManifest {
  std::uint32_t flags = 0;
  std::uint8_t smc_major = 0;
  std::uint8_t smc_minor = 0;
  std::uint16_t smc_reserved = 0;
  std::uint8_t softmod_major = 0;
  std::uint8_t softmod_minor = 0;
  std::uint16_t softmod_revision = 0;
  std::uint32_t entries_offset = 0x30;
  std::vector<SoftmodManifestEntry> entries;
};

SoftmodManifest ParseSoftmodManifest(const std::uint8_t* data,
                                     std::size_t size, Sha1 sha1);
std::vector<std::uint8_t> BuildSoftmodManifest(const SoftmodManifest& manifest,
                                               Sha1 sha1);

}  // namespace bandwagon::updater
