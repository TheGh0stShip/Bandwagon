#pragma once

#include "softmod_manifest.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::updater {

struct SoftmodVersion {
  std::uint8_t major = 0;
  std::uint8_t minor = 0;
  std::uint16_t revision = 0;
};

struct WrappedUpdateArchive {
  SoftmodVersion version;
  std::vector<std::uint8_t> zip_data;
};

WrappedUpdateArchive ParseUpdateArchiveWrapper(const std::uint8_t* data,
                                               std::size_t size, Sha1 sha1);
std::vector<std::uint8_t> BuildUpdateArchiveWrapper(
    SoftmodVersion version, const std::uint8_t* zip_data,
    std::size_t zip_size, Sha1 sha1);

}  // namespace bandwagon::updater
