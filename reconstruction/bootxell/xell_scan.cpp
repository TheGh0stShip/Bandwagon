#include "xell_scan.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

namespace bandwagon::bootxell {
namespace {

constexpr std::array<std::string_view, 3> kDangerousNames{
    "updxell.bin", "flashdmp.bin", "updflash.bin"};

}  // namespace

bool SanitizeDangerousXellNames(std::uint8_t* image, std::size_t size) {
  if (!image) return false;
  bool changed = false;
  for (std::size_t offset = 0; offset < size; ++offset) {
    for (const std::string_view name : kDangerousNames) {
      if (name.size() > size - offset ||
          std::memcmp(image + offset, name.data(), name.size()) != 0) {
        continue;
      }
      std::size_t start = offset;
      std::size_t length = name.size();
      if (start != 0 && image[start - 1] == '/') {
        --start;
        ++length;
      }
      std::fill(image + start, image + start + length, 0);
      image[start] = 1;
      changed = true;
      offset = start;
    }
  }
  return changed;
}

bool SanitizeXellImage(std::uint8_t* image, std::size_t size,
                       XellCompression* compression) {
  if (!image) return false;
  if (SanitizeDangerousXellNames(image, size)) return true;

  if (!compression || size < kXellGzipDataOffset + kXellScanMarkerSize)
    return false;
  if (image[kXellGzipHeaderOffset] != 0x1f ||
      image[kXellGzipHeaderOffset + 1] != 0x8b ||
      image[kXellGzipHeaderOffset + 2] != 8) {
    return false;
  }

  const std::size_t compressed_size =
      size - kXellGzipDataOffset - kXellScanMarkerSize;
  std::vector<std::uint8_t> expanded;
  if (!compression->Decompress(image + kXellGzipDataOffset, compressed_size,
                               kXellDecompressionLimit, &expanded) ||
      expanded.size() > kXellDecompressionLimit ||
      !SanitizeDangerousXellNames(expanded.data(), expanded.size())) {
    return false;
  }

  std::fill(image + kXellGzipDataOffset,
            image + kXellGzipDataOffset + compressed_size, 0);
  std::size_t output_capacity = compressed_size;
  return compression->Compress(expanded, image + kXellGzipDataOffset,
                               &output_capacity);
}

}  // namespace bandwagon::bootxell
