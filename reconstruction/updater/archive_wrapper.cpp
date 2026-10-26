#include "archive_wrapper.hpp"

#include <algorithm>
#include <stdexcept>

namespace bandwagon::updater {
namespace {

constexpr std::size_t kDigestSize = 20;
constexpr std::size_t kVersionSize = 4;
constexpr std::size_t kWrapperSize = kDigestSize + kVersionSize;
constexpr std::uint8_t kZipMagic[] = {'P', 'K', 3, 4};

void Require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

WrappedUpdateArchive ParseUpdateArchiveWrapper(const std::uint8_t* data,
                                               std::size_t size, Sha1 sha1) {
  Require(data && sha1 && size >= kWrapperSize + sizeof(kZipMagic),
          "invalid wrapped update archive");
  const Sha1Digest digest = sha1(data + kDigestSize, size - kDigestSize);
  Require(std::equal(digest.begin(), digest.end(), data),
          "update archive wrapper SHA-1 mismatch");
  Require(std::equal(std::begin(kZipMagic), std::end(kZipMagic),
                     data + kWrapperSize),
          "wrapped payload is not a ZIP archive");

  WrappedUpdateArchive result;
  result.version.major = data[20];
  result.version.minor = data[21];
  result.version.revision =
      static_cast<std::uint16_t>((data[22] << 8) | data[23]);
  result.zip_data.assign(data + kWrapperSize, data + size);
  return result;
}

std::vector<std::uint8_t> BuildUpdateArchiveWrapper(
    SoftmodVersion version, const std::uint8_t* zip_data,
    std::size_t zip_size, Sha1 sha1) {
  Require(zip_data && sha1 && zip_size >= sizeof(kZipMagic),
          "invalid update ZIP input");
  Require(std::equal(std::begin(kZipMagic), std::end(kZipMagic), zip_data),
          "update payload is not a ZIP archive");
  std::vector<std::uint8_t> output(kWrapperSize + zip_size, 0);
  output[20] = version.major;
  output[21] = version.minor;
  output[22] = static_cast<std::uint8_t>(version.revision >> 8);
  output[23] = static_cast<std::uint8_t>(version.revision);
  std::copy(zip_data, zip_data + zip_size, output.begin() + kWrapperSize);
  const Sha1Digest digest = sha1(output.data() + kDigestSize,
                                output.size() - kDigestSize);
  std::copy(digest.begin(), digest.end(), output.begin());
  return output;
}

}  // namespace bandwagon::updater
