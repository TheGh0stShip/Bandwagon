#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::bootxell {

inline constexpr std::size_t kXellGzipHeaderOffset = 0x4000;
inline constexpr std::size_t kXellGzipDataOffset = 0x400a;
inline constexpr std::size_t kXellScanMarkerSize = 16;
inline constexpr std::size_t kXellDecompressionLimit = 0x100000;

class XellCompression {
 public:
  virtual ~XellCompression() = default;
  virtual bool Decompress(const std::uint8_t* input, std::size_t input_size,
                          std::size_t output_limit,
                          std::vector<std::uint8_t>* output) = 0;
  virtual bool Compress(const std::vector<std::uint8_t>& input,
                        std::uint8_t* output,
                        std::size_t* output_capacity) = 0;
};

// Lift of 0x91a05190. Matching paths are zeroed; when the match is preceded
// by '/', that slash is included. The first cleared byte becomes sentinel 1.
bool SanitizeDangerousXellNames(std::uint8_t* image, std::size_t size);

// Lift of 0x91a05410. It first scans the image directly, then recognizes the
// gzip member at 0x4000, excludes the final 16-byte marker, scans the expanded
// content, and recompresses a changed member into the original capacity.
bool SanitizeXellImage(std::uint8_t* image, std::size_t size,
                       XellCompression* compression);

}  // namespace bandwagon::bootxell
